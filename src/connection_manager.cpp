#include <TelepathyQt/AccountSet>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <QDebug>
#include <vector>

#include "connection_manager.hpp"
#include "pipe_proxy.hpp"

namespace init {

    std::vector<PipePtr> discoverPipes() {

        std::vector<PipePtr> pipes;

        return pipes;
    }

} /* init namespace */


// ---- PipeConnectionManager implementation ------------------------------------------------------
PipeConnectionManager::PipeConnectionManager(
        const QDBusConnection& connection, 
        const QString& name) 
: Tp::BaseConnectionManager(connection, name)
{
    init();
}

QVariantMap PipeConnectionManager::immutableProperties() const {

    return QVariantMap();
}

void PipeConnectionManager::init() {

    amp = Tp::AccountManager::create(dbusConnection());
    connect(amp->becomeReady(Tp::Features(Tp::AccountManager::FeatureCore)),
            &Tp::PendingOperation::finished, 
            this, &PipeConnectionManager::onAccountManagerReady);
}

void PipeConnectionManager::onAccountManagerReady(Tp::PendingOperation *op) {

    if(op->isError()) {
        qWarning() << "Account manager cannot become ready:" <<
            op->errorName() << "-" << op->errorMessage();

        return;
    }

    std::vector<PipePtr> pipes = init::discoverPipes();
    for(auto& pipe: pipes) {
        // create protocols and add to cm
    }

    Tp::AccountSetPtr validAccounts = amp->validAccounts();
    qDebug() << "Accounts: " << validAccounts->accounts().size();

    registerObject();
}
