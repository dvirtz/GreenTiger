#include <boost/spirit/include/lex.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <fstream>
#include <string>

namespace lex = boost::spirit::lex;

enum TokenIds {
  TOK_ID = lex::min_token_id + 100,
  TOK_STRING,
  TOK_INT,
  TOK_COMMA,
  TOK_COLON,
  TOK_SEMICOLON,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_LBRACK,
  TOK_RBRACK,
  TOK_LBRACE,
  TOK_RBRACE,
  TOK_DOT,
  TOK_PLUS,
  TOK_MINUS,
  TOK_TIMES,
  TOK_DIVIDE,
  TOK_EQ,
  TOK_NEQ,
  TOK_LT,
  TOK_LE,
  TOK_GT,
  TOK_GE,
  TOK_AND,
  TOK_OR,
  TOK_ASSIGN,
  TOK_ARRAY,
  TOK_IF,
  TOK_THEN,
  TOK_ELSE,
  TOK_WHILE,
  TOK_FOR,
  TOK_TO,
  TOK_DO,
  TOK_LET,
  TOK_IN,
  TOK_END,
  TOK_OF,
  TOK_BREAK,
  TOK_NIL,
  TOK_FUNCTION,
  TOK_VAR,
  TOK_TYPE
};

static std::string tokenName(TokenIds tokenId) {
  static const std::string tokenNames[] = {
    "ID",     "STRING",    "INT",     "COMMA",  "COLON",  "SEMICOLON", "LPAREN",
    "RPAREN", "LBRACK",    "RBRACK",  "LBRACE", "RBRACE", "DOT",       "PLUS",
    "MINUS",  "TIMES",     "DIVIDE",  "EQ",     "NEQ",    "LT",        "LE",
    "GT",     "GE",        "AND",     "OR",     "ASSIGN", "ARRAY",     "IF",
    "THEN",   "ELSE",      "WHILE",   "FOR",    "TO",     "DO",        "LET",
    "IN",     "END",       "OF",      "BREAK",  "NIL",    "FUNCTION",  "VAR",
    "TYPE",   "COM_START", "COM_END", "ANY"};

  return tokenNames[tokenId - TOK_ID];
}

