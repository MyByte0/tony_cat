#ifndef COMMON_MODULE_BASE_H_
#define COMMON_MODULE_BASE_H_

#include "common/core_define.h"

TONY_CAT_SPACE_BEGIN

class ModuleManager;

class ModuleBase {
 public:
    explicit ModuleBase(ModuleManager* pModuleManager);
    virtual ~ModuleBase();

    virtual void BeforeInit() {}
    virtual void OnInit() {}
    virtual void AfterInit() {}

    virtual void BeforeStop() {}
    virtual void OnStop() {}
    virtual void AfterStop() {}

    virtual void BeforeUpdate() {}
    virtual void OnUpdate() {}

    virtual void OnLoadConfig() {}
    virtual void AfterLoadConfig() {}

 protected:
    ModuleManager* m_pModuleManager;
};

TONY_CAT_SPACE_END

#endif  // COMMON_MODULE_BASE_H_
