#pragma once

#include <chrono>
#include <memory>
#include <queue>
#include <shared_mutex>

namespace DnsForwarder
{
class Timer
{
  public:
    Timer() = delete;
    Timer(std::chrono::milliseconds duration, uint16_t index)
        : expire(std::chrono::steady_clock::now() + duration), index(index), valid(true)
    {
    }
    ~Timer() = default;
    Timer(const Timer &) = delete;
    Timer &operator=(const Timer &) = delete;

    std::chrono::time_point<std::chrono::steady_clock> expire;
    uint16_t index;
    bool valid;
};

class TimerHeap
{
  public:
    TimerHeap() = default;
    ~TimerHeap() = default;
    TimerHeap(const TimerHeap &) = delete;
    TimerHeap &operator=(const TimerHeap &) = delete;

    void Push(std::shared_ptr<Timer> timer);

    std::shared_ptr<Timer> Top();

    std::shared_ptr<Timer> Pop();

    void Tick();

    bool Empty();

    std::size_t Size();

  private:
    class TimerCompare
    {
      public:
        bool operator()(const std::shared_ptr<Timer> &lhs, const std::shared_ptr<Timer> &rhs) const
        {
            return lhs->expire > rhs->expire;
        }
    };
    std::priority_queue<std::shared_ptr<Timer>, std::vector<std::shared_ptr<Timer>>, TimerCompare> m_queue;
    std::shared_mutex m_mutex;
};
} // namespace DnsForwarder