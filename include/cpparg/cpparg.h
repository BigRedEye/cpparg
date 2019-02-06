#pragma once

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace cpparg {

namespace util {

inline std::string str(std::string_view view) {
    return std::string(view.begin(), view.end());
}

namespace detail {

template<typename T>
using istream_read_t = decltype(std::declval<std::istream&>() >> std::declval<T&>());

template<typename T, typename U = istream_read_t<T>>
std::true_type test(T);

std::true_type test(std::string);
std::true_type test(std::string_view);

template<typename ...Args>
std::false_type test(Args...);

}

template<typename T>
struct is_convertible_from_string : decltype(detail::test(std::declval<T>())) {};

template<>
struct is_convertible_from_string<void> : std::false_type {};

template<typename T>
inline constexpr bool is_convertible_from_string_v = is_convertible_from_string<T>::value;

class from_string_error : public std::runtime_error {
public:
    from_string_error()
        : std::runtime_error("Cannot parse from string") {
    }
};

namespace string {

template<typename T>
inline T to(std::string_view s) {
    static_assert(is_convertible_from_string_v<T>,
        "Cannot find std::istream& operator>>(std::istream&, T&)");

    if constexpr (std::is_same_v<std::string, T>) {
        return str(s);
    } else if constexpr (std::is_same_v<std::string_view, T>) {
        return s;
    } else {
        static std::istringstream ss;
        ss.clear();
        ss.str(str(s));
        T result;
        ss >> result;

        if (!ss) {
            throw from_string_error{};
        }

        if (ss.peek() != std::char_traits<char>::eof()) {
            throw from_string_error{};
        }

        return result;
    }
}

template<typename T>
inline std::string from(T&& t) {
    if constexpr (std::is_same_v<std::string, std::decay_t<T>>) {
        return t;
    } else {
        static std::ostringstream os;
        os.clear();
        os.str("");
        os << t;
        return os.str();
    }
}

inline bool starts(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

inline bool ends(std::string_view str, std::string_view suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), std::string_view::npos, suffix) == 0;
}

} // namespace string

template<typename... Args>
inline std::string join(Args&&... args) {
    return ("" + ... + string::from(args));
}

inline void normalize_tabs(std::vector<std::string>& vec, size_t tab_width) {
    auto first_tab = [](const std::string& s) { return s.find_first_of('\t') + 1; };

    auto it = std::max_element(
        vec.begin(), vec.end(), [&](const std::string& lhs, const std::string& rhs) {
            return first_tab(lhs) < first_tab(rhs);
        });

    size_t right = first_tab(*it);

    for (auto& s : vec) {
        size_t cur = s.find_first_of('\t');
        if (cur != std::string::npos) {
            s.replace(cur, 1, right - cur + tab_width, ' ');
        }
    }
}

} // namespace util

enum class parsing_error_policy {
    exit,
    rethrow,
};

class parser_error : public std::runtime_error {
public:
    parser_error(const std::string& what_)
        : std::runtime_error(what_) {
    }
};

class processor_error : public parser_error {
public:
    processor_error(const std::string& what_)
        : parser_error(what_) {
    }

    processor_error(const std::string& name, const std::string& descr)
        : parser_error(util::join("Cannot parse option " + name + ": " + descr)) {
    }
};

namespace detail {

static constexpr std::string_view OFFSET = "  ";
static constexpr size_t TAB_WIDTH = 4;

template<typename Child>
class parser_base {
public:
    std::string help_message(std::string_view error_message = "") const {
        std::string help = static_cast<const Child*>(this)->help_message_impl();
        if (error_message.empty()) {
            error_message = title_;
        }

        return util::join(error_message, '\n', help);
    }

	[[noreturn]] void exit_with_help(std::string_view error_message = "", int errc = 1) const {
        print_help(error_message);
        exit(errc);
    }

    void print_help(std::string_view error_message = "") const {
        std::cerr << help_message(error_message) << std::endl;
    }

    Child& title(std::string_view v) {
        title_ = util::str(v);
        return static_cast<Child&>(*this);
    }

