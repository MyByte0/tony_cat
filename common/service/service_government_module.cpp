#include "service_government_module.h"

#include <ctime>
#include <utility>
#include <vector>

#include "common/config/xml_config_module.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/service/rpc_module.h"
#include "common/utility/crc.h"

TONY_CAT_SPACE_BEGIN

ServiceGovernmentModule::ServiceGovernmentModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager) {}

ServiceGovernmentModule::~ServiceGovernmentModule() {}

void ServiceGovernmentModule::BeforeInit() {
    m_pNetModule = FIND_MODULE(m_pModuleManager, NetModule);
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);
    m_pRpcModule = FIND_MODULE(m_pModuleManager, RpcModule);
    m_pXmlConfigModule = FIND_MODULE(m_pModuleManager, XmlConfigModule);

    m_pNetPbModule->RegisterHandle(
        this, &ServiceGovernmentModule::OnHandleSSHeartbeatReq);
    InitConfig();
}

void ServiceGovernmentModule::OnInit() {
    InitListen();
    InitConnect();
}

void ServiceGovernmentModule::InitConfig() {
    m_mapConnectServerList.clear();
    m_mapServerList.clear();
    std::vector<int32_t> vecDestServers;
    auto pMap = m_pXmlConfigModule->MutableServerListConfigDataMap();
    if (nullptr == pMap) {
        return;
    }

    for (const auto& elemMap : *pMap) {
        auto& stServerListConfigData = elemMap.second;
        // self server info
        if (stServerListConfigData.nServerType == GetServerType() &&
            stServerListConfigData.nServerIndex == GetServerId()) {
            AddressToIpPort(stServerListConfigData.strPublicIp, m_strPublicIp,
                            m_nPublicPort);
            AddressToIpPort(stServerListConfigData.strServerIp, m_strServerIp,
                            m_nServerPort);
            MutableMineServerInfo()->nServerConfigId =
                stServerListConfigData.nId;
            MutableMineServerInfo()->strServerIp = m_strServerIp;
            MutableMineServerInfo()->nPort = m_nServerPort;
            vecDestServers = stServerListConfigData.vecConnectList;
            LOG_INFO(
                "Server Public Ip{}, port:{}, Address Ip{}, port:{}, "
                "netThreadNum:{}.",
                m_strPublicIp, m_nPublicPort, m_strServerIp, m_nServerPort,
                stServerListConfigData.nNetThreadsNum);
            break;
        }
    }

    for (const auto& elemMap : *pMap) {
        auto& stServerListConfigData = elemMap.second;

        ServerInstanceInfo stServerInstanceInfo;
        stServerInstanceInfo.nServerConfigId = stServerListConfigData.nId;
        stServerInstanceInfo.nServerType = stServerListConfigData.nServerType;
        stServerInstanceInfo.nServerIndex = stServerListConfigData.nServerIndex;
        AddressToIpPort(stServerListConfigData.strServerIp,
                        stServerInstanceInfo.strServerIp,
                        stServerInstanceInfo.nPort);
        AddressToIpPort(stServerListConfigData.strPublicIp,
                        stServerInstanceInfo.strPublicIp,
                        stServerInstanceInfo.nPublicPort);

        m_mapServerList[stServerListConfigData.nServerType]
                       [stServerListConfigData.nServerIndex] =
                           stServerInstanceInfo;

        if (std::find(vecDestServers.begin(), vecDestServers.end(),
                      stServerListConfigData.nServerType) ==
            vecDestServers.end()) {
            continue;
        }

        if (stServerListConfigData.nServerType == GetServerType() &&
            stServerListConfigData.nServerIndex == GetServerId()) {
            continue;
        }

        if (m_mapConnectServerList[stServerListConfigData.nServerType].count(
                stServerListConfigData.nServerIndex) > 0) {
            LOG_ERROR("on duplicate on :{}, index{}",
                      stServerListConfigData.nServerType,
                      stServerListConfigData.nServerIndex);
            continue;
        }

        m_mapConnectServerList[stServerListConfigData.nServerType]
                              [stServerListConfigData.nServerIndex] =
                                  std::move(stServerInstanceInfo);
    }
}

