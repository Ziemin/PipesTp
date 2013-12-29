#include "protocol.hpp"

PipeProtocol::PipeProtocol(const QDBusConnection &dbusConnection, const QString &name) 
    : Tp::BaseProtocol(dbusConnection, name) 
{

}
