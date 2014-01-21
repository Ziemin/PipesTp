#ifndef PIPE_CONTACT_LIST_HPP
#define PIPE_CONTACT_LIST_HPP

#include <QObject>
#include <TelepathyQt/ConnectionInterfaceContactListInterface>
#include <TelepathyQt/BaseConnection>
#include <atomic>

#include "pipe_exception.hpp"

typedef Tp::Client::ConnectionInterfaceContactListInterface ContactList;

enum class ContactListError {
    UNDEFINED,
    NOT_LOADED,
    NETWORK_ERROR,
    NOT_IMPLEMENTED,
    NOT_AVAILABLE,
    SERVICE_BUSY,
    NOT_YET
};

typedef PipeException<ContactListError> ContactListExeption;

/**
 * Class representing piped list which is stored in a file on disk
 */
class PipeContactList : public QObject {

    public:
        /**
         * @param {ContactList} pipedList contact lists which is piped by this list
         * @param {Tp::BaseConnectionContactListInterfacePtr} contactListiface contact list interface
         *          related to some PipeConnection
         */
        PipeContactList(ContactList *pipedList, const Tp::BaseConnectionContactListInterfacePtr &contactListIface);

        /**
         * @return true if list was loaded and properly initialized
         */
        bool isLoaded() const;
        /**
         * Loads serialized contact list 
         *
         * @throws ContactListException if error happens during the operation
         */
        void loadContactList();

        /**
         * @return attributes for all contacts provided in argument
         * @throws ContactListException if contact list has not been loaded
         */
        Tp::ContactAttributesMap getContactAttributes(
                const Tp::UIntList &handles, const QStringList &interfaces);

        /**
         * @return attributes for all contacts represented by this list
         * @throws ContactListException if contact list has not been loaded
         */
        Tp::ContactAttributesMap getContactListAttributes(const QStringList &interfaces);

        /**
         * Adds to contact list contacts with given ids. If a contact is not in the piped list it 
         * will not be added.
         *
         * @throws ContactListException when contact list has not been loaded
         * @return number of added contacts
         */
        int addToList(const Tp::UIntList &contacts);

    private:
        void contactListStateChangedCb(uint newState);
        void contactsChangedWithIdCb(const Tp::ContactSubscriptionMap &changes, 
                const Tp::HandleIdentifierMap &identifiers, const Tp::HandleIdentifierMap &removals);
        void contactsChangedCb(const Tp::ContactSubscriptionMap& changes, const Tp::UIntList& removals);

    private:
        std::atomic_bool loaded;
        ContactList *pipedList;
        Tp::BaseConnectionContactListInterfacePtr contactListIface;
};

#endif
