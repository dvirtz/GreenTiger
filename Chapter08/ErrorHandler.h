#pragma once
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace tiger {
template <typename Iterator> class ErrorHandler {
public:
  template <typename, typename, typename> struct result { typedef void type; };

  template <typename Message, typename What>
  void operator()(Message const &message, What const &what,
                  Iterator err_pos) const {
    class TigerError : public std::logic_error {
      using std::logic_error::logic_error;
    };
    const auto& pos = err_pos.get_position();
    std::stringstream sst;
    sst
      << pos.file << "(" << pos.line << "," << pos.column << "): "
      << message << ": " << what << '\n';
    sst
      << "'";
    std::copy(err_pos.get_currentline_begin(), err_pos.get_currentline_end(), std::ostream_iterator<char>(sst));
    sst << "'\n";
    sst
      << std::setw(pos.column) << " "
      << "^- here\n";
    throw TigerError{ sst.str() };
  }
};
} // namespace tiger
