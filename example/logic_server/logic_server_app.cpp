#include "logic_server_app.h"

#include "common/config/xml_config_module.h"
#include "common/log/log_module.h"
#include "common/loop.h"
#include "common/module_manager.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/service/rpc_module.h"
#include "common/service/service_government_module.h"
#include "player_manager_module.h"
#include "server_common/server_define.h"

#include <csignal>

TONY_CAT_SPACE_BEGIN

ModuleManager LogicServerApp::m_moduleManager = ModuleManager();

LogicServerApp::LogicServerApp()
{
    m_name = typeid(*this).name();
};

void LogicServerApp::SignalHandle(int sig)
{
    m_moduleManager.Stop();
}

void LogicServerApp::RegisterSignal()
{
    std::signal(SIGABRT, &LogicServerApp::SignalHandle);
    std::signal(SIGFPE, &LogicServerApp::SignalHandle);
    std::signal(SIGILL, &LogicServerApp::SignalHandle);
    std::signal(SIGINT, &LogicServerApp::SignalHandle);
    std::signal(SIGSEGV, &LogicServerApp::SignalHandle);
    std::signal(SIGTERM, &LogicServerApp::SignalHandle);
}

void LogicServerApp::Start(int32_t nServerIndex)
{
    RegisterSignal();
    RegisterModule();
    InitModule(nServerIndex);
    Run();
    DestoryModule();
}

void LogicServerApp::RegisterModule()
{
    REGISTER_MODULE(&m_moduleManager, LogModule);
    REGISTER_MODULE(&m_moduleManager, XmlConfigModule);
    REGISTER_MODULE(&m_moduleManager, NetModule);
    REGISTER_MODULE(&m_moduleManager, NetPbModule);
    REGISTER_MODULE(&m_moduleManager, ServiceGovernmentModule);
    REGISTER_MODULE(&m_moduleManager, RpcModule);

    REGISTER_MODULE(&m_moduleManager, PlayerManagerModule);
}

void LogicServerApp::InitModule(int32_t nServerIndex)
{
    auto pServiceGovernmentModule = FIND_MODULE(&m_moduleManager, ServiceGovernmentModule);
    pServiceGovernmentModule->SetServerType(ServerType::eTypeLogicServer);
    pServiceGovernmentModule->SetServerId(nServerIndex);
    pServiceGovernmentModule->SetServerName(GetServerType<ServerType::eTypeLogicServer>());

    m_moduleManager.Init();

    LOG_INFO("init server {}", m_name);
}

void LogicServerApp::Run()
{
    m_moduleManager.Run();
}

void LogicServerApp::DestoryModule()
{
    m_moduleManager.Stop();
}

TONY_CAT_SPACE_END
