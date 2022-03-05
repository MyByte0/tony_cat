#ifndef COMMON_LOOP_POOL_H_
#define COMMON_LOOP_POOL_H_

#include "core_define.h"
#include "loop.h"

#include <asio.hpp>

TONY_CAT_SPACE_BEGIN

class LoopPool {
public:
    LoopPool();
    ~LoopPool();

public:
    void Start(std::size_t workerNum);
    void Stop();

    void Exec(std::size_t index, Loop::FunctionRun&& func);
    void Exec(std::size_t index, const Loop::FunctionRun& func);

    void Broadcast(Loop::FunctionRun&& func);
    void Broadcast(const Loop::FunctionRun& func);

    asio::io_context& GetIoContext(std::size_t index);

private:
    Loop* GetLoop(std::size_t index);

private:
    std::vector<Loop> m_vecLoops;
};

TONY_CAT_SPACE_END

#endif // COMMON_LOOP_POOL_H_
