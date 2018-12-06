#include <gtest/gtest.h>
#include <cpparg/cpparg.h>

namespace su = cpparg::string_utils;

TEST(string_utils, join) {
    EXPECT_EQ("1 2, qwe, 3", su::join(1, ' ', 2, ", ", "qw", 'e', ", ", 3ull));
    EXPECT_EQ("", su::join());
    EXPECT_EQ("123123123", su::join('1', "23", 123123));
}

struct dummy {
    int n;
    double d;
    std::string s;
};

std::ostream& operator<<(std::ostream& os, const dummy& obj) {
    os << obj.n << ' ' << obj.d << ' ' << obj.s;
    return os;
}

std::istream& operator>>(std::istream& is, dummy& obj) {
    is >> obj.n >> obj.d >> obj.s;
    return is;
}

bool operator==(const dummy& lhs, const dummy& rhs) {
    return lhs.n == rhs.n && lhs.d == rhs.d && lhs.s == rhs.s;
}

TEST(string_utils, to_string) {
    EXPECT_EQ("1", su::to_string(1));
    EXPECT_EQ("abc", su::to_string("abc"));
    EXPECT_EQ("c", su::to_string('c'));
    EXPECT_EQ("0.1", su::to_string(0.1));
    EXPECT_EQ("-123", su::to_string(-123));
    EXPECT_EQ("15 3.14 name", su::to_string(dummy{ 15, 3.14, "name" }));
}

TEST(string_utils, from_string) {
    EXPECT_EQ(1, su::from_string<short>("1"));
    EXPECT_EQ("abc", su::from_string<std::string>("abc"));
    EXPECT_EQ('c', su::from_string<char>("c"));
    EXPECT_EQ(0.1, su::from_string<double>("0.1"));
    EXPECT_EQ(-123, su::from_string<int>("-123"));
    EXPECT_EQ((dummy{ 15, 3.14, "name"}), su::from_string<dummy>("15 3.14 name"));
}

TEST(string_utils, starts_with) {
    EXPECT_TRUE(su::starts_with("123", "12"));
    EXPECT_TRUE(su::starts_with("abc  c", ""));
    EXPECT_TRUE(su::starts_with("abc  c", "abc "));
    EXPECT_TRUE(su::starts_with("", ""));
    EXPECT_FALSE(su::starts_with("abc  c", "abc c"));
    EXPECT_FALSE(su::starts_with("", "cd"));
}

TEST(string_utils, ends_with) {
    using namespace std::literals;
    EXPECT_TRUE(su::ends_with("123", "23"));
    EXPECT_TRUE(su::ends_with("abc  c", ""));
    EXPECT_TRUE(su::ends_with("abc  c", " c"));
    EXPECT_TRUE(su::ends_with("", ""));
    EXPECT_FALSE(su::ends_with("abc  c", " cd"));
    EXPECT_FALSE(su::ends_with("", "cd"));
}
