#ifndef EXPRCLASSESHPP
#define EXPRCLASSESHPP
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <variant>

template <typename T> struct Expr;

template <typename T> class InfixOperation {
public:
  std::unique_ptr<Expr<T>> lhs, rhs;
  std::string type;
  InfixOperation(std::string s, Expr<T> x, Expr<T> y);
  InfixOperation();
  InfixOperation(InfixOperation &&e);
  InfixOperation &operator=(InfixOperation &&e);
};
template <typename T> class PrefixOperation {
public:
  std::unique_ptr<Expr<T>> a;
  std::string type;
  PrefixOperation(std::string s, Expr<T> x);
  PrefixOperation();
  PrefixOperation(PrefixOperation &&e);
  PrefixOperation &operator=(PrefixOperation &&e);
};
template <typename T> class PostfixOperation {
public:
  std::unique_ptr<Expr<T>> a;
  std::string type;
  PostfixOperation(std::string s, Expr<T> x);
  PostfixOperation();
  PostfixOperation(PostfixOperation &&e);
  PostfixOperation &operator=(PostfixOperation &&e);
};
/*
 * data Expr =  Add Expr Expr
 *						| Mul Expr Expr
 *						| Assign Expr Expr
 *						| Incr Expr
 *	in haskell lol kill me
 */
template <typename T> struct Expr {
  std::variant<InfixOperation<T>, PrefixOperation<T>, PostfixOperation<T>, T>
      tree;
  Expr() : tree(T()) {}
  template <typename U> Expr(U t) : tree(std::move(t)) {}
  Expr(Expr &&e);
  Expr &operator=(Expr &&e);
};

template <typename T>
InfixOperation<T>::InfixOperation(std::string s, Expr<T> x, Expr<T> y)
    : lhs(std::make_unique<Expr<T>>(std::move(x))),
      rhs(std::make_unique<Expr<T>>(std::move(y))), type(s) {}
template <typename T> InfixOperation<T>::InfixOperation() = default;
template <typename T> InfixOperation<T>::InfixOperation(InfixOperation<T> &&x) {
  *this = std::move(x);
}
template <typename T>
InfixOperation<T> &InfixOperation<T>::operator=(InfixOperation<T> &&x) {
  type = x.type;
  lhs = std::move(x.lhs);
  rhs = std::move(x.rhs);
  return *this;
}

template <typename T>
PrefixOperation<T>::PrefixOperation(std::string s, Expr<T> x)
    : a(std::make_unique<Expr<T>>(std::move(x))), type(s) {}

template <typename T> PrefixOperation<T>::PrefixOperation() = default;
template <typename T>
PrefixOperation<T>::PrefixOperation(PrefixOperation<T> &&x) {
  *this = std::move(x);
}

template <typename T>
PrefixOperation<T> &PrefixOperation<T>::operator=(PrefixOperation<T> &&x) {
  type = x.type;
  a = std::move(x.a);
  return *this;
}

template <typename T>
PostfixOperation<T>::PostfixOperation(std::string s, Expr<T> x)
    : a(std::make_unique<Expr<T>>(std::move(x))), type(s) {}
template <typename T> PostfixOperation<T>::PostfixOperation() = default;

template <typename T>
PostfixOperation<T>::PostfixOperation(PostfixOperation<T> &&x) {
  *this = std::move(x);
}
template <typename T>
PostfixOperation<T> &PostfixOperation<T>::operator=(PostfixOperation<T> &&x) {
  type = x.type;
  a = std::move(x.a);
  return *this;
}

template <typename T> Expr<T>::Expr(Expr<T> &&e) { *this = std::move(e); }

template <typename T> Expr<T> &Expr<T>::operator=(Expr<T> &&e) {
  this->tree = std::move(e.tree);
  return *this;
}

enum class Assoc { Left, Right };

struct TypeDescription {
  std::string op;
  std::string op_name;
  Assoc associativity;
  TypeDescription(std::string op, std::string op_name, Assoc assoc)
      : op(std::move(op)), op_name(std::move(op_name)), associativity(assoc) {}
};

struct INFIX : TypeDescription {
  INFIX(std::string op, std::string op_name, Assoc assoc)
      : TypeDescription(std::move(op), std::move(op_name), assoc) {}
};
struct PREFIX : TypeDescription {
  PREFIX(std::string op, std::string op_name, Assoc assoc)
      : TypeDescription(std::move(op), std::move(op_name), assoc) {}
};
struct POSTFIX : TypeDescription {
  POSTFIX(std::string op, std::string op_name, Assoc assoc)
      : TypeDescription(std::move(op), std::move(op_name), assoc) {}
};

using ExprType = std::variant<PREFIX, INFIX, POSTFIX>;

[[maybe_unused]] auto visit_for_ExprTypeDescription =
    [](const auto &x) -> TypeDescription {
  using T = std::decay_t<decltype(x)>;
  if constexpr (std::is_same_v<T, PREFIX>) {
    return TypeDescription(PREFIX(x));
  } else if constexpr (std::is_same_v<T, POSTFIX>) {
    return TypeDescription(POSTFIX(x));
  } else {
    return TypeDescription(INFIX(x));
  }
};

template <typename T> struct T_of_Expr { using type = T; };
template <typename T> struct T_of_Expr<InfixOperation<T>> { using type = T; };
template <typename T> struct T_of_Expr<PrefixOperation<T>> { using type = T; };
template <typename T> struct T_of_Expr<PostfixOperation<T>> { using type = T; };

// used by Expr<T>::tree
[[maybe_unused]] auto get_op_name_of_Expr =
    [](const auto &x) -> std::string_view {
  using T = std::decay_t<decltype(x)>;
  using V = typename T_of_Expr<T>::type;
  if constexpr (std::is_same_v<T, PrefixOperation<V>>) {
    return x.type;
  } else if constexpr (std::is_same_v<T, PostfixOperation<V>>) {
    return x.type;
  } else if constexpr (std::is_same_v<T, InfixOperation<V>>) {
    return x.type;
  } else {
    return typeid(x).name();
  }
};

// used by Expr<T>::tree
[[maybe_unused]] auto get_lhs_rhs_out_of_Infix = [](auto &&x) {
  using T = std::decay_t<decltype(x)>;
  using V = typename T_of_Expr<T>::type;

  if constexpr (std::is_same_v<T, InfixOperation<V>>) {
    return InfixOperation<V>(std::move(x));
  }
  throw std::runtime_error("Not an infix operation. Doesnt have lhs and rhs");
  return InfixOperation<V>();
};

template <typename T> concept Printable = requires(T x) {
  { std::cout << x }
  ->std::same_as<std::ostream &>;
};

template <Printable T>
std::ostream &operator<<(std::ostream &out, const Expr<T> &expr) {
  out << "( ";
  std::visit(
      [&](auto &&x) {
        using U = std::decay_t<decltype(x)>;
        // using V = typename T_of_Expr<U>::type;
        if constexpr (std::is_same_v<U, PrefixOperation<T>>) {
          out << x.type << ' ';
          out << *(x.a.get());
        } else if constexpr (std::is_same_v<U, PostfixOperation<T>>) {
          out << x.type << ' ';
          out << *(x.a.get());
        } else if constexpr (std::is_same_v<U, InfixOperation<T>>) {
          out << x.type << ' ';
          out << *(x.lhs.get()) << ' ';
          out << *(x.rhs.get());
        } else {
          out << x;
        }
      },
      expr.tree);
  out << " )";
  return out;
}

template <typename T> void print_expr(const Expr<T> &expr) {
  std::cout << expr << '\n';
}

#endif
