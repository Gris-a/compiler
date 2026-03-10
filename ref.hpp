#pragma once

#include "ASTNodes.hpp"

#include <Util/Overloaded.hpp>
#include <Util/TypeToString.hpp>
#include <Util/VariantCast.hpp>

template<Tokenization::Operator Token, BinaryOperation Operation>
struct TokenToBinaryOperation {
};

template<typename TokenToBinaryOperation, typename Iterator, typename Function>
struct ProcessBinaryOperator;

template
< Tokenization::Operator Token
, BinaryOperation Operation
, typename Iterator
, typename Function
>
struct ProcessBinaryOperator<TokenToBinaryOperation<Token, Operation>, Iterator, Function> {
  Iterator& begin;
  Iterator end;
  Positions& positions;
  ExpressionVariant&& left;
  const Position& old_pos;

  bool operator()([[maybe_unused]] const Token& token) {
    ++begin;

    auto right_pos = getCurrentPosition(begin, end);

    auto right = Function()(begin, end, positions);

    auto new_left = Operation{
        std::make_unique<ExpressionVariant>(std::move(left)),
        std::make_unique<ExpressionVariant>(std::move(right))
    };

    positions[new_left.left_operand.get()] = old_pos;
    positions[new_left.right_operand.get()] = right_pos;

    left = std::move(new_left);

    return true;
  }
};

