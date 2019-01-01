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
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace cpparg {

namespace util {

inline std::string str(std::string_view view) {
    return std::string(view.begin(), view.end());
}

template<typename T>
inline T from_string(std::string_view s) {
    static std::istringstream ss;
    ss.exceptions(std::istringstream::failbit);
    ss.clear();
    ss.str(str(s));
    T result;
    ss >> result;
    return result;
}

template<typename T>
inline std::string to_string(T&& t) {
    static std::ostringstream os;
    os.clear();
    os.str("");
    os << t;
    return os.str();
}

inline bool starts_with(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

inline bool ends_with(std::string_view str, std::string_view suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), std::string_view::npos, suffix) == 0;
}

template<typename... Args>
inline std::string join(Args&&... args) {
    return ("" + ... + to_string(args));
}

} // namespace util

namespace detail {

static constexpr std::string_view OFFSET = "  ";
static constexpr size_t TAB_WIDTH = 4;

} // namespace detail

class processor_error : public std::runtime_error {
public:
    processor_error(const std::string& what_)
        : std::runtime_error(what_) {
    }

    processor_error(const std::string& name, const std::string& descr)
        : std::runtime_error("Cannot parse option " + name + ": " + descr) {
    }
};

class parser_error : public std::runtime_error {
public:
    parser_error(const std::string& what_)
        : std::runtime_error(what_) {
    }
};

class processor {
public:
    processor(size_t position, std::string_view name)
        : lname_(util::str(name))
        , position_(position) {
        if (util::starts_with(name, "-")) {
            throw std::logic_error("Option name cannot start with '-'");
        }
    }

    processor(std::string_view lname)
        : processor(NON_POSITIONAL, lname) {
    }

    processor(char sname, std::string_view lname)
        : processor(lname) {
        sname_ = sname;
        if (sname == '-') {
            throw std::logic_error("Option name cannot start with '-'");
        }
    }

    processor(processor&& other) = default;
    processor& operator=(processor&& other) = default;

    processor(const processor& other) = delete;
    processor& operator=(const processor& other) = delete;

    template<typename Val>
    processor& store(Val& val) {
        handler_ = [this, &val](std::string_view sv) {
            if (flag_) {
                /* --flag anything */
                val = true;
            } else {
                try {
                    val = util::from_string<Val>(sv);
                } catch (std::ios_base::failure& fail) {
                    throw processor_error(name(), fail.what());
                }
            }
        };
        if (flag_) {
            disable_flag_ = [&val] { val = false; };
        }
        return *this;
    }

    template<typename Handler>
    processor& handle(Handler&& handler) {
        static_assert(
            std::is_invocable_v<Handler, std::string_view>,
            "Handler should take std::string_view as the first argument");
        handler_ = std::forward<Handler>(handler);

        if (flag_) {
            disable_flag_ = [] {};
        }
        return *this;
    }

    template<typename Arg, typename Handler>
    processor& handle(Handler&& handler) {
        return handle([handler{std::forward<Handler>(handler)}](std::string_view arg) {
            handler(util::from_string<Arg>(arg));
        });
    }

    processor& required() {
        required_ = true;
        return *this;
    }

    processor& optional() {
        required_ = false;
        return *this;
    }

    processor& value_type(std::string_view type) {
        arg_type_ = util::str(type);
        return *this;
    }

    template<typename T>
    processor& default_value(T val) {
        has_default_value_ = true;
        default_value_ = util::to_string(val);
        return *this;
    }

    processor& description(std::string_view descr) {
        description_ = util::str(descr);
        return *this;
    }

private:
    friend class parser;

    processor& flag() {
        flag_ = true;
        return *this;
    }

    void parse(std::string_view arg = "") const {
        if (!handler_) {
            throw std::logic_error(
                "Cannot parse option " + name() +
                ": the handler was not set. Use either store() or process().");
        }
        if (arg.empty() && !has_default_value_ && !flag_) {
            throw processor_error(name(), "argument required.");
        }
        if (arg.empty() && !flag_) {
            handler_(default_value_);
        } else {
            handler_(arg);
        }
    }

