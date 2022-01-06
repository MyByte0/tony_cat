#include "xml_config_module.h"
#include "common/module_manager.h"
#include "common/utility/crc.h"
#include "log/log_module.h"
#include "net/net_pb_module.h"

SER_NAME_SPACE_BEGIN

XmlConfigModule::XmlConfigModule(ModuleManager* pModuleManager)
    :ModuleBase(pModuleManager) {

}

XmlConfigModule::~XmlConfigModule() {}

void XmlConfigModule::BeforeInit() {
    LoadServerListConfigData();
}

SER_NAME_SPACE_END
