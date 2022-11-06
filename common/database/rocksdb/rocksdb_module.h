#ifndef COMMON_SERVICE_IMPL_MYSQL_MODULE_H_
#define COMMON_SERVICE_IMPL_MYSQL_MODULE_H_

#ifdef USE_ROCKSDB

#include "common/core_define.h"
#include "common/loop/loop_pool.h"
#include "common/module_base.h"

#include "rocksdb/db.h"

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
    
private:
    void InitLoopsRocksDb(uint64_t nThreadNum, const std::string& strDBBasePath);

private:
    void Test();

private:
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

#endif // COMMON_SERVICE_MYSQL_MODULE_H_
