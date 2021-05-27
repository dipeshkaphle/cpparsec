#ifndef PARSERHPP
#define PARSERHPP

#include "util.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#define RETURN_NULLOPT_IF_NO_VALUE(opt)                                        \
  if (!opt.has_value())                                                        \
    return std::nullopt;

#define RETURN_OPT_IF_HAS_VALUE(opt)                                           \
  if (opt.has_value())                                                         \
    return opt;

#define RETURN_VALUE_IF_HAS_VALUE(opt)                                         \
  if (opt.has_value())                                                         \
    return opt.value();

#define OPTIONAL_MAP(opt, func)                                                \
  (!opt.has_value()) ? std::nullopt : std::make_optional(func(opt.value()))

using std::pair;
using std::string_view;
using opt_string_view = std::optional<std::string_view>;
template <typename T> using Fn = std::function<T>;

template <typename T> class Parser {
public:
  std::function<std::optional<pair<T, string_view>>(string_view)> parse;
  Parser() = default;
  Parser(std::function<std::optional<pair<T, string_view>>(string_view)> f) {
    parse = f;
  }
  Parser(const T &val) = delete;
  Parser<T>(const Parser<T> &other) { this->parse = other.parse; }
  Parser<T> operator=(const Parser<T> &other) {
    this->parse = other.parse;
    return *this;
  }
  Parser<T> operator||(const Parser<T> &other) const {
    return Parser<T>([other, this_obj = *this](string_view str) {
      auto x = this_obj.parse(str);

      RETURN_OPT_IF_HAS_VALUE(x);
      // if (x.has_value()) {
      //   return x;
      // }
      return other.parse(str);
    });
  }

  Parser<T> filter(std::function<bool(T)> pred) const {
    std::function<std::optional<T>(T)> f =
        [pred](const T &x) -> std::optional<T> {
      return (pred(x) ? std::make_optional(x) : std::nullopt);
    };
    return map<T>(f);
  }

  template <typename B>
  Parser<B> map(std::function<std::optional<B>(T)> f) const {
    std::function<Parser<B>(T)> g = [f](const T &match) -> Parser<B> {
      return Parser<B>(
          std::function<std::optional<pair<B, string_view>>(string_view)>(
              // f and match are the captures
              [f, match](
                  string_view str) -> std::optional<std::pair<B, string_view>> {
                std::optional<B> x = f(match);
                // if (x.has_value()) {
                //   return std::make_pair(x.value(), str);
                // } else {
                //   return std::nullopt;
                // }
                //
                RETURN_NULLOPT_IF_NO_VALUE(x);
                // else this
                return std::make_pair(x.value(), str);
              }));
    };
    return this->flatmap<B>(g);
  }

  template <typename B> Parser<B> flatmap(std::function<Parser<B>(T)> f) const {
    return Parser<B>([f, this_obj = *this](string_view str)
                         -> std::optional<std::pair<B, string_view>> {
      std::optional<std::pair<T, string_view>> x = this_obj.parse(str);

      RETURN_NULLOPT_IF_NO_VALUE(x);

      auto y = x.value().first;
      Parser<B> p = f(y);
      return p.parse(x.value().second);
      // return x.value();
    });
  }

  Parser<std::vector<T>> oneOrMore() const {
    return this->zeroOrMore().filter(
        [](const std::vector<T> &vec) { return !vec.empty(); });
  }

  Parser<std::vector<T>> zeroOrMore() const {
    return Parser<std::vector<T>>([this_obj = *this](string_view str) {
      std::vector<T> matches;
      std::optional<std::pair<T, string_view>> parseRes = this_obj.parse(str);
      while (parseRes.has_value()) {
        matches.push_back(parseRes.value().first);
        str = parseRes.value().second;
        parseRes = this_obj.parse(str);
      }
      return std::make_optional(std::make_pair(matches, str));
    });
  }

  // If i can make this work then god damnnnnnnn
  template <typename A> auto andThen(const Parser<A> &a) const {
    if constexpr (!is_tuple<T>::value) {
      return Parser<std::tuple<T, A>>(
          [this_obj = *this, a](string_view str)
              -> std::optional<std::pair<std::tuple<T, A>, string_view>> {
            std::optional<pair<T, string_view>> x = this_obj.parse(str);
            if (x.has_value()) {
              std::optional<pair<A, string_view>> y = a.parse(x.value().second);
              if (y.has_value()) {
                return std::make_optional(std::make_pair(
                    std::make_tuple(x.value().first, y.value().first),
                    y.value().second));
              }
            }
            return std::nullopt;
          });
    } else {
      return Parser<decltype(
          std::tuple_cat(std::declval<T>(), std::declval<std::tuple<A>>()))>{
          [this_obj = *this, a](string_view str)
              -> std::optional<std::pair<decltype(std::tuple_cat(
                                             std::declval<T>(),
                                             std::declval<std::tuple<A>>())),
                                         string_view>> {
            std::optional<pair<T, string_view>> res = this_obj.parse(str);
            if (res.has_value()) {
              std::optional<pair<A, string_view>> res2 =
                  a.parse(res.value().second);
              if (res2.has_value()) {
                auto tup1 = res.value().first;
                std::tuple<A> tup2(res2.value().first);
                return std::make_optional(std::make_pair(
                    std::tuple_cat(tup1, tup2), res2.value().second));
              }
            }
            return std::nullopt;
          }};
    }
  }

  Parser<T> orThrow(const char *error_msg) {
    return Parser<T>((Fn<std::optional<pair<T, string_view>>(string_view)>)
                         [ error_msg, this_obj = *this ](string_view str) {
                           auto res = this_obj.parse(str);

                           RETURN_OPT_IF_HAS_VALUE(res);
                           // else throw error
                           throw std::runtime_error(error_msg);
                         });
  }

}; // Parser class end

