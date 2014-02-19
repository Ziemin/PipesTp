#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/AccountSet>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/BaseProtocol>
#include <TelepathyQt/RequestableChannelClassSpecList>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/ChannelDispatcher>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/PendingReady>
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

void pipeChannels(PipeConnectionPtr pipeCon, std::vector<Tp::ChannelPtr> channels) {

    Tp::ObjectPathList pathsToNewChannels;
    for(auto &chan: channels) {
        try {

            Tp::PendingReady *pendingReady = chan->becomeReady(Tp::Features() << Tp::Channel::FeatureCore); 
            { // wait for operation to finish
                QEventLoop loop;
                QObject::connect(pendingReady, &Tp::PendingOperation::finished,
                        &loop, &QEventLoop::quit);
                loop.exec();
            }
            pDebug() << "Piping channel: " << chan->objectPath();
            Tp::DBusError dbError;
            QDBusObjectPath newChan(pipeCon->createChannel(
                        chan->channelType(),
                        chan->targetHandleType(),
                        chan->targetHandle(), 
                        chan->initiatorContact()->handle().front(),
                        false,
                        &dbError)->objectPath());
            if(!dbError.isValid()) {
                pathsToNewChannels << newChan;
            } else {
                pWarning() << "Could not pipe: " << chan->objectPath();
            }

        } catch(...) {
            pWarning() << "Could not pipe: " << chan->objectPath();
        }
    }
    if(pathsToNewChannels.empty()) return;

    Tp::Client::ChannelDispatcherInterface channelDispatcher(
            TP_QT_CHANNEL_DISPATCHER_BUS_NAME, 
            TP_QT_CHANNEL_DISPATCHER_OBJECT_PATH);
    // TODO zero in meanwhile as user action time
    pDebug() << "Delegating " << pathsToNewChannels.size() << " channels";

    QDBusPendingReply<Tp::ObjectPathList, Tp::NotDelegatedMap> notDelegatedRep = 
        channelDispatcher.DelegateChannels(pathsToNewChannels, 0, ""); 

    if(notDelegatedRep.isValid()) {
        Tp::NotDelegatedMap failChans = notDelegatedRep.argumentAt(1).value<Tp::NotDelegatedMap>();
        for(auto it = failChans.cbegin(); it != failChans.cend(); ++it) {
            pWarning() << "Could not delegate: " << it.key().path() << " due to: " << it.value().errorMessage;
        }
    } else {
        pWarning() << "Could not get delegated reply: " << 
            notDelegatedRep.error().name() << " -> " << notDelegatedRep.error().message();
    }
}

// ---- PipeConnectionManager implementation ------------------------------------------------------
PipeConnectionManager::PipeConnectionManager(
        const QDBusConnection& connection) 
: Tp::BaseConnectionManager(connection, TP_QT_PIPE_CONNECTION_MANAGER_NAME)
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
                                new PipeProtocol(dbusConnection(), pipe->name() + "Pipe", pipe, amp, this)));

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
        const Tp::ConnectionPtr &connection, const QList<Tp::ChannelPtr> &channels) const 
{
    // search for connection piping given connection
    QList<Tp::BaseConnectionPtr> pipeConnections = connections();
    for(auto &c: pipeConnections) {
        PipeConnectionPtr pipeCon = PipeConnectionPtr::dynamicCast(c);
        Tp::ConnectionPtr wrappedConnection = pipeCon->getPipedConnection();
        if(wrappedConnection->objectPath() == connection->objectPath()) {

            std::vector<Tp::ChannelPtr> chansToPipe;
            // now check if specific channels for contacts are desired
            for(auto &chan: channels) {

                Tp::PendingReady *pendingReady = chan->becomeReady();
                { // wait for channel to become ready
                    QEventLoop loop;
                    QObject::connect(pendingReady, &Tp::PendingOperation::finished,
                            &loop, &QEventLoop::quit);
                    loop.exec();
                }
                if(pipeCon->checkChannel(*chan.data())) chansToPipe.push_back(chan);
            }
            if(chansToPipe.empty()) return CaseHandler<void>();
            else {
                return CaseHandler<void>(pipeChannels, pipeCon, std::move(chansToPipe));
            }
        }
    }

    return CaseHandler<void>();
}
