#ifndef CLIENT_CLIENT_TEST_APP_H_
#define CLIENT_CLIENT_TEST_APP_H_

#include "common/module_manager.h"
#include "server_define.h"

#include <string>

TONY_CAT_SPACE_BEGIN

class ClientTestApp {
public:
    ClientTestApp();
    ~ClientTestApp() = default;

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

#endif // CLIENT_CLIENT_TEST_APP_H_