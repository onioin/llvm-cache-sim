
#ifndef LLVM_INJECT_CACHE_SIM_H
#define LLVM_INJECT_CACHE_SIM_H

#include "FindMemCall.h"
#include "FindCallInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include <string>

using namespace llvm;

namespace cachePlugin{

    enum EvictPol {
        lfu = 0, lru = 1, fifo = 2, rnd = 3
    };

    cl::OptionCategory optionCategory{"Cache Simulator Options"};

    struct InjectCacheSim : llvm::PassInfoMixin<InjectCacheSim> {

        bool instrument(llvm::Module &M,
                        llvm::ModuleAnalysisManager &AM);

        llvm::PreservedAnalyses run(llvm::Module &M,
                                    llvm::ModuleAnalysisManager &);

        static bool isRequired() { return true; }

    };

    //wrapper for use with the old pass manager
    //the codegen pipeline requires the old pass manager still
    struct CacheSimWrapper : llvm::ModulePass {
        static char ID;

        CacheSimWrapper() : llvm::ModulePass(ID){}

        bool runOnModule(llvm::Module &M) override;
    };

}
#endif
