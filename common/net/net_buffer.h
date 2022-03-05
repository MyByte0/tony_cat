#ifndef NET_NET_BUFFER_H_
#define NET_NET_BUFFER_H_

#include "common/core_define.h"

#include <cstdint>
#include <vector>

TONY_CAT_SPACE_BEGIN

struct SessionBuffer {
private:
    struct BufferContext {
        std::vector<char> vecData;
        size_t nReadPos = 0;
        size_t nWritePos = 0;
    };
    size_t nMaxBuffSize;
    BufferContext bufContext;

public:
    enum {
        k_init_buffer_size_div_number = 16,
        k_default_max_buffer_size = 8 * 1024 * 4096, // 8MB
    };

    SessionBuffer(size_t maxBuffSize = k_default_max_buffer_size);
    ~SessionBuffer();

    bool Empty();
    bool Full();
    const char* GetReadData();

    size_t GetReadableSize();
    void RemoveData(size_t len);
    bool FillData(size_t len);
    char* GetWriteData();
    bool Write(const char* data, size_t len);
    size_t CurrentWritableSize();
    size_t MaxWritableSize();
    bool Shrink();

private:
    void ReorganizeData();
};

TONY_CAT_SPACE_END

#endif // NET_NET_BUFFER_H_
