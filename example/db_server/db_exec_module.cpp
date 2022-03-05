#include "db_exec_module.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "server_common/server_define.h"

TONY_CAT_SPACE_BEGIN

DBExecModule::DBExecModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

DBExecModule::~DBExecModule() { }

void DBExecModule::BeforeInit()
{
    //Pb::ServerHead head;
    //Pb::SSSaveDataReq req;
    //OnServerMessage(0, head, req);
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);
}

void DBExecModule::OnInit()
{
    m_pNetPbModule->RegisterHandle(this, &DBExecModule::OnServerMessage);
}

#ifdef GetMessage // for windows define GetMessage
#undef GetMessage
#endif

void DBExecModule::OnServerMessage(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSSaveDataReq& msgReq)
{
    auto& message = msgReq.kv_data();
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();
    for (int i = 0; i < pDescriptor->field_count(); ++i) {
        const google::protobuf::FieldDescriptor* pField = pDescriptor->field(i);
        if (!pField) {
            continue;
        }
        auto strFieldName = pField->name();
        if (pField->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
            continue;
        }

        PaserProto(pReflection->GetMessage(message, pField));
    }
}

void DBExecModule::PaserProto(const google::protobuf::Message& message)
{
    std::vector<DBKeyValue> vecCommonKeys;
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();
    if (!pReflection || !pDescriptor) {
        return;
    }
    for (int i = 0; i < pDescriptor->field_count(); ++i) {
        const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(i);
        if (!pFieldDescriptor) {
            continue;
        }
        auto strFieldName = pFieldDescriptor->name();

        // handle common keys
        int nTag = pFieldDescriptor->number();
        if (nTag < Db::db_common_primekey_max) {
            PaserProtoBasePrimeKey(vecCommonKeys, message, *pFieldDescriptor, strFieldName);
            continue;
        }

        auto& strTableName = strFieldName;
        std::vector<DBKeyValue> vecDBSchemaAppend;
        if (pFieldDescriptor->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
            std::string strPriKeyCount = "primekey_map_count_";
            auto strClassName = pFieldDescriptor->message_type()->name();
            strPriKeyCount.append(strClassName);
            auto pPrimekeyCount = pFieldDescriptor->file()->FindEnumValueByName(strPriKeyCount);
            if (pPrimekeyCount) {
                int nPrimeKeyCount = pPrimekeyCount->number();

                auto& fieldSubMessage = pReflection->GetMessage(message, pFieldDescriptor);
                PaserMapProto(vecDBSchemaAppend, nPrimeKeyCount, fieldSubMessage);
            } else if (!pFieldDescriptor->is_repeated()) {
                PaserMsgProto(vecDBSchemaAppend, pReflection->GetMessage(message, pFieldDescriptor));
            } else {
                for (int iRepeated = 0; iRepeated < pDescriptor->field_count(); ++iRepeated) {
                    std::vector<DBKeyValue> vecDBSchemaAppendSub;
                    pReflection->FieldSize(message, pFieldDescriptor);
                    pReflection->GetRepeatedMessage(message, pFieldDescriptor, iRepeated);
                    PaserMsgProto(vecDBSchemaAppendSub, pReflection->GetMessage(message, pFieldDescriptor));

                    // \TODO write to db
                    continue;
                }
            }

            // \TODO write to db
            continue;
        }

        if (pFieldDescriptor->is_map()) {
            continue;
        }

        continue;
    }
}

void DBExecModule::PaserProtoBasePrimeKey(std::vector<DBKeyValue>& vecCommonKeys, const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor, const std::string& strKayName)
{
    auto strFieldName = fieldDescriptor.name();
    auto pReflection = message.GetReflection();
    DBKeyValue stDBKeyValue;
    if (fieldDescriptor.cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_INT32
        || fieldDescriptor.cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_INT64
        || fieldDescriptor.cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_UINT32
        || fieldDescriptor.cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_UINT64
        || fieldDescriptor.cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING) {
        stDBKeyValue.bPrimeKey = true;
        stDBKeyValue.strKey = strKayName;
        stDBKeyValue.strValue = GetStringValue(message, fieldDescriptor);
        vecCommonKeys.emplace_back(std::move(stDBKeyValue));
    } else {
        LOG_ERROR("primekey protobuf, key:{}", strFieldName);
    }
}

