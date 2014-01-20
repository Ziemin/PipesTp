#include <QtCore>
#include <QtDebug>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>
#include <TelepathyQt/SharedPtr>

#include "connection_manager.hpp"

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
    PipeConnectionManager cm(dbusConnection);

    return app.exec();
}
