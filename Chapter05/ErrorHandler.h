#pragma once
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <iomanip>
#include <iostream>

namespace tiger {
template <typename Iterator> class ErrorHandler {
public:
  template <typename, typename, typename> struct result { typedef void type; };

  template <typename Message, typename What>
  void operator()(Message const &message, What const &what,
                  Iterator err_pos) const {
    const auto &pos = err_pos.get_position();
    std::cerr << message << ": " << what << " at file " << pos.file << " line "
              << pos.line << " column " << pos.column << '\n';
    std::cerr << "'";
    std::copy(err_pos.get_currentline_begin(), err_pos.get_currentline_end(),
              std::ostream_iterator<char>(std::cerr));
    std::cerr << "'\n";
    std::cerr << std::setw(pos.column) << " "
              << "^- here\n";
  }
};
} // namespace tiger