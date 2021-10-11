// Copyright (c) 2014-2020 The Crown developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_MN_PROCESSING_H
#define CROWN_MN_PROCESSING_H

#include <consensus/params.h>
#include <consensus/validation.h>
#include <net.h>
#include <net_processing.h>
#include <sync.h>
#include <validationinterface.h>

class CChainParams;
class CTxMemPool;

#define SET_CONDITION_FLAG(flag) flag = true;
#define RETURN_ON_CONDITION(condition) if (condition) { return true; }

bool AlreadyHaveMasternodeTypes(const CInv& inv, const CTxMemPool& mempool);
void ProcessGetDataMasternodeTypes(CNode* pfrom, const CChainParams& chainparams, CConnman* connman, const CTxMemPool& mempool, const CInv& inv, bool& pushed) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
bool ProcessMessageMasternodeTypes(CNode* pfrom, const std::string& msg_type, CDataStream& vRecv, const CChainParams& chainparams, CTxMemPool& mempool, CConnman* connman, BanMan* banman, const std::atomic<bool>& interruptMsgProc);

#endif // CROWN_MN_PROCESSING_H
