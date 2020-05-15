#ifndef LOG_ERROR_H
#define LOG_ERROR_H

#include "abstract_syntax_tree.h"
#include <llvm-9/llvm/IR/Value.h>
#include <memory>

std::unique_ptr<ExprAst> LogError(const char *str);
std::unique_ptr<PrototypeAst> LogErrorP(const char *str);
llvm::Value *LogErrorV(const char *str);
void InitializeModuleAndPassManager();
llvm::Function *GetFunction(std::string name);

#endif
