#include "player_manager_module.h"
#include "common/module_manager.h"
#include "common/service/rpc_module.h"
#include "common/log/log_module.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/service/service_government_module.h"
#include "protocol/client_base.pb.h"
#include "protocol/server_base.pb.h"
#include "protocol/server_db.pb.h"

#include "server_common/server_define.h"

#include <cassert>

TONY_CAT_SPACE_BEGIN

PlayerManagerModule::PlayerManagerModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

PlayerManagerModule::~PlayerManagerModule() { }

void PlayerManagerModule::BeforeInit()
{
    m_pRpcModule = FIND_MODULE(m_pModuleManager, RpcModule);
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);
    m_pServiceGovernmentModule = FIND_MODULE(m_pModuleManager, ServiceGovernmentModule);

    m_pNetPbModule->RegisterHandle(this, &PlayerManagerModule::OnHandleCSPlayerLoginReq);
}


CoroutineTask<void> PlayerManagerModule::OnHandleCSPlayerLoginReq(Session::session_id_t sessionId, Pb::ServerHead head, Pb::CSPlayerLoginReq playerLoginReq)
{
    auto pPlayerData = GetPlayerData(head.user_id());
    if (nullptr != pPlayerData) {

        // \TODO: kick player and return login success
        co_return;
    }

    if (m_mapLoadingPlayer.count(head.user_id()) > 0) {
        // \TODO: on loiading, return fail
        co_return;
    }

    assert(m_mapOnlinePlayer.find(head.user_id()) == m_mapOnlinePlayer.end());
    m_mapLoadingPlayer.emplace(head.user_id(), std::make_shared<PlayerData>());

    Pb::SSQueryDataReq queryDataReq;
    queryDataReq.set_user_id(head.user_id());
    auto querySessionId = m_pServiceGovernmentModule->GetServerSessionIdByKey(ServerType::eTypeDBServer, head.user_id());

    Session::session_id_t nSessionIdRsp;
    Pb::ServerHead headRsp;
    Pb::SSQueryDataRsp msgRsp;
    co_await RpcModule::AwaitableRpcRequest(querySessionId, head, queryDataReq, nSessionIdRsp, headRsp, msgRsp);
    if (headRsp.error_code() != Pb::SSMessageCode::ss_msg_success) {
        LOG_ERROR("user loading failed, user_id:{}, error:{}", head.user_id(), head.error_code());
        m_mapLoadingPlayer.erase(head.user_id());
        co_return;
    }
    
    auto itMapLoadingPlayer = m_mapLoadingPlayer.find(head.user_id());
    if (itMapLoadingPlayer == m_mapLoadingPlayer.end()) {
        LOG_ERROR("not find data in loading map, user:{}", head.user_id());
        co_return;
    }

    auto pPlayData = itMapLoadingPlayer->second;
    // pPlayData = queryDataRsp.user_base();

    m_mapOnlinePlayer.emplace(head.user_id(), itMapLoadingPlayer->second);
    m_mapLoadingPlayer.erase(itMapLoadingPlayer);
    LOG_INFO("loading player data success, user_id:{}", head.user_id());

    Pb::CSPlayerLoginRsp clientMsgRsp;
    m_pNetPbModule->SendPacket(sessionId, head, clientMsgRsp);
    co_return;
}



PlayerDataPtr PlayerManagerModule::GetPlayerData(const USER_ID& user_id)
{
    auto itMapOnlinePlayer = m_mapOnlinePlayer.find(user_id);
    if (itMapOnlinePlayer != m_mapOnlinePlayer.end()) {
        return itMapOnlinePlayer->second;
    }

    return nullptr;
}

TONY_CAT_SPACE_END
