#ifndef COMMON_DATABASE_ROCKSDB_ROCKSDB_MODULE_H_
#define COMMON_DATABASE_ROCKSDB_ROCKSDB_MODULE_H_

#ifdef USE_ROCKSDB

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <rocksdb/db.h>

#include <string>

#include "common/core_define.h"
#include "common/database/db_utils.h"
#include "common/loop/loop_pool.h"
#include "common/module_base.h"
#include "common/utility/crc.h"

TONY_CAT_SPACE_BEGIN

class XmlConfigModule;
struct DataBaseConfigData;

class ServiceGovernmentModule;

class RocksDBModule : public ModuleBase {
public:
    explicit RocksDBModule(ModuleManager* pModuleManager);
    virtual ~RocksDBModule();

    void BeforeInit() override;
    void OnInit() override;
    void AfterStop() override;

public:
    rocksdb::DB* GetThreadRocksDB();

    void DBLoopExec(const std::string strIndexKey,
                    std::function<void()>&& funcDo) {
        m_loopPool.Exec(CRC32(strIndexKey, GetCurrentHashSlat()),
                        std::move(funcDo));
    }

public:
    int32_t MessageLoad(google::protobuf::Message& message);
    int32_t MessageUpdate(google::protobuf::Message& message);
    int32_t MessageDelete(google::protobuf::Message& message);

private:
    void InitRocksDb(const DataBaseConfigData& dataBaseConfigData);
    void StartRocksDb(const DataBaseConfigData& dataBaseConfigData);

    rocksdb::DB* CreateDBToc(const DataBaseConfigData& dataBaseConfigData,
                             const std::string& strPathSub);
    void DestoryDBToc(const DataBaseConfigData& dataBaseConfigData,
                      rocksdb::DB* pRocksDB);

    bool CheckDBBaseData(const DataBaseConfigData& dataBaseConfigData);

    void RemapDBDataAndStart(const DataBaseConfigData& dataBaseConfigData);

    const std::string& GetCurrentHashSlat();

private:
    void Test();

private:
    PbMessageKVHandle m_PbMessageKVHandle;
    std::string m_strDBInstanceName;
    LoopPool m_loopPool;
    static THREAD_LOCAL_POD_VAR void* t_pRocksDB;

private:
    XmlConfigModule* m_pXmlConfigModule = nullptr;
    ServiceGovernmentModule* m_pServiceGovernmentModule = nullptr;
};

TONY_CAT_SPACE_END

#else

TONY_CAT_SPACE_BEGIN

class RocksDBModule {};

TONY_CAT_SPACE_END
#endif

#endif  // COMMON_DATABASE_ROCKSDB_ROCKSDB_MODULE_H_
