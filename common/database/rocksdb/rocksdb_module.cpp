#ifdef USE_ROCKSDB

#include "rocksdb_module.h"

#include <atomic>
#include <filesystem>
#include <latch>
#include <memory>

#include "common/config/xml_config_module.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/service/service_government_module.h"
#include "protocol/server_base.pb.h"

TONY_CAT_SPACE_BEGIN

THREAD_LOCAL_POD_VAR void* RocksDBModule::t_pRocksDB = nullptr;

static const char* s_strDataPathPrefix = "shard_";
static const char* s_strMetaPathPrefix = "meta";

RocksDBModule::RocksDBModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager) {
    m_PbMessageKVHandle.funGet = [this](const std::string& strKey,
                                        std::string& strVal) mutable {
        GetThreadRocksDB()->Get(rocksdb::ReadOptions(), strKey, &strVal);
        return 0;
    };
    m_PbMessageKVHandle.funPut = [this](const std::string& strKey,
                                        const std::string& strVal) mutable {
        GetThreadRocksDB()->Put(rocksdb::WriteOptions(), strKey, strVal);
        return 0;
    };
    m_PbMessageKVHandle.funDel = [this](const std::string& strKey) mutable {
        GetThreadRocksDB()->Delete(rocksdb::WriteOptions(), strKey);
        return 0;
    };
}

RocksDBModule::~RocksDBModule() {}

void RocksDBModule::BeforeInit() {
    m_pXmlConfigModule = FIND_MODULE(m_pModuleManager, XmlConfigModule);
    m_pServiceGovernmentModule =
        FIND_MODULE(m_pModuleManager, ServiceGovernmentModule);

    std::string strDBType = "rocksdb";
    auto pDataBaseConfig =
        m_pXmlConfigModule->GetDataBaseConfigDataById(strDBType);
    if (pDataBaseConfig == nullptr) {
        LOG_ERROR("get DataBase Config {}", strDBType);
        return;
    }

    if (pDataBaseConfig->strAddress.empty()) {
        LOG_ERROR("get DataBase strAddress empty {}", strDBType);
        return;
    }

    m_strDBInstanceName = m_pServiceGovernmentModule->GetServerName();
    m_strDBInstanceName.append(
        std::to_string(m_pServiceGovernmentModule->GetServerId()));

    if (!std::filesystem::exists(pDataBaseConfig->strAddress)) {
        std::filesystem::create_directories(pDataBaseConfig->strAddress);
    }

    if (pDataBaseConfig->strAddress.back() != '/') {
        const_cast<std::string&>(pDataBaseConfig->strAddress).append("/");
    }

    InitRocksDb(*pDataBaseConfig);
}

void RocksDBModule::OnInit() {}

void RocksDBModule::AfterStop() {
    m_loopPool.Broadcast([=, this]() {
        if (rocksdb::DB* db = GetThreadRocksDB(); db != nullptr) {
            db->Close();
            delete db;
            db = nullptr;
        }
    });
    m_loopPool.Stop();
}

rocksdb::DB* RocksDBModule::GetThreadRocksDB() {
    rocksdb::DB* db = reinterpret_cast<rocksdb::DB*>(t_pRocksDB);
    return db;
}

const std::string& RocksDBModule::GetCurrentHashSlat() {
    return m_strDBInstanceName;
}

rocksdb::DB* RocksDBModule::CreateDBToc(
    const DataBaseConfigData& dataBaseConfigData,
    const std::string& strPathSub) {
    rocksdb::DB* pRocksDB = nullptr;
    rocksdb::Options options;
    options.create_if_missing = true;
    std::string strDbPath = dataBaseConfigData.strAddress;
    strDbPath.append(strPathSub);

    rocksdb::Status status =
        rocksdb::DB::Open(options, strDbPath.c_str(), &pRocksDB);
    if (!status.ok()) {
        LOG_ERROR("open db error, path: {}, error: {}", strDbPath,
                  status.ToString());
    }
    return pRocksDB;
}

void RocksDBModule::DestoryDBToc(const DataBaseConfigData& dataBaseConfigData,
                                 rocksdb::DB* pRocksDB) {
    if (pRocksDB != nullptr) {
        pRocksDB->Close();
        delete pRocksDB;
    }
}

bool RocksDBModule::CheckDBBaseData(
    const DataBaseConfigData& dataBaseConfigData) {
    auto pRocksDB = CreateDBToc(dataBaseConfigData, s_strMetaPathPrefix);
    if (pRocksDB == nullptr) {
        LOG_ERROR("CreateDBToc fail");
        return false;
    }

    // \TODO: make a tool, auto gen meta data define
    std::string strValShardNum;
    pRocksDB->Get(rocksdb::ReadOptions(), "shared_num", &strValShardNum);
    std::string strValShardPrefix;
    pRocksDB->Get(rocksdb::ReadOptions(), "shared_hash_slat",
                  &strValShardPrefix);

    DestoryDBToc(dataBaseConfigData, pRocksDB);

    if (strValShardNum.empty() ||
        ::atoll(strValShardNum.c_str()) != dataBaseConfigData.nShardNum ||
        strValShardPrefix != GetCurrentHashSlat()) {
        return false;
    }
    return true;
}

