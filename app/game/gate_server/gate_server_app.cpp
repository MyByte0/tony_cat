#include "gate_server_app.h"

#include <csignal>

#include "client_pb_module.h"
#include "common/config/xml_config_module.h"
#include "common/log/log_module.h"
#include "common/loop/loop.h"
#include "common/net/net_http_module.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/service/rpc_module.h"
#include "common/service/service_government_module.h"
#include "common/utility/magic_enum.h"
#include "app/game/server_define.h"

TONY_CAT_SPACE_BEGIN

ModuleManager GateServerApp::m_moduleManager = ModuleManager();

GateServerApp::GateServerApp() { m_name = typeid(*this).name(); }

void GateServerApp::SignalHandle(int sig) { m_moduleManager.Stop(); }

void GateServerApp::RegisterSignal() {
    std::signal(SIGABRT, &GateServerApp::SignalHandle);
    std::signal(SIGFPE, &GateServerApp::SignalHandle);
    std::signal(SIGILL, &GateServerApp::SignalHandle);
    std::signal(SIGINT, &GateServerApp::SignalHandle);
    std::signal(SIGSEGV, &GateServerApp::SignalHandle);
    std::signal(SIGTERM, &GateServerApp::SignalHandle);
}

void GateServerApp::Start(int32_t nServerIndex) {
    RegisterSignal();
    RegisterModule();
    InitModule(nServerIndex);
    Run();
    DestoryModule();
}

void GateServerApp::RegisterModule() {
    REGISTER_MODULE(&m_moduleManager, LogModule);
    REGISTER_MODULE(&m_moduleManager, XmlConfigModule);
    REGISTER_MODULE(&m_moduleManager, NetModule);
    REGISTER_MODULE(&m_moduleManager, NetPbModule);
    REGISTER_MODULE(&m_moduleManager, NetHttpModule);
    REGISTER_MODULE(&m_moduleManager, ServiceGovernmentModule);
    REGISTER_MODULE(&m_moduleManager, RpcModule);
    REGISTER_MODULE(&m_moduleManager, ClientPbModule);
}

void GateServerApp::InitModule(int32_t nServerIndex) {
    auto pServiceGovernmentModule =
        FIND_MODULE(&m_moduleManager, ServiceGovernmentModule);
    pServiceGovernmentModule->SetServerInstance(
        magic_enum::enum_name(ServerType::GateServer),
        ServerType::GateServer, nServerIndex);
    auto pLogModule =
        FIND_MODULE(&m_moduleManager, LogModule);
    pLogModule->SetLogName(
        std::string(magic_enum::enum_name(ServerType::GateServer)).append(std::to_string(nServerIndex)));

    m_moduleManager.Init();

    LOG_INFO("init server {}", m_name);
}

void GateServerApp::Run() { m_moduleManager.Run(); }

void GateServerApp::DestoryModule() { m_moduleManager.Stop(); }

TONY_CAT_SPACE_END
