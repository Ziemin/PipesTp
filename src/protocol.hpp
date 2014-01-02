#ifndef PIPE_PROTOCOL_HPP
#define PIPE_PROTOCOL_HPP

#include <TelepathyQt/BaseProtocol>
#include "pipe_proxy.hpp"
#include "connection.hpp"

class PipeProtocol : public Tp::BaseProtocol {

    Q_OBJECT;
    Q_DISABLE_COPY(PipeProtocol)

    public:

        PipeProtocol(
                const QDBusConnection &dbusConnection, 
                const QString &name,
                PipePtr pipe);
};

#endif
