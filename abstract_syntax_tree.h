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

/// 语句
class StatAst {
public:
    virtual ~StatAst() = default;
    virtual llvm::Value *CodeGen() = 0;
};


/// 语句串
class StatListAst {
protected:
    std::vector<std::unique_ptr<StatAst>> stat_list_;
public:
    StatListAst(std::vector<std::unique_ptr<StatAst>> stat_list)
        : stat_list_(std::move(stat_list)) {}
    llvm::Value *CodeGen();
};

/// 语句块
class CompoundStatAst {
protected:
    std::vector<std::pair<std::string, std::unique_ptr<ExprAst>>> var_names_;
    //std::unique_ptr<ExprAst> body_;
    std::unique_ptr<StatListAst> body_;
public:
    // IntExprAst allows a list of names to be defined all at once,
    // and each name can optionally have an initializer value.
    CompoundStatAst(std::vector<std::pair<std::string, std::unique_ptr<ExprAst>>> var_names, std::unique_ptr<StatListAst> body)
        : var_names_(std::move(var_names)), body_(std::move(body)) {}

    llvm::Value *CodeGen();
};

/// 赋值语句
class AssignmentStatAst : public StatAst {
protected:
    std::string name_;
    std::unique_ptr<ExprAst> expr_;
public:
    AssignmentStatAst(std::string name, std::unique_ptr<ExprAst> expr)
        :name_(name), expr_(std::move(expr)) {}
    llvm::Value *CodeGen() override;
};

/// return 语句
class ReturnStatAst : public StatAst {
protected:
    std::unique_ptr<ExprAst> expr_;
public:
    ReturnStatAst(std::unique_ptr<ExprAst> expr)
        : expr_(std::move(expr)) {}
    llvm::Value *CodeGen() override;
};

/// IfExprAST - Expression class for if/then/else.
class IfStatAst : public StatAst {
protected:
    std::unique_ptr<ExprAst> cond_;
    std::unique_ptr<CompoundStatAst> then_;
    std::unique_ptr<CompoundStatAst> else_;
public:
    IfStatAst(std::unique_ptr<ExprAst> c, std::unique_ptr<CompoundStatAst> t,
              std::unique_ptr<CompoundStatAst> e)
        : cond_(std::move(c)), then_(std::move(t)), else_(std::move(e)) {}
    llvm::Value *CodeGen() override;
};

/// WhileExpreAst - Expression class for while
class WhileStatAst : public StatAst {
protected:
    std::unique_ptr<ExprAst> cond_;
    std::unique_ptr<CompoundStatAst> body_;
public:
    WhileStatAst(std::unique_ptr<ExprAst> cond, std::unique_ptr<CompoundStatAst> body)
        : cond_(std::move(cond)), body_(std::move(body)) {}
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
    std::unique_ptr<CompoundStatAst> body_;
public:
    FunctionAst(std::unique_ptr<PrototypeAst> proto,
                std::unique_ptr<CompoundStatAst> body)
        : proto_(std::move(proto)), body_(std::move(body)) {}
    llvm::Function *CodeGen();
};


//} // end anonymous namespace

#endif
