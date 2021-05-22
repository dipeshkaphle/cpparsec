#include "Parser.hpp"
#include <cassert>
#include <iostream>
#include <memory>

using namespace Parsers;

enum class Assoc { Left, Right };

enum class ExprType { PREFIX, INFIX };

enum Op { MUL, ADD, ASSIGN, NONE };

struct Operation {
  Op op;
  int num;
  std::shared_ptr<Operation> left;
  std::shared_ptr<Operation> right;
  Operation(int n) : op(Op::NONE), num(n) {}
  Operation(Op opr, int num, Operation left_op, Operation right_op)
      : op(opr), num(num), left(std::make_shared<Operation>(left_op)),
        right(std::make_shared<Operation>(right_op)) {}
  Operation(const Operation &other) = default;
  Operation &operator=(const Operation &other) {
    op = other.op;
    num = other.num;
    left = other.left;
    right = other.right;
    return *this;
  }
  void print_pre(Operation *root) {
    if (root == nullptr) {
      return;
    }
    switch (root->op) {
    case Op::ADD /* constant-expression */:
      std::cout << "( ADD ";
      break;
    case Op::MUL /* constant-expression */:
      std::cout << "( MUL ";
      break;
    case Op::ASSIGN /* constant-expression */:
      std::cout << "( ASSIGN ";
      break;

    default:
      std::cout << " " << root->num << ' ';
      return;
    }
    print_pre(root->left.get());
    print_pre(root->right.get());
    std::cout << " )";
  }
};

Parser<Operation> num([](string_view str) {
  return oneOf(Parens(PosNum), PosNum).parse(str);
});

// prototype of what could be done
// I hope to make this possible just on the basis of the table provided
template <typename T>
std::optional<Operation>
parseExpr(string_view str,
          std::span<std::tuple<ExprType, std::string, Op, Assoc>> table,
          const Parser<T> &base_parser) {
  unsigned i = 1;
  for (auto &x : table) {
    string_view new_str = str;
    auto string_parser = String(std::get<1>(x));

    while (true) {
      auto it = new_str.find(std::get<1>(x));
      if (it == std::string::npos)
        break;
      // remove all the elements before the matched part
      new_str.remove_prefix(it);
      auto res = string_parser.parse(new_str);
      auto left_sub_str = str.substr(0, str.size() - new_str.size());
      std::optional<Operation> left_op =
          parseExpr(left_sub_str, table.last(table.size() - 1), base_parser);
      /// if no valid thing in left parse, ignore and move on
      if (!left_op.has_value()) {
        new_str.remove_prefix(1);
        continue;
      }

      std::optional<Operation> right_op =
          parseExpr(res.value().second, table, base_parser);

      // ignore the last parse and move on
      if (!right_op.has_value()) {
        new_str.remove_prefix(1);
        continue;
      }

      // if say we have + currently and the right operand is also a tree with
      // + at the top ,then we bring associativity into play
      if ((std::get<2>(x)) == right_op.value().op) {
        if (std::get<3>(x) == Assoc::Left) {
          auto tmp_left = right_op.value().left;
          right_op.value().left = std::make_shared<Operation>(
              Operation(std::get<2>(x), 0, left_op.value(), *tmp_left));
          return right_op;
        }
      }
      return std::make_optional(
          Operation(std::get<2>(x), 0, left_op.value(), right_op.value()));
    }
    i++;
  }
  return Operation(base_parser.parse(str).value().first);
}

int main() {
  // only for infix expressions for now

  std::vector<std::tuple<ExprType, std::string, Op, Assoc>> table;
  table.push_back(
      std::make_tuple(ExprType::INFIX, "=", Op::ASSIGN, Assoc::Right));
  table.push_back(std::make_tuple(ExprType::INFIX, "+", Op::ADD, Assoc::Left));
  table.push_back(std::make_tuple(ExprType::INFIX, "*", Op::MUL, Assoc::Left));
  std::optional<Operation> operation = parseExpr("2=1+2*3+4*10", table, num);
  operation.value().print_pre(&(operation.value()));
  std::cout << "\n";
  return 0;
}
