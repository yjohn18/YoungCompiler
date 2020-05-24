#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include <string>
#include <memory>
#include <vector>
#include <utility>
#include <llvm-9/llvm/ADT/APFloat.h>
#include <llvm-9/llvm/ADT/STLExtras.h>
#include <llvm-9/llvm/IR/BasicBlock.h>
#include <llvm-9/llvm/IR/Constants.h>
#include <llvm-9/llvm/IR/DerivedTypes.h>
#include <llvm-9/llvm/IR/Function.h>
#include <llvm-9/llvm/IR/Value.h>
#include <llvm-9/llvm/IR/IRBuilder.h>
#include <llvm-9/llvm/IR/LLVMContext.h>
#include <llvm-9/llvm/IR/Module.h>
#include <llvm-9/llvm/IR/Type.h>
#include <llvm-9/llvm/IR/Verifier.h>
#include <llvm-9/llvm/IR/LegacyPassManager.h>
#include <llvm-9/llvm/IR/Instructions.h>
#include "KaleidoscopeJIT.h"

//#ifdef _WIN32
//#define DLLEXPORT __declspec(dllexport)
//#else
//#define DLLEXPORT
//#endif

//extern "C" DLLEXPORT int printi(int n) {
    //fprintf(stderr, "%d\n", n);
    //return 0;
//}


class PrototypeAst;

//namespace {

/// Ast - Base class for all AST nodes.
class Ast {
protected:
    /* LLVM objects */
//        static llvm::LLVMContext the_context_;
//        static llvm::IRBuilder<> builder_(llvm::LLVMContext(the_context_));
//        static std::unique_ptr<llvm::Module> the_module_;
//        static std::map<std::string, llvm::Value *> named_values_;
public:
    virtual ~Ast() = default;
};
// initialization
//    llvm::IRBuilder<> Ast::builder_ = llvm::IRBuilder<>(Ast::the_context_);

/// ExprAST - Base class for all expression nodes.
class ExprAst : public Ast{
public:
    virtual ~ExprAst() = default;
    virtual llvm::Value *CodeGen() = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAst : public ExprAst {
protected:
    double val_;
public:
    NumberExprAst(double val) : val_(val) {}
    llvm::Value *CodeGen() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAst : public ExprAst {
protected:
    std::string name_;
public:
    VariableExprAst(const std::string &name) : name_(name) {}
    const std::string &name() const { return name_; };
    llvm::Value *CodeGen() override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAst : public ExprAst {
protected:
    char op_;
    std::unique_ptr<ExprAst> lhs_, rhs_;
public:
    BinaryExprAst(char op, std::unique_ptr<ExprAst> lhs, 
                    std::unique_ptr<ExprAst> rhs)
        : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}
    llvm::Value *CodeGen() override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAst : public ExprAst {
protected:
    std::string callee_;
    std::vector<std::unique_ptr<ExprAst>> args_;
public:
    CallExprAst(const std::string &callee, std::vector<std::unique_ptr<ExprAst>> args)
        : callee_(callee), args_(std::move(args)) {}
    llvm::Value *CodeGen() override;
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAst : public ExprAst {
protected:
    std::unique_ptr<ExprAst> cond_, then_, else_;
public:
    IfExprAst(std::unique_ptr<ExprAst> c, std::unique_ptr<ExprAst> t,
              std::unique_ptr<ExprAst> e)
        : cond_(std::move(c)), then_(std::move(t)), else_(std::move(e)) {}
    llvm::Value *CodeGen() override;
};

/// WhileExpreAst - Expression class for while
class WhileExprAst : public ExprAst {
protected:
    std::unique_ptr<ExprAst> cond_, body_;
public:
    WhileExprAst(std::unique_ptr<ExprAst> cond, std::unique_ptr<ExprAst> body)
        : cond_(std::move(cond)), body_(std::move(body)) {}
    llvm::Value *CodeGen() override;
};

class IntExprAst : public ExprAst {
protected:
    std::vector<std::pair<std::string, std::unique_ptr<ExprAst>>> var_names_;
    std::unique_ptr<ExprAst> body_;
public:
    // IntExprAst allows a list of names to be defined all at once,
    // and each name can optionally have an initializer value.
    IntExprAst(std::vector<std::pair<std::string, std::unique_ptr<ExprAst>>> var_names, std::unique_ptr<ExprAst> body)
        : var_names_(std::move(var_names)), body_(std::move(body)) {}

    llvm::Value *CodeGen() override;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAst : public Ast{
protected:
    std::string name_;
    std::vector<std::string> args_;
public:
    PrototypeAst(const std::string &name, std::vector<std::string> args)
        : name_(name), args_(std::move(args)) {}
    const std::string &name() const { return name_; };
    llvm::Function *CodeGen();
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAst : public Ast{
protected:
    std::unique_ptr<PrototypeAst> proto_;
    std::unique_ptr<ExprAst> body_;
public:
    FunctionAst(std::unique_ptr<PrototypeAst> proto,
                std::unique_ptr<ExprAst> body)
        : proto_(std::move(proto)), body_(std::move(body)) {}
    llvm::Function *CodeGen();
};

//} // end anonymous namespace

#endif
