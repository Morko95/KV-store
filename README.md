# KV-store-PoC (Proof of Concept)

A lightweight, asynchronous in-memory Key-Value store built on modern Linux systems programming primitives, utilizing `io_uring` for event-driven networking and implementing the Redis Serialization Protocol (RESP).

Currently a functional single-threaded proof of concept validating asynchronous kernel event loops and zero-allocation frame parsing.

---

## Current Status (v0.1-PoC)

- **Single-Threaded Event Loop:** A working main routine orchestrating TCP connection lifecycle (`ACCEPT`, `RECV`, `SEND`) directly via `io_uring` submission (SQE) and completion (CQE) rings.
- **Zero-Copy RESP Parser:** C++20 `std::string_view` and `std::from_chars` based parser capable of decoding bulk strings and commands (`PING`, `GET`, `SET`) without intermediate heap allocations.
- **Tooling:** Strict build configuration enforcing `-Wall -Wextra -Wpedantic` under CMake and C++20.

---

## Architectural Roadmap / TODO

- [ ] **Modular Refactoring:** Decouple connection lifecycle state machine (`ConnContext`), protocol parsing, and storage engine into dedicated compilation units.
- [ ] **Ring Buffering:** Replace fixed-size per-connection arrays with zero-copy ring buffers for pipelined command buffering.
- [ ] **Multi-Threaded Scaling (`std::thread::hardware_concurrency`):**
  - Thread-per-core execution model spawning `hardware_concurrency` worker threads.
  - Dedicated `io_uring` instances per thread with shared buffer pools.
  - Barrier-synchronized startup phase (`std::barrier` / `pthread_barrier`) guaranteeing deterministic server socket initialization and ring setup before servicing traffic.
- [ ] **Storage Engine:** Thread-safe, concurrent hash table or sharded associative array with lock-free reads.

---

## Prerequisites

- Linux Kernel ≥ 5.10 (recommended ≥ 5.15+ for complete `io_uring` feature parity)
- `liburing`
- C++20 compliant compiler (GCC 11+ or Clang 13+)
- CMake ≥ 3.20

---

## Build & Run

```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run server (default port 6379)
./build/resp_server
