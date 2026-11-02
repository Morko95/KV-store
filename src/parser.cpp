#include "parser.hpp"
#include <cstddef>
#include <charconv>
#include <string_view>


Resp_Command parse(std::string_view input){
    [[unlikely]] if (!input.starts_with("*")){
        return Resp_Command{Operation_type::ERROR, 0};
    }
    Resp_Command result{};
    auto clrf = input.find("\r\n");
    int arg_count;
    auto [ptr, ec] = std::from_chars(input.data() + 1, input.data() + clrf-1,   arg_count, 10);
    input.remove_prefix(clrf+3);
    int i;
    for(i = 0; i<arg_count; i++){
        clrf = input.find("\r\n");
        size_t bulkstring_len;
        auto[ptr, ec] = std::from_chars(input.data(), input.data()+clrf-1, bulkstring_len, 10);
        input.remove_prefix(clrf+2);
        std::string_view arg = input.substr(0, bulkstring_len-1);
        result.args[i] = arg;
        input.remove_prefix(bulkstring_len + 3);
    }
    result.args_count = i;
    result.command = [&]() -> Operation_type {
        if (result.args[0] == "PING") return Operation_type::PING;
        if (result.args[0] == "GET")  return Operation_type::GET;
        if (result.args[0] == "SET")  return Operation_type::SET;
        return Operation_type::UNKNOWN;
    }();
    return result;

}
