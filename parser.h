#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <memory>
#include "abstract_syntax_tree.h"
#include "lexer.h"


class Parser {
protected:
    Lexer lexer_;
    int cur_tok_;

    // BinopPrecedence - This holds the precedence for each binary operator that
    // is defined.
    std::map<char, int> bin_op_precedence;

    int GetNextToken();

    // GetTokPrecedence - Get the precedence of the pending binary operator token.
    int GetTokPrecedence();

    // LogError* - These are little helper functions for error handling.
    std::unique_ptr<ExprAst> LogError(const char *str);
    std::unique_ptr<PrototypeAst> LogErrorP(const char *str);

    // numberexpr ::= number
    std::unique_ptr<ExprAst> ParseNumberExpr();

    // parenexpr ::= '(' expression ')'
    std::unique_ptr<ExprAst> ParseParenExpr();

    // identifierexpr
    //   ::= identifier
    //   ::= identifier '(' expression* ')'
    std::unique_ptr<ExprAst> ParseIdentifierExpr();

    // primary
    //   ::= identifierexpr
    //   ::= numberexpr
    //   ::= parenexpr
    std::unique_ptr<ExprAst> ParsePrimaryExpr();
    
    // binoprhs (binary oprator right hand side)
    //   ::= ('+' primary)*
    std::unique_ptr<ExprAst> ParseBinOpRhs();

    // expression
    //   ::= primary binoprhs
    std::unique_ptr<ExprAst> ParseExpression();

    // prototype
    // ::= id '(' id* ')'
    std::unique_ptr<PrototypeAst> ParsePrototype();

    // definition ::= 'def' prototype expression
    std::unique_ptr<FunctionAst> ParseDefinition();

    // toplevelexpr ::= expression
    std::unique_ptr<FunctionAst> ParseTopLevelExpr();

    // external ::= 'extern' prototype
    std::unique_ptr<PrototypeAst> ParseExtern();

    /* Top-Level parsing */
    void HandleDefinition();
    void HandleExtern();
    void HandleTopLevelExpression();
public:
    Parser();
    ~Parser();

    // main loop
    // top ::= definition | external | expression | ';'
    void Parse();
};



#endif