// char template specializtion for Parser
template <> Parser<char>::Parser(const char &c) {
  this->parse = ([c](string_view str) {
    return str.empty() ? std::nullopt
           : (str[0] == c)
               ? std::make_optional(make_pair(str[0], str.substr(1)))
               : std::nullopt;
  });
}

// orThrow specialization for bool types
// We want it to throw when it returns false
template <> Parser<bool> Parser<bool>::orThrow(const char *error_msg) {
  return Parser<bool>([*this, error_msg](string_view str) {
    auto res = this->parse(str);
    // not checking for nullopt cuz bool shouldn't return nullopt
    if (res.value().first) {
      return res;
    } else {
      throw std::runtime_error(error_msg);
    }
  });
}

namespace Parsers { // Parsers::

template <typename T> Parser<T> lazy(Fn<Parser<T>()> fn) {
  return Parser<T>([fn](string_view str) { return fn().parse(str); });
}

template <typename A, typename B>
Parser<std::tuple<A, B>> zip(const Parser<A> &a, const Parser<B> &b) {
  return a.andThen(b);
}

template <typename A, typename B, typename C>
Parser<std::tuple<A, B, C>> zip3(const Parser<A> &a, const Parser<B> &b,
                                 const Parser<C> &c) {
  return a.andThen(b).andThen(c);
}

template <typename A> Parser<A> zipMany(const Parser<A> &a) { return a; }

template <typename A, typename B, typename... T>
auto zipMany(const Parser<A> &a, const Parser<B> &b,
             const Parser<T> &...parsers) {
  if constexpr (!is_tuple<A>::value) {
    return zipMany(a.andThen(b), parsers...);
  } else {
    return zipMany(a.andThen(b), parsers...);
  }
}

template <std::size_t N, typename... T>
auto zipAndGet(const Parser<T> &...parsers) {
  return zipMany(std::forward<decltype(parsers)>(parsers)...)
      .map((std::function<std::optional<typename nth_type<N, T...>::Type>(
                std::tuple<T...>)>)[](const std::tuple<T...> &x)
               ->std::optional<typename nth_type<N, T...>::Type> {
                 return std::make_optional(std::get<N>(x));
               });
}

template <typename... T> auto oneOf(const Parser<T> &...parsers) {
  return (... || parsers);
}

template <typename T> Parser<bool> Optional(const Parser<T> &parser) {
  return Parser<bool>([parser](string_view str) {
    auto x = parser.parse(str);
    if (x.has_value()) {
      return std::make_optional(std::make_pair(true, x.value().second));
    } else {
      return std::make_optional(std::make_pair(false, str));
    }
  });
}

template <typename T> inline Parser<bool> Contains(const Parser<T> &parser) {
  return Optional(parser);
}

Parser<string_view> String(string_view prefix) {
  return Parser<string_view>(
      [prefix](string_view str)
          -> std::optional<std::pair<string_view, string_view>> {
        auto x = str.substr(0, prefix.size());
        if (x == prefix) {
          return make_pair(prefix, str.substr(prefix.size()));
        } else {
          return std::nullopt;
        }
      });
}

Parser<char> Char([](string_view str) {
  return str.empty() ? std::nullopt
                     : std::make_optional(make_pair(str[0], str.substr(1)));
});

Parser<char> Character(char c) {
  return Char.filter(std::bind1st(std::equal_to<char>(), c));
}

Parser<char> Characters(std::span<char> chars) {
  {
    return Char.filter([chars](char c) {
      return std::any_of(chars.begin(), chars.end(),
                         std::bind1st(std::equal_to<char>(), c));
    });
  }
}

template <size_t N> Parser<char> Characters(const std::array<char, N> &chars) {
  {
    return Char.filter([arr = chars](char c) {
      return std::any_of(arr.begin(), arr.end(),
                         std::bind1st(std::equal_to<char>(), c));
    });
  }
}

Parser<char> Char_excluding(char c) {
  return Char.filter(std::bind1st(std::not_equal_to<char>(), c));
}

Parser<char> Char_excluding_many(std::span<char> chars) {
  return Char.filter([chars](char c) {
    return std::none_of(chars.begin(), chars.end(),
                        std::bind1st(std::equal_to<char>(), c));
  });
}

template <size_t N>
Parser<char> Char_excluding_many(const std::array<char, N> &chars) {
  return Char.filter([arr = chars](char c) {
    return std::none_of(arr.begin(), arr.end(),
                        std::bind1st(std::equal_to<char>(), c));
  });
}

const Parser<char> Alpha =
    Char.filter([](char c) { return std::isalpha(c) != 0; });

const Parser<char> Digit =
    Char.filter([](char c) { return std::isdigit(c) != 0; });

const Parser<char> AlphaNum = Char.filter(
    [](char c) { return (std::isdigit(c) != 0) || (std::isalpha(c) != 0); });

const Parser<char> LeftParen = Char.filter([](char c) { return c == '('; });
const Parser<char> RightParen = Char.filter([](char c) { return c == ')'; });
const Parser<char> LeftCurly = Char.filter([](char c) { return c == '{'; });
const Parser<char> RightCurly = Char.filter([](char c) { return c == '}'; });
const Parser<char> WhiteSpace =
    Char.filter([](char c) { return c == ' ' || c == '\n' || c == '\t'; });
const Parser<char> Tab = Char.filter([](char c) { return c == '\t'; });
const Parser<char> Space = Char.filter([](char c) { return c == ' '; });
const Parser<char> NewLine = Char.filter([](char c) { return c == '\n'; });
const Parser<bool>
    End((Fn<std::optional<std::pair<bool, string_view>>(string_view)>)[](
        string_view str) {
      return std::make_optional(std::make_pair(str.empty(), str));
    });

template <typename T> Parser<T> skipPreWhitespace(const Parser<T> &p) {
  static const auto whitespace_zero_or_more = WhiteSpace.zeroOrMore();
  return zipAndGet<1>(whitespace_zero_or_more, p);
}
template <typename T> Parser<T> skipPostWhitespace(const Parser<T> &p) {
  static const auto whitespace_zero_or_more = WhiteSpace.zeroOrMore();
  return zipAndGet<0>(p, whitespace_zero_or_more);
}

template <typename T> Parser<T> skipSurrWhitespace(const Parser<T> &p) {
  static const auto whitespace_zero_or_more = WhiteSpace.zeroOrMore();
  return zipAndGet<1>(whitespace_zero_or_more, p, whitespace_zero_or_more);
}

//
const Parser<size_t> PosNum = Parser<size_t>(
    ([](string_view str) -> std::optional<std::pair<int, string_view>> {
      std::optional<std::pair<std::vector<char>, string_view>> x =
          Digit.oneOrMore().parse(str);
      if (!x.has_value()) {
        return std::nullopt;
      }
      std::string ans;
      for (char c : x.value().first) {
        ans.push_back(c);
      }
      return std::make_optional(
          std::make_pair(std::stoull(ans), x.value().second));
    }));
const Parser<long long> Num = Parser<long long>(
    [](string_view str) -> std::optional<std::pair<long long, string_view>> {
      auto negative_sign = Character('-').parse(str);
      auto remaining_str =
          negative_sign.has_value() ? negative_sign->second : str;
      return PosNum
          .map<long long>([&](size_t n) -> std::optional<long long> {
            long long x = n;
            return negative_sign.has_value() ? std::make_optional(-x)
                                             : std::make_optional(x);
          })
          .parse(remaining_str);
    });
//

template <typename T> Parser<size_t> skipMany(const Parser<T> &parser) {
  return parser.zeroOrMore().map(Fn<std::optional<size_t>(std::vector<T>)>(
      [](const std::vector<T> &vec) -> std::optional<size_t> {
        return std::make_optional(vec.size());
      }));
}

template <typename T> Parser<size_t> skipMany1(const Parser<T> &parser) {
  return parser.oneOrMore().map(Fn<std::optional<size_t>(std::vector<T>)>(
      [](const std::vector<T> &vec) -> std::optional<size_t> {
        return std::make_optional(vec.size());
      }));
}

// can match any two pair of characters and the string inside that will be
// parsed with the parser
template <typename T>
Parser<T> _InsideMatchingPair(const Parser<T> &parser, char opening = '(',
                              char closing = ')') {
  return Parser<T>(
      [parser, opening,
       closing](string_view str) -> std::optional<std::pair<T, string_view>> {
        if (str.empty())
          return std::nullopt;
        if (str[0] != opening)
          return std::nullopt;
        std::vector<char> S;
        S.push_back(opening);
        size_t i = 1;
        // trying to balance the opening and closing characters
        while (!S.empty() && (i < str.size())) {
          if (str[i] == closing && S.back() == opening)
            S.pop_back();
          else if (str[i] == closing)
            return std::nullopt; // no matching pair
          else if (str[i] == opening)
            S.push_back(opening);
          i++;
        }
        if (S.empty()) { // parens match
                         // i is currently at one index ahead of matching paren
          auto temp_result = parser.parse(str.substr(1, (i - 2)));
          // shouldve parsed somehting and that shoudve consumed the entire
          // thing inside the matching par
          if (temp_result.has_value() && temp_result.value().second.empty()) {
            return std::make_optional(
                std::make_pair(temp_result.value().first, str.substr(i)));
          }
        }
        return std::nullopt; // parens are not matching
      });
}

template <typename T> Parser<T> Parens(const Parser<T> &parser) {
  return _InsideMatchingPair(parser, '(', ')');
}

template <typename T> Parser<T> Curlies(const Parser<T> &parser) {
  return _InsideMatchingPair(parser, '{', '}');
}

template <typename T> Parser<T> SquareBraces(const Parser<T> &parser) {
  return _InsideMatchingPair(parser, '[', ']');
}

template <typename A, typename B>
Parser<std::vector<A>> sepBy(const Parser<A> &separatee,
                             const Parser<B> &separator) {
  return Parser<std::vector<A>>(
      // can give empty string
      // But fails if the pattern is not right
      // Empty string will return value while illformed strings wont
      //
      //
      // for separator , "" will return while "," wont return and "a," wont
      // return and "a,b" will return
      [separatee, separator](string_view str)
          -> std::optional<std::pair<std::vector<A>, string_view>> {
        std::vector<A> final_res;
        string_view ret_str = str;
        auto first_A = separatee.parse(str);

        if (first_A.has_value()) {
          final_res.push_back(first_A.value().first);
          ret_str = first_A.value().second;

          while (true) {
            if (ret_str.empty())
              break;
            auto _throw_away = separator.parse(ret_str);

            RETURN_NULLOPT_IF_NO_VALUE(_throw_away);

            ret_str = _throw_away.value().second;
            auto next_A = separatee.parse(ret_str);

            RETURN_NULLOPT_IF_NO_VALUE(next_A);

            final_res.push_back(next_A.value().first);
            ret_str = next_A.value().second;
          }
        }
        return std::make_optional(std::make_pair(final_res, ret_str));
      });
}

template <typename A, typename B>
Parser<std::vector<A>> sepBy1(const Parser<A> &separatee,
                              const Parser<B> &separator) {
  return sepBy(separatee, separator).filter([](auto &&vec) {
    return vec.size() >= 1;
  });
}

} // namespace Parsers
#endif
