#include <cpparg/cpparg.h>
#include <iostream>

int run_handler(int argc, const char* argv[]) {
    cpparg::parser parser("run command");
    
    parser.add('e', "executable").required().description("Executable to run").handle([](auto s) {
        std::cout << "Executable: " << s << std::endl;
    });

    parser.parse(argc, argv);

    return 0;
}

int qmain(int argc, const char* argv[]) {
    cpparg::command_parser cmds("./program");
    cmds
        .command("run").description("Run executable").handle(run_handler);
    cmds
        .command("test").description("Manage tests").handle([](int argc, const char* argv[]) {
            cpparg::command_parser commands("test commands");
            commands.command("run").description("Run tests").handle([](auto, auto) {
                std::cout << "test run called" << std::endl;
                return 0;
            });

            commands.command("add").description("Add new test").handle([](auto, auto) {
                std::cout << "test add called" << std::endl;
                return 0;
            });

            return commands.parse(argc, argv);
        });

    return cmds.parse(argc, argv);
}

int main(int argc, const char* argv[]) {
    cpparg::command_parser parser("./path-to-program");
    parser.title("Test command parser");
    parser
        .command("test")
        .description("Manage tests")
        .handle([](int argc, const char* argv[]) {
            cpparg::parser args("./path-to-program init");

            int i;
            std::string s;

            args.add('i', "int").default_value(123).store(i);
            args.add("string").default_value("qwe").store(s);

            args.parse(argc, argv, cpparg::parsing_error_policy::rethrow);
        });
    parser
        .default_command("commit")
        .description("Commit files")
        .handle([](int, const char*[]) {
            std::cout << "commit" << std::endl;
        });

    return parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow);
}
