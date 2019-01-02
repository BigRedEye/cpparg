#include "args_builder.h"
#include <cpparg/cpparg.h>

#include <gtest/gtest.h>

TEST(command_parser, simple) {
    cpparg::command_parser parser("./path-to-program");
    parser.title("Test command parser");
    parser
        .command("init")
        .description("Initialize empty reposiztory")
        .handle([](int, const char*[]) {
            FAIL();
        });
    parser
        .command("commit")
        .description("Commit files")
        .handle([](int, const char*[]) {
            return 123;
        });

    cpparg::test::args_builder builder("./program");
    builder.add("commit");
    auto [argc, argv] = builder.get();
    ASSERT_EQ(parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow), 123);
}

TEST(command_parser, deafult_handler) {
    cpparg::command_parser parser("./path-to-program");
    parser.title("Test command parser");
    parser
        .command("init")
        .description("Initialize empty reposiztory")
        .handle([](int, const char*[]) {
            FAIL();
        });
    parser
        .default_command("commit")
        .description("Commit files")
        .handle([](int, const char*[]) {
            return 282;
        });

    cpparg::test::args_builder builder("./program");
    auto [argc, argv] = builder.get();
    ASSERT_EQ(parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow), 282);
}

TEST(command_parser, nested_commands) {
    cpparg::command_parser parser("./path-to-program");
    parser.title("Test command parser");
    parser
        .command("test")
        .description("Manage tests")
        .handle([](int argc, const char* argv[]) {
            cpparg::command_parser cmds("./path-to-program init");
            cmds.command("run").description("Run tests").handle([](auto, auto) { FAIL(); });
            cmds.command("add").description("Add test").handle([](auto, auto) { return 1337; });

            return cmds.parse(argc, argv, cpparg::parsing_error_policy::rethrow);
        });
    parser
        .default_command("commit")
        .description("Commit files")
        .handle([](int, const char*[]) {
            FAIL();
        });

    cpparg::test::args_builder builder("./program");
    builder.add("test").add("add");
    auto [argc, argv] = builder.get();
    ASSERT_EQ(parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow), 1337);
}

TEST(command_parser, nested_parsers) {
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

            EXPECT_NO_THROW(args.parse(argc, argv, cpparg::parsing_error_policy::rethrow));

            EXPECT_EQ(s, "qwe");
            EXPECT_EQ(i, 228);
        });
    parser
        .default_command("commit")
        .description("Commit files")
        .handle([](int, const char*[]) {
            FAIL();
        });

    cpparg::test::args_builder builder("./program");
    builder.add("test").add("--int", "228");
    auto [argc, argv] = builder.get();
    ASSERT_NO_THROW(parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow));
}
