#include "db_utils.h"

#include "db_define.h"

TONY_CAT_SPACE_BEGIN

DbPbFieldKey gPbKeys;

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

TONY_CAT_SPACE_END
