#include "Timer.h"

void Timer::SetTimer(unsigned char id, size_t milliseconds,  void* user_data)
{
    TimerInfo ti = {id};
    auto iter = timers_.find(ti);
    if (iter == timers_.end())
    {
        iter = timers_.insert(ti).first;
        iter->timer = boost::make_shared<boost::asio::deadline_timer>(io_service_);
    }

    iter->status = TimerInfo::TIMER_OK;
    iter->milliseconds = milliseconds;
    iter->user_data = (void*)user_data;

    start_timer(*iter);
}

void Timer::StopTimer(unsigned char id)
{
    TimerInfo ti = {id};

    auto iter = timers_.find(ti);
    if (iter != timers_.end())
    {
        stop_timer(*iter);
    }
}

void Timer::StopAllTimer()
{
    auto iter = timers_.begin();
    for (;iter != timers_.end(); ++iter)
    {
        stop_timer(*iter);
    }
}
