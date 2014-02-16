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
        for(auto it = pipedAttrMap.constBegin(); it != pipedAttrMap.constEnd(); ++it) {
            auto idIt = (*it).find(QString(TP_QT_IFACE_CONNECTION) + "/contact-id");
            if(idIt != (*it).end()) {
                idMap[it.key()] = idIt->toString();
                revIdMap[idIt->toString()] = it.key();
            } else {
                pWarning() << "No id for handle: " << it.key();
            }
        }

        QSet<QString> serializedHandles = loadFromFile(dirPath, fileName);
        for(const QString& id: serializedHandles) {
            if(revIdMap.contains(id)) pipedContacts.insert(id);
            else pWarning() << "Could not find handle to pipe in contact list for id: (" << id << ")";
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
        for(auto it = idMap.constBegin(); it != idMap.constEnd(); ++it) {
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
        auto it = idMap.find(h);
        if(it == idMap.end())
            throw ContactListExeption(
                    "No such handle in contact list: " + std::to_string(h), ContactListError::INVALID_HANDLE);

        identifiers.append(*it);
    }
    return identifiers;
}

bool PipeContactList::hasHandle(uint handle) const {
    if(idMap.contains(handle)) 
        return pipedContacts.contains(idMap[handle]);
    return false;
}

bool PipeContactList::hasIdentifier(const QString& identifier) const {
    return pipedContacts.contains(identifier);
}

Tp::ContactAttributesMap PipeContactList::getContactAttributes(
            const Tp::UIntList &handles, const QStringList &/* interfaces */) 
{
    // TODO filter using interfaces
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
    for(const QString& p: pipedContacts) handles.append(revIdMap[p]);

    return getContactAttributes(handles, interfaces);
}

int PipeContactList::addToList(const Tp::UIntList &contacts) {
    if(!isLoaded()) 
        throw PipeException<ContactListError>("Contact list is not loaded", ContactListError::NOT_LOADED);

    Tp::ContactSubscriptionMap subChangeMap;
    Tp::HandleIdentifierMap newIdentifiers;
    int added = 0; 
    Tp::ContactSubscriptions subs;
    for(uint handle: contacts) {
        pDebug() << "Adding to contact list: " << handle;
        if(pipedAttrMap.contains(handle)) {

            auto it = pipedAttrMap[handle].find(QString(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST) + "/subscribe");
            if(it != pipedAttrMap[handle].end()) subs.subscribe = it.value().toUInt();
            else subs.subscribe = Tp::SubscriptionState::SubscriptionStateUnknown;

            it = pipedAttrMap[handle].find(QString(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST) + "/publish");
            if(it != pipedAttrMap[handle].end()) subs.publish = it.value().toUInt();
            else subs.publish = Tp::SubscriptionState::SubscriptionStateUnknown;

            it = pipedAttrMap[handle].find(QString(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST) + "/publish-request");
            if(it != pipedAttrMap[handle].end()) subs.publishRequest = it.value().toString();
            else subs.publishRequest = "";

            auto hIt = idMap.find(handle);
            if(hIt != idMap.end()) {
                newIdentifiers[handle] = *hIt;
                pipedContacts.insert(*hIt);
                subChangeMap[handle] = subs;
                added++;
            }
        } else {
            pWarning() << "No such handle: " << handle;
            throw ContactListExeption("No such handle: " + std::to_string(handle), ContactListError::INVALID_HANDLE);
        }
    }

    if(added) {
        pDebug() << "Added: " << added << " contacts to list";
        contactListIface->contactsChangedWithID(subChangeMap, newIdentifiers, Tp::HandleIdentifierMap());
        saveToFile(dirPath, fileName, pipedContacts);
    }

    return added;
}

QSet<QString> PipeContactList::loadFromFile(const QString &dirPath, const QString &fileName) {

    QSet<QString> deserialized;
    QFile inFile(dirPath + "/" + fileName);
    if(!inFile.exists()) return deserialized;

    inFile.open(QIODevice::ReadOnly);
    QDataStream is(&inFile);
    is >> deserialized;

    return deserialized;
}

void PipeContactList::saveToFile(const QString &dirPath, const QString &filename, const QSet<QString> &pipedHandles) {

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
        auto pIt = pipedContacts.find(it.value());
        if(pIt != pipedContacts.end()) {
            newRemovals[it.key()] = it.value();
            pipedContacts.erase(pIt);
        }
    }

    Tp::HandleIdentifierMap changeIdentifiers;
    Tp::ContactSubscriptionMap newChanges;
    for(auto it = identifiers.constBegin(); it != identifiers.constEnd(); ++it) {
        if(pipedContacts.find(it.value()) != pipedContacts.end()) {
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
            saveToFile(dirPath, fileName, pipedContacts);
    }
}

void PipeContactList::contactsChangedCb(const Tp::ContactSubscriptionMap& /* changes */, const Tp::UIntList& /* removals */) {
    // Documentation says to ignore this signal, anyway BaseContactListInterface does not implement  it
}

