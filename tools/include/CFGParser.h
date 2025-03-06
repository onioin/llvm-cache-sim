//
// Created by onioin on 05/03/25.
//

#ifndef CACHEPASS_CFGPARSER_H
#define CACHEPASS_CFGPARSER_H

#include "llvm/Support/CommandLine.h"

#include "json.h"
#include "fileio.h"

#include <string>
#include <iostream>


typedef struct CCFG {
    uint8_t s;
    uint8_t E;
    uint8_t b;
    uint8_t pol;
    char* name;
} CCFG_t;

template <> class llvm::cl::parser<CCFG_t> : public basic_parser<CCFG_t>{
public:
    parser(llvm::cl::Option &O) : basic_parser(O) {}

    bool parse(llvm::cl::Option &O, llvm::StringRef ArgName,
               llvm::StringRef Arg, CCFG_t &V);

    llvm::StringRef getValueName() const override {return "Cache Config";}

    void printOptionDiff(const llvm::cl::Option &O, CCFG_t V,
                         llvm::cl::OptionValue<CCFG_t> Default, size_t GlobalWidth) const;

    void anchor() override {};

};

bool llvm::cl::parser<CCFG_t>::parse(llvm::cl::Option &O, llvm::StringRef ArgName,
                                    llvm::StringRef Arg, CCFG_t &V){
    std::cout << ArgName.str() << ": " << Arg.str() << std::endl;
    return false;
}

bool parseCCFGList(){
    return false;
}

bool parseCCFG(){
    return false;
}

void llvm::cl::parser<CCFG_t>::printOptionDiff(const llvm::cl::Option &O, CCFG_t V,
                                              llvm::cl::OptionValue<CCFG_t> D,
                                              size_t GlobalWidth) const {
    printOptionName(O, GlobalWidth);
    std::string Str("hello world");
    //TODO: put something useful in Str

    outs() << "= " << Str << "\n";

}

#endif //CACHEPASS_CFGPARSER_H
