#ifndef COMMON_NET_NET_MEMORY_H_
#define COMMON_NET_NET_MEMORY_H_

#include <memory>

#include "common/core_define.h"

TONY_CAT_SPACE_BEGIN

struct NetMemoryPool {
    template <typename T>
    using PacketNode = std::shared_ptr<T>;
    template <typename T, typename... Args>
    static PacketNode<T> PacketCreate(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
};

TONY_CAT_SPACE_END

#endif  // COMMON_NET_NET_MEMORY_H_
