#pragma once
#include "inbound_queue.hpp"
#include "conn_context.hpp"
#include "parser.hpp"
#include <unordered_map>
#include <string>
#include <liburing.h>


;
struct ThreadContext{
    struct io_uring ring {};
    int thread_fd{-1};
    std::unordered_map<std::string, std::string> thread_map;
    size_t thread_id{};
    size_t total_thread_num{};
    InboundQueue<Resp_Command, 8, 4096>* queue{nullptr};
};

void socket_init(ThreadContext &context);
void uring_init(ThreadContext &context);
