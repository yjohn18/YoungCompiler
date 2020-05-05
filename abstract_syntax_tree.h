#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include <string>
#include <memory>
#include <vector>
#include <utility>


namespace {
    /// ExprAST - Base class for all expression nodes.
    class ExprAst {
    public:
        virtual ~ExprAst() = default;
    };

    /// NumberExprAST - Expression class for numeric literals like "1.0".
    class NumberExprAst : public ExprAst {
    protected:
        double val_;
    public:
        NumberExprAst(double val) : val_(val) {}
    };

    /// VariableExprAST - Expression class for referencing a variable, like "a".
    class VariableExprAst : public ExprAst {
    protected:
        std::string name_;
    public:
        VariableExprAst(const std::string &name) : name_(name) {}
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
    };
    
    /// CallExprAST - Expression class for function calls.
    class CallExprAst : public ExprAst {
    protected:
        std::string callee_;
        std::vector<std::unique_ptr<ExprAst>> args_;
    public:
        CallExprAst(const std::string &callee, std::vector<std::unique_ptr<ExprAst>> args)
            : callee_(callee), args_(std::move(args)) {}
    };

    /// PrototypeAST - This class represents the "prototype" for a function,
    /// which captures its name, and its argument names (thus implicitly the number
    /// of arguments the function takes).
    class PrototypeAst {
    protected:
        std::string name_;
        std::vector<std::string> args_;
    public:
        PrototypeAst(const std::string &name, std::vector<std::string> args)
            : name_(name), args_(std::move(args)) {}
        const std::string &name() const { return name_ };
    };

    /// FunctionAST - This class represents a function definition itself.
    class FunctionAst {
    protected:
        std::unique_ptr<PrototypeAst> proto_;
        std::unique_ptr<PrototypeAst> body_;
    public:
        FunctionAst(std::unique_ptr<PrototypeAst> proto,
                    std::unique_ptr<ExprAst> body)
            : proto_(std::move(proto)), body_(std::move(body)) {}
    };

} // end anonymous namespace

#endif
