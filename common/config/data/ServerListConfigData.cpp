
#include "ServerListConfigData.h"

#include "common/core_define.h"
#include "common/log/log_module.h"
#include "common/utility/config_field_paser.h"

#include <tinyxml2.h>

TONY_CAT_SPACE_BEGIN

bool ServerListConfigData::LoadXmlElement(const tinyxml2::XMLAttribute* pNodeAttribute) {
    if (std::string("Id") == pNodeAttribute->Name()) {
        if (false == ConfigValueToInt64(pNodeAttribute->Value(), nId)) {
            LOG_ERROR("Paser error on Id:{}", nId);
            return false;
        }
    }
    else if (std::string("ServerType") == pNodeAttribute->Name()) {
        if (false == ConfigValueToInt32(pNodeAttribute->Value(), nServerType)) {
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
    else if (std::string("PublicIp") == pNodeAttribute->Name()) {
        if (false == ConfigValueToString(pNodeAttribute->Value(), strPublicIp)) {
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
        if (false == ConfigValueToVecInt32(pNodeAttribute->Value(), vecConnectList)) {
            LOG_ERROR("Paser error on Id:{}", nId);
            return false;
        }
    }
    else if (std::string("NetThreadsNum") == pNodeAttribute->Name()) {
        if (false == ConfigValueToInt32(pNodeAttribute->Value(), nNetThreadsNum)) {
            LOG_ERROR("Paser error on Id:{}", nId);
            return false;
        }
    }
    else if (std::string("HttpIp") == pNodeAttribute->Name()) {
        if (false == ConfigValueToString(pNodeAttribute->Value(), strHttpIp)) {
            LOG_ERROR("Paser error on Id:{}", nId);
            return false;
        }
    }
    else if (std::string("HttpThreadsNum") == pNodeAttribute->Name()) {
        if (false == ConfigValueToInt32(pNodeAttribute->Value(), nHttpThreadsNum)) {
            LOG_ERROR("Paser error on Id:{}", nId);
            return false;
        }
    }
    return true;
}

TONY_CAT_SPACE_END
