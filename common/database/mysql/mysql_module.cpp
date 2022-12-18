#ifdef USE_MYSQL

#include "mysql_module.h"

#include "common/config/xml_config_module.h"
#include "common/database/db_utils.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "protocol/db_data.pb.h"
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

    MysqlInit();
}

void MysqlModule::AfterStop()
{
    m_loopPool.Broadcast([this]() {
        mysql_close(GetMysqlHandle());
    });

    m_loopPool.Stop();
}

void MysqlModule::MysqlInit()
{
    std::string strDBType = "mysql";
    auto pDataBaseConfig = m_pXmlConfigModule->GetDataBaseConfigDataById(strDBType);
    if (pDataBaseConfig == nullptr) {
        LOG_ERROR("not find db config: {}", strDBType);
        return;
    }

    auto strIP = pDataBaseConfig->strAddress;
    auto nPort = pDataBaseConfig->nPort;
    auto strPassword = pDataBaseConfig->strPassword;
    auto strUser = pDataBaseConfig->strUser;

    int nThreadNum = 2;
    m_loopPool.Start(nThreadNum);
    m_loopPool.Broadcast([=, this]() {
        if (t_pMysql = mysql_init(nullptr); t_pMysql == nullptr) {
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
        //QuerySelectRows(0, std::string("select * from test.website where id < {}"), std::vector<std::string> { "1000" }, nullptr);
        //QueryModify(0, std::string("insert into test.website (id, name) values({}, '{}') on duplicate key update name='{}'"), std::vector<std::string> { "121", "ok", "ok" }, nullptr);
    });
}

int32_t MysqlModule::QueryModify(uint32_t nIndex, const std::string& strQueryString, const std::vector<std::string>& vecArgs,
    MysqlModifyCb&& cb)
{
    auto& loop = Loop::GetCurrentThreadLoop();
    m_loopPool.Exec(nIndex, [this, strQueryString, vecArgs, &loop, cb = std::move(cb)]() {
        auto pMysqlHandle = GetMysqlHandle();
        assert(nullptr != pMysqlHandle);
        auto [nError, nAffectRows, nInsertId] = QueryModifyInCurrentThread(strQueryString, vecArgs);

        if (cb != nullptr) {
            loop.Exec([cb = std::move(cb), nError, nAffectRows, nInsertId]() {
                cb(nError, nAffectRows, nInsertId);
            });
        }
    });

    return 0;
}

int32_t MysqlModule::QuerySelectRows(uint32_t nIndex, const std::string& strQueryString, const std::vector<std::string>& vecArgs,
    MysqlSelectCb&& cb)
{
    auto& loop = Loop::GetCurrentThreadLoop();
    m_loopPool.Exec(nIndex,
        [this, strQueryString, vecArgs, &loop, cb = std::move(cb)]() {
            auto [nError, mapResult] = QuerySelectRowsInCurrentThread(strQueryString, vecArgs);

            if (cb != nullptr) {
                loop.Exec([cb = std::move(cb), nError, mapResult = std::move(mapResult)]() mutable { cb(nError, mapResult); });
            }
            return;
        });

    return 0;
}

std::string MysqlModule::QueryStringReplace(const std::string& strQueryString, const std::vector<std::vector<char>>& vecBuff)
{
    if (vecBuff.empty() == 0) {
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
            strResult.append(vecBuff[iReplace].data(), vecBuff[iReplace].size());
            nLastRead = i + 1;
            if (++iReplace >= vecBuff.size()) {
                break;
            }
        }
    }

    strResult.append(strQueryString, nLastRead, nLenRead - 1 - nLastRead);
    return strResult;
}

