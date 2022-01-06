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
    int32_t nId = 0;
    std::string strServerName;
    int32_t nServerIndex = 0;
    std::string strServerIp;
    std::vector<std::string> vecConnectList;

    bool LoadXmlElement(const tinyxml2::XMLAttribute* pNodeAttribute) {
        if (std::string("Id") == pNodeAttribute->Name()) {
            if (false == ConfigValueToInt32(pNodeAttribute->Value(), nId)) {
                LOG_ERROR("Paser error on Id:{}", nId);
                return false;
            }
        }
        else if (std::string("ServerName") == pNodeAttribute->Name()) {
            if (false == ConfigValueToString(pNodeAttribute->Value(), strServerName)) {
                LOG_ERROR("Paser error on Id:{}", nId);
                return false;
            }
        }
        else if (std::string("ServerIndex") == pNodeAttribute->Name()) {
            if (false == ConfigValueToInt32(pNodeAttribute->Value(), nServerIndex)) {
                LOG_ERROR("Paser error on Id:{}", nId);
                return false;
            }
        }
        else if (std::string("ServerIp") == pNodeAttribute->Name()) {
            if (false == ConfigValueToString(pNodeAttribute->Value(), strServerIp)) {
                LOG_ERROR("Paser error on Id:{}", nId);
                return false;
            }
        }
        else if (std::string("ConnectList") == pNodeAttribute->Name()) {
            if (false == ConfigValueToVecString(pNodeAttribute->Value(), vecConnectList)) {
                LOG_ERROR("Paser error on Id:{}", nId);
                return false;
            }
        }
        return true;
    }
};

SER_NAME_SPACE_END

#endif // _ServerListConfigData_H
