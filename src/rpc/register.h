// Copyright (c) 2009-2018 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_RPC_REGISTER_H
#define CROWN_RPC_REGISTER_H

/** These are in one header file to avoid creating tons of single-function
 * headers for everything under src/rpc/ */
class CRPCTable;

/** Register block chain RPC commands */
void RegisterBlockchainRPCCommands(CRPCTable &tableRPC);
/** Register P2P networking RPC commands */
void RegisterNetRPCCommands(CRPCTable &tableRPC);
/** Register miscellaneous RPC commands */
void RegisterMiscRPCCommands(CRPCTable &tableRPC);
/** Register mining RPC commands */
void RegisterMiningRPCCommands(CRPCTable &tableRPC);
/** Register raw transaction RPC commands */
void RegisterRawTransactionRPCCommands(CRPCTable &tableRPC);
/** Register Contract RPC commands */
void RegisterSmsgRPCCommands(CRPCTable &tableRPC);
/** Register Contract RPC commands */
void RegisterContractRPCCommands(CRPCTable &tableRPC);
/** Register Insight RPC commands */
void RegisterInsightRPCCommands(CRPCTable &tableRPC);
/** Register masternode RPC commands */
void RegisterMasternodeCommands(CRPCTable &tableRPC);
/** Register systemnode RPC commands */
void RegisterSystemnodeCommands(CRPCTable &tableRPC);
/** Register budget RPC commands */
void RegisterBudgetCommands(CRPCTable &tableRPC);

static inline void RegisterAllCoreRPCCommands(CRPCTable &t)
{
    RegisterBlockchainRPCCommands(t);
    RegisterNetRPCCommands(t);
    RegisterMiscRPCCommands(t);
    RegisterMiningRPCCommands(t);
    RegisterSmsgRPCCommands(t);
    RegisterRawTransactionRPCCommands(t);
    RegisterContractRPCCommands(t);
    RegisterInsightRPCCommands(t);
    RegisterMasternodeCommands(t);
    RegisterSystemnodeCommands(t);
    RegisterBudgetCommands(t);
}

#endif // CROWN_RPC_REGISTER_H
