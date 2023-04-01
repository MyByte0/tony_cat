#ifndef COMMON_LOOP_LOOP_POOL_H_
#define COMMON_LOOP_LOOP_POOL_H_

#include <asio.hpp>
#include <memory>
#include <vector>

#include "common/core_define.h"
#include "common/loop/loop.h"

TONY_CAT_SPACE_BEGIN

class LoopPool {
 public:
    LoopPool();
    LoopPool(LoopPool&&) = default;
    LoopPool(const LoopPool&) = delete;
    ~LoopPool();

 public:
    void Start(std::size_t workerNum);
    void Stop();

    void Exec(std::size_t index, Loop::FunctionRun&& func);
    void Exec(std::size_t index, const Loop::FunctionRun& func);

    void Broadcast(Loop::FunctionRun&& func);
    void Broadcast(const Loop::FunctionRun& func);

    asio::io_context& GetIoContext(std::size_t index);
    Loop* GetLoop(std::size_t index);

 private:
    std::vector<Loop> m_vecLoops;
};

typedef std::shared_ptr<LoopPool> LoopPoolPtr;

TONY_CAT_SPACE_END

#endif  // COMMON_LOOP_LOOP_POOL_H_
