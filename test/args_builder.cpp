#include "args_builder.h"

namespace cpparg::test {

args_builder::args_builder(std::string_view name)
    : ptrs_{nullptr} {
    add(name);
}

args_builder& args_builder::add(std::string_view arg) {
    ptrs_.pop_back();
    ptrs_.emplace_back(arg.data());
    ptrs_.push_back(nullptr);
    return *this;
}

args_builder& args_builder::add(std::string_view key, std::string_view value) {
    return add(key).add(value);
}

std::tuple<int, const char**> args_builder::get() {
    return std::tuple(static_cast<int>(ptrs_.size()), ptrs_.data());
}

} // namespace cpparg::test
