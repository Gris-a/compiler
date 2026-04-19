#pragma once

#include "parser/definition.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Parser::Semantic {

enum class ReferenceKind {
    Unknown,
    Variable,
    Function,
};

struct VariableSymbol {
    std::string name;
    std::string type;
    bool is_argument = false;
    bool is_field = false;
};

struct FunctionSymbol {
    std::string name;
    std::string return_type;
    std::vector<VariableSymbol> arguments;
};

struct SymbolTable {
    std::unordered_map<std::string, FunctionSymbol> functions;
    std::unordered_map<std::string, VariableSymbol> variables;
};

struct Resolution {
    ReferenceKind kind = ReferenceKind::Unknown;
    std::string name;
    const VariableSymbol *variable = nullptr;
    const FunctionSymbol *function = nullptr;
};

class ScopeNode {
private:
    std::string name_;
    ScopeNode *parent_ = nullptr;
    std::vector<std::unique_ptr<ScopeNode>> children_;
    std::unordered_map<std::string, VariableSymbol> variables_;
    std::shared_ptr<SymbolTable> symbol_table_;

protected:
    ScopeNode(std::string name, std::shared_ptr<SymbolTable> symbol_table);

public:
    static std::unique_ptr<ScopeNode> create_root(std::shared_ptr<SymbolTable> symbol_table);

    ScopeNode *add_child(std::string child_name);

    const std::string &get_name() const;
    ScopeNode *get_parent() const;
    const std::vector<std::unique_ptr<ScopeNode>> &get_children() const;
    const std::unordered_map<std::string, VariableSymbol> &get_variables() const;

    const VariableSymbol *find_variable(const std::string &name) const;
    const VariableSymbol *find_variable_in_parent(const std::string &name) const;
    bool has_variable(const std::string &name) const;

    VariableSymbol *add_variable(const std::string &name, VariableSymbol symbol);
};

struct AnalysisResult {
    std::unique_ptr<ScopeNode> root_scope;
    std::shared_ptr<SymbolTable> symbol_table;
    std::vector<std::string> diagnostics;
};

AnalysisResult analyze(const Program &program);

}
