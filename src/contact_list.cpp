#include "contact_list.hpp"
#include "pipe_exception.hpp"

PipeContactList::PipeContactList(
        ContactList *pipedList, 
        const Tp::BaseConnectionContactListInterfacePtr &contactListIface) 
    : loaded(false), pipedList(pipedList), contactListIface(contactListIface)
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
    // TODO implement
}

Tp::ContactAttributesMap PipeContactList::getContactAttributes(
            const Tp::UIntList &handles, const QStringList &interfaces) 
{
    if(!isLoaded()) 
        throw PipeException<ContactListError>("Contact list is not loaded", ContactListError::NOT_LOADED);

    // TODO implement
}

Tp::ContactAttributesMap PipeContactList::getContactListAttributes(const QStringList &interfaces) {

    if(!isLoaded()) 
        throw PipeException<ContactListError>("Contact list is not loaded", ContactListError::NOT_LOADED);

    // TODO implement
}

int PipeContactList::addToList(const Tp::UIntList &contacts) {
    if(!isLoaded()) 
        throw PipeException<ContactListError>("Contact list is not loaded", ContactListError::NOT_LOADED);

    // TODO implement
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