void RocksDBModule::RemapDBDataAndStart(
    const DataBaseConfigData& dataBaseConfigData) {
    if (dataBaseConfigData.nShardNum <= 0) {
        LOG_ERROR("dataBaseConfigData nShardNum error, size: {}",
                  dataBaseConfigData.nShardNum);
        return;
    }

    auto pRocksDBMeta = CreateDBToc(dataBaseConfigData, s_strMetaPathPrefix);
    if (pRocksDBMeta == nullptr) {
        LOG_ERROR("CreateDBToc fail");
        return;
    }

    std::string strValShardNum;
    pRocksDBMeta->Get(rocksdb::ReadOptions(), "shared_num", &strValShardNum);
    std::string strValShardPrefix;
    pRocksDBMeta->Get(rocksdb::ReadOptions(), "shared_hash_slat",
                      &strValShardPrefix);

    auto nOldShardNum = ::atoll(strValShardNum.c_str());

    if (nOldShardNum == dataBaseConfigData.nShardNum &&
        strValShardPrefix == GetCurrentHashSlat()) {
        LOG_INFO(
            "dataBaseConfigData nShardNum not changed, ShardNum: {},HashSlat: "
            "{}",
            nOldShardNum, strValShardPrefix);
        return;
    }

    LOG_INFO(
        "dataBaseConfigData nShardNum changed, old ShardNum: {}, new "
        "ShardNum: {}",
        nOldShardNum, dataBaseConfigData.nShardNum);

    std::shared_ptr<std::vector<rocksdb::DB*>> pNewDBIndex =
        std::make_shared<std::vector<rocksdb::DB*>>(
            dataBaseConfigData.nShardNum, nullptr);
    std::shared_ptr<LoopPool> pPutNewloopPool = std::make_shared<LoopPool>();
    pPutNewloopPool->Start(dataBaseConfigData.nShardNum);
    pPutNewloopPool->Broadcast([=, this]() {
        auto index = LoopPool::GetIndexInLoopPool();
        (*pNewDBIndex)[index] =
            CreateDBToc(dataBaseConfigData,
                        std::string("tmp_remap").append(std::to_string(index)));
    });

    if (nOldShardNum != 0) {
        LoopPool readOldloopPool;
        readOldloopPool.Start(nOldShardNum);
        std::latch sigLoad(nOldShardNum);
        readOldloopPool.Broadcast([=, &sigLoad, this]() mutable {
            LOG_INFO("on shard remap.");

            auto nDBIndex = LoopPool::GetIndexInLoopPool();
            auto pOldRocksDB = CreateDBToc(
                dataBaseConfigData, std::string(s_strDataPathPrefix)
                                        .append(std::to_string(nDBIndex)));
            if (pOldRocksDB == nullptr) {
                LOG_ERROR("CreateDBToc fail");
                abort();
                return;
            }

            // scan load old db data
            rocksdb::ReadOptions read_options;
            auto sliceEnd = rocksdb::Slice("\xFF");
            read_options.iterate_upper_bound = &sliceEnd;
            rocksdb::Iterator* iter = pOldRocksDB->NewIterator(read_options);

            // \TODO: memory control
            for (iter->Seek("\x00"); iter->Valid(); iter->Next()) {
                auto strKey = iter->key().ToString();
                auto strValue = iter->value().ToString();

                // \TODO: batch put will better
                // Put Data to new db path
                pPutNewloopPool->Exec(
                    CRC32(GetCurrentHashSlat(), strKey),
                    [strKey = std::move(strKey), strValue = std::move(strValue),
                     pNewDBIndex]() {
                        auto index = LoopPool::GetIndexInLoopPool();
                        auto pNewDB = (*pNewDBIndex)[index];
                        pNewDB->Put(rocksdb::WriteOptions(), strKey, strValue);
                    });
            }

            if (!iter->status().ok()) {
                LOG_ERROR("range read fail: {}", iter->status().ToString());
                abort();
            }

            delete iter;
            DestoryDBToc(dataBaseConfigData, pOldRocksDB);
            sigLoad.count_down();
        });

        // block thread until fiinish
        sigLoad.wait();
        readOldloopPool.Stop();
    }

    pPutNewloopPool->BroadcastAndDone(
        [=, this]() {
            auto index = LoopPool::GetIndexInLoopPool();
            auto pNewDB = (*pNewDBIndex)[index];
            DestoryDBToc(dataBaseConfigData, pNewDB);
        },
        [=, this]() {
            pPutNewloopPool->Stop();

            // \TODO: a switch stat maybe crash
            // add a recovery status

            // remove old
            for (int64_t index = 0; index < nOldShardNum; ++index) {
                std::string strOldPath = dataBaseConfigData.strAddress;
                strOldPath.append(s_strDataPathPrefix)
                    .append(std::to_string(index));
                std::filesystem::remove_all(strOldPath);
            }

            // switch new
            for (int64_t index = 0; index < dataBaseConfigData.nShardNum;
                 ++index) {
                std::string strTmpPath = dataBaseConfigData.strAddress;
                strTmpPath.append(
                    std::string("tmp_remap").append(std::to_string(index)));
                std::string strNewPath = dataBaseConfigData.strAddress;
                strNewPath.append(s_strDataPathPrefix)
                    .append(std::to_string(index));
                std::filesystem::rename(strTmpPath, strNewPath);
            }

            // update meta
            pRocksDBMeta->Put(rocksdb::WriteOptions(), "shared_num",
                              std::to_string(dataBaseConfigData.nShardNum));
            pRocksDBMeta->Put(rocksdb::WriteOptions(), "shared_hash_slat",
                              GetCurrentHashSlat());
            DestoryDBToc(dataBaseConfigData, pRocksDBMeta);
            StartRocksDb(dataBaseConfigData);
        });

    return;
}

