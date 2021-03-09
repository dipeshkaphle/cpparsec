#pragma once

#include "util.cpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using std::pair;
using std::string_view;
using opt_string_view = std::optional<std::string_view>;

template <typename T> class Parser {
public:
  Parser() = default;
  Parser(std::function<std::optional<pair<T, string_view>>(string_view)> f) {
    parse = f;
  }
  Parser<T>(const Parser<T> &other) { this->parse = other.parse; }
  Parser<T> operator=(const Parser<T> &other) {
    this->parse = other.parse;
    return *this;
  }

  std::function<std::optional<pair<T, string_view>>(string_view)> parse;

  Parser<T> filter(std::function<bool(T)> pred) {
    std::function<std::optional<T>(T)> f =
        [pred](const T &x) -> std::optional<T> {
      return (pred(x) ? std::make_optional(x) : std::nullopt);
    };
    return map<T>(f);
  }

  // TODO
  // NOTE: Has a lot of issues i think, oneOrMore doesnt work idk why
  template <typename B> Parser<B> map(std::function<std::optional<B>(T)> f) {
    std::function<Parser<B>(T)> g = [f](const T &match) -> Parser<B> {
      return Parser<B>(
          std::function<std::optional<pair<B, string_view>>(string_view)>(
              // f and match are the captures
              [f, match](
                  string_view str) -> std::optional<std::pair<B, string_view>> {
                std::optional<B> x = f(match);
                if (x.has_value()) {
                  return std::make_pair(x.value(), str);
                } else {
                  return std::nullopt;
                }
              }));
    };
    return this->flatmap<B>(g);
  }

  template <typename B> Parser<B> flatmap(std::function<Parser<B>(T)> f) {
    return Parser<B>(
        [f, this](string_view str) -> std::optional<std::pair<B, string_view>> {
          std::optional<std::pair<T, string_view>> x = this->parse(str);
          if (not x.has_value()) {
            return std::nullopt;
          } else {
            auto y = x.value().first;
            Parser<B> p = f(y);
            return p.parse(x.value().second);
            // return x.value();
          }
        });
  }

  Parser<std::vector<T>> oneOrMore() {
    // return this->zeroOrMore().filter(
    //     [](const std::vector<T> &vec) { return !vec.empty(); });
    // TODO, not working rn idk why
    // Parser<std::vector<T>> p = this->zeroOrMore();
    // return
    // p.map(std::function<std::optional<std::vector<T>>(std::vector<T>)>(
    //     [](const std::vector<T> &vec) -> std::optional<std::vector<T>> {
    //       return vec.empty() ? std::nullopt : std::make_optional(vec);
    //     }));
    //
    return Parser<std::vector<T>>([this](string_view str) {
      std::vector<T> matches;
      std::optional<std::pair<T, string_view>> parseRes = this->parse(str);
      while (parseRes.has_value()) {
        matches.push_back(parseRes.value().first);
        str = parseRes.value().second;
        parseRes = this->parse(str);
      }
      return matches.empty() ? std::nullopt
                             : std::make_optional(std::make_pair(matches, str));
    });
  }

  Parser<std::vector<T>> zeroOrMore() {
    return Parser<std::vector<T>>([this](string_view str) {
      std::vector<T> matches;
      std::optional<std::pair<T, string_view>> parseRes = this->parse(str);
      while (parseRes.has_value()) {
        matches.push_back(parseRes.value().first);
        str = parseRes.value().second;
        parseRes = this->parse(str);
      }
      return std::make_optional(std::make_pair(matches, str));
    });
  }

  // If i can make this work then god damnnnnnnn
  template <typename A> auto andThen(Parser<A> a) {
    if constexpr (!is_tuple<T>::value) {
      return Parser<std::tuple<T, A>>(
          [this, a](string_view str)
              -> std::optional<std::pair<std::tuple<T, A>, string_view>> {
            std::optional<pair<T, string_view>> x = this->parse(str);
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
      // std::cout << "TUPLEEEEE:" << num_of_template_params<T>::size <<
      // std::endl;
      auto x = T{};
      std::tuple<A> t;
      auto y = std::tuple_cat(x, t);
      return Parser<decltype(y)>{
          [this, a, y](string_view str)
              -> std::optional<std::pair<decltype(y), string_view>> {
            std::optional<pair<T, string_view>> res = this->parse(str);
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
      // return Parser {};
    }
  }

  // template<> Parser<std::tuple<T>> andThen(){
  // }

  // NOTE: Doesnt work yet. Some template issue
  // template <typename B> Parser<pair<T, B>> zip(Parser<B> b) {
  //   auto g = std::function<Parser<std::pair<T, B>>(T)>([b](T matchT) {
  //     // return b.map(std::function<std::optional<B> (T)> f);
  //     std::function<std::optional<std::pair<T, B>>(B)> f =
  //         ([matchT](B matchB) -> std::optional<std::pair<T, B>> {
  //           return std::make_optional(std::make_pair(matchT, matchB));
  //         });
  //     return b.map(f);
  //   });
  //   return this->flatmap(g);
  // }
  //
}; // Parser class end

// helper functions

// NOTE: doesnt work yet
template <typename A, typename B>
Parser<pair<A, B>> zip(Parser<A> a, Parser<B> b) {
  return Parser<pair<A, B>>{
      [a, b](string_view str) -> std::optional<pair<pair<A, B>, string_view>> {
        std::optional<pair<A, string_view>> x = a.parse(str);
        if (x.has_value()) {
          std::optional<pair<B, string_view>> y = b.parse(x.value().second);
          if (y.has_value()) {
            return std::make_optional(
                std::make_pair(std::make_pair(x.value().first, y.value().first),
                               y.value().second));
          } else {
            return std::nullopt;
          }
        } else {
          return std::nullopt;
        }
      }};
}
template <typename A, typename B, typename C>
Parser<std::tuple<A, B, C>> zip3(Parser<A> a, Parser<B> b, Parser<C> c) {
  return Parser<std::tuple<A, B, C>>{
      [a, b, c](string_view str)
          -> std::optional<pair<std::tuple<A, B, C>, string_view>> {
        std::optional<pair<pair<A, B>, string_view>> x = zip(a, b).parse(str);
        if (x.has_value()) {
          std::optional<pair<C, string_view>> y = c.parse(x.value().second);
          if (y.has_value()) {
            return std::make_optional(std::make_pair(
                std::make_tuple(x.value().first.first, x.value().first.second,
                                y.value().first),
                y.value().second));
          } else {
            return std::nullopt;
          }
        } else {
          return std::nullopt;
        }
      }};
}

namespace Parsers { // Parsers::
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

Parser<char> Char = Parser<char>([](string_view str) {
  return str.empty() ? std::nullopt
                     : std::make_optional(make_pair(str[0], str.substr(1)));
});

Parser<char> Alpha = Char.filter([](char c) { return std::isalpha(c) != 0; });

Parser<char> Digit = Char.filter([](char c) { return std::isdigit(c) != 0; });

Parser<char> AlphaNum = Char.filter(
    [](char c) { return (std::isdigit(c) != 0) || (std::isalpha(c) != 0); });

Parser<char> LeftParen = Char.filter([](char c) { return c == '('; });
Parser<char> RightParen = Char.filter([](char c) { return c == ')'; });
Parser<char> LeftCurly = Char.filter([](char c) { return c == '{'; });
Parser<char> RightCurly = Char.filter([](char c) { return c == '}'; });
Parser<char> WhiteSpace =
    Char.filter([](char c) { return c == ' ' || c == '\n' || c == '\t'; });
Parser<char> Tab = Char.filter([](char c) { return c == '\t'; });
Parser<char> Space = Char.filter([](char c) { return c == ' '; });
Parser<char> NewLine = Char.filter([](char c) { return c == '\n'; });

template <typename T> Parser<T> skipPreWhitespace(Parser<T> p) {
  return Parser<T>{[p](string_view str) {
    auto x = WhiteSpace.zeroOrMore().parse(str);
    return p.parse(x.value().second);
  }};
}
template <typename T> Parser<T> skipPostWhitespace(Parser<T> p) {
  return Parser<T>{
      [p](string_view str) -> std::optional<std::pair<T, string_view>> {
        auto y = p.parse(str);
        if (y.has_value()) {
          auto x = WhiteSpace.zeroOrMore().parse(y.value().second);
          str = x.value().second;
          return std::make_optional(std::make_pair(y.value().first, str));
        }
        return std::nullopt;
      }};
}

template <typename T> Parser<T> skipSurrWhitespace(Parser<T> p) {
  return Parser<T>{
      [p](string_view str) -> std::optional<std::pair<T, string_view>> {
        auto y = skipPreWhitespace(p).parse(str);
        if (y.has_value()) {
          auto z = WhiteSpace.zeroOrMore().parse(y.value().second);
          return std::make_optional(
              std::make_pair(y.value().first, z.value().second));
        }
        return std::nullopt;
      }};
}

// TODO , doesnt work
// Parser<int> PosNum =
//     Digit.oneOrMore().map<int>([](std::vector<char> vec) ->
//     std::optional<int> {
//       std::string res;
//       for (char c : vec)
//         res.push_back(c);
//       return std::optional(std::stoi(res));
//     });

//
Parser<int> PosNum = Parser<int>(
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
          std::make_pair(std::stoi(ans), x.value().second));
    }));
//
} // namespace Parsers
