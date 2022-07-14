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

class NetModule;
class ServiceGovernmentModule;

class DBExecModule : public ModuleBase {
public:
    DBExecModule(ModuleManager* pModuleManager);
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
    struct DBKeyValue {
        enum EDBValueType{
            eTypeInvalid = 0,
            eTypeBool = 1,
            eTypeInt = 2,
            eTypeUint = 3,
            eTypeBigInt = 4,
            eTypeBigUint = 5,
            eTypeFloat = 6,
            eTypeDouble = 7,
            eTypeString = 8,
            eTypeBinary = 9,
        };
        EDBValueType eType = eTypeInvalid;
        std::string strKey;
        std::string strValue;
    };

    struct DBRecord {
        std::vector<DBKeyValue> vecPriKeyFields;
        std::vector<DBKeyValue> vecValueFields;
    };
    
    typedef std::map<std::string, std::vector<DBRecord>> MapTableRecord;

    void PaserProtoTables(const google::protobuf::Message& message, MapTableRecord& mapTableRecord);

    void PaserMsgProto(const std::vector<DBKeyValue>& vecPriKeys, std::vector<DBRecord>& vecResultList, 
        const google::protobuf::Message& message, bool bWithValue = true);
    void PaserMapProto(const std::vector<DBKeyValue>& vecPriKeys, std::vector<DBRecord>& vecResultList,
        const google::protobuf::Message& message, bool bWithValue = true);

    bool IsProtoBasePrimeKeyType(const google::protobuf::FieldDescriptor& fieldDescriptor);
    void PaserProtoBasePrimeKey(std::vector<DBKeyValue>& vecCommonKeys, const google::protobuf::Message& message,
        const google::protobuf::FieldDescriptor& fieldDescriptor, const std::string& strKayName);
    std::string PaserStringValue(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor);
    DBKeyValue::EDBValueType GetDBType(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor& fieldDescriptor);

private:
    NetPbModule* m_pNetPbModule = nullptr;

private:
    std::unordered_map<Session::session_id_t, GateUserInfoPtr> m_mapSessionUserInfo;
    std::unordered_map<USER_ID, GateUserInfoPtr> m_mapUserInfo;
};

TONY_CAT_SPACE_END

#endif // GATE_SERVER_CLIENT_PB_MODULE_H_
