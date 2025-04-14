#pragma once

#include "net_callback.h"
#include <atomic>
#include <cstdint>

namespace w_network {

    class Timer {
     public:
      Timer(const TimerCallback& cb, Timestamp when, int64_t interval, int64_t repeat_count = -1);
      Timer(TimerCallback&& cb, Timestamp when, int64_t interval);
    
      void run();
    
      bool is_canceled() const { return canceled_; }
      void cancel(bool off) { canceled_ = off; }
    
      Timestamp expiration() const { return expiration_; }
      int64_t get_repeat_count() const { return repeat_count_; }
      int64_t sequence() const { return sequence_; }
    
      static int64_t num_created() { return created_num_; }
    
     private:
      Timer(const Timer& rhs) = delete;
      Timer& operator=(const Timer& rhs) = delete;
    
      const TimerCallback callback_;
      Timestamp expiration_;
      const int64_t interval_;
      int64_t repeat_count_;  // -1 means infinite repeat
      const int64_t sequence_;
      bool canceled_;
    
      static std::atomic<int64_t> created_num_;
    };
    
}  // namespace net