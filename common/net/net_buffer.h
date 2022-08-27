#ifndef COMMON_NET_NET_BUFFER_H_
#define COMMON_NET_NET_BUFFER_H_

#include <atomic>
#include <cassert>
#include <cstdint>
#include <vector>

#include "common/core_define.h"
#include "common/loop/loop.h"

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
        k_default_max_buffer_size = 8 * 1024 * 4096,  // 8MB
    };

    explicit SessionBuffer(size_t maxBuffSize = k_default_max_buffer_size);
    ~SessionBuffer();

    bool Empty();
    bool Full();
    const char* GetReadData();

    size_t GetReadableSize();
    void RemoveData(size_t len);
    bool PaddingData(size_t len);
    char* GetWriteData(size_t len);
    bool MoveWritePos(size_t len);

    char* GetWriteData();
    bool Write(const char* data, size_t len);
    size_t CurrentWritableSize();
    size_t MaxWritableSize();
    bool Shrink();
    size_t Capacity() { return bufContext.vecData.size(); }

#ifdef NDEBUG
    inline void DebugThreadWriteCheck() { return; }
#else
    std::atomic<Loop*> m_debugWriteThread = nullptr;
    void DebugThreadWriteCheck() {
        Loop* m_loopNull = nullptr;
        Loop* loopExpect = &Loop::GetCurrentThreadLoop();
        m_debugWriteThread.compare_exchange_strong(m_loopNull, loopExpect);
        assert(m_debugWriteThread == loopExpect);
    }
#endif

    // notice: just call by NetModule lambda
public:
    void ReorganizeData();
    void ShrinkByLength(size_t len);
};

TONY_CAT_SPACE_END

#endif  // COMMON_NET_NET_BUFFER_H_
