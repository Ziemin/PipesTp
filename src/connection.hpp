#ifndef PIPE_CONNECTION_HPP
#define PIPE_CONNECTION_HPP

#include <TelepathyQt/BaseConnection>

class PipeConnection : public Tp::BaseConnection {

    Q_OBJECT;
    Q_DISABLE_COPY(PipeConnection)

    public:

        PipeConnection(
                const Tp::ConnectionPtr pipedConnection,
                const QDBusConnection& dbusConnection,
                const QString& cmName,
                const QString& protocolName,
                const QVariantMap& parameters);

        virtual QString uniqueName() const override;

};

#endif
