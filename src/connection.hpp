#ifndef PIPE_CONNECTION_HPP
#define PIPE_CONNECTION_HPP

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/Connection>
#include <TelepathyQt/BaseChannel>
#include <memory>

#include "types.hpp"
#include "contact_list.hpp"
#include "simple_presence.hpp"

struct ConnectionAdditionalData {
    QString contactListFileName;
};


class PipeConnection : public Tp::BaseConnection {


    public:

        PipeConnection(
                const Tp::ConnectionPtr &pipedConnection,
                const PipePtr &pipe,
                const QDBusConnection &dbusConnection,
                const QString &cmName,
                const QString &protocolName,
                const QVariantMap &parameters,
                const ConnectionAdditionalData& additionalData);

        virtual ~PipeConnection() = default;
        virtual QString uniqueName() const override;
        Tp::ConnectionPtr getPipedConnection() const;

        /**
         * Check if given channel has to be piped through this connection
         */
        bool checkChannel(const Tp::Channel &channel) const;
        /**
         * @param channel to be piped
         * @returns piped channel
         */
        Tp::ChannelPtr pipeChannel(const Tp::Channel &channel) const;

    private:

        Tp::BaseChannelPtr createChannelCb(
                const QString &channelType, uint targetHandleType, uint targetHandle, Tp::DBusError *error);

        Tp::UIntList requestHandlesCb(uint handleType, const QStringList &identifiers, Tp::DBusError *error);

        void connectCb(Tp::DBusError *error);

        QStringList inspectHandlesCb(uint handleType, const Tp::UIntList &handles, Tp::DBusError *error);

        Tp::ContactAttributesMap getContactAttributesCb(
                const Tp::UIntList &handles, const QStringList &interfaces, Tp::DBusError *error);

        Tp::ContactAttributesMap getContactListAttributesCb(
                const QStringList &interfaces, bool hold, Tp::DBusError *error);

        void requestSubscriptionCb(const Tp::UIntList &contacts, const QString &message, Tp::DBusError *error);

        void getContactsByVCardFieldCb(
                const QString &field, const QStringList &addresses, const QStringList &interfaces,
                Tp::AddressingNormalizationMap &addressingNormalizationMap, 
                Tp::ContactAttributesMap &contactAttributesMap, Tp::DBusError *error); 

        void getContactsByURICb(
                const QStringList &URIs, const QStringList &interfaces,
                Tp::AddressingNormalizationMap &addressingNormalizationMap, 
                Tp::ContactAttributesMap &contactAttributesMap, Tp::DBusError *error);

        uint setPresenceCb(const QString &status, const QString &statusMessage, Tp::DBusError *error);

        void pipePresenceChange(const Tp::SimpleContactPresences &contactPresences);

    private:

        void addContactsInterface(const QStringList& interfaces);
        void addContactListInterface(const QString& contactListFileName, const QStringList& interfaces);
        void addSimplePresenceInterface();
        void addAdressingInterface();
        void addRequestsInterface();

    private:

        Tp::ConnectionPtr pipedConnection;
        PipePtr pipe;
        std::unique_ptr<PipeContactList> contactListPtr;
        std::unique_ptr<PipeSimplePresence> simplePresencePtr;
};

typedef Tp::SharedPtr<PipeConnection> PipeConnectionPtr;

#endif
