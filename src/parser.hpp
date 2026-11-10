#pragma once
#include "conn_context.hpp"
#include <array>
#include <string_view>

enum Operation_type{
    PING,
    GET,
    SET,
    UNKNOWN
};

struct Resp_Command{
    Operation_type command{Operation_type::UNKNOWN};
    size_t args_count{0};
    ConnContext *ctx;
    std::array<std::string_view, 3> args;

};
Resp_Command parse(std::string_view input);
