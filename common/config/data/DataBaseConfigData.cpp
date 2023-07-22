
#include "DataBaseConfigData.h"

#include "common/core_define.h"
#include "common/log/log_module.h"
#include "common/utility/config_field_paser.h"

#include <tinyxml2.h>

TONY_CAT_SPACE_BEGIN

bool DataBaseConfigData::LoadXmlElement(const tinyxml2::XMLAttribute* pNodeAttribute) {
    if (std::string("Id") == pNodeAttribute->Name()) {
        if (false == ConfigValueToString(pNodeAttribute->Value(), strId)) {
            LOG_ERROR("Paser error on Id:{}", strId);
            return false;
        }
    }
    else if (std::string("Address") == pNodeAttribute->Name()) {
        if (false == ConfigValueToString(pNodeAttribute->Value(), strAddress)) {
            LOG_ERROR("Paser error on Id:{}", strId);
            return false;
        }
    }
    else if (std::string("Port") == pNodeAttribute->Name()) {
        if (false == ConfigValueToInt64(pNodeAttribute->Value(), nPort)) {
            LOG_ERROR("Paser error on Id:{}", strId);
            return false;
        }
    }
    else if (std::string("User") == pNodeAttribute->Name()) {
        if (false == ConfigValueToString(pNodeAttribute->Value(), strUser)) {
            LOG_ERROR("Paser error on Id:{}", strId);
            return false;
        }
    }
    else if (std::string("Password") == pNodeAttribute->Name()) {
        if (false == ConfigValueToString(pNodeAttribute->Value(), strPassword)) {
            LOG_ERROR("Paser error on Id:{}", strId);
            return false;
        }
    }
    else if (std::string("ShardNum") == pNodeAttribute->Name()) {
        if (false == ConfigValueToInt64(pNodeAttribute->Value(), nShardNum)) {
            LOG_ERROR("Paser error on Id:{}", strId);
            return false;
        }
    }
    return true;
}

TONY_CAT_SPACE_END
