#include "loop_pool.h"

#include <algorithm>

SER_NAME_SPACE_BEGIN

LoopPool::LoopPool() {}
LoopPool::~LoopPool() { 
    Stop(); 
}

void LoopPool::Start(std::size_t workerNum) {
    m_vecLoops.resize(workerNum); 
    std::for_each(m_vecLoops.begin(), m_vecLoops.end(),
                  [](Loop& loop) { loop.Start(); });
}

void LoopPool::Stop() {
  std::for_each(m_vecLoops.begin(), m_vecLoops.end(),
                [](Loop& loop) { loop.Stop(); });
  m_vecLoops.clear();
}

void LoopPool::Exec(std::size_t index, Loop::FunctionRun&& func) {
  auto pLoop = GetLoop(index);
  if (pLoop != nullptr) [[likely]] {
    pLoop->Exec(std::move(func));
  }

  return;
}

void LoopPool::Exec(std::size_t index, const Loop::FunctionRun& func) {
  auto pLoop = GetLoop(index);
  if (pLoop != nullptr) [[likely]] {
    pLoop->Exec(func);
  }

  return;
}

void LoopPool::Broadcast(const Loop::FunctionRun& func)
{
    std::for_each(m_vecLoops.begin(), m_vecLoops.end(), [func](Loop& loop) { loop.Exec(std::move(func)); });
}

asio::io_context& LoopPool::GetIoContext(std::size_t index) {
  auto pLoop = GetLoop(index);
  if (pLoop != nullptr) [[likely]] {
    return pLoop->GetIoContext();
  }

  {
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

SER_NAME_SPACE_END
