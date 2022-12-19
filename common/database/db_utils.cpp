#include "db_utils.h"

#include "common/log/log_module.h"
#include "db_define.h"

TONY_CAT_SPACE_BEGIN

DbPbFieldKey gPbKeys;

const char* strTableSpacer = "#";

// not have consisder byte endian
void AppendRawData(std::string& strOutput, const std::string& strInput)
{
    uint32_t nLen = uint32_t(strInput.length());
    strOutput.append((char*)(&nLen), sizeof nLen).append(strInput);
}

uint32_t PaserRawData(const std::string& strInput, uint32_t nStart, std::string& strOutput)
{
    uint32_t nLen = 0;
    if (nStart >= strInput.size()) {
        return UINT32_MAX;
    }

    if (nStart + sizeof nLen >= strInput.size()) {
        LOG_ERROR("Paser RawData Error with start length");
        return UINT32_MAX;
    }

    const char* pInput = strInput.c_str() + nStart;
    nLen = *(uint32_t*)pInput;
    if (strInput.size() < nStart + nLen) {
        LOG_ERROR("Paser RawData Error with data");
        return UINT32_MAX;
    }
    strOutput = std::string(pInput + sizeof nLen, nLen);
    return nStart + sizeof(uint32_t) + nLen;
}

void PaserList(const std::string& strInput, std::vector<std::string>& vecElems)
{
    uint32_t indexBegin = 0;
    std::string strData;
    while (indexBegin < strInput.size()) {
        indexBegin = PaserRawData(strInput, indexBegin, strData);
        if (indexBegin < UINT32_MAX) {
            vecElems.emplace_back(std::move(strData));
        }
    }
}

void PaserList(const std::string& strInput, std::unordered_set<std::string>& setElems)
{
    uint32_t indexBegin = 0;
    std::string strData;
    while (indexBegin < strInput.size()) {
        indexBegin = PaserRawData(strInput, indexBegin, strData);
        if (indexBegin < UINT32_MAX) {
            setElems.insert(std::move(strData));
        }
    }
}

void PacketList(const std::vector<std::string>& setInput, std::string& strOutput)
{
    for (auto& elemInput : setInput) {
        AppendRawData(strOutput, elemInput);
    }
}

void PacketList(const std::unordered_set<std::string>& vecInput, std::string& strOutput)
{
    for (auto& elemInput : vecInput) {
        AppendRawData(strOutput, elemInput);
    }
}

bool IsDBKey(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor)
{
    if (!IsDBCommonKeyType(fieldDescriptor.cpp_type())) {
        return false;
    }

    return gPbKeys.mapMesssageKeys.count(message.GetTypeName()) > 0 && gPbKeys.mapMesssageKeys[message.GetTypeName()].count(fieldDescriptor.name()) > 0;
}


bool IsDBCommonKeyType(google::protobuf::FieldDescriptor::CppType cppType)
{
    return cppType == google::protobuf::FieldDescriptor::CppType::CPPTYPE_INT32 || cppType == google::protobuf::FieldDescriptor::CppType::CPPTYPE_UINT32
        || cppType == google::protobuf::FieldDescriptor::CppType::CPPTYPE_INT64 || cppType == google::protobuf::FieldDescriptor::CppType::CPPTYPE_UINT64
        || cppType == google::protobuf::FieldDescriptor::CppType::CPPTYPE_STRING;
}

void DBStringToProtoMessage(google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor, const std::string& value)
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
        DBStringToProtoMessageAsBlob(message, fieldDescriptor, value);
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

void DBStringToProtoMessageAsBlob(google::protobuf::Message& message,
    const google::protobuf::FieldDescriptor& fieldDescriptor, const std::string& value)
{
    auto reflection = message.GetReflection();
    auto sub_message = reflection->MutableMessage(&message, &fieldDescriptor);
    sub_message->ParseFromArray(value.c_str(), static_cast<int32_t>(value.length()));
}

std::string DBProtoMessageToString(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor)
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
        strRet = DBProtoMessageToStringAsBlob(message, fieldDescriptor);
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

std::string DBProtoMessageToStringAsBlob(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor)
{
    return message.SerializeAsString();
}

