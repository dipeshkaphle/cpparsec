#include "Parser.hpp"
#include "buildExpr.hpp"
#include "buildExprClassesUtils.hpp"

#include <string>
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

std::unordered_map<std::string, int> sym_table;

std::optional<int> evaluate(const Expr<Atom> &tree) {
  return std::visit(
      [](auto &&x) -> std::optional<int> {
        using T = std::decay_t<decltype(x)>;
        // using U = typename T_of_Expr<T>::type;
        using U = Atom;
        if constexpr (std::is_same_v<T, PostfixOperation<U>>) {
          return std::nullopt;
        } else if constexpr (std::is_same_v<T, PrefixOperation<U>>) {
          const PrefixOperation<U> &op = x;
          auto res = evaluate(*(op.a));
          RETURN_NULLOPT_IF_NO_VALUE(res);
          return std::visit(
              [res](auto &&a) -> std::optional<int> {
                using V = std::remove_reference_t<decltype(a)>;
                if constexpr (std::is_same_v<V, Atom>) {
                  return std::visit(
                      [&](auto &&at) mutable -> std::optional<int> {
                        using W = std::decay_t<decltype(at)>;
                        if constexpr (std::is_same_v<W, std::string>) {
                          sym_table[at] = res.value() + 1;
                          return std::make_optional<int>(res.value() + 1);
                        } else {
                          return std::make_optional<int>(res.value() + 1);
                        }
                      },
                      a);
                };
                return std::make_optional<int>(res.value() + 1);
              },
              op.a->tree);

        } else if constexpr (std::is_same_v<T, InfixOperation<U>>) {
          const InfixOperation<U> &op = x;
          auto right = evaluate(*(op.rhs));
          RETURN_NULLOPT_IF_NO_VALUE(right);
          if (op.type == "Assign") {
            return std::visit(
                [right](auto &&y) mutable -> std::optional<int> {
                  using V = std::decay_t<decltype(y)>;
                  if constexpr (std::is_same_v<V, Atom>) {
                    return std::visit(
                        [right](auto &&z) mutable -> std::optional<int> {
                          using W = std::decay_t<decltype(z)>;
                          if constexpr (std::is_same_v<W, std::string>) {
                            sym_table[z] = right.value();
                            return std::make_optional(right.value());
                          } else {
                            return std::nullopt;
                          }
                        },
                        std::forward<decltype(y)>(y));
                  } else {
                    return std::nullopt;
                  }
                },
                op.lhs->tree);
          } else if (op.type == "Add") {
            auto left = evaluate(*(op.lhs));
            RETURN_NULLOPT_IF_NO_VALUE(left);
            return std::make_optional(left.value() + right.value());
          } else {
            auto left = evaluate(*(op.lhs));
            RETURN_NULLOPT_IF_NO_VALUE(left);
            return std::make_optional(left.value() * right.value());
          }
        } else { // means its Atom
          return std::visit(
              [](auto &&at) -> std::optional<int> {
                using V = std::decay_t<decltype(at)>;
                if constexpr (std::is_same_v<V, std::string>) {
                  if (sym_table.find(at) != sym_table.end())
                    return std::make_optional<int>(sym_table[at]);
                  return std::nullopt;
                } else {
                  return std::make_optional<int>(at);
                }
              },
              x);
        }
      },
      tree.tree);
}

void parseArithmeticExpr(const std::string &input) {
  std::vector<ExprType> table{
      INFIX("=", "Assign", Assoc::Right), INFIX("+", "Add", Assoc::Left),
      INFIX("*", "Mul", Assoc::Left), PREFIX("++", "PreIncr", Assoc::Right)};
  // std::string input("x=1+2*3+4*10");
  // std::string input("3+4");
  std::cout << "Tree for arithmetic expression parsing of " << input << "\n";
  auto expr_parser = buildExpressionParser(table, atom_parser);
  auto expr = expr_parser.parse(input);
  if (!expr.has_value() || expr.value().second != "") {
    std::cout << "Invalid parse\n";
    return;
  }
  std::cout << expr.value().first << '\n';
  std::cout << expr.value().second << '\n';
  std::optional<int> eval = evaluate(expr.value().first);
  if (eval.has_value()) {
    std::cout << eval.value() << '\n';
  } else {
    std::cout << "Invalid\n";
  }
}

int main() {
  std::string input;
  while (true) {
    std::cout << ">>>";
    std::getline(std::cin, input);
    parseArithmeticExpr(input);
  }
}
