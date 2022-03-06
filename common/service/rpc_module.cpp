#include "rpc_module.h"

#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"

TONY_CAT_SPACE_BEGIN

RpcModule* RpcModule::m_pRpcModule = nullptr;

RpcModule* RpcModule::GetInstance()
{
    return RpcModule::m_pRpcModule;
}

RpcModule::RpcModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

RpcModule::~RpcModule() { }

void RpcModule::BeforeInit()
{
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);

    RpcModule::m_pRpcModule = this;
}

void RpcModule::AfterStop()
{
    for (auto& elemMapRpcContext : m_mapRpcContextDeleter) {
        auto msgId = elemMapRpcContext.first;
        elemMapRpcContext.second(m_mapRpcContext[msgId]);
    }

    m_mapRpcContextDeleter.clear();
    m_mapRpcContext.clear();
}

int64_t RpcModule::GenQueryId()
{
    // \TODO: if server restart, cause duplicate QueryId
    return ++m_nQueryId;
}

TONY_CAT_SPACE_END
