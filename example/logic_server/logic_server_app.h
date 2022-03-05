#ifndef LOGICSERVER_LOGIC_SERVER_APP_H_
#define LOGICSERVER_LOGIC_SERVER_APP_H_

#include "common/module_manager.h"
#include "server_common/server_define.h"

#include <string>

TONY_CAT_SPACE_BEGIN

class LogicServerApp {
public:
    LogicServerApp();
    ~LogicServerApp() = default;

    static void SignalHandle(int sig);
    void RegisterSignal();
    void Start(int32_t nServerIndex);

private:
    void RegisterModule();

    void InitModule(int32_t nServerIndex);
    void Run();
    void DestoryModule();

private:
    static ModuleManager m_moduleManager;
    std::string m_name;
};

TONY_CAT_SPACE_END

#endif // LOGICSERVER_LOGIC_SERVER_APP_H_