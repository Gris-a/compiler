#include "parser/definition.hpp"
#include "scanner/token.hpp"
#include "util/overloaded.hpp"

namespace Parser {

template<Scanner::token T, std::random_access_iterator Iterator>
T expect_token(Iterator& iter) {
    if (std::holds_alternative<T>(iter->token)) {
        return std::get<T>((iter++)->token);
    }
    throw std::exception();
}

template<Scanner::token T, std::random_access_iterator Iterator>
bool peek_token(Iterator& iter) {
    return std::holds_alternative<T>(iter->token);
}


template<std::random_access_iterator Iterator>
Type parse_type(Iterator &iter);

template<std::random_access_iterator Iterator>
Variable parse_variable(Iterator &iter);


template<std::random_access_iterator Iterator>
Expression parse_expression(Iterator &iter);


template<std::random_access_iterator Iterator>
Scope parse_scope(Iterator &iter);

template<std::random_access_iterator Iterator>
Condition parse_else(Iterator& iter);

template<std::random_access_iterator Iterator>
Condition parse_if(Iterator& iter);

template<std::random_access_iterator Iterator>
Return parse_return(Iterator& iter);

template<std::random_access_iterator Iterator>
Statement parse_statement(Iterator& iter);


template<std::random_access_iterator Iterator>
std::vector<Variable> parse_function_arguments(Iterator &iter);

template<std::random_access_iterator Iterator>
FunctionDeclaration parse_function_declaration(Iterator &iter);

template<std::random_access_iterator Iterator>
FunctionDefinition parse_function_definition(Iterator &iter);

template<std::random_access_iterator Iterator>
VariableDefinition parse_variable_definition(Iterator &iter);

template<std::random_access_iterator Iterator>
Definition parse_definition(Iterator& iter);


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
            [&]([[maybe_unused]] const auto &token) -> void {
                throw std::exception();
            }
        },
        iter->token
    );

    for (bool read_modifier = true; read_modifier; ) {
        std::visit(
            Overloaded {
                [&]([[maybe_unused]] const Scanner::Reference &token) -> void {
                    ++iter, type = Pointer{std::make_unique<Type>(std::move(type))};
                },
                [&]([[maybe_unused]] const auto &token) -> void {
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
    variable.identifier = expect_token<Scanner::Identifier>(iter).value;

    return variable;
}


template<typename Function, typename... TokenToBinaryOperation>
inline auto ParseBinaryExpression = []<typename Iterator>(Iterator& iter) {
  auto old_pos = getCurrentPosition(begin, end);

  auto left = Function()(begin, end, positions);

  while (begin != end) {
    auto are_operators_remain = std::visit(
        overloaded{
            ProcessBinaryOperator<TokenToBinaryOperation, Iterator, Function>{
                begin, end, positions, std::move(left), old_pos
            } ...,
            []([[maybe_unused]] const Tokenization::Token auto& token) {
              return false;
            }
        },
        begin->token
    );

    if (!are_operators_remain) {
      break;
    }
  }

  return left;
};


inline auto parseFactor =
  ParseBinaryExpression
  < decltype(parseUnary)
  , TokenToBinaryOperation<Tokenization::Star, Multiplication>
  , TokenToBinaryOperation<Tokenization::Slash, Division>
  , TokenToBinaryOperation<Tokenization::Percent, Remainder>
  >;

inline auto parseTerm =
  ParseBinaryExpression
  < decltype(parseFactor)
  , TokenToBinaryOperation<Tokenization::Plus, Addition>
  , TokenToBinaryOperation<Tokenization::Minus, Subtraction>
  >;

inline auto parseComparison =
  ParseBinaryExpression
  < decltype(parseTerm)
  , TokenToBinaryOperation<Tokenization::Greater, Greater>
  , TokenToBinaryOperation<Tokenization::GreaterEqual, GreaterEqual>
  , TokenToBinaryOperation<Tokenization::Less, Less>
  , TokenToBinaryOperation<Tokenization::LessEqual, LessEqual>
  >;

inline auto parseEquality =
  ParseBinaryExpression
  < decltype(parseComparison)
  , TokenToBinaryOperation<Tokenization::Equal, Equal>
  , TokenToBinaryOperation<Tokenization::NotEqual, NotEqual>
  >;

inline auto parseAnd =
  ParseBinaryExpression
  < decltype(parseEquality)
  , TokenToBinaryOperation<Tokenization::And, And>
  >;

inline auto parseXor =
  ParseBinaryExpression
  < decltype(parseAnd)
  , TokenToBinaryOperation<Tokenization::Xor, Xor>
  >;

inline auto parse_or =
  ParseBinaryExpression
  < decltype(parseXor)
  , TokenToBinaryOperation<Tokenization::Or, Or>
  >;

template<std::random_access_iterator Iterator>
Expression parse_expression([[maybe_unused]] Iterator &iter) {
    return Expression{}; // TODO
}


template<std::random_access_iterator Iterator>
Scope parse_scope(Iterator &iter) {
    Scope scope;
    
    expect_token<Scanner::OpenFigure>(iter);
    while (!peek_token<Scanner::CloseFigure>(iter)) {
        scope.statements.emplace_back(parse_statement(iter));
    }
    ++iter;

    return scope;
}

template<std::random_access_iterator Iterator>
Condition parse_else(Iterator& iter) {
    Condition statement;
    
    expect_token<Scanner::Else>(iter);
    std::visit(
        Overloaded {
            [&]([[maybe_unused]] const Scanner::If &token) -> void {
                statement = parse_if(iter);
            },
            [&]([[maybe_unused]] const Scanner::OpenFigure &token) -> void {
                statement.if_expression = nullptr;
                statement.statement = parse_scope(iter);
                statement.else_statement = nullptr;
            },
            [&]([[maybe_unused]] const auto &token) -> void {
                throw std::exception();
            }
        },
        iter->token
    );

    return statement;
}

template<std::random_access_iterator Iterator>
Condition parse_if(Iterator& iter) {
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
Return parse_return(Iterator& iter) {
    Return ret;
    
    expect_token<Scanner::Return>(iter);
    ret.expression = parse_expression(iter);
    expect_token<Scanner::Semicolon>(iter);

    return ret;
}

template<std::random_access_iterator Iterator>
Statement parse_statement(Iterator& iter) {
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
            [&]([[maybe_unused]] const auto &token) -> Statement {
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
                ++iter, definition.initializer = nullptr;
            },
            [&]([[maybe_unused]] const auto &token) -> void {
                throw std::exception();
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
    for (bool parse_arguments = !peek_token<Scanner::CloseBrace>(iter); parse_arguments;) {
        arguments.emplace_back(parse_variable(iter));
        std::visit(
            Overloaded {
                [&]([[maybe_unused]] const Scanner::Comma &token) -> void {
                    ++iter;
                },
                [&]([[maybe_unused]] const Scanner::CloseBrace &token) -> void {
                    parse_arguments = false;
                },
                [&]([[maybe_unused]] const auto &token) -> void {
                    throw std::exception();
                }
            },
            iter->token
        );
    }
    ++iter;

    return arguments;
}

template<std::random_access_iterator Iterator>
FunctionDeclaration parse_function_declaration(Iterator &iter) {
    FunctionDeclaration declaration;

    expect_token<Scanner::Function>(iter);
    declaration.name = expect_token<Scanner::Identifier>(iter).value;
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
                ++iter, definition.definition = nullptr;
            },
            [&]([[maybe_unused]] const Scanner::OpenFigure &token) -> void {
                definition.definition = std::make_unique<Scope>(parse_scope(iter));
            },
            [&]([[maybe_unused]] const auto &token) -> void {
                throw std::exception();
            }
        },
        iter->token
    );
    
    return definition;
}

template<std::random_access_iterator Iterator>
Definition parse_definition(Iterator& iter) {
    return std::visit(
        Overloaded {
            [&]([[maybe_unused]] const Scanner::Function &token) -> Definition {
                return parse_function_definition(iter);
            },
            [&]([[maybe_unused]] const Scanner::type auto &token) -> Definition {
                return parse_variable_definition(iter);
            },
            [&]([[maybe_unused]] const auto &token) -> Definition {
                throw std::exception();
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