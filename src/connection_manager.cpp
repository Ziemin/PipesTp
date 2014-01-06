#include <TelepathyQt/AccountSet>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/BaseProtocol>
#include <QDebug>
#include <QtDBus>
#include <vector>

#include "connection_manager.hpp"
#include "types.hpp"
#include "defines.hpp"
#include "pipe_interface.h"
#include "protocol.hpp"

namespace init {

    std::vector<PipePtr> discoverPipes(const QDBusConnection& connection) {

        QDBusConnectionInterface *dci = connection.interface();
        QDBusInterface dbus(dci->service(), dci->path(), dci->interface(), connection);
        QDBusReply<QStringList> servicesRep = dbus.call("ListActivatableNames");

        std::vector<PipePtr> pipes;

        if(servicesRep.isValid()) {

            QStringList pipeServices = servicesRep.value()
                .filter(QRegExp("^" TP_QT_IFACE_PIPE ".*$"));

            QString path;
            for(auto it = pipeServices.constBegin(); it != pipeServices.constEnd(); ++it) {
                QDBusReply<bool> isRegisteredRep = dci->isServiceRegistered(*it);
                QDBusReply<void> startServiceRep;
                if((isRegisteredRep.isValid() && isRegisteredRep.value()) 
                        || (startServiceRep = dci->startService(*it), startServiceRep.isValid())) 
                {
                    qDebug() << "Pipe service - > " + *it + " is started";
                    path = "/" + *it;
                    path.replace('.', '/');
                    pipes.push_back(std::make_shared<Pipe>(*it, path, connection));
                } else {
                    qWarning() << "Pipe service - > " + *it + " could not be started";
               
                }
            }
        } else {
            qWarning() << "Could not obtain services list from dbus connection";
        }

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
        qWarning() << "Account manager cannot become ready:" 
            << op->errorName() << "-" << op->errorMessage();

        return;
    }
    std::vector<PipePtr> pipes = init::discoverPipes(dbusConnection());
    for(auto& pipe: pipes) {
        addProtocol(Tp::BaseProtocolPtr(
                    new PipeProtocol(dbusConnection(), pipe->name() + "Pipe", pipe, amp)));
    }
    registerObject();
}
