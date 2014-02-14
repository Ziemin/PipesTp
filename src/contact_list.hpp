#ifndef PIPE_CONTACT_LIST_HPP
#define PIPE_CONTACT_LIST_HPP

#include <QObject>
#include <TelepathyQt/ConnectionInterfaceContactListInterface>
#include <TelepathyQt/BaseConnection>
#include <atomic>
#include <map>
#include <vector>
#include <utility>

#include "pipe_exception.hpp"

typedef Tp::Client::ConnectionInterfaceContactListInterface ContactList;
typedef Tp::Client::ConnectionInterfaceContactsInterface ContactsIface;

enum class ContactListError {
    UNDEFINED,
    NOT_LOADED,
    NETWORK_ERROR,
    NOT_IMPLEMENTED,
    NOT_AVAILABLE,
    SERVICE_BUSY,
    NOT_YET,
    INVALID_HANDLE
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
        PipeContactList(
                ContactList *pipedList, 
                const Tp::BaseConnectionContactListInterfacePtr &contactListIface,
                const QString &contactListFileName, 
                const QStringList &attributeInterfaces);

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

        /**
         * @return piped handles for given identifiers
         * @throw ContactListException if there is no handle for at least on of identifiers
         */
        Tp::UIntList getHandlesFor(const QStringList &identifiers) const;

        /**
         * @return piped identifiers for given handles
         * @throw ContactListException if there is no identifier for at least on of handles
         */
        QStringList getIdentifiersFor(const Tp::UIntList &handles) const;

        /**
         * @return true if such handle exists in this contact list
         */
        bool hasHandle(uint handle) const;
        /**
         * @return true if such identifier exists in this contact list
         */
        bool hasIdentifier(const QString& identifier) const;

    private:
        void contactListStateChangedCb(uint newState);
        void contactsChangedWithIdCb(const Tp::ContactSubscriptionMap &changes, 
                const Tp::HandleIdentifierMap &identifiers, const Tp::HandleIdentifierMap &removals);
        void contactsChangedCb(const Tp::ContactSubscriptionMap &changes, const Tp::UIntList &removals);

        static QMap<uint, QString> loadFromFile(const QString &dirPath, const QString& fileName);
        static void saveToFile(const QString &dirPath, const QString &filename, const QMap<uint, QString> &pipedHandles);

    private:
        std::atomic_bool loaded;
        ContactList *pipedList;
        Tp::BaseConnectionContactListInterfacePtr contactListIface;
        QString dirPath;
        QString fileName;
        QStringList attributeInterfaces;
        Tp::ContactAttributesMap pipedAttrMap;
        QMap<uint, QString> pipedHandles;
};

#endif
