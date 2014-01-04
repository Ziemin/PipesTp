#include <QtCore>
#include <QtDebug>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>
#include <TelepathyQt/AbstractClient>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/SharedPtr>

#include "connection_manager.hpp"
#include "approver.hpp"

void registerPipeTypes() {
    typedef Tp::RequestableChannelClassList RequestableChannelClassList;
    qRegisterMetaType<RequestableChannelClassList>("RequestableChannelClassList");
    qDBusRegisterMetaType<RequestableChannelClassList>();
}

int main(int argc, char **argv) {

    QCoreApplication app(argc, argv);

    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    registerPipeTypes();

    QDBusConnection dbusConnection = QDBusConnection::sessionBus();
    PipeConnectionManager cm(dbusConnection, QLatin1String("pipes"));

    Tp::ClientRegistrarPtr registrar = Tp::ClientRegistrar::create();
    Tp::AbstractClientPtr approver = Tp::AbstractClientPtr::dynamicCast(
            Tp::SharedPtr<PipeApprover>(new PipeApprover(
                    Tp::ChannelClassSpecList(), 
                    cm)));

    registrar->registerClient(approver, "pipeApprover");

    return app.exec();
}
