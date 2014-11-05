#ifndef INC_CALIBER_SRC_PATHS_HPP
#define INC_CALIBER_SRC_PATHS_HPP

#include <string>

// Try to use N4082's string_view class, or fall back to Boost's.
#ifdef __has_include
#  if __has_include(<experimental/string_view>)
#    include <experimental/string_view>
#    define CALIBER_STRING_VIEW std::experimental::string_view
#  else
#    include <boost/utility/string_ref.hpp>
#    define CALIBER_STRING_VIEW boost::string_ref
#  endif
#else
#  include <boost/utility/string_ref.hpp>
#  define CALIBER_STRING_VIEW boost::string_ref
#endif

namespace caliber {

std::string which(const std::string &command);

std::string parent_path(const CALIBER_STRING_VIEW &filename);
std::string leafname(const CALIBER_STRING_VIEW &filename);

} // namespace caliber

#endif
