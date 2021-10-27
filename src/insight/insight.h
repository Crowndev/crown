// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2017-2021 The Particl Core developers
// Copyright (c) 2017-2021 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_INSIGHT_INSIGHT_H
#define CROWN_INSIGHT_INSIGHT_H

#include <threadsafety.h>
#include <script/standard.h>

#include <amount.h>
#include <sync.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <utility>

extern RecursiveMutex cs_main;

extern bool fAddressIndex;
extern bool fSpentIndex;
extern bool fTimestampIndex;
extern bool fBalancesIndex;

class CTxOut;
struct CAsset;
class CScript;
class uint256;
class uint160;
class CTxMemPool;
class CMempoolAddressDeltaKey;
class CMempoolAddressDelta;
class BlockBalances;
struct CAddressIndexKey;
struct CAddressUnspentKey;
struct CAddressUnspentValue;
struct CSpentIndexKey;
struct CSpentIndexValue;

bool ExtractIndexInfo(const CScript *pScript, int &scriptType, std::vector<uint8_t> &hashBytes);
bool ExtractIndexInfo(const CTxOut *out, int &scriptType, std::vector<uint8_t> &hashBytes, CAmount &nValue, CAsset &nAsset, const CScript *&pScript);

/** Functions for insight block explorer */
bool GetTimestampIndex(const unsigned int &high, const unsigned int &low, const bool fActiveOnly, std::vector<std::pair<uint256, unsigned int> > &hashes) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
bool GetSpentIndex(CSpentIndexKey &key, CSpentIndexValue &value, const CTxMemPool *pmempool);
bool GetAddressIndex(uint160 addressHash, int type, std::string sAssetName,
                     std::vector<std::pair<CAddressIndexKey, CAmount> > &addressIndex,
                     int start = 0, int end = 0);
bool GetAddressUnspent(uint160 addressHash, int type, std::string sAssetName, 
                       std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &unspentOutputs);
bool GetBlockBalances(const uint256 &block_hash, BlockBalances &balances);

bool getAddressFromIndex(const int &type, const uint160 &hash, std::string &address);

bool HashOnchainActive(const uint256 &hash);
bool GetIndexKey(const CTxDestination &dest, uint160 &hashBytes, int &type);
bool heightSort(std::pair<CAddressUnspentKey, CAddressUnspentValue> a,
                std::pair<CAddressUnspentKey, CAddressUnspentValue> b);

bool timestampSort(std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> a,
                   std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> b);
#endif // CROWN_INSIGHT_INSIGHT_H
