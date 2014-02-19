#include "proxy_channel.hpp"
#include "utils.hpp"

#include <TelepathyQt/Channel>
#include <TelepathyQt/PendingVariant>
#include <TelepathyQt/PendingVariantMap>
#include <QObject>
#include <QtDBus>

// ------------ PipeProxyChannel --------------------------------------------------------------------------------
PipeProxyChannelPtr PipeProxyChannel::create(Tp::BaseConnection* connection, Tp::ChannelPtr underChan) {

    return PipeProxyChannelPtr(new PipeProxyChannel( QDBusConnection::sessionBus(), connection, underChan));
}

PipeProxyChannel::PipeProxyChannel(
        const QDBusConnection &dbusConnection, Tp::BaseConnection* connection, Tp::ChannelPtr underChan) 
    : Tp::BaseChannel(
            dbusConnection,
            connection,
            underChan->channelType(),
            underChan->targetHandle(),
            underChan->targetHandleType()),
    pipedChannel(underChan),
    pipedIface(QDBusConnection::sessionBus(), underChan->busName(), underChan->objectPath())
{
    // assuming this to interfaces are supported always at the same time
    if(underChan->channelType() == TP_QT_IFACE_CHANNEL_TYPE_TEXT &&
            underChan->hasInterface(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES)) 
    {
        Tp::BaseChannelTextTypePtr textTypePtr = addBaseChannelTextType();
        addBaseChannelMessagesInterface(textTypePtr);
    } 
    else if(underChan->channelType() == TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION) {
        addBaseChannelServerAuthenticationType();
    }

    // plug necessary interfaces
    for(const QString &iface: underChan->interfaces()) {
        if(iface == TP_QT_IFACE_CHANNEL_INTERFACE_GROUP) {
            addBaseChannelGroupInterface();
        } else if(iface == TP_QT_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION) {
            addBaseChannelCaptchaAuthenticationInterface();
        }
    }

    connect(&pipedIface, &Tp::Client::ChannelInterface::Closed, this, &PipeProxyChannel::closedCb);
}

PipeProxyChannel::~PipeProxyChannel() {
    if(pipedChannel && pipedChannel->isValid()) {
        pipedChannel->requestClose();
    }
}

void PipeProxyChannel::closedCb() {
    emit closed();
}

Tp::BaseChannelTextTypePtr PipeProxyChannel::addBaseChannelTextType() {

    Tp::Client::ChannelTypeTextInterface *pipedTextIface = pipedChannel->interface<Tp::Client::ChannelTypeTextInterface>();
    Tp::Client::ChannelInterfaceMessagesInterface *pipedMesIface = pipedChannel->interface<Tp::Client::ChannelInterfaceMessagesInterface>();
    Tp::BaseChannelTextTypePtr textTypePtr(new PipeChannelTextType(this, pipedTextIface, pipedMesIface));
    plugInterface(textTypePtr);

    return textTypePtr;
}

void PipeProxyChannel::addBaseChannelMessagesInterface(Tp::BaseChannelTextTypePtr textTypePtr) {

    Tp::Client::ChannelInterfaceMessagesInterface *pipedMesIface = pipedChannel->interface<Tp::Client::ChannelInterfaceMessagesInterface>();
    Tp::PendingVariantMap *pendingRep = pipedMesIface->requestAllProperties();
    { // wait for operation to finish
        QEventLoop loop;
        QObject::connect(pendingRep, &Tp::PendingOperation::finished,
                &loop, &QEventLoop::quit);
        loop.exec();
    }
    if(pendingRep->isValid()) {

        QVariantMap resMap = pendingRep->result();
        QStringList supportedContentTypes = resMap["SupportedContentTypes"].toStringList();
        Tp::UIntList messageTypes = resMap["MessageTypes"].value<Tp::UIntList>();
        uint supportedFlags = resMap["MessagePartSupportFlags"].toUInt();
        uint deliveryReportingSupport = resMap["DeliveryReportingSupport"].toUInt();

        Tp::BaseChannelMessagesInterfacePtr messagesPtr(
                new PipeChannelMessagesInterface(
                    textTypePtr.data(),
                    pipedMesIface,
                    supportedContentTypes,
                    messageTypes,
                    supportedFlags,
                    deliveryReportingSupport));

        plugInterface(messagesPtr);
    } else {
        pWarning() << "Could not get all properties of messages interface to pipe";
    }

}

