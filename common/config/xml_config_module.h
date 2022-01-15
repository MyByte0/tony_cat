#ifndef XML_CONFIG_MODULE_H_
#define XML_CONFIG_MODULE_H_

#include "ServerListConfigData.h"

#include "common/core_define.h"
#include "common/loop.h"
#include "common/loop_coroutine.h"
#include "common/loop_pool.h"
#include "common/module_base.h"
#include "log/log_module.h"
#include "net/net_session.h"
#include "protocol/server_base.pb.h"
#include "protocol/server_common.pb.h"

#include <tinyxml2.h>

#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <algorithm>
#endif

TONY_CAT_SPACE_BEGIN

#define DEFINE_CONFIG_DATA(_CONFIG_DATA_TYPE, _FILE_PATH)                                             \
public:                                                                                               \
    const _CONFIG_DATA_TYPE* Get##_CONFIG_DATA_TYPE##ById(int64_t nId) const                          \
    {                                                                                                 \
        auto it = m_map##_CONFIG_DATA_TYPE.find(nId);                                                 \
        if (it != m_map##_CONFIG_DATA_TYPE.end()) {                                                   \
            return &it->second;                                                                       \
        }                                                                                             \
        return nullptr;                                                                               \
    }                                                                                                 \
    std::unordered_map<int64_t, _CONFIG_DATA_TYPE>* Mutable##_CONFIG_DATA_TYPE##Map()                 \
    {                                                                                                 \
        return &m_map##_CONFIG_DATA_TYPE;                                                             \
    }                                                                                                 \
    bool Load##_CONFIG_DATA_TYPE()                                                                    \
    {                                                                                                 \
        m_map##_CONFIG_DATA_TYPE.clear();                                                             \
        return XmlConfigModule::LoadXmlFile<_CONFIG_DATA_TYPE>(_FILE_PATH, m_map##_CONFIG_DATA_TYPE); \
    }                                                                                                 \
                                                                                                      \
private:                                                                                              \
    std::unordered_map<int64_t, _CONFIG_DATA_TYPE> m_map##_CONFIG_DATA_TYPE;

class NetPbModule;

class XmlConfigModule : public ModuleBase {
public:
    XmlConfigModule(ModuleManager* pModuleManager);
    virtual ~XmlConfigModule();

public:
    virtual void BeforeInit() override;

    template <typename _TConfgData>
    static bool LoadXmlFile(const char* xmlPath, std::unordered_map<int64_t, _TConfgData>& mapData)
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
        tinyxml2::XMLElement* root = doc.RootElement();
        for (tinyxml2::XMLElement* elementNode = root->FirstChildElement("element"); elementNode != nullptr;
             elementNode = elementNode->NextSiblingElement()) {
            _TConfgData stConfgData;
            for (const tinyxml2::XMLAttribute* pXMLAttribute = elementNode->FirstAttribute();
                 pXMLAttribute != nullptr; pXMLAttribute = pXMLAttribute->Next()) {
                if (false == stConfgData.LoadXmlElement(pXMLAttribute)) {
                    LOG_ERROR("Paser error on:{}", xmlPath);
                    return false;
                }
            }
            mapData[stConfgData.nId] = stConfgData;
        }
        return true;
    }

    DEFINE_CONFIG_DATA(ServerListConfigData, "../../config/xml/ServerList.xml");
};

TONY_CAT_SPACE_END

#endif // XML_CONFIG_MODULE_H_
