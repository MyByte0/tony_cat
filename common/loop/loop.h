#ifndef COMMON_LOOP_LOOP_H_
#define COMMON_LOOP_LOOP_H_

#include <functional>
#include <memory>

#include "asio/steady_timer.hpp"
#include "common/core_define.h"
#include "common/loop/thread_local.h"

namespace std {
class thread;
}
namespace asio {
class io_context;
}

TONY_CAT_SPACE_BEGIN

class Loop {
public:
    Loop();
    Loop(Loop&& loop) = default;
    ~Loop();

public:
    void Start();
    void Stop();
    void RunInThread();

    typedef std::function<void()> FunctionRun;
    typedef std::shared_ptr<asio::steady_timer> TimerHandle;

    void Exec(FunctionRun&& func);
    void Exec(const FunctionRun& func);
    TimerHandle ExecAfter(uint32_t millSeconds, FunctionRun&& func);
    TimerHandle ExecAfter(uint32_t millSeconds, const FunctionRun& func);
    TimerHandle ExecEvery(uint32_t millSeconds, FunctionRun&& func);
    TimerHandle ExecEvery(uint32_t millSeconds, const FunctionRun& func);
    static void Cancel(TimerHandle& timerHandle);

    asio::io_context& GetIoContext();

public:
    static Loop& GetCurrentThreadLoop();

private:
    void ExecEveryHelper(TimerHandle timerHandle, uint32_t millSeconds,
        FunctionRun&& func);

private:
    asio::io_context* m_pIoContext = nullptr;
    std::thread* m_pthread = nullptr;
    bool m_bRunning = false;

private:
    static THREAD_LOCAL_POD_VAR void* t_runingLoop;

private:
    friend class ModuleManager;

    void InitThreadlocalLoop();
};

TONY_CAT_SPACE_END

#endif // COMMON_LOOP_LOOP_H_