#include "ExprClasses.hpp"
#include "Parser.hpp"
#include "buildExpr.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <variant>

using namespace Parsers;

Parser<int> num(Fn<std::optional<pair<int, string_view>>(string_view)>(
    [](string_view str) -> std::optional<pair<int, string_view>> {
      auto res = PosNum.parse(str);
      return res;
    }));

int main() {
  std::vector<ExprType> table{
      INFIX("=", "Assign", Assoc::Right), INFIX("+", "Add", Assoc::Left),
      INFIX("*", "Mul", Assoc::Left), PREFIX("++", "PreIncr", Assoc::Right)};
  auto expr = buildExpr(table, num, "2=1+2*3+4*10");
  // auto expr = buildExpr(table, num, "2+1*6");
  // auto expr = buildExpr(table, num, "++2");
  std::cout << "OK\n";
}
