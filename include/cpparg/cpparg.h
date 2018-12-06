#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <limits>
#include <tuple>

namespace cpparg {

namespace string_utils {

template<typename T>
T from_string(std::string_view s) {
    static std::istringstream ss;
    ss.exceptions(std::istringstream::failbit);
    ss.clear();
    ss.str(std::string(s.begin(), s.end()));
    T result;
    ss >> result;
    return result;
}

template<typename T>
std::string to_string(T&& t) {
    static std::ostringstream os;
    os.clear();
    os.str("");
    os << t;
    return os.str();
}

bool starts_with(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

bool ends_with(std::string_view str, std::string_view suffix) {
    return str.size() >= suffix.size()
        && str.compare(str.size() - suffix.size(), std::string_view::npos, suffix) == 0;
}

template<typename ...Args>
std::string join(Args&&... args) {
    std::string s;
    auto printer = [&s] (const auto& val) {
        s += to_string(val);
        return s;
    };
    std::apply([&printer](const auto& ...val) {
        std::make_tuple(printer(val)...);
    }, std::tuple(args...));
    return s;
}

}

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

class processor {
public:
    processor(size_t position, std::string_view name)
        : position_(position)
        , lname_(name.begin(), name.end()) {
    }

    processor(std::string_view lname)
        : processor(NON_POSITIONAL, lname) {
    }

    processor(char sname, std::string_view lname)
        : processor(lname) {
        sname_ = sname;
    }

    template<typename Val>
    processor& store(Val& val) {
        handler_ = [this, &val](std::string_view sv) {
            if (flag_) {
                /* --flag foo */
                val = true;
            } else {
                try {
                    val = string_utils::from_string<Val>(sv);
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
        return *this;
    }

    processor& required() {
        required_ = true;
        return *this;
    }

    processor& optional() {
        required_ = false;
        return *this;
    }

    processor& flag() {
        flag_ = true;
        return *this;
    }

    processor& value_type(std::string_view type) {
        arg_type_ = std::string(type.begin(), type.end());
        return *this;
    }

    processor& default_value(std::string_view default_val) {
        has_default_value_ = true;
        default_value_ = std::string(default_val.begin(), default_val.end());
        return *this;
    }

    processor& description(std::string_view descr) {
        description_ = std::string(descr.begin(), descr.end());
        return *this;
    }

private:
    friend class parser;

    void parse(std::string_view arg = "") {
        if (!handler_) {
            throw std::logic_error(
                "Cannot parse option " + name() +
                ": the handler was not set. Use either store() or process().");
        }
        if (arg.empty() && !has_default_value_) {
            throw processor_error(name(), "argument required.");
        }
        if (arg.empty()) {
            handler_(default_value_);
        } else {
            handler_(arg);
        }
    }

    void empty_option_handler() {
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
            return std::string(sname_, 1);
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
    static constexpr size_t NON_POSITIONAL = 0;

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

class free_args_processor {
public:
    class invalid_free_arguments_count : public std::runtime_error {
    public:
        invalid_free_arguments_count(size_t count, size_t maximum)
            : std::runtime_error(
                string_utils::join("Invalid free arguments count, got ", count, " while maximum is ", maximum)) {
        }
    };

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
                free_args.push_back(string_utils::from_string<T>(sw));
            }
        };
        return *this;
    }

    free_args_processor& name(std::string_view name) {
        name_ = std::string(name.begin(), name.end());
        return *this;
    }

    void parse(const std::vector<std::string_view>& args) {
        if (args.size() > max_count_) {
            throw invalid_free_arguments_count(args.size(), max_count_);
        }
    }

    size_t max_count() const {
        return max_count_;
    }

    std::string usage() const {
        if (max_count() > 0) {
            return string_utils::join(name_, "...");
        } else {
            return "";
        }
    }

    std::string help() const {
        if (max_count() > 0) {
            return string_utils::join(name_, "...");
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
    enum class arg_type {
        positional,
        keyword,
        free_arg
    };

    argument_parser(std::string_view arg, size_t position, size_t positional_count)
        : arg_(arg)
        , can_be_positilnal_(position < positional_count) {
    }

    arg_type type() const {
        if (string_utils::starts_with(arg_, "-")) {
            return arg_type::keyword;
        } else if (can_be_positilnal_) {
            return arg_type::positional;
        } else {
            return arg_type::free_arg;
        }
    }

private:
    std::string_view arg_;
    bool can_be_positilnal_;
};

}

class parser {
public:
    parser(std::string_view program, std::string_view mode = "")
        : program_(program.begin(), program.end())
        , mode_(mode.begin(), mode.end()) {
    }

    parser& title(std::string_view v) {
        title_ = std::string(v.begin(), v.end());
        return *this;
    }

    processor& add(std::string_view lname) {
        return create_processor(lname);
    }

    processor& add(char sname, std::string_view lname = "") {
        return create_processor(sname, lname);
    }

    processor& positional(std::string_view name) {
        processors_.emplace_back(processors_.size(), name);
        positional_.push_back(std::addressof(processors_.back()));
        return processors_.back();
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

    void parse(int argc, const char* argv[]) {
        size_t next_positional = 0;
        while (*++argv) {
            detail::argument_parser arg_parser(*argv, next_positional, positional_.size());
            switch (arg_parser.type()) {
            case detail::argument_parser::arg_type::keyword:
                break;
            default:
                break;
            }
        }
    }

    void exit_with_help(std::string_view error_message = "") const {
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
                return std::addressof(p);
            });
        std::sort(sorted.begin(), sorted.end(), [](const processor* lhs, const processor* rhs) {
            if (lhs->is_positional() != rhs->is_positional()) {
                return rhs->is_positional();
            } else {
                return lhs->is_required();
            }
        });

        if (help_) {
            auto it = std::find(sorted.begin(), sorted.end(), help_);
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
            sorted.insert(sorted.begin(), help_);
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
        processors_.emplace_back(std::forward<Args>(args)...);
        processor& result = processors_.back();

        auto fail_if_exists = [](auto& map, const auto& key, const auto& value) {
            auto it = map.find(key);
            if (it != map.end()) {
                std::stringstream err;
                err << "Cannot add option " << key << ": the name is already used";
                throw std::logic_error(err.str());
            }
            map.emplace(key, value);
        };

        fail_if_exists(long_, result.long_name(), &result);
        fail_if_exists(short_, result.short_name(), &result);

        return result;
    }

private:
    std::string program_;
    std::string mode_;
    std::string title_;

    std::vector<processor> processors_;
    std::vector<processor*> positional_;
    std::unordered_map<std::string_view, processor*> long_;
    std::unordered_map<char, processor*> short_;
    processor* help_{nullptr};

    free_args_processor free_args_processor_;
};

} // namespace cpparg
