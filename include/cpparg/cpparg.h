#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace cpparg {

namespace detail {

template<typename T>
T from_string(std::string_view s) {
    static std::istringstream ss;
    ss.exceptions(std::istringstream::failbit);
    ss.str(std::string(s.begin(), s.end()));
    T result;
    ss >> result;
    return result;
}

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
    processor(std::string_view lname)
        : lname_(lname.begin(), lname.end()) {
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
                    val = detail::from_string<Val>(sv);
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

    processor& argument_type(std::string_view type) {
        arg_type_ = std::string(type.begin(), type.end());
        return *this;
    }

    processor& default_argument(std::string_view default_arg) {
        has_default_argument_ = true;
        default_argument_ = std::string(default_arg.begin(), default_arg.end());
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
        if (arg.empty() && !has_default_argument_) {
            throw processor_error(name(), "argument required.");
        }
        if (arg.empty()) {
            handler_(default_argument_);
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
        } else if (has_default_argument_) {
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

    bool is_optional() const {
        return !required_;
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

        if (sname_ != EMPTY_SHORT_NAME) {
            result << "-" << sname_;
        }
        if (has_two_names) {
            result << ", ";
        }
        if (!lname_.empty()) {
            result << "--" << lname_;
        }
        if (!type().empty()) {
            result << " <" << type() << ">";
        }
        result << '\t';
        result << description_;
        if (has_default_argument_ && !flag_) {
            result << " [default = " << default_argument_ << "]";
        }

        return result.str();
    }

    std::string usage() const {
        std::stringstream result;

        if (is_optional()) {
            result << '[';
        }
        if (lname_.empty()) {
            result << "-" << std::string(1, sname_);
        } else {
            result << "--" << lname_;
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

    bool required_{false};
    bool flag_{false};

    char sname_{EMPTY_SHORT_NAME};
    std::string lname_;

    std::string arg_type_;
    std::string description_;
    std::string default_argument_;
    bool has_default_argument_{false};

    std::function<void(std::string_view)> handler_;
    std::function<void()> disable_flag_;
};

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
        (void)argc;
        (void)argv;
    }

    void exit_with_help(std::string_view error_message = "") const {
        print_help(error_message);
        exit(1);
    }

    void print_help(std::string_view error_message = "") const {
        if (!error_message.empty()) {
            std::cerr << error_message << "\n";
        } else {
            std::cerr << title_ << "\n";
        }
        std::cerr << "\nUsage:\n" << detail::OFFSET << program_;
        if (!mode_.empty()) {
            std::cerr << ' ' << mode_;
        }

        /* put required arguments first */
        std::vector<const processor*> sorted;
        std::transform(
            processors_.begin(), processors_.end(), std::back_inserter(sorted), [](const auto& p) {
                return std::addressof(p);
            });
        std::ptrdiff_t count = std::count_if(
            sorted.begin(), sorted.end(), [](const processor* p) { return !p->is_optional(); });
        std::nth_element(
            sorted.begin(),
            sorted.begin() + count,
            sorted.end(),
            [](const processor* lhs, const processor*) { return !lhs->is_optional(); });
        if (help_) {
            auto it = std::find(sorted.begin(), sorted.end(), help_);
            /* to preserve relative order */
            if (it != sorted.end()) {
                sorted.erase(it);
                sorted.push_back(help_);
            }
        }

        for (auto p : sorted) {
            std::cerr << ' ' << p->usage();
        }
        std::cerr << "\n\nOptions:\n";

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
            std::cerr << s << std::endl;
        }

        std::cerr << std::endl;
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
    std::unordered_map<std::string_view, processor*> long_;
    std::unordered_map<char, processor*> short_;
    processor* help_{nullptr};
};

} // namespace cpparg
