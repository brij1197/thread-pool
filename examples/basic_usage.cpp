///
/// This demonstrates the core ThreadPool functionality
///

#include "threadpool/thread_pool.hpp"

#include <chrono>
#include <iostream>
#include <vector>

int main() {
  std::cout << "ThreadPool Basic Usage" << std::endl;

  // Creating a pool with 4 workers
  tp::ThreadPool pool(4);
  std::cout << "Pool created with " << pool.size() << " threads." << std::endl;

  // Submitting tasks
  std::cout << "Submitting computation tasks" << std::endl;
  std::vector<std::future<int>> futures;

  for (int i = 1; i <= 8; ++i) {
    futures.push_back(pool.submit([i] {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(100 * i)); // Simulate work
      return i * i;
    }));
  }

  // Retrieving results
  std::cout << "Retrieving results:" << std::endl;
  for (int i = 0; i < 8; ++i) {
    std::cout << "Task " << i + 1 << " result: " << futures[i].get()
              << std::endl;
  }

  // Submitting a void task
  std::cout << "Submitting void tasks" << std::endl;
  for (int i = 0; i < 4; ++i) {
    pool.submit([i] {
      std::cout << "  Fire-and-forget task " << i << " done (thread "
                << std::this_thread::get_id() << ")" << std::endl;
    });
  }

  // Waiting for everything to finish
  pool.wait_all();
  std::cout << "All tasks completed." << std::endl;

  // Handling Exceptions
  std::cout << "Exception Propagation" << std::endl;
  auto bad = pool.submit([] {
    throw std::runtime_error("Something went wrong!");
    return 0;
  });

  try {
    bad.get();
  } catch (const std::exception &e) {
    std::cout << "Caught exception from task: " << e.what() << std::endl;
  }

  std::cout << "Shutting down pool" << std::endl;
  return 0;
}