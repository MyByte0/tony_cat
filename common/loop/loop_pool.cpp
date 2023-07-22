#include "loop_pool.h"

#include <algorithm>
#include <utility>

TONY_CAT_SPACE_BEGIN

LoopPool::LoopPool() {}
LoopPool::~LoopPool() { Stop(); }

void LoopPool::Start(std::size_t workerNum) {
    m_vecLoops.resize(workerNum);
    for (uint64_t iVecLoops = 0; iVecLoops < m_vecLoops.size(); ++iVecLoops) {
        m_vecLoops[iVecLoops].StartWith(
            [iVecLoops]() { Loop::t_indexInLoop = iVecLoops; });
    }
}

void LoopPool::Stop() {
    std::for_each(m_vecLoops.begin(), m_vecLoops.end(),
                  [](Loop& loop) { loop.Stop(); });
    m_vecLoops.clear();
}

uint64_t LoopPool::GetIndexInLoopPool() { return Loop::t_indexInLoop; }

void LoopPool::Exec(std::size_t index, Loop::FunctionRun&& func) {
    if (auto pLoop = GetLoop(index); pLoop != nullptr) [[likely]] {
        pLoop->Exec(std::move(func));
    }
    return;
}

void LoopPool::Exec(std::size_t index, const Loop::FunctionRun& func) {
    if (auto pLoop = GetLoop(index); pLoop != nullptr) [[likely]] {
        pLoop->Exec(func);
    }
    return;
}

void LoopPool::Broadcast(Loop::FunctionRun&& func) {
    auto pFunDo = std::make_shared<Loop::FunctionRun>(std::move(func));
    std::for_each(m_vecLoops.begin(), m_vecLoops.end(),
                  [pFunDo](Loop& loop) mutable {
                      loop.Exec([pFunDo]() { (*pFunDo)(); });
                  });
}

void LoopPool::Broadcast(const Loop::FunctionRun& func) {
    std::for_each(m_vecLoops.begin(), m_vecLoops.end(),
                  [func](Loop& loop) { loop.Exec(func); });
}

void LoopPool::BroadcastAndDone(Loop::FunctionRun&& funcDo,
                                Loop::FunctionRun&& funcFinish) {
    auto& curLoop = Loop::GetCurrentThreadLoop();
    auto pFinishCount = std::make_shared<std::atomic<uint64_t>>(0);
    auto nFinishSize = m_vecLoops.size();
    auto pFinishDo = std::make_shared<Loop::FunctionRun>(std::move(funcFinish));
    auto pFunDo = std::make_shared<Loop::FunctionRun>(std::move(funcDo));
    for (std::size_t iVecLoops = 0; iVecLoops < nFinishSize; ++iVecLoops) {
        m_vecLoops[iVecLoops].Exec(
            [pFunDo, pFinishDo, &curLoop, pFinishCount, nFinishSize]() {
                (*pFunDo)();
                auto nCurCount = ++(*pFinishCount);
                if (nCurCount == nFinishSize) {
                    curLoop.Exec([pFinishDo]() { (*pFinishDo)(); });
                }
            });
    }
}

asio::io_context& LoopPool::GetIoContext(std::size_t index) {
    if (auto pLoop = GetLoop(index); pLoop != nullptr) [[likely]] {
        return pLoop->GetIoContext();
    }

    {
        // maybe abort is better
        static asio::io_context s_oi_context;
        return s_oi_context;
    }
}

Loop* LoopPool::GetLoop(std::size_t index) {
    if (!m_vecLoops.empty()) [[likely]] {
        std::size_t indexLoop = index % m_vecLoops.size();
        return &m_vecLoops[indexLoop];
    }

    return nullptr;
}

TONY_CAT_SPACE_END
