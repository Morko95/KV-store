#include "inbound_queue.hpp"
#include "parser.hpp"
#include "thread_utils.hpp"
#include <barrier>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <thread>
#include <array>
#include "thread_routine.hpp"
#define KV_THREAD_NUM 8
int main() {
    InboundQueue<Resp_Command, KV_THREAD_NUM, 4096> inbound_queue;
    std::barrier init_barrier(KV_THREAD_NUM);

    std::array<ThreadContext, KV_THREAD_NUM> contexts;
    std::array<std::jthread, KV_THREAD_NUM> threads;

    for (size_t i = 0; i < KV_THREAD_NUM; ++i) {
        threads[i] = std::jthread([i, &inbound_queue, &init_barrier, &contexts]() {
            auto& thread_context = contexts[i];

            thread_context.thread_id = i;
            thread_context.total_thread_num = KV_THREAD_NUM;
            thread_context.queue = &inbound_queue;

            socket_init(thread_context);
            uring_init(thread_context);

            init_barrier.arrive_and_wait();

            thread_routine(&thread_context);
        });
    }


    return 0;
}
