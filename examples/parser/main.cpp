#include <cpparg/cpparg.h>

#include <numeric>
#include <string>

int main(int argc, const char** argv) {
    cpparg::parser parser("example");
    parser.title("cpparg-example -- example usage of cpparg.");
    std::string s;
    parser.add('q', "qwe")
        .store(s)
        .default_value("str")
        .value_type("STRING")
        .description("some string");
    parser.add('a', "add")
        .handle([](std::string_view v) { std::cout << v << std::endl; })
        .required()
        .value_type("FILE")
        .description("files to commit");
    int i;
    parser.positional("positional")
        .store(i)
        .required()
        .value_type("INTEGER")
        .description("positional interger");
    std::vector<int> v;
    parser.add('i', "int")
        .repeatable()
        .append(v)
        .value_type("INTEGER")
        .description("some integers");
    parser.add("delete").value_type("DIR").description("delete directory");
    parser.add('c').description("do something");
    parser.add_help('h', "help");
    std::vector<int> free_args;
    parser.free_arguments("files").unlimited().store(free_args);
    parser.parse(argc, argv);

    std::cout << "Integers sum = " << std::accumulate(v.begin(), v.end(), 0) << std::endl;
    std::cout << "Help:" << std::endl;

    parser.print_help();
}
