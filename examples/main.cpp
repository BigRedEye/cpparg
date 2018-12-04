#include <cpparg/cpparg.h>

#include <string>

int main(int argc, const char** argv) {
    cpparg::parser parser("test");
    parser.title("cpparg-test -- test for cpparg.");
    std::string s;
    parser.add('q', "qwe")
        .store(s)
        .default_argument("str")
        .argument_type("STRING")
        .description("some string");
    parser.add('a', "add")
        .handle([](std::string_view v) { std::cout << v << std::endl; })
        .required()
        .argument_type("FILE")
        .description("files to commit");
    parser.add("delete").argument_type("DIR").description("delete directory");
    parser.add('c').description("do something");
    parser.add_help('h', "help");
    parser.parse(argc, argv);
    parser.print_help();
}
