///
/// This demonstrates a real-world pattern: splitting a large dataset into
/// chunks, processing each chunk in parallel, and then combining the results.
///

#include "threadpool/thread_pool.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <vector>

/// Parallel transform - applies `func` to each element of `input` in parallel
/// and stores results in `output` Splits the work across `num_chunks` tasks in
/// the pool
template <typename T, typename Func>
void parallel_transform(tp::ThreadPool &pool, const std::vector<T> &input,
                        std::vector<T> &output, Func func,
                        std::size_t num_chunks = 0) {

  if (num_chunks == 0) {
    num_chunks = pool.size();
  }

  output.resize(input.size());
  std::size_t chunk_size = input.size() / num_chunks;
  std::vector<std::future<void>> futures;
  for (std::size_t i = 0; i < num_chunks; ++i) {
    std::size_t start = i * chunk_size;
    std::size_t end =
        (i == num_chunks - 1) ? input.size() : (i + 1) * chunk_size;

    futures.push_back(pool.submit([&input, &output, func, start, end] {
      std::transform(input.begin() + static_cast<long>(start),
                     input.begin() + static_cast<long>(end),
                     output.begin() + static_cast<long>(start), func);
    }));
  }

  for (auto &f : futures) {
    f.get();
  }
}

int main() {
  constexpr std::size_t N = 10'000'000;
  std::cout << "Parallel Transform Example" << std::endl;
  std::cout << "Processing " << N << " elements..." << std::endl;

  // Preparing input data
  std::vector<double> input(N);
  std::iota(input.begin(), input.end(), 1.0);
  std::vector<double> output;

  tp::ThreadPool pool(4);

  // Heavy math computation: sqrt of each element
  auto heavy_func = [](double x) -> double {
    return std::sqrt(x) * std::log(x + 1.0) + std::sin(x);
  };

  //  Sequential baseline
  auto t0 = std::chrono::steady_clock::now();

  std::vector<double> sequential(N);
  std::transform(input.begin(), input.end(), sequential.begin(), heavy_func);

  auto t1 = std::chrono::steady_clock::now();
  auto seq_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  std::cout << "Sequential time: " << seq_ms << " ms" << std::endl;

  //  Parallel version
  auto t2 = std::chrono::steady_clock::now();

  parallel_transform(pool, input, output, heavy_func);

  auto t3 = std::chrono::steady_clock::now();
  auto par_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
  std::cout << "Parallel time: " << par_ms << " ms" << " (" << pool.size()
            << " threads)" << std::endl;

  //  Verify correctness
  bool correct =
      std::equal(sequential.begin(), sequential.end(), output.begin());
  std::cout << "Results " << (correct ? "YES" : "NO") << std::endl;

  if (seq_ms > 0) {
    double speedup = static_cast<double>(seq_ms) / static_cast<double>(par_ms);
    std::cout << "Speedup: " << speedup << "x" << std::endl;
  }
  return 0;
}