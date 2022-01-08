#ifndef SERVICE_GOVERNMENT_MODULE_H_
#define SERVICE_GOVERNMENT_MODULE_H_

#include "common/core_define.h"
#include "common/module_base.h"
#include "common/loop.h"
#include "common/loop_coroutine.h"
#include "common/loop_pool.h"
#include "common/var_define.h"
#include "net/net_session.h"
#include "protocol/server_base.pb.h"
#include "protocol/server_common.pb.h"

#include <cstdint>
#include <coroutine>
#include <functional>
#include <thread>
#include <unordered_map>

SER_NAME_SPACE_BEGIN

class NetModule;
class NetPbModule;
class XmlConfigModule;

class ServiceGovernmentModule : public ModuleBase {
public:
    ServiceGovernmentModule(ModuleManager* pModuleManager);
    virtual ~ServiceGovernmentModule();

    virtual void BeforeInit() override;

    void InitConfig();
    void InitListen();
    void InitConnect();

public:
    struct ServerInstanceInfo {
        std::string strServerName;
        int32_t nId = 0;
        Session::session_id_t nSessionId = 0;
        std::string strServerIp;
        int32_t nPort = 0;
    };

    DEFINE_MEMBER_STR(ServerName);
    DEFINE_MEMBER_UINT32(ServerId);
    DEFINE_MEMBER_STR(ServerIp);
    DEFINE_MEMBER_INT32(ServerPort);
    DEFINE_MEMBER_VAR(ServerInstanceInfo, ServerInfo);

    Session::session_id_t GetServerSessionId(const std::string& strServerName, int32_t nServerId);

public:
    void OnHandleSSHeartbeatReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSHeartbeatReq& heartbeat);

private:
    static bool AdressToIpPort(const std::string& strAddress, std::string& strIp, int32_t& nPort);

private:
    NetModule*          m_pNetModule = nullptr;
    NetPbModule*        m_pNetPbModule = nullptr;
    XmlConfigModule*    m_pXmlConfigModule = nullptr;

    // map<ServerName, map<ServerId, ServerInstanceInfo>>
    std::unordered_map<std::string, std::map<int32_t, ServerInstanceInfo> > m_mapServerConnectList;
};

SER_NAME_SPACE_END

#endif  // SERVICE_GOVERNMENT_MODULE_H_
