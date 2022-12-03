#ifdef USE_ROCKSDB

#include "rocksdb_module.h"

#include "common/config/xml_config_module.h"
#include "common/database/db_utils.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"

#include <atomic>
#include <memory>

TONY_CAT_SPACE_BEGIN

THREAD_LOCAL_POD_VAR void* RocksDBModule::t_pRocksDB = nullptr;

RocksDBModule::RocksDBModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

RocksDBModule::~RocksDBModule() { }

void RocksDBModule::BeforeInit()
{
    m_pXmlConfigModule = FIND_MODULE(m_pModuleManager, XmlConfigModule);

    std::string strPath = "/data/tonycat";
    // strPath.append(std::to_sring(serverIndex));
    InitLoopsRocksDb(2, strPath);
}

void RocksDBModule::AfterStop()
{
    m_loopPool.Broadcast([=, this]() {
        if (rocksdb::DB* db = GetThreadRocksDB(); db != nullptr) {
            db->Close();
            db = nullptr;
        }
    });
    m_loopPool.Stop();
}

rocksdb::DB* RocksDBModule::GetThreadRocksDB()
{
    rocksdb::DB* db = reinterpret_cast<rocksdb::DB*>(t_pRocksDB);
    return db;
}

bool RocksDBModule::CheckDBBaseData(uint64_t nThreadNum, const std::string& strDBBasePath)
{
    // \TODO: check rocks database schema and table schema
    return true;
}

void RocksDBModule::RemapDBData()
{
    // \TODO remap onchange rocks database schema / table schema
}

void RocksDBModule::InitLoopsRocksDb(uint64_t nThreadNum, const std::string& strDBBasePath)
{
    if (CheckDBBaseData(nThreadNum, strDBBasePath) == false) {
        LOG_ERROR("rocksdb save infomation change, please remap this database");
        return;
    }

    std::shared_ptr<std::atomic<int32_t>> pDBIndex = std::make_shared<std::atomic<int32_t>>(0);
    m_loopPool.Start(nThreadNum);
    m_loopPool.Broadcast([=, this]() {
        rocksdb::DB** db = reinterpret_cast<rocksdb::DB**>(&t_pRocksDB);
        rocksdb::Options options;
        options.create_if_missing = true;
        std::string strDbPath = strDBBasePath;
        auto indexDb = ++(*pDBIndex);
        strDbPath.append(std::to_string(indexDb));
        rocksdb::Status status = rocksdb::DB::Open(options, strDbPath.c_str(), db);

        // std::string a;
        // db->Put(rocksdb::WriteOptions(), a, a);

        // db->Get(rocksdb::ReadOptions(), a, &a);

        // db->Delete(rocksdb::WriteOptions(), a);
    });
}

int32_t RocksDBModule::LoadMessage(google::protobuf::Message& message)
{
    LoadMessageOnKV(message, [this](const std::string& strKey, std::string& strVal) mutable { GetThreadRocksDB()->Get(rocksdb::ReadOptions(), strKey, &strVal);  return 0; });
    return 0;
}

int32_t RocksDBModule::UpdateMessage(google::protobuf::Message& message)
{
    UpdateMessageOnKV(message,
        [this](const std::string& strKey, std::string& strVal) mutable { GetThreadRocksDB()->Get(rocksdb::ReadOptions(), strKey, &strVal);  return 0; },
        [this](const std::string& strKey, const std::string& strVal) mutable { GetThreadRocksDB()->Put(rocksdb::WriteOptions(), strKey, strVal);  return 0; },
        [this](const std::string& strKey) mutable { GetThreadRocksDB()->Delete(rocksdb::WriteOptions(), strKey);  return 0; });
 
    return 0;
}

int32_t RocksDBModule::DeleteMessage(google::protobuf::Message& message)
{
    DeleteMessageOnKV(
        message,
        [this](const std::string& strKey, std::string& strVal) mutable { GetThreadRocksDB()->Get(rocksdb::ReadOptions(), strKey, &strVal);  return 0; },
        [this](const std::string& strKey, const std::string& strVal) mutable { GetThreadRocksDB()->Put(rocksdb::WriteOptions(), strKey, strVal);  return 0; },
        [this](const std::string& strKey) mutable { GetThreadRocksDB()->Delete(rocksdb::WriteOptions(), strKey);  return 0; });

    return 0;
}

void RocksDBModule::Test()
{
    auto pDataBaseConfig = m_pXmlConfigModule->GetDataBaseConfigDataById(10002);
    if (pDataBaseConfig == nullptr) {
        LOG_ERROR("not find db config");
        return;
    }

    auto strPassword = pDataBaseConfig->strPassword;
    auto strUser = pDataBaseConfig->strUser;

    int nThreadNum = 2;
    std::string strDbPath = "/tmp/testdb";
    InitLoopsRocksDb(nThreadNum, strDbPath);
}

TONY_CAT_SPACE_END

#endif
