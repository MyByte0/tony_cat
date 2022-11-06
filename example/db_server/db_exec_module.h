#ifndef GATE_SERVER_CLIENT_PB_MODULE_H_
#define GATE_SERVER_CLIENT_PB_MODULE_H_

#include "common/core_define.h"
#include "common/module_base.h"
#include "common/module_manager.h"
#include "common/net/net_pb_module.h"
#include "common/net/net_session.h"
#include "protocol/db_data.pb.h"
#include "protocol/server_base.pb.h"
#include "protocol/server_db.pb.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

TONY_CAT_SPACE_BEGIN

class MysqlModule;
class NetModule;
class ServiceGovernmentModule;

class DBExecModule : public ModuleBase {
public:
    explicit DBExecModule(ModuleManager* pModuleManager);
    ~DBExecModule();
    virtual void BeforeInit() override;

public:
    typedef std::string USER_ID;

    struct GateUserInfo {
        USER_ID userId;
        Session::session_id_t userSessionId;
    };

    typedef std::shared_ptr<GateUserInfo> GateUserInfoPtr;

private:
    void OnHandleSSSaveDataReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSSaveDataReq& msgReq);
    void OnHandleSSQueryDataReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSQueryDataReq& msgReq);

private:
    int32_t LoadMessage(google::protobuf::Message& message);    
    int32_t UpdateMessage(google::protobuf::Message& message); 
    int32_t DeleteMessage(google::protobuf::Message& message); 
    void StringToProtoMessage(google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor, const std::string& value);
    void StringToProtoMessageAsBlob(google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor, const std::string& value);

    std::string ProtoMessageToString(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor);
    std::string ProtoMessageToStringAsBlob(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor);

    bool IsKey(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor);
    bool IsCommonKeyType(google::protobuf::FieldDescriptor::CppType);
    void AddQueryCondition(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor, std::string& strQuerys, std::vector<std::string>& vecArgs);

    template<class _Func>
    void ForEachMessageData(google::protobuf::Message& message, _Func func)
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
            }
            else {
                auto pSubDescriptor = pFieldDescriptor->message_type();
                auto pMsg = pReflection->MutableMessage(&message, pFieldDescriptor);
                func(*pMsg, pFieldDescriptor);
            }
        }
    }

private:
    void OnTest();

private:
    NetPbModule* m_pNetPbModule = nullptr;
    MysqlModule* m_pMysqlModule = nullptr;

};

TONY_CAT_SPACE_END

#endif // GATE_SERVER_CLIENT_PB_MODULE_H_
