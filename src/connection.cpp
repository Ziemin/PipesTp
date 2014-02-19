#include "connection.hpp"
#include "connection_utils.hpp"
#include "utils.hpp"
#include "simple_presence.hpp"
#include "proxy_channel.hpp"

#include <TelepathyQt/PendingVariant>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/Connection>
#include <future>
#include <QEventLoop>

PipeConnection::PipeConnection(
        const Tp::ConnectionPtr &pipedConnection,
        const PipePtr &pipe,
        const QDBusConnection &dbusConnection,
        const QString &cmName,
        const QString &protocolName,
        const QVariantMap &parameters,
        const ConnectionAdditionalData& additionalData) 
    : Tp::BaseConnection(dbusConnection, cmName, protocolName, parameters),
    pipedConnection(pipedConnection), pipe(pipe)
{

    pDebug() << "PipeConnection::PipeConnection: " << pipedConnection->objectPath();

    setConnectCallback(Tp::memFun(this, &PipeConnection::connectCb));
    setCreateChannelCallback(Tp::memFun(this, &PipeConnection::createChannelCb));
    setRequestHandlesCallback(Tp::memFun(this, &PipeConnection::requestHandlesCb));
    setInspectHandlesCallback(Tp::memFun(this, &PipeConnection::inspectHandlesCb));

    setSelfHandle(pipedConnection->selfHandle());

    // check for interfaces and add if implemented
    QStringList supportedInterfaces;
    bool isContactsSupported = false, isContactListSupported = false;

    for(const QString &interface: pipedConnection->interfaces()) {
        if(interface == TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS) {
            isContactsSupported = true;
            supportedInterfaces << interface;
        } else if(interface == TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST) {
            supportedInterfaces << interface;
            isContactListSupported = true;
        } else if(interface == TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE) {
            supportedInterfaces << interface;
            pDebug() << "Adding simple presence interface to connection piping: " << pipedConnection->objectPath();
            addSimplePresenceInterface();
        } else if(interface == TP_QT_IFACE_CONNECTION_INTERFACE_ADDRESSING) {
            supportedInterfaces << interface;
            pDebug() << "Adding addressing interface to connection piping: " << pipedConnection->objectPath();
            addAdressingInterface();
        } else if(interface == TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS) {
            supportedInterfaces << interface;
            pDebug() << "Adding requests interface to connection piping: " << pipedConnection->objectPath();
            addRequestsInterface();
        }
    }

    // Assuming these both interfaces are always implemented at the same time
    if(isContactListSupported && isContactsSupported) {
        Tp::Client::DBus::PropertiesInterface propsIface(
                QDBusConnection::sessionBus(), pipedConnection->busName(), pipedConnection->objectPath());
        QDBusPendingReply<QDBusVariant> interfacesRep = propsIface.Get(
                TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS, "ContactAttributeInterfaces");
        interfacesRep.waitForFinished();

        if(interfacesRep.isValid()) {
            QStringList interfaces = interfacesRep.value().variant().value<QStringList>();
            pDebug() << "Adding contact list interface to connection piping: " << pipedConnection->objectPath();
            addContactListInterface(additionalData.contactListFileName, interfaces);
            pDebug() << "Adding contacts interface to connection piping: " << pipedConnection->objectPath();
            addContactsInterface(interfaces);
        } else {
            pWarning() << "Could not get list of ContactAttributeInterfaces";
        }

    }

    if(contactListPtr != nullptr && simplePresencePtr != nullptr) 
        simplePresencePtr->setContactList(contactListPtr.get());

    // lets set status to piped status
    setStatus(pipedConnection->status(), Tp::ConnectionStatusReasonNoneSpecified); 

    connect(pipedConnection.data(), &Tp::Connection::statusChanged,
            this, [this](Tp::ConnectionStatus pipedStatus) {
                // setting new status to with reason none specified due to incomplete implementation 
                // of signals in Connection class - TODO
                pDebug() << "piped connection changed status to: " << pipedStatus;
                setStatus(pipedStatus, Tp::ConnectionStatusReasonNoneSpecified); 
                if(pipedStatus == Tp::ConnectionStatus::ConnectionStatusDisconnected) {
                    emit disconnected();
                }
            });

    // connect to invalidated proxy signal
    connect(pipedConnection.data(), &Tp::Connection::invalidated,
            this, [this](Tp::DBusProxy* /* proxy */, const QString &errorName, const QString &errorMessage) {
                pDebug() << "piped connection proxy has been invalidated: error - > " << errorName
                         << " message - > " << errorMessage;
                setStatus(Tp::ConnectionStatus::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonNoneSpecified); 
                emit disconnected();
            });
}

