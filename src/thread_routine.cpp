#include"thread_utils.hpp"
#include <liburing.h>
#include "input_handlers.hpp"

#include <immintrin.h>

void thread_routine(ThreadContext *thread_context) {
    while (true) {
        handle_inbound_queue(thread_context);

        struct io_uring_cqe* cqe = nullptr;
        int ret = io_uring_peek_cqe(&thread_context->ring, &cqe);

        if (ret != 0 || !cqe) {
            _mm_pause();
            continue;
        }

        auto* ctx = reinterpret_cast<ConnContext*>(io_uring_cqe_get_data(cqe));
        if (ctx) {
            switch (ctx->type) {
                case OpType::ACCEPT: handle_accept(cqe, thread_context); break;
                case OpType::RECV:   handle_recv(ctx, cqe, thread_context);   break;
                case OpType::SEND:   handle_send(ctx, cqe, thread_context);   break;
            }
        }
        io_uring_cqe_seen(&thread_context->ring, cqe);
    }
}
