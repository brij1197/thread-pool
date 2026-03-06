#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

namespace tp {
/// A fixed-size thread pool that accepts callables and returns futures
/// Usage:
///    tp::ThreadPool pool(4);
///    auto future = pool.submit([](int a, int b){ return a + b; }, 2, 3);
///    assert(future.get() == 5);
///
class ThreadPool {
private:
  void workerLoop();

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;

  mutable std::mutex mutex_;
  std::condition_variable
      task_cv_; // workers wait on this when there are no tasks
  std::condition_variable
      done_cv_; // waiters wait on this when all tasks are done

  std::atomic<bool> stop_{false};
  std::size_t active_{0}; // number of tasks currently being processed

public:
  /// Construct a thread pool with `numThreads` workers
  explicit ThreadPool(std::size_t numThreads = 0);

  /// Destructor joins all workers after draining the queue
  ~ThreadPool();

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;

  //  Task Submission
  /// Submit a callable task to the thread pool and get a future for its result
  /// Throws std::runtime_error if the pool is stopped
  template <typename Func, typename... Args>
  auto submit(Func &&func, Args &&...args)
      -> std::future<std::invoke_result_t<Func, Args...>>;

  //  Observers

  /// Number of worker threads
  [[nodiscard]] std::size_t size() const noexcept;

  /// Number of tasks currently waiting in the queue
  [[nodiscard]] std::size_t pending() const;

  /// True if shutdown() has been called
  [[nodiscard]] bool isShutdown() const noexcept;

  //  Synchronization

  /// Block until all tasks have completed and the pool is idle
  void wait_all();

  /// Stop accepting new tasks and shutdown the pool after completing pending
  /// tasks
  void shutdown();
};

//  Template impementation
template <typename Func, typename... Args>
auto ThreadPool::submit(Func &&func, Args &&...args)
    -> std::future<std::invoke_result_t<Func, Args...>> {
  using ReturnType = std::invoke_result_t<Func, Args...>;
  auto task = std::make_shared<std::packaged_task<ReturnType()>>(
      std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

  std::future<ReturnType> result = task->get_future();

  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stop_) {
      throw std::runtime_error("ThreadPool::submit() is called after shutdown");
    }
    tasks_.emplace([task]() { (*task)(); });
  }
  task_cv_.notify_one();
  return result;
}

}