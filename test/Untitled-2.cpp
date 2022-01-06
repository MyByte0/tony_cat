#include <iostream>
#include <coroutine>

struct Generator
{
    struct promise_type;
    using handle_t = std::coroutine_handle<promise_type>;

    struct promise_type
    {
        int value;

        auto get_return_object()
        {
            return Generator(handle_t::from_promise(*this));
        }

        auto initial_suspend()
        {
            return std::suspend_always();
        }

        auto final_suspend() noexcept
        {
            return std::suspend_always();
        }

        void return_void()
        {}

        auto yield_value(int v)
        {
            value = v;
            return std::suspend_always();
        }

        void unhandled_exception()
        {
            std::terminate();
        }
    };

    bool next()
    {
        if (m_handle)
        {
            m_handle.resume();
            return !m_handle.done();
        }

        return false;
    }

    int value() const
    {
        return m_handle.promise().value;
    }

    ~Generator()
    {
        if (m_handle)
        {
            m_handle.destroy();
        }
    }

private:
    Generator(handle_t h)
        : m_handle(h)
    {}

private:
    handle_t m_handle;
};

Generator f(int n)
{
    int value = 1;

    while(value <= n)
    {
        co_yield value++;
    }
}


int main()
{
    Generator g(f(10));

    while (g.next())
    {
        std::cout << g.value() << ' ';
    }
}