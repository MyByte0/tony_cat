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
    NetPbModule* m_pNetPbModule = nullptr;
    MysqlModule* m_pMysqlModule = nullptr;

};

TONY_CAT_SPACE_END

#endif // GATE_SERVER_CLIENT_PB_MODULE_H_
