#include "Parser.hpp"
#include <cassert>
#include <iostream>

using namespace Parsers;

int main() {
  auto digit_check = Digit.parse("12");
  assert(digit_check ==
         std::make_optional(std::make_pair('1', string_view("2"))));
  digit_check = Digit.parse(digit_check.value().second);
  assert(digit_check ==
         std::make_optional(std::make_pair('2', string_view(""))));

  auto oneormore_check = AlphaNum.oneOrMore().parse("a12");
  assert(oneormore_check == std::make_optional(std::make_pair(
                                std::vector{'a', '1', '2'}, string_view(""))));

  auto pos_num_check = PosNum.parse("123a");
  assert(pos_num_check ==
         std::make_optional(std::make_pair(123, string_view("a"))));

  auto lcurly_check = LeftCurly.parse("{)");
  assert(lcurly_check ==
         std::make_optional(std::make_pair('{', string_view(")"))));
  lcurly_check = LeftCurly.parse(lcurly_check.value().second);
  assert(lcurly_check == std::nullopt);

  auto andThenCheck =
      String("Hi").andThen(String("Da")).andThen(String("F")).parse("HiDaD");
  assert(andThenCheck == std::nullopt);
  andThenCheck =
      String("Hi").andThen(String("Da")).andThen(String("D")).parse("HiDaD");
  assert(andThenCheck.has_value());

  auto zips = zip3(Alpha, PosNum, Alpha);
  auto zipcheck = zips.parse("a12b");
  assert(zipcheck.has_value());
  zipcheck = zips.parse("a12");
  assert(not zipcheck.has_value());

  auto space_surr_skip = skipSurrWhitespace(Alpha).parse("    a  ");
  assert(space_surr_skip.has_value());

  auto space_pre_skip = skipPreWhitespace(Alpha).parse("    a  ");
  assert(space_pre_skip.has_value());

  auto zipmanycheck = zipMany(Alpha, Digit, Alpha, Digit);
  auto zipmany_res = zipmanycheck.parse("a1a1");
  assert(zipmany_res.has_value());
  zipmany_res = zipmanycheck.parse("a1aa");
  assert(not zipmany_res.has_value());

  auto or_check = (Digit || Character('c')).parse("c1");
  assert(or_check.has_value());
  or_check = (Digit || Character('c')).parse("d1");
  assert(not or_check.has_value());

  auto oneOfCheck = oneOf(Digit, Alpha).parse("a1");
  assert(oneOfCheck.value().first == 'a');
  auto oneOfCheck2 = oneOf(Digit, Alpha, Character('.')).parse(".a1");
  assert(oneOfCheck2.value().first == '.');
  auto oneOfCheck3 = oneOf(Digit, Alpha, Character('.')).parse(" a1");
  assert(oneOfCheck3.has_value() == false);

  auto zipAndGetCheck = zipAndGet<0>(Alpha, Digit).parse("c1");
  assert(zipAndGetCheck.has_value());

  auto OptionalCheck1 = Optional(String("(")).parse("())");
  assert(OptionalCheck1.value().first == true);
  assert(OptionalCheck1.value().second == "))");

  auto OptionalCheck2 = Optional(String("(")).parse("}}}");
  assert(OptionalCheck2.value().first == false);
  assert(OptionalCheck2.value().second == "}}}");

  auto char_template_specialization = Optional(Parser('.')).parse(".1");
  assert(char_template_specialization.has_value());

  auto excludes1 = Char_excluding(']').parse("]Hi");
  assert(!excludes1.has_value());
  excludes1 = Char_excluding(']').parse("Hi");
  assert(excludes1.has_value());

  auto excludes2 = Char_excluding_many(std::array{'[', ']'}).parse("[Hi");
  assert(!excludes2.has_value());

  auto end = End.parse("");
  assert(end.value().first);

  auto includes1 = Characters(std::array{'a', 'b', 'c'}).parse("ab");
  assert(includes1.has_value());
  includes1 = Characters(std::array{'a', 'b', 'c'}).parse("db");
  assert(!includes1.has_value());

  // auto error = Character('a').orThrow("Couldnt parse a").parse("bc");
  // assert(!error.has_value());

  auto skipmanycheck = skipMany(WhiteSpace).parse("a");
  assert(skipmanycheck.value().first == 0);
  skipmanycheck = skipMany1(WhiteSpace).parse("  a");
  assert(skipmanycheck.value().first == 2);
  skipmanycheck = skipMany1(WhiteSpace).parse("a");
  assert(!skipmanycheck.has_value());
  skipmanycheck = skipMany(WhiteSpace).parse(" a");
  assert(skipmanycheck.value().first == 1);

  auto inside_parens = Parens(PosNum).parse("(12)");
  assert(inside_parens.has_value());
  assert(inside_parens.value().first == 12);
  assert(inside_parens.value().second == "");

  auto inside_curlies = Curlies(PosNum).parse("{12}");
  assert(inside_curlies.has_value());
  assert(inside_curlies.value().first == 12);
  assert(inside_curlies.value().second == "");

  auto sepby_check = sepBy(Char, Character(',')).parse("");
  assert(sepby_check.has_value());
  assert(sepby_check.value().first.empty());
  sepby_check = sepBy(Char, Character(',')).parse("a");
  assert(sepby_check.has_value());
  assert(sepby_check.value().first.size() == 1);
  sepby_check = sepBy(Char, Character(',')).parse("a,");
  assert(!sepby_check.has_value());
  sepby_check = sepBy(Char, Character(',')).parse("a,b");
  assert(sepby_check.has_value());
  assert(sepby_check.value().first.size() == 2);

  auto sepby1_check = sepBy1(Char, Character(',')).parse("");
  assert(!sepby1_check.has_value());
  sepby1_check = sepBy1(Char, Character(',')).parse("a");
  assert(sepby1_check.has_value());
  assert(sepby1_check.value().first.size() == 1);
  assert(sepby1_check.value().second == "");
  sepby1_check = sepBy1(Char, Character(',')).parse("a,");
  assert(!sepby1_check.has_value());
  sepby1_check = sepBy1(Char, Character(',')).parse("a,b");
  assert(sepby1_check.has_value());
  assert(sepby1_check.value().first.size() == 2);
  assert(sepby1_check.value().second == "");

  std::cout << "Passed all tests\n";
}
