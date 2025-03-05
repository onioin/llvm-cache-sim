
//#include "llvm/Support/Program.h"
#include "llvm/Support/CommandLine.h"

#include "CFGParser.h"
#include <stdlib.h>

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

cl::list<CFG_t> ConfigFiles("c");

cl::opt<CFG_t> ConfigFile("C")

int main(int argc, char** argv){
    cl::ParseCommandLineOptions(argc, argv);
    exit(EXIT_SUCCESS);
}
