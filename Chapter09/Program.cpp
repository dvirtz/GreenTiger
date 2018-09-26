#include "Program.h"
#include "CallingConvention.h"
#include "Canonicalizer.h"
#include "CodeGenerator.h"
#include "EscapeAnalyser.h"
#include "ExpressionParser.h"
#include "MachineRegistrar.h"
#include "MachineRegistration.h"
#include "SemanticAnalyzer.h"
#include "Translator.h"
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <fstream>
#include <range/v3/view/transform.hpp>

namespace tiger {

namespace spirit = boost::spirit;
namespace qi     = spirit::qi;

namespace detail {

template <typename Iterator>
CompileResult compile(const std::string &arch, Iterator &first,
                      const Iterator &last) {
  using Grammer      = ExpressionParser<Iterator>;
  using Skipper      = Skipper<Iterator>;
  using ErrorHandler = ErrorHandler<Iterator>;
  using Annotation   = Annotation<Iterator>;

  ErrorHandler errorHandler;
  Annotation annotation;
  Grammer grammer{errorHandler, annotation};
  Skipper skipper;
  EscapeAnalyser escapeAnalyser;

  try {
    ast::Expression ast;

    if (qi::phrase_parse(first, last, grammer, skipper, ast) && first == last) {
      simplifyTree(ast);
#ifdef BOOST_SPIRIT_DEBUG
      BOOST_SPIRIT_DEBUG_OUT << "AST after simplification:\n";
      boost::spirit::traits::print_attribute(BOOST_SPIRIT_DEBUG_OUT, ast);
      BOOST_SPIRIT_DEBUG_OUT << '\n';
#endif

      escapeAnalyser.analyse(ast);

      auto machine            = createMachine(arch);
      auto &callingConvention = machine->callingConvention();
      temp::Map tempMap{machine->predefinedRegisters()};
      SemanticAnalyzer semanticAnalyzer{errorHandler, annotation, tempMap,
                                        callingConvention};
      auto compiled = semanticAnalyzer.compile(ast);

      Canonicalizer canonicalizer{tempMap};
      auto &codeGenerator = machine->codeGenerator();

      using namespace ranges;
      auto instructions =
        compiled | view::transform([&](Fragment &fragment) {
          return helpers::match(fragment)(
            [&](FunctionFragment &function) {
              auto canonicalized =
                canonicalizer.canonicalize(std::move(function.m_body));
              auto translated =
                codeGenerator.translateFunction(canonicalized, tempMap);
              return function.m_frame->procEntryExit3(
                callingConvention.procEntryExit2(translated));
            },
            [&](StringFragment &str) {
              return codeGenerator.translateString(str.m_label, str.m_string,
                                                   tempMap);
            });
        });

      std::stringstream sst;
      print(sst, assembly::joinInstructions(instructions), tempMap);
      return sst.str();
    }

    errorHandler("Parsing failed", "", first);
  } catch (const std::exception &e) { std::cerr << e.what(); }
  return {};
}
} // namespace detail

CompileResult compileFile(const std::string &arch,
                          const std::string &filename) {
  std::ifstream inputFile(filename, std::ios::in);
  if (!inputFile) {
    std::cerr << "failed to read from " << filename << "\n";
    return {};
  }

  using FileIterator    = std::istreambuf_iterator<char>;
  using ForwardIterator = spirit::multi_pass<FileIterator>;
  using Iterator        = spirit::classic::position_iterator2<ForwardIterator>;

  Iterator first{ForwardIterator(FileIterator(inputFile)), ForwardIterator(),
                 filename};
  Iterator last;

  return detail::compile(arch, first, last);
}

CompileResult compile(const std::string &arch, const std::string &string) {
  using ForwardIterator = std::string::const_iterator;
  using Iterator        = spirit::classic::position_iterator2<ForwardIterator>;

  Iterator first{ForwardIterator(string.begin()), ForwardIterator(string.end()),
                 "STRING"};
  Iterator last;

  return detail::compile(arch, first, last);
}

} // namespace tiger
