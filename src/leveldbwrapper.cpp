// Copyright (c) 2012-2014 The Crown developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <leveldbwrapper.h>
#include <boost/filesystem.hpp>

#include <leveldb/cache.h>
#include <leveldb/env.h>
#include <leveldb/filter_policy.h>
#include <memenv.h>

void HandleError(const leveldb::Status& status) // throw(leveldb_error)
{
    if (status.ok())
        return;
    LogPrintf("%s\n", status.ToString());
    if (status.IsCorruption())
        throw leveldb_error("Database corrupted");
    if (status.IsIOError())
        throw leveldb_error("Database I/O error");
    if (status.IsNotFound())
        throw leveldb_error("Database entry missing");
    throw leveldb_error("Unknown database error");
}

static leveldb::Options GetOptions(size_t nCacheSize)
{
    leveldb::Options options;
    options.block_cache = leveldb::NewLRUCache(nCacheSize / 2);
    options.write_buffer_size = nCacheSize / 4; // up to two write buffers may be held in memory simultaneously
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options.compression = leveldb::kNoCompression;
    options.max_open_files = 64;
    if (leveldb::kMajorVersion > 1 || (leveldb::kMajorVersion == 1 && leveldb::kMinorVersion >= 16)) {
        // LevelDB versions before 1.16 consider short writes to be corruption. Only trigger error
        // on corruption in later versions.
        options.paranoid_checks = true;
    }
    return options;
}

CLevelDBWrapper::CLevelDBWrapper(const fs::path& m_path, size_t nCacheSize, bool fMemory, bool fWipe)
{
    penv = NULL;
    readoptions.verify_checksums = true;
    iteroptions.verify_checksums = true;
    iteroptions.fill_cache = false;
    syncoptions.sync = true;
    options = GetOptions(nCacheSize);
    options.create_if_missing = true;
    if (fMemory) {
        penv = leveldb::NewMemEnv(leveldb::Env::Default());
        options.env = penv;
    } else {
        if (fWipe) {
            LogPrintf("Wiping LevelDB in %s\n", fs::PathToString(m_path));
            leveldb::Status result = leveldb::DestroyDB(fs::PathToString(m_path), options);
            HandleError(result);
        }
        // TODO fix
        //TryCreateDirectory(path);
        LogPrintf("Opening LevelDB in %s\n", fs::PathToString(m_path));
    }
    leveldb::Status status = leveldb::DB::Open(options, fs::PathToString(m_path), &pdb);
    HandleError(status);
    LogPrintf("Opened LevelDB successfully\n");
}

CLevelDBWrapper::~CLevelDBWrapper()
{
    delete pdb;
    pdb = NULL;
    delete options.filter_policy;
    options.filter_policy = NULL;
    delete options.block_cache;
    options.block_cache = NULL;
    delete penv;
    options.env = NULL;
}

bool CLevelDBWrapper::WriteBatch(CLevelDBBatch& batch, bool fSync) // throw(leveldb_error)
{
    leveldb::Status status = pdb->Write(fSync ? syncoptions : writeoptions, &batch.batch);
    HandleError(status);
    return true;
}

TransactionLevelDBWrapper::TransactionLevelDBWrapper(const std::string & dbName, size_t nCacheSize, bool fMemory, bool fWipe)
: m_db(GetDataDir() / fs::PathFromString(dbName), nCacheSize, fMemory, fWipe)
, m_dbTransaction(m_db)
{
}
