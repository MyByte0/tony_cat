#ifndef COMMON_LOOP_LOOPCOROUTINE_H_
#define COMMON_LOOP_LOOPCOROUTINE_H_

#include "common/core_define.h"

#include "common/loop/loop.h"

#include <coroutine>
#include <functional>

//#include <windows.h>

TONY_CAT_SPACE_BEGIN

// use co_await Awaitable
// need call function return Task with define promise_type
// run:
// Task::promise_type()         -->
// get_return_object()          -->
// Task::Task()                 -->
// initial_suspend()            -->
// if exception, go unhandled_exception(),
// go final_suspend()
// otherwise                    -->
// yield_value() if use         -->
// return_void()/return_value() -->
// final_suspend()              -->
// Task::~promise_type()        -->
// Task::~Task()                -->

template <class _TypeRet>
struct CoroutineTask {
    struct promise_type {
        CoroutineTask get_return_object()
        {
            return std::coroutine_handle<promise_type>::from_promise(*this);
        }
        std::suspend_never initial_suspend() noexcept
        {
            return {};
        }
        void return_value(_TypeRet&) noexcept
        {
        }
        std::suspend_never final_suspend() noexcept
        {
            return {};
        }
        void unhandled_exception() { }
    };

    CoroutineTask(std::coroutine_handle<promise_type> h) { }
    ~CoroutineTask() { }
};

template <>
struct CoroutineTask<void> {
    struct promise_type {
        CoroutineTask get_return_object()
        {
            return std::coroutine_handle<promise_type>::from_promise(*this);
        }
        std::suspend_never initial_suspend() noexcept
        {
            return {};
        }
        void return_void() noexcept
        {
        }
        std::suspend_never final_suspend() noexcept
        {
            return {};
        }
        void unhandled_exception() { }
    };

    CoroutineTask(std::coroutine_handle<promise_type> h) { }
    ~CoroutineTask() { }
};

struct CoroutineAsyncTask {
    struct promise_type {
        CoroutineAsyncTask get_return_object()
        {
            return CoroutineAsyncTask();
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        void return_void() noexcept { }
        std::suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() { }
    };
};

struct AwaitableAsync {
    typedef int RetType;
    int m_nInit;
    RetType m_result;

    // call:
    // --> Awaitable::Awaitable()
    // --> await_ready()
    // --> await_suspend() if await_ready()==false
    // --> await_resume(),and co_await get result from await_resume()
    // --> Awaitable::~Awaitable()
    AwaitableAsync(int init)
        : m_nInit(init)
        , m_result()
    {
    }

    // if await_ready return true, await_suspend not be called.
    bool await_ready() const { return false; }

    void await_suspend(std::coroutine_handle<> handle)
    {
        handle.resume();
    }

    RetType await_resume() { return m_result; }
};

struct AwaitableGetCoroutineHandle {
    typedef std::coroutine_handle<> RetType;
    RetType m_result;
    AwaitableGetCoroutineHandle() { }

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle)
    {
        m_result = handle;
        m_result.resume();
    }

    RetType await_resume() { return m_result; }
};

struct AwaitableLoopSwitch {
    AwaitableLoopSwitch(Loop& loop)
        : m_loop(loop)
    {
    }

    bool await_ready() const { return false; }
    void await_suspend(std::coroutine_handle<> handle)
    {
        m_loop.Exec([handle]() {
            handle.resume();
        });
    }

    void await_resume() { return; }

private:
    Loop& m_loop;
};

struct AwaitableExecAfter {
    AwaitableExecAfter(uint32_t millSeconds)
        : m_nWaitMillSecond(millSeconds)
    {
    }
    bool await_ready() const { return m_nWaitMillSecond == 0; }
    auto await_resume() { return; }
    void await_suspend(std::coroutine_handle<> handle)
    {
        Loop::GetCurrentThreadLoop().ExecAfter(m_nWaitMillSecond, [handle]() {
            handle.resume();
        });
    }

private:
    uint32_t m_nWaitMillSecond;
};

struct AwaitableExecAfterOnLoop {
    AwaitableExecAfterOnLoop(Loop& loop, uint32_t millSeconds)
        : m_loop(loop)
        , m_nWaitMillSecond(millSeconds)
    {
    }
    bool await_ready() const { return m_nWaitMillSecond != 0; }
    auto await_resume() { return; }
    void await_suspend(std::coroutine_handle<> handle)
    {
        m_loop.ExecAfter(m_nWaitMillSecond, [handle]() {
            handle.resume();
        });
    }

private:
    uint32_t m_nWaitMillSecond;
    Loop& m_loop;
};

struct AwaitableAsyncFunction {
    typedef std::function<CoroutineAsyncTask(std::coroutine_handle<>)> _TFunction;
    _TFunction func_;
    CoroutineAsyncTask m_result;

    AwaitableAsyncFunction(_TFunction&& func)
        : func_(std::move(func))
    {
    }
    AwaitableAsyncFunction(const _TFunction& func)
        : func_(func)
    {
    }

    bool await_ready() const { return false; }

    void await_suspend(std::coroutine_handle<> handle)
    {
        m_result = func_(handle); // handle.resume() called by func_
    }

    CoroutineAsyncTask await_resume() { return m_result; }
};

struct AwaitableSyncCall {
    struct SyncCallCoroutineHandle { // could use unique_ptr and set deleter
        SyncCallCoroutineHandle(std::coroutine_handle<> handle)
            : handle_(handle)
        {
        }
        SyncCallCoroutineHandle(const SyncCallCoroutineHandle& rValue)
        {
            if (handle_) {
                handle_.resume();
            }

            handle_ = rValue.handle_;
            rValue.handle_ = nullptr;
        }
        SyncCallCoroutineHandle(SyncCallCoroutineHandle&& rValue)
        {
            if (handle_) {
                handle_.resume();
            }

            handle_ = rValue.handle_;
            rValue.handle_ = nullptr;
        }

        SyncCallCoroutineHandle& operator=(const SyncCallCoroutineHandle& rValue)
        {
            if (handle_) {
                handle_.resume();
            }

            handle_ = rValue.handle_;
            rValue.handle_ = nullptr;
            return *this;
        }

        SyncCallCoroutineHandle& operator=(SyncCallCoroutineHandle&& rValue)
        {
            if (handle_) {
                handle_.resume();
            }

            handle_ = rValue.handle_;
            rValue.handle_ = nullptr;
            return *this;
        }

        ~SyncCallCoroutineHandle()
        {
            if (handle_) {
                handle_.resume();
            }
        }

    private:
        mutable std::coroutine_handle<> handle_;
    };

    typedef std::function<CoroutineAsyncTask(SyncCallCoroutineHandle)> _TFunction;
    _TFunction func_;
    CoroutineAsyncTask m_result;
    std::coroutine_handle<> handle_;

    AwaitableSyncCall(_TFunction&& func)
        : func_(std::move(func))
    {
    }
    AwaitableSyncCall(const _TFunction& func)
        : func_(func)
    {
    }

    bool await_ready() const { return false; }

    void await_suspend(std::coroutine_handle<> handle)
    {
        handle_ = handle;
        m_result = func_(SyncCallCoroutineHandle(handle)); // handle.resume() on func_ return
    }

    CoroutineAsyncTask await_resume()
    {
        return m_result;
    }
};

TONY_CAT_SPACE_END

#endif // COMMON_LOOP_LOOPCOROUTINE_H_
