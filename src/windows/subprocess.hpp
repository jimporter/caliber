#ifndef INC_CALIBER_SRC_WINDOWS_SUBPROCESS_HPP
#define INC_CALIBER_SRC_WINDOWS_SUBPROCESS_HPP

#include <string>

namespace caliber::windows {

  std::string slurp(const char *argv[]);

} // namespace caliber::windows

#endif
