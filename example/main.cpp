#include "common/loop.h"

#include "common/module_manager.h"
#include "log/log_module.h"
#include "net/net_module.h"
#include "net/net_pb_module.h"

#include <csignal>
#include <memory>


SER_NAME_SPACE_BEGIN

class TestServer {
public:
    TestServer() {
        m_name = typeid(*this).name();
    };
    ~TestServer() = default;

    static void SignalHandle(int sig) {
        m_moduleManager.Stop();
    }

    void RegisterSignal() {
        std::signal(SIGABRT, &TestServer::SignalHandle);
        std::signal(SIGFPE, &TestServer::SignalHandle);
        std::signal(SIGILL, &TestServer::SignalHandle);
        std::signal(SIGINT, &TestServer::SignalHandle);
        std::signal(SIGSEGV, &TestServer::SignalHandle);
        std::signal(SIGTERM, &TestServer::SignalHandle);
    }

    void Start() {
        RegisterSignal();
        RegisterModule();
        InitModule();
        Run();
        DestoryModule();
    }

private:
    void RegisterModule()
    {
        REGISTER_MODULE(&m_moduleManager, LogModule);
        REGISTER_MODULE(&m_moduleManager, NetModule);
        REGISTER_MODULE(&m_moduleManager, NetPbModule);
    }

    void InitModule()
    {
        m_moduleManager.Init();

        LOG_INFO("init server {}", m_name);
    }

    void Run() {
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

ModuleManager TestServer::m_moduleManager = ModuleManager();

SER_NAME_SPACE_END



int main() {
  demo::TestServer serverApp;
  serverApp.Start();
}
