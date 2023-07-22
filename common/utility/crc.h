#ifndef COMMON_UTILITY_CRC_H_
#define COMMON_UTILITY_CRC_H_

#include <cstddef>
#include <cstdint>
#include <string>

#include "common/core_define.h"

TONY_CAT_SPACE_BEGIN

uint16_t CRC16(const void* data, size_t len);
uint32_t CRC32(const void* data, size_t len);
uint32_t CRC32(const std::string& data);
uint32_t CRC32(const void* dataHead, size_t lenHead, const void* data,
               size_t len);
uint32_t CRC32(const std::string& dataHead, const std::string& data);

TONY_CAT_SPACE_END

#endif  // COMMON_UTILITY_CRC_H_
