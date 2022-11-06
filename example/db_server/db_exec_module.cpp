#include "db_exec_module.h"
#include "common/database/db_define.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/database/mysql/mysql_module.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/utility/crc.h"
#include "server_common/server_define.h"

#include  <cstdlib>

TONY_CAT_SPACE_BEGIN

DBExecModule::DBExecModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

DBExecModule::~DBExecModule() { }

void DBExecModule::BeforeInit()
{
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);
    m_pMysqlModule = FIND_MODULE(m_pModuleManager, MysqlModule);

    // OnTest();
}

void DBExecModule::OnHandleSSSaveDataReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSSaveDataReq& msgReq)
{
    auto userId = head.user_id();

    auto& loop = Loop::GetCurrentThreadLoop();
    m_pMysqlModule->GetLoopPool().Exec(CRC32(userId),
        [this, sessionId, head, userId, &loop, msgReq = std::move(msgReq)]() mutable {
            auto pMsgRsp = std::make_shared<Pb::SSSaveDataRsp>();
            UpdateMessage(*msgReq.mutable_kv_data_update()->mutable_user_data());
            DeleteMessage(*msgReq.mutable_kv_data_delete()->mutable_user_data());
            loop.Exec([this, sessionId, head, pMsgRsp]() mutable { m_pNetPbModule->SendPacket(sessionId, head, *pMsgRsp); });
        });
}

void DBExecModule::OnHandleSSQueryDataReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSQueryDataReq& msgReq)
{
    auto userId = msgReq.user_id();

    auto& loop = Loop::GetCurrentThreadLoop();
    m_pMysqlModule->GetLoopPool().Exec(CRC32(userId),
        [this, sessionId, head, userId, &loop]() mutable {
            auto pMsgRsp = std::make_shared<Pb::SSQueryDataRsp>();
            auto pUserData = pMsgRsp->mutable_user_data();
            pUserData->set_user_id(userId);
            LoadMessage(*pUserData);
            loop.Exec([this, sessionId, head, pMsgRsp]() mutable { m_pNetPbModule->SendPacket(sessionId, head, *pMsgRsp); });
            return;
        });
    
    return;
}

int32_t DBExecModule::LoadMessage(google::protobuf::Message& message)
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
        if (IsCommonKeyType(pFieldDescriptor->cpp_type())) {
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
        auto [nError, mapResult] = m_pMysqlModule->QuerySelectRowsInCurrentThread(strQueryString, vecArgs);
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
                        StringToProtoMessageAsBlob(*pMsg, *pMsgFieldDescriptor, mapResult[pMsgFieldDescriptor->name()][iResult]);
                    } else {
                        StringToProtoMessage(*pMsg, *pMsgFieldDescriptor, mapResult[pMsgFieldDescriptor->name()][iResult]);
                    }
                }
            }
        }
    }

    return 0;
}

int32_t DBExecModule::UpdateMessage(google::protobuf::Message& message)
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
        if (IsCommonKeyType(pFieldDescriptor->cpp_type())) {
            vecFieldNameCommon.emplace_back(pFieldDescriptor->name());
            vecCommonKeyArgs.emplace_back(ProtoMessageToString(message, *pFieldDescriptor));
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
            if (!IsKey(message, *pSubFieldDescriptor)) {
                vecCurFieldDataNames.emplace_back(pSubFieldDescriptor->name());
            }
            vecCurFieldName.emplace_back(pSubFieldDescriptor->name());
            if (pSubFieldDescriptor->cpp_type() == google::protobuf::FieldDescriptor::CppType::CPPTYPE_STRING) {
                vecCurFieldParamCharCommon.emplace_back("'{}'");
            } else {
                vecCurFieldParamCharCommon.emplace_back("{}");
            }
        }

        // QueryModify(0, std::string("insert into test (id, name) values({}, '{}') on duplicate key update name=values(name)"), std::vector<std::string> { "121", "ok" }, nullptr);
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

                    vecArgs.emplace_back(ProtoMessageToString(*pMsg, *pMsgFieldDescriptor));
                }

                if (iChildField != 0) {
                    strQueryString.append(",");
                }
                strQueryString.append(strValuesElem);
            }
        }
        else {
            auto pMsg = pReflection->MutableMessage(&message, pFieldDescriptor);
            auto pMsgReflection = pMsg->GetReflection();
            auto pMsgDescriptor = pMsg->GetDescriptor();
            vecArgs.insert(vecArgs.end(), vecCommonKeyArgs.begin(), vecCommonKeyArgs.end());
            for (int iMsgField = 0; iMsgField < pMsgDescriptor->field_count(); ++iMsgField) {
                const google::protobuf::FieldDescriptor* pMsgFieldDescriptor = pMsgDescriptor->field(iMsgField);
                if (!pMsgFieldDescriptor) {
                    continue;
                }
                if (IsKey(*pMsg, *pMsgFieldDescriptor)) {
                    continue;
                }

                vecArgs.emplace_back(ProtoMessageToString(*pMsg, *pMsgFieldDescriptor));
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

        auto [nError, nAffectRows, nInsertId] = m_pMysqlModule->QueryModifyInCurrentThread(strQueryString, vecArgs);
        if (nError != 0) {
            LOG_ERROR("load data error:{}, {}", nError, strTabName);
            return nError;
        }
    }

    return 0;
}

