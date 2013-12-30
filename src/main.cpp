#include <QtCore>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Types>
#include "connection_manager.hpp"

int main(int argc, char **argv) {

    QCoreApplication app(argc, argv);

    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    PipeConnectionManager cm(QDBusConnection::sessionBus(), QLatin1String("PipeCM"));
    cm.registerObject();

    return app.exec();
}
