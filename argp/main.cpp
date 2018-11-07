#include "argp.h"

#include <string>

int main(int argc, const char** argv) {
    argp::Parser parser("todo");
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
    parser.parse(argc, argv);
    parser.printHelp();
}