std::string PbMessageKVHandle::GetMessageElementKey(const std::string& strTabName, const std::string& strCommonKeyList, 
    google::protobuf::Message& message, const google::protobuf::Reflection* pReflection, const google::protobuf::FieldDescriptor* pFieldDescriptor)
{
    std::string strElementKey;
    strElementKey.append(strTabName).append(strTableSpacer).append(strCommonKeyList);
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

        AppendRawData(strElementKey, DBProtoMessageToString(*pMsg, *pMsgFieldDescriptor));
    }

    return strElementKey;
}

std::string PbMessageKVHandle::GetMessageRepeatedElementKey(const std::string& strTabName, const std::string& strCommonKeyList, int iChildField,
    google::protobuf::Message& message, const google::protobuf::Reflection* pReflection, const google::protobuf::FieldDescriptor* pFieldDescriptor)
{
    std::string strElementKey;
    strElementKey.append(strTabName).append(strTableSpacer).append(strCommonKeyList);
    auto pMsg = &(pReflection->GetRepeatedMessage(message, pFieldDescriptor, iChildField));

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

        AppendRawData(strElementKey, DBProtoMessageToString(*pMsg, *pMsgFieldDescriptor));
    }

    return strElementKey;
}

int32_t PbMessageKVHandle::MessageLoadOnKV(google::protobuf::Message& message)
{
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();

    std::string strCommonKeyList;
    // loop tables
    for (int iField = 0; iField < pDescriptor->field_count(); ++iField) {
        const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(iField);
        if (!pFieldDescriptor) {
            continue;
        }
        auto strTabName = pFieldDescriptor->name();
        // add common key
        if (IsDBCommonKeyType(pFieldDescriptor->cpp_type())) {
            AppendRawData(strCommonKeyList, DBProtoMessageToString(message, *pFieldDescriptor));
            continue;
        }

        std::vector<std::string> vecKeySet;
        // get data keys
        if (pFieldDescriptor->is_repeated() && pReflection->FieldSize(message, pFieldDescriptor) > 0) {
            for (int iChildField = 0; iChildField < pReflection->FieldSize(message, pFieldDescriptor); ++iChildField) {
                std::string strElementKey = GetMessageRepeatedElementKey(strTabName, strCommonKeyList, iChildField, message, pReflection, pFieldDescriptor);
                vecKeySet.emplace_back(std::move(strElementKey));
            }
        }
        else if (!pFieldDescriptor->is_repeated() && pReflection->HasField(message, pFieldDescriptor)) {
            std::string strElementKey = GetMessageElementKey(strTabName, strCommonKeyList, message, pReflection, pFieldDescriptor);
            vecKeySet.emplace_back(std::move(strElementKey));
        }
        else {
            // load range list keys
            std::string strCommonKeyResult;
            std::string strCommonKey;
            strCommonKey.append(strTabName).append(strTableSpacer).append(strCommonKeyList);
            funGet(strCommonKey, strCommonKeyResult);
            PaserList(strCommonKeyResult, vecKeySet);
        }

        // get element data
        for (auto& fullKey : vecKeySet) {
            std::string strValueResult;
            funGet(fullKey, strValueResult);

            if (pFieldDescriptor->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
                continue;
            }

            google::protobuf::Message* pMsg = nullptr;
            if (pFieldDescriptor->is_repeated()) {
                pMsg = pReflection->AddMessage(&message, pFieldDescriptor);
            } else {
                pMsg = pReflection->MutableMessage(&message, pFieldDescriptor);
            }

            pMsg->ParseFromString(strValueResult);
        }
    }

    return 0;
}

