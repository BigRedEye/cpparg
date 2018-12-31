# cpparg

[![Build Status](https://travis-ci.com/BigRedEye/cpparg.svg?token=HabA2F1p73cnpyrz3Jdj&branch=master)](https://travis-ci.com/BigRedEye/cpparg)
[![GitHub tag](https://img.shields.io/github/tag/BigRedEye/cpparg.svg)](https://semver.org)
[![license](https://img.shields.io/github/license/BigRedEye/cpparg.svg)](https://github.com/BigRedEye/cpparg/master/LICENSE)

Yet another single header command line arguments parser.

## Example

```cpp
#include <cpparg/cpparg.h>

int main(int argc, const char** argv) {
    cpparg::parser parser("Example");
    parser.title("Some title");
    std::string name;
    parser.add('n', "name")
        .store(name)
        .default_value("Bob")
        .value_type("STRING")
        .description("Bob's name");
    parser.add('a', "add")
        .handle([](std::string_view v) { std::cout << v << std::endl; })
        .required()
        .value_type("FILE")
        .description("Add files to commit");
    int i;
    parser.positional("i")
        .store(i)
        .required()
        .value_type("INTEGER")
        .description("Positional interger");
    parser.add("delete").value_type("DIR").description("Directory to directory");
    std::vector<int> free_args;
    parser.free_arguments("files").unlimited().store(free_args);
    parser.add_help('h', "help");
    parser.parse(argc, argv);
}
```

## Installation

##### CMake

+ Clone this repository
```sh
git clone https://github.com/BigRedEye/cpparg.git
```

+ Add the following in your CMakeLists.txt:
```
add_subdirectory(cpparg)

target_link_libraries(YOUR_TARGET cpparg)
```

##### Manual

Just put [cpparg.h](include/cpparg/cpparg.h) somewhere inside your build tree and include it.

## Usage

TODO
