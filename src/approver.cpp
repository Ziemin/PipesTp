#include "approver.hpp"

PipeApprover::PipeApprover(const Tp::ChannelClassSpecList& channelFilter, 
        const PipeConnectionManager &pipeCM) 
    : Tp::AbstractClientApprover(channelFilter),
    pipeCM(pipeCM)
{

}

void PipeApprover::addDispatchOperation(const Tp::MethodInvocationContextPtr<>& context,
        const Tp::ChannelDispatchOperationPtr& dispatchOperation) {

}