template<typename Function, typename... TokenToBinaryOperation>
inline auto ParseBinaryExpression =
    []<typename Iterator>(Iterator& begin, Iterator end, Positions& positions){
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

template<typename Iterator>
ExpressionVariant parseExpression(
    Iterator& begin,
    Iterator end,
    Positions& positions
);


template<typename Iterator>
ExpressionVariant parsePrimary(
    Iterator& begin,
    Iterator end,
    Positions& positions
)
{
  throwIfNoMoreTokens(begin, end);

  return std::visit(
      overloaded {
          [&](const Tokenization::IntLiteral& token) -> ExpressionVariant {
            ++begin;

            return IntLiteral(token.value);
          },
          [&](const Tokenization::FltLiteral& token) -> ExpressionVariant {
            ++begin;

            return FltLiteral(token.value);
          },
          [&](const Tokenization::StrLiteral& token) -> ExpressionVariant {
            ++begin;

            return StrLiteral(token.value);
          },
          [&](const Tokenization::Identifier& token) -> ExpressionVariant {
            ++begin;

            return Variable(token.name);
          },
          [&]([[maybe_unused]] const Tokenization::LeftParenthesis& token) -> ExpressionVariant {
            ++begin;

            auto expression = parseExpression(begin, end, positions);

            expectToken<Tokenization::RightParenthesis>(begin, end);

            return expression;
          },
          [&]([[maybe_unused]] const Tokenization::If& token) -> ExpressionVariant {
            ++begin;

            auto if_pos = getCurrentPosition(begin, end);
            auto if_expression = parseExpression(begin, end, positions);

            expectToken<Tokenization::Then>(begin, end);

            auto then_pos = getCurrentPosition(begin, end);
            auto then_expression = parseExpression(begin, end, positions);

            expectToken<Tokenization::Else>(begin, end);

            auto else_pos = getCurrentPosition(begin, end);
            auto else_expression = parseExpression(begin, end, positions);

            auto result = ConditionalExpression {
                std::make_unique<ExpressionVariant>(std::move(if_expression)),
                std::make_unique<ExpressionVariant>(std::move(then_expression)),
                std::make_unique<ExpressionVariant>(std::move(else_expression))
            };

            positions[result.condition.get()] = if_pos;
            positions[result.true_expression.get()] = then_pos;
            positions[result.false_expression.get()] = else_pos;

            return result;
          },
          [&]([[maybe_unused]] const Tokenization::Match& token) -> ExpressionVariant {
            ++begin;

            auto match_expression_pos = getCurrentPosition(begin, end);
            auto match_expression = parseExpression(begin, end, positions);

            auto cases = parseCases(begin, end, positions);

            if (cases.size() < 2) {
              throw std::runtime_error(
                  std::format(
                      "Two or more cases expected on position {}.\n",
                      match_expression_pos.toString()
                  )
              );
            }

            auto result = MatchExpression {
                std::make_unique<ExpressionVariant>(std::move(match_expression)),
                std::move(cases)
            };

            positions[result.expression_to_match.get()] = match_expression_pos;

            return result;
          },
          [&]([[maybe_unused]] const Tokenization::Token auto& token) -> ExpressionVariant {
            throwSyntaxError(begin->position, "Expression expected");
          }
      },
      begin->token
  );
}

template<typename Iterator>
ExpressionVariant parsePostfix(
    Iterator& begin,
    Iterator end,
    Positions& positions
)
{
  auto old_pos = getCurrentPosition(begin, end);

  auto current = parsePrimary(begin, end, positions);

  while (begin != end) {
    bool are_postfix_expressions_remain = std::visit(
        overloaded {
            [&]([[maybe_unused]] const Tokenization::Dot& token) {
              ++begin;

              auto name = expectToken<Tokenization::Identifier>(begin, end).name;

              auto sub_expression = std::make_unique<ExpressionVariant>(std::move(current));
              positions[sub_expression.get()] = old_pos;

              current = FieldRetrieving(
                  std::move(sub_expression),
                  name
              );

              return true;
            },
            [&]([[maybe_unused]] const Tokenization::LeftParenthesis& token) {
              ++begin;

              auto current_pos = getCurrentPosition(begin, end);

              auto arguments = std::deque<ExpressionVariant>{};

              if (!std::holds_alternative<Tokenization::RightParenthesis>(begin->token)) {
                while(true) {
                  arguments.push_back(parseExpression(begin, end, positions));

                  positions[&arguments.back()] = current_pos;

                  throwIfNoMoreTokens(begin, end);

                  bool are_arguments_remain = std::visit(
                      overloaded {
                        []([[maybe_unused]] const Tokenization::Comma& token) -> bool {
                          return true;
                        },
                        []([[maybe_unused]] const Tokenization::RightParenthesis& token) -> bool {
                          return false;
                        },
                        [&]([[maybe_unused]] const Tokenization::Token auto& token) -> bool {
                          throwSyntaxError(
                              begin->position,
                              std::format(
                                  "{} or {} token expected",
                                  toStringUnqualified<Tokenization::Comma>(),
                                  toStringUnqualified<Tokenization::RightParenthesis>()
                              )
                          );
                        },
                      },
                      begin->token
                  );

                  if (are_arguments_remain == true) {
                    ++begin;
                    current_pos = getCurrentPosition(begin, end);
                  } else {
                    break;
                  }
                }
              }

              assert(std::holds_alternative<Tokenization::RightParenthesis>(begin->token));
              ++begin;

              auto sub_expression = std::make_unique<ExpressionVariant>(std::move(current));
              positions[sub_expression.get()] = old_pos;

              if (arguments.empty()) {
                current = Call(
                    std::move(sub_expression)
                );
              } else {
                auto currying = std::make_unique<ExpressionVariant>(
                    Currying(
                        std::move(sub_expression),
                        std::move(arguments)
                    )
                );

                positions[currying.get()] = old_pos;

                current = Call(
                    std::move(currying)
                );
              }

              return true;
            },
            [&]([[maybe_unused]] const Tokenization::LeftBrace& token) {
              auto start_pos = begin->position;

              ++begin;

              auto current_pos = getCurrentPosition(begin, end);

              auto arguments = std::deque<ExpressionVariant>{};

              if (!std::holds_alternative<Tokenization::RightBrace>(begin->token)) {
                while(true) {
                  arguments.push_back(parseExpression(begin, end, positions));

                  positions[&arguments.back()] = current_pos;

                  throwIfNoMoreTokens(begin, end);

                  bool are_arguments_remain = std::visit(
                      overloaded {
                        []([[maybe_unused]] const Tokenization::Comma& token) -> bool {
                          return true;
                        },
                        []([[maybe_unused]] const Tokenization::RightBrace& token) -> bool {
                          return false;
                        },
                        [&]([[maybe_unused]] const Tokenization::Token auto& token) -> bool {
                          throwSyntaxError(
                              begin->position,
                              std::format(
                                  "{} or {} token expected",
                                  toStringUnqualified<Tokenization::Comma>(),
                                  toStringUnqualified<Tokenization::RightBrace>()
                              )
                          );
                        },
                      },
                      begin->token
                  );

                  if (are_arguments_remain == true) {
                    ++begin;
                    current_pos = getCurrentPosition(begin, end);
                  } else {
                    break;
                  }
                }
              }

              assert(std::holds_alternative<Tokenization::RightBrace>(begin->token));
              ++begin;

              auto sub_expression = std::make_unique<ExpressionVariant>(std::move(current));
              positions[sub_expression.get()] = old_pos;

              if (arguments.empty()) {
                throw std::runtime_error(
                    std::format("Empty curreing on position {}\n", start_pos.toString())
                );
              } else {
                current = Currying(
                    std::move(sub_expression),
                    std::move(arguments)
                );
              }

              return true;
            },
            []([[maybe_unused]] const Tokenization::Token auto& token) {
              return false;
            }
        },
        begin->token
    );

    if (!are_postfix_expressions_remain) {
      break;
    }
  }

  return current;
}

#define PROCESS_UNARY_OPERATOR(operator, expression) \
[&]([[maybe_unused]] const operator& token) { \
  ++begin; \
  auto pos = getCurrentPosition(begin, end); \
  auto result = expression{ \
      std::make_unique<ExpressionVariant>(parseUnary(begin, end, positions)) \
  }; \
  positions[result.operand.get()] = pos; \
  return ExpressionVariant{std::move(result)}; \
},

/*

parseUnary is a lambda because I need to know it's type below without substituing Iterator template
parametr

*/

inline auto parseUnary = []<typename Iterator>(
    this const auto& parseUnary,
    Iterator& begin,
    Iterator end,
    Positions& positions
) -> ExpressionVariant
{
  throwIfNoMoreTokens(begin, end);

  return std::visit(
      overloaded {
          PROCESS_UNARY_OPERATOR(Tokenization::Not, Not)
          PROCESS_UNARY_OPERATOR(Tokenization::Plus, UnaryPlus)
          PROCESS_UNARY_OPERATOR(Tokenization::Minus, UnaryMinus)
          [&]([[maybe_unused]] const Tokenization::Token auto& token) {
            return parsePostfix(begin, end, positions);
          }
      },
      begin->token
  );
};

#undef PROCESS_UNARY_OPERATOR

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

inline auto parseOr =
  ParseBinaryExpression
  < decltype(parseXor)
  , TokenToBinaryOperation<Tokenization::Or, Or>
  >;

template<typename Iterator>
ExpressionVariant parseLetExpression(
    Iterator& begin,
    Iterator end,
    Positions& positions
)
{
  throwIfNoMoreTokens(begin, end);

  if (std::holds_alternative<Tokenization::Lt>(begin->token)) {
    ++begin;

    auto variable_name = expectToken<Tokenization::Identifier>(begin, end).name;

    throwIfNoMoreTokens(begin, end);

    auto type = [&]() -> std::optional<TypeVariant> {
      if (std::holds_alternative<Tokenization::Colon>(begin->token)) {
        ++begin;

        return parseType(begin, end);
      } else {
        return std::nullopt;
      }
    }();

    expectToken<Tokenization::Assign>(begin, end);

    auto old_pos = getCurrentPosition(begin, end);

    auto value = std::make_unique<ExpressionVariant>(parseExpression(begin, end, positions));

    positions[value.get()] = old_pos;

    expectToken<Tokenization::In>(begin, end);

    old_pos = getCurrentPosition(begin, end);

    auto sub_expression =
        std::make_unique<ExpressionVariant>(parseExpression(begin, end, positions));

    positions[sub_expression.get()] = old_pos;

    return
        LetExpression{variable_name, std::move(type), std::move(value), std::move(sub_expression)};
  }

  return parseOr(begin, end, positions);
}

template<typename Iterator>
ExpressionVariant parseExpression(
    Iterator& begin,
    Iterator end,
    Positions& positions
)
{
  return parseLetExpression(begin, end, positions);
}


} // namespace detail


} // namespace Parsing
