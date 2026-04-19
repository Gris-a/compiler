#include "parser/parse.hpp"
#include "util/overloaded.hpp"

namespace Parser {

template<Scanner::token T>
T expect_token(TokenIterator &iter) {
    if (std::holds_alternative<T>(iter->token)) {
        return std::get<T>((iter++)->token);
    }
    throw iter->pos;
}

template<Scanner::token T>
bool peek_token(TokenIterator &iter) {
    return std::holds_alternative<T>(iter->token);
}


Expression parse_expression(TokenIterator &iter);


Type parse_type(TokenIterator &iter);

Variable parse_variable(TokenIterator &iter);


Scope parse_scope(TokenIterator &iter);

Condition parse_else(TokenIterator &iter);

Condition parse_if(TokenIterator &iter);

Return parse_return(TokenIterator &iter);

Statement parse_statement(TokenIterator &iter);


std::vector<Variable> parse_function_arguments(TokenIterator &iter);

FunctionDeclaration parse_function_declaration(TokenIterator &iter);

FunctionDefinition parse_function_definition(TokenIterator &iter);

VariableDefinition parse_variable_definition(TokenIterator &iter);

Definition parse_definition(TokenIterator &iter);


Expression parse_primary(TokenIterator &iter);

Expression parse_function_call(TokenIterator &iter);

template<typename Function, binop... Operation>
auto parse_binary_expression = [](TokenIterator &iter) -> Expression {
    Expression expression = Function()(iter);

    for (bool parse = true; parse;) {
        std::visit(
            Overloaded {
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

Expression parse_unary_expression(TokenIterator &iter) {
    return [&]<unop... Operation>(TTuple<Operation...>) -> Expression {
        return std::visit(
            Overloaded {
                [&]([[maybe_unused]] const typename Operation::Token &token) -> Expression {
                    return Operation{std::make_unique<Expression>(parse_unary_expression(++iter))};
                }...,
                [&]([[maybe_unused]] const Scanner::token auto &token) -> Expression {
                    return parse_function_call(iter);
                }
            },
            iter->token
        );
    }(UnaryOps{});
}

auto parse_unary = [](TokenIterator &iter) -> Expression {
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

Expression parse_expression(TokenIterator &iter) {
    return parse_assign(iter);
}

Expression parse_function_call(TokenIterator &iter) {
    Expression expression = parse_primary(iter);

    for (bool parse = true; parse;) {
        std::visit(
            Overloaded {
                [&]([[maybe_unused]] const Scanner::OpenBrace &token) {
                    std::vector<Expression> args;
                    if (!peek_token<Scanner::CloseBrace>(++iter)) {
                        --iter;
                        do {
                            args.emplace_back(parse_assign(++iter));
                        } while (peek_token<Scanner::Comma>(iter));
                    }
                    expression = FunctionCall {
                        std::make_unique<Expression>(std::move(expression)),
                        std::move(args)
                    };
                    expect_token<Scanner::CloseBrace>(iter);
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

Expression parse_primary(TokenIterator &iter) {
    return [&]<literal... Expr>(TTuple<Expr...>) -> Expression {
        return std::visit(
            Overloaded {
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
    }(Literals{});
}

Type parse_type(TokenIterator &iter) {
    Type type = [&]<primary_type... T>(TTuple<T...>) -> Type {
        return std::visit(
            Overloaded {
                [&]([[maybe_unused]] const typename T::Token &token) -> Type {
                    ++iter;
                    return T{};
                }...,
                [&]([[maybe_unused]] const Scanner::token auto &token) -> Type {
                    throw iter->pos;
                }
            },
            iter->token
        );
    }(Types{});

    for (bool read_modifier = true; read_modifier; ) {
        std::visit(
            Overloaded {
                [&]([[maybe_unused]] const Scanner::Star &token) -> void {
                    ++iter;
                    type = Pointer{std::make_unique<Type>(std::move(type))};
                    std::cout << "ptr\n";
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

Variable parse_variable(TokenIterator &iter) {
    Variable variable;
    
    variable.type = parse_type(iter);
    variable.identifier.value = expect_token<Scanner::Identifier>(iter).value;

    return variable;
}


Scope parse_scope(TokenIterator &iter) {
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

Condition parse_else(TokenIterator &iter) {
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

Condition parse_if(TokenIterator &iter) {
    Condition statement;
    
    expect_token<Scanner::If>(iter);
    statement.if_expression = std::make_unique<Expression>(parse_expression(iter));
    statement.statement = parse_scope(iter);
    if (peek_token<Scanner::Else>(iter)) {
        statement.else_statement = std::make_unique<Condition>(parse_else(iter));
    }

    return statement;
}

Return parse_return(TokenIterator &iter) {
    Return ret;
    
    expect_token<Scanner::Return>(iter);
    if (!peek_token<Scanner::Semicolon>(iter)) {
        ret.expression = std::make_unique<Expression>(parse_expression(iter));
    }
    expect_token<Scanner::Semicolon>(iter);

    return ret;
}

Statement parse_statement(TokenIterator &iter) {
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


VariableDefinition parse_variable_definition(TokenIterator &iter) {
    VariableDefinition definition;

    definition.variable = parse_variable(iter);
    std::visit(
        Overloaded {
            [&]([[maybe_unused]] const Scanner::Equal &token) -> void {
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

std::vector<Variable> parse_function_arguments(TokenIterator &iter) {
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

FunctionDeclaration parse_function_declaration(TokenIterator &iter) {
    FunctionDeclaration declaration;

    expect_token<Scanner::Function>(iter);
    declaration.name.value = expect_token<Scanner::Identifier>(iter).value;
    declaration.arguments = parse_function_arguments(iter);
    expect_token<Scanner::Arrow>(iter);
    declaration.return_type = parse_type(iter);

    return declaration;
}

FunctionDefinition parse_function_definition(TokenIterator &iter) {
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

Definition parse_definition(TokenIterator &iter) {
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


Program parse(const std::vector<Scanner::TokenInfo> &tokens) {
    Program definitions;

    TokenIterator iter = tokens.begin();
    while (Scanner::valid_token(iter->token)) {
        definitions.emplace_back(parse_definition(iter));
    }

    return definitions;
}

};