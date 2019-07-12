#ifndef DRIVER_UTIL_WAIT_QUEUE_H_
#define DRIVER_UTIL_WAIT_QUEUE_H_

#include <pthread.h>
#include <deque>

#include "./mutex_lock.h"

namespace driver_util {

template <class T>
class WaitQueue {
 public:
  typedef std::deque<T> container_type;
  typedef typename container_type::value_type value_type;
  typedef typename container_type::size_type size_type;

  WaitQueue() {
    pthread_mutex_init(&lock_, nullptr);
    pthread_cond_init(&data_ready_, NULL);
  }

  ~WaitQueue() {
    pthread_mutex_destroy(&lock_);
    pthread_cond_destroy(&data_ready_);
  }

  size_type size() const {
    MutexLock l(&lock_);
    return queue_.size();
  }

  bool empty() const {
    MutexLock l(&lock_);
    return queue_.empty();
  }

  void clear() {
    MutexLock l(&lock_);
    return queue_.clear();
  }

  void pushBack(const value_type& x) {
    MutexLock l(&lock_);
    if (queue_.empty())
      pthread_cond_signal(&data_ready_);
    queue_.push_back(x);
  }

  bool wait(value_type* p) {
    MutexLock l(&lock_);
    while (queue_.empty()) {
      if (stop_requested_)
        return false;
      pthread_cond_wait(&data_ready_, &lock_);
    }
    *p = queue_.front();
    queue_.pop_front();
    return true;
  }

  void stopWaiters() {
    MutexLock l(&lock_);
    stop_requested_ = true;
    pthread_cond_signal(&data_ready_);
  }

  void reset() {
    MutexLock l(&lock_);
    stop_requested_ = false;
    queue_.clear();
  }

 private:
  mutable pthread_mutex_t lock_;
  container_type queue_;
  pthread_cond_t data_ready_;
  bool stop_requested_ = false;
};

}  // namespace driver_util

#endif  // DRIVER_UTIL_WAIT_QUEUE_H_
