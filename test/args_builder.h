#pragma once

#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace cpparg::test {

class args_builder {
public:
    args_builder(std::string_view name = "");

    args_builder& add(std::string_view arg);
    args_builder& add(std::string_view key, std::string_view value);
    std::tuple<int, const char**> get();

private:
    std::vector<const char*> ptrs_;
};

} // namespace cpparg::test
