#ifndef PIPE_CONNECTION_MANAGER_HPP
#define PIPE_CONNECTION_MANAGER_HPP

#include <TelepathyQt/BaseConnectionManager>

class PipeConnectionManager : public Tp::BaseConnectionManager {

    Q_OBJECT;
    Q_DISABLE_COPY(PipeConnectionManager)

    public:

        PipeConnectionManager(const QDBusConnection &connection, const QString &name);

        virtual QVariantMap immutableProperties() const override;

};

#endif
