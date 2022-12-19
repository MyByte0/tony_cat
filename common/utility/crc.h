#ifndef COMMON_UTILITY_CRC_H_
#define COMMON_UTILITY_CRC_H_

#include "../core_define.h"

#include <cstddef>
#include <cstdint>

SER_NAME_SPACE_BEGIN

uint16_t CRC16(const void* data, size_t len);
uint32_t CRC32(const void* data, size_t len);
uint32_t CRC32(const void* dataHead, size_t lenHead, const void* data, size_t len);

SER_NAME_SPACE_END

#endif // COMMON_UTILITY_CRC_H_