void ServiceGovernmentModule::InitListen() {
    if (GetMineServerInfo().strServerIp.empty() ||
        GetMineServerInfo().nPort == 0) {
        LOG_INFO("Get App Address Empty");
        return;
    }
    m_pNetPbModule->Listen(GetMineServerInfo().strServerIp,
                           GetMineServerInfo().nPort);
}

void ServiceGovernmentModule::InitConnect() {
    for (const auto& elemMapServerConnectList : m_mapConnectServerList) {
        const auto& mapServerInstanceInfo = elemMapServerConnectList.second;
        for (const auto& elemMapServerInstanceInfo : mapServerInstanceInfo) {
            const auto& stServerInstanceInfo = elemMapServerInstanceInfo.second;
            ConnectServerInstance(stServerInstanceInfo);
        }
    }
}

void ServiceGovernmentModule::ConnectServerInstance(
    const ServerInstanceInfo& stServerInstanceInfo) {
    m_pNetPbModule->Connect(
        stServerInstanceInfo.strServerIp, (uint16_t)stServerInstanceInfo.nPort,
        std::bind(&ServiceGovernmentModule::OnConnectSucess, this,
                  std::placeholders::_1, stServerInstanceInfo,
                  std::placeholders::_2),
        std::bind(&ServiceGovernmentModule::OnDisconnect, this,
                  std::placeholders::_1, stServerInstanceInfo));
    LOG_INFO("server connect, dest server type:{}, index:{}, ip:{} {}",
             stServerInstanceInfo.nServerType,
             stServerInstanceInfo.nServerIndex,
             stServerInstanceInfo.strServerIp, stServerInstanceInfo.nPort);
    return;
}

void ServiceGovernmentModule::OnConnectSucess(
    Session::session_id_t nSessionId,
    const ServerInstanceInfo& stServerInstanceInfo, bool bSuccess) {
    if (bSuccess) {
        LOG_INFO(
            "server connect success, SessionId:{}, ServerType:{}, "
            "ServerIndex:{}",
            nSessionId, stServerInstanceInfo.nServerType,
            stServerInstanceInfo.nServerIndex);
    } else {
        LOG_WARN(
            "server connect fail, SessionId:{}, ServerType:{}, "
            "ServerIndex:{}",
            nSessionId, stServerInstanceInfo.nServerType,
            stServerInstanceInfo.nServerIndex);
        OnDisconnect(nSessionId, stServerInstanceInfo);
        return;
    }

    auto pServerInstanceInfo = GetServerInstanceInfo(
        stServerInstanceInfo.nServerType, stServerInstanceInfo.nServerIndex);
    if (nullptr == pServerInstanceInfo) {
        LOG_ERROR("not find server info, server type:{}, server index:{}",
                  stServerInstanceInfo.nServerType,
                  stServerInstanceInfo.nServerIndex);
        return;
    }
    pServerInstanceInfo->nSessionId = nSessionId;
    OnHeartbeat(*pServerInstanceInfo);
}

