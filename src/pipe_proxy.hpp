#include <QtCore>
#include <TelepathyQt/StatelessDBusProxy>
#include <TelepathyQt/Feature>

#include <memory>

class Pipe : Tp::StatelessDBusProxy {

    public:
        Pipe(const QDBusConnection &dbusConnection,
                const QString &busName,
                const QString &objectPath,
                const Tp::Feature &featureCore);

};

typedef std::shared_ptr<Pipe> PipePtr;
