#include "config_field_paser.h"

#include <cstdlib>
#include <cstring>
#include <ctime>

TONY_CAT_SPACE_BEGIN

// maybe vecResult use string_view is better
void SplitString(const char* pData, std::vector<std::string>& vecResult, const char cSplit)
{
    if (nullptr == pData) {
        return;
    }

    const char* pIndexStart = pData;
    const char* pIndexEnd = pData;
    for (; '\0' != *pIndexEnd; ++pIndexEnd) {
        if (cSplit == *pIndexEnd) {
            vecResult.emplace_back(pIndexStart, pIndexEnd);
            pIndexStart = pIndexEnd + 1;
        }
    }

    if (pIndexEnd != pData) {
        vecResult.emplace_back(pIndexStart, pIndexEnd);
    }
}

void SplitStringString(const char* pData, std::vector<std::vector<std::string>>& vecResult, const char cSplitFirst, const char cSplitSecond)
{
    if (nullptr == pData) {
        return;
    }

    std::vector<std::string> vecStringResult;
    SplitString(pData, vecStringResult, cSplitSecond);
    for (const auto& elemStringResult : vecStringResult) {
        std::vector<std::string> vecResElem;
        SplitString(elemStringResult.c_str(), vecResElem, cSplitFirst);
        vecResult.emplace_back(std::move(vecResElem));
    }
}

bool ConfigValueToInt32(const char* pData, int32_t& nResult)
{
    nResult = atoi(pData);
    return true;
}

bool ConfigValueToInt64(const char* pData, int64_t& nResult)
{
    nResult = atoll(pData);
    return true;
}

bool ConfigValueToDouble(const char* pData, double& nResult)
{
    nResult = atof(pData);
    return true;
}

bool ConfigValueToString(const char* pData, std::string& strResult)
{
    if (nullptr == pData) {
        return false;
    }

    strResult = pData;
    return true;
}

bool ConfigValueToTimestamp(const char* pData, time_t& tResult)
{
    if (nullptr == pData) {
        return false;
    }
    struct tm tm;
    memset(&tm, 0, sizeof(tm));

    sscanf(pData, "%d-%d-%d %d.%d.%d",
        &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
        &tm.tm_hour, &tm.tm_min, &tm.tm_sec);

    tm.tm_year -= 1900;
    tm.tm_mon--;

    tResult = mktime(&tm);
    return true;
}

bool ConfigValueToVecInt32(const char* pData, std::vector<int32_t>& vecResult)
{
    std::vector<std::string> vecStringResult;
    SplitString(pData, vecStringResult, ':');
    for (const auto& elemStringResult : vecStringResult) {
        int32_t nResult = 0;
        ConfigValueToInt32(elemStringResult.c_str(), nResult);
        vecResult.emplace_back(nResult);
    }
    return true;
}

bool ConfigValueToVecInt64(const char* pData, std::vector<int64_t>& vecResult)
{
    std::vector<std::string> vecStringResult;
    SplitString(pData, vecStringResult, ':');
    for (const auto& elemStringResult : vecStringResult) {
        int64_t nResult = 0;
        ConfigValueToInt64(elemStringResult.c_str(), nResult);
        vecResult.emplace_back(nResult);
    }
    return true;
}

bool ConfigValueToVecString(const char* pData, std::vector<std::string>& vecResult)
{
    SplitString(pData, vecResult, ':');
    return true;
}

bool ConfigValueToVecTimestamp(const char* pData, std::vector<time_t>& vecResult)
{
    std::vector<std::string> vecStringResult;
    SplitString(pData, vecStringResult, ':');
    for (const auto& elemStringResult : vecStringResult) {
        time_t nResult = 0;
        ConfigValueToTimestamp(elemStringResult.c_str(), nResult);
        vecResult.emplace_back(nResult);
    }
    return true;
}

bool ConfigValueToInt32MapInt32(const char* pData, std::unordered_map<int32_t, int32_t>& mapResult)
{
    std::vector<std::vector<std::string>> vecResult;
    SplitStringString(pData, vecResult, ':', '|');

    for (const auto& elemVecResult : vecResult) {
        int32_t nKey = 0;
        if (elemVecResult.size() > 0) {
            ConfigValueToInt32(elemVecResult[0].c_str(), nKey);
        }
        int32_t nValue = 0;
        if (elemVecResult.size() > 1) {
            ConfigValueToInt32(elemVecResult[1].c_str(), nValue);
        }
        mapResult[nKey] = nValue;
    }

    return true;
}

