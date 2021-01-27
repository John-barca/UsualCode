#pragma once

#include <mutex>
#include <condition_variable>

class CountDownLatch {
public:
  explicit CountDownLatch(int count);
  void wait();
  void countDown();
  int getCount() const;
private:
  mutable std::mutex mtx;
  std::condition_variable cond;
  int count;
}