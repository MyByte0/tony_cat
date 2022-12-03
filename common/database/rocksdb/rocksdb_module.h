#ifndef COMMON_SERVICE_IMPL_ROCKSDB_MODULE_H_
#define COMMON_SERVICE_IMPL_ROCKSDB_MODULE_H_

#ifdef USE_ROCKSDB

#include "common/core_define.h"
#include "common/loop/loop_pool.h"
#include "common/module_base.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <rocksdb/db.h>

TONY_CAT_SPACE_BEGIN

class XmlConfigModule;


class RocksDBModule : public ModuleBase {
public:
    explicit RocksDBModule(ModuleManager* pModuleManager);
    virtual ~RocksDBModule();

    virtual void BeforeInit() override;
    virtual void AfterStop() override;

public:
    rocksdb::DB* GetThreadRocksDB();

    LoopPool& GetLoopPool() { return m_loopPool; }
    
public:
    int32_t LoadMessage(google::protobuf::Message& message);
    int32_t UpdateMessage(google::protobuf::Message& message);
    int32_t DeleteMessage(google::protobuf::Message& message);

private:
    void InitLoopsRocksDb(uint64_t nThreadNum, const std::string& strDBBasePath);

    bool CheckDBBaseData(uint64_t nThreadNum, const std::string& strDBBasePath);

    void RemapDBData();

private:
    void Test();

private:
    std::string m_strDBInstanceName;
    LoopPool m_loopPool;
    static THREAD_LOCAL_POD_VAR void* t_pRocksDB;

private:
    XmlConfigModule* m_pXmlConfigModule = nullptr;
};

TONY_CAT_SPACE_END

#else 

TONY_CAT_SPACE_BEGIN

class RocksDBModule { };

TONY_CAT_SPACE_END
#endif

#endif // COMMON_SERVICE_IMPL_ROCKSDB_MODULE_H_
