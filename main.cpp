#include "Parser.hpp"
#include "buildExpr.hpp"
#include "buildExprClassesUtils.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <variant>

using namespace cpparsec;
using namespace cpparsec::Parsers;

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

void parseArithmeticExpr() {
  std::vector<ExprType> table{
      INFIX("=", "Assign", Assoc::Right), INFIX("+", "Add", Assoc::Left),
      INFIX("*", "Mul", Assoc::Left), PREFIX("++", "PreIncr", Assoc::Right)};
  std::string input("x=(1)+2*3+4*10");
  std::cout << "Tree for arithmetic expression parsing of " << input << "\n";
  auto expr = buildExpr(table, atom_parser, input);
  std::cout << expr.value().first << '\n';
}

void parseRegexExpr() {
  std::vector<ExprType> table{INFIX("|", "Alternate", Assoc::Left),
                              INFIX(".", "Concat", Assoc::Left),
                              POSTFIX("*", "Kleene", Assoc::Right)};
  std::string input("a*.b*|c*|d*");
  std::cout << "Tree for regex parsing of " << input << "\n";
  auto expr_parser = buildExpressionParser(table, Alpha);
  auto expr = expr_parser.parse(input);
  if (expr.has_value())
    std::cout << expr.value().first << '\n';
}

int main() {
  parseArithmeticExpr();
  std::cout << '\n';
  parseRegexExpr();
}
