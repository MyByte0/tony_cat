#ifndef COMMON_SERVICE_MYSQL_MODULE_H_
#define COMMON_SERVICE_MYSQL_MODULE_H_

#include "common/core_define.h"
#include "common/loop/loop_pool.h"
#include "common/module_base.h"

#include <mysql/mysql.h>

#include <functional>

TONY_CAT_SPACE_BEGIN

class XmlConfigModule;

class MysqlQuery {

};

class MysqlModule : public ModuleBase {
public:
    MysqlModule(ModuleManager* pModuleManager);
    virtual ~MysqlModule();

    virtual void BeforeInit() override;
    virtual void AfterStop() override;

public:
    enum : int {
        kArgNumMax = 80,
        kArgLenMax = 2000,
        kQueryLenMax = 20000,
    };

    // get return: (nError, mapResult)
    typedef std::function<void(int32_t, std::unordered_map<std::string, std::vector<std::string>>& mapResult)> MysqlSelectCb;
    // get return: (nError, nAffectRows, nInsertId)
    typedef std::function<void(int32_t, uint64_t, uint64_t)> MysqlModifyCb;
    
    // not support biniary data now.
    int32_t QueryModify(uint32_t nIndex, const std::string& strQueryString, const std::vector<std::string>& vecArgs, const MysqlModifyCb& cb);
    int32_t QuerySelectRows(uint32_t nIndex, const std::string& strQueryString, const std::vector<std::string>& vecArgs, const MysqlSelectCb& cb);


     static MYSQL* GetMysqlHandle()
    {
        return reinterpret_cast<MYSQL*>(t_pMysql);
    }

private:
    std::string QueryStringReplace(const std::string& strQueryString, const std::vector<std::string>& vecReplaces);
    std::string QueryStringReplace(const std::string& strQueryString, const char buffReplaces[][kArgLenMax], unsigned long buffLengths[], std::size_t nBuffSize);

    int32_t MysqlQuery(MYSQL* pMysqlHandle, const std::string& strQueryString, const std::vector<std::string>& vecArgs);

private:
    void MysqlTest();

private:
    XmlConfigModule* m_pXmlConfigModule = nullptr;

private:
    LoopPool m_loopPool;
    static THREAD_LOCAL_POD_VAR void* t_pMysql;
};

TONY_CAT_SPACE_END

#endif // COMMON_SERVICE_MYSQL_MODULE_H_
