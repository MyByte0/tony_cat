#include "mysql_module.h"

#include "common/config/xml_config_module.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "protocol/server_base.pb.h"

TONY_CAT_SPACE_BEGIN

THREAD_LOCAL_POD_VAR void* MysqlModule::t_pMysql = nullptr;

MysqlModule::MysqlModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

MysqlModule::~MysqlModule() { }

void MysqlModule::BeforeInit()
{
    m_pXmlConfigModule = FIND_MODULE(m_pModuleManager, XmlConfigModule);

    // MysqlTest();
}

void MysqlModule::AfterStop()
{
    m_loopPool.Broadcast([this]() {
        mysql_close(GetMysqlHandle());
    });

    m_loopPool.Stop();
}

// test example
void MysqlModule::MysqlTest()
{
    auto pDataBaseConfig = m_pXmlConfigModule->GetDataBaseConfigDataById(10001);
    if (pDataBaseConfig == nullptr) {
        LOG_ERROR("not find db config");
        return;
    }

    auto strIP = pDataBaseConfig->strIP;
    auto nPort = pDataBaseConfig->nPort;
    auto strPassword = pDataBaseConfig->strPassword;
    auto strUser = pDataBaseConfig->strUser;

    int nThreadNum = 2;
    m_loopPool.Start(nThreadNum);
    m_loopPool.Broadcast([=, this]() {
        t_pMysql = mysql_init(nullptr);
        if (t_pMysql == nullptr) {
            LOG_ERROR("mysql init error");
            return;
        }

        if (mysql_real_connect(GetMysqlHandle(), strIP.c_str(), strUser.c_str(),
                strPassword.c_str(), nullptr, 0, nullptr, 0)
            == nullptr) {
            LOG_ERROR("mysql connect error:{} {}", mysql_errno(GetMysqlHandle()), mysql_error(GetMysqlHandle()));
            return;
        }

        char cArg = 1;
        mysql_options(GetMysqlHandle(), MYSQL_OPT_RECONNECT, &cArg);
        QuerySelectRows(0, std::string("select * from test.website where id < {}"), std::vector<std::string> { "1000" }, nullptr);
        QueryModify(0, std::string("insert into test.website (id, name) values({}, '{}') on duplicate key update name='{}'"), std::vector<std::string> { "121", "ok", "ok" }, nullptr);
    });
}

int32_t MysqlModule::QueryModify(uint32_t nIndex, const std::string& strQueryString, const std::vector<std::string>& vecArgs,
    const MysqlModifyCb& cb)
{
    auto& loop = Loop::GetCurrentThreadLoop();
    m_loopPool.Exec(nIndex, [this, strQueryString, vecArgs, &loop, cb = std::move(cb)]() {
        auto pMysqlHandle = GetMysqlHandle();
        int32_t nError = 0;
        int64_t nAffectRows = 0;
        int64_t nInsertId = 0;
        do {
            if ((nError = MysqlQuery(pMysqlHandle, strQueryString, vecArgs)) != 0) {
                LOG_ERROR("query error, aueryString:{}", strQueryString);
                break;
            }

            nAffectRows = mysql_affected_rows(pMysqlHandle);
            nInsertId = mysql_insert_id(pMysqlHandle);
        } while (false);

        if (cb != nullptr) {
            loop.Exec([cb = std::move(cb), nError, nAffectRows, nInsertId]() {
                cb(nError, nAffectRows, nInsertId);
            });
        }
    });

    return 0;
}

int32_t MysqlModule::QuerySelectRows(uint32_t nIndex, const std::string& strQueryString, const std::vector<std::string>& vecArgs,
    const MysqlSelectCb& cb)
{
    auto& loop = Loop::GetCurrentThreadLoop();
    m_loopPool.Exec(nIndex,
        [this, strQueryString, vecArgs, &loop, cb = std::move(cb)]() {
            int32_t nError = 0;
            std::unordered_map<std::string, std::vector<std::string>> mapResult;
            auto pMysqlHandle = GetMysqlHandle();
            do {
                if ((nError = MysqlQuery(pMysqlHandle, strQueryString, vecArgs)) != 0) {
                    LOG_ERROR("query error, aueryString:{}", strQueryString);
                    break;
                }

                std::vector<std::string> vecFieldName;
                MYSQL_ROW row;
                MYSQL_FIELD* field;
                auto result = mysql_store_result(pMysqlHandle);
                int32_t num_fields = mysql_num_fields(result);
                while ((row = mysql_fetch_row(result))) {
                    unsigned long* length = mysql_fetch_lengths(result);
                    for (int i = 0; i < num_fields; i++) {
                        if (i == 0) {
                            while (field = mysql_fetch_field(result)) {
                                vecFieldName.emplace_back(field->name);
                            }
                        }

                        const char* pRow = row[i];
                        if (pRow != nullptr) {
                            mapResult[vecFieldName[i]].emplace_back(pRow, length[i]);
                        } else {
                            mapResult[vecFieldName[i]].emplace_back("");
                        }
                    }
                }
            } while (false);

            if (cb != nullptr) {
                loop.Exec([cb = std::move(cb), nError, mapResult = std::move(mapResult)]() mutable { cb(nError, mapResult); });
            }
            return;
        });

    return 0;
}

