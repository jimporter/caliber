#include "cmd_line.hpp"

#include <set>

namespace caliber {

  namespace {
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

    // Some types whose only job is to be a placeholder for args that get
    // forwarded to another command.
    struct fwd_value {};
    void validate(boost::any &v, const std::vector<std::string> &, fwd_value *,
                  int) {
      boost::program_options::validators::check_first_occurrence(v);
    }

    struct fwd_vector {};
    void validate(boost::any &, const std::vector<std::string> &, fwd_vector *,
                  int) {}
  }

  std::vector<std::string>
  extract_comment(std::istream &is, const std::string &name) {
    if(is.get() == '/') {
      char c = is.get();
      if(c == '/') {
        if(matches_name(is, name))
          return boost::program_options::split_unix(readline(is));
      } else if(c == '*') {
        if(matches_name(is, name, " \t\n\r")) {
          std::string comment;
          while(true) {
            comment += readline(is, '*');
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
      ("name,n", value(&opts.name)->value_name("NAME"), "the test's name")
      ("attr,a", value(&opts.attrs)->value_name("ATTR"),
       "the test's attributes")
      ("compiler,c", value(&opts.compilers)->value_name("NAME"),
       "the compiler to use for this test")
      (",X", value(&opts.raw_args)->value_name("FLAVOR=OPTION"),
       "forward untranslated argument directly to the compiler being used")
    ;
    return desc;
  }

  boost::program_options::options_description
  make_compiler_options() {
    using namespace boost::program_options;
    options_description desc;
    desc.add_options()
      (",I", value<fwd_vector>()->value_name("PATH"),
       "add a directory to the include search path")
      (",D", value<fwd_vector>()->value_name("MACRO"), "pre-define a macro")
      (",U", value<fwd_vector>()->value_name("MACRO"), "undefine a macro")
      ("std", value<fwd_value>()->value_name("LANG"), "language standard")
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

  void validate(boost::any &v, const std::vector<std::string> &values,
                raw_option *, int) {
    using namespace boost::program_options;
    validators::check_first_occurrence(v);
    const std::string &val = validators::get_single_string(values);

    try {
      size_t i = val.find('=');
      if(i != std::string::npos)
        v = raw_option{val.substr(0, i), val.substr(i + 1)};
      else
        v = raw_option{"", val};
    }
    catch(...) {
      boost::throw_exception(invalid_option_value(val));
    }
  }

} // namespace caliber