void ServiceGovernmentModule::OnDisconnect(
    Session::session_id_t nSessionId,
    const ServerInstanceInfo& stServerInstanceInfo) {
    LOG_WARN(
        "server disconnect, SessionId:{}, dest server type:{}, index:{}, ip:{} "
        "{}",
        stServerInstanceInfo.nSessionId, stServerInstanceInfo.nServerType,
        stServerInstanceInfo.nServerIndex, stServerInstanceInfo.strServerIp,
        stServerInstanceInfo.nPort);
    auto pServerInstanceInfo = GetServerInstanceInfo(
        stServerInstanceInfo.nServerType, stServerInstanceInfo.nServerIndex);
    if (nullptr == pServerInstanceInfo) {
        LOG_ERROR("not find server info, server type:{}, server index:{}",
                  stServerInstanceInfo.nServerType,
                  stServerInstanceInfo.nServerIndex);
        return;
    }

    if (0 != pServerInstanceInfo->nSessionId &&
        pServerInstanceInfo->nSessionId == nSessionId) {
        pServerInstanceInfo->nSessionId = 0;
    } else {
        LOG_ERROR("server disconnect on sessionId:{}",
                  pServerInstanceInfo->nSessionId);
    }

    // when net session disconnect, restart connect work
    if (nullptr != pServerInstanceInfo->timerHeartbeat) {
        pServerInstanceInfo->timerHeartbeat->cancel();
        pServerInstanceInfo->timerHeartbeat = nullptr;
    }
    if (nullptr != pServerInstanceInfo->timerReconnect) {
        pServerInstanceInfo->timerReconnect->cancel();
        pServerInstanceInfo->timerReconnect = nullptr;
    }
    pServerInstanceInfo->timerReconnect =
        m_pModuleManager->GetMainLoop().ExecAfter(
            kReconnectServerTimeMillSeconds, [this, stServerInstanceInfo]() {
                auto pServerInstanceInfo =
                    GetServerInstanceInfo(stServerInstanceInfo.nServerType,
                                          stServerInstanceInfo.nServerIndex);
                ConnectServerInstance(*pServerInstanceInfo);
            });
}

CoroutineTask<void> ServiceGovernmentModule::OnHeartbeat(
    ServerInstanceInfo stServerInstanceInfo) {
    auto nSessionId = stServerInstanceInfo.nSessionId;
    auto head = NetMemoryPool::PacketCreate<Pb::ServerHead>();
    auto pHeartbeat = NetMemoryPool::PacketCreate<Pb::SSHeartbeatReq>();
    pHeartbeat->set_req_time(time(nullptr));
    // m_pRpcModule->RpcRequest(
    //     nSessionId, head, pHeartbeat,
    //     [](Session::session_id_t sessionId,
    //        NetMemoryPool::PacketNode<Pb::ServerHead> head,
    //        NetMemoryPool::PacketNode<Pb::SSHeartbeatRsp> heartbeat) {
    //         if (head->error_code() != Pb::SSMessageCode::ss_msg_success) {
    //             LOG_ERROR("server heartbeat request timeout");
    //             return;
    //         }
    //         LOG_TRACE("server heartbeat request");
    //     });

    auto [rspSessionId, headRsp, msgRsp] =
        co_await RpcModule::AwaitableRpcFetch(Pb::SSHeartbeatRsp, nSessionId,
                                              head, pHeartbeat);
    auto pServerInstanceInfo = GetServerInstanceInfo(
        stServerInstanceInfo.nServerType, stServerInstanceInfo.nServerIndex);
    if (nullptr == pServerInstanceInfo) {
        LOG_ERROR("not find server info, server type:{}, server index:{}",
                  stServerInstanceInfo.nServerType,
                  stServerInstanceInfo.nServerIndex);
        co_return;
    }

    if (nullptr != pServerInstanceInfo->timerHeartbeat) {
        pServerInstanceInfo->timerHeartbeat->cancel();
        pServerInstanceInfo->timerHeartbeat = nullptr;
    }
    pServerInstanceInfo->timerHeartbeat =
        m_pModuleManager->GetMainLoop().ExecAfter(
            kHeartbeatServerTimeMillSeconds, [this, stServerInstanceInfo]() {
                if (auto pServerInstanceInfo = GetServerInstanceInfo(
                        stServerInstanceInfo.nServerType,
                        stServerInstanceInfo.nServerIndex);
                    nullptr != pServerInstanceInfo) {
                    OnHeartbeat(*pServerInstanceInfo);
                }
            });

    if (headRsp->error_code() != Pb::SSMessageCode::ss_msg_success) {
        LOG_ERROR("server heartbeat request timeout");
        m_pNetModule->CloseInMainLoop(nSessionId);
        OnDisconnect(nSessionId, stServerInstanceInfo);
        co_return;
    }

    LOG_TRACE("server heartbeat send success ,serverType:{}, serverIndex:{}",
              stServerInstanceInfo.nServerType,
              stServerInstanceInfo.nServerIndex);
    co_return;
}

