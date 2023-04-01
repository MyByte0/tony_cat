#ifndef APP_GAME_CLIENT_CLIENT_MANAGER_MODULE_H_
#define APP_GAME_CLIENT_CLIENT_MANAGER_MODULE_H_

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "common/core_define.h"
#include "common/loop/loop_coroutine.h"
#include "common/module_base.h"
#include "common/module_manager.h"
#include "common/net/net_session.h"
#include "protocol/client_common.pb.h"

TONY_CAT_SPACE_BEGIN

class RpcModule;
class NetPbModule;
class ServiceGovernmentModule;

class ClientManagerModule : public ModuleBase {
 public:
    explicit ClientManagerModule(ModuleManager* pModuleManager);
    ~ClientManagerModule();

    void BeforeInit() override;
    void OnInit() override;

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
    CoroutineAsyncTask<int32_t> CreateNewClient();

 private:
    std::unordered_map<ACCOUNT_ID, ClientDataPtr> m_mapClient;

 private:
    RpcModule* m_pRpcModule = nullptr;
    NetPbModule* m_pNetPbModule = nullptr;
    ServiceGovernmentModule* m_pServiceGovernmentModule = nullptr;
};

TONY_CAT_SPACE_END

#endif  // APP_GAME_CLIENT_CLIENT_MANAGER_MODULE_H_
