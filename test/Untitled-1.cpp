#include <iostream>
#include <chrono>
#include <coroutine>
#include <thread>
#include <functional>

using namespace std::literals;

using Callback = std::function<void(int)>;

// 异步执行（模拟耗时的计算）
void asyncCompute(int v, Callback cb)
{
    std::thread t([v, cb]()
        {
            std::this_thread::sleep_for(10ms);
            int result = v + 100;
            cb(result);
        });

    t.detach();
}

// 协程的返回值类型
struct Task
{
    struct promise_type;
    using handle_t = std::coroutine_handle<promise_type>;

    ~Task()
    {
        if (m_handle)
        {
            m_handle.destroy();
        }
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& task) noexcept
        : m_handle(std::move(task.m_handle))
    {
        task.m_handle = handle_t::from_address(nullptr);
    }

    Task& operator=(Task&& task) noexcept
    {
        m_handle = std::move(task.m_handle);
        task.m_handle = handle_t::from_address(nullptr);
    }

private:
    Task(handle_t h): m_handle(h)
    {}

public:
    struct promise_type
    {
        Task get_return_object()
        {
            return Task(handle_t::from_promise(*this));
        }

        std::suspend_never initial_suspend()
        {
            return std::suspend_never{};
        }

        std::suspend_never final_suspend() noexcept
        {
            return std::suspend_never{};
        }

        void return_void()
        {}

        void unhandled_exception()
        {
            std::cout << "unhandled exception\n";
        }
    };

private:
    handle_t m_handle;
};

// co_await 操作数的类型
class ComputeAwaitable
{
public:
    ComputeAwaitable(int initValue)
        : m_init(initValue), m_result(0)
    {}

    bool await_ready()
    {
        return false;
    }

    // 调用异步函数
    void await_suspend(std::coroutine_handle<> handle)
    {
        auto cb = [handle, this](int value) mutable
        {
            m_result = value;
            handle.resume();
        };

        asyncCompute(m_init, cb);
    }

    int await_resume()
    {
        return m_result;
    }

private:
    int m_init;
    int m_result;
};

Task computeByCoroutine(int v)
{
    int ret = co_await ComputeAwaitable(v);
    ret = co_await ComputeAwaitable(ret);
    ret = co_await ComputeAwaitable(ret);

    std::cout << ret << '\n';

    co_return;
}

int main()
{
    Task task(computeByCoroutine(200));

    std::this_thread::sleep_for(3s);
}