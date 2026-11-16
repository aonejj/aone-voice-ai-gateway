#ifndef __GRPC_AGENT_TAGS_H__
#define __GRPC_AGENT_TAGS_H__

#include <grpcpp/grpcpp.h>

class GRPCCQTag {
public:
    virtual ~GRPCCQTag() = default;
    virtual void Proceed(bool ok) = 0;
};

#endif // __GRPC_AGENT_TAGS_H__