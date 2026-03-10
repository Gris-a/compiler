#pragma once

#include "token_base.hpp"

namespace Scanner {

TOKEN(VoidType, void, "void");

TOKEN(IntegerType, void, "int");
TOKEN(UnsignedType, void, "uint");

TOKEN(If, void, "if");
TOKEN(Else, void, "else");

TOKEN(Function, void, "func");
TOKEN(Return, void, "return");

TOKEN(Comma, void, ",");
TOKEN(Semicolon, void, ";");

TOKEN(OpenBrace, void, "(");
TOKEN(CloseBrace, void, ")");

TOKEN(OpenFigure, void, "{");
TOKEN(CloseFigure, void, "}");

TOKEN(OpenSquare, void, "[");
TOKEN(CloseSquare, void, "]");

TOKEN(Plus, void, "+");
TOKEN(Minus, void, "-");

TOKEN(Divide, void, "/");
TOKEN(Remainder, void, "%");

TOKEN(Multiply, void, "*");

TOKEN(Increment, void, "++");
TOKEN(Decrement, void, "--");

TOKEN(Assign, void, "=");

TOKEN(Equal, void, "==");
TOKEN(NotEqual, void, "!=");

TOKEN(Less, void, "<");
TOKEN(LessEqual, void, "<=");

TOKEN(Greater, void, "<");
TOKEN(GreaterEqual, void, ">=");

TOKEN(NotLogical, void, "!");

TOKEN(AndLogical, void, "&&");
TOKEN(OrLogical, void, "||");

TOKEN(Not, void, "~");
TOKEN(Xor, void, "^");
TOKEN(And, void, "&");
TOKEN(Or, void, "|");

TOKEN(LeftShift, void, "<<");
TOKEN(RightShift, void, ">>");

TOKEN(Arrow, void, "->");

using Reference = And;
using Dereference = Multiply;

using Keywords = TTuple
< VoidType

, IntegerType
, UnsignedType

, If
, Else

, Function
, Return

, Comma
, Semicolon

, OpenBrace
, CloseBrace

, OpenFigure
, CloseFigure

, OpenSquare
, CloseSquare

, Plus
, Minus

, Divide
, Remainder

, Multiply

, Increment
, Decrement

, Assign

, Equal
, NotEqual
, Less
, LessEqual
, Greater
, GreaterEqual

, NotLogical
, AndLogical
, OrLogical

, Not
, And
, Or
, Xor

, LeftShift
, RightShift

, Arrow
>;

};