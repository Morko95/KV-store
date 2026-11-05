#include "parser.hpp"
#include <cstddef>
#include <charconv>
#include <string_view>

Resp_Command parse(std::string_view input) {
    if (!input.starts_with("*")) [[unlikely]] {
        return Resp_Command{.command = Operation_type::UNKNOWN};
    }

    Resp_Command result{};
    auto clrf = input.find("\r\n");
    if (clrf == std::string_view::npos) return Resp_Command{.command = Operation_type::UNKNOWN};

    int arg_count = 0;
    // Czytamy liczbę argumentów po znaku '*'
    auto [ptr, ec] = std::from_chars(input.data() + 1, input.data() + clrf, arg_count, 10);
    if (ec != std::errc{}) return Resp_Command{.command = Operation_type::UNKNOWN};

    input.remove_prefix(clrf + 2);

    int i = 0;
    for (i = 0; i < arg_count; i++) {
        if (!input.starts_with("$")) [[unlikely]] return Resp_Command{.command = Operation_type::UNKNOWN};

        clrf = input.find("\r\n");
        if (clrf == std::string_view::npos) [[unlikely]] return Resp_Command{.command = Operation_type::UNKNOWN};

        size_t bulkstring_len = 0;
        auto [ptr2, ec2] = std::from_chars(input.data() + 1, input.data() + clrf, bulkstring_len, 10);
        if (ec2 != std::errc{}) [[unlikely]] return Resp_Command{.command = Operation_type::UNKNOWN};

        input.remove_prefix(clrf + 2);

        if (input.size() < bulkstring_len) return Resp_Command{.command = Operation_type::UNKNOWN};
        std::string_view arg = input.substr(0, bulkstring_len);
        result.args[i] = arg;

        input.remove_prefix(bulkstring_len + 2);
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