std::string MysqlModule::QueryStringReplace(const std::string& strQueryString, const std::vector<std::string>& vecReplaces)
{
    if (vecReplaces.empty()) {
        return strQueryString;
    }

    std::string strResult;
    std::size_t nLenRead = strQueryString.length() - 1;
    std::size_t nLastRead = 0;
    std::size_t iReplace = 0;
    for (size_t i = 0; i < nLenRead; i++) {
        if (strQueryString[i] == '{' && strQueryString[i + 1] == '}') {
            ++i;
            strResult.append(strQueryString, nLastRead, i - 1 - nLastRead);
            strResult.append(vecReplaces[iReplace]);
            nLastRead = i + 1;
            if (++iReplace >= vecReplaces.size()) {
                break;
            }
        }
    }

    strResult.append(strQueryString, nLastRead, nLenRead - 1 - nLastRead);
    return strResult;
}

std::string MysqlModule::QueryStringReplace(const std::string& strQueryString, const char buffReplaces[][kArgLenMax], unsigned long buffLengths[], std::size_t nBuffSize)
{
    if (nBuffSize == 0) {
        return strQueryString;
    }

    std::string strResult;
    std::size_t nLenRead = strQueryString.length() - 1;
    std::size_t nLastRead = 0;
    std::size_t iReplace = 0;
    for (size_t i = 0; i < nLenRead; i++) {
        if (strQueryString[i] == '{' && strQueryString[i + 1] == '}') {
            ++i;
            strResult.append(strQueryString, nLastRead, i - 1 - nLastRead);
            strResult.append(buffReplaces[iReplace], buffLengths[iReplace]);
            nLastRead = i + 1;
            if (++iReplace >= nBuffSize) {
                break;
            }
        }
    }

    strResult.append(strQueryString, nLastRead, nLenRead - 1 - nLastRead);
    return strResult;
}

int32_t MysqlModule::MysqlQuery(MYSQL* pMysqlHandle, const std::string& strQueryString, const std::vector<std::string>& vecArgs)
{
    if (vecArgs.size() >= kArgNumMax) {
        LOG_ERROR("vecArgs size, query: {}", strQueryString);
        return Pb::SSMessageCode::ss_msg_cache_length_overflow;
    }

    char buffField[kArgNumMax][kArgLenMax];
    unsigned long buffLength[kArgNumMax];

    int32_t iArgs = 0;
    for (; iArgs < vecArgs.size(); ++iArgs) {
        auto& strArg = vecArgs[iArgs];
        if (strArg.length() * 2 + 1 > kArgLenMax) [[unlikely]] {
            LOG_ERROR("query arg:{} aueryString:{}", strArg, strQueryString);
            return Pb::SSMessageCode::ss_msg_arg_length_overflow;
        }
        buffLength[iArgs] = mysql_real_escape_string(pMysqlHandle, buffField[iArgs], strArg.c_str(), static_cast<unsigned long>(strArg.length()));
    }

    std::string strQuery = QueryStringReplace(strQueryString, buffField, buffLength, iArgs);
    if (mysql_real_query(pMysqlHandle, strQuery.c_str(), static_cast<unsigned long>(strQuery.length()))) {
        LOG_ERROR("query test:{}, {}, aueryString:{}", mysql_errno(pMysqlHandle), mysql_error(pMysqlHandle), strQuery);
        return Pb::SSMessageCode::ss_msg_error_sql;
    }

    return Pb::SSMessageCode::ss_msg_success;
}

TONY_CAT_SPACE_END
