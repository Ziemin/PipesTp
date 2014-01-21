#include "connection.hpp"
#include "connection_utils.hpp"
#include "utils.hpp"
#include "simple_presence.hpp"

#include <TelepathyQt/PendingVariant>
#include <future>

PipeConnection::PipeConnection(
        const Tp::ConnectionPtr &pipedConnection,
        const PipePtr &pipe,
        const QDBusConnection &dbusConnection,
        const QString &cmName,
        const QString &protocolName,
        const QVariantMap &parameters) 
    : Tp::BaseConnection(dbusConnection, cmName, protocolName, parameters),
    pipedConnection(pipedConnection), pipe(pipe)
{

    pDebug() << "PipeConnection::PipeConnection: " << pipedConnection->objectPath();

    setConnectCallback(Tp::memFun(this, &PipeConnection::connectCb));
    setCreateChannelCallback(Tp::memFun(this, &PipeConnection::createChannelCb));
    setRequestHandlesCallback(Tp::memFun(this, &PipeConnection::requestHandlesCb));
    setInspectHandlesCallback(Tp::memFun(this, &PipeConnection::inspectHandlesCb));

    // check for interfaces and add if implemented
    QStringList supportedInterfaces;
    bool isContactsSupported = false, isContactListSupported = false;
    for(auto &interface: pipedConnection->interfaces()) {
        if(interface == TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS) {
            isContactsSupported = true;
            supportedInterfaces << interface;
        } else if(interface == TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST) {
            supportedInterfaces << interface;
            isContactListSupported = true;
            pDebug() << "Adding contact list interface to connection: " << objectPath();
            addContactListInterface();
        } else if(interface == TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE) {
            supportedInterfaces << interface;
            pDebug() << "Adding simple presence interface to connection: " << objectPath();
            addSimplePresenceInterface();
        } else if(interface == TP_QT_IFACE_CONNECTION_INTERFACE_ADDRESSING) {
            supportedInterfaces << interface;
            pDebug() << "Adding addressing interface to connection: " << objectPath();
            addAdressingInterface();
        } else if(interface == TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS) {
            supportedInterfaces << interface;
            pDebug() << "Adding requests interface to connection: " << objectPath();
            addRequestsInterface();
        }
    }

    if(isContactListSupported && isContactsSupported) {
        pDebug() << "Adding contacts interface to connection: " << objectPath();
        addContactsInterface(supportedInterfaces);
    }

    if(contactListPtr != nullptr && simplePresencePtr != nullptr) 
        simplePresencePtr->setContactList(contactListPtr.get());

    connect(pipedConnection.data(), &Tp::Connection::statusChanged,
            this, [this](Tp::ConnectionStatus pipedStatus) {
                // setting new status to with reason none specified due to incomplete implementation 
                // of signals in Connection class - TODO
                setStatus(pipedStatus, Tp::ConnectionStatusReasonNoneSpecified); 
            });
}

void PipeConnection::addContactsInterface(const QStringList &supportedInterfaces) {
    // TODO getContactByID not implemented in telepathy-qt
    Tp::BaseConnectionContactsInterfacePtr contactsIface = Tp::BaseConnectionContactsInterface::create();
    contactsIface->setGetContactAttributesCallback(Tp::memFun(this, &PipeConnection::getContactAttributesCb));
    contactsIface->setContactAttributeInterfaces(supportedInterfaces);

    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(contactsIface));
}

void PipeConnection::addContactListInterface() {

    Tp::BaseConnectionContactListInterfacePtr contactListIface = Tp::BaseConnectionContactListInterface::create();
    contactListIface->setGetContactListAttributesCallback(Tp::memFun(this, &PipeConnection::getContactListAttributesCb));
    contactListIface->setRequestSubscriptionCallback(Tp::memFun(this, &PipeConnection::requestSubscriptionCb));

    ContactList *pipedList = pipedConnection->interface<ContactList>();

    contactListPtr.reset(new PipeContactList(pipedList, contactListIface));
    // obtain contact list asynchronously
    std::async(std::launch::async, 
            [this, contactListIface] {
                try {
                    this->contactListPtr->loadContactList();
                } catch(PipeException<ContactListError> &e) { // it failed set it from here
                    contactListIface->setContactListState(Tp::ContactListStateFailure);
                    pWarning() << "Could not load contact list for connection: " << this->objectPath()
                        << " because of: " << e.what();
                }
            });

    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(contactListIface));
}

void PipeConnection::addSimplePresenceInterface() {

    Tp::BaseConnectionSimplePresenceInterfacePtr simplePresenceIface = Tp::BaseConnectionSimplePresenceInterface::create();
    simplePresenceIface->setSetPresenceCallback(Tp::memFun(this, &PipeConnection::setPresenceCb));

    Tp::Client::ConnectionInterfaceSimplePresenceInterface *pipedPresenceIface = 
        pipedConnection->interface<Tp::Client::ConnectionInterfaceSimplePresenceInterface>();

    simplePresencePtr.reset(new PipeSimplePresence(pipedPresenceIface, simplePresenceIface));

    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(simplePresenceIface));
}

