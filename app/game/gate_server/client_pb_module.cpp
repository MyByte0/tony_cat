#include "client_pb_module.h"

#include "app/game/server_define.h"
#include "common/config/xml_config_module.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/service/rpc_module.h"
#include "common/service/service_government_module.h"
#include "common/utility/crc.h"
#include "protocol/client_base.pb.h"
#include "protocol/server_base.pb.h"

TONY_CAT_SPACE_BEGIN

ClientPbModule::ClientPbModule(ModuleManager* pModuleManager)
    : NetPbModule(pModuleManager) {}

ClientPbModule::~ClientPbModule() {}

void ClientPbModule::BeforeInit() {
    m_pNetModule = FIND_MODULE(m_pModuleManager, NetModule);
    m_pServiceGovernmentModule =
        FIND_MODULE(m_pModuleManager, ServiceGovernmentModule);
    m_pXmlConfigModule = FIND_MODULE(m_pModuleManager, XmlConfigModule);

    m_workLoop = std::make_shared<LoopPool>();
    m_pRpcModule = FIND_MODULE(m_pModuleManager, RpcModule);
}

void ClientPbModule::OnInit() {
    NetPbModule::OnInit();
    Listen(m_pServiceGovernmentModule->GetPublicIp(),
           m_pServiceGovernmentModule->GetPublicPort());

    this->RegisterHandle(this, &ClientPbModule::OnHandleCSPlayerLoginReq);
    this->SetDefaultPacketHandle(std::bind(
        &ClientPbModule::OnClientMessage, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    SetDefaultPacketHandle(std::bind(
        &ClientPbModule::OnServerMessage, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void ClientPbModule::OnClientMessage(Session::session_id_t clientSessionId,
                                     uint32_t msgType, const char* data,
                                     std::size_t length) {
    if (msgType >= Pb::SSMessageId::ss_msgid_begin) {
        LOG_WARN("client req invalid msg_id, sessionId:{} msg_id:{}",
                 clientSessionId, msgType);
        return;
    }

    const char* pData = nullptr;
    std::size_t pbPacketBodyLen = 0;
    Pb::ClientHead clientHead;
    PaserPbHead(data, length, clientHead, pData, pbPacketBodyLen);

    auto pUserInfo = GetGateUserInfoBySessionId(clientSessionId);
    if (nullptr == pUserInfo) {
        LOG_ERROR("not find user info, clientSessionId:{}", clientSessionId);
        return;
    }
    auto pServerHead = NetMemoryPool::PacketCreate<Pb::ServerHead>();
    pServerHead->set_user_id(pUserInfo->userId);

    auto pbPacket = NetMemoryPool::PacketCreate<NetPbModule::StrData>(
        pData, pbPacketBodyLen);
    SendPacket(m_pServiceGovernmentModule->GetServerSessionId(
                   ServerType::LogicServer, 1),
               msgType, pServerHead, pbPacket);
}

void ClientPbModule::OnServerMessage(Session::session_id_t serverSessionId,
                                     uint32_t msgType, const char* data,
                                     std::size_t length) {
    const char* pData = nullptr;
    std::size_t pbPacketBodyLen = 0;
    auto pServerHead = NetMemoryPool::PacketCreate<Pb::ServerHead>();
    // auto userId = serverHead.user_id();
    PaserPbHead(data, length, *pServerHead, pData, pbPacketBodyLen);

    Pb::ClientHead clientHead;
    USER_ID userId;
    if (auto pUserInfo = GetGateUserInfoByUserId(userId);
        nullptr != pUserInfo) {
        auto pbPacket = NetMemoryPool::PacketCreate<NetPbModule::StrData>(
            pData, pbPacketBodyLen);
        SendPacket(pUserInfo->userSessionId, msgType, pServerHead, pbPacket);
    } else {
        LOG_ERROR("not find user info, userId:{}", userId);
    }
    return;
}

void ClientPbModule::OnHandleCSPlayerLoginReq(
    Session::session_id_t sessionId,
    NetMemoryPool::PacketNode<Pb::ClientHead> head,
    NetMemoryPool::PacketNode<Pb::CSPlayerLoginReq> playerLoginReq) {
    playerLoginReq->user_name();
    playerLoginReq->token();

    // \TODO : check token fail return
    // Get user_id by token

    USER_ID userId = playerLoginReq->user_name();
    auto sessionServer = m_pServiceGovernmentModule->GetServerSessionIdByKey(
        ServerType::LogicServer, userId);
    auto pHeadServer = NetMemoryPool::PacketCreate<Pb::ServerHead>();
    pHeadServer->set_user_id(userId);
    m_pRpcModule->RpcRequest(
        sessionServer, pHeadServer, playerLoginReq,
        [this, sessionId, head](
            Session::session_id_t serverSessionId,
            NetMemoryPool::PacketNode<Pb::ServerHead> headRsp,
            NetMemoryPool::PacketNode<Pb::CSPlayerLoginRsp>
                queryDataRsp) mutable {
            if (headRsp->error_code() != Pb::SSMessageCode::ss_msg_success) {
                LOG_ERROR(
                    "server PlayerLogin request error:{}, sessionId:{} user{}",
                    headRsp->error_code(), serverSessionId, headRsp->user_id());
                return;
            }

            if (!OnUserLogin(sessionId, headRsp->user_id())) {
                LOG_ERROR("server OnUserLogin user:{}, sessionId:{}",
                          headRsp->user_id(), sessionId);
                return;
            }

            SendPacket(sessionId, head, queryDataRsp);
        });
}

bool ClientPbModule::OnUserLogin(Session::session_id_t clientSessionId,
                                 USER_ID userId) {
    if (m_mapSessionUserInfo.count(clientSessionId) > 0) {
        LOG_ERROR("map seassion clientSessionId aleardy used, id:{}",
                  clientSessionId);
        return false;
    }

    if (m_mapUserInfo.count(userId) > 0) {
        bool bKick = true;
        if (!OnUserLogout(userId, bKick)) {
            LOG_ERROR("UserLogout faield, userId:{}, SessionId:{}", userId,
                      clientSessionId);
        }
    }

    auto pUserInfo = std::make_shared<GateUserInfo>();
    pUserInfo->userId = userId;
    pUserInfo->userSessionId = clientSessionId;
    m_mapSessionUserInfo.emplace(clientSessionId, pUserInfo);
    m_mapUserInfo.emplace(userId, pUserInfo);
    LOG_INFO("user login success, user:{}, sessionId:{}", userId,
             clientSessionId);
    return true;
}

bool ClientPbModule::OnUserLogout(USER_ID userId, bool bKick) {
    auto itMapUserInfo = m_mapUserInfo.find(userId);
    if (itMapUserInfo == m_mapUserInfo.end()) {
        LOG_ERROR("user kick out not find userinfo, user:{}", userId);
        return false;
    }
    auto pUserInfo = itMapUserInfo->second;
    if (pUserInfo == nullptr) {
        LOG_ERROR("user pUserInfo null, user:{}", userId);
        return false;
    }
    LOG_INFO("user logout, user_id:{}, SessionId:{}", userId,
             pUserInfo->userSessionId);
    if (bKick) {
        LOG_INFO("user kick out, user_id:{}, SessionId:{}", userId,
                 pUserInfo->userSessionId);
        // \TODO:send kickout to old session
    }

    m_pNetModule->CloseInMainLoop(pUserInfo->userSessionId);
    m_mapSessionUserInfo.erase(pUserInfo->userSessionId);
    m_mapUserInfo.erase(userId);
    return true;
}

ClientPbModule::GateUserInfoPtr ClientPbModule::GetGateUserInfoByUserId(
    USER_ID userId) {
    if (auto itMapUserInfo = m_mapUserInfo.find(userId);
        itMapUserInfo != m_mapUserInfo.end()) {
        return itMapUserInfo->second;
    }

    return nullptr;
}

ClientPbModule::GateUserInfoPtr ClientPbModule::GetGateUserInfoBySessionId(
    Session::session_id_t clientSessionId) {
    if (auto itMapSessionUserInfo = m_mapSessionUserInfo.find(clientSessionId);
        itMapSessionUserInfo != m_mapSessionUserInfo.end()) {
        return itMapSessionUserInfo->second;
    }

    return nullptr;
}

TONY_CAT_SPACE_END
