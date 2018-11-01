namespace argp {

namespace detail {

template<typename T>
T from_string(const std::string& s) {
    std::stringstream ss;
    ss.str(s);
    T result;
    ss >> result;
    return result;
}

} // namespace detail

template<typename T>
class Values {
public:
};

class Processor {
public:
    Processor(char sname, std::string_view lname)
        : shortName_(sname)
        , longName_(lname.data(), lname.size())
    {}

    template<typename ...Vals>
    Processor& store(Vals& ...vals) {
        auto tuple = std::forward_as_tuple<Vals>(vals...);
        auto pusher = [this] (auto& val) {
            processors_.push_back([&val, this, i=processors_.size()] (std::string_view v) {
                if (v.empty()) {
                    if (defaults.size() > i) {
                    } else {

                    }
                }
            });
        };
        std::apply(tuple, pusher);
    }

protected:
    void parse(const std::vector<std::string_view>& args) {
        size_t i = 0;
        for (; i < std::min(args.size(), processors_.size()); ++i) {
            processors_[i](args[i]);
        }
        for (; i < processors_.size(); ++i) {
            if (i < defaults_.size()) {
                defaults_[i]();
            } else {
                throw std::runtime_error("Cannot set default value for argument " << longName_);
            }
        }
    }

private:
    char shortName_;
    std::string longName_;
    std::function<void(std::string_view)> handler_;
    std::function<void()> defaulter_;
    std::vector<std::function<void(std::string_view)>> processors_;
    std::vector<std::function<void(std::string_view)>> defaults_;
};

class Parser {
public:
    Parser() = default;

    Parser& mode(std::string_view mode = "");
    Processor& add(std::string_view lname);
    Processor& add(char sname, std::string_view lname = "");
    void parse(int argc, const char* argv[]);

private:
    std::string mode_;

    struct ModeProcessor {
        std::unordered_map<std::string, Processor*> byName_;
        std::vector<Processor> processors_;
    };

    std::unordered_map<std::string, ModeProcessor> processors_;
};

} // namespace argp
