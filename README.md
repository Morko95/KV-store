# KV-store-PoC (High-Performance Engine)

A lightweight, asynchronous, in-memory Key-Value store built on modern Linux systems programming primitives, utilizing `io_uring` for event-driven networking and implementing the Redis Serialization Protocol (RESP).

Designed around a **shared-nothing, thread-per-core architecture** with lock-free inter-thread communication, achieving **~138k RPS** in synchronous ping-pong benchmarks.

---

## Current Architecture & Status (v0.2-Multithreaded)

- **Shared-Nothing Thread-Per-Core:** Multiple worker threads (`KV_THREAD_NUM`) running independent event loops. Each worker owns a private `io_uring` ring instance and an isolated key-value map, eliminating cross-thread locks and data contention.
- **Deterministic Key Affinity (Sharding):** Incoming commands route to dedicated worker shards using `XXH3_64bits` key hashing.
- **Lock-Free Inter-Thread Bus:** Cross-shard task dispatch relies on a compile-time bidirectional SPSC (Single-Producer Single-Consumer) ring buffer matrix with `alignas(64)` cache-line alignment to avoid false sharing.
- **Zero-Allocation Critical Path:** Eliminated dynamic heap allocations (`operator new` / `malloc`) and stack memory vector clearing across request lifecycles by recycling context buffers.
- **Asynchronous Synchronization:** Deterministic startup orchestrated via `std::barrier`, preventing connection intake before ring and socket initialization complete.
- **Zero-Copy Parser:** In-place parsing using C++20 `std::string_view` and `std::from_chars` without intermediate heap allocations.

---

## Performance Baseline

Benchmarked on AMD Ryzen 7 8845HS using `redis-benchmark` in synchronous ping-pong mode (`-c 250 -P 1`):

- **SET:** ~138,200 req/sec (p50 = 1.01 ms)
- **GET:** ~112,900 req/sec (p50 = 1.23 ms)

---

## Architectural Roadmap / TODO

- [ ] **RESP Pipelining Support:** Implement multi-command packet parsing within a single `recv` window to handle batched requests (`-P > 1`).
- [ ] **Reorder Buffer / Outbound Response Aggregator:** Maintain strict RESP response ordering when pipelined commands within the same connection target different worker shards.
- [ ] **TCP Packet Splitting & Ring Buffering:** Support TCP streaming dynamics (handling partial/fragmented commands across read boundaries) via circular input buffers.
- [ ] **Heterogeneous Key Lookup:** Leverage C++20 transparent hashing (`std::string_view` lookup) on underlying shard maps to remove remaining temporary allocations during `find()`.
- [ ] **Batched Submission & SQPOLL:** Batch SQE submissions per event-loop iteration instead of immediate submission, with optional `IORING_SETUP_SQPOLL` evaluation.
- [ ] **Dynamic Core Scaling & Affinity:** Replace the compile-time worker count (`KV_THREAD_NUM`) with runtime detection (`std::thread::hardware_concurrency`), dynamically sizing the inter-thread SPSC matrix and pinning workers to physical cores via `pthread_setaffinity_np` to minimize context switching and cache thrashing.
---

## Prerequisites

- Linux Kernel ≥ 5.10 (recommended ≥ 5.15+ for full `io_uring` feature set)
- `liburing`
- `xxhash`
- C++20 compliant compiler (GCC 11+ or Clang 13+)
- CMake ≥ 3.20

---

## Build & Run

```bash
# Configure and build in Release mode
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run server (default port 8080)
./build/resp_server