void RocksDBModule::InitRocksDb(const DataBaseConfigData& dataBaseConfigData) {
    if (CheckDBBaseData(dataBaseConfigData) == false) {
        LOG_INFO("rocksdb save infomation change, auto remap this database");
        RemapDBDataAndStart(dataBaseConfigData);
        LOG_INFO("rocksdb save infomation change, finish remap this database");
        return;
    } else {
        StartRocksDb(dataBaseConfigData);
    }
}

void RocksDBModule::StartRocksDb(const DataBaseConfigData& dataBaseConfigData) {
    m_loopPool.Start(dataBaseConfigData.nShardNum);
    m_loopPool.Broadcast([=]() {
        rocksdb::DB** db = reinterpret_cast<rocksdb::DB**>(&t_pRocksDB);
        rocksdb::Options options;
        options.create_if_missing = true;
        std::string strDbPath = dataBaseConfigData.strAddress;
        auto indexDb = LoopPool::GetIndexInLoopPool();
        strDbPath.append(s_strDataPathPrefix).append(std::to_string(indexDb));
        rocksdb::Status status =
            rocksdb::DB::Open(options, strDbPath.c_str(), db);
    });
}

int32_t RocksDBModule::MessageLoad(google::protobuf::Message& message) {
    m_PbMessageKVHandle.MessageLoadOnKV(message);
    return 0;
}

int32_t RocksDBModule::MessageUpdate(google::protobuf::Message& message) {
    m_PbMessageKVHandle.MessageUpdateOnKV(message);
    return 0;
}

int32_t RocksDBModule::MessageDelete(google::protobuf::Message& message) {
    m_PbMessageKVHandle.MessageDeleteOnKV(message);
    return 0;
}

void RocksDBModule::Test() {
    DataBaseConfigData dataBaseConfigData;
    dataBaseConfigData.strAddress = "/data/test";
    InitRocksDb(dataBaseConfigData);

    // test
    //m_loopPool.Exec(0, [this]() mutable {
        // Db::KVData msgReq1;
        // msgReq1.mutable_user_data()->set_user_id("user_1");
        ////
        /// msgReqmutable_user_data()->mutable_user_base()->set_user_name("hi");
        // msgReq1.mutable_user_data()->add_user_counts()->set_count_type(1);
        // msgReq1.mutable_user_data()->add_user_counts()->set_count_type(2);
        // msgReq1.mutable_user_data()->add_user_counts()->set_count_type(3);
        // MessageUpdate(*msgReq1.mutable_user_data());

        // Db::KVData msgReq2;
        // msgReq2.mutable_user_data()->set_user_id("user_1");
        ////
        /// msgReqmutable_user_data()->mutable_user_base()->set_user_name("hi");
        // msgReq2.mutable_user_data()->add_user_counts()->set_count_type(1);
        // msgReq2.mutable_user_data()->add_user_counts()->set_count_type(3);
        // MessageDelete(*msgReq2.mutable_user_data());

        // Db::KVData msgRsp1;
        // msgRsp1.mutable_user_data()->set_user_id("user_1");
        // MessageLoad(*msgRsp1.mutable_user_data());
        // msgRsp1.Clear();
    //});
}

TONY_CAT_SPACE_END

#endif
