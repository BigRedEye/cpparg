#include "cpparg.h"

#include <string>

int main(int argc, const char** argv) {
    cpparg::Parser parser("test");
    parser.title("cpparg-test -- test for cpparg.");
    std::string s;
    parser.add('q', "qwe")
        .store(s)
        .defaultArgument("str")
        .argumentType("STRING")
        .description("some string");
    parser.add('a', "add")
        .handle([](std::string_view v) {
            std::cout << v << std::endl;
        })
        .required()
        .argumentType("FILE")
        .description("files to commit");
    parser.add("delete")
        .argumentType("DIR")
        .description("delete directory");
    parser.add('c')
        .description("do something");
    parser.add_help('h', "help");
    parser.parse(argc, argv);
    parser.printHelp();
}
