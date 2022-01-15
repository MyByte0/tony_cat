#include "service_government_module.h"
#include "common/config/xml_config_module.h"
#include "common/module_manager.h"
#include "common/service/rpc_module.h"
#include "common/utility/crc.h"
#include "log/log_module.h"
#include "net/net_module.h"
#include "net/net_pb_module.h"

#include <ctime>

TONY_CAT_SPACE_BEGIN

ServiceGovernmentModule::ServiceGovernmentModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

ServiceGovernmentModule::~ServiceGovernmentModule() { }

void ServiceGovernmentModule::BeforeInit()
{
    m_pNetModule = FIND_MODULE(m_pModuleManager, NetModule);
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);
    m_pRpcModule = FIND_MODULE(m_pModuleManager, RpcModule);
    m_pXmlConfigModule = FIND_MODULE(m_pModuleManager, XmlConfigModule);

    m_pNetPbModule->RegisterHandle(this, &ServiceGovernmentModule::OnHandleSSHeartbeatReq);
    InitConfig();
}

void ServiceGovernmentModule::OnInit()
{
    InitListen();
    InitConnect();
}

void ServiceGovernmentModule::InitConfig()
{
    m_mapServerConnectList.clear();
    std::vector<int32_t> vecDestServers;
    auto pMap = m_pXmlConfigModule->MutableServerListConfigDataMap();
    if (nullptr == pMap) {
        return;
    }

    for (const auto& elemMap : *pMap) {
        auto& stServerListConfigData = elemMap.second;
        if (stServerListConfigData.nServerType == GetServerType() && stServerListConfigData.nServerIndex == GetServerId()) {
            AddressToIpPort(stServerListConfigData.strPublicIp, m_strPublicIp, m_nPublicPort);
            AddressToIpPort(stServerListConfigData.strServerIp, m_strServerIp, m_nServerPort);
            MutableServerInfo()->strServerIp = m_strServerIp;
            MutableServerInfo()->nPort = m_nServerPort;
            vecDestServers = stServerListConfigData.vecConnectList;
            m_pNetModule->SetNetThreadNum(stServerListConfigData.nNetThreadsNum);
            LOG_INFO("Server Public Ip{}, port:{}, Address Ip{}, port:{}, netThreadNum:{}.",
                m_strPublicIp, m_nPublicPort, m_strServerIp, m_nServerPort, stServerListConfigData.nNetThreadsNum);
            break;
        }
    }

    for (const auto& elemMap : *pMap) {
        auto& stServerListConfigData = elemMap.second;
        if (std::find(vecDestServers.begin(), vecDestServers.end(), stServerListConfigData.nServerType)
            == vecDestServers.end()) {
            continue;
        }

        if (stServerListConfigData.nServerType == GetServerType() && stServerListConfigData.nServerIndex == GetServerId()) {
            continue;
        }

        if (m_mapServerConnectList[stServerListConfigData.nServerType].count(stServerListConfigData.nServerIndex) > 0) {
            LOG_ERROR("on duplicate on :{}, index{}", stServerListConfigData.nServerType, stServerListConfigData.nServerIndex);
            continue;
        }

        ServerInstanceInfo stServerInstanceInfo;
        stServerInstanceInfo.nServerType = stServerListConfigData.nServerType;
        stServerInstanceInfo.nServerIndex = stServerListConfigData.nServerIndex;
        AddressToIpPort(stServerListConfigData.strServerIp, stServerInstanceInfo.strServerIp, stServerInstanceInfo.nPort);
        m_mapServerConnectList[stServerListConfigData.nServerType][stServerListConfigData.nServerIndex] = std::move(stServerInstanceInfo);
    }
}

void ServiceGovernmentModule::InitListen()
{
    m_pNetPbModule->Listen(GetServerInfo().strServerIp, GetServerInfo().nPort);
}

void ServiceGovernmentModule::InitConnect()
{
    for (auto& elemMapServerConnectList : m_mapServerConnectList) {
        auto& mapServerInstanceInfo = elemMapServerConnectList.second;
        for (auto& elemMapServerInstanceInfo : mapServerInstanceInfo) {
            auto& stServerInstanceInfo = elemMapServerInstanceInfo.second;
            stServerInstanceInfo.nSessionId = ConnectServerInstance(stServerInstanceInfo);
        }
    }
}

