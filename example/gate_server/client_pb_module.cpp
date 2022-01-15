#include "client_pb_module.h"
#include "common/module_manager.h"
#include "common/service/service_government_module.h"
#include "log/log_module.h"
#include "net/net_module.h"
#include "net/net_pb_module.h"
#include "protocol/client_base.pb.h"
#include "protocol/server_base.pb.h"
#include "server_common/server_define.h"

TONY_CAT_SPACE_BEGIN

ClientPbModule::ClientPbModule(ModuleManager* pModuleManager)
    : NetPbModule(pModuleManager)
{
}

ClientPbModule::~ClientPbModule() { }

void ClientPbModule::BeforeInit()
{
    this->NetPbModule::BeforeInit();
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);
    m_pServiceGovernmentModule = FIND_MODULE(m_pModuleManager, ServiceGovernmentModule);
}

void ClientPbModule::OnInit()
{
    this->NetPbModule::OnInit();
    Listen(m_pServiceGovernmentModule->GetPublicIp(), m_pServiceGovernmentModule->GetPublicPort());

    SetDefaultPacketHandle(std::bind(&ClientPbModule::OnClientMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    m_pNetPbModule->SetDefaultPacketHandle(std::bind(&ClientPbModule::OnServerMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void ClientPbModule::OnClientMessage(Session::session_id_t clientSessionId, uint32_t msgType, const char* data, std::size_t length)
{
    if (msgType >= Pb::SSMessageId::ss_msgid_begin) {
        LOG_WARN("client req invalid msg_id, session_id:{} msg_id:{}", clientSessionId, msgType);
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
    Pb::ServerHead serverHead;
    // serverHead.set_user_id();
    SendPacket(m_pServiceGovernmentModule->GetServerSessionId(ServerType::eTypeLogicServer, 1), msgType, serverHead, pData, pbPacketBodyLen);
}

void ClientPbModule::OnServerMessage(Session::session_id_t serverSessionId, uint32_t msgType, const char* data, std::size_t length)
{
    const char* pData = nullptr;
    std::size_t pbPacketBodyLen = 0;
    Pb::ServerHead serverHead;
    //auto userId = serverHead.user_id();
    PaserPbHead(data, length, serverHead, pData, pbPacketBodyLen);

    Pb::ClientHead clientHead;
    USER_ID userId;
    auto pUserInfo = GetGateUserInfoByUserId(userId);
    if (nullptr == pUserInfo) {
        LOG_ERROR("not find user info, userId:{}", userId);
        return;
    }
    SendPacket(pUserInfo->userSessionId, msgType, serverHead, pData, pbPacketBodyLen);
}

void ClientPbModule::OnUserLogin(Session::session_id_t clientSessionId)
{
}

ClientPbModule::GateUserInfoPtr ClientPbModule::GetGateUserInfoByUserId(USER_ID userId)
{
    auto itMapUserInfo = m_mapUserInfo.find(userId);
    if (itMapUserInfo != m_mapUserInfo.end()) {
        return itMapUserInfo->second;
    }

    return nullptr;
}

ClientPbModule::GateUserInfoPtr ClientPbModule::GetGateUserInfoBySessionId(Session::session_id_t clientSessionId)
{
    auto itMapSessionUserInfo = m_mapSessionUserInfo.find(clientSessionId);
    if (itMapSessionUserInfo != m_mapSessionUserInfo.end()) {
        return itMapSessionUserInfo->second;
    }

    return nullptr;
}

TONY_CAT_SPACE_END
