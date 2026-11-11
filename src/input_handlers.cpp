#include "input_handlers.hpp"
#include <format>
#include <liburing.h>
#include <liburing/io_uring.h>
#include "thread_utils.hpp"
#include "parser.hpp"
#include <cstring>
#include "inbound_queue.hpp"
#include <xxhash.h>
size_t get_target_worker(std::string_view key, size_t total_threads) {
    return XXH3_64bits(key.data(), key.size()) & (total_threads-1);
}
void handle_accept(io_uring_cqe *cqe, ThreadContext *thread_context){
    int client_fd = cqe->res;
    if (client_fd >= 0) {

        auto* client_ctx = new ConnContext{ .fd = client_fd, .type = OpType::RECV };
        struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
        io_uring_prep_recv(r_sqe, client_fd, client_ctx->buffer, sizeof(client_ctx->buffer) - 1, 0);
        io_uring_sqe_set_data(r_sqe, client_ctx);
        io_uring_submit(&thread_context->ring);
    }
}
void handle_task(ThreadContext *thread_context, Resp_Command *task) {
    ConnContext *ctx = task->ctx;
    ctx->type = OpType::SEND;

    switch (task->command) {
        case PING: {
            static constexpr std::string_view ping_response = "+PONG\r\n";
            std::memcpy(ctx->buffer, ping_response.data(), ping_response.size());

            struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
            io_uring_prep_send(r_sqe, ctx->fd, ctx->buffer, ping_response.size(), 0);
            io_uring_sqe_set_data(r_sqe, ctx);
            io_uring_submit(&thread_context->ring);
            break;
        }
        case GET: {
            std::string_view key = task->args[1];
            auto get = thread_context->thread_map.find(std::string(key));

            if (get != thread_context->thread_map.end()) [[likely]] {
                const std::string& val = get->second;
                auto res = std::format_to_n(ctx->buffer, sizeof(ctx->buffer), "${}\r\n", val.size());
                size_t offset = res.size;

                std::memcpy(ctx->buffer + offset, val.data(), val.size());
                offset += val.size();
                ctx->buffer[offset++] = '\r';
                ctx->buffer[offset++] = '\n';

                struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
                io_uring_prep_send(r_sqe, ctx->fd, ctx->buffer, offset, 0);
                io_uring_sqe_set_data(r_sqe, ctx);
                io_uring_submit(&thread_context->ring);
            } else {
                static constexpr std::string_view NIL_RESP = "$-1\r\n";
                std::memcpy(ctx->buffer, NIL_RESP.data(), NIL_RESP.size());

                struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
                io_uring_prep_send(r_sqe, ctx->fd, ctx->buffer, NIL_RESP.size(), 0);
                io_uring_sqe_set_data(r_sqe, ctx);
                io_uring_submit(&thread_context->ring);
            }
            break;
        }
        case SET: {
            std::string_view key = task->args[1];
            std::string_view value = task->args[2];
            thread_context->thread_map.insert_or_assign(std::string{key}, std::string{value});

            static constexpr std::string_view ok_response = "+OK\r\n";
            std::memcpy(ctx->buffer, ok_response.data(), ok_response.size());

            struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
            io_uring_prep_send(r_sqe, ctx->fd, ctx->buffer, ok_response.size(), 0);
            io_uring_sqe_set_data(r_sqe, ctx);
            io_uring_submit(&thread_context->ring);
            break;
        }
        case UNKNOWN: {
            static constexpr std::string_view unknown_response = "-ERR unknown command\r\n";
            std::memcpy(ctx->buffer, unknown_response.data(), unknown_response.size());

            struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
            io_uring_prep_send(r_sqe, ctx->fd, ctx->buffer, unknown_response.size(), 0);
            io_uring_sqe_set_data(r_sqe, ctx);
            io_uring_submit(&thread_context->ring);
            break;
        }
    }
}
void handle_recv(ConnContext *ctx, io_uring_cqe *cqe, ThreadContext *thread_context){
    int bytes_read = cqe->res;
    if (bytes_read > 0) {
        ctx->buffer[bytes_read] = '\0';
        Resp_Command in_command = parse(static_cast<std::string_view>(ctx->buffer));
        in_command.ctx = std::move(ctx);
        size_t worker = get_target_worker(in_command.args[1], thread_context->total_thread_num);
        if(thread_context->thread_id==worker){
            handle_task(thread_context, &in_command);
        }
        else{
            thread_context->queue->put(thread_context->thread_id, worker, std::move(in_command));
        }
        }

    else {
        close(ctx->fd);
        delete ctx;
    }
}

void handle_send(ConnContext *ctx, io_uring_cqe *cqe, ThreadContext *thread_context){
    ctx->type = OpType::RECV;
    struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
    io_uring_prep_recv(r_sqe, ctx->fd, ctx->buffer, sizeof(ctx->buffer) - 1, 0);
    io_uring_sqe_set_data(r_sqe, ctx);
    io_uring_submit(&thread_context->ring);
}

void handle_inbound_queue(ThreadContext *thread_context){
    size_t thread_num = thread_context->thread_id;
    size_t threads_total = thread_context -> total_thread_num;
    for (size_t i = thread_num*threads_total; i<thread_num*threads_total + threads_total; i++){
        Resp_Command task;
        while(thread_context->queue->queues[i]->pop(task)){
            handle_task(thread_context, &task);
        }
    }
}
