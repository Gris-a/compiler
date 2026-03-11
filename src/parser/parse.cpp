#include "parser/definition.hpp"
#include "scanner/token.hpp"
#include "util/overloaded.hpp"

namespace Parser {

template<Scanner::token T, std::random_access_iterator Iterator>
T expect_token(Iterator &iter) {
    if (std::holds_alternative<T>(iter->token)) {
        return std::get<T>((iter++)->token);
    }
    throw iter->pos;
}

template<Scanner::token T, std::random_access_iterator Iterator>
bool peek_token(Iterator &iter) {
    return std::holds_alternative<T>(iter->token);
}


template<std::random_access_iterator Iterator>
Expression parse_expression(Iterator &iter);


template<std::random_access_iterator Iterator>
Type parse_type(Iterator &iter);

template<std::random_access_iterator Iterator>
Variable parse_variable(Iterator &iter);


template<std::random_access_iterator Iterator>
Scope parse_scope(Iterator &iter);

template<std::random_access_iterator Iterator>
Condition parse_else(Iterator &iter);

template<std::random_access_iterator Iterator>
Condition parse_if(Iterator &iter);

template<std::random_access_iterator Iterator>
Return parse_return(Iterator &iter);

template<std::random_access_iterator Iterator>
Statement parse_statement(Iterator &iter);


template<std::random_access_iterator Iterator>
std::vector<Variable> parse_function_arguments(Iterator &iter);

template<std::random_access_iterator Iterator>
FunctionDeclaration parse_function_declaration(Iterator &iter);

template<std::random_access_iterator Iterator>
FunctionDefinition parse_function_definition(Iterator &iter);

template<std::random_access_iterator Iterator>
VariableDefinition parse_variable_definition(Iterator &iter);

template<std::random_access_iterator Iterator>
Definition parse_definition(Iterator &iter);


template<std::random_access_iterator Iterator>
Expression parse_primary(Iterator &iter) {
    return [&]<primary_expression... Expr>(TTuple<Expr...>) -> Expression {
        return std::visit(
            Overloaded{
                [&]([[maybe_unused]] const typename Expr::Token &token) -> Expression {
                    ++iter;
                    return Expr{token.value};
                }...,
                [&]([[maybe_unused]] const Scanner::OpenBrace &token) -> Expression {
                    auto expression = parse_expression(++iter);
                    expect_token<Scanner::CloseBrace>(iter);

                    return expression;   
                },
                [&]([[maybe_unused]] const Scanner::token auto &token) -> Expression {
                    throw iter->pos;
                }
            },
            iter->token
        );
    }(PrimaryExpressions{});
}

template<std::random_access_iterator Iterator>
Expression parse_special(Iterator &iter) {
    Expression expression = parse_primary(iter);

    for (bool parse = true; parse;) {
        std::visit(
            Overloaded{
                [&]([[maybe_unused]] const Scanner::OpenBrace &token) {
                    std::vector<Expression> args;
                    if (!peek_token<Scanner::CloseBrace>(++iter)) {
                        --iter;
                        do {
                            args.emplace_back(parse_expression(++iter));
                        } while (peek_token<Scanner::Comma>(iter));
                    }
                    expression = FunctionCall {
                        std::make_unique<Expression>(std::move(expression)),
                        std::move(args)
                    };
                    expect_token<Scanner::CloseBrace>(iter);
                },
                [&]([[maybe_unused]] const Scanner::OpenSquare &token) {
                    expression = Index {
                        std::make_unique<Expression>(std::move(expression)),
                        std::make_unique<Expression>(parse_expression(++iter))
                    };
                    expect_token<Scanner::CloseSquare>(iter);
                },
                [&]([[maybe_unused]] const Scanner::token auto &token) {
                    parse = false;
                }
            },
            iter->token
        );
    }

    return expression;
}

template<typename Function, binary_operation... Operation>
auto parse_binary_expression = []<std::random_access_iterator Iterator>(Iterator &iter) -> Expression {
    Expression expression = Function()(iter);

    for (bool parse = true; parse;) {
        std::visit(
            Overloaded{
                [&]([[maybe_unused]] const typename Operation::Token &token) -> void {
                    expression = Operation {
                        std::make_unique<Expression>(std::move(expression)),
                        std::make_unique<Expression>(Function()(++iter))
                    };
                }...,
                [&]([[maybe_unused]] const Scanner::token auto &token) -> void {
                    parse = false;
                }
            },
            iter->token
        );
    }

    return expression;
};

template<std::random_access_iterator Iterator>
Expression parse_unary_expression(Iterator &iter) {
    return [&]<unary_operation... Operation>(TTuple<Operation...>) -> Expression {
        return std::visit(
            Overloaded{
                [&]([[maybe_unused]] const typename Operation::Token &token) -> Expression {
                    return Operation{std::make_unique<Expression>(parse_unary_expression(++iter))};
                }...,
                [&]([[maybe_unused]] const Scanner::token auto &token) -> Expression {
                    return parse_special(iter);
                }
            },
            iter->token
        );
    }(UnaryOperations{});
};

auto parse_unary = []<std::random_access_iterator Iterator>(Iterator &iter) -> Expression {
    return parse_unary_expression(iter);
};

auto parse_factor = parse_binary_expression
< decltype(parse_unary)
, Multiplication
, Division
, Remainder
>;

auto parse_term = parse_binary_expression
< decltype(parse_factor)
, Addition
, Subtraction
>;

auto parse_shift = parse_binary_expression
< decltype(parse_term)
, LeftShift
, RightShift
>;

auto parse_comparison = parse_binary_expression
< decltype(parse_shift)
, Greater
, GreaterEqual
, Less
, LessEqual
>;

inline auto parse_equality = parse_binary_expression
< decltype(parse_comparison)
, Equal
, NotEqual
>;

auto parse_and = parse_binary_expression
< decltype(parse_equality)
, And
>;

auto parse_xor = parse_binary_expression
< decltype(parse_and)
, Xor
>;

auto parse_or = parse_binary_expression
< decltype(parse_xor)
, Or
>;

auto parse_logical_and = parse_binary_expression
< decltype(parse_or)
, AndLogical
>;

auto parse_logical_or = parse_binary_expression
< decltype(parse_logical_and)
, OrLogical
>;

auto parse_assign = parse_binary_expression
< decltype(parse_logical_or)
, Assign
>;

auto parse_comma = parse_binary_expression
< decltype(parse_assign)
, Comma
>;

template<std::random_access_iterator Iterator>
Expression parse_expression(Iterator &iter) {
    return parse_comma(iter);
}


template<std::random_access_iterator Iterator>
Type parse_type(Iterator &iter) {
    Type type;

    std::visit(
        Overloaded {
            [&]([[maybe_unused]] const Scanner::VoidType &token) -> void {
                ++iter, type = VoidType{};
            },
            [&]([[maybe_unused]] const Scanner::IntegerType &token) -> void {
                ++iter, type = IntegerType{};
            },
            [&]([[maybe_unused]] const Scanner::UnsignedType &token) -> void {
                ++iter, type = UnsignedType{};
            },
            [&]([[maybe_unused]] const Scanner::token auto &token) -> void {
                throw iter->pos;
            }
        },
        iter->token
    );

    for (bool read_modifier = true; read_modifier; ) {
        std::visit(
            Overloaded {
                [&]([[maybe_unused]] const Scanner::Dereference &token) -> void {
                    ++iter, type = Pointer{std::make_unique<Type>(std::move(type))};
                },
                [&]([[maybe_unused]] const Scanner::token auto &token) -> void {
                    read_modifier = false;
                }
            },
            iter->token
        );
    }

    return type;
}

template<std::random_access_iterator Iterator>
Variable parse_variable(Iterator &iter) {
    Variable variable;
    
    variable.type = parse_type(iter);
    variable.identifier.value = expect_token<Scanner::Identifier>(iter).value;

    return variable;
}


template<std::random_access_iterator Iterator>
Scope parse_scope(Iterator &iter) {
    Scope scope;
    
    expect_token<Scanner::OpenFigure>(iter);
    while (!peek_token<Scanner::CloseFigure>(iter)) {
        if (!peek_token<Scanner::Semicolon>(iter)) {
            scope.statements.emplace_back(parse_statement(iter));
        } else ++iter;
    }
    ++iter;

    return scope;
}

template<std::random_access_iterator Iterator>
Condition parse_else(Iterator &iter) {
    Condition statement;
    
    expect_token<Scanner::Else>(iter);
    std::visit(
        Overloaded {
            [&]([[maybe_unused]] const Scanner::If &token) -> void {
                statement = parse_if(iter);
            },
            [&]([[maybe_unused]] const Scanner::OpenFigure &token) -> void {
                statement.statement = parse_scope(iter);
            },
            [&]([[maybe_unused]] const Scanner::token auto &token) -> void {
                throw iter->pos;
            }
        },
        iter->token
    );

    return statement;
}

template<std::random_access_iterator Iterator>
Condition parse_if(Iterator &iter) {
    Condition statement;
    
    expect_token<Scanner::If>(iter);
    statement.if_expression = std::make_unique<Expression>(parse_expression(iter));
    statement.statement = parse_scope(iter);
    if (peek_token<Scanner::Else>(iter)) {
        statement.else_statement = std::make_unique<Condition>(parse_else(iter));
    }

    return statement;
}

template<std::random_access_iterator Iterator>
Return parse_return(Iterator &iter) {
    Return ret;
    
    expect_token<Scanner::Return>(iter);
    if (!peek_token<Scanner::Semicolon>(iter)) {
        ret.expression = std::make_unique<Expression>(parse_expression(iter));
    }
    expect_token<Scanner::Semicolon>(iter);

    return ret;
}

template<std::random_access_iterator Iterator>
Statement parse_statement(Iterator &iter) {
    return std::visit(
        Overloaded {
            [&]([[maybe_unused]] const Scanner::If &token) -> Statement {
                return parse_if(iter);
            },
            [&]([[maybe_unused]] const Scanner::Return &token) -> Statement {
                return parse_return(iter);
            },
            [&]([[maybe_unused]] const Scanner::OpenFigure &token) -> Statement {
                return parse_scope(iter);
            },
            [&]([[maybe_unused]] const Scanner::type auto &token) -> Statement {
                return parse_variable_definition(iter);
            },
            [&]([[maybe_unused]] const Scanner::token auto &token) -> Statement {
                Statement statement = parse_expression(iter);
                expect_token<Scanner::Semicolon>(iter);
                
                return statement;
            }
        },
        iter->token
    );    
}


template<std::random_access_iterator Iterator>
VariableDefinition parse_variable_definition(Iterator &iter) {
    VariableDefinition definition;

    definition.variable = parse_variable(iter);
    std::visit(
        Overloaded {
            [&]([[maybe_unused]] const Scanner::Assign &token) -> void {
                definition.initializer = std::make_unique<Expression>(parse_expression(++iter));
                expect_token<Scanner::Semicolon>(iter);
            },
            [&]([[maybe_unused]] const Scanner::Semicolon &token) -> void {
                ++iter;
            },
            [&]([[maybe_unused]] const Scanner::token auto &token) -> void {
                throw iter->pos;
            }
        },
        iter->token
    );

    return definition;
}

template<std::random_access_iterator Iterator>
std::vector<Variable> parse_function_arguments(Iterator &iter) {
    std::vector<Variable> arguments;

    expect_token<Scanner::OpenBrace>(iter);
    if (!peek_token<Scanner::CloseBrace>(iter)) {
        --iter;
        do {
            arguments.emplace_back(parse_variable(++iter));
        } while (peek_token<Scanner::Comma>(iter));
    }
    expect_token<Scanner::CloseBrace>(iter);

    return arguments;
}

template<std::random_access_iterator Iterator>
FunctionDeclaration parse_function_declaration(Iterator &iter) {
    FunctionDeclaration declaration;

    expect_token<Scanner::Function>(iter);
    declaration.name.value = expect_token<Scanner::Identifier>(iter).value;
    declaration.arguments = parse_function_arguments(iter);
    expect_token<Scanner::Arrow>(iter);
    declaration.return_type = parse_type(iter);

    return declaration;
}

template<std::random_access_iterator Iterator>
FunctionDefinition parse_function_definition(Iterator &iter) {
    FunctionDefinition definition;
    
    definition.declaration = parse_function_declaration(iter);
    std::visit(
        Overloaded {
            [&]([[maybe_unused]] const Scanner::Semicolon &token) -> void {
                ++iter;
            },
            [&]([[maybe_unused]] const Scanner::OpenFigure &token) -> void {
                definition.definition = std::make_unique<Scope>(parse_scope(iter));
            },
            [&]([[maybe_unused]] const Scanner::token auto &token) -> void {
                throw iter->pos;
            }
        },
        iter->token
    );
    
    return definition;
}

template<std::random_access_iterator Iterator>
Definition parse_definition(Iterator &iter) {
    return std::visit(
        Overloaded {
            [&]([[maybe_unused]] const Scanner::Function &token) -> Definition {
                return parse_function_definition(iter);
            },
            [&]([[maybe_unused]] const Scanner::type auto &token) -> Definition {
                return parse_variable_definition(iter);
            },
            [&]([[maybe_unused]] const Scanner::token auto &token) -> Definition {
                throw iter->pos;
            }
        },
        iter->token
    );
}


std::vector<Definition> parse(const std::vector<Scanner::TokenInfo> &tokens) {
    std::vector<Definition> definitions;

    auto iter = tokens.begin();
    while (Scanner::valid_token(iter->token)) {
        definitions.emplace_back(parse_definition(iter));
    }

    return definitions;
}

};