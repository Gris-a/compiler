#pragma once

#include "parser/definition.hpp"

namespace Parser {

class Vizitor {
public:
    virtual void vizit(const Program &program) = 0;

    void vizit(const Definition &def) {
        std::visit([this](const auto &concrete) { vizit(concrete); }, def);
    }

    void vizit(const Statement &statement) {
        std::visit([this](const auto &concrete) { vizit(concrete); }, statement);
    }

    void vizit(const Expression &expression) {
        std::visit([this](const auto &concrete) { vizit(concrete); }, expression);
    }

    void vizit(const Type &type) {
        std::visit([this](const auto &concrete) { vizit(concrete); }, type);
    }

    virtual void vizit(const FunctionDefinition &func_def) = 0;
    virtual void vizit(const VariableDefinition &variable_def) = 0;
    virtual void vizit(const Condition &cond) = 0;
    virtual void vizit(const Return &ret) = 0;
    virtual void vizit(const Scope &scope) = 0;

    virtual void vizit(const Variable &variable) = 0;

    virtual void vizit(const Identifier &ident) = 0;    
    virtual void vizit(const Integer &int_type) = 0;
    virtual void vizit(const Unsigned &uint_type) = 0;

    virtual void vizit(const VoidType &type) = 0;
    virtual void vizit(const IntegerType &type) = 0;
    virtual void vizit(const UnsignedType &type) = 0;
    virtual void vizit(const Pointer &type) = 0;

    virtual void vizit(const Comma &binop) = 0;
    virtual void vizit(const Assign &binop) = 0;
    virtual void vizit(const Addition &binop) = 0;
    virtual void vizit(const Subtraction &binop) = 0;
    virtual void vizit(const Division &binop) = 0;
    virtual void vizit(const Remainder &binop) = 0;
    virtual void vizit(const Multiplication &binop) = 0;
    virtual void vizit(const Equal &binop) = 0;
    virtual void vizit(const NotEqual &binop) = 0;
    virtual void vizit(const Less &binop) = 0;
    virtual void vizit(const LessEqual &binop) = 0;
    virtual void vizit(const Greater &binop) = 0;
    virtual void vizit(const GreaterEqual &binop) = 0;
    virtual void vizit(const OrLogical &binop) = 0;
    virtual void vizit(const AndLogical &binop) = 0;
    virtual void vizit(const Or &binop) = 0;
    virtual void vizit(const Xor &binop) = 0;
    virtual void vizit(const And &binop) = 0;
    virtual void vizit(const LeftShift &binop) = 0;
    virtual void vizit(const RightShift &binop) = 0;

    virtual void vizit(const Reference &unop) = 0;
    virtual void vizit(const Dereference &unop) = 0;
    virtual void vizit(const Not &unop) = 0;
    virtual void vizit(const NotLogical &unop) = 0;
    virtual void vizit(const Plus &unop) = 0;
    virtual void vizit(const Minus &unop) = 0;
    virtual void vizit(const Increment &unop) = 0;
    virtual void vizit(const Decrement &unop) = 0;

    virtual void vizit(const Index &index) = 0;
    virtual void vizit(const FunctionCall &call) = 0;   
};

}