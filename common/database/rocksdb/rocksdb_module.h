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

TONY_CAT_SPACE_BEGIN

class XmlConfigModule;

class RocksDBModule : public ModuleBase {
 public:
    explicit RocksDBModule(ModuleManager* pModuleManager);
    virtual ~RocksDBModule();

    void BeforeInit() override;
    void AfterStop() override;

 public:
    rocksdb::DB* GetThreadRocksDB();

    LoopPool& GetLoopPool() { return m_loopPool; }

 public:
    int32_t MessageLoad(google::protobuf::Message& message);
    int32_t MessageUpdate(google::protobuf::Message& message);
    int32_t MessageDelete(google::protobuf::Message& message);

 private:
    void InitLoopsRocksDb(const std::string& strDBBasePath);

    bool CheckDBBaseData(const std::string& strDBBasePath);

    void RemapDBData();

 private:
    void Test();

 private:
    PbMessageKVHandle m_PbMessageKVHandle;
    std::string m_strDBInstanceName;
    LoopPool m_loopPool;
    static THREAD_LOCAL_POD_VAR void* t_pRocksDB;

 private:
    XmlConfigModule* m_pXmlConfigModule = nullptr;
};

TONY_CAT_SPACE_END

#else

TONY_CAT_SPACE_BEGIN

class RocksDBModule {};

TONY_CAT_SPACE_END
#endif

#endif  // COMMON_DATABASE_ROCKSDB_ROCKSDB_MODULE_H_
