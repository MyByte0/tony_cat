#include "xml_config_module.h"

#include "common/log/log_module.h"
#include "common/module_manager.h"

TONY_CAT_SPACE_BEGIN

XmlConfigModule::XmlConfigModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager) {}

XmlConfigModule::~XmlConfigModule() {}

void XmlConfigModule::BeforeInit() {
    if (false == LoadServerListConfigData()) {
        LOG_ERROR("LoadServerListConfigData false.");
        return;
    }
    if (false == LoadDataBaseConfigData()) {
        LOG_ERROR("LoadDataBaseConfigData false.");
        return;
    }
}

const tinyxml2::XMLElement* XmlConfigModule::FirstSiblingElement(
    tinyxml2::XMLDocument& doc) {
    tinyxml2::XMLElement* root = doc.RootElement();
    if (nullptr == root) {
        return nullptr;
    }
    return root->FirstChildElement("element");
}

const tinyxml2::XMLElement* XmlConfigModule::NextSiblingElement(
    const tinyxml2::XMLElement& elementNode) {
    return elementNode.NextSiblingElement();
}

const tinyxml2::XMLAttribute* XmlConfigModule::FirstAttribute(
    const tinyxml2::XMLElement& elementNode) {
    return elementNode.FirstAttribute();
}

const tinyxml2::XMLAttribute* XmlConfigModule::NextAttribute(
    const tinyxml2::XMLAttribute& elementAttribute) {
    return elementAttribute.Next();
}

TONY_CAT_SPACE_END
