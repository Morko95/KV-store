#include"thread_utils.hpp"
#include <liburing.h>
#include <cstring>
#include "input_handlers.hpp"

void thread_routine(ThreadContext thread_context){
    while (true) {
        //TODO first empty thread's ring buffers
        struct io_uring_cqe* cqe = nullptr;
        int ret = io_uring_wait_cqe(&thread_context.ring, &cqe);
        if (ret < 0) {
            std::fprintf(stderr, "wait_cqe error: %s\n", strerror(-ret));
            break;
        }

        auto* ctx = reinterpret_cast<ConnContext*>(io_uring_cqe_get_data(cqe));

        switch (ctx->type) {
            case OpType::ACCEPT: {
                handle_accept(ctx, cqe, &thread_context);
                break;
            }
            case OpType::RECV: {
               handle_recv(ctx, cqe, &thread_context);
               break;
            }
            case OpType::SEND: {
               handle_send(ctx, cqe, &thread_context);
            }
        }

        io_uring_cqe_seen(&thread_context.ring, cqe);
}
}
