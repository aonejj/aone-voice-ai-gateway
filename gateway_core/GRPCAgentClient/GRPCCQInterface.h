#ifndef __GRPC_CQ_INTERFACE_H__
#define __GRPC_CQ_INTERFACE_H__

class GRPCCQInterface {
public:
    GRPCCQInterface() = default;
    virtual ~GRPCCQInterface() = default;

public:
    virtual void* GetCQPtr() = 0;    
};

#endif // __GRPC_CQ_INTERFACE_H__