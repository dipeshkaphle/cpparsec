#ifndef BUILDEXPR
#define BUILDEXPR

#include "Parser.hpp"
#include "buildExprClassesUtils.hpp"

#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <variant>

using namespace std::placeholders;

using namespace cpparsec;
using namespace cpparsec::Parsers;

template <typename T>
std::optional<std::pair<Expr<T>, string_view>>
buildExpr(std::span<ExprType> table, const Parser<T> &base_parser,
          string_view str) {

  unsigned iter_count = 1;
  for (auto &x : table) {
    TypeDescription descr = std::visit(visit_for_ExprTypeDescription, x);
    auto result(std::visit(
        [&](const auto &type) -> std::optional<pair<Expr<T>, string_view>> {
          using V = std::decay_t<decltype(type)>;
          // poor man's pattern matching
          if constexpr (std::is_same_v<V, PREFIX>) { // IF THE TYPE IS PREFIX
            string_view op = descr.op;
            auto has_operator = str.starts_with(op);
            if (!has_operator)
              return std::nullopt;
            auto res = buildExpr(table, base_parser, str.substr(op.size()));

            RETURN_NULLOPT_IF_NO_VALUE(res);

            return std::make_optional<std::pair<Expr<T>, string_view>>(
                {Expr<T>(PrefixOperation<T>(descr.op_name,
                                            std::move(res.value().first))),
                 res.value().second});

          } else if constexpr (std::is_same_v<V, POSTFIX>) { // IF THE TYPE IS
                                                             // POSTFIX
            // must be of x++ form where operator is ++
            // must end with the operator that is
            auto has_operator = str.ends_with(descr.op);
            if (!has_operator)
              return std::nullopt;
            auto res =
                buildExpr(table.last(table.size() - iter_count), base_parser,
                          str.substr(0, str.size() - descr.op.size()));

            RETURN_NULLOPT_IF_NO_VALUE(res);

            return std::make_optional<std::pair<Expr<T>, string_view>>(
                {PostfixOperation<T>(descr.op_name,
                                     std::move(res.value().first)),
                 res.value().second});

          } else { // FOR INFIX TYPES
            string_view new_str = str;
            while (true) {
              auto it = new_str.find(descr.op);
              auto string_parser = String(descr.op);
              if (it == std::string::npos)
                break;
              // remove all the elements before the matched part
              new_str.remove_prefix(it);
              auto res = string_parser.parse(new_str);
              auto left_sub_str = str.substr(0, str.size() - new_str.size());
              std::optional<pair<Expr<T>, string_view>> left_op(
                  buildExpr(table.last(table.size() - iter_count), base_parser,
                            left_sub_str));
              /// if no valid thing in left parse, ignore and move on
              if (!left_op.has_value() || left_op.value().second != "") {
                new_str.remove_prefix(1);
                continue;
              }

              std::optional<pair<Expr<T>, string_view>> right_op(
                  buildExpr(table, base_parser, res.value().second));

              // ignore the last parse and move on
              if (!right_op.has_value()) {
                new_str.remove_prefix(1);
                continue;
              }

              // if say we have + currently and the right operand is also a tree
              // with
              // + at the top ,then we bring associativity into play
              if (descr.op_name == std::visit(get_op_name_of_Expr,
                                              right_op.value().first.tree)) {
                if (descr.associativity == Assoc::Left) {
                  InfixOperation<T> right_infix_op = std::visit(
                      get_lhs_rhs_out_of_Infix, right_op.value().first.tree);
                  InfixOperation<T> rights_left;
                  rights_left.type = descr.op_name;
                  rights_left.lhs = std::make_unique<Expr<T>>(
                      std::move(left_op.value().first));
                  rights_left.rhs = std::move(right_infix_op.lhs);
                  right_infix_op.lhs = std::make_unique<Expr<T>>(
                      Expr<T>(std::move(rights_left)));
                  return std::make_optional<std::pair<Expr<T>, string_view>>(
                      {Expr<T>(std::move(right_infix_op)),
                       right_op.value().second});
                }
              }
              return std::make_optional<std::pair<Expr<T>, string_view>>(
                  {Expr<T>(InfixOperation<T>(
                       descr.op_name, std::move(left_op.value().first),
                       std::move(right_op.value().first))),
                   right_op.value().second});
            }
            return std::nullopt;
          }
        },
        x));
    if (result.has_value())
      return result;
    iter_count++;
  }

  auto base_parse_res = base_parser.parse(str);
  RETURN_NULLOPT_IF_NO_VALUE(base_parse_res);
  return std::make_optional<std::pair<Expr<T>, string_view>>(
      {Expr<T>(T(base_parser.parse(str).value().first)),
       base_parse_res.value().second});
}

template <typename T>
Parser<Expr<T>> buildExpressionParser(std::span<ExprType> table,
                                      const Parser<T> &base_parser) {
  using RetType = std::optional<std::pair<Expr<T>, string_view>>;
  return Parser<Expr<T>>(Fn<RetType(string_view)>(
      [=](string_view str) { return buildExpr(table, base_parser, str); }));
}

#endif
