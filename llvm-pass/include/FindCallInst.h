//
// Created by onioin on 13/02/25.
//

#ifndef CACHEPASS_FINDCALLINST_H
#define CACHEPASS_FINDCALLINST_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include <vector>

namespace llvm{

    class CallInst;
    class Function;
    class Module;
    class raw_ostream;

}

// ---------------------------------------

class FindCallInst : public llvm::AnalysisInfoMixin<FindCallInst> {
public:
    using Result = std::vector<llvm::CallInst *>;
    Result run(llvm::Function &Func, llvm::FunctionAnalysisManager &FAM);

private:
    friend struct llvm::AnalysisInfoMixin<FindCallInst>;
    static llvm::AnalysisKey Key;
};

// -------------------------------------

class FindCallInstPrinter : public llvm::PassInfoMixin<FindCallInstPrinter>{
public:
    explicit FindCallInstPrinter(llvm::raw_ostream &OutStream) : OS(OutStream){};

    llvm::PreservedAnalyses run(llvm::Function &Func,
                                llvm::FunctionAnalysisManager &FAM);
private:
    llvm::raw_ostream &OS;

};

#endif //CACHEPASS_FINDCALLINST_H