    void default_handler() const {
        if (required_) {
            throw processor_error("Option " + name() + " is required.");
        } else if (flag_) {
            if (disable_flag_) {
                disable_flag_();
            } else {
                /* flag() was called after store() */
                throw std::logic_error("Do not call store() before flag().");
            }
        } else if (has_default_value_) {
            parse();
        }
    }

    char short_name() const {
        return sname_;
    }

    std::string_view long_name() const {
        return lname_;
    }

    std::string_view type() const {
        if (flag_) {
            return "flag";
        }
        return arg_type_;
    }

    bool is_required() const {
        return required_;
    }

    bool is_optional() const {
        return !required_;
    }

    bool is_positional() const {
        return position_ != NON_POSITIONAL;
    }

    std::string name() const {
        if (lname_.empty()) {
            /* option has only short name */
            return std::string(1, sname_);
        } else {
            return lname_;
        }
    }

    std::string help() const {
        std::stringstream result;

        result << detail::OFFSET;

        const bool has_two_names = !lname_.empty() && sname_ != EMPTY_SHORT_NAME;

        if (is_positional()) {
            result << lname_;
        } else {
            if (sname_ != EMPTY_SHORT_NAME) {
                result << "-" << sname_;
            }
            if (has_two_names) {
                result << ", ";
            }
            if (!lname_.empty()) {
                result << "--" << lname_;
            }
        }
        if (!type().empty()) {
            result << " <" << type() << ">";
        }
        result << '\t';
        result << description_;
        if (has_default_value_ && !flag_) {
            result << " [default = " << default_value_ << "]";
        }

        return result.str();
    }

    std::string usage() const {
        std::stringstream result;

        if (is_optional()) {
            result << '[';
        }
        if (is_positional()) {
            result << lname_;
        } else {
            if (lname_.empty()) {
                result << "-" << std::string(1, sname_);
            } else {
                result << "--" << lname_;
            }
        }
        if (!flag_ && !arg_type_.empty()) {
            result << " <" << arg_type_ << ">";
        }
        if (is_optional()) {
            result << ']';
        }

        return result.str();
    }

private:
    static constexpr char EMPTY_SHORT_NAME = '\0';
    static constexpr size_t NON_POSITIONAL = std::numeric_limits<size_t>::max();

    bool required_{false};
    bool flag_{false};
    bool has_default_value_{false};

    char sname_{EMPTY_SHORT_NAME};
    std::string lname_;

    size_t position_{NON_POSITIONAL};

    std::string arg_type_;
    std::string description_;
    std::string default_value_;

    std::function<void(std::string_view)> handler_;
    std::function<void()> disable_flag_;
};

class invalid_free_arguments_count : public processor_error {
public:
    invalid_free_arguments_count(size_t count, size_t maximum)
        : processor_error(util::join(
              "Invalid free arguments count, got ",
              count,
              " while maximum is ",
              maximum)) {
    }
};

class free_args_processor {
public:
    free_args_processor() = default;

    free_args_processor& max(size_t count) {
        max_count_ = count;
        return *this;
    }

    free_args_processor& unlimited() {
        return max(UNLIMITED);
    }

    template<typename Handler>
    free_args_processor& handle(Handler&& handler) {
        static_assert(
            std::is_invocable_v<Handler, const std::vector<std::string_view>&>,
            "Handler should take std::vector<std::string_view> as the first argument");
        handler_ = std::forward<Handler>(handler);
        return *this;
    }

    template<typename T>
    free_args_processor& store(std::vector<T>& free_args) {
        handler_ = [&free_args](const std::vector<std::string_view>& args) {
            for (auto sw : args) {
                free_args.push_back(util::from_string<T>(sw));
            }
        };
        return *this;
    }

    free_args_processor& name(std::string_view name) {
        name_ = util::str(name);
        return *this;
    }

    void parse(const std::vector<std::string_view>& args) {
        if (args.size() > max_count_) {
            throw invalid_free_arguments_count(args.size(), max_count_);
        }
        if (handler_) {
            handler_(args);
        }
    }

    size_t max_count() const {
        return max_count_;
    }

    std::string usage() const {
        if (max_count() > 0) {
            return util::join(name_, "...");
        } else {
            return "";
        }
    }

