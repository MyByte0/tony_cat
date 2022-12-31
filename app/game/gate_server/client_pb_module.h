#ifndef GATE_SERVER_CLIENT_PB_MODULE_H_
#define GATE_SERVER_CLIENT_PB_MODULE_H_

#include "common/core_define.h"
#include "common/module_base.h"
#include "common/module_manager.h"
#include "common/net/net_pb_module.h"
#include "common/net/net_session.h"
#include "game/data_define.h"
#include "protocol/client_base.pb.h"
#include "protocol/client_common.pb.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

TONY_CAT_SPACE_BEGIN

class NetModule;
class RpcModule;
class ServiceGovernmentModule;

class ClientPbModule : public NetPbModule {
public:
    explicit ClientPbModule(ModuleManager* pModuleManager);
    ~ClientPbModule();
    virtual void BeforeInit() override;
    virtual void OnInit() override;

public:
    struct GateUserInfo {
        USER_ID userId;
        Session::session_id_t userSessionId;
    };

    typedef std::shared_ptr<GateUserInfo> GateUserInfoPtr;

private:
    void OnClientMessage(Session::session_id_t clientSessionId, uint32_t msgType, const char* data, std::size_t length);
    void OnServerMessage(Session::session_id_t serverSessionId, uint32_t msgType, const char* data, std::size_t length);

    void OnHandleCSPlayerLoginReq(Session::session_id_t sessionId, Pb::ClientHead& head, Pb::CSPlayerLoginReq& playerLoginReq);

    bool OnUserLogin(Session::session_id_t clientSessionId, USER_ID userId);
    bool OnUserLogout(USER_ID userId, bool bKick);
    GateUserInfoPtr GetGateUserInfoByUserId(USER_ID userId);
    GateUserInfoPtr GetGateUserInfoBySessionId(Session::session_id_t clientSessionId);

private:
    NetPbModule* m_pNetPbModule = nullptr;
    ServiceGovernmentModule* m_pServiceGovernmentModule = nullptr;
    RpcModule* m_pRpcModule = nullptr;

private:
    std::unordered_map<Session::session_id_t, GateUserInfoPtr> m_mapSessionUserInfo;
    std::unordered_map<USER_ID, GateUserInfoPtr> m_mapUserInfo;
};

TONY_CAT_SPACE_END

#endif // GATE_SERVER_CLIENT_PB_MODULE_H_
