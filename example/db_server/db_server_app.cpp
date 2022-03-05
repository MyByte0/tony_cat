#include "db_server_app.h"

#include "common/config/xml_config_module.h"
#include "common/log/log_module.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/service/rpc_module.h"
#include "common/service/service_government_module.h"
#include "db_exec_module.h"

#include <csignal>

TONY_CAT_SPACE_BEGIN

ModuleManager DBServerApp::m_moduleManager = ModuleManager();

DBServerApp::DBServerApp()
{
    m_name = typeid(*this).name();
};

void DBServerApp::SignalHandle(int sig)
{
    m_moduleManager.Stop();
}

void DBServerApp::RegisterSignal()
{
    std::signal(SIGABRT, &DBServerApp::SignalHandle);
    std::signal(SIGFPE, &DBServerApp::SignalHandle);
    std::signal(SIGILL, &DBServerApp::SignalHandle);
    std::signal(SIGINT, &DBServerApp::SignalHandle);
    std::signal(SIGSEGV, &DBServerApp::SignalHandle);
    std::signal(SIGTERM, &DBServerApp::SignalHandle);
}

void DBServerApp::Start(int32_t nServerIndex)
{
    RegisterSignal();
    RegisterModule();
    InitModule(nServerIndex);
    Run();
    DestoryModule();
}

void DBServerApp::RegisterModule()
{
    REGISTER_MODULE(&m_moduleManager, LogModule);
    REGISTER_MODULE(&m_moduleManager, XmlConfigModule);
    REGISTER_MODULE(&m_moduleManager, NetModule);
    REGISTER_MODULE(&m_moduleManager, NetPbModule);
    REGISTER_MODULE(&m_moduleManager, ServiceGovernmentModule);
    REGISTER_MODULE(&m_moduleManager, RpcModule);
    REGISTER_MODULE(&m_moduleManager, DBExecModule);
}

void DBServerApp::InitModule(int32_t nServerIndex)
{
    auto pServiceGovernmentModule = FIND_MODULE(&m_moduleManager, ServiceGovernmentModule);
    pServiceGovernmentModule->SetServerType(ServerType::eTypeDBServer);
    pServiceGovernmentModule->SetServerId(nServerIndex);
    pServiceGovernmentModule->SetServerName(GetServerType<ServerType::eTypeDBServer>());

    m_moduleManager.Init();

    LOG_INFO("init server {}", m_name);
}

void DBServerApp::Run()
{
    m_moduleManager.Run();
}

void DBServerApp::DestoryModule()
{
    m_moduleManager.Stop();
}

TONY_CAT_SPACE_END
