#include "Parser.hpp"
#include "buildExpr.hpp"
#include "buildExprClassesUtils.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <variant>

using namespace Parsers;

using Atom = std::variant<std::string, int>;

std::ostream &operator<<(std::ostream &out, const Atom &x) {
  std::visit(
      [&](const auto &y) {
        using T = std::decay_t<decltype(y)>;
        if constexpr (std::is_same_v<T, std::string>) {
          out << y;
        } else {
          out << y;
        }
      },
      x);
  return out;
}

Parser<Atom>
    atom_parser(Fn<std::optional<pair<Atom, string_view>>(string_view)>(
        []([[maybe_unused]] string_view str)
            -> std::optional<pair<Atom, string_view>> {
          return oneOf(Alpha.oneOrMore().map<Atom>(
                           [](const std::vector<char> &vec) {
                             return std::make_optional<Atom>(
                                 Atom(std::string(vec.begin(), vec.end())));
                           }),
                       oneOf(PosNum, Parens(PosNum)).map<Atom>([](int x) {
                         return std::make_optional<Atom>(Atom(int(x)));
                       }))
              .parse(str);
        }));

int main() {
  std::vector<ExprType> table{
      INFIX("=", "Assign", Assoc::Right), INFIX("+", "Add", Assoc::Left),
      INFIX("*", "Mul", Assoc::Left), PREFIX("++", "PreIncr", Assoc::Right)};
  auto expr = buildExpr(table, atom_parser, "x=(1)+2*3+4*10");
  // auto expr = buildExpr(table, num, "2+1*6");
  // auto expr = buildExpr(table, num, "++2");
  std::cout << "OK\n";
  std::cout << expr.value() << '\n';
}