void PipeConnection::addContactsInterface(const QStringList& interfaces) {
    // TODO getContactByID not implemented in telepathy-qt
    Tp::BaseConnectionContactsInterfacePtr contactsIface = Tp::BaseConnectionContactsInterface::create();
    contactsIface->setGetContactAttributesCallback(Tp::memFun(this, &PipeConnection::getContactAttributesCb));
    contactsIface->setContactAttributeInterfaces(interfaces);

    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(contactsIface));
}

void PipeConnection::addContactListInterface(const QString& contactListFilename, const QStringList& interfaces) {

    Tp::BaseConnectionContactListInterfacePtr contactListIface = Tp::BaseConnectionContactListInterface::create();
    contactListIface->setGetContactListAttributesCallback(Tp::memFun(this, &PipeConnection::getContactListAttributesCb));
    contactListIface->setRequestSubscriptionCallback(Tp::memFun(this, &PipeConnection::requestSubscriptionCb));
    contactListIface->setRemoveContactsCallback(Tp::memFun(this, &PipeConnection::removeContactsCb));

    ContactList *pipedList = pipedConnection->interface<ContactList>();

    contactListPtr.reset(new PipeContactList(pipedList, contactListIface, contactListFilename, interfaces));
    // obtain contact list asynchronously
    std::async(std::launch::async, 
            [this, contactListIface] {
                try {
                    this->contactListPtr->loadContactList();
                    setStatus(pipedConnection->status(), Tp::ConnectionStatusReasonNoneSpecified);
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

bool PipeConnection::checkChannelType(const QString &channelType) const {
    Tp::RequestableChannelClassList reqChanList = pipe->requestableChannelClasses();
    for(auto& cc: reqChanList) {
        if(cc.fixedProperties["org.freedesktop.Telepathy.Channel.ChannelType"].toString() == channelType)
            return true;
    }
    return false;
}

bool PipeConnection::checkHandleType(uint targetHandleType) const {
    // check if channel is directed to one of piped contacts
    // for now only simple contact interfaces are implemented
    // TODO group interface
    return targetHandleType == Tp::HandleTypeContact;
}

bool PipeConnection::checkTargetHandle(uint targetHandle) const {
    return (contactListPtr && contactListPtr->hasHandle(targetHandle));
}

bool PipeConnection::checkChannel(const Tp::Channel &channel) const {
    pDebug() << "Checking channel: type -> " << channel.channelType()
        << " handleType -> " << channel.targetHandleType()
        << " targetHandle -> " << channel.targetHandleType();

    if(!checkChannelType(channel.channelType())) return false;
    if(!checkHandleType(channel.targetHandleType())) return false;
    return checkTargetHandle(channel.targetHandle());
}

Tp::BaseChannelPtr PipeConnection::pipeChannel(Tp::ChannelPtr channel) {
    // TODO implement
    // for now lets not use pipe - just return the same guy
    return Tp::BaseChannelPtr::dynamicCast(PipeProxyChannel::create(this, channel));
}


Tp::BaseChannelPtr PipeConnection::createChannelCb(
        const QString &channelType, uint targetHandleType, uint targetHandle, Tp::DBusError *error) 
{
    pDebug() << "Creating channel for: channelType -> " << channelType 
        << " targetHandleType -> " << targetHandleType << " targetHandle -> " << targetHandle;

    if(!checkChannelType(channelType)) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, "Wrong channel type for this connection");
        return Tp::BaseChannelPtr();
    }
    if(!checkHandleType(targetHandleType)) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, "This handle type is not implemented in this connection");
        return Tp::BaseChannelPtr();
    }
    if(!checkTargetHandle(targetHandle)) {
        error->set(TP_QT_ERROR_INVALID_HANDLE, "No such handle in this connection");
        return Tp::BaseChannelPtr();
    }

    if(this->interface(TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS)) {
        pDebug() << "Getting channel from piped connection";
        Tp::Client::ConnectionInterfaceRequestsInterface *reqIface = 
            pipedConnection->interface<Tp::Client::ConnectionInterfaceRequestsInterface>();

        QString channelTypeProp = QString(TP_QT_IFACE_CHANNEL)+ QString(".ChannelType");
        QString targetHandleProp = QString(TP_QT_IFACE_CHANNEL)+ QString(".TargetHandle");
        QString targetHandleTypeProp = QString(TP_QT_IFACE_CHANNEL)+ QString(".TargetHandleType");

        // first check if such channel already exists, if not create it
        Tp::PendingVariant *pendingChans = reqIface->requestPropertyChannels();
        { // wait for operation to finish -- damn async methods!!!
            QEventLoop loop;
            QObject::connect(pendingChans, &Tp::PendingOperation::finished,
                    &loop, &QEventLoop::quit);
            loop.exec();
        }
        if(pendingChans->isValid()) {
            QDBusArgument dbusArg = pendingChans->result().value<QDBusArgument>();
            Tp::ChannelDetailsList chans;
            dbusArg >> chans;

            for(const Tp::ChannelDetails &cd: chans) {
                // found our channel
                if(cd.properties[channelTypeProp].toString() == channelType &&
                        cd.properties[targetHandleProp].toUInt() == targetHandle &&
                        cd.properties[targetHandleTypeProp].toUInt() == targetHandleType) 
                {
                    Tp::ChannelPtr chan = Tp::Channel::create(pipedConnection, cd.channel.path(), cd.properties);
                    Tp::PendingReady *pendingReady = chan->becomeReady();
                    { // wait for channel to become ready
                        QEventLoop loop;
                        QObject::connect(pendingReady, &Tp::PendingOperation::finished,
                                &loop, &QEventLoop::quit);
                        loop.exec();
                    }
                    return pipeChannel(chan);
                }
            }
        } else {
            pWarning() << "Invalid reply when getting list of channels: " << pendingChans->errorMessage();
            error->set(pendingChans->errorName(), pendingChans->errorMessage());
            return Tp::BaseChannelPtr();
        }

        // have to create a new channel for piped connection
        QVariantMap request;
        request[channelTypeProp] = QVariant(channelType);
        request[targetHandleProp] = QVariant(targetHandle);
        request[targetHandleTypeProp] = QVariant(targetHandleType);

        QDBusPendingReply<QDBusObjectPath, QVariantMap> newChanRep = reqIface->CreateChannel(request);

        newChanRep.waitForFinished();
        if(newChanRep.isValid()) {

            QDBusObjectPath objectPath = newChanRep.argumentAt<0>();
            QVariantMap props = newChanRep.argumentAt<1>();
            pDebug() << "Creating proxy for channel at: " << objectPath.path();
            return pipeChannel(Tp::Channel::create(pipedConnection, objectPath.path(), props));
        } else {

            pWarning() << "Invalid reply when creating channel: " << newChanRep.error();
            error->set(newChanRep.error().name(), newChanRep.error().message());
            return Tp::BaseChannelPtr();
        }
    } else {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, "Requests interface is not implemented");
        return Tp::BaseChannelPtr();
    }
}

