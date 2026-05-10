#pragma once

#include "parser/common/definition.hpp"
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <memory>
#include <optional>

namespace Parser::Semantic {

struct VariableSymbol {
    const Variable *variable;
};

struct FunctionSymbol {
    std::vector<const FunctionDefinition *> overloads;
};

using Symbol = std::variant<VariableSymbol, FunctionSymbol>;
using SymbolTable = std::unordered_map<std::string, Symbol>;

using ResolvedSymbol = std::variant<const Variable *, const FunctionDefinition *>;

class Issue {
public:
    enum class Severity { Error, Warning };

    Issue(Severity severity, std::string message);
    
    Severity severity() const;
    const std::string &message() const;

private:
    Severity severity_;
    std::string message_;
};

class Semantic {
public:
    explicit Semantic(const Program &program);

private:
    class Scope {
    public:
        explicit Scope(Scope *parent = nullptr);
        
        Scope *parent();
        
        Symbol *find_symbol(const std::string &name);
        Symbol *find_symbol_local(const std::string &name);

        bool add_symbol(const std::string &name, const Symbol &symbol);
        Scope *add_child();

    private:
        Scope *parent_;
        std::vector<Scope> children_;

        SymbolTable symbols_;
    };
    
    void analyze_function(Scope *scope, const FunctionDefinition &func_def);
    void analyze_variable(Scope *scope, const VariableDefinition &var_def);
    
    void analyze_scope(Scope *scope, const Parser::Scope &ast_scope, const Type &expected_ret);
    
    const Type *analyze_expression(Scope *scope, const Expression &expr);
    const FunctionDefinition *&resolve_overload(const FunctionSymbol &sym, const std::vector<Variable> &args);

    Scope global_scope_{};
    std::unordered_map<const Identifier *, ResolvedSymbol> resolved_symbols_;

    std::vector<Issue> issues_;
};

} // namespace Parser::Semantic