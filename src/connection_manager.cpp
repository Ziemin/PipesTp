#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/AccountSet>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/BaseProtocol>
#include <TelepathyQt/RequestableChannelClassSpecList>
#include <TelepathyQt/ChannelClassSpecList>
#include <QDebug>
#include <QtDBus>
#include <vector>

#include "connection_manager.hpp"
#include "types.hpp"
#include "defines.hpp"
#include "pipe_interface.h"
#include "protocol.hpp"
#include "utils.hpp"

namespace init {

    std::vector<PipePtr> discoverPipes(const QDBusConnection& connection) {

        QDBusConnectionInterface *dci = connection.interface();
        QDBusInterface dbus(dci->service(), dci->path(), dci->interface(), connection);
        QDBusReply<QStringList> servicesRep = dbus.call("ListActivatableNames");

        std::vector<PipePtr> pipes;

        if(servicesRep.isValid()) {

            // obtaining all pipe services
            QStringList pipeServices = servicesRep.value().filter(QRegExp("^" TP_QT_IFACE_PIPE ".*$"));
            QDBusReply<QStringList> regServicesReply = dci->registeredServiceNames(); // not activatable
            if(regServicesReply.isValid()) {
                QStringList registeredPipes = regServicesReply.value().filter(QRegExp("^" TP_QT_IFACE_PIPE ".*$"));
                for(auto &servName: registeredPipes) {
                    if(!pipeServices.contains(servName)) pipeServices << servName;
                }
            }

            QString path;
            for(auto it = pipeServices.constBegin(); it != pipeServices.constEnd(); ++it) {
                QDBusReply<bool> isRegisteredRep = dci->isServiceRegistered(*it);
                QDBusReply<void> startServiceRep;
                if((isRegisteredRep.isValid() && isRegisteredRep.value()) 
                        || (startServiceRep = dci->startService(*it), startServiceRep.isValid())) 
                {
                    pDebug() << "Pipe service - > " + *it + " is started";
                    path = "/" + *it;
                    path.replace('.', '/');
                    pipes.push_back(std::make_shared<Pipe>(*it, path, connection));
                } else {
                    pWarning() << "Pipe service - > " + *it + " could not be started";
                }
            }
        } else {
            pWarning() << "Could not obtain services list from dbus connection";
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
    registrar = Tp::ClientRegistrar::create();
    init();
}

QVariantMap PipeConnectionManager::immutableProperties() const {

    return QVariantMap();
}

void PipeConnectionManager::init() {

    amp = Tp::AccountManager::create(dbusConnection());
    connect(amp->becomeReady(Tp::Features(Tp::AccountManager::FeatureCore)),
            &Tp::PendingOperation::finished, 
            this, [this](Tp::PendingOperation *op) {

                if(op->isError()) {
                    pCritical() << "Account manager cannot become ready:" 
                        << op->errorName() << "-" << op->errorMessage();

                        QCoreApplication::exit(1);
                }
                Tp::ChannelClassSpecList channelFilter;
                std::vector<PipePtr> pipes = init::discoverPipes(dbusConnection());
                if(pipes.empty()) pWarning() << "No pipes found";
                for(auto& pipe: pipes) {
                    addProtocol(Tp::BaseProtocolPtr(
                                new PipeProtocol(dbusConnection(), pipe->name() + "Pipe", pipe, amp)));

                    // building channel filter for approver
                    Tp::RequestableChannelClassSpecList protoRecList = pipe->requestableChannelClasses();
                    for(auto &reqChanSpec: protoRecList) {
                        channelFilter << Tp::ChannelClassSpec(
                            reqChanSpec.channelType(), 
                            reqChanSpec.targetHandleType(),
                            reqChanSpec.fixedProperties());
                    }
                }

                // registering objects
                if(!registerObject()) {
                    qCritical() << "Could not register pipe connection manager";
                    QCoreApplication::exit(1);
                }

                pipeApprover = PipeApproverPtr(new PipeApprover(channelFilter, *this));
                if(!registrar->registerClient(
                    Tp::AbstractClientPtr::dynamicCast(pipeApprover), 
                    "pipeApprover")) 
                {
                    pCritical() << "Could not register pipeApprover";
                    QCoreApplication::exit(1);
                }
            });
}


CaseHandler<void> PipeConnectionManager::checkNewChannel(
        const Tp::ConnectionPtr &connection, const Tp::AccountPtr &account, const QList<Tp::ChannelPtr> &channels) const 
{
    // TODO implement
    return CaseHandler<void>();
}
