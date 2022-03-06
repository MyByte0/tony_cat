#ifndef CLIENT_CLIENT_MANAGER_MODULE_H_
#define CLIENT_CLIENT_MANAGER_MODULE_H_

#include "common/core_define.h"
#include "common/loop/loop_coroutine.h"
#include "common/module_base.h"
#include "common/module_manager.h"
#include "common/net/net_session.h"
#include "protocol/client_common.pb.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

TONY_CAT_SPACE_BEGIN

class RpcModule;
class NetPbModule;
class ServiceGovernmentModule;

class ClientManagerModule : public ModuleBase {
public:
    ClientManagerModule(ModuleManager* pModuleManager);
    ~ClientManagerModule();

    virtual void BeforeInit() override;
    virtual void OnInit() override;

public:
    typedef std::string ACCOUNT_ID;
    struct ClientData {
        ACCOUNT_ID strAccount;
        Session::session_id_t nSessionId = 0;
    };
    typedef std::shared_ptr<ClientData> ClientDataPtr;

public:
    ClientDataPtr GetPlayerData(const ACCOUNT_ID& user_id);

private:
    CoroutineTask<void> StartTestClient();

private:
    std::unordered_map<ACCOUNT_ID, ClientDataPtr> m_mapClient;

private:
    RpcModule* m_pRpcModule = nullptr;
    NetPbModule* m_pNetPbModule = nullptr;
    ServiceGovernmentModule* m_pServiceGovernmentModule = nullptr;
};

TONY_CAT_SPACE_END

#endif // CLIENT_CLIENT_MANAGER_MODULE_H_
