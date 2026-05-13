#pragma once
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <vector>
#include <memory>
#include <ranges>
#include <forward_list>

#include "parser/common/definition.hpp"

namespace Parser {

struct VariableSymbol {
    const Variable *variable;
};

struct FunctionSymbol {
    const FunctionDeclaration *declaration;
    const FunctionDefinition *definition;
};

using Symbol = std::variant<VariableSymbol, FunctionSymbol>;
using SymbolTable = std::unordered_map<std::string, Symbol>;

class Semantic {
public:
    explicit Semantic(const Program &program);

    class Issue {
    public:
        enum class Severity { Error, Warning };

        Issue(Severity severity, std::string message, Scanner::TokenInfo info);
        
        Severity severity() const;

        const std::string &message() const;
        const Scanner::TokenInfo &info() const;

    private:
        Severity severity_;
        std::string message_;
        Scanner::TokenInfo info_;
    };

    const std::vector<Issue> &issues() const;
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
        std::forward_list<Scope> children_;

        SymbolTable symbols_;
    };
    
    void analyze_variable_definition(Scope *scope, const VariableDefinition &var_def);
    void analyze_variable(Scope *scope, const Variable &var);
    void analyze_function(Scope *scope, const FunctionDefinition &func_def);
    
    void analyze_scope(Scope *scope, const Parser::Scope &ast_scope);
    void analyze_expression(Scope *scope, const Expression &expr);

    Scope global_scope_{};
    std::vector<Issue> issues_;

    std::unordered_map<const Identifier *, const Symbol *> resolved_symbols_;
};

} // namespace Parser