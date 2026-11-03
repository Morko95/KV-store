#include "parser.hpp"
#include <cstddef>
#include <charconv>
#include <string_view>

Resp_Command parse(std::string_view input) {
    if (!input.starts_with("*")) [[unlikely]] {
        return Resp_Command{Operation_type::ERROR, 0};
    }
    Resp_Command result{};
    auto clrf = input.find("\r\n");
    int arg_count = 0;

    auto [ptr, ec] = std::from_chars(input.data() + 1, input.data() + clrf, arg_count, 10);

    input.remove_prefix(clrf + 3);

    int i = 0;
    for (i = 0; i < arg_count; i++) {
        clrf = input.find("\r\n");
        size_t bulkstring_len = 0;

        auto [ptr2, ec2] = std::from_chars(input.data(), input.data() + clrf, bulkstring_len, 10);
        input.remove_prefix(clrf + 2);

        std::string_view arg = input.substr(0, bulkstring_len);
        result.args[i] = arg;

        if (i + 1 < arg_count) {
            input.remove_prefix(bulkstring_len + 3);
        } else {
            input.remove_prefix(bulkstring_len + 2);
        }
    }

    result.args_count = i;

    result.command = [&]() -> Operation_type {
        if (result.args_count == 0)   return Operation_type::UNKNOWN;
        if (result.args[0] == "PING") return Operation_type::PING;
        if (result.args[0] == "GET")  return Operation_type::GET;
        if (result.args[0] == "SET")  return Operation_type::SET;
        return Operation_type::UNKNOWN;
    }();

    return result;
}
