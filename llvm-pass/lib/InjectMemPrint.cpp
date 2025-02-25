

#include "InjectMemPrint.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>




/*
 *
 * runs per module
 * inject definition for printf
 * inject const fstring definition
 * run find-mem-call for each function in module
 * for each mem-call in res, inject printf call
 *
 */

using namespace llvm;

PreservedAnalyses InjectMemPrint::run (llvm::Module &M,
                                       llvm::ModuleAnalysisManager &AM){
  bool inserted = false;

  auto &CTX = M.getContext();
  auto &FAM = AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

  FunctionCallee Printf =
      M.getOrInsertFunction("printf",
                            FunctionType::get(
                                IntegerType::getInt32Ty(CTX),
                                PointerType::getUnqual(
                                      Type::getInt8Ty(CTX)
                                      ),
                                true
                                    )
      );

  Function* PrintfF = dyn_cast<Function>(Printf.getCallee());
  PrintfF->setDoesNotThrow();
  PrintfF->addParamAttr(0, Attribute::NoCapture);
  PrintfF->addParamAttr(0, Attribute::ReadOnly);

  llvm::Constant* PrintfFormatStr = llvm::ConstantDataArray::getString(
      CTX, "(Memory Access in %s): %c %llx,%d\n");
  llvm::Constant* PrintfFormatStrVar =
      M.getOrInsertGlobal("PrintfFormatStr", PrintfFormatStr->getType());
  dyn_cast<GlobalVariable>(PrintfFormatStrVar)->setInitializer(PrintfFormatStr);

  for(auto &F : M){
    if(F.isDeclaration()){
      continue;
    }
    auto &ops = FAM.getResult<FindMemCall>(F);
    if(ops.empty()){
      continue;
    }
    IRBuilder<> Builder(&*F.getEntryBlock().getFirstInsertionPt());
    Value *FormatStrPtr =
        Builder.CreatePointerCast(PrintfFormatStrVar,
                                  PointerType::getUnqual(Type::getInt8Ty(CTX)),
                                  "formatStr");
    auto Fname = Builder.CreateGlobalStringPtr(F.getName());

    for(llvm::Instruction *Inst : ops){
      if(auto* LD = dyn_cast<LoadInst>(Inst)){
        //This is a casting nightmare +
        //uses some code marked as a potential problem
        Builder.SetInsertPoint(Inst);
        Builder.CreateCall(
            Printf,
            {FormatStrPtr, Fname,
             ConstantInt::get(
                 Type::getInt8Ty(CTX),
                 'L'),
             LD->getPointerOperand(),
             ConstantInt::get(
                 Type::getInt64Ty(CTX),
                 LD->getAlign().value())}
        );
        inserted = true;
        continue;
      }
      if(auto* ST = dyn_cast<StoreInst>(Inst)){
        Builder.SetInsertPoint(Inst);
        Builder.CreateCall(
            Printf,
            {FormatStrPtr, Fname,
             ConstantInt::get(
                 Type::getInt8Ty(CTX),
                 'S'),
             ST->getPointerOperand(),
             ConstantInt::get(
                 Type::getInt64Ty(CTX),
                 ST->getAlign().value())}
        );
        inserted = true;
        continue;
      }
    }
  }
  return (inserted ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}

llvm::PassPluginLibraryInfo getInjectMemPrintPluginInfo(){
  return {LLVM_PLUGIN_API_VERSION, "inject-mem-print", LLVM_VERSION_STRING,
          [](PassBuilder &PB){
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>){
                  if(Name == "inject-mem-print"){
                    MPM.addPass(InjectMemPrint());
                    return true;
                  }
                  return false;
              });
        }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo(){
  return getInjectMemPrintPluginInfo();
}
