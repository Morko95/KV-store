#pragma once
#include "thread_utils.hpp"


void handle_accept(ConnContext *ctx, io_uring_cqe *cqe, ThreadContext *thread_context);
void handle_recv(ConnContext *ctx, io_uring_cqe *cqe, ThreadContext *thread_context);
void handle_send(ConnContext *ctx, io_uring_cqe *cqe, ThreadContext *thread_context);
