#pragma once

#include <time.h>
#include <stdint.h>

// time definitions
#define HOURS_IN_DAY                    24
#define MINUTES_IN_HOUR                 60
#define SECONDS_IN_MINUTE               60
#define SECONDS_IN_HOUR                 (SECONDS_IN_MINUTE * MINUTES_IN_HOUR)
#define MINUTES_IN_DAY                  (HOURS_IN_DAY * MINUTES_IN_HOUR)
#define SECONDS_IN_DAY                  (MINUTES_IN_DAY * SECONDS_IN_MINUTE)
#define WEEK_DAYS_NUM                   7

#define NANOSECONDS_IN_SECOND_NUM       1000000000     // billion
#define NANOSECONDS_IN_MILLISECOND_NUM  1000000        // million
#define MILLISECONDS_IN_SECOND_NUM      1000           // thousand

// math definitions
#define ROUND_UP(value, mod)            ((((value) - 1) / (mod) + 1) * (mod))
#define ROUND_DOWN(value, mod)          (((value) / (mod)) * (mod))

extern "C" {
/**
 * @brief Adds milliseconds to given time spec
 * @param ts Time spec structure to be modified
 * @param ms_num Number of milliseconds
 */
inline struct timespec AddMillisecondsToCurrentTime(uint32_t ms_num)
{
   struct timespec ts = {0, 0};
   clock_gettime(CLOCK_REALTIME, &ts);
   ts.tv_nsec += ((long int) ms_num % MILLISECONDS_IN_SECOND_NUM) * NANOSECONDS_IN_MILLISECOND_NUM;
   ts.tv_sec += (__time_t) ms_num / MILLISECONDS_IN_SECOND_NUM + ts.tv_nsec / NANOSECONDS_IN_SECOND_NUM;
   ts.tv_nsec %= NANOSECONDS_IN_SECOND_NUM;
   return ts;
}
}

#include <functional>

/**
   * @class Timer system_utils.h
   * @brief It is a wrapper around Linux system timer functions.
   */
class Timer
{
public:
   Timer();
   /**
      * @param callback Function to be called after timer expiration.
      */
   Timer(std::function<void()> callback);
   Timer(const Timer& other) = delete;
   Timer(Timer&& other) = delete;
   ~Timer();

   void SetCallback(std::function<void()> callback) {
      callback_ = std::move(callback);
   }
   /**
      * @brief Arms the timer.
      * @param initial_expiration_ms Amount of time in milliseconds until the first timer expiration.
      * @param period_ms If not 0, timer will be automatically restarted with given period.
      * @return true on success, else false.
      * @note If the timer was already armed, then the previous settings are overwritten.
      */
   bool Start(uint32_t initial_expiration_ms, uint32_t period_ms = 0);
   /**
      * @brief Stops timer.
      * @return true on success, else false.
      */
   bool Stop();

private:
   std::function<void()> callback_;
   timer_t timer_id_;
};
