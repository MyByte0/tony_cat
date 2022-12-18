#ifndef XML_CONFIG_MODULE_H_
#define XML_CONFIG_MODULE_H_

#include "common/config/data/ServerListConfigData.h"
#include "common/config/data/DataBaseConfigData.h"

#include "common/core_define.h"
#include "common/log/log_module.h"
#include "common/module_base.h"

#include <tinyxml2.h>

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>


TONY_CAT_SPACE_BEGIN

#define DEFINE_CONFIG_DATA(_CONFIG_DATA_TYPE, _FILE_PATH)                                                 \
public:                                                                                                   \
    const _CONFIG_DATA_TYPE* Get##_CONFIG_DATA_TYPE##ById(_CONFIG_DATA_TYPE::TypeKeyFunArg keyId) const   \
    {                                                                                                     \
        auto it = m_map##_CONFIG_DATA_TYPE.find(keyId);                                                   \
        if (it != m_map##_CONFIG_DATA_TYPE.end()) {                                                       \
            return &it->second;                                                                           \
        }                                                                                                 \
        return nullptr;                                                                                   \
    }                                                                                                     \
    std::unordered_map<_CONFIG_DATA_TYPE::TypeKey, _CONFIG_DATA_TYPE>* Mutable##_CONFIG_DATA_TYPE##Map()  \
    {                                                                                                     \
        return &m_map##_CONFIG_DATA_TYPE;                                                                 \
    }                                                                                                     \
    bool Load##_CONFIG_DATA_TYPE()                                                                        \
    {                                                                                                     \
        m_map##_CONFIG_DATA_TYPE.clear();                                                                 \
        return XmlConfigModule::LoadXmlFile<_CONFIG_DATA_TYPE>(_FILE_PATH, m_map##_CONFIG_DATA_TYPE);     \
    }                                                                                                     \
                                                                                                          \
private:                                                                                                  \
    std::unordered_map<_CONFIG_DATA_TYPE::TypeKey, _CONFIG_DATA_TYPE> m_map##_CONFIG_DATA_TYPE;

#ifdef _WIN32
#define CONFIG_PATH_COMMON_PREFIX "../../../"
#elif __GNUC__
#define CONFIG_PATH_COMMON_PREFIX "../"
#endif

class NetPbModule;

class XmlConfigModule : public ModuleBase {
public:
    explicit XmlConfigModule(ModuleManager* pModuleManager);
    virtual ~XmlConfigModule();

public:
    virtual void BeforeInit() override;

    template <typename _TConfgData>
    static bool LoadXmlFile(const char* xmlPath, std::unordered_map<typename _TConfgData::TypeKey, _TConfgData>& mapData)
    {
        tinyxml2::XMLDocument doc;
        tinyxml2::XMLError errorXml;

#ifdef __linux__
        if ((errorXml = doc.LoadFile(xmlPath)) != tinyxml2::XML_SUCCESS) {
            LOG_ERROR("load file {} error:{}", xmlPath, static_cast<int32_t>(errorXml));
            return false;
        }
#elif _WIN32
        std::string strPath = xmlPath;
        std::replace(strPath.begin(), strPath.end(), '/', '\\');
        if ((errorXml = doc.LoadFile(strPath.c_str())) != tinyxml2::XML_SUCCESS) {
            LOG_ERROR("load file {} error:{}", strPath, static_cast<int32_t>(errorXml));
            return false;
        }
#endif
        for (const tinyxml2::XMLElement* pElementNode = FirstSiblingElement(doc); pElementNode != nullptr;
             pElementNode = NextSiblingElement(*pElementNode)) {
            _TConfgData stConfgData;
            for (const tinyxml2::XMLAttribute* pXMLAttribute = FirstAttribute(*pElementNode);
                 pXMLAttribute != nullptr; pXMLAttribute = NextAttribute(*pXMLAttribute)) {
                if (false == stConfgData.LoadXmlElement(pXMLAttribute)) {
                    LOG_ERROR("Paser error on:{}", xmlPath);
                    return false;
                }
            }
            mapData[stConfgData.GetKey()] = stConfgData;
        }
        return true;
    }

private:
    static const tinyxml2::XMLElement* FirstSiblingElement(tinyxml2::XMLDocument& doc); 
    static const tinyxml2::XMLElement* NextSiblingElement(const tinyxml2::XMLElement& elementNode); 

    static const tinyxml2::XMLAttribute* FirstAttribute(const tinyxml2::XMLElement& elementNode); 
    static const tinyxml2::XMLAttribute* NextAttribute(const tinyxml2::XMLAttribute& elementAttribute); 

public:
    DEFINE_CONFIG_DATA(ServerListConfigData, CONFIG_PATH_COMMON_PREFIX "config/xml/ServerList.xml");
    DEFINE_CONFIG_DATA(DataBaseConfigData, CONFIG_PATH_COMMON_PREFIX "config/xml/DataBase.xml");

};

TONY_CAT_SPACE_END

#endif // XML_CONFIG_MODULE_H_
