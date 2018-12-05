#include <gtest/gtest.h>
#include <cpparg/cpparg.h>

TEST(string_utils, join) {
    EXPECT_EQ("1 2, qwe, 3", cpparg::string_utils::join(1, ' ', 2, ", ", "qw", 'e', ", ", 3ull));
    EXPECT_EQ("", cpparg::string_utils::join());
    EXPECT_EQ("123123123", cpparg::string_utils::join('1', "23", 123123));
}
