#include <asm-generic/socket.h>
#include <liburing.h>
#include <sys/socket.h>
#include <system_error>
#include <thread>
#include <string.h>
#include <netinet/in.h>
#include "thread_queue.hpp"
#include "parser.hpp"
#include <unordered_map>;
enum class OpType {
    ACCEPT,
    RECV,
    SEND
};

struct ConnContext {
    int fd;
    OpType type;
    char buffer[1024];
};
void thread_init(){
    struct io_uring ring;
    unsigned int flags = 0;

    flags |= IORING_SETUP_SINGLE_ISSUER;
    flags |= IORING_SETUP_COOP_TASKRUN;

    struct io_uring_params ring_params;
    struct thread_queue internal_queue;

    int inited = io_uring_queue_init(512,  &ring, flags);
    if(inited!=0){
        throw std::system_error(-inited, std::generic_category(), "io_uring_queue_init failed");
    }
    int opt = 1;
    int thread_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(thread_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(thread_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    sockaddr_in sock_addr{};
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons(8080);
    sock_addr.sin_family = AF_INET;
    if (bind(thread_fd, reinterpret_cast<sockaddr*>(&sock_addr), sizeof(sock_addr))!=0){
        throw std::system_error(errno, std::generic_category(), "Bind failed");}
    listen(thread_fd, 512);
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_multishot_accept(sqe, thread_fd, NULL, NULL, 0);
    auto *accept_ctx = new ConnContext{.fd = thread_fd, .type = OpType::ACCEPT};
    io_uring_sqe_set_data(sqe, accept_ctx);
    io_uring_submit(&ring);
    std::unordered_map<std::string, std::string> thread_map;
    while (true) {
        struct io_uring_cqe* cqe = nullptr;
        int ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0) {
            std::fprintf(stderr, "wait_cqe error: %s\n", strerror(-ret));
            break;
        }

        auto* ctx = reinterpret_cast<ConnContext*>(io_uring_cqe_get_data(cqe));

        switch (ctx->type) {
            case OpType::ACCEPT: {
                int client_fd = cqe->res;
                if (client_fd >= 0) {
                    std::printf("[+] Połączono: client_fd=%d\n", client_fd);

                    auto* client_ctx = new ConnContext{ .fd = client_fd, .type = OpType::RECV };
                    struct io_uring_sqe* r_sqe = io_uring_get_sqe(&ring);
                    io_uring_prep_recv(r_sqe, client_fd, client_ctx->buffer, sizeof(client_ctx->buffer) - 1, 0);
                    io_uring_sqe_set_data(r_sqe, client_ctx);
                    io_uring_submit(&ring);
                }
                break;
            }
            case OpType::RECV: {
                int bytes_read = cqe->res;
                if (bytes_read > 0) {
                    ctx->buffer[bytes_read] = '\0';
                    std::printf("[fd=%d] Odebrano (%d B): %s\n", ctx->fd, bytes_read, ctx->buffer);
                    Resp_Command in_command = parse(static_cast<std::string_view>(ctx->buffer));
                    switch (in_command.command){
                        case PING:{
                            break;
                        }
                        case GET:{
                            break;
                            }
                        case SET:{
                            std::string_view key = in_command.args[1];
                            std::string_view value = in_command.args[2];
                            thread_map.insert_or_assign(std::string{key}, std::string{value});

                            }
                        case OK:{
                            break;
                            }
                        case ERROR:{
                            break;
                            }
                        case UNKNOWN:{
                            break;
                            }

                    }
                } else {
                    std::printf("[-] Rozłączono: client_fd=%d\n", ctx->fd);
                    close(ctx->fd);
                    delete ctx;
                }
                break;
            }
            case OpType::SEND: {
                ctx->type = OpType::RECV;
                struct io_uring_sqe* r_sqe = io_uring_get_sqe(&ring);
                io_uring_prep_recv(r_sqe, ctx->fd, ctx->buffer, sizeof(ctx->buffer) - 1, 0);
                io_uring_sqe_set_data(r_sqe, ctx);
                io_uring_submit(&ring);
                break;
            }
        }

        io_uring_cqe_seen(&ring, cqe);
    }
};
