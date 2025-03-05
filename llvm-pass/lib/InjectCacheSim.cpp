
#include "InjectCacheSim.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include <string>


using namespace llvm;

namespace cachePlugin{

    char CacheSimWrapper::ID = 0;

    //when using opt, load the pass with -load and --load-pass-plugin
    //-load allows the pass to access the command line options

    cl::opt<EvictPol> EvictPolicy("policy", cl::desc("Set the cache replacement policy:"),
                                  cl::values(
                                          clEnumVal(lfu, "Least Frequently Used"),
                                          clEnumVal(lru, "Least Recently Used"),
                                          clEnumVal(fifo, "First In First Out"),
                                          clEnumValN(rnd, "rand", "Random")),
                                  cl::Required,
                                  cl::cat(cachePlugin::optionCategory));
    cl::opt<int> SetBits("s", cl::desc("Number of bits used for set index (number of sets is 2^s)"),
                         cl::value_desc("num"), cl::Required, cl::cat(cachePlugin::optionCategory),
                         cl::Prefix);
    cl::opt<int> LinesPer("E", cl::desc("Number of lines per set (associativity/number of ways)"),
                          cl::value_desc("num"), cl::Required, cl::cat(cachePlugin::optionCategory),
                          cl::Prefix);
    cl::opt<int> BlockBits("b", cl::desc("Number of bits used for byte offset (block size is 2^b bytes)"),
                           cl::value_desc("num"), cl::Required, cl::cat(cachePlugin::optionCategory),
                           cl::Prefix);
    cl::opt<std::string> CacheName("name", cl::desc("Name of the cache (optional)"), cl::init("L1"),
                                   cl::cat(cachePlugin::optionCategory));
    cl::alias CacheNameA("n", cl::desc("Alias for -name"), cl::aliasopt(CacheName),
                         cl::cat(cachePlugin::optionCategory));

bool InjectCacheSim::instrument(llvm::Module &M,
                                llvm::ModuleAnalysisManager &AM) {
    bool injected = false;

    auto &CTX = M.getContext();
    //auto &FAM = AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
    auto FAM = AnalysisManager<llvm::Function>();

    //declaration of the cache struct
    llvm::StructType *CacheTy =
            StructType::get(CTX,
                            {
                                IntegerType::getInt32Ty(CTX),
                                IntegerType::getInt32Ty(CTX),
                                IntegerType::getInt32Ty(CTX),
                                IntegerType::getInt32Ty(CTX),
                                IntegerType::getInt32Ty(CTX),
                                IntegerType::getInt32Ty(CTX),
                                IntegerType::getInt32Ty(CTX),
                                PointerType::getUnqual(CTX),
                                PointerType::getUnqual(CTX)
                            });

    //declaration of the operateCache function
    FunctionCallee CacheOperate =
            M.getOrInsertFunction("operateCache",
                                  FunctionType::get(
                                          Type::getVoidTy(CTX),
                                          {IntegerType::getInt64Ty(CTX),
                                           PointerType::getUnqual(CacheTy)},
                                           false));

    //declaration of the cacheSetup function
    FunctionCallee CacheCreate =
            M.getOrInsertFunction("cacheSetup",
                                  FunctionType::get(
                                          Type::getVoidTy(CTX),
                                          {
                                              PointerType::getUnqual(CacheTy),
                                              IntegerType::getInt32Ty(CTX),
                                              IntegerType::getInt32Ty(CTX),
                                              IntegerType::getInt32Ty(CTX),
                                              IntegerType::getInt32Ty(CTX),
                                              PointerType::getUnqual(
                                                      IntegerType::getInt8Ty(CTX)
                                              )},
                                              false));
    //declaration of the cacheDeallocate function
    FunctionCallee CacheDelete =
            M.getOrInsertFunction("cacheDeallocate",
                                  FunctionType::get(
                                  Type::getVoidTy(CTX),
                                  {
                                      PointerType::getUnqual(CacheTy)
                                  },
                                  false));

    //declaration of the safe exit function, deallocates the cache and prints the result
    FunctionCallee SE =
            M.getOrInsertFunction("cacheExit",
                                  FunctionType::get(
                                          Type::getVoidTy(CTX),
                                          false));
    //stdlib function atexit()
    FunctionCallee AtExit =
            M.getOrInsertFunction("atexit",
                                  FunctionType::get(
                                          IntegerType::getInt32Ty(CTX),
                                          {PointerType::getUnqual(CTX)},
                                          false));
    Function* AE = dyn_cast<Function>(AtExit.getCallee());
    AE->addFnAttr(Attribute::NoUnwind);
    AE->addParamAttr(0, Attribute::NoUndef);

    //inject global variables for cache and cache name
    llvm::Constant *CachePtr = M.getOrInsertGlobal("cache_", PointerType::getUnqual(CacheTy));
    dyn_cast<GlobalVariable>(CachePtr)->setInitializer(ConstantPointerNull::get(
            PointerType::getUnqual(CacheTy)));
    llvm::Constant *CacheNameStr = llvm::ConstantDataArray::getString(CTX, CacheName);
    llvm::Constant *CacheNameVar = M.getOrInsertGlobal("CacheName_", CacheNameStr->getType());
    dyn_cast<GlobalVariable>(CacheNameVar)->setInitializer(CacheNameStr);

    //inject a definition for a cleanup function to run at exit
    if(Function *SafeExit = M.getFunction("cacheExit")){
        SafeExit->addFnAttr(Attribute::NoInline);
        SafeExit->addFnAttr(Attribute::NoUnwind);
        SafeExit->setDSOLocal(true);
        BasicBlock *BB = BasicBlock::Create(CTX, "", SafeExit);
        IRBuilder<> Builder(BB);
        Builder.CreateCall(CacheDelete, {CachePtr});
        Builder.CreateRetVoid();
    }

    //inject cache setup and cleanup
    if (Function *mainFunc = M.getFunction("main")) {
        IRBuilder<> Builder(&*mainFunc->getEntryBlock().getFirstInsertionPt());
        Value *CacheNamePtr = Builder.CreatePointerCast(CacheNameVar,
                                                        PointerType::getUnqual(
                                                                Type::getInt8Ty(CTX)));
        Builder.CreateCall(CacheCreate,
                   {CachePtr,
                    ConstantInt::get(
                            Type::getInt32Ty(CTX),
                            SetBits),
                    ConstantInt::get(
                            Type::getInt32Ty(CTX),
                            LinesPer),
                    ConstantInt::get(
                            Type::getInt32Ty(CTX),
                            BlockBits),
                    ConstantInt::get(
                            Type::getInt32Ty(CTX),
                            EvictPolicy),
                    CacheNamePtr});
        Builder.CreateCall(AtExit, {dyn_cast<Function>(SE.getCallee())});
    }

    for (auto &F: M) {
        if (F.isDeclaration()) {
            //declarations have no instructions
            continue;
        }

        //get all memory related instructions (loads and stores)
        //auto &ops = FAM.getResult<FindMemCall>(F);
        auto ops = (new FindMemCall())->run(F, FAM);
        IRBuilder<> Builder(&*F.getEntryBlock().getFirstInsertionPt());

        if (ops.empty()) {
            continue;
        }

        for (llvm::Instruction *Inst: ops) {
            //per instruction, cast the pointer to i64
            //inject cache operation
            if (auto *LD = dyn_cast<LoadInst>(Inst)) {
                Builder.SetInsertPoint(Inst);
                auto* ptrCast = Builder.CreatePtrToInt(LD->getPointerOperand(),
                                                       IntegerType::getInt64Ty(CTX));
                Builder.CreateCall(CacheOperate,
                                   {
                                           ptrCast,
                                           CachePtr
                                   });
                injected = true;
                continue;
            }
            if (auto *ST = dyn_cast<StoreInst>(Inst)) {
                Builder.SetInsertPoint(Inst);
                auto* ptrCast = Builder.CreatePtrToInt(ST->getPointerOperand(),
                                                       IntegerType::getInt64Ty(CTX));
                Builder.CreateCall(CacheOperate,
                                   {
                                           ptrCast,
                                           CachePtr
                                   });
                injected = true;
            }
        }
    }

    return injected;
}

    llvm::PreservedAnalyses InjectCacheSim::run(llvm::Module &M,
                                                llvm::ModuleAnalysisManager &AM){
    bool changed = instrument(M, AM);
    return (changed ? llvm::PreservedAnalyses::none()
                    : llvm::PreservedAnalyses::all());
}

    bool CacheSimWrapper::runOnModule(llvm::Module &M){
        auto AM = llvm::AnalysisManager<Module>();
        return (new InjectCacheSim())->instrument(M, AM);
    }

}

//register with the new pass manager
llvm::PassPluginLibraryInfo getInjectCacheSimPluginInfo(){
    return {LLVM_PLUGIN_API_VERSION, "inject-cache-sim", LLVM_VERSION_STRING,
            [](PassBuilder &PB){
                PB.registerPipelineParsingCallback(
                        [](StringRef Name, ModulePassManager &MPM,
                           ArrayRef<PassBuilder::PipelineElement>){
                            if(Name == "inject-cache-sim"){
                                MPM.addPass(cachePlugin::InjectCacheSim());
                                return true;
                            }
                            return false;
                        });
            }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo(){
    return getInjectCacheSimPluginInfo();
}
