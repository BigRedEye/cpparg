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

    parser.parse(argc, argv);

    ASSERT_EQ(i, 0xdeadface);
    ASSERT_EQ(s, "some string");
    ASSERT_EQ(d, 3.141592653589);
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

    parser.parse(argc, argv);

    ASSERT_EQ(i, 123);
    ASSERT_EQ(d, 1.41421356);
}

TEST(parser, default_arguments) {
    cpparg::parser parser("parser::default_arguments test");
    parser.title("Test parser with default arguments");

    unsigned i = 0xdeadface;
    double d = 3.141592653589;
    std::string s = "before";

    parser.add('i').store(i).default_value("228");
    parser.add('d', "double").store(d).default_value("1.41421356");
    parser.add('s', "string").store(s).default_value("after");

    cpparg::test::args_builder builder("./program");
    builder
        .add("-i")
        .add("--double")
        .add("--string");
    auto [argc, argv] = builder.get();

    parser.parse(argc, argv);

    ASSERT_EQ(i, 228);
    ASSERT_EQ(d, 1.41421356);
    ASSERT_EQ(s, "after");
}