void PipeProxyChannel::addBaseChannelServerAuthenticationType() {

    // TODO
    //Tp::BaseChannelServerAuthenticationTypePtr serverPtr(new PipeChannelServerAuthenticationType(this));
    //plugInterface(serverPtr);
}

void PipeProxyChannel::addBaseChannelCaptchaAuthenticationInterface() {

    // TODO
    //Tp::BaseChannelCaptchaAuthenticationInterfacePtr captchaPtr(new PipeChannelCaptchaAuthenticationInterface(this));
    //plugInterface(captchaPtr);
}

void PipeProxyChannel::addBaseChannelGroupInterface() {

    // TODO
    //Tp::BaseChannelGroupInterfacePtr groupPtr(new PipeChannelGroupInterface(this));
    //plugInterface(groupPtr);
}
        
// ------------ TextType ----------------------------------------------------------------------------------------
PipeChannelTextType::PipeChannelTextType(Tp::BaseChannel *chan,
        Tp::Client::ChannelTypeTextInterface *textIface,
        Tp::Client::ChannelInterfaceMessagesInterface *mesIface) 
    : Tp::BaseChannelTextType(chan), 
    textIface(textIface),
    mesIface(mesIface)
{
    setMessageAcknowledgedCallback(Tp::memFun(this, &PipeChannelTextType::messageAcknowledgedCb));

    Tp::PendingVariant *pendingRep = mesIface->requestPropertyPendingMessages();
    connect(pendingRep, &Tp::PendingVariant::finished,
            this, [this, pendingRep](Tp::PendingOperation* op) {
                if(op->isValid()) {
                    // get already collected messages
                    QDBusArgument dbusArg = pendingRep->result().value<QDBusArgument>();
                    Tp::MessagePartListList messages;
                    dbusArg >> messages;
                    for(const Tp::MessagePartList &mes: messages) 
                        this->mesageReceivedCb(mes);

                    // connect to new ones
                    this->connect(this->mesIface, &Tp::Client::ChannelInterfaceMessagesInterface::MessageReceived,
                        this, &PipeChannelTextType::mesageReceivedCb);
                } else {
                    pDebug() << "Error happened when getting list of pending messages: "
                            << op->errorName() << " -> " << op->errorMessage();
                }
            });
}

void PipeChannelTextType::messageAcknowledgedCb(QString token) {

    auto it = pendingTokenMap.find(token);
    if(it != pendingTokenMap.end()) {
        Tp::UIntList ids;
        ids.append(*it);
        pendingTokenMap.erase(it); // remove mapping
        textIface->AcknowledgePendingMessages(ids);
    } else {
        pWarning() << "Cannot acknowledge message with token: " << token;
    }
}

void PipeChannelTextType::mesageReceivedCb(const Tp::MessagePartList &newMessage) {

    if(!newMessage.empty()) {
        const Tp::MessagePart &header = newMessage.front();
        auto itToken = header.find(QLatin1String("message-token"));
        auto itId = header.find(QLatin1String("pending-message-id"));
        if(itToken != header.end() && itId != header.end()) {
            // add new mapping
            pendingTokenMap[itToken->variant().toString()] = itId->variant().toUInt();
            addReceivedMessage(newMessage);
        } else {
            pWarning() << "Received message has no message-token or pending-message-id";
        }
    } else {
        pDebug() << "Received an empty message";
    }
}

// ------------ MessagesInterface -------------------------------------------------------------------------------
PipeChannelMessagesInterface::PipeChannelMessagesInterface(
                Tp::BaseChannelTextType *chan,
                Tp::Client::ChannelInterfaceMessagesInterface *pipedMesIface,
                QStringList supportedContentTypes,
                Tp::UIntList messageTypes,
                uint messagePartSupportFlags,
                uint deliveryReportingSupport)
: Tp::BaseChannelMessagesInterface(chan, supportedContentTypes, messageTypes, messagePartSupportFlags, deliveryReportingSupport),
    pipedMesIface(pipedMesIface) 
{
    setSendMessageCallback(Tp::memFun(this, &PipeChannelMessagesInterface::sendMessageCb));
}

QString PipeChannelMessagesInterface::sendMessageCb(const Tp::MessagePartList &messages, uint flags, Tp::DBusError* error) {

    QDBusPendingReply<QString> pendingToken = pipedMesIface->SendMessage(messages, flags);
    pendingToken.waitForFinished();
    if(pendingToken.isValid()) {
        return pendingToken.value();
    } else {
        error->set(pendingToken.error().name(), pendingToken.error().message());
        return QString();
    }
}
