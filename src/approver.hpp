#ifndef PIPE_APPROVER_HPP
#define PIPE_APPROVER_HPP

#include <TelepathyQt/AbstractClientApprover>
#include "connection_manager.hpp"

class PipeConnectionManager;

class PipeApprover : public Tp::AbstractClientApprover {

    public:

        PipeApprover(const Tp::ChannelClassSpecList& channelFilter, const PipeConnectionManager &pipeCM);

        virtual void addDispatchOperation(const Tp::MethodInvocationContextPtr<>& context,
                const Tp::ChannelDispatchOperationPtr& dispatchOperation) override;

    private:

        const PipeConnectionManager& pipeCM;
};

#endif
