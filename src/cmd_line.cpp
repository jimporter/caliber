#include "cmd_line.hpp"

#include <set>

namespace caliber {

namespace detail {
  inline std::string readline(std::istream &is, char delim = '\n') {
    std::string line;
    std::getline(is, line, delim);
    return line;
  }

  inline bool matches_name(std::istream &is, const std::string &name,
                           const char *ws = " \t") {
    while(strchr(ws, is.peek()))
      is.get();
    for(auto c : name) {
      if(is.get() != c)
        return false;
    }
    return strchr(ws, is.get());
  }
}

std::vector<std::string>
extract_comment(std::istream &is, const std::string &name) {
  if(is.get() == '/') {
    char c = is.get();
    if(c == '/') {
      if(detail::matches_name(is, name))
        return boost::program_options::split_unix(detail::readline(is));
    }
    else if(c == '*') {
      if(detail::matches_name(is, name, " \t\n\r")) {
        std::string comment;
        while(true) {
          comment += detail::readline(is, '*');
          if(is.peek() == '/')
            return boost::program_options::split_unix(comment, " \t\n\r");
          else
            comment += '*';
        }
      }
    }
  }

  return {};
}

boost::program_options::options_description
make_per_file_options(per_file_options &opts) {
  using namespace boost::program_options;
  options_description desc("Per-file options");
  desc.add_options()
    ("fail,F", value(&opts.expect_fail)->zero_tokens(),
     "expect the test to fail")
    ("name,n", value(&opts.name), "the test's name")
    ("attr,a", value(&opts.attrs), "the test's attributes")
  ;
  return desc;
}

boost::program_options::options_description
make_compiler_options() {
  using namespace boost::program_options;
  options_description desc;
  desc.add_options()
    (",I", value<std::vector<std::string>>(),
     "add a directory to the include search path")
    (",D", value<std::vector<std::string>>(), "pre-define a macro")
    (",U", value<std::vector<std::string>>(), "undefine a macro")
  ;
  return desc;
}

namespace {
  struct attr_less {
    using is_transparent = void;
    using value_type = std::unique_ptr<mettle::attr_base>;

    bool operator ()(const value_type &lhs, const value_type &rhs) const {
      return lhs->name() < rhs->name();
    }

    bool operator ()(const value_type &lhs, const std::string &rhs) const {
      return lhs->name() < rhs;
    }

    bool operator ()(const std::string &lhs, const value_type &rhs) const {
      return lhs < rhs->name();
    }
  };

  inline std::set<std::unique_ptr<mettle::attr_base>, attr_less>
  make_known_attrs() {
    using namespace mettle;
    std::set<std::unique_ptr<attr_base>, attr_less> known;
    known.insert(std::make_unique<bool_attr>("skip", test_action::skip));
    return known;
  }
}

static std::set<std::unique_ptr<mettle::attr_base>, attr_less> known_attrs(
  make_known_attrs()
);

mettle::attributes make_attributes(const std::vector<std::string> &attrs) {
  mettle::attributes final;
  for(const auto &i : attrs) {
    auto attr = known_attrs.find(i);
    if(attr == known_attrs.end())
      attr = known_attrs.insert(std::make_unique<mettle::bool_attr>(i)).first;
    final.insert({**attr, {}});
  }
  return final;
}

} // namespace caliber
