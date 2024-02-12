#include "Timer.h"

#include <signal.h>
#include <cstring>
#include <iostream>

using namespace std;


Timer::Timer() : timer_id_(nullptr)
{
   sigevent timeout_event;
   memset(&timeout_event, 0, sizeof(timeout_event));
   timeout_event.sigev_notify = SIGEV_THREAD;
   timeout_event.sigev_value.sival_ptr = static_cast<void*>(this);
   timeout_event._sigev_un._sigev_thread._function = [] (sigval_t sigval) {
      reinterpret_cast<Timer*>(sigval.sival_ptr)->callback_();
   };

   if (timer_create(CLOCK_REALTIME, &timeout_event, &timer_id_) != 0)
   {
      timer_id_ = nullptr;
      cerr << "Create timer failed" << endl;
   }
}


Timer::Timer(std::function<void()> callback) : Timer()
{
   callback_ = std::move(callback);
}


Timer::~Timer()
{
   if (timer_id_ != nullptr)
   {
      timer_delete(timer_id_);
   }
}


bool Timer::Start(uint32_t initial_expiration_ms, uint32_t period_ms)
{
   if (timer_id_ == nullptr) return false;
   if (!callback_) return false;

   itimerspec timer_spec = { {0, 0}, {0, 0} };
   timer_spec.it_value.tv_sec = initial_expiration_ms / MILLISECONDS_IN_SECOND_NUM;
   timer_spec.it_value.tv_nsec = (initial_expiration_ms % MILLISECONDS_IN_SECOND_NUM) * NANOSECONDS_IN_MILLISECOND_NUM;
   if (period_ms > 0)
   {
      timer_spec.it_interval.tv_sec = period_ms / MILLISECONDS_IN_SECOND_NUM;
      timer_spec.it_interval.tv_nsec = (period_ms % MILLISECONDS_IN_SECOND_NUM) * NANOSECONDS_IN_MILLISECOND_NUM;
   }

   if (timer_settime(timer_id_, 0, &timer_spec, nullptr) != 0)
   {
      cerr << "Start timer failed" << endl;
      return false;
   }
   return true;
}


bool Timer::Stop()
{
   return Start(0);
}
