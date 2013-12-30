#include "connection_manager.hpp"


PipeConnectionManager::PipeConnectionManager(const QDBusConnection& connection, const QString& name) 
: Tp::BaseConnectionManager(connection, name) 
{

}

QVariantMap PipeConnectionManager::immutableProperties() const {

    return QVariantMap();
}
