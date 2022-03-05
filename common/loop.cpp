#include "loop.h"

#include <cassert>
#include <thread>
#include <utility>

#include <asio.hpp>

TONY_CAT_SPACE_BEGIN

THREAD_LOCAL_POD_VAR void* Loop::t_runingLoop = nullptr;

Loop::Loop()
    : m_pIoContext(new asio::io_context())
{
}

Loop::~Loop()
{
    Stop();

    delete m_pIoContext;
    m_pIoContext = nullptr;
}

Loop& Loop::GetCurrentThreadLoop()
{
    assert(t_runingLoop != nullptr);
    return *(static_cast<Loop*>(t_runingLoop));
}

void Loop::RunInThread()
{
    m_bRunning = true;
    asio::io_context::work worker(*m_pIoContext);
    t_runingLoop = this;
    m_pIoContext->run();
}

void Loop::Start()
{
    if (true == m_bRunning) {
        return;
    }

    m_pthread = new std::thread([this]() { RunInThread(); });
    m_bRunning = true;
}

void Loop::Stop()
{
    if (false == m_bRunning) {
        return;
    }

    m_pIoContext->stop();

    if (m_pthread != nullptr) {
        m_pthread->join();
        delete m_pthread;
        m_pthread = nullptr;
    }

    m_bRunning = false;
}

asio::io_context& Loop::GetIoContext() { return *m_pIoContext; }

void Loop::Exec(FunctionRun&& func) { m_pIoContext->post(std::move(func)); }

void Loop::Exec(const FunctionRun& func) { m_pIoContext->post(func); }

Loop::TimerHandle Loop::ExecAfter(uint32_t millSeconds, FunctionRun&& func)
{
    auto pTimer = std::make_shared<asio::steady_timer>(GetIoContext());
    pTimer->expires_after(asio::chrono::milliseconds(millSeconds));
    pTimer->async_wait([this, pTimer, func = std::move(func)](
                           const std::error_code& ec) { func(); });

    return pTimer;
}

Loop::TimerHandle Loop::ExecAfter(uint32_t millSeconds,
    const FunctionRun& func)
{
    auto pTimer = std::make_shared<asio::steady_timer>(GetIoContext());
    pTimer->expires_after(asio::chrono::milliseconds(millSeconds));
    pTimer->async_wait(
        [this, pTimer, func](const std::error_code& ec) { func(); });

    return pTimer;
}

void Loop::ExecEveryHelper(TimerHandle timerHandle, uint32_t millSeconds,
    FunctionRun&& func)
{
    timerHandle->expires_after(asio::chrono::milliseconds(millSeconds));
    timerHandle->async_wait([this, timerHandle, millSeconds,
                                funcall = std::move(func)](auto... args) mutable {
        funcall();
        ExecEveryHelper(timerHandle, millSeconds, std::move(funcall));
    });
}

Loop::TimerHandle Loop::ExecEvery(uint32_t millSeconds, FunctionRun&& func)
{
    auto pTimer = std::make_shared<asio::steady_timer>(GetIoContext());
    ExecEveryHelper(pTimer, millSeconds, std::move(func));

    return pTimer;
}

Loop::TimerHandle Loop::ExecEvery(uint32_t millSeconds,
    const FunctionRun& func)
{
    auto pTimer = std::make_shared<asio::steady_timer>(GetIoContext());

    ExecEveryHelper(pTimer, millSeconds, FunctionRun(func));

    return pTimer;
}

void Loop::Cancel(TimerHandle& timerHandle) { timerHandle->cancel(); }

TONY_CAT_SPACE_END
