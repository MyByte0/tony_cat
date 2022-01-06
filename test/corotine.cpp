#include <format>
#include <coroutine>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <functional>

using call_back = std::function<void(int)>;
void Add100ByCallback(int init, call_back f) // 异步调用
{
	std::thread t([init, f]() {
		std::this_thread::sleep_for(std::chrono::seconds(5));
		std::cout << std::format("Coroutine resumed {} on {}\n", __LINE__, __FUNCTION__);
		f(init + 100);
		});
	t.detach();
}

struct Add100AWaitable
{
	int init_;
	int result_;

	Add100AWaitable(int init) :init_(init) {}
	bool await_ready() const { return false; }
	int await_resume() { return result_; }
	void await_suspend(std::coroutine_handle<> handle)
	{
		//auto f = [handle, this](int value) mutable {
		//	result_ = value;
		//	handle.resume();
		//	std::cout << "Coroutine resumed on " << __LINE__ << " thread: " << std::this_thread::get_id() << "\n";
		//};
		//Add100ByCallback(init_, f); // 调用原来的异步调用

	/*	std::thread t([handle, this]() mutable {*/
			this->result_ = this->init_+1;
			handle.resume();
			handle.resume();
			std::cout << std::format("Coroutine resumed on {}\n", __LINE__, __FUNCTION__);
			//});
		//t.detach();
	}

};

struct CoroutineDem {
	struct promise_type {
		CoroutineDem get_return_object() {
			std::cout << std::format("Coroutine call on {}\n", __FUNCTION__); 
			return std::coroutine_handle<promise_type>::from_promise(*this); 
		}
		std::suspend_never initial_suspend() {
			std::cout << std::format("Coroutine call on {}\n", __FUNCTION__); 
			return {};
		}
		std::suspend_never final_suspend() noexcept {
			std::cout << std::format("Coroutine call on {}\n", __FUNCTION__);
			return {};
		}
		//void return_void() {
		//	std::cout << std::format("Coroutine call on {}\n", __FUNCTION__);
		//}
		void return_value(int) {
			std::cout << std::format("Coroutine call on {}\n", __FUNCTION__);
		}
		void unhandled_exception() {
			std::cout << std::format("Coroutine call on {}\n", __FUNCTION__);
		}
		std::suspend_always yield_value(int i) {
			std::cout << std::format("Coroutine call:{} on {}\n", i, __FUNCTION__);
			return {};
		}
	};

	CoroutineDem(std::coroutine_handle<promise_type> h) :handle(h) {}
	std::coroutine_handle<promise_type> handle;
};

CoroutineDem Add100ByCoroutine(int init, call_back f)
{
	int ret = co_await Add100AWaitable(init);
	ret = co_await Add100AWaitable(ret);
	ret = co_await Add100AWaitable(ret);
	co_yield 1;
	co_yield 2;
	f(ret);
	co_return 1;
}

void s() {
	Add100ByCoroutine(50, [](int value) {std::cout << std::format("get result from coroutine5: {}\n", value); });
}

int main()
{
	std::cout << std::format("Coroutine resumed on {} thread: {}\n", __LINE__, __FUNCTION__);
	auto corn = Add100ByCoroutine(10, [](int value) { std::cout << std::format("get result from coroutine1: {}\n" ,value); });
	corn.handle.resume();
	corn.handle.resume();
	//Add100ByCoroutine(20, [](int value) { std::cout << std::format("get result from coroutine2: {}\n" ,value); });
	//Add100ByCoroutine(30, [](int value) { std::cout << std::format("get result from coroutine3: {}\n" ,value); });
	//Add100ByCoroutine(40, [](int value) { std::cout << std::format("get result from coroutine4: {}\n" ,value); });
	//s();
	getchar();
}
