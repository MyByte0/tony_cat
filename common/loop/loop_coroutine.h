#ifndef COMMON_LOOP_LOOP_COROUTINE_H_
#define COMMON_LOOP_LOOP_COROUTINE_H_

#include <coroutine>
#include <functional>

#include "common/core_define.h"
#include "common/loop/loop.h"

// #include <windows.h>

TONY_CAT_SPACE_BEGIN

// use co_await Awaitable / promise.await_transform / operator co_await
// need call function return Task with define promise_type
// run:
// Task::promise_type()         -->
// get_return_object()          -->
// Task::Task()                 -->
// initial_suspend()            -->
// if exception, go unhandled_exception(),
// go final_suspend()
// otherwise                    -->
// yield_value() if use co_yield-->
// return_void()/return_value() -->
// final_suspend()              -->
// Task::~promise_type()        -->
// Task::~Task()                -->

// awaiter��
// if (!awaiter::await_ready) {
//  // note: if await_suspend return bool false, continue suspend.
//   awaiter::await_suspend();
// }
// return awaiter::await_resume();

// for normal CoroutineFunction return type
template <class _TypeRet>
struct CoroutineTask {
    struct promise_type {
        CoroutineTask get_return_object() {
            return std::coroutine_handle<promise_type>::from_promise(*this);
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        void return_value(_TypeRet&) noexcept {}
        std::suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() {}
    };

    CoroutineTask(std::coroutine_handle<promise_type> h) {}
    ~CoroutineTask() {}
};

template <>
struct CoroutineTask<void> {
    struct promise_type {
        CoroutineTask get_return_object() {
            return std::coroutine_handle<promise_type>::from_promise(*this);
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        std::suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() {}
    };

    CoroutineTask(std::coroutine_handle<promise_type> h) {}
    ~CoroutineTask() {}
};

// use for: call CoroutineAsyncTask<TypeRet> func(...)
// and co_await func(...);
template <class _TypeRet>
struct CoroutineAsyncTask {
    struct promise_type {
        auto get_return_object() {
            return CoroutineAsyncTask(
                std::coroutine_handle<promise_type>::from_promise(*this));
        }
        std::suspend_always initial_suspend() { return {}; }
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            void await_resume() noexcept {}
            std::coroutine_handle<> await_suspend(
                std::coroutine_handle<promise_type> h) noexcept {
                if (auto previous = h.promise().previous; previous) {
                    return previous;
                } else {
                    return std::noop_coroutine();
                }
            }
        };
        final_awaiter final_suspend() noexcept { return {}; }
        void unhandled_exception() { throw; }
        void return_value(_TypeRet value) { result = std::move(value); }
        _TypeRet result;
        std::coroutine_handle<> previous;
    };

    CoroutineAsyncTask(std::coroutine_handle<promise_type> h) : coro(h) {}
    CoroutineAsyncTask(CoroutineAsyncTask&& t) = delete;
    ~CoroutineAsyncTask() { coro.destroy(); }

    struct awaiter {
        bool await_ready() { return false; }
        _TypeRet await_resume() { return std::move(coro.promise().result); }
        auto await_suspend(std::coroutine_handle<> h) {
            coro.promise().previous = h;
            return coro;
        }
        std::coroutine_handle<promise_type> coro;
    };

    awaiter operator co_await() { return awaiter{coro}; }
    _TypeRet operator()() {
        coro.resume();
        return std::move(coro.promise().result);
    }

private:
    std::coroutine_handle<promise_type> coro;
};

template <>
struct CoroutineAsyncTask<void> {
    struct promise_type {
        auto get_return_object() {
            return CoroutineAsyncTask(
                std::coroutine_handle<promise_type>::from_promise(*this));
        }
        std::suspend_always initial_suspend() { return {}; }
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            void await_resume() noexcept {}
            std::coroutine_handle<> await_suspend(
                std::coroutine_handle<promise_type> h) noexcept {
                if (auto previous = h.promise().previous; previous) {
                    return previous;
                } else {
                    return std::noop_coroutine();
                }
            }
        };
        final_awaiter final_suspend() noexcept { return {}; }
        void unhandled_exception() { throw; }
        void return_void() {}
        std::coroutine_handle<> previous;
    };

    CoroutineAsyncTask(std::coroutine_handle<promise_type> h) : coro(h) {}
    CoroutineAsyncTask(CoroutineAsyncTask&& t) = delete;
    ~CoroutineAsyncTask() { coro.destroy(); }

    struct awaiter {
        bool await_ready() { return false; }
        void await_resume() { return; }
        auto await_suspend(std::coroutine_handle<> h) {
            coro.promise().previous = h;
            return coro;
        }
        std::coroutine_handle<promise_type> coro;
    };

    awaiter operator co_await() { return awaiter{coro}; }
    void operator()() {
        coro.resume();
        return;
    }

private:
    std::coroutine_handle<promise_type> coro;
};

// use for: co_await switch loop
struct AwaitableLoopSwitch {
    AwaitableLoopSwitch(Loop& loop) : m_loop(loop) {}

    bool await_ready() const { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        m_loop.Exec([handle]() { handle.resume(); });
    }

    void await_resume() { return; }

private:
    Loop& m_loop;
};

// use for: co_await timer
struct AwaitableExecAfter {
    AwaitableExecAfter(uint32_t millSeconds) : m_nWaitMillSecond(millSeconds) {}
    bool await_ready() const { return m_nWaitMillSecond == 0; }
    auto await_resume() { return; }
    void await_suspend(std::coroutine_handle<> handle) {
        Loop::GetCurrentThreadLoop().ExecAfter(m_nWaitMillSecond,
                                               [handle]() { handle.resume(); });
    }

private:
    uint32_t m_nWaitMillSecond;
};

struct AwaitableExecAfterOnLoop {
    AwaitableExecAfterOnLoop(Loop& loop, uint32_t millSeconds)
        : m_loop(loop), m_nWaitMillSecond(millSeconds) {}
    bool await_ready() const { return m_nWaitMillSecond == 0; }
    auto await_resume() { return; }
    void await_suspend(std::coroutine_handle<> handle) {
        m_loop.ExecAfter(m_nWaitMillSecond, [handle]() { handle.resume(); });
    }

private:
    Loop& m_loop;
    uint32_t m_nWaitMillSecond;
};

TONY_CAT_SPACE_END

#endif  // COMMON_LOOP_LOOP_COROUTINE_H_