void DBExecModule::PaserMapProto(std::vector<DBKeyValue>& vecResult, int nPrimeKeyCnt, const google::protobuf::Message& message)
{
    if (--nPrimeKeyCnt < 0) {
        PaserMsgProto(vecResult, message);
        return;
    }

    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();
    if (!pReflection || !pDescriptor) {
        return;
    }
    if (pDescriptor->field_count() > 1) {
        LOG_ERROR("prime key not support more than one args");
        return;
    }
    for (int i = 0; i < pDescriptor->field_count(); ++i) {
        const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(i);
        if (!pFieldDescriptor) {
            continue;
        }

        if (!pFieldDescriptor->is_map()) {
            LOG_ERROR("prime key need map");
            return;
        }

        auto strFieldName1 = pFieldDescriptor->name();
        auto pMassageDescriptor = pFieldDescriptor->message_type();
        auto pFieldDescriptorKey = pMassageDescriptor->map_key();
        auto pFieldDescriptorValue = pMassageDescriptor->map_value();

        std::string strPrfix = "map_";
        if (strFieldName1.find(strPrfix) != 0) {
            LOG_ERROR("map field name on {}", strFieldName1);
            return;
        }
        DBKeyValue stDBKeyValue;
        stDBKeyValue.bPrimeKey = true;
        stDBKeyValue.strKey = strFieldName1.substr(strPrfix.length());
        stDBKeyValue.strValue = ""; // need contain on vector
        vecResult.emplace_back(std::move(stDBKeyValue));

        auto& massageMapValue = pReflection->GetMessage(message, pFieldDescriptor);
        PaserMapProto(vecResult, nPrimeKeyCnt, massageMapValue);
    }
}

void DBExecModule::PaserMsgProto(std::vector<DBKeyValue>& vecResult, const google::protobuf::Message& message)
{
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();
    if (!pReflection || !pDescriptor) {
        return;
    }
    for (int i = 0; i < pDescriptor->field_count(); ++i) {
        const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(i);
        if (!pFieldDescriptor) {
            continue;
        }
        auto strFieldName = pFieldDescriptor->name();

        // handle common keys
        int nTag = pFieldDescriptor->number();
        if (nTag < Db::db_common_primekey_max) {
            PaserProtoBasePrimeKey(vecResult, message, *pFieldDescriptor, strFieldName);
            continue;
        }

        DBKeyValue stDBKeyValue;
        stDBKeyValue.bPrimeKey = false;
        stDBKeyValue.strKey = strFieldName;
        stDBKeyValue.strValue = GetStringValue(message, *pFieldDescriptor);
        vecResult.emplace_back(std::move(stDBKeyValue));
    }
}

std::string DBExecModule::GetStringValue(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor)
{
    if (fieldDescriptor.is_repeated()) {
        LOG_ERROR("get field type error");
    }
    std::string result;
    auto pReflection = message.GetReflection();
    switch (fieldDescriptor.cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        result = std::to_string(pReflection->GetInt32(
            message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        result = std::to_string(pReflection->GetInt64(
            message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        result = std::to_string(pReflection->GetUInt32(
            message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        result = std::to_string(pReflection->GetUInt64(
            message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        result = std::to_string(pReflection->GetDouble(
            message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        result = std::to_string(pReflection->GetFloat(
            message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        result = pReflection->GetBool(
                     message, &fieldDescriptor)
            ? "true"
            : "false";
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        result = pReflection->GetEnum(
                                message, &fieldDescriptor)
                     ->full_name();
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        result = pReflection->GetString(
            message, &fieldDescriptor);
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        result = pReflection->GetMessage(
                                message, &fieldDescriptor)
                     .SerializeAsString();
        break;
    }

    return result;
}

TONY_CAT_SPACE_END
