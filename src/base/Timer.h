#ifndef PUSH_TIMER_H
#define PUSH_TIMER_H

#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/container/set.hpp>

class Timer
{
protected:
    struct TimerInfo
    {
        enum TimerStatus {TIMER_OK, TIMER_CANCELED};

        unsigned char id;
        TimerStatus status;
        size_t milliseconds;
        void* user_data;
        boost::shared_ptr<boost::asio::deadline_timer> timer;

        bool operator <(const TimerInfo& other) const {return id < other.id;}
    };

    explicit Timer(boost::asio::io_service& io_service) : io_service_(io_service){}

    virtual ~Timer() {}

public:
    void SetTimer(unsigned char id, size_t milliseconds,  void* user_data);

    void StopTimer(unsigned char id);

    void StopAllTimer();

    virtual bool OnTimer(unsigned char id,  void* user_data) {return false;}

protected:

    void start_timer(TimerInfo& ti)
    {
        ti.timer->expires_from_now(boost::posix_time::milliseconds(ti.milliseconds));
        ti.timer->async_wait(boost::bind(&Timer::timer_handler, this,
                                         boost::asio::placeholders::error, boost::ref(ti)));
    }

    void stop_timer(TimerInfo& ti)
    {
        boost::system::error_code ec;
        ti.timer->cancel(ec);
        ti.status = TimerInfo::TIMER_CANCELED;
    }

    void timer_handler(const boost::system::error_code& ec, TimerInfo& ti)
    {
        if (!ec && OnTimer(ti.id, ti.user_data) && TimerInfo::TIMER_OK == ti.status)
        {
            start_timer(ti);
        }
    }

    boost::asio::io_service& io_service_;
    boost::container::set<TimerInfo> timers_;
};

#endif // TIMER_H
