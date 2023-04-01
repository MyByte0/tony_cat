#ifndef APP_GAME_LOGIC_SERVER_LOGIC_SERVER_APP_H_
#define APP_GAME_LOGIC_SERVER_LOGIC_SERVER_APP_H_

#include <string>

#include "common/module_manager.h"
#include "game/server_define.h"

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

#endif  // APP_GAME_LOGIC_SERVER_LOGIC_SERVER_APP_H_
