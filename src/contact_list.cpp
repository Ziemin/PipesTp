#include "contact_list.hpp"
#include "pipe_exception.hpp"
#include "defines.hpp"
#include "utils.hpp"

#include <algorithm>
#include <QDataStream>

PipeContactList::PipeContactList(
        ContactList *pipedList, 
        const Tp::BaseConnectionContactListInterfacePtr &contactListIface,
        const QString &contactListFileName,
        const QStringList &attributeInterfaces) 
    : 
        loaded(false), pipedList(pipedList),
        contactListIface(contactListIface),
        fileName(contactListFileName),
        attributeInterfaces(attributeInterfaces)
{
    // users are added to the list in order to pipe their connections
    contactListIface->setCanChangeContactList(true); 
    // contact list will be obtained as soon as connection is created - TODO contacts have to be kept in some file
    contactListIface->setDownloadAtConnection(true); 
    // solution for now - TODO implement it to work with different cases
    contactListIface->setContactListPersists(true);
    // no needed when only piping
    contactListIface->setRequestUsesMessage(false);
    // trying to acquire contact list state 
    contactListIface->setContactListState(Tp::ContactListState::ContactListStateNone);

    dirPath = QDir::homePath() + QString("/" TP_QT_PIPE_CONTACT_LISTS);

    connect(pipedList, &ContactList::ContactListStateChanged, this, &PipeContactList::contactListStateChangedCb);
    connect(pipedList, &ContactList::ContactsChangedWithID, this, &PipeContactList::contactsChangedWithIdCb);
    connect(pipedList, &ContactList::ContactsChanged, this, &PipeContactList::contactsChangedCb);
}

bool PipeContactList::isLoaded() const {
    return loaded;
}


void PipeContactList::loadContactList() {
    if(loaded) return;

    contactListIface->setContactListState(Tp::ContactListStateWaiting);

    QDBusPendingReply<Tp::ContactAttributesMap> attrMapRep = pipedList->GetContactListAttributes(attributeInterfaces, false);
    attrMapRep.waitForFinished();

    if(attrMapRep.isValid()) {
        pipedAttrMap = attrMapRep.value();

        QMap<uint, QString> serializedHandles = loadFromFile(dirPath, fileName);
        for(auto it = serializedHandles.constBegin(); it != serializedHandles.constEnd(); ++it) {
            if(pipedAttrMap.contains(it.key())) pipedHandles[it.key()] = it.value();
            else pWarning() << "Could not find handle to pipe in contact list for: (" << it.key()
                << ", " << it.value() << ")";
        }

        loaded = true;
        contactListIface->setContactListState(Tp::ContactListState::ContactListStateSuccess);

    } else {
        pWarning() << "Could not get attributes map of piped list";
    }
}

Tp::UIntList PipeContactList::getHandlesFor(const QStringList &identifiers) const {
    // Maybe add reverse mapping in future, now lets stay with this inefficient solution
    Tp::UIntList handles;
    for(const QString &id: identifiers) {
        bool found = false;
        for(auto it = pipedHandles.constBegin(); it != pipedHandles.constEnd(); ++it) {
            if(it.value() == id) {
                found = true;
                handles.append(it.key());
            }
        }
        if(!found) throw ContactListExeption(
                "No handle for identifier: " + id.toStdString(), ContactListError::INVALID_HANDLE);
    }
    return handles;
}

QStringList PipeContactList::getIdentifiersFor(const Tp::UIntList &handles) const {

    QStringList identifiers;
    for(uint h: handles) {
        auto it = pipedHandles.find(h);
        if(it == pipedHandles.end())
            throw ContactListExeption(
                    "No such handle in contact list: " + std::to_string(h), ContactListError::INVALID_HANDLE);

        identifiers.append(*it);
    }
    return identifiers;
}

bool PipeContactList::hasHandle(uint handle) const {
    return pipedHandles.contains(handle);
}

bool PipeContactList::hasIdentifier(const QString& identifier) const {
    for(const QString& s: pipedHandles) 
        if(s == identifier) return true;

    return false;
}

Tp::ContactAttributesMap PipeContactList::getContactAttributes(
            const Tp::UIntList &handles, const QStringList &/* interfaces */) 
{
    if(!isLoaded()) 
        throw PipeException<ContactListError>("Contact list is not loaded", ContactListError::NOT_LOADED);

    Tp::ContactAttributesMap attrsToReturn;
    for(uint handle: handles) {
        auto it = pipedAttrMap.find(handle);
        if(it != pipedAttrMap.end()) 
            attrsToReturn[handle] = it.value();
    }

    return attrsToReturn;
}

Tp::ContactAttributesMap PipeContactList::getContactListAttributes(const QStringList &interfaces) {

    if(!isLoaded()) 
        throw PipeException<ContactListError>("Contact list is not loaded", ContactListError::NOT_LOADED);

    Tp::UIntList handles;
    for(uint p: pipedHandles.keys()) handles.append(p);

    return getContactAttributes(handles, interfaces);
}

