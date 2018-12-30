#include <cpparg/cpparg.h>

#include <string>
#include <map>

int qmain(int argc, const char** argv) {
    cpparg::parser parser("example");
    parser.title("cpparg-example -- example usage of cpparg.");
    std::string s;
    parser.add('q', "qwe")
        .store(s)
        .default_value("str")
        .value_type("STRING")
        .description("some string");
    parser.add('a', "add")
        .handle([](std::string_view v) { std::cout << v << std::endl; })
        .required()
        .value_type("FILE")
        .description("files to commit");
    int i;
    parser.positional("i")
        .store(i)
        .required()
        .value_type("INTEGER")
        .description("positional interger");
    parser.add("delete").value_type("DIR").description("delete directory");
    parser.add('c').description("do something");
    parser.add_help('h', "help");
    std::vector<int> free_args;
    parser.free_arguments("files").unlimited().store(free_args);
    parser.parse(argc, argv);
    parser.print_help();
}

int main(int argc, const char** argv) {
    cpparg::parser parser("parser::flags test");
    parser.title("Test flags");

    bool a = true;
    bool f = false;
    bool d = false;
    bool e = false;
    parser.flag('a').store(a);
    // parser.flag('b', "boo").handle([](auto) { std::cout << "WTF" << std::endl; });
    parser.flag('c').handle([](auto) { std::cout << "WTF" << std::endl; });
    parser.flag('d').store(d).default_value(true);
    parser.flag('e').store(e).default_value(false);
    parser.flag('f', "foo").store(f).default_value(true);

    parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow);
}
