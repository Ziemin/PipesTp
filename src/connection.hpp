#ifndef PIPE_CONNECTION_HPP
#define PIPE_CONNECTION_HPP

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/Connection>
#include <TelepathyQt/BaseChannel>
#include <memory>

#include "types.hpp"


class PipeConnection : public Tp::BaseConnection {

    protected:

        PipeConnection(
                const Tp::ConnectionPtr &pipedConnection,
                const PipePtr &pipe,
                const QDBusConnection &dbusConnection,
                const QString &cmName,
                const QString &protocolName,
                const QVariantMap &parameters);

    public:

        virtual ~PipeConnection() = default;
        virtual QString uniqueName() const override;

    private:

        Tp::BaseChannelPtr createChannelCb(
                const QString &channelType, uint targetHandleType, uint targetHandle, DBusError *error);

        Tp::UIntList requestHandlesCb(uint handleType, const QStringList &identifiers, DBusError *error);
        void connectCb(DBusError *error);
        QStringList inspectHandlesCb(uint handleType, const Tp::UIntList &handles, DBusError *error);

    private:

        Tp::ConnectionPtr pipedConnection;
        PipePtr pipe;
};


class PipeConnectionContactsInterface : public Tp::BaseConnectionContactsInterface {

    public:

        PipeConnectionContactsInterface(const Tp::ConnectionPtr &pipedConnection);
        virtual ~PipeConnectionContactsInterface() = default;

    private:

        Tp::ContactAttributesMap getContactAttributesCb(
                const Tp::UIntList &handles, const QStringList &interfaces, DBusError *error);

    private:

        Tp::ConnectionPtr pipedConnection;
};


class PipeConnectionSimplePresenceInterface : public Tp::BaseConnectionSimplePresenceInterface {

    public:

        PipeConnectionSimplePresenceInterface(const Tp::ConnectionPtr &pipedConnection);
        virtual ~PipeConnectionSimplePresenceInterface() = default;

    private:

        uint setPresenceCb(const QString &status, const QString &statusMessage, DBusError *error);

    private:

        Tp::ConnectionPtr pipedConnection;
};


class PipeConnectionContactListInterface : public Tp::BaseConnectionContactListInterface {

    public:

        PipeConnectionContactListInterface(const Tp::ConnectionPtr &pipedConnection);
        virtual ~PipeConnectionContactListInterface() = default;

    private:

        Tp::ContactAttributesMap setGetContactListAttributesCb(
                const QStringList &interfaces, bool hold, DBusError *error);

        void setRequestSubscriptionCb(const Tp::UIntList &contacts, const QString &message, DBusError *error);

    private:

        Tp::ConnectionPtr pipedConnection;
};


class PipeConnectionAddressingInterface : public Tp::BaseConnectionAddressingInterface {

    public:

        PipeConnectionAddressingInterface(const Tp::ConnectionPtr &pipedConnection);
        virtual ~PipeConnectionAddressingInterface() = default;

    private:

        void getContactsByVCardFieldCb(
                const QString &field, const QStringList &addresses, const QStringList &interfaces,
                Tp::AddressingNormalizationMap &addressingNormalizationMap, 
                Tp::ContactAttributesMap &contactAttributesMap, DBusError *error); 

        void getContactsByURICb(
                const QStringList &URIs, const QStringList &interfaces,
                Tp::AddressingNormalizationMap &addressingNormalizationMap, 
                Tp::ContactAttributesMap &contactAttributesMap, DBusError *error);

    private:

        Tp::ConnectionPtr pipedConnection;

};

typedef Tp::SharedPtr<PipeConnection> PipeConnectionPtr;
typedef Tp::SharedPtr<PipeConnectionSimplePresenceInterface> PipeConnectionSimplePresenceInterfacePtr;
typedef Tp::SharedPtr<PipeConnectionContactListInterface> PipeConnectionContactListInterfacePtr;
typedef Tp::SharedPtr<PipeConnectionAddressingInterface> PipeConnectionAddressingInterfacePtr;

#endif
