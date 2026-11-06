#include <asm-generic/socket.h>
#include <cstddef>
#include <cstring>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <sys/socket.h>
#include <system_error>
#include <string.h>
#include <netinet/in.h>
#include "thread_queue.hpp"
#include <unordered_map>
#include "thread_utils.hpp"

void socket_init(ThreadContext context){
    context.thread_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(context.thread_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(context.thread_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    sockaddr_in sock_addr{};
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons(8080);
    sock_addr.sin_family = AF_INET;
    if (bind(context.thread_fd, reinterpret_cast<sockaddr*>(&sock_addr), sizeof(sock_addr))!=0){
        throw std::system_error(errno, std::generic_category(), "Bind failed");}
    listen(context.thread_fd, 512);
}
void uring_init(ThreadContext context){
    unsigned int flags = 0;

    flags |= IORING_SETUP_SINGLE_ISSUER;
    flags |= IORING_SETUP_COOP_TASKRUN;


    int inited = io_uring_queue_init(512,  &context.ring, flags);
    if(inited!=0){
        throw std::system_error(-inited, std::generic_category(), "io_uring_queue_init failed");
    }

    struct io_uring_sqe *sqe = io_uring_get_sqe(&context.ring);
    io_uring_prep_multishot_accept(sqe, context.thread_fd, NULL, NULL, 0);
    auto *accept_ctx = new ConnContext{.fd = context.thread_fd, .type = OpType::ACCEPT};
    io_uring_sqe_set_data(sqe, accept_ctx);
    io_uring_submit(&context.ring);
}
ThreadContext thread_init(){
    struct io_uring ring;
    std::unordered_map<std::string, std::string> thread_map;
    ThreadContext thread_context = ThreadContext{.ring = ring, .thread_map = thread_map};
    socket_init(&thread_context);
    uring_init(&thread_context);
    return thread_context;
}
