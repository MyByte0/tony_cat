#include "client_manager_module.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/service/rpc_module.h"
#include "common/service/service_government_module.h"
#include "protocol/client_base.pb.h"
#include "protocol/server_base.pb.h"
#include "protocol/server_db.pb.h"

#include "server_common/server_define.h"

#include <cassert>

TONY_CAT_SPACE_BEGIN

ClientManagerModule::ClientManagerModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

ClientManagerModule::~ClientManagerModule() { }

void ClientManagerModule::BeforeInit()
{
    m_pRpcModule = FIND_MODULE(m_pModuleManager, RpcModule);
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);
    m_pServiceGovernmentModule = FIND_MODULE(m_pModuleManager, ServiceGovernmentModule);
}

void ClientManagerModule::OnInit()
{
    StartTestClient();
}

ClientManagerModule::ClientDataPtr ClientManagerModule::GetPlayerData(const ACCOUNT_ID& user_id)
{
    auto itMapOnlinePlayer = m_mapClient.find(user_id);
    if (itMapOnlinePlayer != m_mapClient.end()) {
        return itMapOnlinePlayer->second;
    }

    return nullptr;
}

CoroutineTask<void> ClientManagerModule::StartTestClient()
{
    // waiting for server start
    uint32_t nWaitMillSecond = 20000;
    co_await AwaitableExecAfter(nWaitMillSecond);

    nWaitMillSecond = 1000;
    for (int i = 0; i < 10; ++i) {
        LOG_TRACE("start login client");
        auto nResult = co_await CreateNewClient();
        if (nResult != 0) {
            LOG_ERROR("connect to server error:{}", nResult);
        } else {
            LOG_TRACE("start login success");
        }
    }
}

CoroutineAsyncTask<int32_t> ClientManagerModule::CreateNewClient()
{
    co_await AwaitableExecAfter(1000);
    // connect to gate server
    auto pServerInstanceInfo = m_pServiceGovernmentModule->GetServerInstanceInfo(ServerType::eTypeGateServer, 1);
    if (nullptr == pServerInstanceInfo) {
        LOG_ERROR("not find gate info");
        co_return 1;
    }
    LOG_TRACE("client login start");
    auto nSessionId = co_await NetPbModule::AwaitableConnect(pServerInstanceInfo->strPublicIp, pServerInstanceInfo->nPublicPort);
    if (0 == nSessionId) {
        LOG_ERROR("connect gate error, addr:{}, port:{}", pServerInstanceInfo->strPublicIp, pServerInstanceInfo->nPublicPort);
        co_return 1;
    }

    static int32_t s_nCLientNum = 0;

    ACCOUNT_ID strAccount = std::string("test").append(std::to_string(++s_nCLientNum));
    auto pClientData = std::make_shared<ClientData>();
    pClientData->strAccount = strAccount;
    pClientData->nSessionId = nSessionId;
    m_mapClient.emplace(strAccount, pClientData);

    // msg request
    Pb::ClientHead head;
    Pb::CSPlayerLoginReq req;
    req.set_user_name(strAccount);
    // msg respond
    Session::session_id_t nSessionIdRsp;
    Pb::ClientHead headRsp;
    Pb::CSPlayerLoginRsp msgRsp;
    co_await RpcModule::AwaitableRpcRequest(nSessionId, head, req, nSessionIdRsp, headRsp, msgRsp);
    if (headRsp.error_code() != 0) {
        LOG_ERROR("login failed, client:{}", strAccount);
        co_return headRsp.error_code();
    }

    LOG_TRACE("client login success on:{}", strAccount);
    co_return 0;
}

TONY_CAT_SPACE_END
