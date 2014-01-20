#ifndef PIPE_CONNECTION_MANAGER_HPP
#define PIPE_CONNECTION_MANAGER_HPP

#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/Account>
#include <TelepathyQt/ClientRegistrar>

#include "casehandler.hpp"
#include "approver.hpp"

class PipeConnectionManager : public Tp::BaseConnectionManager {

    Q_OBJECT;
    Q_DISABLE_COPY(PipeConnectionManager)

    public:

        PipeConnectionManager(const QDBusConnection &connection);
        virtual QVariantMap immutableProperties() const override;

        /**
         * Checks if upcoming channel should be piped
         * @return CaseHandler void with valid handler if true, otherwise handler is set to false
         */
        CaseHandler<void> checkNewChannel(const Tp::ConnectionPtr &connection, const QList<Tp::ChannelPtr> &channels) const;

    private:
        
        /**
         * Initializes all fields, protocols and registers an object on the bus
         */
        void init();

    private:

        Tp::ClientRegistrarPtr registrar;
        Tp::AccountManagerPtr amp;
        PipeApproverPtr pipeApprover;
};

#endif
