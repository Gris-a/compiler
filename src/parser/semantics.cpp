#include "parser/semantics.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>

#include "util/overloaded.hpp"

namespace Parser::Semantic {

static const Type int_type{IntegerType{}};
static const Type void_type{VoidType{}};

static bool operator==(const Type &lhs, const Type &rhs) {
    return std::visit(Overloaded {
        [&](const Pointer &l, const Pointer &r) {
            return *l.type == *r.type;
        },
        [&]([[maybe_unused]] const auto &l, [[maybe_unused]] const auto &r) {
            return (lhs.index() == rhs.index());
        }
    }, lhs, rhs);
}


Issue::Issue(Severity severity, std::string message)
    : severity_(severity), message_(std::move(message)) {}

Issue::Severity Issue::severity() const { return severity_; }

const std::string &Issue::message() const { return message_; }


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
    return &children_.emplace_back(this); 
}


Semantic::Semantic(const Program &program) {
    for (const auto &definition : program) {
        std::visit(Overloaded {
            [&](const FunctionDefinition &func_def) { analyze_function(&global_scope_, func_def); },
            [&](const VariableDefinition &var_def) { analyze_variable(&global_scope_, var_def); }
        }, definition);
    }
}

void Semantic::analyze_variable(Scope *scope, const VariableDefinition &var_def) {
    const Type &type = var_def.variable.type;
    const std::string &name = var_def.variable.identifier.value;

    if (std::holds_alternative<VoidType>(type)) {
        issues_.emplace_back(Issue::Severity::Error, "Incomplete type.");
    }

    if (var_def.initializer) {
        const Type *init_type = analyze_expression(scope, *var_def.initializer);
        if (init_type && (*init_type != type)) {
            issues_.emplace_back(Issue::Severity::Error, "Unknown conversion.");
        }
    }

    if (scope->find_symbol_local(name)) {
        issues_.emplace_back(Issue::Severity::Error, "Multiple declarations.");
    } else {
        if (scope->find_symbol(name)) {
            issues_.emplace_back(Issue::Severity::Warning, "Shadow declaration.");
        }
        scope->add_symbol(name, VariableSymbol{&var_def.variable});
    }
}

void Semantic::analyze_function(Scope *scope, const FunctionDefinition &func_def) {
    const Type &return_type = func_def.declaration.return_type;
    const std::string &name = func_def.declaration.name.value;
    const auto &args = func_def.declaration.arguments;

    if (Symbol *symbol = scope->find_symbol_local(name)) {
        FunctionSymbol *function = std::get_if<FunctionSymbol>(symbol);
        if (!function) {
            issues_.emplace_back(Issue::Severity::Error, "Multiple declarations in the same scope.");
        } else if (const auto *&match = resolve_overload(*function, args)) {
            if (match->declaration.return_type != return_type) {
                issues_.emplace_back(Issue::Severity::Error, "Multiple declarations return types conflict.");
            }
            if (match->definition && func_def.definition) {
                issues_.emplace_back(Issue::Severity::Error, "Multiple definitions in same scope.");
            }
            match = &func_def;
        }
    } else {
        if (scope->find_symbol(name)) {
            issues_.emplace_back(Issue::Severity::Warning, "Shadow declaration.");
        }

        scope->add_symbol(name, FunctionSymbol{});
        FunctionSymbol *function = std::get_if<FunctionSymbol>(scope->find_symbol_local(name));
        function->overloads.push_back(&func_def);
    }

    if (func_def.definition) {
        Scope *f_scope = scope->add_child();
        for (const auto &arg : func_def.declaration.arguments) {
            // Проверка на дубликаты имен аргументов
            if (f_scope->find_symbol_local(arg.identifier.value)) {
                issues_.emplace_back(Issue::Severity::Error, "Duplicate parameter name '" + arg.identifier.value + "' in function '" + name + "'.");
            }
            f_scope->add_symbol(arg.identifier.value, VariableSymbol{&arg});
        }
        
        // Рекурсивно анализируем блок инструкций внутри функции
        analyze_scope(f_scope, *func_def.definition, func_def.declaration.return_type);
    }
}

