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


typedef struct CFG {
    uint8_t s;
    uint8_t E;
    uint8_t b;
    uint8_t pol;
    char* name;
} CFG_t;

template <> class llvm::cl::parser<CFG_t> : public basic_parser<CFG_t>{
public:
    parser(llvm::cl::Option &O) : basic_parser(O) {}

    bool parse(llvm::cl::Option &O, llvm::StringRef ArgName,
               llvm::StringRef Arg, CFG_t &V);

    llvm::StringRef getValueName() const override {return "Cache Config";}

    void printOptionDiff(const llvm::cl::Option &O, CFG_t V,
                         llvm::cl::OptionValue<CFG_t> Default, size_t GlobalWidth) const;

    void anchor() override {};

};

bool llvm::cl::parser<CFG_t>::parse(llvm::cl::Option &O, llvm::StringRef ArgName,
                                    llvm::StringRef Arg, CFG_t &V){
    std::cout << ArgName.str() << std::endl;
    return false;
}

bool parseCFGList()

bool parseCFG()

void llvm::cl::parser<CFG_t>::printOptionDiff(const llvm::cl::Option &O, CFG_t V,
                                              llvm::cl::OptionValue<CFG_t> D,
                                              size_t GlobalWidth) const {
    printOptionName(O, GlobalWidth);
    std::string Str("hello world");
    //TODO: put something useful in Str

    outs() << "= " << Str << "\n";

}

#endif //CACHEPASS_CFGPARSER_H
