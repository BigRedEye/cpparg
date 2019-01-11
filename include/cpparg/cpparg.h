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

    void exit_with_help(std::string_view error_message = "", int errc = 1) const
        __attribute__((noreturn)) {
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
    }

private:
    std::string title_;
};

} // namespace detail

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
        return handle([handler{std::forward<Handler>(handler)}](std::string_view arg) mutable {
            handler(util::from_string<Arg>(arg));
        });
    }

    template<
        typename output_it,
        std::enable_if_t<std::is_convertible_v<
            std::output_iterator_tag,
            typename std::iterator_traits<output_it>::iterator_category>>* = nullptr>
    processor& append(output_it it) {
        return append_impl<typename std::iterator_traits<output_it>::value_type>(it);
    }

    template<typename Container>
    processor& append(Container& cont) {
        return append_impl<std::decay_t<decltype(*cont.begin())>>(std::back_inserter(cont));
    }

    processor& required() {
        required_ = true;
        return *this;
    }

    processor& optional() {
        required_ = false;
        return *this;
    }

    processor& repeatable() {
        repeatable_ = true;
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
        if (default_value_.empty()) {
            throw std::logic_error("Empty default value");
        }
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
        if (arg.empty()) {
            if (flag_) {
                handler_(arg);
            } else {
                throw processor_error(name(), "argument required.");
            }
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
            parse(default_value_);
        }
    }

    template<
        typename arg_t,
        typename output_it,
        std::enable_if_t<std::is_convertible_v<
            std::output_iterator_tag,
            typename std::iterator_traits<output_it>::iterator_category>>* = nullptr>
    processor& append_impl(output_it it) {
        if (!is_repeatable()) {
            throw std::logic_error(
                "Cannot use append with non-repeatable processor; call repeatable() before "
                "append()");
        }

        return handle<arg_t>([it](arg_t arg) mutable { *(it++) = std::move(arg); });
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

    bool is_repeatable() const {
        return repeatable_;
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
        if (is_repeatable()) {
            result << " (repeatable)";
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
    bool repeatable_{false};
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

    void parse(const std::vector<std::string_view>& args) const {
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

class parser : public detail::parser_base<parser> {
public:
    parser(std::string_view program)
        : program_(util::str(program)) {
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
        result.description("Print this help and exit").handle([this](std::string_view) {
            print_help();
            exit(0);
        });

        help_ = &result;

        return *this;
    }

    int parse_impl(int, const char* argv[]) const {
        size_t next_positional = 0;
        std::vector<std::string_view> free_args;

        std::unordered_set<const processor*> unused;
        for (const auto& p : processors_) {
            unused.insert(p.get());
        }

        bool was_free_arg_delimiter = false;

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

            auto it = unused.find(*p);
            if (it == unused.end() && !(*p)->is_repeatable()) {
                throw processor_error(
                    util::join("Option '", arg_parser.name(), "' is not repeatable"));
            } else if (it != unused.end()) {
                unused.erase(it);
            }
        }

        for (const processor* p : unused) {
            p->default_handler();
        }

        free_args_processor_.parse(free_args);

        return 0;
    }

    std::string help_message_impl() const {
        std::stringstream out;

        out << "\nUsage:\n" << detail::OFFSET << program_;

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
        util::normalize_tabs(opts, detail::TAB_WIDTH);
        for (auto& s : opts) {
            out << s << std::endl;
        }

        return out.str();
    }

private:
    template<typename... Args>
    processor& create_processor(Args&&... args) {
        processors_.emplace_back(std::make_unique<processor>(std::forward<Args>(args)...));
        processor& result = *processors_.back();

        auto try_to_insert = [](auto& map, const auto& key, const auto& value) {
            auto [it, inserted] = map.emplace(key, value);
            if (!inserted) {
                throw std::logic_error(
                    util::join("Cannot add option ", key, ": the name is already used"));
            }
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
    std::string title_;

    std::vector<std::unique_ptr<processor>> processors_;
    std::vector<processor*> positional_;
    std::unordered_map<std::string, processor*> long_;
    std::unordered_map<char, processor*> short_;
    std::optional<processor*> help_;

    free_args_processor free_args_processor_;
};

class command_handler {
public:
    command_handler(std::string_view name, bool is_default = true)
        : name_(util::str(name))
        , default_(is_default) {
    }

    template<typename F>
    command_handler& handle(F&& f) {
        using Result = std::invoke_result_t<F, int, const char*[]>;
        using IsVoid = std::is_same<std::decay_t<Result>, void>;

        static_assert(
            std::is_convertible_v<Result, int> || IsVoid::value,
            "Command handler should return either int or void");

        if constexpr (IsVoid::value) {
            handler_ = [func{std::forward<F>(f)}](int argc, const char* argv[]) -> int {
                func(argc, argv);
                return 0;
            };
        } else {
            handler_ = std::forward<F>(f);
        }
        return *this;
    }

    command_handler& description(std::string_view descr) {
        description_ = util::str(descr);
        return *this;
    }

    int operator()(int argc, const char* argv[]) const {
        return handler_(argc, argv);
    }

private:
    friend class command_parser;

    std::string help() const {
        std::string suffix = is_default() ? util::str(" [default]") : util::str("");
        return util::join(detail::OFFSET, name_, '\t', description_, suffix);
    }

    bool is_default() const {
        return default_;
    }

private:
    using command_func = std::function<int(int, const char*[])>;

    command_func handler_;
    std::string description_;
    std::string name_;
    bool default_;
};

class command_parser : public detail::parser_base<command_parser> {
public:
    command_parser(std::string_view name)
        : name_(util::str(name)) {
    }

    command_handler& command(std::string_view name) {
        if (name.empty()) {
            throw std::logic_error(util::join("Command name cannot be empty"));
        }
        return command_impl(name, false);
    }

    command_handler& default_command(std::string_view name) {
        if (default_) {
            throw std::logic_error("Cannot add two default commands");
        }
        command_handler& result = command_impl(name, true);
        default_ = &result;
        return result;
    }

    int parse_impl(int argc, const char* argv[]) const {
        std::string_view cmd = "";
        if (argc > 1) {
            cmd = argv[1];
        }

        auto it = command_by_name_.find(util::str(cmd));
        if (it == command_by_name_.end()) {
            if (cmd.empty()) {
                throw parser_error("Command name is required.");
            } else {
                throw parser_error(util::join("Unknown command '", cmd, "'."));
            }
        }

        return (*it->second)(argc - 1, argv + 1);
    }

    std::string help_message_impl() const {
        std::stringstream out;

        out << "\nUsage:\n" << detail::OFFSET << name_ << " <command> <command args>";

        out << "\n\nCommands:\n";

        std::vector<std::string> cmds;

        cmds.reserve(commands_.size());
        for (auto& ptr : commands_) {
            if (ptr->is_default()) {
                cmds.insert(cmds.begin(), ptr->help());
            } else {
                cmds.push_back(ptr->help());
            }
        }

        util::normalize_tabs(cmds, detail::TAB_WIDTH);

        for (auto& s : cmds) {
            out << s << std::endl;
        }

        return out.str();
    }

private:
    command_handler& command_impl(std::string_view name = "", bool is_default = false) {
        commands_.emplace_back(std::make_unique<command_handler>(name, is_default));
        command_handler& result = *commands_.back();

        auto [it, inserted] = command_by_name_.emplace(util::str(name), &result);
        if (!inserted) {
            throw std::logic_error(util::join("Multiple commands with same name '", name, "'"));
        }

        if (is_default) {
            command_by_name_.emplace("", &result);
        }

        return result;
    }

private:
    std::string name_;
    std::vector<std::unique_ptr<command_handler>> commands_;
    std::unordered_map<std::string, command_handler*> command_by_name_;
    std::optional<command_handler*> default_;
};

} // namespace cpparg
