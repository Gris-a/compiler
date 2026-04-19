#include <sstream>
#include <unordered_map>
#include <string>
#include <functional>
#include <memory>
#include <typeindex>

#include "parser/vizitor.hpp"

namespace Vizitor {

using namespace Parser;

std::string to_string(const Identifier &t) { return t.value; }
std::string to_string(const Integer &t)    { return std::to_string(t.value); }

class PrintVizitor : public Vizitor {
public:
    using Vizitor::vizit;

    PrintVizitor() {
        dot_stream_ << "digraph AST {\n";
        dot_stream_ << "  ordering=out;\n";
        dot_stream_ << "  rankdir=TB;\n";
        dot_stream_ << "  node [shape=box, fontname=\"Arial\"];\n";
        dot_stream_ << "  edge [fontname=\"Arial\"];\n";
    }

    std::string get_dot() const {
        return dot_stream_.str() + "}\n";
    }

    void vizit(const Program &program) override {
        size_t id = get_node_id(&program);
        dot_stream_ << "  " << id << " [label=\"Program\", shape=doublecircle, color=lightgreen];\n";
        for (const auto &def : program) {
            size_t child_id = get_node_id(&def);
            dot_stream_ << "  " << id << " -> " << child_id << " [label=\"definition\"];\n";
            vizit(def);
        }
    }

    void vizit(const FunctionDefinition &fd) override {
        size_t id = get_node_id(&fd);
        dot_stream_ << "  " << id << " [label=\"FunctionDefinition\", shape=doublecircle, color=lightgreen];\n";

        size_t name_id = get_node_id(&fd.declaration.name);
        dot_stream_ << "  " << id << " -> " << name_id << " [label=\"name\"];\n";
        vizit(fd.declaration.name);

        size_t ret_id = get_node_id(&fd.declaration.return_type);
        dot_stream_ << "  " << id << " -> " << ret_id << " [label=\"return_type\"];\n";
        vizit(fd.declaration.return_type);

        for (const auto &arg : fd.declaration.arguments) {
            size_t arg_id = get_node_id(&arg);
            dot_stream_ << "  " << id << " -> " << arg_id << " [label=\"argument\"];\n";
            vizit(arg);
        }

        if (fd.definition) {
            size_t body_id = get_node_id(fd.definition.get());
            dot_stream_ << "  " << id << " -> " << body_id << " [label=\"body\"];\n";
            vizit(*fd.definition);
        }
    }

    void vizit(const VariableDefinition &vd) override {
        size_t id = get_node_id(&vd);
        dot_stream_ << "  " << id << " [label=\"VariableDefinition\", shape=doublecircle, color=lightgreen];\n";

        size_t var_id = get_node_id(&vd.variable);
        dot_stream_ << "  " << id << " -> " << var_id << " [label=\"variable\"];\n";
        vizit(vd.variable);

        if (vd.initializer) {
            size_t init_id = get_node_id(vd.initializer.get());
            dot_stream_ << "  " << id << " -> " << init_id << " [label=\"initializer\"];\n";
            vizit(*vd.initializer);
        }
    }

    void vizit(const Scope &scope) override {
        size_t id = get_node_id(&scope);
        dot_stream_ << "  " << id << " [label=\"Scope\", shape=box, color=lightyellow];\n";
        for (const auto &stmt : scope.statements) {
            size_t child_id = get_node_id(&stmt);
            dot_stream_ << "  " << id << " -> " << child_id << " [label=\"statement\"];\n";
            vizit(stmt);
        }
    }

    void vizit(const Condition &cond) override {
        size_t id = get_node_id(&cond);
        dot_stream_ << "  " << id << " [label=\"Condition\", shape=box, color=lightyellow];\n";

        if (cond.if_expression) {
            size_t cond_expr_id = get_node_id(cond.if_expression.get());
            dot_stream_ << "  " << id << " -> " << cond_expr_id << " [label=\"condition\"];\n";
            vizit(*cond.if_expression);
        }

        size_t then_id = get_node_id(&cond.statement);
        dot_stream_ << "  " << id << " -> " << then_id << " [label=\"then\"];\n";
        vizit(cond.statement);

        if (cond.else_statement) {
            size_t else_id = get_node_id(cond.else_statement.get());
            dot_stream_ << "  " << id << " -> " << else_id << " [label=\"else\"];\n";
            vizit(*cond.else_statement);
        }
    }

    void vizit(const Return &ret) override {
        size_t id = get_node_id(&ret);
        dot_stream_ << "  " << id << " [label=\"Return\", shape=box, color=lightyellow];\n";
        if (ret.expression) {
            size_t expr_id = get_node_id(ret.expression.get());
            dot_stream_ << "  " << id << " -> " << expr_id << " [label=\"value\"];\n";
            vizit(*ret.expression);
        }
    }

    void vizit(const Variable &var) override {
        size_t id = get_node_id(&var);
        dot_stream_ << "  " << id << " [label=\"Variable\", shape=ellipse, color=lightblue];\n";

        size_t name_id = get_node_id(&var.identifier);
        dot_stream_ << "  " << id << " -> " << name_id << " [label=\"name\"];\n";
        vizit(var.identifier);

        size_t type_id = get_node_id(&var.type);
        dot_stream_ << "  " << id << " -> " << type_id << " [label=\"type\"];\n";
        vizit(var.type);
    }

