#pragma once
#include "thread_utils.hpp"
#include "parser.hpp"


void handle_accept( io_uring_cqe *cqe, ThreadContext *thread_context);
void handle_recv(ConnContext *ctx, io_uring_cqe *cqe, ThreadContext *thread_context);
void handle_send(ConnContext *ctx, io_uring_cqe *cqe, ThreadContext *thread_context);
void handle_task(ThreadContext *thread_context, Resp_Command *task);
void handle_inbound_queue(ThreadContext *thread_context);