    std::string help() const {
        if (max_count() > 0) {
            return util::join(name_, "...");
        } else {
            return "";
        }
    }

private:
    static constexpr size_t UNLIMITED = std::numeric_limits<size_t>::max();

    size_t max_count_{0};
    std::string name_;
    std::function<void(const std::vector<std::string_view>&)> handler_;
};

namespace detail {

class argument_parser {
public:
    enum class arg_type { positional, short_name, long_name, free_arg, free_arg_delimiter };

    argument_parser(std::string_view arg, size_t position, size_t positional_count)
        : arg_(arg)
        , can_be_positilnal_(position < positional_count) {
    }

    arg_type type() const {
        if (util::starts_with(arg_, "--")) {
            /* ./command ... -- free args */
            if (arg_.size() == 2) {
                return arg_type::free_arg_delimiter;
            } else {
                return arg_type::long_name;
            }
        } else if (util::starts_with(arg_, "-")) {
            return arg_type::short_name;
        } else if (can_be_positilnal_) {
            return arg_type::positional;
        } else {
            return arg_type::free_arg;
        }
    }

    std::string_view name() const {
        switch (type()) {
        case arg_type::short_name:
            return arg_.substr(1);
        case arg_type::long_name:
            return arg_.substr(2);
        default:
            return arg_;
        }
    }

private:
    std::string_view arg_;
    bool can_be_positilnal_;
};

} // namespace detail

enum class parsing_error_policy {
    exit,
    rethrow,
};

class parser {
public:
    parser(std::string_view program, std::string_view mode = "")
        : program_(util::str(program))
        , mode_(util::str(mode)) {
    }

    parser& title(std::string_view v) {
        title_ = util::str(v);
        return *this;
    }

    processor& add(std::string_view lname) {
        return create_processor(lname);
    }

    processor& add(char sname, std::string_view lname = "") {
        return create_processor(sname, lname);
    }

    processor& flag(std::string_view lname) {
        return add(lname).flag();
    }

    processor& flag(char sname, std::string_view lname = "") {
        return add(sname, lname).flag();
    }

    processor& positional(std::string_view name) {
        processors_.emplace_back(std::make_unique<processor>(positional_.size(), name));
        processor& result = *processors_.back();
        positional_.push_back(&result);
        return result;
    }

    free_args_processor& free_arguments(std::string_view name) {
        return free_args_processor_.name(name);
    }

    parser& add_help(char sname, std::string_view lname = "") {
        if (help_) {
            throw std::logic_error("Cannot add two help options");
        }

        processor& result = create_processor(sname, lname);
        result.description("print this help and exit").handle([this](std::string_view) {
            print_help();
            exit(0);
        });

        help_ = &result;

        return *this;
    }

    void parse(int, const char* argv[], parsing_error_policy err = parsing_error_policy::exit) {
        size_t next_positional = 0;
        std::vector<std::string_view> free_args;

        std::unordered_set<const processor*> unused;
        for (const auto& p : processors_) {
            unused.insert(p.get());
        }

        bool was_free_arg_delimiter = false;
        try {
            while (*++argv) {
                detail::argument_parser arg_parser(*argv, next_positional, positional_.size());
                if (arg_parser.type() == detail::argument_parser::arg_type::free_arg ||
                    was_free_arg_delimiter) {
                    free_args.push_back(arg_parser.name());
                    continue;
                }

                std::optional<processor*> p;
                auto try_to_find = [&](const auto& map, const auto& name) -> decltype(p) {
                    auto it = map.find(name);
                    if (it != map.end()) {
                        return it->second;
                    } else {
                        return {};
                    }
                };

                switch (arg_parser.type()) {
                case detail::argument_parser::arg_type::short_name:
                    p = try_to_find(short_, arg_parser.name()[0]);
                    break;
                case detail::argument_parser::arg_type::long_name:
                    p = try_to_find(long_, util::str(arg_parser.name()));
                    break;
                case detail::argument_parser::arg_type::positional:
                    p = positional_[next_positional++];
                    break;
                case detail::argument_parser::arg_type::free_arg_delimiter:
                    was_free_arg_delimiter = true;
                    continue;
                default:
                    break;
                }

                std::string_view arg = "";

                const char* next = *(argv + 1);
                if (arg_parser.type() == detail::argument_parser::arg_type::positional) {
                    arg = arg_parser.name();
                } else if (next) {
                    if (!util::starts_with(next, "-")) {
                        arg = next;
                        ++argv;
                    }
                }

                if (p) {
                    (*p)->parse(arg);
                } else {
                    throw processor_error(util::join("Unknown option ", arg_parser.name(), "."));
                }

                unused.extract(*p);
            }

            for (const processor* p : unused) {
                p->default_handler();
            }

            free_args_processor_.parse(free_args);
        } catch (const processor_error& error) {
            switch (err) {
            case parsing_error_policy::exit:
                exit_with_help(error.what());
            case parsing_error_policy::rethrow:
                throw parser_error(error.what());
            }
        }
    }

