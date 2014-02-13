#include "contact_list.hpp"
#include "pipe_exception.hpp"
#include "defines.hpp"
#include "utils.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <algorithm>

PipeContactList::PipeContactList(
        ContactList *pipedList, 
        const Tp::BaseConnectionContactListInterfacePtr &contactListIface,
        const QString &contactListFileName,
        const QStringList &attributeInterfaces) 
    : 
        loaded(false), pipedList(pipedList),
        contactListIface(contactListIface),
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

    // acquire a file with data
    struct passwd *pw = getpwuid(getuid());
    pathToContactList = QString(pw->pw_dir) + QString("/" TP_QT_PIPE_CONTACT_LISTS "/") + contactListFileName;

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
    } else {
        pWarning() << "Could not get attributes map of piped list";
    }
    
    std::vector<uint> serializedHandles = loadFromFile(pathToContactList);
    for(uint id: serializedHandles) {
        if(pipedAttrMap.contains(id)) pipedHandles.insert(id);
        else pWarning() << "Could not find handle to pipe in contact list for id: " << id;
    }
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
    std::for_each(begin(pipedHandles), end(pipedHandles), [&handles](uint handle){ handles.append(handle); });

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
            if(it != pipedAttrMap[handle].end()) newIdentifiers[handle] = it.value().toString();

            pipedHandles.insert(handle);
            added++;
        }

    contactListIface->contactsChangedWithID(subChangeMap, newIdentifiers, Tp::HandleIdentifierMap());
    return added;
}

std::vector<uint> PipeContactList::loadFromFile(const QString &fileName) {
    // TODO implement
    return std::vector<uint>();
}

void PipeContactList::contactListStateChangedCb(uint newState) {
    // TODO implement
}

void PipeContactList::contactsChangedWithIdCb(const Tp::ContactSubscriptionMap &changes,
        const Tp::HandleIdentifierMap &identifiers, const Tp::HandleIdentifierMap &removals) 
{
    // TODO implement
}

void PipeContactList::contactsChangedCb(const Tp::ContactSubscriptionMap& changes, const Tp::UIntList& removals) {
    // TODO implement
}
