#include "simple_presence.hpp"
#include "utils.hpp"

#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingVariant>

// in current implementation I assume 
// that I always get a full list of statuses when signal is emitted
PipeSimplePresence::PipeSimplePresence(
        SimplePresence *pipedPresence, Tp::BaseConnectionSimplePresenceInterfacePtr presenceIface) 
    : pipedPresence(pipedPresence), presenceIface(presenceIface) 
{
    Tp::PendingVariant *pendingRep = pipedPresence->requestPropertyMaximumStatusMessageLength();
    connect(pendingRep,
            &Tp::PendingOperation::finished,
            this, 
            [this, pendingRep](Tp::PendingOperation *op) {
                if(op->isValid()) {
                   uint maxMesLen = pendingRep->result().value<uint>(); 
                   this->presenceIface->setMaxmimumStatusMessageLength(maxMesLen);
                } else {
                    pWarning() << "Error happened during getting maximum message length from connection: " 
                        << this->pipedPresence->path() << " " << op->errorMessage();
                }
            });

    pendingRep = pipedPresence->requestPropertyStatuses();
    connect(pendingRep,
            &Tp::PendingOperation::finished,
            this,
            [this, pendingRep](Tp::PendingOperation *op) {
                if(op->isValid()) {
                    Tp::SimpleStatusSpecMap statuses = pendingRep->result().value<Tp::SimpleStatusSpecMap>();
                    this->presenceIface->setStatuses(statuses);
                } else {
                    pWarning() << "Error happened during getting statuses from connection: " 
                        << this->pipedPresence->path() << " " << op->errorMessage();
                }
            });

    connect(pipedPresence, &SimplePresence::PresencesChanged,
            this, &PipeSimplePresence::presenceChangedCb);
}

void PipeSimplePresence::setContactList(PipeContactList *pipeList) {
    this->pipeList = pipeList;
}

void PipeSimplePresence::setPresence(const QString &/* status */, const QString &/* statusMessager */) {
    // nothing to do
}

void PipeSimplePresence::presenceChangedCb(const Tp::SimpleContactPresences &presences) {

    Tp::SimpleContactPresences newPresences;
    if(pipeList != nullptr) {
        for(auto it = presences.cbegin(); it != presences.cend(); ++it) {
            if(pipeList->hasHandle(it.key())) 
                newPresences[it.key()] = it.value();
        }
        presenceIface->setPresences(newPresences);
    } 
}

