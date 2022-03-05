#ifndef COMMON_LOOP_COROUTINE_H_
#define COMMON_LOOP_COROUTINE_H_

#include "core_define.h"

#include "loop.h"

#include <coroutine>
#include <functional>

//#include <windows.h>

TONY_CAT_SPACE_BEGIN

// use co_await Waitable
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
struct Task {
    struct promise_type {
        Task get_return_object()
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

    Task(std::coroutine_handle<promise_type> h) { }
    ~Task() { }
};

template <>
struct Task<void> {
    struct promise_type {
        Task get_return_object()
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

    Task(std::coroutine_handle<promise_type> h) { }
    ~Task() { }
};

struct AsyncTask {
    struct promise_type {
        AsyncTask get_return_object()
        {
            return AsyncTask();
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        void return_void() noexcept { }
        std::suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() { }
    };
};

struct AsyncWaitable {
    typedef int RetType;
    int init_;
    RetType result_;

    // call:
    // --> Waitable::Waitable()
    // --> await_ready()
    // --> await_suspend() if await_ready()==false
    // --> await_resume(),and co_await get result from await_resume()
    // --> Waitable::~Waitable()
    AsyncWaitable(int init)
        : init_(init)
        , result_()
    {
    }

    // if await_ready return true, await_suspend not be called.
    bool await_ready() const { return false; }

    void await_suspend(std::coroutine_handle<> handle)
    {
        handle.resume();
    }

    RetType await_resume() { return result_; }
};

struct GetCoroutineHandleWaitable {
    typedef std::coroutine_handle<> RetType;
    RetType result_;
    GetCoroutineHandleWaitable() { }

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle)
    {
        result_ = handle;
        result_.resume();
    }

    RetType await_resume() { return result_; }
};

struct LoopSwitchWaitable {
    Loop& loop_;

    LoopSwitchWaitable(Loop& loop)
        : loop_(loop)
    {
    }

    // if await_ready return true, await_suspend not be called.
    bool await_ready() const { return false; }

    void await_suspend(std::coroutine_handle<> handle)
    {
        loop_.Exec([handle]() {
            handle.resume();
        });
    }

    void await_resume() { return; }
};

struct AsyncFunctionWaitable {
    typedef std::function<AsyncTask(std::coroutine_handle<>)> _TFunction;
    _TFunction func_;
    AsyncTask result_;

    AsyncFunctionWaitable(_TFunction&& func)
        : func_(std::move(func))
    {
    }
    AsyncFunctionWaitable(const _TFunction& func)
        : func_(func)
    {
    }

    bool await_ready() const { return false; }

    void await_suspend(std::coroutine_handle<> handle)
    {
        result_ = func_(handle); // handle.resume() need called by func_
    }

    AsyncTask await_resume() { return result_; }
};

struct SyncFunctionWaitable {
    struct SyncFunctionCoroutineHandle { // could use unique_ptr and set deleter
        SyncFunctionCoroutineHandle(std::coroutine_handle<> handle)
            : handle_(handle)
        {
        }
        SyncFunctionCoroutineHandle(const SyncFunctionCoroutineHandle& rValue)
        {
            if (handle_) {
                handle_.resume();
            }

            handle_ = rValue.handle_;
            rValue.handle_ = nullptr;
        }
        SyncFunctionCoroutineHandle(SyncFunctionCoroutineHandle&& rValue)
        {
            if (handle_) {
                handle_.resume();
            }

            handle_ = rValue.handle_;
            rValue.handle_ = nullptr;
        }

        SyncFunctionCoroutineHandle& operator=(const SyncFunctionCoroutineHandle& rValue)
        {
            if (handle_) {
                handle_.resume();
            }

            handle_ = rValue.handle_;
            rValue.handle_ = nullptr;
            return *this;
        }

        SyncFunctionCoroutineHandle& operator=(SyncFunctionCoroutineHandle&& rValue)
        {
            if (handle_) {
                handle_.resume();
            }

            handle_ = rValue.handle_;
            rValue.handle_ = nullptr;
            return *this;
        }

        ~SyncFunctionCoroutineHandle()
        {
            if (handle_) {
                handle_.resume();
            }
        }

    private:
        mutable std::coroutine_handle<> handle_;
    };

    typedef std::function<AsyncTask(SyncFunctionCoroutineHandle)> _TFunction;
    _TFunction func_;
    AsyncTask result_;
    std::coroutine_handle<> handle_;

    SyncFunctionWaitable(_TFunction&& func)
        : func_(std::move(func))
    {
    }
    SyncFunctionWaitable(const _TFunction& func)
        : func_(func)
    {
    }

    bool await_ready() const { return false; }

    void await_suspend(std::coroutine_handle<> handle)
    {
        handle_ = handle;
        result_ = func_(SyncFunctionCoroutineHandle(handle)); // handle.resume() on func_ return
    }

    AsyncTask await_resume()
    {
        return result_;
    }
};

TONY_CAT_SPACE_END

#endif // COMMON_LOOP_COROUTINE_H_