    void vizit(const Identifier &ident) override {
        size_t id = get_node_id(&ident);
        std::string label = "Identifier: " + to_string(ident);
        dot_stream_ << "  " << id << " [label=\"" << label << "\", shape=ellipse, color=lightblue];\n";
    }

    void vizit(const Integer &integer) override {
        size_t id = get_node_id(&integer);
        std::string label = "Integer: " + to_string(integer);
        dot_stream_ << "  " << id << " [label=\"" << label << "\", shape=ellipse, color=lightblue];\n";
    }

    #define VIZIT_BINOP(name) \
    void vizit(const name &op) override { \
        size_t id = get_node_id(&op); \
        dot_stream_ << "  " << id << " [label=\"" << name::Token::key << "\", shape=box, color=lightyellow];\n"; \
        if (op.lhs) { \
            size_t lhs_id = get_node_id(op.lhs.get()); \
            dot_stream_ << "  " << id << " -> " << lhs_id << " [label=\"lhs\"];\n"; \
            vizit(*op.lhs); \
        } \
        if (op.rhs) { \
            size_t rhs_id = get_node_id(op.rhs.get()); \
            dot_stream_ << "  " << id << " -> " << rhs_id << " [label=\"rhs\"];\n"; \
            vizit(*op.rhs); \
        } \
    }

    VIZIT_BINOP(Assign)
    VIZIT_BINOP(Addition)
    VIZIT_BINOP(Subtraction)
    VIZIT_BINOP(Division)
    VIZIT_BINOP(Remainder)
    VIZIT_BINOP(Multiplication)
    VIZIT_BINOP(Equal)
    VIZIT_BINOP(NotEqual)
    VIZIT_BINOP(Less)
    VIZIT_BINOP(LessEqual)
    VIZIT_BINOP(Greater)
    VIZIT_BINOP(GreaterEqual)
    VIZIT_BINOP(OrLogical)
    VIZIT_BINOP(AndLogical)
    VIZIT_BINOP(Or)
    VIZIT_BINOP(Xor)
    VIZIT_BINOP(And)
    VIZIT_BINOP(LeftShift)
    VIZIT_BINOP(RightShift)
    #undef VIZIT_BINOP

    #define VIZIT_UNOP(name) \
    void vizit(const name &op) override { \
        size_t id = get_node_id(&op); \
        dot_stream_ << "  " << id << " [label=\"" << name::Token::key << "\", shape=box, color=lightyellow];\n"; \
        if (op.operand) { \
            size_t operand_id = get_node_id(op.operand.get()); \
            dot_stream_ << "  " << id << " -> " << operand_id << " [label=\"operand\"];\n"; \
            vizit(*op.operand); \
        } \
    }

    VIZIT_UNOP(Reference)
    VIZIT_UNOP(Dereference)
    VIZIT_UNOP(Not)
    VIZIT_UNOP(NotLogical)
    VIZIT_UNOP(Plus)
    VIZIT_UNOP(Minus)
    VIZIT_UNOP(Increment)
    VIZIT_UNOP(Decrement)
    #undef VIZIT_UNOP

    void vizit(const FunctionCall &call) override {
        size_t id = get_node_id(&call);
        dot_stream_ << "  " << id << " [label=\"Call\", shape=box, color=lightyellow];\n";
        if (call.function) {
            size_t func_id = get_node_id(call.function.get());
            dot_stream_ << "  " << id << " -> " << func_id << " [label=\"function\"];\n";
            vizit(*call.function);
        }
        for (size_t i = 0; i < call.arguments.size(); ++i) {
            size_t arg_id = get_node_id(&call.arguments[i]);
            dot_stream_ << "  " << id << " -> " << arg_id << " [label=\"arg" << i << "\"];\n";
            vizit(call.arguments[i]);
        }
    }

    void vizit(const VoidType &vt) {
        size_t id = get_node_id(&vt);
        dot_stream_ << "  " << id << " [label=\"Void\", shape=ellipse, color=lightblue];\n";
    }

    void vizit(const IntegerType &it) {
        size_t id = get_node_id(&it);
        dot_stream_ << "  " << id << " [label=\"Integer\", shape=ellipse, color=lightblue];\n";
    }

    void vizit(const Pointer &ptr) {
        size_t id = get_node_id(&ptr);
        dot_stream_ << "  " << id << " [label=\"Pointer\", shape=ellipse, color=lightblue];\n";
        if (ptr.type) {
            size_t pointee_id = get_node_id(ptr.type.get());
            dot_stream_ << "  " << id << " -> " << pointee_id << " [label=\"pointee\"];\n";
            vizit(*ptr.type);
        }
    }

private:
    std::unordered_map<const void *, size_t> node_ids_;
    size_t next_id_{};
    std::ostringstream dot_stream_;

    size_t get_node_id(const void *node) {
        auto it = node_ids_.find(node);
        if (it != node_ids_.end()) {
            return it->second;
        }

        node_ids_[node] = ++next_id_;
        return next_id_;
    }
};

} // namespace Parser