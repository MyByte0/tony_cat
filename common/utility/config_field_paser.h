#ifndef COMMON_UTILITY_CONFIG_FIELD_PASER_H_
#define COMMON_UTILITY_CONFIG_FIELD_PASER_H_

#include "common/core_define.h"

#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

SER_NAME_SPACE_BEGIN

void SplitString(const char* pData, std::vector<std::string>& vecResult, const char cSplit);
void SplitStringString(const char* pData, std::vector<std::vector<std::string>>& vecResult, const char cSplitFirst, const char cSplitSecond);

bool ConfigValueToInt32(const char* pData, int32_t& nResult);
bool ConfigValueToInt64(const char* pData, int64_t& nResult);
bool ConfigValueToDouble(const char* pData, double& nResult);
bool ConfigValueToString(const char* pData, std::string& strResult);
bool ConfigValueToTimestamp(const char* pData, time_t& tResult);

bool ConfigValueToVecInt32(const char* pData, std::vector<int32_t>& vecResult);
bool ConfigValueToVecInt64(const char* pData, std::vector<int64_t>& vecResult);
bool ConfigValueToVecString(const char* pData, std::vector<std::string>& vecResult);
bool ConfigValueToVecTimestamp(const char* pData, std::vector<time_t>& vecResult);

bool ConfigValueToInt32MapInt32(const char* pData, std::unordered_map<int32_t, int32_t>& mapResult);
bool ConfigValueToInt32MapInt64(const char* pData, std::unordered_map<int32_t, int64_t>& mapResult);
bool ConfigValueToInt32MapString(const char* pData, std::unordered_map<int32_t, std::string>& mapResult);
bool ConfigValueToInt64MapInt32(const char* pData, std::unordered_map<int64_t, int32_t>& mapResult);
bool ConfigValueToInt64MapInt64(const char* pData, std::unordered_map<int64_t, int64_t>& mapResult);
bool ConfigValueToInt64MapString(const char* pData, std::unordered_map<int64_t, std::string>& mapResult);
bool ConfigValueToStringMapInt32(const char* pData, std::unordered_map<std::string, int32_t>& mapResult);
bool ConfigValueToStringMapInt64(const char* pData, std::unordered_map<std::string, int64_t>& mapResult);
bool ConfigValueToStringMapString(const char* pData, std::unordered_map<std::string, std::string>& mapResult);

SER_NAME_SPACE_END

#endif // COMMON_UTILITY_CONFIG_FIELD_PASER_H_
