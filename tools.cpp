#include "tools.h"
#include <iostream>
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "KaleidoscopeJIT.h"

extern llvm::LLVMContext kTheContext;
extern llvm::IRBuilder<> kBuilder;
extern std::unique_ptr<llvm::Module> kTheModule; // = std::make_unique<llvm::Module>("my cool jit", kTheContext);
extern std::map<std::string, llvm::Value *> kNamedValues;
extern std::unique_ptr<llvm::legacy::FunctionPassManager> kTheFpm;
extern std::unique_ptr<llvm::orc::KaleidoscopeJIT> kTheJit;// = std::make_unique<llvm::orc::KaleidoscopeJIT>();
extern std::map<std::string, std::unique_ptr<PrototypeAst>> kFunctionProtos;


using std::cerr;
using std::endl;

std::unique_ptr<ExprAst> LogError(const char *str) {
    std::cerr << "Error: " << str << std::endl;
    return nullptr;
}

std::unique_ptr<PrototypeAst> LogErrorP(const char *str) {
    LogError(str);
    return nullptr;
}

llvm::Value *LogErrorV(const char *str) {
    LogError(str);
    return nullptr;
}

void InitializeModuleAndPassManager() {
    // Open a new module.
    kTheModule = std::make_unique<llvm::Module>("my niubi jit", kTheContext);
    kTheModule->setDataLayout(kTheJit->getTargetMachine().createDataLayout());

    // Create a new pass manager attached to it
    kTheFpm = std::make_unique<llvm::legacy::FunctionPassManager>(kTheModule.get());

	// Do simple "peephole" optimizations and bit-twiddling optzns.
	kTheFpm->add(llvm::createInstructionCombiningPass());
	// Reassociate expressions.
	kTheFpm->add(llvm::createReassociatePass());
	// Eliminate Common SubExpressions.
	kTheFpm->add(llvm::createGVNPass());
	// Simplify the control flow graph (deleting unreachable blocks, etc).
	kTheFpm->add(llvm::createCFGSimplificationPass());

	kTheFpm->doInitialization();
}

llvm::Function *GetFunction(std::string name) {
    if (auto *f = kTheModule->getFunction(name))
        return f;

    auto fi = kFunctionProtos.find(name);
    if (fi != kFunctionProtos.end())
        return fi->second->CodeGen();

    return nullptr;
}

