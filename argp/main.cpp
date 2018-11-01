#include "parser.h"

#include <string>

int main(int argc, const char** argv) {
    args::Parser parser;
    std::string s;
    parser.add('q', "qwe", s);
    parser.parse(argc, argv);
}
