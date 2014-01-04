#include "protocol.hpp"

PipeProtocol::PipeProtocol(
        const QDBusConnection &dbusConnection, 
        const QString &name,
        PipePtr pipe,
        Tp::AccountManagerPtr amp) : 
    Tp::BaseProtocol(dbusConnection, name),
    pipe(pipe),
    amp(amp)
{

    setEnglishName(QLatin1String("Pipe: ") + QLatin1String(pipe->name().toStdString().c_str()));
    setIconName(englishName() + QLatin1String("-icon"));
    setVCardField(englishName().toLower());

    setRequestableChannelClasses(
            Tp::RequestableChannelClassSpecList(pipe->requestableChannelClasses()));

    setParameters(Tp::ProtocolParameterList() <<
            Tp::ProtocolParameter(QLatin1String("example-param"),
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

Tp::BaseConnectionPtr PipeProtocol::createConnection(const QVariantMap &parameters, Tp::DBusError *error) {
    return Tp::BaseConnectionPtr();
}

QString PipeProtocol::identifyAccount(const QVariantMap &parameters, Tp::DBusError *error) {
    return QString();
}

QString PipeProtocol::normalizeContact(const QString &contactId, Tp::DBusError *error) {
    return QString();
}
