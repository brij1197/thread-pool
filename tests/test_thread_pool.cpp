#include <catch2/catch_test_macros.hpp>

#include "threadpool/thread_pool.hpp"

#include <atomic>
#include <cstddef>
#include <numeric>
#include <string>
#include <vector>

using namespace std::chrono_literals;

// Construction

TEST_CASE("ThreadPool can be constructed with explicit thread count",
          "[ctor]") {
  tp::ThreadPool pool(4);
  REQUIRE(pool.size() == 4);
}

TEST_CASE("ThreadPool defaults to hardware concurrency threads", "[ctor]") {
  tp::ThreadPool pool;
  auto hw = std::thread::hardware_concurrency();
  std::size_t expected = (hw > 0) ? hw : 2;
  REQUIRE(pool.size() == expected);
}

TEST_CASE("ThreadPool with 1 thread works", "[ctor]") {
  tp::ThreadPool pool(1);
  REQUIRE(pool.size() == 1);
  auto f = pool.submit([] { return 42; });
  REQUIRE(f.get() == 42);
}

//  Basic Submit & future

TEST_CASE("Submit returns correct result via future", "[submit]") {
  tp::ThreadPool pool(2);

  auto f1 = pool.submit([] { return 10 + 32; });
  auto f2 = pool.submit([](int a, int b) { return a * b; }, 6, 7);

  REQUIRE(f1.get() == 42);
  REQUIRE(f2.get() == 42);
}

TEST_CASE("Submit works with void return type", "[submit]") {
  tp::ThreadPool pool(2);
  std::atomic<int> counter{0};

  auto f = pool.submit([&counter] { counter.fetch_add(1); });
  f.get();

  REQUIRE(counter.load() == 1);
}

TEST_CASE("Submit works with string return type", "[submit]") {
  tp::ThreadPool pool(2);

  auto f = pool.submit([] { return std::string("hello, pool"); });
  REQUIRE(f.get() == "hello, pool");
}

TEST_CASE("Submit propagates exceptions", "[submit]") {
  tp::ThreadPool pool(2);

  auto f = pool.submit([] {
    throw std::runtime_error("test error");
    return 0;
  });

  REQUIRE_THROWS_AS(f.get(), std::runtime_error);
}

TEST_CASE("Submit after shutdown throws", "[submit]") {
  tp::ThreadPool pool(2);
  pool.shutdown();

  REQUIRE_THROWS_AS(pool.submit([] { return 42; }), std::runtime_error);
}

//  Many Tasks

TEST_CASE("Pool handles many tasks correctly", "[stress]") {
    constexpr int N = 1000;
    tp::ThreadPool pool(4);
    std::vector<std::future<int>> futures;
    futures.reserve(N);

    for (int i = 0; i < N; ++i) {
        futures.push_back(pool.submit([i] { return i * i; }));
    }

    for (int i = 0; i < N; ++i) {
        REQUIRE(futures[i].get() == i * i);
    }
}

TEST_CASE("Pool handles heterogeneous task durations", "[stress]") {
    tp::ThreadPool pool(4);
    std::atomic<int> sum{0};

    for (int i = 0; i < 20; ++i) {
        pool.submit([&sum, i] {
            // Varying workloads
            volatile int x = 0;
            for (int j = 0; j < (i + 1) * 100; ++j) { ++x; }
            sum.fetch_add(i);
        });
    }

    pool.wait_all();
    REQUIRE(sum.load() == 190);
}

//  wait_all

TEST_CASE("wait_all blocks until all tasks finish", "[sync]") {
  tp::ThreadPool pool(4);
  std::atomic<int> counter{0};

  for (int i = 0; i < 10; ++i) {
    pool.submit([&counter] {
      std::this_thread::sleep_for(100ms);
      counter.fetch_add(1);
    });
  }

  pool.wait_all();
  REQUIRE(counter.load() == 10);
}

TEST_CASE("wait_all returns immediately if no tasks", "[sync]") {
  tp::ThreadPool pool(4);
  pool.wait_all();
  REQUIRE(pool.pending() == 0);
}

//  Observers

TEST_CASE("is_shutdown reflects pool state", "[observer]") {
    tp::ThreadPool pool(2);
    REQUIRE_FALSE(pool.isShutdown());
    pool.shutdown();
    REQUIRE(pool.isShutdown());
}

TEST_CASE("shutdown is idempotent", "[observer]") {
    tp::ThreadPool pool(2);
    pool.shutdown();
    pool.shutdown();
    REQUIRE(pool.isShutdown());
}

//  Real-world patterns

TEST_CASE("Parallel accumulate pattern", "[pattern]") {
    constexpr std::size_t N = 10000;
    std::vector<int> data(N);
    std::iota(data.begin(), data.end(), 1);

    tp::ThreadPool pool(4);
    constexpr std::size_t CHUNKS = 4;
    std::size_t chunk_size = N / CHUNKS;

    std::vector<std::future<long long>> futures;
    for (std::size_t c = 0; c < CHUNKS; ++c) {
        auto begin = data.begin() + static_cast<long>(c * chunk_size);
        auto end = (c == CHUNKS - 1) ? data.end()
                                     : data.begin() + static_cast<long>((c + 1) * chunk_size);
        futures.push_back(pool.submit([begin, end] {
            return std::accumulate(begin, end, 0LL);
        }));
    }

    long long total = 0;
    for (auto& f : futures) {
        total += f.get();
    }

    REQUIRE(total == static_cast<long long>(N) * (N + 1) / 2);
}