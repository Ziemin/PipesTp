#ifndef PIPE_SIMPLE_PRESENCE_HPP
#define PIPE_SIMPLE_PRESENCE_HPP

#include <TelepathyQt/Connection>
#include <QObject>

#include "contact_list.hpp"

typedef Tp::Client::ConnectionInterfaceSimplePresenceInterface SimplePresence;

enum class SimplePresenceError {
    UNDEFINED,
    CONTACT_LIST_REQUIRED,
    NETWORK_ERROR,
    INVALID_ARGUMENT,
    NOT_AVAILABLE
};

typedef PipeException<SimplePresenceError> SimplePresenceException;

class PipeSimplePresence : public QObject {
    
    public:
        PipeSimplePresence(
                SimplePresence *pipedPresence, Tp::BaseConnectionSimplePresenceInterfacePtr presenceIface);

        /**
         * Sets contact list as a source of contacts to pipe
         */
        void setContactList(PipeContactList *pipeList);

        void setPresence(const QString &status, const QString &statusMessager);

    private:
        void presenceChangedCb(const Tp::SimpleContactPresences &presence);

    private:
        PipeContactList *pipeList = nullptr;
        SimplePresence *pipedPresence;
        Tp::BaseConnectionSimplePresenceInterfacePtr presenceIface;

};

#endif
