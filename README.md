# cpparg

[![Build Status](https://travis-ci.com/BigRedEye/cpparg.svg?token=HabA2F1p73cnpyrz3Jdj&branch=master)](https://travis-ci.com/BigRedEye/cpparg)
[![GitHub tag](https://img.shields.io/github/tag/BigRedEye/cpparg.svg)](https://semver.org)
[![license](https://img.shields.io/github/license/BigRedEye/cpparg.svg)](https://github.com/BigRedEye/cpparg/master/LICENSE)

Yet another single header command line arguments parser.

## Example

```cpp
#include <cpparg/cpparg.h>
#include <filesystem>

int main(int argc, const char** argv) {
    cpparg::parser parser("Example parser");
    parser.title("Some title");
    
    std::string name;
    parser
        .add('n', "name")
        .optional()
        .value_type("STRING")
        .default_value("Bob")
        .description("Bob's name")
        .store(name);
        
    int i;
    parser
        .positional("i")
        .required()
        .description("Positional interger")
        .value_type("INTEGER")
        .store(i);
        
    parser
        .add("delete")
        .optional()
        .value_type("DIR")
        .description("Directory to delete")
        .default_value("/")
        .handle([](std::string_view dir) {
            std::filesystem::remove(dir);
        });
        
    std::vector<int> free_args;
    parser
        .free_arguments("ints")
        .unlimited()
        .store(free_args);
        
    parser.add_help('h', "help");
    parser.parse(argc, argv);
}
```

## Requirements

+ Compiler with C++17 support
+ CMake (optional)

## Installation


##### Manual

Just put [cpparg.h](include/cpparg/cpparg.h) somewhere inside your build tree and include it.

##### CMake

+ Clone this repository
```sh
git clone https://github.com/BigRedEye/cpparg.git
```

+ Add the following in your CMakeLists.txt:
```
add_subdirectory(cpparg)

target_link_libraries(YOUR_TARGET PUBLIC cpparg)
```

## Usage

TODO
