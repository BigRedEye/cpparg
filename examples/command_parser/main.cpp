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

int main(int argc, const char* argv[]) {
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