int32_t MysqlModule::MysqlQuery(MYSQL* pMysqlHandle, const std::string& strQueryString, const std::vector<std::string>& vecArgs)
{
    assert(nullptr != pMysqlHandle);
    assert(GetMysqlHandle() == pMysqlHandle);
    if (vecArgs.size() >= kArgNumMax) {
        LOG_ERROR("vecArgs size, query: {}", strQueryString);
        return Pb::SSMessageCode::ss_msg_cache_length_overflow;
    }

    std::vector<std::vector<char>> vecBuff(vecArgs.size(), std::vector<char>());

    int32_t iArgs = 0;
    for (; iArgs < vecArgs.size(); ++iArgs) {
        auto& strArg = vecArgs[iArgs];
        auto& elemBuff = vecBuff[iArgs];
        elemBuff.resize(strArg.length() * 2 + 1);
        if (strArg.length() * 2 + 1 > kArgLenMax) [[unlikely]] {
            LOG_ERROR("query arg:{} aueryString:{}", strArg, strQueryString);
            return Pb::SSMessageCode::ss_msg_arg_length_overflow;
        }
        
        auto nBuffLen = mysql_real_escape_string(pMysqlHandle, elemBuff.data(), strArg.c_str(), static_cast<unsigned long>(strArg.length()));
        elemBuff.resize(nBuffLen);
    }

    if (std::string strQuery = QueryStringReplace(strQueryString, vecBuff); mysql_real_query(pMysqlHandle, strQuery.c_str(), static_cast<unsigned long>(strQuery.length()))) {
        LOG_ERROR("query test:{}, {}, aueryString:{}", mysql_errno(pMysqlHandle), mysql_error(pMysqlHandle), strQuery);
        return Pb::SSMessageCode::ss_msg_error_sql;
    }

    return Pb::SSMessageCode::ss_msg_success;
}

std::tuple<int32_t, MysqlModule::MysqlSelectResultMap> MysqlModule::QuerySelectRowsInCurrentThread(
    const std::string& strQueryString, const std::vector<std::string>& vecArgs)
{
    int32_t nError = 0;
    MysqlSelectResultMap mapResult;
    auto pMysqlHandle = GetMysqlHandle();
    assert(nullptr != pMysqlHandle);
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

                if (const char* pRow = row[i]; pRow != nullptr) {
                    mapResult[vecFieldName[i]].emplace_back(pRow, length[i]);
                } else {
                    mapResult[vecFieldName[i]].emplace_back("");
                }
            }
        }
    } while (false);

    return { nError, mapResult };
}

std::tuple<int32_t, uint64_t, uint64_t> MysqlModule::QueryModifyInCurrentThread(const std::string& strQueryString, const std::vector<std::string>& vecArgs)
{
    auto pMysqlHandle = GetMysqlHandle();
    assert(nullptr != pMysqlHandle);
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

    return { nError, nAffectRows, nInsertId };
}

int32_t MysqlModule::LoadMessage(google::protobuf::Message& message)
{
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();

    std::vector<std::string> vecArgs;
    std::string strQueryCommonKey;
    // loop tables
    for (int iField = 0; iField < pDescriptor->field_count(); ++iField) {
        const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(iField);
        if (!pFieldDescriptor) {
            continue;
        }
        auto strTabName = pFieldDescriptor->name();
        // add common key
        if (IsDBCommonKeyType(pFieldDescriptor->cpp_type())) {
            AddQueryCondition(message, *pFieldDescriptor, strQueryCommonKey, vecArgs);
            continue;
        }

        // load db data
        std::string strQueryString = "SELECT ";
        auto pSubDescriptor = pFieldDescriptor->message_type();
        for (int iSubField = 0; iSubField < pSubDescriptor->field_count(); ++iSubField) {
            const google::protobuf::FieldDescriptor* pSubFieldDescriptor = pSubDescriptor->field(iSubField);
            if (!pSubFieldDescriptor) {
                continue;
            }

            auto strFieldName = pSubFieldDescriptor->name();
            strQueryString.append(strFieldName).append(",");
        }

        if (!strQueryString.empty()) {
            strQueryString.back() = ' ';
        }

        strQueryString.append(STR_FORMAT(" FROM {}", strTabName));
        if (!strQueryString.empty()) {
            strQueryString.append(" WHERE").append(strQueryCommonKey);
        }
        auto [nError, mapResult] = QuerySelectRowsInCurrentThread(strQueryString, vecArgs);
        if (nError != 0) {
            LOG_ERROR("load data error:{}, {}", nError, strTabName);
            return nError;
        }

        // fill protobuf message
        if (!mapResult.empty()) {
            std::size_t nResultSize = mapResult.begin()->second.size();
            for (std::size_t iResult = 0; iResult < nResultSize; ++iResult) {
                if (pFieldDescriptor->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
                    continue;
                }

                google::protobuf::Message* pMsg = nullptr;
                if (pFieldDescriptor->is_repeated()) {
                    pMsg = pReflection->AddMessage(&message, pFieldDescriptor);
                } else {
                    pMsg = pReflection->MutableMessage(&message, pFieldDescriptor);
                }

                auto pMsgReflection = pMsg->GetReflection();
                auto pMsgDescriptor = pMsg->GetDescriptor();
                for (int iSubField = 0; iSubField < pMsgDescriptor->field_count(); ++iSubField) {
                    const google::protobuf::FieldDescriptor* pMsgFieldDescriptor = pMsgDescriptor->field(iSubField);
                    if (!pMsgFieldDescriptor) {
                        continue;
                    }

                    if (pMsgFieldDescriptor->is_repeated()) {
                        DBStringToProtoMessageAsBlob(*pMsg, *pMsgFieldDescriptor, mapResult[pMsgFieldDescriptor->name()][iResult]);
                    } else {
                        DBStringToProtoMessage(*pMsg, *pMsgFieldDescriptor, mapResult[pMsgFieldDescriptor->name()][iResult]);
                    }
                }
            }
        }
    }

    return 0;
}

