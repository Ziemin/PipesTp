#include <TelepathyQt/ChannelDispatchOperation>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Debug>
#include <iostream>

#include "approver.hpp"
#include "casehandler.hpp"
#include "utils.hpp"
#include "connection_manager.hpp"

PipeApprover::PipeApprover(const Tp::ChannelClassSpecList& channelFilter, 
        const PipeConnectionManager &pipeCM) 
    : Tp::AbstractClientApprover(channelFilter),
    pipeCM(pipeCM)
{ }

void PipeApprover::addDispatchOperation(const Tp::MethodInvocationContextPtr<> &context,
        const Tp::ChannelDispatchOperationPtr &dispatchOperation) 
{
    CaseHandler<void> handler = pipeCM.checkNewChannel(
            dispatchOperation->connection(),
            dispatchOperation->channels());

    if(handler) {
        pDebug() << "Claiming ownership of some channels from: \n"
            << "    Connection: " << dispatchOperation->connection()->busName() 
            << "\n    Channels number: " << dispatchOperation->channels().size();
        MoveOnCopy<CaseHandler<void>> moved_handler(std::move(handler));

        connect(dispatchOperation->claim(), 
                &Tp::PendingOperation::finished, 
                this, 
                [moved_handler](Tp::PendingOperation *op) {
                    if(op->isError()) 
                        pDebug() << "Error while claiming channel: " << op->errorMessage();
                    else moved_handler.value(); 
                });
    } else {
        pDebug() << "New channels are rejected: \n" 
            << "    Connection: " << dispatchOperation->connection()->busName() 
            << "\n    Channels number: " << dispatchOperation->channels().size();
    }
    SCOPE_EXIT( context->setFinished(); );
}
