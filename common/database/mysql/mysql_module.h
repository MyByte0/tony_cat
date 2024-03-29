#ifndef COMMON_SERVICE_IMPL_MYSQL_MODULE_H_
#define COMMON_SERVICE_IMPL_MYSQL_MODULE_H_

#ifdef USE_MYSQL

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <mysql/mysql.h>

#include <functional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "common/core_define.h"
#include "common/loop/loop_pool.h"
#include "common/module_base.h"
#include "common/utility/crc.h"

TONY_CAT_SPACE_BEGIN

class XmlConfigModule;
class ServiceGovernmentModule;

class MysqlQuery {};

class MysqlModule : public ModuleBase {
public:
    explicit MysqlModule(ModuleManager* pModuleManager);
    virtual ~MysqlModule();

    void BeforeInit() override;
    void OnInit() override;
    void AfterStop() override;

public:
    enum : int {
        kArgNumMax = 100,
        kArgLenMax = 128000000,
    };

    typedef std::unordered_map<std::string, std::vector<std::string>>
        MysqlSelectResultMap;
    // get return: (nError, mapResult)
    typedef std::function<void(int32_t, MysqlSelectResultMap& mapResult)>
        MysqlSelectCb;
    // get return: (nError, nAffectRows, nInsertId)
    typedef std::function<void(int32_t, uint64_t, uint64_t)> MysqlModifyCb;

    // not support biniary data now.
    int32_t QueryModify(uint32_t nIndex, const std::string& strQueryString,
                        const std::vector<std::string>& vecArgs,
                        MysqlModifyCb&& cb);
    int32_t QuerySelectRows(uint32_t nIndex, const std::string& strQueryString,
                            const std::vector<std::string>& vecArgs,
                            MysqlSelectCb&& cb);

    static MYSQL* GetMysqlHandle() {
        return reinterpret_cast<MYSQL*>(t_pMysql);
    }

    void DBLoopExec(const std::string& strIndexKey,
                    std::function<void()>&& funcDo) {
        m_loopPool.Exec(CRC32(strIndexKey, GetCurrentHashSlat()),
                        std::move(funcDo));
    }
    // return: (nError, mapResult)
    std::tuple<int32_t, MysqlSelectResultMap> QuerySelectRowsInCurrentThread(
        const std::string& strQueryString,
        const std::vector<std::string>& vecArgs);
    // return: (nError, nAffectRows, nInsertId)
    std::tuple<int32_t, uint64_t, uint64_t> QueryModifyInCurrentThread(
        const std::string& strQueryString,
        const std::vector<std::string>& vecArgs);

public:
    int32_t MessageLoad(google::protobuf::Message& message);
    int32_t MessageUpdate(google::protobuf::Message& message);
    int32_t MessageDelete(google::protobuf::Message& message);

private:
    const char* GetProtoSqlStringPlaceholder(
        const google::protobuf::FieldDescriptor& fieldDescriptor);

    void AddQueryItemCondition(
        const google::protobuf::Message& message,
        const google::protobuf::FieldDescriptor& fieldDescriptor,
        std::string& strQuerys, std::vector<std::string>& vecArgs);

    void AddQueryMessageCondition(google::protobuf::Message& message,
                                  std::string& strAppendQuerys,
                                  std::vector<std::string>& vecArgs);

    std::string QueryStringReplace(
        const std::string& strQueryString,
        const std::vector<std::vector<char>>& vecBuff);

    int32_t MysqlQuery(MYSQL* pMysqlHandle, const std::string& strQueryString,
                       const std::vector<std::string>& vecArgs);

    const std::string& GetCurrentHashSlat();

private:
    void MysqlInit();

    void OnTest();

private:
    XmlConfigModule* m_pXmlConfigModule = nullptr;
    ServiceGovernmentModule* m_pServiceGovernmentModule = nullptr;

private:
    LoopPool m_loopPool;
    std::string m_strDBInstanceName;
    static THREAD_LOCAL_POD_VAR void* t_pMysql;
};

TONY_CAT_SPACE_END

#else

#include "common/core_define.h"

TONY_CAT_SPACE_BEGIN

class MysqlModule {};

TONY_CAT_SPACE_END

#endif

#endif  // COMMON_SERVICE_MYSQL_MODULE_H_
