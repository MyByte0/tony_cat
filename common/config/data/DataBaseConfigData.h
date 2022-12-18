#ifndef CONFIG_DataBaseConfigData_H_
#define CONFIG_DataBaseConfigData_H_

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

struct DataBaseConfigData {
    using TypeKey = std::string;
    using TypeKeyFunArg = const std::string &;
    TypeKeyFunArg GetKey() const { return strId; }

    std::string strId;
    std::string strAddress;
    int64_t nPort = 0;
    std::string strUser;
    std::string strPassword;

    bool LoadXmlElement(const tinyxml2::XMLAttribute* pNodeAttribute);
};

TONY_CAT_SPACE_END

#endif // CONFIG_DataBaseConfigData_H
