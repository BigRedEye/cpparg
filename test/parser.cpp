#include <gtest/gtest.h>
#include <cpparg/cpparg.h>

#include "args_builder.h"

TEST(parser, no_arguments) {
    cpparg::parser parser("parser::no_arguments test");
    parser.title("Test parser with no arguments");

    unsigned i = 0xdeadface;
    std::string s = "some string";
    double d = 3.141592653589;

    parser.add('i', "int").store(i);
    parser.add('d', "double").store(d);
    parser.add('s', "string").store(s);

    cpparg::test::args_builder builder("./program");
    auto [argc, argv] = builder.get();

    parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow);

    EXPECT_EQ(i, 0xdeadface);
    EXPECT_EQ(s, "some string");
    EXPECT_EQ(d, 3.141592653589);
}

TEST(parser, numbers_parsing) {
    cpparg::parser parser("parser::numbers_parsing test");
    parser.title("Test parser with numeric arguments");

    unsigned i = 0xdeadface;
    double d = 3.141592653589;

    parser.add('i', "int").store(i);
    parser.add('d', "double").store(d);

    cpparg::test::args_builder builder("./program");
    builder
        .add("-i", "123")
        .add("--double", "1.41421356");
    auto [argc, argv] = builder.get();

    parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow);

    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 1.41421356);
}

TEST(parser, default_arguments) {
    cpparg::parser parser("parser::default_arguments test");
    parser.title("Test parser with default arguments");

    unsigned i = 0xdeadface;
    double d = 3.141592653589;
    std::string s = "before";

    parser.add('i').store(i).default_value(228);
    parser.add('d', "double").store(d).default_value("1.41421356");
    parser.add('s', "string").store(s).default_value("after");

    cpparg::test::args_builder builder("./program");
    builder
        .add("-i")
        .add("--double")
        .add("--string");
    auto [argc, argv] = builder.get();

    parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow);

    EXPECT_EQ(i, 228);
    EXPECT_DOUBLE_EQ(d, 1.41421356);
    EXPECT_EQ(s, "after");
}

TEST(parser, handlers) {
    cpparg::parser parser("parser::handlers test");
    parser.title("Test parser handlers");

    size_t calls = 0;

    auto handler = [&](auto){
        ++calls;
    };
    parser.add('i').handle<int>(handler).default_value(123);
    parser.add('d', "double").handle<int>(handler).default_value(1.41421356);
    parser.add('s', "string").handle<std::string>(handler).default_value("after");

    cpparg::test::args_builder builder("./program");
    builder
        .add("-i")
        .add("--double")
        .add("--string");
    auto [argc, argv] = builder.get();

    parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow);

    EXPECT_EQ(calls, 3);
}

TEST(parser, free_arguments) {
    cpparg::parser parser("parser::free_arguments test");
    parser.title("Test parser with free arguments");

    int sum = 0;

    parser.add('i').handle([](auto){}).default_value(0);
    parser.add('d', "double").handle([](auto){}).default_value(0);
    parser.add('s', "string").handle([](auto){}).default_value(0);
    parser.free_arguments("numbers").unlimited().handle([&](const auto& files) {
        for (std::string_view v : files) {
            sum += cpparg::util::from_string<int>(v);
        }
    });

    cpparg::test::args_builder builder("./program");
    builder
        .add("-i")
        .add("--double")
        .add("123").add("15").add("1024");
    auto [argc, argv] = builder.get();

    parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow);

    EXPECT_EQ(sum, 1039);
}

TEST(parser, free_arguments_delimiter) {
    cpparg::parser parser("parser::free_arguments_delimiter test");
    parser.title("Test free arguments delimiter");

    int sum = 0;

    parser.add('i').handle([](auto){}).default_value(0);
    parser.add('d', "double").handle([](auto){}).default_value(0);
    parser.add('s', "string").handle([](auto){}).default_value(0);
    parser.free_arguments("numbers").unlimited().handle([&](const auto& files) {
        for (std::string_view v : files) {
            sum += cpparg::util::from_string<int>(v);
        }
    });

    cpparg::test::args_builder builder("./program");
    builder
        .add("-i")
        .add("--double")
        .add("--").add("123").add("15").add("1024");
    auto [argc, argv] = builder.get();

    parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow);

    EXPECT_EQ(sum, 1162);
}

TEST(parser, positional) {
    cpparg::parser parser("parser::positional test");
    parser.title("Test parser with positional arguments");

    int x = 0;
    unsigned i = 0xdeadface;
    double d = 3.141592653589;
    std::string s = "before";

    parser.positional("int").store(i);
    parser.positional("double").store(d);
    parser.positional("string").store(s);
    parser.positional("default").store(x).default_value(-228);

    cpparg::test::args_builder builder("./program");
    builder
        .add("123")
        .add("1.41421356")
        .add("after");
    auto [argc, argv] = builder.get();

    parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow);

    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 1.41421356);
    EXPECT_EQ(s, "after");
    EXPECT_EQ(x, -228);
}

TEST(parser, flags) {
    cpparg::parser parser("parser::flags test");
    parser.title("Test flags");

    bool a = true;
    bool f = false;
    bool d = false;
    bool e = false;
    parser.flag('a').store(a);
    parser.flag('b', "boo").handle([](auto) { ASSERT_TRUE(true); });
    parser.flag('c').handle([](auto) { ASSERT_FALSE(true); });
    parser.flag('d').store(d).default_value(true);
    parser.flag('e').store(e).default_value(false);
    parser.flag('f', "foo").store(f).default_value(true);

    cpparg::test::args_builder builder("./program");
    builder
        .add("--boo")
        .add("-d")
        .add("--foo");
    auto [argc, argv] = builder.get();

    parser.parse(argc, argv, cpparg::parsing_error_policy::rethrow);

    EXPECT_FALSE(a);
    EXPECT_TRUE(d);
    EXPECT_FALSE(e);
    EXPECT_TRUE(f);
}
