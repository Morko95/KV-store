#pragma once

#include <array>
#include <string_view>

enum Operation_type{
    PING,
    GET,
    SET,
    OK,
    ERROR,
    UNKNOWN
};

struct Resp_Command{
    Operation_type command{Operation_type::UNKNOWN};
    size_t args_count{0};
    std::array<std::string_view, 2> args;

};
Resp_Command parse(std::string_view input);
