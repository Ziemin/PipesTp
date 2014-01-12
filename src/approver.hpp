#ifndef PIPE_APPROVER_HPP
#define PIPE_APPROVER_HPP

#include <TelepathyQt/AbstractClientApprover>

class PipeConnectionManager;

class PipeApprover : public QObject, public Tp::AbstractClientApprover {

    Q_OBJECT;
    Q_DISABLE_COPY(PipeApprover)

    public:

        PipeApprover(const Tp::ChannelClassSpecList& channelFilter, const PipeConnectionManager &pipeCM);

        virtual void addDispatchOperation(const Tp::MethodInvocationContextPtr<>& context,
                const Tp::ChannelDispatchOperationPtr &dispatchOperation) override;

    private:

        const PipeConnectionManager &pipeCM;
};

typedef Tp::SharedPtr<PipeApprover> PipeApproverPtr;

#endif
