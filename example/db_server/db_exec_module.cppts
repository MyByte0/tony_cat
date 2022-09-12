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
    Pb::ServerHead head;
    Pb::SSSaveDataReq req;
    req.mutable_kv_data()->mutable_user_data()->set_user_id("111");
    req.mutable_kv_data()->mutable_user_data()->mutable_user_base()->set_update_time(1);
    auto pUser = req.mutable_kv_data()->mutable_user_data()->mutable_user_counts()->add_user_count();
    pUser->set_count_type(1);
    pUser = req.mutable_kv_data()->mutable_user_data()->mutable_user_counts()->add_user_count();
    pUser->set_count_type(3);
    OnHandleSSSaveDataReq(0, head, req);
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);

    m_pNetPbModule->RegisterHandle(this, &DBExecModule::OnHandleSSSaveDataReq);
    m_pNetPbModule->RegisterHandle(this, &DBExecModule::OnHandleSSQueryDataReq);
}

#ifdef GetMessage // for windows define GetMessage
#undef GetMessage
#endif

void DBExecModule::OnHandleSSSaveDataReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSSaveDataReq& msgReq)
{
    auto& message = msgReq.kv_data();
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();
    for (int i = 0; i < pDescriptor->field_count(); ++i) {
        const google::protobuf::FieldDescriptor* pField = pDescriptor->field(i);
        if (!pField) {
            continue;
        }
        auto strDatabaseName = pField->name();
        if (pField->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
            continue;
        }
        
        std::vector<DBRecord> vecResultList;
        MapTableRecord mapTable;
        PaserProtoTables(pReflection->GetMessage(message, pField), mapTable);
        // \TODO write to db
    }
}

void DBExecModule::OnHandleSSQueryDataReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSQueryDataReq& msgReq)
{
    Pb::SSQueryDataRsp msgRsp;

    m_pNetPbModule->SendPacket(sessionId, head, msgRsp);
    return;

}

void DBExecModule::PaserProtoTables(const google::protobuf::Message& message, MapTableRecord& mapTableRecord)
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
        if (nTag < Db::db_common_tag_table_primekey_begin) {
            PaserProtoBasePrimeKey(vecCommonKeys, message, *pFieldDescriptor, strFieldName);
            continue;
        }

        auto& strTableName = strFieldName;
        auto& vecResultList = mapTableRecord[strTableName];
        if (pFieldDescriptor->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
            if (pFieldDescriptor->is_map()) {
               // \TODO: read proto map type
               // auto& fieldSubMessage = pReflection->GetMessage(message, pFieldDescriptor);
               // PaserMapProto(vecDBSchemaAppend, vecResultList, fieldSubMessage);
            } else if (!pFieldDescriptor->is_repeated()) {
                if (pReflection->HasField(message, pFieldDescriptor)) {
                    PaserMsgProto(vecCommonKeys, vecResultList, pReflection->GetMessage(message, pFieldDescriptor));
                }
            } else {
                for (int iRepeated = 0; iRepeated < pReflection->FieldSize(message, pFieldDescriptor); ++iRepeated) {
                    PaserMsgProto(vecCommonKeys, vecResultList, pReflection->GetRepeatedMessage(message, pFieldDescriptor, iRepeated));
                    continue;
                }
            }
            continue;
        }

        if (pFieldDescriptor->is_map()) {
            continue;
        }

        continue;
    }

    return;
}

void DBExecModule::PaserMsgProto(const std::vector<DBKeyValue>& vecPriKeys, std::vector<DBRecord>& vecResultList, 
    const google::protobuf::Message& message, bool bWithValue /*= true*/)
{
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();
    if (!pReflection || !pDescriptor) {
        return;
    }
    DBRecord stDBRecord;
    stDBRecord.vecPriKeyFields = vecPriKeys;
    for (int i = 0; i < pDescriptor->field_count(); ++i) {
        const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(i);
        if (!pFieldDescriptor) {
            continue;
        }
        auto strFieldName = pFieldDescriptor->name();

        // handle common keys
        int nTag = pFieldDescriptor->number();
        if (nTag < Db::db_common_tag_no_primekey_begin) {
            // key type map not support other values
            if (pFieldDescriptor->is_map()) {
                // PaserMapProto();
                return;
            }
            // type repeated not support other values
            if (pFieldDescriptor->is_repeated()) {
                for (int iRepeated = 0; iRepeated < pReflection->FieldSize(message, pFieldDescriptor); ++iRepeated) {
                    PaserMsgProto(stDBRecord.vecPriKeyFields, vecResultList, pReflection->GetRepeatedMessage(message, pFieldDescriptor, iRepeated));
                }
                return;
            }
            if (IsProtoBasePrimeKeyType(*pFieldDescriptor)) {
                PaserProtoBasePrimeKey(stDBRecord.vecPriKeyFields, message, *pFieldDescriptor, strFieldName);
            } else if (pFieldDescriptor->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
                PaserMsgProto(stDBRecord.vecPriKeyFields, vecResultList, pReflection->GetMessage(message, pFieldDescriptor));
                break;
            } else {
                LOG_ERROR("paser data error on:{}", strFieldName);
                break;
            }
        }
        // handle values
        else if (bWithValue) {
            DBKeyValue stDBKeyValue;
            stDBKeyValue.eType = GetDBType(message, *pFieldDescriptor);
            stDBKeyValue.strValue = PaserStringValue(message, *pFieldDescriptor);
            stDBKeyValue.strKey = strFieldName;
            stDBRecord.vecValueFields.emplace_back(std::move(stDBKeyValue));
        }
    }

    vecResultList.emplace_back(std::move(stDBRecord));
}

