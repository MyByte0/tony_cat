#ifndef CONFIG_ServerListConfigData_H_
#define CONFIG_ServerListConfigData_H_

#include "common/core_define.h"

#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace tinyxml2 {
class XMLAttribute;
}

TONY_CAT_SPACE_BEGIN

struct ServerListConfigData {
    using TypeKey = int64_t;
    using TypeKeyFunArg = int64_t;
    TypeKeyFunArg GetKey() const { return nId; }

    int64_t nId = 0;
    int32_t nServerType = 0;
    std::string strServerName;
    int32_t nServerIndex = 0;
    std::string strPublicIp;
    std::string strServerIp;
    std::vector<int32_t> vecConnectList;
    int32_t nNetThreadsNum = 0;

    bool LoadXmlElement(const tinyxml2::XMLAttribute* pNodeAttribute);
};

TONY_CAT_SPACE_END

#endif // CONFIG_ServerListConfigData_H