    void exit_with_help(std::string_view error_message = "") const __attribute__((noreturn)) {
        print_help(error_message);
        exit(1);
    }

    std::string help_message(std::string_view error_message = "") const {
        std::stringstream out;

        if (!error_message.empty()) {
            out << error_message << "\n";
        } else {
            out << title_ << "\n";
        }
        out << "\nUsage:\n" << detail::OFFSET << program_;
        if (!mode_.empty()) {
            out << ' ' << mode_;
        }

        /* put required arguments first */
        std::vector<const processor*> sorted;
        std::transform(
            processors_.begin(), processors_.end(), std::back_inserter(sorted), [](const auto& p) {
                return p.get();
            });
        std::sort(sorted.begin(), sorted.end(), [](const processor* lhs, const processor* rhs) {
            if (lhs->is_positional() != rhs->is_positional()) {
                return rhs->is_positional();
            } else {
                return lhs->is_required();
            }
        });

        if (help_) {
            auto it = std::find(sorted.begin(), sorted.end(), *help_);
            if (it != sorted.end()) {
                sorted.erase(it);
            }
        }

        for (auto p : sorted) {
            out << ' ' << p->usage();
        }

        out << ' ' << free_args_processor_.usage();

        /* add help at first line */
        if (help_) {
            sorted.insert(sorted.begin(), *help_);
        }

        out << "\n\nOptions:\n";

        std::vector<std::string> opts;
        std::transform(sorted.begin(), sorted.end(), std::back_inserter(opts), [](auto p) {
            return p->help();
        });
        auto normailze_tabs = [&opts]() {
            size_t right = 0;
            for (auto& s : opts) {
                size_t cur = s.find_first_of('\t');
                if (cur != std::string::npos) {
                    right = std::max(cur, right);
                }
            }
            for (auto& s : opts) {
                size_t cur = s.find_first_of('\t');
                if (cur != std::string::npos) {
                    s.replace(cur, 1, right - cur + detail::TAB_WIDTH, ' ');
                }
            }
        };
        normailze_tabs();
        for (auto& s : opts) {
            out << s << std::endl;
        }

        return out.str();
    }

    void print_help(std::string_view error_message = "") const {
        std::cerr << help_message(error_message) << std::endl;
    }

private:
    template<typename... Args>
    processor& create_processor(Args&&... args) {
        processors_.emplace_back(std::make_unique<processor>(std::forward<Args>(args)...));
        processor& result = *processors_.back();

        auto try_to_insert = [](auto& map, const auto& key, const auto& value) {
            auto it = map.find(key);
            if (it != map.end()) {
                throw std::logic_error(
                    util::join("Cannot add option ", key, ": the name is already used"));
            }
            map.emplace(key, value);
        };

        if (!result.long_name().empty()) {
            try_to_insert(long_, util::str(result.long_name()), &result);
        }
        if (result.short_name() != processor::EMPTY_SHORT_NAME) {
            try_to_insert(short_, result.short_name(), &result);
        }

        return result;
    }

private:
    std::string program_;
    std::string mode_;
    std::string title_;

    std::vector<std::unique_ptr<processor>> processors_;
    std::vector<processor*> positional_;
    std::unordered_map<std::string, processor*> long_;
    std::unordered_map<char, processor*> short_;
    std::optional<processor*> help_;

    free_args_processor free_args_processor_;
};

} // namespace cpparg
