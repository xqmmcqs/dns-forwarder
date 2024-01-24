#include "timer.h"

#include <cstring>
#include <mutex>

#include "utils.h"

using namespace std;

void DnsForwarder::TimerHeap::Push(shared_ptr<Timer> timer)
{
    {
        unique_lock<shared_mutex> lock(m_mutex);
        m_queue.push(timer);
    }
    if (Size() == 1)
    {
        auto now = chrono::steady_clock::now();
        itimerval itimer;
        memset(&itimer, 0, sizeof(itimer));
        itimer.it_value.tv_usec = chrono::duration_cast<chrono::microseconds>(timer->expire - now).count() % 1000000;
        itimer.it_value.tv_sec = chrono::duration_cast<chrono::seconds>(timer->expire - now).count();
        Wrapper::SetITimer(ITIMER_REAL, &itimer, nullptr);
    }
}

shared_ptr<DnsForwarder::Timer> DnsForwarder::TimerHeap::Top()
{
    shared_lock<shared_mutex> lock(m_mutex);
    return m_queue.top();
}

shared_ptr<DnsForwarder::Timer> DnsForwarder::TimerHeap::Pop()
{
    unique_lock<shared_mutex> lock(m_mutex);
    auto timer = m_queue.top();
    m_queue.pop();
    return timer;
}

void DnsForwarder::TimerHeap::Tick()
{
    if (!Empty())
    {
        auto top_timer = Top();
        auto now = chrono::steady_clock::now();
        itimerval itimer;
        memset(&itimer, 0, sizeof(itimer));
        itimer.it_value.tv_usec =
            chrono::duration_cast<chrono::microseconds>(top_timer->expire - now).count() % 1000000;
        itimer.it_value.tv_sec = chrono::duration_cast<chrono::seconds>(top_timer->expire - now).count();
        Wrapper::SetITimer(ITIMER_REAL, &itimer, nullptr);
    }
    else
    {
        itimerval itimer;
        memset(&itimer, 0, sizeof(itimer));
        Wrapper::SetITimer(ITIMER_REAL, &itimer, nullptr);
    }
}

bool DnsForwarder::TimerHeap::Empty()
{
    shared_lock<shared_mutex> lock(m_mutex);
    return m_queue.empty();
}

std::size_t DnsForwarder::TimerHeap::Size()
{
    shared_lock<shared_mutex> lock(m_mutex);
    return m_queue.size();
}
