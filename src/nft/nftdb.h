// Copyright (c) 2021 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NFTDB_H
#define NFTDB_H

#include <nft/token.h>
#include <shutdown.h>
#include <util/system.h>
#include <validation.h>

class NftDb;
class NfToken;
class NfTokenProtocol;

class NftDb {
private:
    leveldb::DB* db;
    leveldb::Options options;
    const std::string dbName = "nftstore";
    std::vector<NfToken> m_nfttoken;
    std::vector<NfTokenProtocol> m_nftprotocol;    
public:
    leveldb::DB* nftStoreDb();
    void nftStoreRead();
    void nftStoreWrite();
    bool haveTokenId(uint256& id);
    bool haveProtocolId(uint64_t& id);
    bool haveProtocolName(std::string protocolName);
};

#endif // NFTDB_H
