#include "player_manager_module.h"

#include <cassert>

#include "app/game/server_define.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/service/rpc_module.h"
#include "common/service/service_government_module.h"
#include "protocol/client_base.pb.h"

TONY_CAT_SPACE_BEGIN

PlayerManagerModule::PlayerManagerModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager) {}

PlayerManagerModule::~PlayerManagerModule() {}

void PlayerManagerModule::BeforeInit() {
    m_pRpcModule = FIND_MODULE(m_pModuleManager, RpcModule);
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);
    m_pServiceGovernmentModule =
        FIND_MODULE(m_pModuleManager, ServiceGovernmentModule);

    m_pNetPbModule->RegisterHandle(
        this, &PlayerManagerModule::OnHandleCSPlayerLoginReq);
}

void PlayerManagerModule::OnHandleCSPlayerLoginReq(
    Session::session_id_t sessionId,
    NetMemoryPool::PacketNode<Pb::ServerHead> head,
    NetMemoryPool::PacketNode<Pb::CSPlayerLoginReq> playerLoginReq) {
    auto pPlayerData = GetPlayerData(head->user_id());
    if (nullptr != pPlayerData) {
        head->set_error_code(Pb::CSMessageCode::cs_msg_not_find_user);
        // \TODO: or kick player and return login success
        auto pClientMsgRsp =
            NetMemoryPool::PacketCreate<Pb::CSPlayerLoginRsp>();
        m_pNetPbModule->SendPacket(sessionId, head, pClientMsgRsp);
        return;
    }

    if (m_mapLoadingPlayer.count(head->user_id()) > 0) {
        head->set_error_code(Pb::CSMessageCode::cs_msg_user_loading);
        auto pClientMsgRsp =
            NetMemoryPool::PacketCreate<Pb::CSPlayerLoginRsp>();
        m_pNetPbModule->SendPacket(sessionId, head, pClientMsgRsp);
        return;
    }

    assert(m_mapOnlinePlayer.find(head->user_id()) == m_mapOnlinePlayer.end());
    m_mapLoadingPlayer.emplace(head->user_id(), std::make_shared<PlayerData>());

    // request msg data
    auto pQueryDataReq = NetMemoryPool::PacketCreate<Pb::SSQueryDataReq>();
    auto pQueryHeadReq = NetMemoryPool::PacketCreate<Pb::ServerHead>(*head);
    pQueryDataReq->set_user_id(head->user_id());
    auto querySessionId = m_pServiceGovernmentModule->GetServerSessionIdByKey(
        ServerType::DBServer, head->user_id());
    // respond msg data
    return m_pRpcModule->RpcRequest(
        querySessionId, pQueryHeadReq, pQueryDataReq,
        [=, this](
            Session::session_id_t nSessionIdRsp,
            NetMemoryPool::PacketNode<Pb::ServerHead> headRsp,
            NetMemoryPool::PacketNode<Pb::SSQueryDataRsp> msgRsp) mutable {
            do {
                if (headRsp->error_code() !=
                    Pb::SSMessageCode::ss_msg_success) {
                    LOG_ERROR("user loading failed, user_id:{}, error:{}",
                              head->user_id(), headRsp->error_code());
                    m_mapLoadingPlayer.erase(head->user_id());
                    // head->set_error_code(1);
                    break;
                }

                // move player from mapLoadingPlayer to mapOnlinePlayer,
                // and load playerData
                auto itMapLoadingPlayer =
                    m_mapLoadingPlayer.find(head->user_id());
                if (itMapLoadingPlayer == m_mapLoadingPlayer.end()) {
                    // head->set_error_code(1);
                    LOG_ERROR("not find data in loading map, user:{}",
                              head->user_id());
                    break;
                }

                auto pPlayData = itMapLoadingPlayer->second;
                if (nullptr == pPlayData) {
                    LOG_ERROR(" find data null in loading map, user:{}",
                              head->user_id());
                    // head->set_error_code(1);
                    break;
                }

                if (!LoadPlayerData(*pPlayData, *msgRsp)) {
                    LOG_ERROR(" load player data fail, user:{}",
                              head->user_id());
                    // head->set_error_code(1);
                    break;
                }

                m_mapOnlinePlayer.emplace(head->user_id(),
                                          itMapLoadingPlayer->second);
                m_mapLoadingPlayer.erase(itMapLoadingPlayer);
                LOG_INFO("loading player data success, user_id:{}",
                         head->user_id());
            } while (false);
            auto pClientMsgRsp =
                NetMemoryPool::PacketCreate<Pb::CSPlayerLoginRsp>();
            m_pNetPbModule->SendPacket(sessionId, head, pClientMsgRsp);
        });

    return;
}

PlayerDataPtr PlayerManagerModule::GetPlayerData(const USER_ID& user_id) {
    if (auto itMapOnlinePlayer = m_mapOnlinePlayer.find(user_id);
        itMapOnlinePlayer != m_mapOnlinePlayer.end()) {
        return itMapOnlinePlayer->second;
    }

    return nullptr;
}

bool PlayerManagerModule::LoadPlayerData(PlayerData& playerData,
                                         const Pb::SSQueryDataRsp& queryData) {
    // \TODO

    return true;
}

TONY_CAT_SPACE_END