int PipeContactList::addToList(const Tp::UIntList &contacts) {
    if(!isLoaded()) 
        throw PipeException<ContactListError>("Contact list is not loaded", ContactListError::NOT_LOADED);

    Tp::ContactSubscriptionMap subChangeMap;
    Tp::HandleIdentifierMap newIdentifiers;
    int added = 0; 
    Tp::ContactSubscriptions subs;
    for(uint handle: contacts) 
        if(!pipedAttrMap.contains(handle)) {

            auto it = pipedAttrMap[handle].find(QString(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST) + "/subscribe");
            if(it != pipedAttrMap[handle].end()) subs.subscribe = it.value().toUInt();
            else subs.subscribe = Tp::SubscriptionState::SubscriptionStateUnknown;

            it = pipedAttrMap[handle].find(QString(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST) + "/publish");
            if(it != pipedAttrMap[handle].end()) subs.publish = it.value().toUInt();
            else subs.publish = Tp::SubscriptionState::SubscriptionStateUnknown;

            it = pipedAttrMap[handle].find(QString(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST) + "/publish-request");
            if(it != pipedAttrMap[handle].end()) subs.publishRequest = it.value().toString();
            else subs.publishRequest = "";

            it = pipedAttrMap[handle].find(QString(TP_QT_IFACE_CONNECTION) + "/contact-id");

            if(it != pipedAttrMap[handle].end()) {
                newIdentifiers[handle] = it.value().toString();
                pipedHandles[handle] = it.value().toString();
                subChangeMap[handle] = subs;
                added++;
            }
        }

    if(added) {
        contactListIface->contactsChangedWithID(subChangeMap, newIdentifiers, Tp::HandleIdentifierMap());
        saveToFile(dirPath, fileName, pipedHandles);
    }

    return added;
}

QMap<uint, QString> PipeContactList::loadFromFile(const QString &dirPath, const QString &fileName) {

    QMap<uint, QString> deserialized;
    QFile inFile(dirPath + "/" + fileName);
    if(!inFile.exists()) return deserialized;

    inFile.open(QIODevice::ReadOnly);
    QDataStream is(&inFile);
    is >> deserialized;

    return deserialized;
}

void PipeContactList::saveToFile(const QString &dirPath, const QString &filename, const QMap<uint, QString> &pipedHandles) {

    QDir dir(dirPath);
    if(!dir.exists()) {
        if(!dir.mkpath(dirPath)) {
            pCritical() << "Cannot create path for contact list file to save: " 
                << "path: " << dirPath << " file: " << filename;
            return;
        }
    }
    QFile outFile(dirPath + "/" + filename);
    outFile.open(QIODevice::WriteOnly);
    QDataStream os(&outFile);
    os << pipedHandles;
}

void PipeContactList::contactListStateChangedCb(uint newState) {
    if(!loaded && newState == Tp::ContactListState::ContactListStateSuccess) loadContactList();
    contactListIface->setContactListState(newState);
}

void PipeContactList::contactsChangedWithIdCb(const Tp::ContactSubscriptionMap &changes,
        const Tp::HandleIdentifierMap &identifiers, const Tp::HandleIdentifierMap &removals) 
{
    Tp::HandleIdentifierMap newRemovals;
    for(auto it = removals.constBegin(); it != removals.constEnd(); ++it) {
        auto pIt = pipedHandles.find(it.key());
        if(pIt != pipedHandles.end()) {
            newRemovals[it.key()] = it.value();
            pipedHandles.erase(pIt);
        }
    }

    Tp::HandleIdentifierMap changeIdentifiers;
    Tp::ContactSubscriptionMap newChanges;
    for(auto it = identifiers.constBegin(); it != identifiers.constEnd(); ++it) {
        if(pipedHandles.find(it.key()) != pipedHandles.end()) {
            changeIdentifiers[it.key()] = it.value();
            newChanges[it.key()] = changes[it.key()];
        }
        Tp::ContactSubscriptions subs = changes[it.key()];
        pipedAttrMap[it.key()]
            [QString(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST) + "/subscribe"] = subs.subscribe;
        pipedAttrMap[it.key()]
            [QString(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST) + "/publish"] = subs.publish;
        pipedAttrMap[it.key()]
            [QString(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST) + "/publish-request"] = subs.publishRequest;
    }

    if(!(newChanges.empty() && changeIdentifiers.empty() && newRemovals.empty())) {
        contactListIface->contactsChangedWithID(newChanges, changeIdentifiers, newRemovals);
        if(!newRemovals.empty()) 
            saveToFile(dirPath, fileName, pipedHandles);
    }
}

void PipeContactList::contactsChangedCb(const Tp::ContactSubscriptionMap& /* changes */, const Tp::UIntList& /* removals */) {
    // Documentation says to ignore this signal, anyway BaseContactListInterface does not implement  it
}

