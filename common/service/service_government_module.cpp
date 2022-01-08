#include "service_government_module.h"
#include "common/config/xml_config_module.h"
#include "common/module_manager.h"
#include "common/utility/crc.h"
#include "log/log_module.h"
#include "net/net_module.h"
#include "net/net_pb_module.h"

SER_NAME_SPACE_BEGIN

ServiceGovernmentModule::ServiceGovernmentModule(ModuleManager* pModuleManager)
    :ModuleBase(pModuleManager) {

}

ServiceGovernmentModule::~ServiceGovernmentModule() {}

void ServiceGovernmentModule::BeforeInit() {
    m_pNetModule = FIND_MODULE(m_pModuleManager, NetModule);
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);
    m_pXmlConfigModule = FIND_MODULE(m_pModuleManager, XmlConfigModule);

    m_pNetPbModule->RegisterHandle(this, &ServiceGovernmentModule::OnHandleSSHeartbeatReq);

    InitConfig();
    InitListen();
    InitConnect();
}

void ServiceGovernmentModule::InitConfig() {
    m_mapServerConnectList.clear();
    std::vector<std::string> vecDestServers;
    auto pMap = m_pXmlConfigModule->MutableServerListConfigDataMap();
    if (nullptr != pMap) {
        for (const auto& elemMap :*pMap) {
            auto& stServerListConfigData = elemMap.second;
            if (stServerListConfigData.strServerName == GetServerName() && stServerListConfigData.nServerIndex == GetServerId()) {
                AdressToIpPort(stServerListConfigData.strServerIp, m_strServerIp, m_nServerPort);
                vecDestServers = stServerListConfigData.vecConnectList;
                LOG_INFO("Server Public Address Ip{}, port:{}", m_strServerIp, m_nServerPort);
                break;
            }
        }

        for (const auto& elemMap : *pMap) {
            auto& stServerListConfigData = elemMap.second;
            if (std::find(vecDestServers.begin(), vecDestServers.end(), stServerListConfigData.strServerName)
                == vecDestServers.end()) {
                continue;
            }

            if (stServerListConfigData.strServerName == GetServerName() && stServerListConfigData.nServerIndex == GetServerId()) {
                continue;
            }

            if (m_mapServerConnectList[stServerListConfigData.strServerName].count(stServerListConfigData.nServerIndex) > 0)
            {
                LOG_ERROR("on duplicate on :{}, index{}", stServerListConfigData.strServerName, stServerListConfigData.nServerIndex);
                continue;
            }

            ServerInstanceInfo stServerInstanceInfo;
            stServerInstanceInfo.strServerName = stServerListConfigData.strServerName;
            stServerInstanceInfo.nId = stServerListConfigData.nServerIndex;
            AdressToIpPort(stServerListConfigData.strServerIp, stServerInstanceInfo.strServerIp, stServerInstanceInfo.nPort);
            m_mapServerConnectList[stServerListConfigData.strServerName][stServerListConfigData.nServerIndex] = std::move(stServerInstanceInfo);
        }
    }
}

void ServiceGovernmentModule::InitListen() {
    m_pNetModule->Listen(GetServerInfo().strServerIp, GetServerInfo().nPort);
}

void ServiceGovernmentModule::InitConnect() {
    for (auto& elemMapServerConnectList : m_mapServerConnectList) {
        auto& mapServerInstanceInfo = elemMapServerConnectList.second;
        for (auto& elemMapServerInstanceInfo : mapServerInstanceInfo) {
            auto& stServerInstanceInfo = elemMapServerInstanceInfo.second;
            stServerInstanceInfo.nSessionId = m_pNetModule->Connect(stServerInstanceInfo.strServerIp, (uint16_t)stServerInstanceInfo.nPort);
        }
    }
}

Session::session_id_t ServiceGovernmentModule::GetServerSessionId(const std::string& strServerName, int32_t nServerId) {
    auto itMapServerConnectList = m_mapServerConnectList.find(strServerName);
    if (itMapServerConnectList == m_mapServerConnectList.end()) {
        return 0;
    }
    
    auto& mapServerInfo = itMapServerConnectList->second;
    auto itMapServerInfo = mapServerInfo.find(nServerId);
    if (itMapServerInfo == mapServerInfo.end()) {
        return 0;
    }

    return itMapServerInfo->second.nSessionId;
}

void ServiceGovernmentModule::OnHandleSSHeartbeatReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSHeartbeatReq& heartbeat) {
    Pb::SSHeartbeatRsp msgHeartbeatRsp;
    msgHeartbeatRsp.set_req_time(heartbeat.req_time());
    m_pNetPbModule->SendPacket(sessionId, head, msgHeartbeatRsp);
}

bool ServiceGovernmentModule::AdressToIpPort(const std::string& strAddress, std::string& strIp, int32_t& nPort) {
    auto posSpace = strAddress.find(' ');
    if (posSpace == strAddress.npos) {
        return false;
    }

    strIp = std::string(strAddress.c_str(), posSpace);
    nPort = std::atoi(strAddress.c_str() + posSpace + 1);
    return true;
}

SER_NAME_SPACE_END