void PipeConnection::addAdressingInterface() {

    Tp::BaseConnectionAddressingInterfacePtr addressingIface = Tp::BaseConnectionAddressingInterface::create();
    addressingIface->setGetContactsByURICallback(Tp::memFun(this, &PipeConnection::getContactsByURICb));
    addressingIface->setGetContactsByVCardFieldCallback(Tp::memFun(this, &PipeConnection::getContactsByVCardFieldCb));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(addressingIface));
}

void PipeConnection::addRequestsInterface() {

    Tp::BaseConnectionRequestsInterfacePtr requestsIface = Tp::BaseConnectionRequestsInterface::create(this);
    requestsIface->requestableChannelClasses << pipe->requestableChannelClasses(); 
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(requestsIface));
}

QString PipeConnection::uniqueName() const {
    return Tp::BaseConnection::uniqueName() + "_pipe_" + pipe->name();
}

Tp::ConnectionPtr PipeConnection::getPipedConnection() const {
    return pipedConnection;
}

bool PipeConnection::checkChannel(const Tp::Channel &channel) const {
    // TODO implement
    return false;
}

Tp::ChannelPtr PipeConnection::pipeChannel(const Tp::Channel &channel) const {
    // TODO implement
    return Tp::ChannelPtr();
}


Tp::BaseChannelPtr PipeConnection::createChannelCb(
        const QString &channelType, uint targetHandleType, uint targetHandle, Tp::DBusError *error) 
{
    // TODO implement 
    return Tp::BaseChannelPtr();
}

Tp::UIntList PipeConnection::requestHandlesCb(uint handleType, const QStringList &identifiers, Tp::DBusError *error) {
    // TODO implement
    return Tp::UIntList();
}

void PipeConnection::connectCb(Tp::DBusError *error) {

}

QStringList PipeConnection::inspectHandlesCb(uint handleType, const Tp::UIntList &handles, Tp::DBusError *error) {
    // TODO implement
    return QStringList();
}

Tp::ContactAttributesMap PipeConnection::getContactAttributesCb(
        const Tp::UIntList &handles, const QStringList &interfaces, Tp::DBusError *error) 
{
    try {
        // try again if failed previously
        if(!contactListPtr->isLoaded()) contactListPtr->loadContactList();
        return contactListPtr->getContactAttributes(handles, interfaces);
    } catch(PipeException<ContactListError> &e) {
        pWarning() << "Exception happened while getting contact attributes for connection: " 
            << objectPath() << " with message: " << e.what();
        setContactListDbusError(e, error);

        return Tp::ContactAttributesMap();
    }
}

Tp::ContactAttributesMap PipeConnection::getContactListAttributesCb(
        const QStringList &interfaces, bool /* hold */, Tp::DBusError *error) 
{
    try {
        // try again if failed previously
        if(!contactListPtr->isLoaded()) contactListPtr->loadContactList();
        return contactListPtr->getContactListAttributes(interfaces);
    } catch(PipeException<ContactListError> &e) {
        pWarning() << "Exception happened while getting contact attributes for connection: " 
            << objectPath() << " with message: " << e.what();
        setContactListDbusError(e, error);

        return Tp::ContactAttributesMap();
    }
}

void PipeConnection::requestSubscriptionCb(const Tp::UIntList &contacts, const QString &/* message */, Tp::DBusError *error) {

    try {
        // try again if failed previously
        if(!contactListPtr->isLoaded()) contactListPtr->loadContactList();
        contactListPtr->addToList(contacts);
    } catch(PipeException<ContactListError> &e) {
        pWarning() << "Exception happened while requesting subscription for connection: " 
            << objectPath() << " with message: " << e.what();
        setContactListDbusError(e, error);
    }
}

void PipeConnection::getContactsByVCardFieldCb(
        const QString &field, const QStringList &addresses, const QStringList &interfaces,
        Tp::AddressingNormalizationMap &addressingNormalizationMap, 
        Tp::ContactAttributesMap &contactAttributesMap, Tp::DBusError *error) 
{

}

void PipeConnection::getContactsByURICb(
        const QStringList &URIs, const QStringList &interfaces,
        Tp::AddressingNormalizationMap &addressingNormalizationMap, 
        Tp::ContactAttributesMap &contactAttributesMap, Tp::DBusError *error)
{

}

uint PipeConnection::setPresenceCb(const QString &status, const QString &statusMessage, Tp::DBusError *error) {

    try {
        simplePresencePtr->setPresence(status, statusMessage);
    } catch(SimplePresenceException &e) {
        pWarning() << "Exception happened while setting presence for connection: " 
            << objectPath() << " with message: " << e.what();
        setSimplePresenceDbusError(e, error);
    }
}

