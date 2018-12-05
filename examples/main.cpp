#include <cpparg/cpparg.h>

#include <string>
#include <map>

int main(int argc, const char** argv) {
    cpparg::parser parser("test");
    parser.title("cpparg-test -- test for cpparg.");
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
