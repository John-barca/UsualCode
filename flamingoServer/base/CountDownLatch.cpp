#include "CountDownLatch.h"

// 用来维护同步与互斥关系

CountDownLatch::CountDownLatch(int count) :count(count) {}
void CountDownLatch::wait() {
  std::unique_lock<std::mutex> lock(mtx);
  while (count > 0) {
    cond.wait(lock);
  }
}

void CountDownLatch::countDown() {
  std::unique_lock<std::mutex> lock(mtx);
  --count;
  if (count == 0) {
    cond.notify_all();
  }
}

int CountDownLatch::getCount() const {
  std::unique_lock<std::mutex> lock(mtx);
  return count;
}