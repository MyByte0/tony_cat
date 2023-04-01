#ifndef APP_GAME_LOGIC_SERVER_PLAYER_MANAGER_MODULE_H_
#define APP_GAME_LOGIC_SERVER_PLAYER_MANAGER_MODULE_H_

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

#include "common/core_define.h"
#include "common/loop/loop_coroutine.h"
#include "common/module_base.h"
#include "common/module_manager.h"
#include "common/net/net_session.h"
#include "game/data_define.h"
#include "protoc/server_db.pb.h"
#include "protocol/client_common.pb.h"
#include "protocol/server_base.pb.h"

TONY_CAT_SPACE_BEGIN

class RpcModule;
class NetPbModule;
class ServiceGovernmentModule;

class PlayerManagerModule : public ModuleBase {
 public:
    explicit PlayerManagerModule(ModuleManager* pModuleManager);
    ~PlayerManagerModule();

    void BeforeInit() override;

 public:
    void OnHandleCSPlayerLoginReq(Session::session_id_t sessionId,
                                  Pb::ServerHead& head,
                                  Pb::CSPlayerLoginReq& playerLoginReq);

 public:
    PlayerDataPtr GetPlayerData(const USER_ID& user_id);

 private:
    bool LoadPlayerData(PlayerData& playerData,
                        const Pb::SSQueryDataRsp& queryData);

 private:
    std::unordered_map<USER_ID, PlayerDataPtr> m_mapLoadingPlayer;
    std::unordered_map<USER_ID, PlayerDataPtr> m_mapOnlinePlayer;

 private:
    RpcModule* m_pRpcModule = nullptr;
    NetPbModule* m_pNetPbModule = nullptr;
    ServiceGovernmentModule* m_pServiceGovernmentModule = nullptr;
};

TONY_CAT_SPACE_END

#endif  // APP_GAME_LOGIC_SERVER_PLAYER_MANAGER_MODULE_H_