int32_t ServiceGovernmentModule::GetServerKeyIndex(
    int32_t nServerType, const ::std::string& strKey) {
    auto itMapServerConnectList = m_mapServerList.find(nServerType);
    if (itMapServerConnectList == m_mapServerList.end()) {
        return 0;
    }

    // \TODO fix: ServerIndex calculate
    auto& mapServerInfo = itMapServerConnectList->second;
    return (std::size_t(CRC32(strKey)) % mapServerInfo.size()) + 1;
}

ServiceGovernmentModule::ServerInstanceInfo*
ServiceGovernmentModule::GetServerInstanceInfo(int32_t nServerType,
                                               int32_t nServerId) {
    auto itMapServerList = m_mapServerList.find(nServerType);
    if (itMapServerList == m_mapServerList.end()) {
        return nullptr;
    }

    auto& mapServerInfo = itMapServerList->second;
    auto itMapServerInfo = mapServerInfo.find(nServerId);
    if (itMapServerInfo == mapServerInfo.end()) {
        return nullptr;
    }

    return &itMapServerInfo->second;
}

Session::session_id_t ServiceGovernmentModule::GetServerSessionId(
    int32_t nServerType, int32_t nServerId) {
    auto pServerInstanceInfo = GetServerInstanceInfo(nServerType, nServerId);
    if (nullptr == pServerInstanceInfo) {
        return 0;
    }
    return pServerInstanceInfo->nSessionId;
}

Session::session_id_t ServiceGovernmentModule::GetServerSessionIdByKey(
    int32_t nServerType, const std::string& strKey) {
    return GetServerSessionId(nServerType,
                              GetServerKeyIndex(nServerType, strKey));
}

void ServiceGovernmentModule::OnHandleSSHeartbeatReq(
    Session::session_id_t sessionId,
    NetMemoryPool::PacketNode<Pb::ServerHead> head,
    NetMemoryPool::PacketNode<Pb::SSHeartbeatReq> heartbeat) {
    auto pSrcServerInstanceInfo = GetServerInstanceInfo(
        head->src_server_type(), head->src_server_index());
    if (nullptr == pSrcServerInstanceInfo) {
        head->set_error_code(1);

    } else {
        auto pConnectServerInstanceInfo = GetServerInstanceInfo(
            head->src_server_type(), head->src_server_index());
        if (nullptr == pConnectServerInstanceInfo) {
            LOG_INFO(
                "on server connect, serverType:{}, serverIndex:{}, "
                "sessionId:{}",
                head->src_server_type(), head->src_server_index(), sessionId);
            m_mapServerList[head->src_server_type()].emplace(
                head->src_server_index(), *pSrcServerInstanceInfo);
            pConnectServerInstanceInfo =
                &(m_mapServerList[head->src_server_type()]
                                 [head->src_server_index()]);
        }

        if (pConnectServerInstanceInfo->nSessionId != sessionId) {
            LOG_INFO(
                "on server session update, serverType:{}, serverIndex:{}, "
                "oldSessionId:{}, newSessionId:{}",
                head->src_server_type(), head->src_server_index(),
                pConnectServerInstanceInfo->nSessionId, sessionId);
            pConnectServerInstanceInfo->nSessionId = sessionId;
        }
    }

    LOG_TRACE("receive heartbeat from server:{}, index:{} sessionId:{}",
              head->src_server_type(), head->src_server_index(), sessionId);
    auto pMsgHeartbeatRsp = NetMemoryPool::PacketCreate<Pb::SSHeartbeatRsp>();
    pMsgHeartbeatRsp->set_req_time(heartbeat->req_time());
    m_pNetPbModule->SendPacket(sessionId, head, pMsgHeartbeatRsp);
}

bool ServiceGovernmentModule::AddressToIpPort(const std::string& strAddress,
                                              std::string& strIp,
                                              int32_t& nPort) {
    auto posSpace = strAddress.find(' ');
    if (posSpace == strAddress.npos) {
        return false;
    }

    strIp = std::string(strAddress.c_str(), posSpace);
    nPort = std::atoi(strAddress.c_str() + posSpace + 1);
    return true;
}

TONY_CAT_SPACE_END
