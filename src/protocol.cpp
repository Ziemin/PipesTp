#include <TelepathyQt/AccountSet>
#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/PendingReady>
#include <QEventLoop>
#include <QObject>

#include "protocol.hpp"
#include "utils.hpp"
#include "defines.hpp"

PipeProtocol::PipeProtocol(
        const QDBusConnection &dbusConnection, 
        const QString &name,
        const PipePtr &pipe,
        Tp::AccountManagerPtr amp,
        Tp::BaseConnectionManager* cm)
        : 
    Tp::BaseProtocol(dbusConnection, name),
    pipe(pipe),
    amp(amp),
    cm(cm)
{

    setEnglishName(QLatin1String("Pipe-") + QLatin1String(pipe->name().toStdString().c_str()));
    setIconName(englishName() + QLatin1String("-icon"));
    setVCardField(englishName().toLower());

    setRequestableChannelClasses(
            Tp::RequestableChannelClassSpecList(pipe->requestableChannelClasses()));

    setParameters(Tp::ProtocolParameterList() <<
            Tp::ProtocolParameter(QLatin1String("Protocol"),
                QLatin1String("s"), Tp::ConnMgrParamFlagRequired)
            << Tp::ProtocolParameter(QLatin1String("Identificator"),
                QLatin1String("s"), Tp::ConnMgrParamFlagRequired));

    // set callbacks
    setCreateConnectionCallback(Tp::memFun(this, &PipeProtocol::createConnection));
    setIdentifyAccountCallback(Tp::memFun(this, &PipeProtocol::identifyAccount));
    setNormalizeContactCallback(Tp::memFun(this, &PipeProtocol::normalizeContact));

    // set pluggable interfaces
    avatarsIface = Tp::BaseProtocolAvatarsInterface::create();
    avatarsIface->setAvatarDetails(Tp::AvatarSpec(
                QStringList() << QLatin1String("image/png"), 16, 64, 32, 16, 64, 32, 1024));
    plugInterface(Tp::AbstractProtocolInterfacePtr::dynamicCast(avatarsIface));

    Tp::SimpleStatusSpec spAvail;
    spAvail.type = Tp::ConnectionPresenceTypeAvailable;
    spAvail.maySetOnSelf = false;
    spAvail.canHaveMessage = false;

    Tp::SimpleStatusSpec spOff;
    spOff.type = Tp::ConnectionPresenceTypeOffline;
    spOff.maySetOnSelf = false;
    spOff.canHaveMessage = false;

    Tp::SimpleStatusSpecMap specs;
    specs.insert("available", spAvail);
    specs.insert("offline", spOff);

    presenceIface = Tp::BaseProtocolPresenceInterface::create();
    presenceIface->setStatuses(Tp::PresenceSpecList(specs));
    plugInterface(Tp::AbstractProtocolInterfacePtr::dynamicCast(presenceIface));
}


bool isConnectionPipable(const PipeProtocol &protocol, const Tp::ConnectionPtr &connection) {

    // check requestable channel classes
    QString busName = QString(TP_QT_CONNECTION_MANAGER_BUS_NAME_BASE) + connection->cmName();
    QString objectPath = QString(TP_QT_CONNECTION_MANAGER_OBJECT_PATH_BASE)
        + connection->cmName() + "/" + connection->protocolName();

    Tp::Client::DBus::PropertiesInterface propsIface(QDBusConnection::sessionBus(), busName, objectPath);

    QDBusPendingReply<QDBusVariant> reqChanClassesRep = propsIface.Get(
            TP_QT_IFACE_PROTOCOL, "RequestableChannelClasses");
    reqChanClassesRep.waitForFinished();

    if(reqChanClassesRep.isValid()) {
        // unmarshalling
        QDBusArgument dbusArg = reqChanClassesRep.value().variant().value<QDBusArgument>();
        Tp::RequestableChannelClassList chList;
        dbusArg >> chList;

        Tp::RequestableChannelClassSpecList chSpecList(chList);
        for(Tp::RequestableChannelClassSpec& chClass: protocol.requestableChannelClasses()) {
            bool found = false;
            for(auto& pipedChannelType: chSpecList) {
                if(pipedChannelType.channelType() == chClass.channelType()) {
                    found = true;
                    break;
                }
            }
            if(!found) return false;
        }
        return true;
    } else {
        pDebug() << "Cannnot get RequestableChannelClasses property: " << reqChanClassesRep.error();
        return false;
    }
}

