#ifndef PIPE_PROTOCOL_HPP
#define PIPE_PROTOCOL_HPP

#include <TelepathyQt/BaseProtocol>

class PipeProtocol : public Tp::BaseProtocol {

    Q_OBJECT;
    Q_DISABLE_COPY(PipeProtocol)

    public:
        PipeProtocol(const QDBusConnection &dbusConnection, const QString &name);
};

#endif
