#ifndef PIPE_PROTOCOL_HPP
#define PIPE_PROTOCOL_HPP

#include <TelepathyQt/BaseProtocol>
#include <TelepathyQt/AccountManager>

#include "connection.hpp"
#include "types.hpp"


class PipeProtocol : public Tp::BaseProtocol {

    Q_OBJECT;
    Q_DISABLE_COPY(PipeProtocol)

    public:

        PipeProtocol(
                const QDBusConnection &dbusConnection, 
                const QString &name,
                PipePtr pipe,
                Tp::AccountManagerPtr amp);

    private:

        Tp::BaseConnectionPtr createConnection(const QVariantMap &parameters, Tp::DBusError *error);
        QString identifyAccount(const QVariantMap &parameters, Tp::DBusError *error);
        QString normalizeContact(const QString &contactId, Tp::DBusError *error);

    private:

        Tp::BaseProtocolAvatarsInterfacePtr avatarsIface;
        Tp::BaseProtocolPresenceInterfacePtr presenceIface;

        PipePtr pipe;
        Tp::AccountManagerPtr amp;
};

#endif
