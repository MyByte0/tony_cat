#ifndef COMMON_SERVICE_RPC_MODULE_H_
#define COMMON_SERVICE_RPC_MODULE_H_

#include "common/core_define.h"
#include "common/module_base.h"

TONY_CAT_SPACE_BEGIN

class MysqlModule : public ModuleBase {
public:
    MysqlModule(ModuleManager* pModuleManager);
    virtual ~MysqlModule();

    virtual void BeforeInit() override;
    virtual void AfterStop() override;

private:
};

TONY_CAT_SPACE_END

#endif // COMMON_SERVICE_RPC_MODULE_H_