void DBExecModule::PaserMapProto(const std::vector<DBKeyValue>& vecPriKeys, std::vector<DBRecord>& vecResultList,
    const google::protobuf::Message& message, bool bWithValue /*= true*/)
{
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();
    if (!pReflection || !pDescriptor) {
        return;
    }
    // handle common keys
    if (pDescriptor->field_count() > 1) {
        LOG_ERROR("prime key not support more than one args");
        return;
    }

    for (int i = 0; i < pDescriptor->field_count(); ++i) {
        const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(i);
        if (!pFieldDescriptor) {
            LOG_ERROR("FieldDescriptor empty");
            continue;
        }

        if (!pFieldDescriptor->is_map()) {
            LOG_ERROR("prime key need map");
            return;
        }

        auto strFieldName = pFieldDescriptor->name();
        int nTag = pFieldDescriptor->number();
        if (nTag >= Db::db_common_tag_no_primekey_begin) {
            PaserMsgProto(vecPriKeys, vecResultList, pReflection->GetMessage(message, pFieldDescriptor));
            continue;
        }

        auto pMassageDescriptor = pFieldDescriptor->message_type();
        auto pFieldDescriptorKey = pMassageDescriptor->map_key();
        auto pFieldDescriptorValue = pMassageDescriptor->map_value();

        std::string strPrfix = "map_";
        if (strFieldName.find(strPrfix) != 0) {
            LOG_ERROR("map field name on {}", strFieldName);
            return;
        }

        //DBRecord stDBRecord;
        //// stDBKeyValue.bPrimeKey = true;
        //stDBRecord.strKey = strFieldName.substr(strPrfix.length());
        //stDBKeyValue.strValue = ""; // need contain on vector
        //vecResultList.emplace_back(std::move(stDBRecord));
        auto& massageMapValue = pReflection->GetMessage(message, pFieldDescriptor);
        PaserMapProto(vecPriKeys, vecResultList, massageMapValue);
    }
}

bool DBExecModule::IsProtoBasePrimeKeyType(const google::protobuf::FieldDescriptor& fieldDescriptor)
{
    return fieldDescriptor.cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_INT32
        || fieldDescriptor.cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_INT64
        || fieldDescriptor.cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_UINT32
        || fieldDescriptor.cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_UINT64
        || fieldDescriptor.cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING;
}

void DBExecModule::PaserProtoBasePrimeKey(std::vector<DBKeyValue>& vecCommonKeys, const google::protobuf::Message& message,
    const google::protobuf::FieldDescriptor& fieldDescriptor, const std::string& strKayName)
{
    auto strFieldName = fieldDescriptor.name();
    auto pReflection = message.GetReflection();
    DBKeyValue stDBKeyValue;
    if (IsProtoBasePrimeKeyType(fieldDescriptor)) {
        stDBKeyValue.strKey = strKayName;
        stDBKeyValue.eType = GetDBType(message, fieldDescriptor);
        stDBKeyValue.strValue = PaserStringValue(message, fieldDescriptor);
        vecCommonKeys.emplace_back(std::move(stDBKeyValue));
    } else {
        LOG_ERROR("primekey protobuf, key:{}", strFieldName);
    }
}

std::string DBExecModule::PaserStringValue(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor)
{
    std::string result;
    if (fieldDescriptor.is_repeated()) {
        LOG_ERROR("not support repeated");
        return result;
    } else if (fieldDescriptor.is_map())
    {
        LOG_ERROR("not support map");
        return result;
    }

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
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        result = std::to_string(pReflection->GetFloat(
            message, &fieldDescriptor));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        result = std::to_string(pReflection->GetDouble(
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

DBExecModule::DBKeyValue::EDBValueType DBExecModule::GetDBType(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor)
{
    DBKeyValue::EDBValueType result = DBKeyValue::eTypeInvalid;
    if (fieldDescriptor.is_repeated()) {
        result = DBKeyValue::eTypeBinary;
        return result;
    } else if (fieldDescriptor.is_map()) {
        result = DBKeyValue::eTypeBinary;
        return result;
    }

    auto pReflection = message.GetReflection();
    switch (fieldDescriptor.cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        result = DBKeyValue::eTypeInt;
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        result = DBKeyValue::eTypeBigInt;
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        result = DBKeyValue::eTypeUint;
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        result = DBKeyValue::eTypeBigUint;
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        result = DBKeyValue::eTypeFloat;
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        result = DBKeyValue::eTypeDouble;
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        result = DBKeyValue::eTypeBool;
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        result = DBKeyValue::eTypeInt;
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        result = DBKeyValue::eTypeString;
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        result = DBKeyValue::eTypeBinary;
        break;
    }

    return result;
}

TONY_CAT_SPACE_END
