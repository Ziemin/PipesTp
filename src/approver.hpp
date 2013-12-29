#ifndef PIPE_APPROVER_HPP
#define PIPE_APPROVER_HPP

#include <TelepathyQt/AbstractClientApprover>

class PipeApprover : public Tp::AbstractClientApprover {

    public:

        PipeApprover(const Tp::ChannelClassSpecList& channelFilter);

        virtual void addDispatchOperation(const Tp::MethodInvocationContextPtr<>& context,
                const Tp::ChannelDispatchOperationPtr& dispatchOperation) override;
};

#endif
