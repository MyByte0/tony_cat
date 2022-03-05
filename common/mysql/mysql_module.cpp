#include "mysql_module.h"

#include "common/log/log_module.h"
#include "common/module_manager.h"

TONY_CAT_SPACE_BEGIN

MysqlModule::MysqlModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

MysqlModule::~MysqlModule() { }

void MysqlModule::BeforeInit()
{
}

void MysqlModule::AfterStop()
{
}

TONY_CAT_SPACE_END
