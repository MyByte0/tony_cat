#include "client_test_app.h"

#include <csignal>

#include "client_manager_module.h"
#include "common/config/xml_config_module.h"
#include "common/log/log_module.h"
#include "common/loop/loop.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/service/rpc_module.h"
#include "common/service/service_government_module.h"
#include "app/game/server_define.h"

TONY_CAT_SPACE_BEGIN

ModuleManager ClientTestApp::m_moduleManager = ModuleManager();

ClientTestApp::ClientTestApp() { m_name = typeid(*this).name(); }

void ClientTestApp::SignalHandle(int sig) { m_moduleManager.Stop(); }

void ClientTestApp::RegisterSignal() {
    std::signal(SIGABRT, &ClientTestApp::SignalHandle);
    std::signal(SIGFPE, &ClientTestApp::SignalHandle);
    std::signal(SIGILL, &ClientTestApp::SignalHandle);
    std::signal(SIGINT, &ClientTestApp::SignalHandle);
    std::signal(SIGSEGV, &ClientTestApp::SignalHandle);
    std::signal(SIGTERM, &ClientTestApp::SignalHandle);
}

void ClientTestApp::Start(int32_t nServerIndex) {
    RegisterSignal();
    RegisterModule();
    InitModule(nServerIndex);
    Run();
    DestoryModule();
}

void ClientTestApp::RegisterModule() {
    REGISTER_MODULE(&m_moduleManager, LogModule);
    REGISTER_MODULE(&m_moduleManager, XmlConfigModule);
    REGISTER_MODULE(&m_moduleManager, NetModule);
    REGISTER_MODULE(&m_moduleManager, NetPbModule);
    REGISTER_MODULE(&m_moduleManager, ServiceGovernmentModule);
    REGISTER_MODULE(&m_moduleManager, RpcModule);

    REGISTER_MODULE(&m_moduleManager, ClientManagerModule);
}

void ClientTestApp::InitModule(int32_t nServerIndex) {
    auto pLogModule =
        FIND_MODULE(&m_moduleManager, LogModule);
    pLogModule->SetLogName(
        std::string("TestClient").append(std::to_string(nServerIndex)));

    m_moduleManager.Init();

    LOG_INFO("init server {}", m_name);
}

void ClientTestApp::Run() { m_moduleManager.Run(); }

void ClientTestApp::DestoryModule() { m_moduleManager.Stop(); }

TONY_CAT_SPACE_END
