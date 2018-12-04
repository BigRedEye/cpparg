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

static constexpr std::string_view offset = "  ";
static constexpr size_t TabWidth = 4;

} // namespace detail

// using namespace std::string_literals;

class ProcessorError : public std::runtime_error {
public:
    ProcessorError(const std::string& what_)
        : std::runtime_error(what_) {
    }

    ProcessorError(const std::string& name, const std::string& descr)
        : std::runtime_error("Cannot parse option " + name + ": " + descr) {
    }
};

class Processor {
public:
    Processor(std::string_view lname)
        : lname_(lname.begin(), lname.end()) {
    }

    Processor(char sname, std::string_view lname)
        : Processor(lname) {
        sname_ = sname;
    }

    template<typename Val>
    Processor& store(Val& val) {
        handler_ = [this, &val](std::string_view sv) {
            if (flag_) {
                /* --flag foo */
                val = true;
            } else {
                try {
                    val = detail::from_string<Val>(sv);
                } catch (std::ios_base::failure& fail) {
                    throw ProcessorError(name(), fail.what());
                }
            }
        };
        if (flag_) {
            disableFlag_ = [&val] { val = false; };
        }
        return *this;
    }

    template<typename Handler>
    Processor& handle(Handler&& handler) {
        static_assert(
            std::is_invocable_v<Handler, std::string_view>,
            "Handler should take std::string_view as the first argument");
        handler_ = std::forward<Handler>(handler);
        return *this;
    }

    Processor& required() {
        required_ = true;
        return *this;
    }

    Processor& optional() {
        required_ = false;
        return *this;
    }

    Processor& flag() {
        flag_ = true;
        return *this;
    }

    Processor& argumentType(std::string_view type) {
        argType_ = std::string(type.begin(), type.end());
        return *this;
    }

    Processor& defaultArgument(std::string_view defaultArg) {
        hasDefaultArgument_ = true;
        defaultArgument_ = std::string(defaultArg.begin(), defaultArg.end());
        return *this;
    }

    Processor& description(std::string_view descr) {
        description_ = std::string(descr.begin(), descr.end());
        return *this;
    }

private:
    friend class Parser;

    void parse(std::string_view arg = "") {
        if (!handler_) {
            throw std::logic_error(
                "Cannot parse option " + name() + ": the handler was not set. Use either store() or process().");
        }
        if (arg.empty() && !hasDefaultArgument_) {
            throw ProcessorError(name(), "argument required.");
        }
        if (arg.empty()) {
            handler_(defaultArgument_);
        } else {
            handler_(arg);
        }
    }

    void emptyOptionHandler() {
        if (required_) {
            throw ProcessorError("Option " + name() + " is required.");
        } else if (flag_) {
            if (disableFlag_) {
                disableFlag_();
            } else {
                /* flag() was called after store() */
                throw std::logic_error("Do not call store() before flag().");
            }
        } else if (hasDefaultArgument_) {
            parse();
        }
    }

    char shortName() const {
        return sname_;
    }

    std::string_view longName() const {
        return lname_;
    }

    std::string_view type() const {
        if (flag_) {
            return "flag";
        }
        return argType_;
    }

    bool isOptional() const {
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

        result << detail::offset;

        const bool hasTwoNames = !lname_.empty() && sname_ != EmptyShortName;

        if (sname_ != EmptyShortName) {
            result << "-" << sname_;
        }
        if (hasTwoNames) {
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
        if (hasDefaultArgument_ && !flag_) {
            result << " [default = " << defaultArgument_ << "]";
        }

        return result.str();
    }

    std::string usage() const {
        std::stringstream result;

        if (isOptional()) {
            result << '[';
        }
        if (lname_.empty()) {
            result << "-" << std::string(1, sname_);
        } else {
            result << "--" << lname_;
        }
        if (!flag_ && !argType_.empty()) {
            result << " <" << argType_ << ">";
        }
        if (isOptional()) {
            result << ']';
        }

        return result.str();
    }

private:
    static constexpr char EmptyShortName = '\0';

    bool required_{false};
    bool flag_{false};

    char sname_{EmptyShortName};
    std::string lname_;

    std::string argType_;
    std::string description_;
    std::string defaultArgument_;
    bool hasDefaultArgument_{false};

    std::function<void(std::string_view)> handler_;
    std::function<void()> disableFlag_;
};

class Parser {
public:
    Parser(std::string_view program, std::string_view mode = "")
        : program_(program.begin(), program.end())
        , mode_(mode.begin(), mode.end()) {
    }

    Parser& title(std::string_view v) {
        title_ = std::string(v.begin(), v.end());
        return *this;
    }

    Processor& add(std::string_view lname) {
        return createProcessor(lname);
    }

    Processor& add(char sname, std::string_view lname = "") {
        return createProcessor(sname, lname);
    }

    Parser& add_help(char sname, std::string_view lname = "") {
        if (help_) {
            throw std::logic_error("Cannot add two help options");
        }
        Processor& result = createProcessor(sname, lname);
        result.description("print this help and exit").handle([this](std::string_view) {
            printHelp();
            exit(0);
        });

        help_ = &result;

        return *this;
    }

    void parse(int argc, const char* argv[]) {
        (void)argc;
        (void)argv;
    }

    void exitWithHelp(std::string_view errorMessage = "") const {
        printHelp(errorMessage);
        exit(1);
    }

    void printHelp(std::string_view errorMessage = "") const {
        if (!errorMessage.empty()) {
            std::cerr << errorMessage << "\n";
        } else {
            std::cerr << title_ << "\n";
        }
        std::cerr << "\nUsage:\n" << detail::offset << program_;
        if (!mode_.empty()) {
            std::cerr << ' ' << mode_;
        }

        /* put required arguments first */
        std::vector<const Processor*> sorted;
        std::transform(processors_.begin(), processors_.end(), std::back_inserter(sorted), [](const auto& p) {
            return std::addressof(p);
        });
        std::ptrdiff_t count =
            std::count_if(sorted.begin(), sorted.end(), [](const Processor* p) { return !p->isOptional(); });
        std::nth_element(
            sorted.begin(), sorted.begin() + count, sorted.end(), [](const Processor* lhs, const Processor*) {
                return !lhs->isOptional();
            });
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
        std::transform(sorted.begin(), sorted.end(), std::back_inserter(opts), [](auto p) { return p->help(); });
        auto normailzeTabs = [&opts]() {
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
                    s.replace(cur, 1, right - cur + detail::TabWidth, ' ');
                }
            }
        };
        normailzeTabs();
        for (auto& s : opts) {
            std::cerr << s << std::endl;
        }

        std::cerr << std::endl;
    }

private:
    template<typename... Args>
    Processor& createProcessor(Args&&... args) {
        processors_.emplace_back(std::forward<Args>(args)...);
        Processor& result = processors_.back();

        auto failIfExists = [](auto& map, const auto& key, const auto& value) {
            auto it = map.find(key);
            if (it != map.end()) {
                std::stringstream err;
                err << "Cannot add option " << key << ": the name is already used";
                throw std::logic_error(err.str());
            }
            map.emplace(key, value);
        };

        failIfExists(long_, result.longName(), &result);
        failIfExists(short_, result.shortName(), &result);

        return result;
    }

private:
    std::string program_;
    std::string mode_;
    std::string title_;

    std::vector<Processor> processors_;
    std::unordered_map<std::string_view, Processor*> long_;
    std::unordered_map<char, Processor*> short_;
    Processor* help_{nullptr};
};

} // namespace cpparg
