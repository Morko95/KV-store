#include "input_handlers.hpp"
#include <format>
#include <liburing.h>
#include <liburing/io_uring.h>
#include "thread_utils.hpp"
#include "parser.hpp"
#include <cstring>
void handle_accept(ConnContext *ctx, io_uring_cqe *cqe, ThreadContext *thread_context){
    int client_fd = cqe->res;
    if (client_fd >= 0) {
        std::printf("[+] Połączono: client_fd=%d\n", client_fd);

        auto* client_ctx = new ConnContext{ .fd = client_fd, .type = OpType::RECV };
        struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
        io_uring_prep_recv(r_sqe, client_fd, client_ctx->buffer, sizeof(client_ctx->buffer) - 1, 0);
        io_uring_sqe_set_data(r_sqe, client_ctx);
        io_uring_submit(&thread_context->ring);
    }
}

void handle_recv(ConnContext *ctx, io_uring_cqe *cqe, ThreadContext *thread_context){
    int bytes_read = cqe->res;
    if (bytes_read > 0) {
        ctx->buffer[bytes_read] = '\0';
        std::printf("[fd=%d] Odebrano (%d B): %s\n", ctx->fd, bytes_read, ctx->buffer);
        Resp_Command in_command = parse(static_cast<std::string_view>(ctx->buffer));
        switch (in_command.command){
            case PING:{
                static constexpr std::string_view ping_response = "+PONG";
                ConnContext *output = new ConnContext{ctx->fd, OpType::SEND};
                std::memcpy(output->buffer, ping_response.data(), ping_response.size());
                struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
                io_uring_prep_send(r_sqe, output->fd, output->buffer,  ping_response.size(), 0);
                io_uring_sqe_set_data(r_sqe, output);
                io_uring_submit(&thread_context->ring);
                break;
            }
            case GET:{
                std::string_view key = in_command.args[1];
                auto get = thread_context->thread_map.find(std::string(key));
                ConnContext* result = new ConnContext{ctx->fd, OpType::SEND};

                if(get != thread_context->thread_map.end())[[likely]]{
                    const std::string& val = get->second;
                    auto res = std::format_to_n(result->buffer, sizeof(result->buffer), "${}\r\n", val.size());
                    size_t offset = res.size;

                    std::memcpy(result->buffer + offset, val.data(), val.size());
                    offset +=val.size();
                    result->buffer[offset++] = '\r';
                    result->buffer[offset++] = '\n';
                    size_t total_len = offset + val.size();
                    result->buffer[total_len++] = '\r';
                    result->buffer[total_len++] = '\n';
                    struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
                    io_uring_prep_send(r_sqe, result->fd, result->buffer,  offset, 0);
                    io_uring_sqe_set_data(r_sqe, result);
                    io_uring_submit(&thread_context->ring);
                }
                else{

                    static constexpr std::string_view NIL_RESP = "$-1\r\n";
                    std::memcpy(result->buffer, NIL_RESP.data(), NIL_RESP.size());
                    std::size_t write_len = NIL_RESP.size();
                    struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
                    io_uring_prep_send(r_sqe, result->fd, result->buffer, write_len, 0);
                    io_uring_sqe_set_data(r_sqe, result);
                    io_uring_submit(&thread_context->ring);
                }
                break;
                }
            case SET:{
                std::string_view key = in_command.args[1];
                std::string_view value = in_command.args[2];
                thread_context->thread_map.insert_or_assign(std::string{key}, std::string{value});
                static constexpr std::string_view ok_response = "+OK\r\n";
                ConnContext *output = new ConnContext{ctx->fd, OpType::SEND};
                std::memcpy(output->buffer, ok_response.data(), ok_response.size());
                struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
                io_uring_prep_send(r_sqe, output->fd, output->buffer,  ok_response.size(), 0);
                io_uring_sqe_set_data(r_sqe, output);
                io_uring_submit(&thread_context->ring);
                break;
                }

            case UNKNOWN:{
                static constexpr std::string_view unknown_response = "+UNKNOWN COMMAND\r\n";
                ConnContext *output = new ConnContext{ctx->fd, OpType::SEND};
                std::memcpy(output->buffer, unknown_response.data(), unknown_response.size());
                struct io_uring_sqe* r_sqe = io_uring_get_sqe(&thread_context->ring);
                io_uring_prep_send(r_sqe, output->fd, output->buffer,  unknown_response.size(), 0);
                io_uring_sqe_set_data(r_sqe, output);
                io_uring_submit(&thread_context->ring);
                break;
                }

        }
    } else {
        std::printf("[-] Rozłączono: client_fd=%d\n", ctx->fd);
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
