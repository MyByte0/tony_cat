#ifndef ServerListConfigData_
#define ServerListConfigData_

#include "common/core_define.h"
#include "common/utility/config_field_paser.h"
#include "log/log_module.h"

#include <tinyxml2.h>

#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

SER_NAME_SPACE_BEGIN

struct ServerListConfigData {
    int32_t Id;
    std::string ServerName;
    int32_t ServerIndex;
    std::string ServerIp;
    std::vector<int32_t> connectList;

    bool LoadXmlElement(const tinyxml2::XMLAttribute* pNodeAttribute) {
        if (std::string("Id") == pNodeAttribute->Name()) {
            if (ConfigValueToInt32(pNodeAttribute->Value(), nId)) {
                LOG_ERROR("Paser error on Id:{}", nId);
                return false;
            }
        }
        else if (std::string("ServerName") == pNodeAttribute->Name()) {
            if (ConfigValueToString(pNodeAttribute->Value(), strServerName)) {
                LOG_ERROR("Paser error on Id:{}", nId);
                return false;
            }
        }
        else if (std::string("ServerIndex") == pNodeAttribute->Name()) {
            if (ConfigValueToInt32(pNodeAttribute->Value(), nServerIndex)) {
                LOG_ERROR("Paser error on Id:{}", nId);
                return false;
            }
        }
        else if (std::string("ServerIp") == pNodeAttribute->Name()) {
            if (ConfigValueToString(pNodeAttribute->Value(), strServerIp)) {
                LOG_ERROR("Paser error on Id:{}", nId);
                return false;
            }
        }
        else if (std::string("connectList") == pNodeAttribute->Name()) {
            if (ConfigValueToVecInt32(pNodeAttribute->Value(), vecconnectList)) {
                LOG_ERROR("Paser error on Id:{}", nId);
                return false;
            }
        }
};

SER_NAME_SPACE_END

#endif // _ServerListConfigData_H
