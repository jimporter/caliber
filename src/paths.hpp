#ifndef INC_CALIBER_SRC_PATHS_HPP
#define INC_CALIBER_SRC_PATHS_HPP

#include <string>

namespace caliber {

std::string which(const std::string &command);

std::string parent_path(const std::string_view &filename);
std::string leafname(const std::string_view &filename);

} // namespace caliber

#endif
