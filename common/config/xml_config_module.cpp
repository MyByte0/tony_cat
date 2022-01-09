#include "xml_config_module.h"
#include "common/module_manager.h"
#include "common/utility/crc.h"
#include "log/log_module.h"
#include "net/net_pb_module.h"

TONY_CAT_SPACE_BEGIN

XmlConfigModule::XmlConfigModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

XmlConfigModule::~XmlConfigModule() { }

void XmlConfigModule::BeforeInit()
{
    if (false == LoadServerListConfigData()) {
        LOG_ERROR("LoadServerListConfigData false.");
        return;
    }
}

TONY_CAT_SPACE_END