void PipeConnection::connectCb(Tp::DBusError * /* error */) {
    // do nothing
}

Tp::UIntList PipeConnection::requestHandlesCb(uint handleType, const QStringList &identifiers, Tp::DBusError *error) {

    if(handleType == Tp::HandleType::HandleTypeContact) {
        try {
            return contactListPtr->getHandlesFor(identifiers);
        } catch(const ContactListExeption &e) {
            pWarning() << "Could not return handles for identifiers in: " << objectPath();
            setContactListDbusError(e, error);
        }
    }
    return Tp::UIntList();
}

QStringList PipeConnection::inspectHandlesCb(uint handleType, const Tp::UIntList &handles, Tp::DBusError *error) {

    if(handleType == Tp::HandleType::HandleTypeContact) {
        try {
            return contactListPtr->getIdentifiersFor(handles);
        } catch(const ContactListExeption &e) {
            pWarning() << "Could not return identifiers for handles in: " << objectPath();
            setContactListDbusError(e, error);
        }
    }
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

void PipeConnection::removeContactsCb(const Tp::UIntList &contacts, Tp::DBusError *error) {

    try {
        // try again if failed previously
        if(!contactListPtr->isLoaded()) contactListPtr->loadContactList();
        contactListPtr->remove(contacts);
    } catch(PipeException<ContactListError> &e) {
        pWarning() << "Exception happened while removing contacts for connection: " 
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
    return selfHandle();
}

