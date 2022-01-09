#ifndef GATE_SERVER_CLIENT_PB_MODULE_H_
#define GATE_SERVER_CLIENT_PB_MODULE_H_

#include "common/core_define.h"
#include "common/module_base.h"
#include "common/module_manager.h"
#include "net/net_pb_module.h"
#include "net/net_session.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

TONY_CAT_SPACE_BEGIN

class NetModule;
class ServiceGovernmentModule;

// NetPacket:
// || 4 bytes packetLen(PbPacket Len) || 4 bytes checkCode(for PbPacket) || 4 bytes msgType || PbPacket ||
// the PbPacket in NetPacket:
// || 4 bytes ，，，，，，，，，，，，，，，，，，，，，，，，，，，，，，，，，， Len || PbPacketHead || PbPacketBody ||

class ClientPbModule : public NetPbModule {
public:
    ClientPbModule(ModuleManager* pModuleManager);
    ~ClientPbModule();
    virtual void BeforeInit() override;
    virtual void OnInit() override;

public:
    typedef std::string USER_ID;

    struct GateUserInfo {
        USER_ID userId;
        Session::session_id_t userSessionId;
    };

    typedef std::shared_ptr<GateUserInfo> GateUserInfoPtr;

private:
    void OnClientMessage(Session::session_id_t clientSessionId, uint32_t msgType, const char* data, std::size_t length);
    void OnServerMessage(Session::session_id_t serverSessionId, uint32_t msgType, const char* data, std::size_t length);

    void OnUserLogin(Session::session_id_t clientSessionId);
    GateUserInfoPtr GetGateUserInfoByUserId(USER_ID userId);
    GateUserInfoPtr GetGateUserInfoBySessionId(Session::session_id_t clientSessionId);

private:
    ServiceGovernmentModule* m_pServiceGovernmentModule = nullptr;
    NetPbModule* m_pNetPbModule = nullptr;

private:
    std::unordered_map<Session::session_id_t, GateUserInfoPtr> m_mapSessionUserInfo;
    std::unordered_map<USER_ID, GateUserInfoPtr> m_mapUserInfo;
};

TONY_CAT_SPACE_END

#endif // GATE_SERVER_CLIENT_PB_MODULE_H_
