#include "threadpool/thread_pool.hpp"
#include <cstddef>
#include <utility>

namespace tp {
//  Construction/Destruction
ThreadPool::ThreadPool(std::size_t numThreads) : stop_(false) {
  if (numThreads == 0) {
    numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) {
      numThreads = 2; // safe fallback
    }
  }
  workers_.reserve(numThreads);
  for (std::size_t i = 0; i < numThreads; ++i) {
    workers_.emplace_back(&ThreadPool::workerLoop, this);
  }
}

ThreadPool::~ThreadPool() { 
    shutdown(); 
}

//  Worker Loop

void ThreadPool::workerLoop() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      task_cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
      if (stop_ && tasks_.empty()) {
        return;
      }
      task = std::move(tasks_.front());
      tasks_.pop();
      ++active_;
    }
    task();
    {
      std::lock_guard<std::mutex> lock(mutex_);
      --active_;
    }
    done_cv_.notify_all();
  }
}

//  Observers
std::size_t ThreadPool::size() const noexcept { return workers_.size(); }

std::size_t ThreadPool::pending() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return tasks_.size();
}

bool ThreadPool::isShutdown() const noexcept { return stop_; }

//  Synchronization
void ThreadPool::wait_all() {
  std::unique_lock<std::mutex> lock(mutex_);
  done_cv_.wait(lock, [this] { return tasks_.empty() && active_ == 0; });
}

void ThreadPool::shutdown() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stop_) {
      return;
    }
    stop_ = true;
  }
  task_cv_.notify_all();

  for (auto &worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

} // namespace tp