#ifndef SERVICE_GOVERNMENT_MODULE_H_
#define SERVICE_GOVERNMENT_MODULE_H_

#include "common/core_define.h"
#include "common/loop.h"
#include "common/loop_coroutine.h"
#include "common/loop_pool.h"
#include "common/module_base.h"
#include "common/var_define.h"
#include "net/net_session.h"
#include "protocol/server_base.pb.h"
#include "protocol/server_common.pb.h"

#include <coroutine>
#include <cstdint>
#include <functional>
#include <thread>
#include <unordered_map>

TONY_CAT_SPACE_BEGIN

class NetModule;
class NetPbModule;
class XmlConfigModule;
class RpcModule;

class ServiceGovernmentModule : public ModuleBase {
public:
    ServiceGovernmentModule(ModuleManager* pModuleManager);
    virtual ~ServiceGovernmentModule();

    virtual void BeforeInit() override;
    virtual void OnInit() override;

    void InitConfig();
    void InitListen();
    void InitConnect();

public:
    enum {
        kReconnectServerTimeMillSeconds = 15000,
    };

    struct ServerInstanceInfo {
        int32_t nServerType = 0;
        int32_t nServerIndex = 0;
        Session::session_id_t nSessionId = 0;
        std::string strServerIp;
        int32_t nPort = 0;
        Loop::TimerHandle timerReconnect;
        Loop::TimerHandle timerHeartbeat;
    };

    DEFINE_MEMBER_STR(ServerName);
    DEFINE_MEMBER_INT32(ServerType);
    DEFINE_MEMBER_UINT32(ServerId);
    DEFINE_MEMBER_STR(ServerIp);
    DEFINE_MEMBER_INT32(ServerPort);
    DEFINE_MEMBER_STR(PublicIp);
    DEFINE_MEMBER_INT32(PublicPort);
    DEFINE_MEMBER_VAR(ServerInstanceInfo, ServerInfo);

public:
    Session::session_id_t GetServerSessionId(int32_t nServerType, int32_t nServerId);
    void OnHandleSSHeartbeatReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSHeartbeatReq& heartbeat);

private:
    ServerInstanceInfo* GetServerInstanceInfo(int32_t nServerType, int32_t nServerId);
    Session::session_id_t ConnectServerInstance(const ServerInstanceInfo& stServerInstanceInfo);
    void OnConnectSucess(Session::session_id_t nSessionId, bool bSuccess);
    void OnDisconnect(Session::session_id_t nSessionId, ServerInstanceInfo stServerInstanceInfo);

private:
    static bool AddressToIpPort(const std::string& strAddress, std::string& strIp, int32_t& nPort);

private:
    NetModule* m_pNetModule = nullptr;
    NetPbModule* m_pNetPbModule = nullptr;
    RpcModule* m_pRpcModule = nullptr;
    XmlConfigModule* m_pXmlConfigModule = nullptr;

    // map<ServerType, map<ServerId, ServerInstanceInfo>>
    std::unordered_map<int32_t, std::map<int32_t, ServerInstanceInfo>> m_mapServerConnectList;
};

TONY_CAT_SPACE_END

#endif // SERVICE_GOVERNMENT_MODULE_H_
