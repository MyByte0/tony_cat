
#ifndef DB_UTILS_H_
#define DB_UTILS_H_

#include "common/core_define.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <functional>
#include <string>
#include <unordered_set>

TONY_CAT_SPACE_BEGIN

bool IsDBKey(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor);

bool IsDBCommonKeyType(google::protobuf::FieldDescriptor::CppType);

void DBStringToProtoMessage(google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor, const std::string& value);

void DBStringToProtoMessageAsBlob(google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor, const std::string& value);

std::string DBProtoMessageToString(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor);

std::string DBProtoMessageToStringAsBlob(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor);


struct PbMessageKVHandle{
    typedef std::function<int32_t(const std::string&, std::string&)> KVGetCb;
    typedef std::function<int32_t(const std::string&, const std::string&)> KVPutCb;
    typedef std::function<int32_t(const std::string&)> KVDelCb;

    int32_t LoadMessageOnKV(google::protobuf::Message& message);
    int32_t UpdateMessageOnKV(google::protobuf::Message& message);
    int32_t DeleteMessageOnKV(google::protobuf::Message& message);

    KVGetCb funGet;
    KVPutCb funPut;
    KVDelCb funDel;

private:
    std::string GetMessageElementKey(const std::string& strTabName, const std::string& strCommonKeyList,
        google::protobuf::Message& message, const google::protobuf::Reflection* pReflection, const google::protobuf::FieldDescriptor* pFieldDescriptor);
    std::string GetMessageRepeatedElementKey(const std::string& strTabName, const std::string& strCommonKeyList, int iChildField,
        google::protobuf::Message& message, const google::protobuf::Reflection* pReflection, const google::protobuf::FieldDescriptor* pFieldDescriptor);
};



template <class _Func>
void ForEachDBMessageData(google::protobuf::Message& message, const _Func& func)
{
    auto pReflection = message.GetReflection();
    auto pDescriptor = message.GetDescriptor();

    for (int iField = 0; iField < pDescriptor->field_count(); ++iField) {
        const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(iField);
        if (!pFieldDescriptor) {
            continue;
        }
        if (pFieldDescriptor->cpp_type() != google::protobuf::FieldDescriptor::CppType::CPPTYPE_MESSAGE) {
            continue;
        }
        if (pFieldDescriptor->is_repeated() && pReflection->FieldSize(message, pFieldDescriptor) <= 0) {
            continue;
        }
        if (!pFieldDescriptor->is_repeated() && !pReflection->HasField(message, pFieldDescriptor)) {
            continue;
        }

        if (pFieldDescriptor->is_repeated()) {
            for (int iChildField = 0; iChildField < pReflection->FieldSize(message, pFieldDescriptor); ++iChildField) {
                auto pMsg = pReflection->MutableRepeatedMessage(&message, pFieldDescriptor, iChildField);
                func(*pMsg, pFieldDescriptor);
            }
        } else {
            auto pSubDescriptor = pFieldDescriptor->message_type();
            auto pMsg = pReflection->MutableMessage(&message, pFieldDescriptor);
            func(*pMsg, pFieldDescriptor);
        }
    }
}

TONY_CAT_SPACE_END

#endif // DB_UTILS_H_