int32_t MysqlModule::UpdateMessage(google::protobuf::Message& message)
{
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();

    std::vector<std::string> vecCommonKeyArgs;
    std::vector<std::string> vecFieldNameCommon;
    std::vector<std::string> vecFieldParamCharCommon;
    // loop tables
    for (int iField = 0; iField < pDescriptor->field_count(); ++iField) {
        const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(iField);
        if (!pFieldDescriptor) {
            continue;
        }

        auto strTabName = pFieldDescriptor->name();
        // handle common keys
        if (IsDBCommonKeyType(pFieldDescriptor->cpp_type())) {
            vecFieldNameCommon.emplace_back(pFieldDescriptor->name());
            vecCommonKeyArgs.emplace_back(DBProtoMessageToString(message, *pFieldDescriptor));
            if (pFieldDescriptor->cpp_type() == google::protobuf::FieldDescriptor::CppType::CPPTYPE_STRING) {
                vecFieldParamCharCommon.emplace_back("'{}'");
            } else {
                vecFieldParamCharCommon.emplace_back("{}");
            }

            continue;
        }

        if (pFieldDescriptor->is_repeated() && pReflection->FieldSize(message, pFieldDescriptor) <= 0) {
            continue;
        }
        if (!pFieldDescriptor->is_repeated() && !pReflection->HasField(message, pFieldDescriptor)) {
            continue;
        }

        std::vector<std::string> vecCurFieldName(vecFieldNameCommon.begin(), vecFieldNameCommon.end());
        std::vector<std::string> vecCurFieldParamCharCommon(vecFieldParamCharCommon.begin(), vecFieldParamCharCommon.end());
        std::vector<std::string> vecCurFieldDataNames;
        auto pSubDescriptor = pFieldDescriptor->message_type();
        for (int iSubField = 0; iSubField < pSubDescriptor->field_count(); ++iSubField) {
            const google::protobuf::FieldDescriptor* pSubFieldDescriptor = pSubDescriptor->field(iSubField);
            if (!pSubFieldDescriptor) {
                continue;
            }
            if (!IsDBKey(message, *pSubFieldDescriptor)) {
                vecCurFieldDataNames.emplace_back(pSubFieldDescriptor->name());
            }
            vecCurFieldName.emplace_back(pSubFieldDescriptor->name());
            if (pSubFieldDescriptor->cpp_type() == google::protobuf::FieldDescriptor::CppType::CPPTYPE_STRING) {
                vecCurFieldParamCharCommon.emplace_back("'{}'");
            } else {
                vecCurFieldParamCharCommon.emplace_back("{}");
            }
        }

        // example: QueryModify(0, std::string("insert into test (id, name) values({}, '{}')
        // on duplicate key update name=values(name)"), std::vector<std::string> { "121", "ok" }, nullptr);

        // load db data
        std::vector<std::string> vecArgs;
        std::string strQueryString = "INSERT INTO ";
        strQueryString.append(strTabName);
        strQueryString.append(" (");

        for (size_t iFieldName = 0; iFieldName < vecCurFieldName.size(); ++iFieldName) {
            if (iFieldName != 0) {
                strQueryString.append(", ");
            }
            strQueryString.append(vecCurFieldName[iFieldName]);
        }
        strQueryString.append(")");
        strQueryString.append(" VALUES");
        std::string strValuesElem;
        strValuesElem.append("(");
        for (size_t iFieldName = 0; iFieldName < vecCurFieldParamCharCommon.size(); ++iFieldName) {
            if (iFieldName != 0) {
                strValuesElem.append(", ");
            }
            strValuesElem.append(vecCurFieldParamCharCommon[iFieldName]);
        }
        strValuesElem.append(" )");
        if (pFieldDescriptor->is_repeated()) {
            for (int iChildField = 0; iChildField < pReflection->FieldSize(message, pFieldDescriptor); ++iChildField) {
                auto pMsg = &(pReflection->GetRepeatedMessage(message, pFieldDescriptor, iChildField));

                vecArgs.insert(vecArgs.end(), vecCommonKeyArgs.begin(), vecCommonKeyArgs.end());
                auto pMsgReflection = pMsg->GetReflection();
                auto pMsgDescriptor = pMsg->GetDescriptor();
                for (int iMsgField = 0; iMsgField < pMsgDescriptor->field_count(); ++iMsgField) {
                    const google::protobuf::FieldDescriptor* pMsgFieldDescriptor = pMsgDescriptor->field(iMsgField);
                    if (!pMsgFieldDescriptor) {
                        continue;
                    }

                    vecArgs.emplace_back(DBProtoMessageToString(*pMsg, *pMsgFieldDescriptor));
                }

                if (iChildField != 0) {
                    strQueryString.append(",");
                }
                strQueryString.append(strValuesElem);
            }
        } else {
            auto pMsg = pReflection->MutableMessage(&message, pFieldDescriptor);
            auto pMsgReflection = pMsg->GetReflection();
            auto pMsgDescriptor = pMsg->GetDescriptor();
            vecArgs.insert(vecArgs.end(), vecCommonKeyArgs.begin(), vecCommonKeyArgs.end());
            for (int iMsgField = 0; iMsgField < pMsgDescriptor->field_count(); ++iMsgField) {
                const google::protobuf::FieldDescriptor* pMsgFieldDescriptor = pMsgDescriptor->field(iMsgField);
                if (!pMsgFieldDescriptor) {
                    continue;
                }
                if (IsDBKey(*pMsg, *pMsgFieldDescriptor)) {
                    continue;
                }

                vecArgs.emplace_back(DBProtoMessageToString(*pMsg, *pMsgFieldDescriptor));
            }
            strQueryString.append(strValuesElem);
        }

        strQueryString.append(" ON DUPLICATE KEY UPDATE ");
        for (size_t iFieldName = 0; iFieldName < vecCurFieldDataNames.size(); ++iFieldName) {
            if (iFieldName != 0) {
                strQueryString.append(", ");
            }
            strQueryString.append(vecCurFieldDataNames[iFieldName]).append("=values(").append(vecCurFieldDataNames[iFieldName]).append(")");
        }

        auto [nError, nAffectRows, nInsertId] = QueryModifyInCurrentThread(strQueryString, vecArgs);
        if (nError != 0) {
            LOG_ERROR("modify data error:{}, {}", nError, strTabName);
            return nError;
        }
    }

    return 0;
}

