#ifndef INC_CALIBER_SRC_POSIX_SUBPROCESS_HPP
#define INC_CALIBER_SRC_POSIX_SUBPROCESS_HPP

#include <string>

namespace caliber::posix {

  std::string slurp(const char *argv[]);

} // namespace caliber::posix

#endif
