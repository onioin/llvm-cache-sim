
#include "llvm/Support/Program.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"

#include "LinkedList.h"
#include "CFGListParser.h"
#include "CFGParser.h"
#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

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

void inject(CCFG_t cfg, std::string injector);

cl::OptionCategory optCat{"Multiple Cache Injector Options"};

cl::opt<std::string> InPath(cl::Positional, cl::desc("<Module to instrument>"),
                            cl::value_desc("bitcode filename"),
                            cl::init(""), cl::Required,
                            cl::cat(optCat));

cl::opt<std::string> OutFile("o", cl::desc("Output filename"),
                             cl::value_desc("filename"), cl::Required,
                             cl::init(""), cl::cat(optCat));

cl::opt<std::string> InjectorPath("i", cl::desc("The directory that includes inject-cache-sim"),
                                  cl::value_desc("path"), cl::Optional,
                                  cl::cat(optCat));

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

cl::list<CCFG_t> ConfigFiles("c", cl::value_desc("cfg filename"),
                            cl::desc("A cache config file to be run"),
                            cl::cat(optCat));

cl::opt<osl::LinkedList<CCFG_t*>*> ConfigFile("C", cl::value_desc("cfg filename"),
                                             cl::desc("A single file containing a list of configs to be run"),
                                             cl::cat(optCat));


int main(int argc, char** argv){
    llvm_shutdown_obj shutdown;
    cl::HideUnrelatedOptions(optCat);
    cl::ParseCommandLineOptions(argc, argv, "Multiple Cache Simulator\n\n"
                                            "   This program allows the user to inject several different caches into "
                                            "an llvm bitcode file\n"
                                            "(equivalent to using bin/inject-cache-sim multiple times)");
    auto injector = llvm::sys::findProgramByName("");
    if(!InjectorPath.getNumOccurrences()){
        SmallString<32> invoc = llvm::StringRef(argv[0]);
        llvm::sys::path::remove_filename(invoc);
        injector = llvm::sys::findProgramByName("inject-cache-sim",
                                                {invoc.str().str()});
    }
    else{
        injector = llvm::sys::findProgramByName("inject-cache-sim",
                                                {InjectorPath.getValue()});
    }
    if(!injector){
        report_fatal_error("Could not locate injector.");
    }

    for(CCFG_t cfg : ConfigFiles) {
        //CCFGPrint(std::cout, cfg);
        inject(cfg, injector.get());
    }

    for(CCFG_t* cfg: *ConfigFile){
        inject(*cfg, injector.get());
    }

    exit(EXIT_SUCCESS);
}

void inject(CCFG_t cfg, std::string injector){
    std::stringstream oFile;
    oFile << OutFile.getValue() << "-" << cfg.getField(CCFG_t::name) << ".bin";

    std::stringstream nameOpt;
    nameOpt << "--name=" << cfg.getField(CCFG_t::name);
    std::stringstream polOpt;
    polOpt << "--policy=" << cfg.getField(CCFG_t::pol);

    std::stringstream optOpt;
    optOpt << "-O" << OptLevel;

    std::vector<std::string> args{injector, InPath.getValue(), "-o", oFile.str(), optOpt.str(),
                                  "-E", cfg.getField(CCFG_t::E), "-b", cfg.getField(CCFG_t::b),
                                  "-s", cfg.getField(CCFG_t::s), nameOpt.str(), polOpt.str()};

    for(auto & libPath : LibPaths){
        args.push_back("-L" + libPath);
    }
    for(auto & lib : LibNames){
        args.push_back("-l" + lib);
    }

    std::vector<llvm::StringRef> charArgs;
    charArgs.reserve(args.size());

    for(auto & arg : args){
        charArgs.emplace_back(arg);
    }
    for(auto & arg : args){
        outs() << arg.c_str() << " ";
    }
    outs() << "\n";

    auto res = llvm::sys::ExecuteAndWait(injector, charArgs);

    if(-1 == res){
        report_fatal_error("Injector failed");
    }
}