Session::session_id_t ServiceGovernmentModule::ConnectServerInstance(const ServerInstanceInfo& stServerInstanceInfo)
{
    LOG_INFO("server connect, SessionId:{}, dest server type:{}, index:{}, ip:{} {}", stServerInstanceInfo.nSessionId,
        stServerInstanceInfo.nServerType, stServerInstanceInfo.nServerIndex, stServerInstanceInfo.strServerIp, stServerInstanceInfo.nPort);
    return m_pNetPbModule->Connect(stServerInstanceInfo.strServerIp, (uint16_t)stServerInstanceInfo.nPort,
        std::bind(&ServiceGovernmentModule::OnConnectSucess, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&ServiceGovernmentModule::OnDisconnect, this, std::placeholders::_1, stServerInstanceInfo));
}

void ServiceGovernmentModule::OnConnectSucess(Session::session_id_t nSessionId, bool bSuccess)
{
    if (bSuccess) {
        LOG_INFO("server connect success, SessionId:{}, ", nSessionId);
    } else {
        LOG_WARN("server connect fail, SessionId:{}, ", nSessionId);
    }

    Pb::ServerHead head;
    Pb::SSHeartbeatReq heartbeat;
    heartbeat.set_req_time(time(nullptr));
    m_pRpcModule->RpcRequest(nSessionId, head, heartbeat, [](Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSHeartbeatRsp& heartbeat) {
        LOG_INFO("server heartbeat request");
    });
}

void ServiceGovernmentModule::OnDisconnect(Session::session_id_t nSessionId, ServerInstanceInfo stServerInstanceInfo)
{
    LOG_WARN("server disconnect, SessionId:{}, dest server type:{}, index:{}, ip:{} {}", stServerInstanceInfo.nSessionId,
        stServerInstanceInfo.nServerType, stServerInstanceInfo.nServerIndex, stServerInstanceInfo.strServerIp, stServerInstanceInfo.nPort);
    auto pServerInstanceInfo = GetServerInstanceInfo(stServerInstanceInfo.nServerType, stServerInstanceInfo.nServerIndex);
    if (nullptr == pServerInstanceInfo) {
        LOG_ERROR("not find server info, server type:{}, server index:{}", stServerInstanceInfo.nServerType, stServerInstanceInfo.nServerIndex);
        return;
    }

    if (pServerInstanceInfo->timerReconnect) {
        pServerInstanceInfo->timerReconnect->cancel();
    }
    pServerInstanceInfo->timerReconnect = m_pModuleManager->GetMainLoop().ExecAfter(kReconnectServerTimeMillSeconds, [this, stServerInstanceInfo]() {
        auto pServerInstanceInfo = GetServerInstanceInfo(stServerInstanceInfo.nServerType, stServerInstanceInfo.nServerIndex);
        pServerInstanceInfo->nSessionId = ConnectServerInstance(*pServerInstanceInfo);
    });
}

ServiceGovernmentModule::ServerInstanceInfo* ServiceGovernmentModule::GetServerInstanceInfo(int32_t nServerType, int32_t nServerId)
{
    auto itMapServerConnectList = m_mapServerConnectList.find(nServerType);
    if (itMapServerConnectList == m_mapServerConnectList.end()) {
        return 0;
    }

    auto& mapServerInfo = itMapServerConnectList->second;
    auto itMapServerInfo = mapServerInfo.find(nServerId);
    if (itMapServerInfo == mapServerInfo.end()) {
        return 0;
    }

    return &itMapServerInfo->second;
}

Session::session_id_t ServiceGovernmentModule::GetServerSessionId(int32_t nServerType, int32_t nServerId)
{
    auto pServerInstanceInfo = GetServerInstanceInfo(nServerType, nServerId);
    if (nullptr == pServerInstanceInfo) {
        return 0;
    }
    return pServerInstanceInfo->nSessionId;
}

void ServiceGovernmentModule::OnHandleSSHeartbeatReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSHeartbeatReq& heartbeat)
{
    Pb::SSHeartbeatRsp msgHeartbeatRsp;
    msgHeartbeatRsp.set_req_time(heartbeat.req_time());
    m_pNetPbModule->SendPacket(sessionId, head, msgHeartbeatRsp);
}

bool ServiceGovernmentModule::AddressToIpPort(const std::string& strAddress, std::string& strIp, int32_t& nPort)
{
    auto posSpace = strAddress.find(' ');
    if (posSpace == strAddress.npos) {
        return false;
    }

    strIp = std::string(strAddress.c_str(), posSpace);
    nPort = std::atoi(strAddress.c_str() + posSpace + 1);
    return true;
}

TONY_CAT_SPACE_END
