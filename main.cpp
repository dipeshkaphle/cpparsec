#include "Parser.hpp"
#include <iostream>

using namespace Parsers;

int main() {
  auto x = Digit.parse("12");
  // auto x = Char.parse("12");
  // std::cout << x.has_value() << std::endl;
  if (x.has_value()) {
    std::cout << x.value().first << "||" << x.value().second << std::endl;
  }
  x = Digit.parse(x.value().second);
  if (x.has_value()) {
    std::cout << x.value().first << "||" << x.value().second << std::endl;
  }
  std::cout << "ONE OR MORE\n";
  fflush(stdout);

  auto z = AlphaNum.oneOrMore().parse("a12");
  if (z.has_value()) {
    for (auto x : z->first) {
      std::cout << x;
    }
    std::cout << std::endl << z->second << std::endl;
  } else {
    std::cout << "NO MATCH" << std::endl;
  }

  std::cout << "POSNUM\n";
  auto a = PosNum.parse("123a");
  if (a.has_value()) {
    std::cout << a.value().first << "||" << a.value().second << std::endl;
  } else {
    std::cout << "Failed aiyo" << std::endl;
  }

  auto lcurly = LeftCurly.parse("{)");
  if (lcurly.has_value()) {
    std::cout << lcurly.value().first << "||" << lcurly.value().second
              << std::endl;
    lcurly = RightParen.parse(lcurly.value().second);
    if (lcurly.has_value()) {
      std::cout << "YES PAREN IS THERE(RIGHT)\n";
    }
  } else {
    std::cout << "Failed aiyo" << std::endl;
  }

  // auto pre = String("Hi").parse("Hilooo");
  //
  // auto pre = zip(String("Hi"), String("Da")).parse("HiDaD");
  // const Parser<std::tuple<string_view, string_view>> &manyAndThens =
  //     String("Hi").andThen(String("Da"));
  // auto pre = manyAndThens.parse("HiDaD");
  auto pre =
      String("Hi").andThen(String("Da")).andThen(String("F")).parse("HiDaD");
  if (pre.has_value()) {
    std::cout << std::get<0>(pre.value().first) << "&"
              << std::get<1>(pre.value().first) << "&"
              << "||" << pre.value().second << std::endl;
  } else {
    std::cout << "FAILED F\n";
  }

  auto zips = zip3(Alpha, PosNum, Alpha);
  auto zipcheck = zips.parse("a12b");
  if (zipcheck.has_value()) {
    std::cout << std::get<0>(zipcheck.value().first)
              << std::get<1>(zipcheck.value().first) << std::endl;
    // << std::get<2>(zipcheck.value().first) << std::endl;
  } else {
    std::cout << "Doesnt work\n";
  }

  // Parser<std::tuple<char, char>> alpdi(Alpha.andThen(Digit));
  Parser<std::tuple<char, char, char, char>> alpdialpdi(
      Alpha.andThen(Digit).andThen(Alpha).andThen(Digit));
  auto s = alpdialpdi.parse("a1a2");
  if (s.has_value()) {
    std::cout << "Not temp ig\n";
  } else {
    std::cout << "Defo temp\n";
  }

  auto andThenCheck = Digit.andThen(Alpha).andThen(Digit).parse("1a1");
  if (andThenCheck.has_value()) {

    std::cout << (std::get<0>(andThenCheck.value().first))
              << std::get<1>(andThenCheck.value().first)
              << std::get<2>(andThenCheck.value().first) << "||"
              << andThenCheck.value().second << std::endl;
  }

  // Parse a range Quantifier
  auto r = LeftCurly.andThen(PosNum)
               .andThen(String(","))
               .andThen(PosNum)
               .andThen(RightCurly)
               .parse("{1,2}");
  if (r.has_value()) {
    std::cout << "BROOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n";
  } else {
    std::cout << "OH NOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOo\n";
  }

  auto space = skipSurrWhitespace(Alpha).parse("    a  ");
  if (space.has_value()) {
    std::cout << "WORKS\n" << space.value().first << "\n";
  } else {
    std::cout << "DOESNT WOKR\n";
  }

  auto zipmanycheck = experimental::zipMany(Alpha, Digit, Alpha, Digit);
  auto res = zipmanycheck.parse("a1a1");
  if (res.has_value()) {
    std::cout << "Has value\n";
  } else {
    std::cout << "Oh well\n";
  }
}