void Semantic::analyze_scope(Scope *scope, const Parser::Scope &ast_scope, const Type &expected_ret) {
    for (const auto &stmt : ast_scope.statements) {
        std::visit(Overloaded {
            [&](const Parser::Scope &nested) { analyze_scope(scope->add_child(), nested, expected_ret); },
            [&](const Condition &cond) {
                for (const auto *c = &cond; c; c = c->else_statement.get()) {
                    if (c->if_expression) {
                        const Type *t = analyze_expression(scope, *c->if_expression);
                        if (t && std::holds_alternative<VoidType>(*t))
                            issues_.emplace_back(Issue::Severity::Error, "Condition cannot be void.");
                    }
                    analyze_scope(scope->add_child(), c->statement, expected_ret);
                }
            },
            [&](const Return &ret) {
                if (ret.expression) {
                    const Type *t = analyze_expression(scope, *ret.expression);
                    if (t && (*t != expected_ret)) {
                        issues_.emplace_back(Issue::Severity::Error, "Return type mismatch.");
                    }
                } else if (!std::holds_alternative<VoidType>(expected_ret)) {
                    issues_.emplace_back(Issue::Severity::Error, "Return with no value in non-void function.");
                }
            },
            [&](const Expression &expr) { analyze_expression(scope, expr); },
            [&](const VariableDefinition &var_def) { analyze_variable(scope, var_def); }
        }, stmt);
    }
}

const Type *Semantic::analyze_expression(Scope *scope, const Expression &expr) {
    return std::visit(Overloaded {
        [&]([[maybe_unused]] const Integer &integer) -> const Type * { 
            return &int_type;
        },
        [&](const Identifier &ident) -> const Type * {
            if (auto *sym = scope->find_symbol(ident.value)) {
                if (auto *v = std::get_if<VariableSymbol>(sym)) {
                    resolved_symbols_[&ident] = v->variable;
                    return &v->variable->type;
                }
                issues_.emplace_back(Issue::Severity::Error, "'" + ident.value + "' is not a variable.");
                return nullptr;
            }
            issues_.emplace_back(Issue::Severity::Error, "Use of undeclared identifier '" + ident.value + "'.");
            return nullptr;
        },
        [&](const FunctionCall &call) -> const Type* {
            std::vector<Variable> dummy_args;
            for (const auto &arg_expr : call.arguments) {
                const Type *t = analyze_expression(scope, arg_expr);
                // Создаем фиктивную переменную для сопоставления типов в resolve_overload
                dummy_args.push_back(Variable{ t ? *t : int_type, Identifier{""} });
            }

            if (auto *id = std::get_if<Identifier>(call.function.get())) {
                if (auto *sym = scope->find_symbol(id->value)) {
                    if (auto *f_sym = std::get_if<FunctionSymbol>(sym)) {
                        if (auto *match = resolve_overload(*f_sym, dummy_args)) {
                            resolved_symbols_[id] = match;
                            return &match->declaration.return_type;
                        }
                    }
                }
                issues_.emplace_back(Issue::Severity::Error, "No matching function for call to '" + id->value + "'.");
            }
            return nullptr;
        },
        [&](const binop auto &node) -> const Type * {
            const Type *l = analyze_expression(scope, *node.lhs);
            const Type *r = analyze_expression(scope, *node.rhs);
            return nullptr;
        },
        [&](const unop auto &node) -> const Type * { 
            const Type *t = analyze_expression(scope, *node.operand); 
            return nullptr;
        },
        [&]([[maybe_unused]] const auto &) -> const Type * { 
            return nullptr; 
        }
    }, expr);
}

const FunctionDefinition *&Semantic::resolve_overload(const FunctionSymbol &sym, const std::vector<Variable> &args) {
    for (const auto *def : sym.overloads) {
        const auto &params = def->declaration.arguments;
        if (params.size() != args.size()) continue;

        bool match = std::ranges::equal(params, args,
            [](const auto &param, const auto &arg) {
                return param.type == arg.type;
            }
        );
        if (match) return def;
    }
    return nullptr;
}

} // namespace Parser::Semantic