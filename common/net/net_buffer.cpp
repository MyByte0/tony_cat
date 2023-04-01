#include "net_buffer.h"

#include <algorithm>
#include <cstring>

TONY_CAT_SPACE_BEGIN

SessionBuffer::SessionBuffer(size_t maxBuffSize) {
    nMaxBuffSize = maxBuffSize;
    size_t nDefaultBuffSize = nMaxBuffSize / k_init_buffer_size_div_number;
    bufContext.vecData.resize(nDefaultBuffSize, 0);
    bufContext.nReadPos = 0;
    bufContext.nWritePos = 0;
}

SessionBuffer::~SessionBuffer() {}

bool SessionBuffer::Empty() {
    return bufContext.nReadPos == bufContext.nWritePos;
}

bool SessionBuffer::Full() {
    return bufContext.nWritePos - bufContext.nReadPos ==
           bufContext.vecData.size();
}

const char* SessionBuffer::GetReadData() {
    return &bufContext.vecData[bufContext.nReadPos];
}

size_t SessionBuffer::GetReadableSize() {
    return bufContext.nWritePos - bufContext.nReadPos;
}

void SessionBuffer::RemoveData(size_t len) {
    if (len > GetReadableSize()) [[unlikely]] {
        len = GetReadableSize();
    }

    bufContext.nReadPos += len;
    if (bufContext.nReadPos == bufContext.nWritePos) {
        bufContext.nReadPos = bufContext.nWritePos = 0;
    }
}

bool SessionBuffer::FillData(size_t len) {
    if (len > MaxWritableSize()) [[unlikely]] {
        return false;
    }

    if (len > CurrentWritableSize()) {
        size_t beforeSize = bufContext.vecData.size();
        size_t curSize = beforeSize;

        ReorganizeData();
        while (len > curSize - bufContext.nWritePos) {
            curSize = std::min(curSize << 2, nMaxBuffSize);
        }

        bufContext.vecData.resize(curSize);
    }

    bufContext.nWritePos += len;
    return true;
}

char* SessionBuffer::GetWriteData() {
    return &bufContext.vecData[bufContext.nWritePos];
}

bool SessionBuffer::Write(const char* data, size_t len) {
    if (false == FillData(len)) {
        return false;
    }

    size_t writePos = bufContext.nWritePos - len;
    std::copy(data, data + len, bufContext.vecData.begin() + writePos);
    return true;
}

size_t SessionBuffer::CurrentWritableSize() {
    return bufContext.vecData.size() - bufContext.nWritePos;
}

size_t SessionBuffer::MaxWritableSize() {
    return nMaxBuffSize - GetReadableSize();
}

void SessionBuffer::ReorganizeData() {
    if (bufContext.nReadPos > 0) {
        std::memmove(bufContext.vecData.data(), GetReadData(),
                     GetReadableSize());
        bufContext.nWritePos -= bufContext.nReadPos;
        bufContext.nReadPos = 0;
    }
}

bool SessionBuffer::Shrink() {
    size_t nDefaultBuffSize = nMaxBuffSize / k_init_buffer_size_div_number;
    if (GetReadableSize() >= nDefaultBuffSize) {
        return false;
    }

    std::vector<char> vecBuff(nDefaultBuffSize);
    std::memmove(vecBuff.data(), GetReadData(), GetReadableSize());
    bufContext.nWritePos -= bufContext.nReadPos;
    bufContext.nReadPos = 0;
    bufContext.vecData.swap(vecBuff);
    return true;
}

TONY_CAT_SPACE_END