template <typename Lexer> class TigerLexer : public lex::lexer<Lexer> {
public:
  TigerLexer() {
    using namespace boost::phoenix;
    using namespace lex;

    this->self.add_pattern("COM_START", R"(\/\*)")("COM_END", R"(\*\/)");

    auto initStartComment = [this](auto & /* start */, auto & /* end */,
                                   auto & /* matched */, auto & /* id */,
                                   auto &ctx) {
      ++m_commentCount;
      ctx.set_state_name("COMMENT");
    };

    auto initEndComment = [this](auto & /* start */, auto & /* end */,
                                 auto &matched, auto & /* id */,
                                 auto & /* ctx */) {
      assert(m_commentCount == 0);
      (void)(m_commentCount);
      matched = lex::pass_flags::pass_fail;
    };

    auto commentStartComment = [this](auto & /* start */, auto & /* end */,
                                      auto & /* matched */, auto & /* id */,
                                      auto & /* ctx */) { ++m_commentCount; };

    auto commentEndComment = [this](auto & /* start */, auto & /* end */,
                                    auto & /* matched */, auto & /* id */,
                                    auto &ctx) {
      assert(m_commentCount > 0);
      --m_commentCount;
      if (m_commentCount == 0) {
        ctx.set_state_name("INITIAL");
      }
    };

    auto print = [](auto &start, auto &end, auto & /* matched */, auto &id,
                    auto & /* ctx */) {
      std::cout << tokenName(static_cast<TokenIds>(id));
      switch (id) {
        case TOK_ID:
        case TOK_INT:
        case TOK_STRING:
          std::cout << ": ";
          std::copy(start, end, std::ostream_iterator<char>(std::cout));
          break;
      }
      std::cout << '\n';
    };

    static const std::vector<std::pair<std::string, TokenIds>> punctuations{
      {R"(,)", TOK_COMMA},   {R"(\:)", TOK_COLON},  {R"(;)", TOK_SEMICOLON},
      {R"(\()", TOK_LPAREN}, {R"(\))", TOK_RPAREN}, {R"(\[)", TOK_LBRACK},
      {R"(\])", TOK_RBRACK}, {R"(\{)", TOK_LBRACE}, {R"(\})", TOK_RBRACE},
      {R"(\.)", TOK_DOT},    {R"(\+)", TOK_PLUS},   {R"(-)", TOK_MINUS},
      {R"(\*)", TOK_TIMES},  {R"(\/)", TOK_DIVIDE}, {R"(=)", TOK_EQ},
      {R"(<>)", TOK_NEQ},    {R"(<)", TOK_LT},      {R"(<=)", TOK_LE},
      {R"(>)", TOK_GT},      {R"(>=)", TOK_GE},     {R"(&)", TOK_AND},
      {R"(\|)", TOK_OR},     {R"(:=)", TOK_ASSIGN}};

    static const std::vector<std::pair<std::string, TokenIds>> reservedWords = {
      {"array", TOK_ARRAY}, {"if", TOK_IF},       {"then", TOK_THEN},
      {"else", TOK_ELSE},   {"while", TOK_WHILE}, {"for", TOK_FOR},
      {"to", TOK_TO},       {"do", TOK_DO},       {"let", TOK_LET},
      {"in", TOK_IN},       {"end", TOK_END},     {"of", TOK_OF},
      {"break", TOK_BREAK}, {"nil", TOK_NIL},     {"function", TOK_FUNCTION},
      {"var", TOK_VAR},     {"type", TOK_TYPE}};

    this->self =
      m_initStartComment[initStartComment] | m_initEndComment[initEndComment];

    for (const auto &punctuation : punctuations) {
      this->self.define(
        lex::token_def<>(punctuation.first, punctuation.second)[print]);
    }

    for (const auto &reservedWord : reservedWords) {
      this->self.define(
        lex::token_def<>(reservedWord.first, reservedWord.second)[print]);
    }

    this->self.define("[ \t\n\r]+" | m_identifier[print] | m_string[print]
                      | m_int[print]);

    this->self("COMMENT") = m_commentStartComment[commentStartComment]
                            | m_commentEndComment[commentEndComment]
                            | m_commentAny | "\n";
  }

private:
  lex::token_def<> m_identifier{R"([a-zA-Z][a-zA-Z0-9_]*)", TOK_ID};
  lex::token_def<> m_string{R"(\"[^\"]*\")", TOK_STRING};
  lex::token_def<> m_int{R"([0-9]+)", TOK_INT};
  lex::token_def<> m_initStartComment{"{COM_START}"};
  lex::token_def<> m_initEndComment{"{COM_END}"};
  lex::token_def<> m_commentStartComment{"{COM_START}"};
  lex::token_def<> m_commentEndComment{"{COM_END}"};
  lex::token_def<> m_commentAny{R"(.)"};

  size_t m_commentCount = 0;
};

bool lexFile(const std::string &filename) {
  using iterator = std::string::iterator;
  using token_type =
    lex::lexertl::token<iterator, boost::mpl::vector<std::string, int>,
                        boost::mpl::true_>;
  using lexer_type = lex::lexertl::actor_lexer<token_type>;

  std::ifstream inputFile(filename, std::ios::in);
  if (!inputFile) {
    std::cerr << "failed to read from " << filename << "\n";
    return false;
  }

  TigerLexer<lexer_type> tigerLexer;

  std::string input{std::istreambuf_iterator<char>(inputFile),
                    std::istreambuf_iterator<char>()};

  auto iter = input.begin();
  auto end  = input.end();
  if (!lex::tokenize(iter, end, tigerLexer)) {
    std::cerr << "Error at:\n";
    std::copy(iter, end, std::ostream_iterator<char>(std::cerr));
    std::cerr << '\n';
    return false;
  }
  return true;
}