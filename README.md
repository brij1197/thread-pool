# ThreadPool

A modern **C++17 thread pool** library.

```
┌────────────┐     ┌──────────────┐     ┌─────────────┐
│ Client Code│────▶│  Task Queue  │────▶│Worker Thread│
│  submit(fn)│     │  (mutex + cv)│     │ (N threads) │
└────────────┘     └──────────────┘     └─────────────┘
        ▲                                       │
        └──────── std::future<T> ◀──────────────┘
```

## Features

- **`submit(f, args...)`** — submit any callable; returns `std::future<T>`
- **`wait_all()`** — block until every submitted task has finished
- **Exception propagation** — exceptions thrown in tasks surface via `future::get()`
- **Graceful shutdown** — destructor drains the queue, then joins threads
- **Zero dependencies** — pure C++17, only needs `<thread>`, `<mutex>`, `<future>`
- **Catch2 unit tests** with CTest integration
- **GitHub Actions CI** for Ubuntu & macOS

## Quick Start

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)
```

### Run examples

```bash
./build/basic_usage
./build/parallel_transform
```

### Run tests

```bash
cd build && ctest --output-on-failure
```

## Usage

```cpp
#include "threadpool/thread_pool.hpp"

int main() {
    tp::ThreadPool pool(4);

    // Submit a task that returns a value
    auto future = pool.submit([](int x) { return x * 2; }, 21);
    assert(future.get() == 42);

    // Submit many tasks
    std::vector<std::future<int>> results;
    for (int i = 0; i < 100; ++i) {
        results.push_back(pool.submit([i] { return i * i; }));
    }

    // Wait for everything
    pool.wait_all();

    // Collect results
    for (int i = 0; i < 100; ++i) {
        std::cout << results[i].get() << "\n";
    }

    return 0;
} // pool destructor joins all threads
```

## API Reference

| Method | Description |
|---|---|
| `ThreadPool(n)` | Create pool with `n` threads (default: hardware concurrency) |
| `submit(f, args...)` | Submit callable, returns `std::future<T>` |
| `wait_all()` | Block until all tasks complete |
| `shutdown()` | Signal stop and join all threads |
| `size()` | Number of worker threads |
| `pending()` | Number of queued (not yet started) tasks |
| `is_shutdown()` | Whether `shutdown()` has been called |

## Project Structure

```
thread-pool/
├── CMakeLists.txt              # Top-level build
├── include/threadpool/
│   └── thread_pool.hpp         # Header (API + template impl)
├── src/
│   └── thread_pool.cpp         # Non-template implementation
├── tests/
│   ├── CMakeLists.txt          # Catch2 test setup
│   └── test_thread_pool.cpp    # Unit tests
├── examples/
│   ├── basic_usage.cpp         # Simple demo
│   └── parallel_transform.cpp  # Real-world parallel pattern
└── .github/workflows/
    └── ci.yml                  # CI pipeline
```