Tp::BaseConnectionPtr PipeProtocol::createConnection(const QVariantMap &parameters, Tp::DBusError *error) {

    auto protocolIt = parameters.constFind("Protocol");
    auto nameIt = parameters.constFind("Identificator");
    if(protocolIt == parameters.constEnd() || nameIt == parameters.constEnd()
            || !protocolIt->isValid() || !nameIt->isValid()) 
    {
        pDebug() << "Creating connection with not enaugh parameters for protocol: " << name();
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, 
                QString("Not enaugh parameters were provided to create connection for protocol: ") + name());
        return Tp::BaseConnectionPtr();
    }

    Tp::AccountSetPtr accSet = amp->validAccounts();
    QList<Tp::AccountPtr> accnts= accSet->accounts();
    for(auto ap: accnts) {
        // found proper account
        if(ap->protocolName() == protocolIt->value<QString>() && ap->normalizedName() == nameIt->value<QString>()) {
            pDebug() << "Creating piped connection for account: " << ap->objectPath();

            Tp::ConnectionPtr pipedConnection = ap->connection();
            if(pipedConnection.data() == nullptr) {
                pWarning() << "Connection returned by account is null: ";
                error->set(TP_QT_ERROR_NOT_AVAILABLE, "Connection is already being piped");
                return Tp::BaseConnectionPtr();
            }
            Tp::PendingReady *pendingReady = pipedConnection->becomeReady(Tp::Connection::FeatureCore); 
            { // wait for operation to finish
                QEventLoop loop;
                QObject::connect(pendingReady, &Tp::PendingOperation::finished,
                        &loop, &QEventLoop::quit);
                loop.exec();
            }

            if(isConnectionPipable(*this, pipedConnection)) {

                // check if not exists
                QList<Tp::BaseConnectionPtr> existingCons = cm->connections();
                for(auto& conPtr: existingCons) {
                    if(PipeConnectionPtr::dynamicCast(conPtr)->getPipedConnection()->objectPath() 
                            == pipedConnection->objectPath()) 
                    {
                        pWarning() << "Connection is already being piped: " << pipedConnection->objectPath();
                        error->set(TP_QT_ERROR_NOT_AVAILABLE, "Connection is already being piped");
                        return Tp::BaseConnectionPtr();
                    }
                }

                if(pipedConnection->isReady())
                    return Tp::BaseConnectionPtr(new PipeConnection(
                                pipedConnection,
                                pipe,
                                QDBusConnection::sessionBus(),
                                TP_QT_PIPE_CONNECTION_MANAGER_NAME,
                                name(),
                                parameters,
                                { name() + "_" + protocolIt->value<QString>() + "_" + nameIt->value<QString>() }));
                else {
                    error->set(TP_QT_ERROR_NETWORK_ERROR, "Piped connection has problems becoming ready: " + pipe->name());
                    return Tp::BaseConnectionPtr();
                }
            } else {
                error->set(TP_QT_ERROR_INVALID_ARGUMENT, "Connection cannot be piped through pipe: " + pipe->name());
                return Tp::BaseConnectionPtr();
            }
        }
    }

    pDebug() << "Could not find suitable connection to pipe for parameters:\n"
        << "protocol: " << protocolIt->value<QString>() << " Display name: " << nameIt->value<QString>();
    error->set(TP_QT_ERROR_INVALID_ARGUMENT, "Could not find connection to pipe for provided parameters");
    return Tp::BaseConnectionPtr();
}

QString PipeProtocol::identifyAccount(const QVariantMap &parameters, Tp::DBusError *error) {
    return QString();
}

QString PipeProtocol::normalizeContact(const QString &contactId, Tp::DBusError *error) {
    return QString();
}
