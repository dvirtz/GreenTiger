#include "Program.h"
#include "CallingConvention.h"
#include "Canonicalizer.h"
#include "CodeGenerator.h"
#include "EscapeAnalyser.h"
#include "ExpressionParser.h"
#include "FlowGraph.h"
#include "LivenessAnalyser.h"
#include "MachineRegistrar.h"
#include "MachineRegistration.h"
#include "SemanticAnalyzer.h"
#include "Translator.h"
#include "irange.h"
#include "printRange.h"
#include <boost/graph/graph_utility.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <fstream>
#include <range/v3/action/join.hpp>
#include <range/v3/action/transform.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>

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

      namespace rv = ranges::views;
      namespace ra = ranges::actions;
      auto const fragments =
        compiled | rv::transform([&](Fragment &fragment) {
          return helpers::match(fragment)(
            [&](FunctionFragment &function) {
              auto canonicalized =
                canonicalizer.canonicalize(std::move(function.m_body));
              auto translated =
                codeGenerator.translateFunction(canonicalized, tempMap);
              auto instructions = function.m_frame->procEntryExit3(
                callingConvention.procEntryExit2(translated));

              return instructions;
            },
            [&](StringFragment &str) {
              return codeGenerator.translateString(str.m_label, str.m_string,
                                                   tempMap);
            });
        });

      auto const flowGraphs =
        fragments
        | rv::transform([](const assembly::Instructions &instructions) {
            return regalloc::FlowGraph{instructions};
          })
        | ranges::to_vector;

      auto const toString = helpers::overload(
        [&tempMap](const temp::Register &reg) { return *tempMap.lookup(reg); },
        [](size_t i) { return std::to_string(i); });

      auto instructions =
        flowGraphs | rv::transform([&](const regalloc::FlowGraph &flowGraph) {
          return irange(boost::vertices(flowGraph))
                 | rv::transform([&](auto v) {
                     std::stringstream sst;
                     flowGraph[v].print(sst, tempMap);
                     sst << "; successors: ";
                     helpers::printRange(
                       sst, irange(boost::adjacent_vertices(v, flowGraph)),
                       toString);
                     sst << " uses: ";
                     helpers::printRange(sst, flowGraph.uses(v), toString);
                     sst << " defs: ";
                     helpers::printRange(sst, flowGraph.defs(v), toString);
                     sst << " isMove: " << std::boolalpha
                         << flowGraph.isMove(v);
                     return sst.str();
                   });
        })
        | ra::join;

      auto const interferenceGraphs =
        flowGraphs
        | rv::transform([&toString,
                         &tempMap](const regalloc::FlowGraph &flowGraph) {
            regalloc::LivenessAnalyser livenessAnalyser{flowGraph, tempMap};
            auto const &interferenceGraph =
              livenessAnalyser.interferenceGraph();
            auto const vertexToString =
              [&interferenceGraph, &toString](
                const regalloc::InterferenceGraph::vertex_descriptor &v) {
                return toString(interferenceGraph[v]);
              };

            return irange(boost::vertices(interferenceGraph))
                   | rv::transform([&vertexToString,
                                    &interferenceGraph](const auto &v) {
                       return std::make_pair(
                         vertexToString(v),
                         irange(boost::adjacent_vertices(v, interferenceGraph))
                           | rv::transform(vertexToString)
                           | ranges::to_<std::set>());
                     })
                   | ranges::to_<
                       std::map<std::string, std::set<std::string>>>();
          })
        | ranges::to_vector;
      return CompileResults{instructions | rv::join('\n'), interferenceGraphs};
    }

    errorHandler("Parsing failed", "", first);
  } catch (const std::exception &e) { std::cerr << e.what(); }
  return {};
} // namespace detail
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

std::ostream &operator<<(std::ostream &ost, const CompileResults &results) {
  ost << results.m_assembly;
  ost << "Interference:\n";
  ranges::for_each(
    results.m_interferenceGraphs,
    [&ost](const std::map<std::string, std::set<std::string>> &g) {
      ranges::for_each(
        g,
        [&ost](const std::pair<const std::string, std::set<std::string>> &e) {
          ost << e.first << " <--> ";
          helpers::printRange(ost, e.second, ranges::ident{});
          ost << '\n';
        });
    });
  return ost;
}

} // namespace tiger
