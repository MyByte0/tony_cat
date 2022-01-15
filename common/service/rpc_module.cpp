#include "rpc_module.h"
#include "common/module_manager.h"
#include "log/log_module.h"
#include "net/net_module.h"
#include "net/net_pb_module.h"

TONY_CAT_SPACE_BEGIN

RpcModule::RpcModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

RpcModule::~RpcModule() { }

void RpcModule::BeforeInit()
{
    m_pNetModule = FIND_MODULE(m_pModuleManager, NetModule);
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);
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

TONY_CAT_SPACE_END
