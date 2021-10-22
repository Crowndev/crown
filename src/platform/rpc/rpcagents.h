#ifndef CROWN_PLATFORM_RPCAGENTS_H
#define CROWN_PLATFORM_RPCAGENTS_H

#include <rpc/server.h>

UniValue agents(const JSONRPCRequest& request);

UniValue vote(const JSONRPCRequest& request);
UniValue list(const JSONRPCRequest& request);
UniValue listcandidates(const JSONRPCRequest& request);

#endif //CROWN_PLATFORM_RPCAGENTS_H