int32_t DBExecModule::DeleteMessage(google::protobuf::Message& message)
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
        if (IsCommonKeyType(pFieldDescriptor->cpp_type())) {
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
                   
                    if (!IsKey(*pMsg, *pMsgFieldDescriptor)) {
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
                if (!IsKey(*pMsg, *pMsgFieldDescriptor)) {
                    continue;
                }

                AddQueryCondition(*pMsg, *pMsgFieldDescriptor, strQueryConditionAppend, vecArgs);
            }
        }

        if (!strQueryConditionAppend.empty()) {
            strQueryString.append(" AND").append(strQueryConditionAppend);
        }
        auto [nError, nAffectRows, nInsertId] = m_pMysqlModule->QueryModifyInCurrentThread(strQueryString, vecArgs);
        if (nError != 0) {
            LOG_ERROR("load data error:{}, {}", nError, strTabName);
            return nError;
        }
    }

    return 0;
}

bool DBExecModule::IsKey(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor)
{
    if (!IsCommonKeyType(fieldDescriptor.cpp_type())) {
        return false;
    }

    static DbPbFieldKey pbKeys;
    return pbKeys.mapMesssageKeys.count(message.GetTypeName()) > 0 && pbKeys.mapMesssageKeys[message.GetTypeName()].count(fieldDescriptor.name()) > 0;
}

void DBExecModule::StringToProtoMessage(google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor, const std::string& value)
{
    auto reflection = message.GetReflection();

    switch (fieldDescriptor.cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        reflection->SetDouble(&message, &fieldDescriptor, ::atof(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        reflection->SetFloat(&message, &fieldDescriptor, static_cast<float>(::atof(value.c_str())));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        reflection->SetEnumValue(&message, &fieldDescriptor, ::atoi(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
        StringToProtoMessageAsBlob(message, fieldDescriptor, value);
        break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        reflection->SetString(&message, &fieldDescriptor, value);
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        reflection->SetInt64(&message, &fieldDescriptor, ::atoll(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        reflection->SetInt32(&message, &fieldDescriptor, ::atoi(value.c_str()));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        reflection->SetUInt64(&message, &fieldDescriptor, ::strtoull(value.c_str(), nullptr, 10));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        reflection->SetUInt32(&message, &fieldDescriptor, static_cast<uint32_t>(::atoll(value.c_str())));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        reflection->SetBool(&message, &fieldDescriptor, ::atoi(value.c_str()) != 0);
        break;
	default:
		break;
    }
}

void DBExecModule::StringToProtoMessageAsBlob(google::protobuf::Message& message, 
    const google::protobuf::FieldDescriptor& fieldDescriptor, const std::string& value)
{
    auto reflection = message.GetReflection();
    auto sub_message = reflection->MutableMessage(&message, &fieldDescriptor);
    sub_message->ParseFromArray(value.c_str(), static_cast<int32_t>(value.length()));
}


std::string DBExecModule::ProtoMessageToString(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor)
{
    std::string strRet;
    auto reflection = message.GetReflection();

    switch (fieldDescriptor.cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        strRet = std::to_string(reflection->GetDouble(message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        strRet = std::to_string(reflection->GetFloat(message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        strRet = std::to_string(reflection->GetEnumValue(message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
        strRet = ProtoMessageToStringAsBlob(message, fieldDescriptor);
        break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        strRet = reflection->GetString(message, &fieldDescriptor);
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        strRet = std::to_string(reflection->GetInt64(message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        strRet = std::to_string(reflection->GetInt32(message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        strRet = std::to_string(reflection->GetUInt64(message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        strRet = std::to_string(reflection->GetUInt32(message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        strRet = std::to_string(reflection->GetBool(message, &fieldDescriptor));
        break;
	default:
		break;
    }

    return strRet;
}

std::string DBExecModule::ProtoMessageToStringAsBlob(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor)
{
    return message.SerializeAsString();
}

bool DBExecModule::IsCommonKeyType(google::protobuf::FieldDescriptor::CppType cppType) {
    return cppType == google::protobuf::FieldDescriptor::CppType::CPPTYPE_INT32 || cppType == google::protobuf::FieldDescriptor::CppType::CPPTYPE_UINT32
        || cppType == google::protobuf::FieldDescriptor::CppType::CPPTYPE_INT64 || cppType == google::protobuf::FieldDescriptor::CppType::CPPTYPE_UINT64
        || cppType == google::protobuf::FieldDescriptor::CppType::CPPTYPE_STRING;
}

void DBExecModule::AddQueryCondition(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor, std::string& strQuerys, std::vector<std::string>& vecArgs)
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
    vecArgs.emplace_back(ProtoMessageToString(message, fieldDescriptor));
}

void DBExecModule::OnTest()
{

    //Pb::SSQueryDataRsp msgRsp;
    //auto pUserData = msgRsp.mutable_user_data();
    //pUserData->set_user_id("user_1");
    //LoadMessage(*pUserData);

    Pb::SSSaveDataReq msgReq;
    msgReq.mutable_kv_data_delete()->mutable_user_data()->set_user_id("user_1");
    // msgReq.mutable_kv_data_delete()->mutable_user_data()->mutable_user_base()->set_user_name("hi");
    msgReq.mutable_kv_data_delete()->mutable_user_data()->add_user_counts()->set_count_type(1);
    msgReq.mutable_kv_data_delete()->mutable_user_data()->add_user_counts()->set_count_type(3);
    DeleteMessage(*msgReq.mutable_kv_data_delete()->mutable_user_data());
}

TONY_CAT_SPACE_END