bool ConfigValueToInt32MapInt64(const char* pData, std::unordered_map<int32_t, int64_t>& mapResult)
{
    std::vector<std::vector<std::string>> vecResult;
    SplitStringString(pData, vecResult, ':', '|');

    for (const auto& elemVecResult : vecResult) {
        int32_t nKey = 0;
        if (elemVecResult.size() > 0) {
            ConfigValueToInt32(elemVecResult[0].c_str(), nKey);
        }
        int64_t nValue = 0;
        if (elemVecResult.size() > 1) {
            ConfigValueToInt64(elemVecResult[1].c_str(), nValue);
        }
        mapResult[nKey] = nValue;
    }

    return true;
}

bool ConfigValueToInt32MapString(const char* pData, std::unordered_map<int32_t, std::string>& mapResult)
{
    std::vector<std::vector<std::string>> vecResult;
    SplitStringString(pData, vecResult, ':', '|');

    for (auto& elemVecResult : vecResult) {
        int32_t nKey = 0;
        if (elemVecResult.size() > 0) {
            ConfigValueToInt32(elemVecResult[0].c_str(), nKey);
        }
        std::string strValue;
        if (elemVecResult.size() > 1) {
            strValue = elemVecResult[1];
        }
        mapResult[nKey] = strValue;
    }

    return true;
}

bool ConfigValueToInt64MapInt32(const char* pData, std::unordered_map<int64_t, int32_t>& mapResult)
{
    std::vector<std::vector<std::string>> vecResult;
    SplitStringString(pData, vecResult, ':', '|');

    for (const auto& elemVecResult : vecResult) {
        int64_t nKey = 0;
        if (elemVecResult.size() > 0) {
            ConfigValueToInt64(elemVecResult[0].c_str(), nKey);
        }
        int32_t nValue = 0;
        if (elemVecResult.size() > 1) {
            ConfigValueToInt32(elemVecResult[1].c_str(), nValue);
        }
        mapResult[nKey] = nValue;
    }

    return true;
}

bool ConfigValueToInt64MapInt64(const char* pData, std::unordered_map<int64_t, int64_t>& mapResult)
{
    std::vector<std::vector<std::string>> vecResult;
    SplitStringString(pData, vecResult, ':', '|');

    for (const auto& elemVecResult : vecResult) {
        int64_t nKey = 0;
        if (elemVecResult.size() > 0) {
            ConfigValueToInt64(elemVecResult[0].c_str(), nKey);
        }
        int64_t nValue = 0;
        if (elemVecResult.size() > 1) {
            ConfigValueToInt64(elemVecResult[1].c_str(), nValue);
        }
        mapResult[nKey] = nValue;
    }

    return true;
}

bool ConfigValueToInt64MapString(const char* pData, std::unordered_map<int64_t, std::string>& mapResult)
{
    std::vector<std::vector<std::string>> vecResult;
    SplitStringString(pData, vecResult, ':', '|');

    for (auto& elemVecResult : vecResult) {
        int64_t nKey = 0;
        if (elemVecResult.size() > 0) {
            ConfigValueToInt64(elemVecResult[0].c_str(), nKey);
        }
        std::string strValue;
        if (elemVecResult.size() > 1) {
            strValue = elemVecResult[1];
        }
        mapResult[nKey] = strValue;
    }

    return true;
}

bool ConfigValueToStringMapInt32(const char* pData, std::unordered_map<std::string, int32_t>& mapResult)
{
    std::vector<std::vector<std::string>> vecResult;
    SplitStringString(pData, vecResult, ':', '|');

    for (const auto& elemVecResult : vecResult) {
        std::string strKey;
        if (elemVecResult.size() > 0) {
            strKey = elemVecResult[0];
        }
        int32_t nValue = 0;
        if (elemVecResult.size() > 1) {
            ConfigValueToInt32(elemVecResult[1].c_str(), nValue);
        }
        mapResult[strKey] = nValue;
    }

    return true;
}

bool ConfigValueToStringMapInt64(const char* pData, std::unordered_map<std::string, int64_t>& mapResult)
{
    std::vector<std::vector<std::string>> vecResult;
    SplitStringString(pData, vecResult, ':', '|');

    for (const auto& elemVecResult : vecResult) {
        std::string strKey;
        if (elemVecResult.size() > 0) {
            strKey = elemVecResult[0];
        }
        int64_t nValue = 0;
        if (elemVecResult.size() > 1) {
            ConfigValueToInt64(elemVecResult[1].c_str(), nValue);
        }
        mapResult[strKey] = nValue;
    }

    return true;
}

bool ConfigValueToStringMapString(const char* pData, std::unordered_map<std::string, std::string>& mapResult)
{
    std::vector<std::vector<std::string>> vecResult;
    SplitStringString(pData, vecResult, ':', '|');

    for (const auto& elemVecResult : vecResult) {
        std::string strKey;
        if (elemVecResult.size() > 0) {
            strKey = elemVecResult[0];
        }
        std::string strValue;
        if (elemVecResult.size() > 1) {
            strValue = elemVecResult[1];
        }
        mapResult[strKey] = strValue;
    }

    return true;
}

TONY_CAT_SPACE_END
