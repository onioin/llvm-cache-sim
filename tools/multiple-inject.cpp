
//#include "llvm/Support/Program.h"
#include "llvm/Support/CommandLine.h"

#include "CFGParser.h"
#include <stdlib.h>
#include <string>

/*
 * get config file off command line
 *      config file is list of json objects
 *          bitcode file
 *          cache options
 *          output file and linking info
 * SANITIZE
 * for each config in config file
 *      invoke inject-cache-sim with sanitized options
 *      
 */

//TODO: use -c for individual configs, -C for config including multiple configs

using namespace llvm;

cl::OptionCategory optCat{"Multiple Cache Injector Options"};

cl::opt<std::string> InPath(cl::Positional, cl::desc("<Module to instrument>"),
                            cl::value_desc("bitcode filename"),
                            cl::init(""), cl::Required,
                            cl::cat(optCat));

cl::opt<std::string> OutFile("o", cl::desc("Output filename"),
                             cl::value_desc("filename"), cl::Required,
                             cl::init(""), cl::cat(optCat));

cl::opt<char> OptLevel("O", cl::desc("Clang optimization level [-O0, -O1, -O2(default), -O3]"),
                       cl::init('2'), cl::Prefix,
                       cl::value_desc("level"),
                       cl::cat(optCat));

cl::list<std::string> LibPaths("L", cl::Prefix,
                               cl::desc("Specify library search paths"),
                               cl::value_desc("directory"), cl::cat(optCat));

cl::list<std::string> LibNames("l", cl::Prefix,
                               cl::desc("Specify libraries to link against"),
                               cl::value_desc("library"), cl::cat(optCat));

cl::list<CFG_t> ConfigFiles("c",
                            cl::cat(optCat));

cl::opt<CFG_t> ConfigFile("C",
                          cl::cat(optCat));

int main(int argc, char** argv){
    cl::HideUnrelatedOptions(optCat);
    cl::ParseCommandLineOptions(argc, argv, "Multiple Cache Simulator\n\n"
                                            "   This program allows the user to inject several different caches into "
                                            "an llvm bitcode file\n"
                                            "(equivalent to using bin/inject-cache-sim multiple times)");
    exit(EXIT_SUCCESS);
}
