#pragma once

#include <functional>
#include <future>
#include <memory>
#include <mutex>

namespace DnsForwarder
{
class ThreadPool
{
  public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());
    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        (*task)();
        return res;
    }
    ~ThreadPool();

  private:
    // std::vector<std::thread> workers;
};
} // namespace DnsForwarder