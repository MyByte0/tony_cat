#ifdef USE_ROCKSDB

#include "rocksdb_module.h"

#include <atomic>
#include <memory>

#include "common/config/xml_config_module.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "protocol/server_base.pb.h"

TONY_CAT_SPACE_BEGIN

THREAD_LOCAL_POD_VAR void* RocksDBModule::t_pRocksDB = nullptr;

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

    std::string strDBType = "rocksdb";
    auto pDataBaseConfig =
        m_pXmlConfigModule->GetDataBaseConfigDataById(strDBType);
    if (pDataBaseConfig == nullptr) {
        LOG_ERROR("get DataBase Config {}", strDBType);
        return;
    }
    std::string strPath = pDataBaseConfig->strAddress;
    InitLoopsRocksDb(strPath);
}

void RocksDBModule::AfterStop() {
    m_loopPool.Broadcast([=, this]() {
        if (rocksdb::DB* db = GetThreadRocksDB(); db != nullptr) {
            db->Close();
            db = nullptr;
        }
    });
    m_loopPool.Stop();
}

rocksdb::DB* RocksDBModule::GetThreadRocksDB() {
    rocksdb::DB* db = reinterpret_cast<rocksdb::DB*>(t_pRocksDB);
    return db;
}

bool RocksDBModule::CheckDBBaseData(const std::string& strDBBasePath) {
    // \TODO: check rocks database schema and table schema
    return true;
}

void RocksDBModule::RemapDBData() {
    // \TODO remap onchange rocks database schema / table schema
}

void RocksDBModule::InitLoopsRocksDb(const std::string& strDBBasePath) {
    if (CheckDBBaseData(strDBBasePath) == false) {
        LOG_ERROR("rocksdb save infomation change, please remap this database");
        return;
    }

    std::shared_ptr<std::atomic<int32_t>> pDBIndex =
        std::make_shared<std::atomic<int32_t>>(0);
    m_loopPool.Start(1);
    m_loopPool.Broadcast([=, this]() {
        rocksdb::DB** db = reinterpret_cast<rocksdb::DB**>(&t_pRocksDB);
        rocksdb::Options options;
        options.create_if_missing = true;
        std::string strDbPath = strDBBasePath;
        auto indexDb = (*pDBIndex)++;
        strDbPath.append(std::to_string(indexDb));
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
    std::string strDbPath = "/data/test";
    InitLoopsRocksDb(strDbPath);

    m_loopPool.Exec(0, [this]() mutable {
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
    });
}

TONY_CAT_SPACE_END

#endif
