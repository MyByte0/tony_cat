#ifndef COMMON_SERVICE_SERVICE_GOVERNMENT_MODULE_H_
#define COMMON_SERVICE_SERVICE_GOVERNMENT_MODULE_H_

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

#include "common/core_define.h"
#include "common/loop/loop_coroutine.h"
#include "common/module_base.h"
#include "common/net/net_memory_pool.h"
#include "common/net/net_pb_module.h"
#include "common/net/net_session.h"
#include "common/var_define.h"
#include "protocol/server_base.pb.h"
#include "protocol/server_common.pb.h"

TONY_CAT_SPACE_BEGIN

class NetModule;
class XmlConfigModule;
class RpcModule;

class ServiceGovernmentModule : public ModuleBase {
public:
    explicit ServiceGovernmentModule(ModuleManager* pModuleManager);
    virtual ~ServiceGovernmentModule();

    void BeforeInit() override;
    void OnInit() override;

    void InitConfig();
    void InitListen();
    void InitConnect();

public:
    enum : int {
        kReconnectServerTimeMillSeconds = 10000,
        kHeartbeatServerTimeMillSeconds = 12000,
    };

    struct ServerInstanceInfo {
        int64_t nServerConfigId = 0;
        int32_t nServerType = 0;
        int32_t nServerIndex = 0;
        Session::session_id_t nSessionId = 0;
        std::string strServerIp;
        int32_t nPort = 0;
        std::string strPublicIp;
        int32_t nPublicPort = 0;
        Loop::TimerHandle timerReconnect = nullptr;
        Loop::TimerHandle timerHeartbeat = nullptr;
    };

    DEFINE_MEMBER_STR_PUBLIC(ServerName);
    DEFINE_MEMBER_INT32_PUBLIC(ServerType);
    DEFINE_MEMBER_INT32_PUBLIC(ServerId);
    DEFINE_MEMBER_STR_PUBLIC(ServerIp);
    DEFINE_MEMBER_INT32_PUBLIC(ServerPort);
    DEFINE_MEMBER_STR_PUBLIC(PublicIp);
    DEFINE_MEMBER_INT32_PUBLIC(PublicPort);
    DEFINE_MEMBER_VAR_PUBLIC(ServerInstanceInfo, MineServerInfo);

public:
    void SetServerInstance(const std::string& strName, int32_t nServerType,
                           uint32_t nServerId) {
        SetServerName(strName);
        SetServerId(nServerId);
        SetServerType(nServerType);
    }

    void SetServerInstance(const std::string_view& strName, int32_t nServerType,
                           uint32_t nServerId) {
        SetServerName(strName);
        SetServerId(nServerId);
        SetServerType(nServerType);
    }

public:
    int32_t GetServerKeyIndex(int32_t nServerType, const ::std::string& strKey);
    ServerInstanceInfo* GetServerInstanceInfo(int32_t nServerType,
                                              int32_t nServerId);
    Session::session_id_t GetServerSessionId(int32_t nServerType,
                                             int32_t nServerId);
    Session::session_id_t GetServerSessionIdByKey(int32_t nServerType,
                                                  const std::string& strKey);
    void OnHandleSSHeartbeatReq(
        Session::session_id_t sessionId,
        NetMemoryPool::PacketNode<Pb::ServerHead> head,
        NetMemoryPool::PacketNode<Pb::SSHeartbeatReq> heartbeat);

private:
    void ConnectServerInstance(const ServerInstanceInfo& stServerInstanceInfo);
    void OnConnectSucess(Session::session_id_t nSessionId,
                         const ServerInstanceInfo& stServerInstanceInfo,
                         bool bSuccess);
    void OnDisconnect(Session::session_id_t nSessionId,
                      const ServerInstanceInfo& stServerInstanceInfo);

    CoroutineTask<void> OnHeartbeat(ServerInstanceInfo stServerInstanceInfo);

public:
    static bool AddressToIpPort(const std::string& strAddress,
                                std::string& strIp, int32_t& nPort);

private:
    NetModule* m_pNetModule = nullptr;
    NetPbModule* m_pNetPbModule = nullptr;
    RpcModule* m_pRpcModule = nullptr;
    XmlConfigModule* m_pXmlConfigModule = nullptr;

    std::unordered_map<int32_t, std::map<int32_t, ServerInstanceInfo>>
        m_mapConnectServerList;
    std::unordered_map<int32_t, std::map<int32_t, ServerInstanceInfo>>
        m_mapServerList;
};

TONY_CAT_SPACE_END

#endif  // COMMON_SERVICE_SERVICE_GOVERNMENT_MODULE_H_
