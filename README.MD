# CPParsec

A parser combinator written in C++, inspired by
[Haskell's parsec](https://github.com/haskell/parsec)

- Some of the primitive parser's already defined are:

  - `Char`
  - `Alpha`
  - `Digit`
  - `AlphaNum`
  - `LeftParen`
  - `RightParen`
  - `LeftCurly`
  - `RightCurly`
  - `WhiteSpace`
  - `Tab`
  - `Space`
  - `NewLine`
  - `PosNum`
  - `End`

- Combinators that are available :

  - `lazy(Fn<Parser<T>()> fn)` : Used to wrap a parser inside another parser.
    Useful when there's a lot of recursion involved.

  A use demonstrated below

  ```cpp
  Parser<SubExpression> SubExpressionParser() {
    static const auto subsubparser = SubSubExpressionParser();
    // this makes sure we dont have calls to SubExpression() in all possible
    // paths of function
    static const auto lazy_subexprparser =
  	  lazy<SubExpression>([]() { return SubExpressionParser(); });
    return Parser<SubExpression>(
  	  [](string_view str)
  		  -> std::optional<std::pair<SubExpression, string_view>> {
  		auto sub_sub = subsubparser.parse(str);
  		// if we can parser sub_sub expression, we take this branch
  		if (sub_sub.has_value()) {
  		  // if the string has been consumed entirely
  		  if (sub_sub.value().second.empty()) {
  			return std::make_optional(
  				std::make_pair(SubExpression(std::move(sub_sub.value().first)),
  							   sub_sub.value().second));
  		  }

  		  auto new_str = sub_sub.value().second;
  		  // if the next token is a pipe
  		  auto has_pipe =
  			  Contains(Character('|'))
  				  .parse(new_str); // guaranteed to return true or false

  		  // now parse a subexpression again
  		  auto sub_expr = lazy_subexprparser.parse(has_pipe.value().second);
  		  if (sub_expr.has_value()) {
  			return std::make_optional(std::make_pair(
  				SubExpression((has_pipe.value().first) ? JoinExpr::ALTERNATE
  													   : JoinExpr::CONCAT,
  							  SubExpression(std::move(sub_sub.value().first)),
  							  std::move(sub_expr.value().first)),
  				sub_expr.value().second));
  		  }
  		  return std::nullopt;
  		}
  		return std::nullopt;
  	  });
  }
  ```

  - `zip(Parser<A> a, Parser<B> b)` : zips two parsers together and returns a
    new parser of type `Parser<std::tuple<A,B>>`
  - `Parser<std::tuple<A, B, C>> zip3(Parser<A> a, Parser<B> b, Parser<C> c)` :
    zips 3 parsers together
  - `zipMany(Parser<T>...)` : Takes in variable number of parsers as argument
    and returns a new parser that combines all those parsers into one. See
    `tests.cpp` for usage.
  - `zipAndGet<N>(Parser<T>...)` : Basically a wrapper over zipMany that returns
    at the end a parser of Nth type. This means all the parsed output will be
    ignored except Nth parser's output.
  - `Optional(Parser<T>) -> Parser<bool>` : Returns a parser which tells us
    whether the given parser successfully parsed or not.
  - `oneOf(Parser<T>...)` : Takes in parser of same type and returns the first
    successful parse.
  - `Contains(parser<T>)` : Same as `Optional(Parser<T>)`
  - `Parser<size_t> skipMany(const Parser<T> &parser)` : Returns a parser that
    skips zero or more of the successful parses done the given parser and
    returns the skip count.
  - `Parser<size_t> skipMany1(const Parser<T> &parser)` : same as skipMany1
    except that it must parse atleast once successfully. Returns `std::nullopt`
    on unsuccessful parse.
  - `skipPreWhitespace(Parser<T>)` , `skipPostWhitespace(Parser<T>)` and
    `skipSurrWhitespace(Parser<T>)` : They will return a new parser that parses
    T and skips left whitespaces, right whitespaces and left and right
    whitespaces respectively.
  - `Parser<T> Parens(const Parser<T> &parser)` : Returns a parser that parses
    inside matching parens. Lets say , we have a string "(123)" , then we can do
    `Parens(PosNum)` to parse the number 123. It basically creates the illusion
    that our string is only within balanced parens.
  - `Parser<T> Curlies(const Parser<T> &parser)` : Same as `Parens` but for '{'
    and '}'.
  - `Parser<T> SquareBraces(const Parser<T> &parser)` : Same as `Parens` but for
    '[' and ']'.
  - ` Parser<T> \_InsideMatchingPair(const Parser<T> &parser, char opening = '(', char closing = ')')`
    : Parens , Curlies and SquareBraces just call this with different values for
    opening and closing. Using this, you could easily define a parser for '<'
    and '>' as well.
  - `Parser<std::vector<A>> sepBy(const Parser<A> &separatee, const Parser<B> &separator)`
    : Returns a parser that returns a vector of A's separated by zero or more
    B's. Example demonstrated in `tests.cpp`
  - `Parser<std::vector<A>> sepBy1(const Parser<A> &separatee, const Parser<B> &separator)`
    : Same as `sepBy` except that it should find atleast one A separated by B's.
    Else returns `std::nullopt`

- Other utility functions:

  - `Parser<string_view> String(string_view prefix)` : Returns a parser for the
    given string
  - `Parser<char> Character(char c)` : Returns parser for character c
  - `Parser<char> Characters(std::span<char> chars)` or
    `Parser<char> Characters(const std::array<char, N> &chars)` : Returns a
    character parser that parsers either of the given characters.
  - `Parser<char> Char_excluding(char c)` : Returns a parser that parses any
    character except c.
  - `Char_excluding_many(array of characters or span of characters)`: Returns a
    parser that parses any characters except those in the given array.

- Methods for Parser<T>

  - `filter(Fn)`
  - `map(Fn)` : Returns a new parser of type B that applies the function
    Fn(which takes in T and returns B) to the parsed output of `this` parser.
  - `flatmap(Fn)` : Returns a parser of type B , and takes in an function that
    takes T as input and returns Parser of type B as output. So it basically
    applies Fn to the T that we get from `this` parser and returns the returns
    basically that parser of type B.
  - `oneOrMore()` : returns one or more T's after parsing with `this`
    repeatedly. Will return std::nullopt when there's zero match.
  - `zeroOrMore()` : returns zero or more T's after parsing with `this`
    repeatedly. Will always return(cant return std::nullopt).
  - `andThen(Parser<A> a)` : It will return a parser that at first parses with
    `this` parser and then if it was successful, it goes onto parse with a. If
    both were successful, it returns the result as tuple. `zip` combinators are
    implemented with the help of this.
  - `operator||(const Parser<T> &other)` : Returns a parser that will first try
    to parse with `this` parser. If that fails, then it tries with `other`
    parser. If both fail, returns std::nullopt
  - `orThrow(const char *error_message)` : Instead of return std::nullopt, it
    throws exception. When `this` is of type Parser<bool>, then it throws on
    returning false.

- Usage Instructions:

  - pip install conan (if you dont have it installed already)
  - mkdir build
  - cd build

  > If you want to build tests

  - conan install ..
  - cmake -DBUILD_PROJECT=tests ..
  - make tests

  > For no tests

  - cmake ..
  - make <binary_name>
