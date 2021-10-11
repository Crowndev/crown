// Copyright (c) 2021 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <nft/nftdb.h>

leveldb::DB* NftDb::nftStoreDb()
{
    options.create_if_missing = true;

    fs::path nftStoreHnd = GetDataDir() / dbName;
    leveldb::Status status = leveldb::DB::Open(options, nftStoreHnd.string().c_str(), &db);
    if (!status.ok()) {
        LogPrintf("%s - Can't open %s, exiting..\n", __func__, nftStoreHnd.string().c_str());
        delete db;
        StartShutdown();
    }

    return db;
}

void NftDb::nftStoreRead()
{
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        // cout << it->key().ToString() << ": "  << it->value().ToString() << endl;
    }
    assert(it->status().ok());
    delete it;
}

void NftDb::nftStoreWrite()
{
}

bool NftDb::haveTokenId(uint256& id)
{
    for (auto& l : m_nfttoken) {
        if (id == l.getTokenId())
            return true;
    }
    return false;
}

bool NftDb::haveProtocolId(uint64_t& id)
{
    for (auto& l : m_nftprotocol) {
        if (id == l.getTokenProtocolId())
            return true;
    }
    return false;
}


bool NftDb::haveProtocolName(std::string protocolName)
{
    for (auto& l : m_nftprotocol) {
        if (protocolName == l.getTokenProtocolName())
            return true;
    }
    return false;
}