    int parse(int argc, const char* argv[], parsing_error_policy err = parsing_error_policy::exit)
        const {
        try {
            return static_cast<const Child*>(this)->parse_impl(argc, argv);
        } catch (const parser_error& error) {
            switch (err) {
            case parsing_error_policy::exit:
                exit_with_help(error.what());
            case parsing_error_policy::rethrow:
                throw;
            }
        }

        /* suppress warnings */
        return 0;
    }

private:
    std::string title_;
};

template<typename T, bool HasDefault>
class stored_value;

template<typename T>
class stored_value<T, false> {
public:
    stored_value(T& dest)
        : dest_(dest) {
    }

protected:
    T& dest_;
};

template<typename T>
class stored_value<T, true> {
public:
    stored_value(T& dest, T&& default_val)
        : dest_(dest)
        , default_(std::forward<T>(default_val)) {
    }

protected:
    T& dest_;
    T default_;
};

template<typename T, bool HasDefault>
class value_handler : public stored_value<T, HasDefault> {
public:
    static_assert(std::is_move_assignable_v<T>,
        "T should be move assignable");
    static_assert(!HasDefault || std::is_move_constructible_v<T>,
        "T should be move constructible (to initialize default value holder)");
    static_assert(util::is_convertible_from_string_v<T>,
        "Cannot find std::istream& operator>>(std::istream&, T&)");

    using base = stored_value<T, HasDefault>;

    template<typename ...Args>
    value_handler(Args&&... args)
        : stored_value<T, HasDefault>(std::forward<Args>(args)...) {
    }

    void handle(T&& arg) override {
        base::dest_ = std::move(arg);
    }

    void handle_default() override {
        if constexpr (HasDefault) {
            base::dest_ = std::move(base::default_);
        }
    }
};

}

template<typename T>
auto value(T& dest) {
    return detail::stored_value<T, false>(dest);
}

template<typename T>
auto value(T& dest, T&& vdefault) {
    return detail::stored_value<T, true>(dest, vdefault);
}

namespace params {

template<bool Short>
struct short_name_tag {};

template<>
struct short_name_tag<true> {
    char short_name;
};

template<bool Long>
struct long_name_tag {};

template<>
struct long_name_tag<true> {
    std::string_view long_name;
};

template<bool Short, bool Long>
struct name_tag : public short_name_tag<Short>, long_name_tag<Long> {
    static_assert(Short || Long, "No name was specified");
};

template<typename T, bool Default>
struct value_tag {
    detail::value_handler<T, Default> handler;
};

template<typename T, bool Default>
struct value_tag {
    detail::value_handler<T, Default> handler;
};

struct help_tag {
    std::string_view help;
};

struct type_tag {
    std::string_view type;
};

template<bool Required>
struct required_tag {};

template<bool Repeatable>
struct repeatable_tag {};

template<typename Mapped>
struct mapped_tag {};


struct name {
    template<bool Short, bool Long>
    name_tag<Short, Long> operator=(name_tag<Short, Long> tag) {
        return tag;
    }
};

struct store {
    template<typename T, bool HasValue>
    auto operator>(store) {
        return { std::move(handler) };
    }
};

struct handle {
    template<typename Callable>
    handler_tag operator>(Callable&& callable) {
    }
};

}

inline constexpr params::name name;
inline constexpr params::store store;
inline constexpr params::handle handle;

template<typename ...Args>
struct mapped_type_impl;

template<>
struct mapped_type_impl<> {
    using type = std::string_view;
};

template<typename T, typename ...Args>
struct mapped_type_impl<params::mapped_tag<T>, Args...> {
    using type = T;
};

template<typename T, typename ...Args>
struct mapped_type_impl<T, Args...> {
    using type = typename mapped_type_impl<Args...>::type;
};

template<typename ...Args>
struct callable_type_impl;

template<typename T, typename ...Args>
struct callable_type_impl<params::callable_tag<T>, Args...> {
};

template<typename ...Args>
struct parser_traits {
    using mapped_type = typename mapped_type_impl<Args...>::type;
};

class parser : public detail::parser_base<parser> {
public:
    template<typename ...Args>
    parser& add(Args&& ...args) {
        using traits = parser_traits<Args...>;
    }
};

} // namespace cpparg
