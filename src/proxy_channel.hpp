#ifndef PIPE_PROXY_CHANNEL_HPP
#define PIPE_PROXY_CHANNEL_HPP

#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/ChannelInterface>

class PipeProxyChannel;
typedef Tp::SharedPtr<PipeProxyChannel> PipeProxyChannelPtr;

class PipeProxyChannel : public Tp::BaseChannel {

    public:
        static PipeProxyChannelPtr create(Tp::BaseConnection* connection, Tp::ChannelPtr underChan);
        virtual ~PipeProxyChannel();

    protected:
        PipeProxyChannel(const QDBusConnection &dbusConnection, Tp::BaseConnection* connection, Tp::ChannelPtr underChan);

    private:
        Tp::BaseChannelTextTypePtr addBaseChannelTextType();
        void addBaseChannelMessagesInterface(Tp::BaseChannelTextTypePtr textTypePtr);
        void addBaseChannelServerAuthenticationType();
        void addBaseChannelCaptchaAuthenticationInterface();
        void addBaseChannelGroupInterface();

        void closedCb();

    private:
        Tp::ChannelPtr pipedChannel;
        Tp::Client::ChannelInterface pipedIface;
};

class PipeChannelTextType : public Tp::BaseChannelTextType {

    public:
        PipeChannelTextType(Tp::BaseChannel *chan, 
                Tp::Client::ChannelTypeTextInterface *textIface, 
                Tp::Client::ChannelInterfaceMessagesInterface *mesIface);

    private:
        void messageAcknowledgedCb(QString);
        void mesageReceivedCb(const Tp::MessagePartList &newMessage);

    private:
        Tp::Client::ChannelTypeTextInterface *textIface;
        Tp::Client::ChannelInterfaceMessagesInterface *mesIface;
        QMap<QString, uint> pendingTokenMap;
};

class PipeChannelMessagesInterface : public Tp::BaseChannelMessagesInterface {

    public:
        PipeChannelMessagesInterface(
                Tp::BaseChannelTextType *chan,
                Tp::Client::ChannelInterfaceMessagesInterface *pipedMesIface,
                QStringList supportedContentTypes,
                Tp::UIntList messageTypes,
                uint messagePartSupportFlags,
                uint deliveryReportingSupport);

    private:
        QString sendMessageCb(const Tp::MessagePartList &messages, uint flags, Tp::DBusError* error);

    private:
        Tp::Client::ChannelInterfaceMessagesInterface *pipedMesIface;

};

class PipeChannelServerAuthenticationType : public Tp::BaseChannelServerAuthenticationType {

    public:
        PipeChannelServerAuthenticationType(Tp::BaseChannel *chan);
};

class PipeChannelCaptchaAuthenticationInterface : public Tp::BaseChannelCaptchaAuthenticationInterface {

    public:
        PipeChannelCaptchaAuthenticationInterface(Tp::BaseChannel *chan);

    private:
        void getCaptchasCb(Tp::CaptchaInfoList&, uint&, QString&, DBusError*);
        QByteArray getCaptchaDataCb(uint, const QString&, DBusError*);
        void answerCaptchasCb(const Tp::CaptchaAnswers&, DBusError*);
        void cancelCaptchaCb(const QString&, DBusError*);
};

class PipeChannelGroupInterface : public Tp::BaseChannelGroupInterface {

    public:
        PipeChannelGroupInterface(Tp::BaseChannel *chan);

    private:
        void removeMembersCb(const Tp::UIntList&, const QString&, DBusError*);
        void setAddMembersCallback(const Tp::UIntList&, const QString&, DBusError*);
};

#endif
