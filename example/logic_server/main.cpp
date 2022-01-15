#include "common/config/xml_config_module.h"
#include "common/loop.h"
#include "common/module_manager.h"
#include "common/service/rpc_module.h"
#include "common/service/service_government_module.h"
#include "log/log_module.h"
#include "net/net_module.h"
#include "net/net_pb_module.h"
#include "server_common/server_define.h"

#include <csignal>
#include <memory>

TONY_CAT_SPACE_BEGIN

class LogicServer {
public:
    LogicServer()
    {
        m_name = typeid(*this).name();
    };
    ~LogicServer() = default;

    static void SignalHandle(int sig)
    {
        m_moduleManager.Stop();
    }

    void RegisterSignal()
    {
        std::signal(SIGABRT, &LogicServer::SignalHandle);
        std::signal(SIGFPE, &LogicServer::SignalHandle);
        std::signal(SIGILL, &LogicServer::SignalHandle);
        std::signal(SIGINT, &LogicServer::SignalHandle);
        std::signal(SIGSEGV, &LogicServer::SignalHandle);
        std::signal(SIGTERM, &LogicServer::SignalHandle);
    }

    void Start(int32_t nServerIndex)
    {
        RegisterSignal();
        RegisterModule();
        InitModule(nServerIndex);
        Run();
        DestoryModule();
    }

private:
    void RegisterModule()
    {
        REGISTER_MODULE(&m_moduleManager, LogModule);
        REGISTER_MODULE(&m_moduleManager, XmlConfigModule);
        REGISTER_MODULE(&m_moduleManager, NetModule);
        REGISTER_MODULE(&m_moduleManager, NetPbModule);
        REGISTER_MODULE(&m_moduleManager, ServiceGovernmentModule);
        REGISTER_MODULE(&m_moduleManager, RpcModule);
    }

    void InitModule(int32_t nServerIndex)
    {
        auto pServiceGovernmentModule = FIND_MODULE(&m_moduleManager, ServiceGovernmentModule);
        pServiceGovernmentModule->SetServerType(ServerType::eTypeLogicServer);
        pServiceGovernmentModule->SetServerId(nServerIndex);
        pServiceGovernmentModule->SetServerName(GetServerType<ServerType::eTypeLogicServer>());

        m_moduleManager.Init();

        LOG_INFO("init server {}", m_name);
    }

    void Run()
    {
        m_moduleManager.Run();
    }

    void DestoryModule()
    {
        m_moduleManager.Stop();
    }

private:
    static ModuleManager m_moduleManager;
    std::string m_name;
};

ModuleManager LogicServer::m_moduleManager = ModuleManager();

TONY_CAT_SPACE_END

int main(int argc, char* argv[])
{
    int32_t nServerIndex = 1;
    if (argc > 1) {
        nServerIndex = std::atoi(argv[1]);
    }

    tony_cat::LogicServer serverApp;
    serverApp.Start(nServerIndex);
}
