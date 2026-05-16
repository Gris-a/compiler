#include "parser/semantics.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>

#include "util/overloaded.hpp"

namespace Parser {

Semantic::Issue::Issue(Severity severity, std::string message, Scanner::TokenInfo info)
    : severity_(severity), message_(std::move(message)), info_(std::move(info)) {}

Semantic::Issue::Severity Semantic::Issue::severity() const { return severity_; }

const std::string &Semantic::Issue::message() const { return message_; }

const Scanner::TokenInfo &Semantic::Issue::info() const { return info_; }


Semantic::Scope::Scope(Scope *parent) : parent_(parent) {}

Semantic::Scope *Semantic::Scope::parent() { return parent_; }

Symbol *Semantic::Scope::find_symbol(const std::string &name) {
    Symbol *sym = find_symbol_local(name);
    if (sym) return sym;
    return parent_ ? parent_->find_symbol(name) : nullptr;
}

Symbol *Semantic::Scope::find_symbol_local(const std::string &name) {
    auto it = symbols_.find(name);
    return (it != symbols_.end()) ? &it->second : nullptr;
}

bool Semantic::Scope::add_symbol(const std::string &name, const Symbol &symbol) { 
    return symbols_.emplace(name, symbol).second; 
}

Semantic::Scope *Semantic::Scope::add_child() { 
    return &children_.emplace_front(this); 
}


Semantic::Semantic(const Program &program) {
    for (const auto &definition : program) {
        std::visit(Overloaded {
            [&](const FunctionDefinition &func_def) { analyze_function(&global_scope_, func_def); },
            [&](const VariableDefinition &var_def) { analyze_variable_definition(&global_scope_, var_def); }
        }, definition);
    }
}

const std::vector<Semantic::Issue> &Semantic::issues() const {
    return issues_;
}

const Symbol *Semantic::lookup_symbol(const Identifier &id) const {
    auto it = resolved_symbols_.find(&id);
    if (it != resolved_symbols_.end())
        return it->second;
    return nullptr;
}

void Semantic::analyze_variable_definition(Scope *scope, const VariableDefinition &var_def) {
    if (var_def.initializer)
        analyze_expression(scope, *var_def.initializer);
    analyze_variable(scope, var_def.variable);
}

void Semantic::analyze_variable(Scope *scope, const Variable &var) {
    const std::string &name = var.identifier.value;

    if (!scope->find_symbol_local(name)) {
        if (scope->find_symbol(name))
            issues_.emplace_back(Issue::Severity::Warning, "Shadow declaration.", var.identifier.token_info);
        scope->add_symbol(name, VariableSymbol{&var});
    } else issues_.emplace_back(Issue::Severity::Error, "Multiple declarations in same scope.", var.identifier.token_info);
}

void Semantic::analyze_function(Scope *scope, const FunctionDefinition &func_def) {
    const std::string &name = func_def.declaration.name.value;

    if (Symbol *sym = scope->find_symbol_local(name)) {
        if (auto *func = std::get_if<FunctionSymbol>(sym); func && (func->function->declaration == func_def.declaration)) {
            if (func_def.definition) {
                if (func->function->definition)
                    issues_.emplace_back(Issue::Severity::Error, "Multiple function definitions in same scope.", func_def.declaration.token_info);
                else func->function = &func_def;
            }
        } else issues_.emplace_back(Issue::Severity::Error, "Multiple declarations in same scope.", func_def.declaration.token_info);
    } else {
        if (scope->find_symbol(name))
            issues_.emplace_back(Issue::Severity::Warning, "Shadow declaration.", func_def.declaration.token_info);
        scope->add_symbol(name, FunctionSymbol{&func_def});
    }

    if (func_def.definition) {
        Scope *func_scope = scope->add_child();
        for (const auto &arg : func_def.declaration.arguments)
            analyze_variable(func_scope, arg);
        analyze_scope(func_scope, *func_def.definition);
    }
}

void Semantic::analyze_scope(Scope *scope, const Parser::Scope &ast) {
    for (const auto &statement : ast.statements) {
        std::visit(Overloaded {
            [&](const Parser::Scope &inner) { analyze_scope(scope->add_child(), inner); },
            [&](const Expression &expr) { analyze_expression(scope, expr); },
            [&](const VariableDefinition &var_def) { analyze_variable_definition(scope, var_def); },
            [&](const FunctionDefinition &func_def) { analyze_function(scope, func_def); },
            [&](const Return &ret) { if (ret.expression) analyze_expression(scope, *ret.expression); },
            [&](const Condition &condition) {
                for (const auto *cond = &condition; cond; cond = cond->else_statement.get()) {
                    if (cond->if_expression)
                        analyze_expression(scope, *cond->if_expression);
                    analyze_scope(scope->add_child(), cond->statement);
                }
            }
        }, statement);
    }
}

void Semantic::analyze_expression(Scope *scope, const Expression &expr) {
    return std::visit(Overloaded {
        [&](const Identifier &ident) {
            if (const auto *sym = scope->find_symbol(ident.value)) {
                if (std::holds_alternative<VariableSymbol>(*sym)) {
                    resolved_symbols_[&ident] = sym;
                } else issues_.emplace_back(Issue::Severity::Error, "Not a variable.", ident.token_info);
            } else issues_.emplace_back(Issue::Severity::Error, "Undefined identifier.", ident.token_info);
        },
        [&](const FunctionCall &call) {
            if (const auto *sym = scope->find_symbol(call.function.value)) {
                if (std::holds_alternative<FunctionSymbol>(*sym)) {
                        resolved_symbols_[&call.function] = sym;
                } else issues_.emplace_back(Issue::Severity::Error, "Not a function.", call.function.token_info);
            } else issues_.emplace_back(Issue::Severity::Error, "Undefined identifier.", call.function.token_info);
        },
        [&](const binop auto &op) {
            analyze_expression(scope, *op.lhs);
            analyze_expression(scope, *op.rhs);
        },
        [&](const unop auto &op) {
            analyze_expression(scope, *op.operand);
        },
        [&]([[maybe_unused]] const auto &) { return; }
    }, expr);
}

} // namespace Parser