int32_t PbMessageKVHandle::MessageUpdateOnKV(google::protobuf::Message& message)
{
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();

    std::string strCommonKeyList;
    // loop tables
    for (int iField = 0; iField < pDescriptor->field_count(); ++iField) {
        const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(iField);
        if (!pFieldDescriptor) {
            continue;
        }

        auto strTabName = pFieldDescriptor->name();
        // handle common keys
        if (IsDBCommonKeyType(pFieldDescriptor->cpp_type())) {
            AppendRawData(strCommonKeyList, DBProtoMessageToString(message, *pFieldDescriptor));
            continue;
        }

        if (pFieldDescriptor->is_repeated() && pReflection->FieldSize(message, pFieldDescriptor) <= 0) {
            continue;
        }
        if (!pFieldDescriptor->is_repeated() && !pReflection->HasField(message, pFieldDescriptor)) {
            continue;
        }

        // get data keys
        std::string strCommonKeyResult;
        std::string strCommonKey;
        strCommonKey.append(strTabName).append(strTableSpacer).append(strCommonKeyList);
        funGet(strCommonKey, strCommonKeyResult);

        std::unordered_set<std::string> setKeys;
        PaserList(strCommonKeyResult, setKeys);

        bool bSetKeysModify = false;
        if (pFieldDescriptor->is_repeated()) {
            for (int iChildField = 0; iChildField < pReflection->FieldSize(message, pFieldDescriptor); ++iChildField) {
                std::string strElementKey = GetMessageRepeatedElementKey(strTabName, strCommonKeyList, iChildField, message, pReflection, pFieldDescriptor);
                auto pMsg = &(pReflection->GetRepeatedMessage(message, pFieldDescriptor, iChildField));

                if (bSetKeysModify == false) {
                    if (!setKeys.contains(strElementKey)) {
                        bSetKeysModify = true;
                        setKeys.insert(strElementKey);
                    }
                }

                funPut(strElementKey, pMsg->SerializeAsString());
            }
        } else {
            std::string strElementKey = GetMessageElementKey(strTabName, strCommonKeyList, message, pReflection, pFieldDescriptor);
            auto pMsg = pReflection->MutableMessage(&message, pFieldDescriptor);
            if (bSetKeysModify == false) {
                if (!setKeys.contains(strElementKey)) {
                    bSetKeysModify = true;
                    setKeys.insert(strElementKey);
                }
            }

            funPut(strElementKey, pMsg->SerializeAsString());
        }

        if (bSetKeysModify) {
            std::string strKeySet;
            PacketList(setKeys, strKeySet);
            funPut(strCommonKey, strKeySet);
        }
    }

    return 0;
}

int32_t PbMessageKVHandle::MessageDeleteOnKV(google::protobuf::Message& message)
{
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();

    std::string strCommonKeyList;
    // loop tables
    for (int iField = 0; iField < pDescriptor->field_count(); ++iField) {
        const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(iField);
        if (!pFieldDescriptor) {
            continue;
        }

        auto strTabName = pFieldDescriptor->name();
        // handle common keys
        if (IsDBCommonKeyType(pFieldDescriptor->cpp_type())) {
            AppendRawData(strCommonKeyList, DBProtoMessageToString(message, *pFieldDescriptor));
            continue;
        }

        if (pFieldDescriptor->is_repeated() && pReflection->FieldSize(message, pFieldDescriptor) <= 0) {
            continue;
        }
        if (!pFieldDescriptor->is_repeated() && !pReflection->HasField(message, pFieldDescriptor)) {
            continue;
        }

        // get data keys
        std::string strCommonKeyResult;
        std::string strCommonKey;
        strCommonKey.append(strTabName).append(strTableSpacer).append(strCommonKeyList);
        funGet(strCommonKey, strCommonKeyResult);

        std::unordered_set<std::string> setKeys;
        PaserList(strCommonKeyResult, setKeys);

        if (pFieldDescriptor->is_repeated()) {
            for (int iChildField = 0; iChildField < pReflection->FieldSize(message, pFieldDescriptor); ++iChildField) {
                std::string strElementKey = GetMessageRepeatedElementKey(strTabName, strCommonKeyList, iChildField, message, pReflection, pFieldDescriptor);
                setKeys.erase(strElementKey);
                funDel(strElementKey);
            }
        } else {
            std::string strElementKey = GetMessageElementKey(strTabName, strCommonKeyList, message, pReflection, pFieldDescriptor);
            setKeys.erase(strElementKey);
            funDel(strElementKey);
        }

        if (setKeys.empty()) {
            funDel(strCommonKey);
        } else {
            std::string strKeySet;
            PacketList(setKeys, strKeySet);
            funPut(strCommonKey, strKeySet);
        }
    }

    return 0;
}

TONY_CAT_SPACE_END
