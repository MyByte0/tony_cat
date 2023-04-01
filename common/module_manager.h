#ifndef COMMON_MODULE_MANAGER_H_
#define COMMON_MODULE_MANAGER_H_

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include "common/core_define.h"
#include "common/loop/loop.h"
#include "common/module_base.h"

TONY_CAT_SPACE_BEGIN

class ModuleManager {
 public:
    ModuleManager();
    ~ModuleManager();

    bool RegisterModule(const std::string& strModuleName, ModuleBase* pModule);
    ModuleBase* FindModule(const std::string& strModuleName);

    void Init();
    void Stop();
    void LoadConfig();
    void Run();

    Loop& GetMainLoop();
    bool OnMainLoop();

 private:
    void UpdateModules();
    void StopModules();

 private:
    struct ModuleInfo {
        std::string strModuleName;
        ModuleBase* pModule = nullptr;
    };
    std::vector<ModuleInfo> m_vecModules;
    Loop m_loop;
    Loop::TimerHandle m_updateTimerHandler;
    bool m_onStop = false;
    uint32_t m_nframe = 50;
};

#define REGISTER_MODULE(MODULE_MANAGER, MODULE_NAME) \
    (MODULE_MANAGER)                                 \
        ->RegisterModule(#MODULE_NAME, new MODULE_NAME((MODULE_MANAGER)));

#define FIND_MODULE(MODULE_MANAGER, MODULE_NAME)                            \
    dynamic_cast<MODULE_NAME*>((MODULE_MANAGER)->FindModule(#MODULE_NAME)); \
    assert(dynamic_cast<MODULE_NAME*>(                                      \
               (MODULE_MANAGER)->FindModule(#MODULE_NAME)) != nullptr);

TONY_CAT_SPACE_END

#endif  // COMMON_MODULE_MANAGER_H_