int32_t MysqlModule::DeleteMessage(google::protobuf::Message& message)
{
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();

    std::vector<std::string> vecKeyCommonArgs;
    std::string strQueryCommonKey;
    // loop tables
    for (int iField = 0; iField < pDescriptor->field_count(); ++iField) {
        const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(iField);
        if (!pFieldDescriptor) {
            continue;
        }

        // handle common keys
        if (IsDBCommonKeyType(pFieldDescriptor->cpp_type())) {
            AddQueryCondition(message, *pFieldDescriptor, strQueryCommonKey, vecKeyCommonArgs);
            continue;
        }

        if (pFieldDescriptor->is_repeated() && pReflection->FieldSize(message, pFieldDescriptor) <= 0) {
            continue;
        }
        if (!pFieldDescriptor->is_repeated() && !pReflection->HasField(message, pFieldDescriptor)) {
            continue;
        }
        // QueryModify(0, std::string("delete from test.website where name='{}'"), std::vector<std::string> { "ok" }, nullptr);
        // load db data
        auto strTabName = pFieldDescriptor->name();
        std::vector<std::string> vecArgs(vecKeyCommonArgs.begin(), vecKeyCommonArgs.end());
        std::string strQueryString = "DELETE FROM ";
        strQueryString.append(strTabName);
        strQueryString.append(" WHERE ");
        strQueryString.append(strQueryCommonKey);
        std::string strQueryConditionAppend;

        if (pFieldDescriptor->is_repeated()) {
            std::vector<std::string> vecRepeatedQueryString;
            for (int iChildField = 0; iChildField < pReflection->FieldSize(message, pFieldDescriptor); ++iChildField) {
                auto pMsg = &(pReflection->GetRepeatedMessage(message, pFieldDescriptor, iChildField));
                std::string strRepeatedQueryString;
                std::vector<std::string> elemRepeatedArgs;

                auto pMsgReflection = pMsg->GetReflection();
                auto pMsgDescriptor = pMsg->GetDescriptor();
                for (int iMsgField = 0; iMsgField < pMsgDescriptor->field_count(); ++iMsgField) {
                    const google::protobuf::FieldDescriptor* pMsgFieldDescriptor = pMsgDescriptor->field(iMsgField);
                    if (!pMsgFieldDescriptor) {
                        continue;
                    }

                    if (!IsDBKey(*pMsg, *pMsgFieldDescriptor)) {
                        continue;
                    }

                    AddQueryCondition(*pMsg, *pMsgFieldDescriptor, strRepeatedQueryString, elemRepeatedArgs);
                }
                vecRepeatedQueryString.emplace_back(std::move(strRepeatedQueryString));
                vecArgs.insert(vecArgs.end(), elemRepeatedArgs.begin(), elemRepeatedArgs.end());
            }

            for (size_t iRepeatedQuery = 0; iRepeatedQuery < vecRepeatedQueryString.size(); ++iRepeatedQuery) {
                if (iRepeatedQuery != 0) {
                    strQueryConditionAppend.append(" OR");
                }
                strQueryConditionAppend.append(" (").append(vecRepeatedQueryString[iRepeatedQuery]).append(")");
            }
        } else {
            auto pSubDescriptor = pFieldDescriptor->message_type();
            auto pMsg = pReflection->MutableMessage(&message, pFieldDescriptor);
            auto pMsgReflection = pMsg->GetReflection();
            auto pMsgDescriptor = pMsg->GetDescriptor();
            for (int iMsgField = 0; iMsgField < pMsgDescriptor->field_count(); ++iMsgField) {
                const google::protobuf::FieldDescriptor* pMsgFieldDescriptor = pMsgDescriptor->field(iMsgField);
                if (!pMsgFieldDescriptor) {
                    continue;
                }
                if (!IsDBKey(*pMsg, *pMsgFieldDescriptor)) {
                    continue;
                }

                AddQueryCondition(*pMsg, *pMsgFieldDescriptor, strQueryConditionAppend, vecArgs);
            }
        }

        if (!strQueryConditionAppend.empty()) {
            strQueryString.append(" AND").append(strQueryConditionAppend);
        }
        auto [nError, nAffectRows, nInsertId] = QueryModifyInCurrentThread(strQueryString, vecArgs);
        if (nError != 0) {
            LOG_ERROR("load data error:{}, {}", nError, strTabName);
            return nError;
        }
    }

    return 0;
}

void MysqlModule::AddQueryCondition(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor, std::string& strQuerys, std::vector<std::string>& vecArgs)
{
    if (!vecArgs.empty()) {
        strQuerys.append(" AND ");
    }
    strQuerys.append(" ").append(fieldDescriptor.name()).append(" = ");
    if (fieldDescriptor.cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING) {
        strQuerys.append(" '{}'");
    } else {
        strQuerys.append(" {}");
    }
    vecArgs.emplace_back(DBProtoMessageToString(message, fieldDescriptor));
}

void MysqlModule::OnTest()
{

    //Db::KVData msgRsp;
    //msgRsp.set_user_id("user_1");
    //LoadMessage(*pUserData);

    Db::KVData msgReq;
    msgReq.mutable_user_data()->set_user_id("user_1");
    // msgReqmutable_user_data()->mutable_user_base()->set_user_name("hi");
    msgReq.mutable_user_data()->add_user_counts()->set_count_type(1);
    msgReq.mutable_user_data()->add_user_counts()->set_count_type(3);
    DeleteMessage(*msgReq.mutable_user_data());
}

TONY_CAT_SPACE_END

#endif
