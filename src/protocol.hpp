#ifndef PIPE_PROTOCOL_HPP
#define PIPE_PROTOCOL_HPP

#include <TelepathyQt/BaseProtocol>
#include <TelepathyQt/AccountManager>

#include "connection.hpp"
#include "types.hpp"

namespace Tp { class BaseConnectionManager; }

class PipeProtocol : public Tp::BaseProtocol {

    Q_OBJECT;
    Q_DISABLE_COPY(PipeProtocol)

    public:

        PipeProtocol(
                const QDBusConnection &dbusConnection, 
                const QString &name,
                const PipePtr &pipe,
                Tp::AccountManagerPtr amp,
                Tp::BaseConnectionManager* cm);

        virtual ~PipeProtocol() = default;

    private:

        Tp::BaseConnectionPtr createConnection(const QVariantMap &parameters, Tp::DBusError *error);
        QString identifyAccount(const QVariantMap &parameters, Tp::DBusError *error);
        QString normalizeContact(const QString &contactId, Tp::DBusError *error);

    private:

        Tp::BaseProtocolAvatarsInterfacePtr avatarsIface;
        Tp::BaseProtocolPresenceInterfacePtr presenceIface;

        PipePtr pipe;
        Tp::AccountManagerPtr amp;
        Tp::BaseConnectionManager* cm;
};

#endif
