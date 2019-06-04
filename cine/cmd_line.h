/*! \file cmd_line.h
* \brief simple command line parser
* \author Hanno Hildenbrandt
*/

#ifndef CMD_LINE_H_INCLUDED
#define CMD_LINE_H_INCLUDED

#include <stdexcept>
#include <cassert>
#include <string.h>
#include <sstream>
#include <utility>
#include <string>
#include <functional>
#include <type_traits>
#include <regex>
#include <set>
#include <vector>


//! \brief namespace of the command line parser
namespace cmd {


  class parse_error
  {
  public:
    parse_error(const std::string& msg) : msg_(msg) 
    {
    }
    const char* what() const { return msg_.c_str(); }

  private: 
    std::string msg_;
  };


  template <typename T>
  class parse_vector
  {
  public:
    using value_type = T;

    explicit parse_vector(char lb = '{', char rb = '}', char delim = ',')
      : lb_(lb), rb_(rb), delim_(delim)
    {
    }

    friend std::istream& operator>>(std::istream& is, parse_vector<T>& val)
    {
      val.res_.clear();
      char chr = '\0';
      is >> chr;
      if (chr != val.lb_) throw parse_error(std::string("'") + val.lb_ + " expected");
      std::string arg;
      while (is >> chr) {
        if (chr == val.delim_ || chr == val.rb_) {
          std::stringstream ss(arg);
          T x;
          if (!(ss >> x)) throw parse_error("invalid argument");
          val.res_.push_back(x);
          arg.clear();
          if (chr == val.rb_) break;
          continue;
        }
        arg += chr;
      } 
      if (chr != val.rb_) throw parse_error(std::string("'") + val.rb_ + "' expected");
      return is;
    }

    char lb_, rb_, delim_;
    std::vector<T> res_;
  };



  //! \brief command line parser
  class cmd_line_parser
  {
  public:
    //! ctor main() interface
    cmd_line_parser(int argc, const char** argv)
    {
      for (int i=0; i<argc; ++i) argv_.emplace_back(argv[i]);
    }


    //! ctor single white-space delimited string
    explicit cmd_line_parser(const char* cmdline)
    {
      char buf[2048];
      const char* p0 = cmdline;
      for (;;)
      {
        while (isspace(*p0)) ++p0;
        if (*p0 == '\0') break;
        char* pb = buf;
        std::function<bool(char)> delim = [](char chr) { return 0 != isspace(chr); };
        while (!delim(*p0))
        {
          if (*p0 == '\0') break;
          if (*p0 == '\"') 
          {
            p0++; 
            delim = [](char chr) { return chr == '\"'; };
          }
          if (*p0 == '#') 
          {
            while (*p0 && *p0 != '\n') ++p0; 
          }
          else
          {
            *pb++ = *p0++;
          }
        }
        if (*p0) ++p0;
        *pb = '\0';
        if (buf[0]) argv_.push_back(buf);
      }
    }


    //! ctor single white-space delimited string
    explicit cmd_line_parser(const std::string& cmdline) : cmd_line_parser(cmdline.c_str())
    {}


    //! ctor single white-space delimited string
    explicit cmd_line_parser(std::vector<std::string>& argv) : argv_(std::move(argv))
    {}


    //! adds the entries from the second clp
    cmd_line_parser& append(const cmd_line_parser& rhs)
    {
      argv_.insert(argv_.end(), rhs.argv_.cbegin(), rhs.argv_.cend());
      return *this;
    }


    //! Returns true if \p name exists in argument, false otherwise
    //! \param name the name of the flag
    bool flag(const char* name) const;


    //! \brief Parse name value pair
    //! \tparam T the type to parse
    //! \param name the name
    //! \param val the value
    //! \param delim delimiter
    //! \returns true if the name-value could be read.
    //!
    //! \p val contains the parsed value on success, otherwise
    //! its unchanged.
    template <typename T>
    bool optional(const char* name, T& val, char delim = '=') const;


    //! \brief Parse name value pair
    //! \tparam T the type to parse
    //! \param name the name
    //! \param val the value
    //! \param delim delimiter
    //! \returns the optional value
    template <typename T>
    T optional_val(const char* name, T val, char delim = '=') const;


    //! \brief Parse name-value pair
    //! \tparam T the return type
    //! \param name the name
    //! \param delim delimiter
    //! \returns the parsed value on success.
    //!
    //! Throws parse_error on failure.
    template <typename T>
    T required(const char* name, char delim = '=') const;


    template <typename C>
    bool optional_vec(const char* name, C& val, char delim = '=') const;


    //! \brief Checks for unwanted arguments
    //! \return vector of superfluous argument names
    std::vector<std::string> unrecognized() const;


    //! \brief returns command line [1,end]
    std::string argv() const;


  private:
    std::vector<std::string> argv_;
    mutable std::set<std::pair<std::string, char>> memoized_;
    void memoize(const char* name, char delim) const;
  };


  // split argument at delim
  inline std::pair<std::string, std::string> split_arg(const char* carg, char delim)
  {
    const char* s = strchr(carg, delim);
    if (nullptr == s)
    {
      return{ "", "" };
    }
    return{{carg, s}, {s + 1}};
  }


  template <typename T>
  inline void convert_arg(std::pair<std::string, std::string> const& arg, T& x)
  {
    std::istringstream iss(arg.second);
    if (!(iss >> x))
    {
      throw parse_error(std::string("invalid value for argument ") + arg.first);
    }
  }


  inline void parse_cmd_flag(const char* name, bool& val, const std::vector<std::string>& argv)
  {
    for (const auto& arg : argv) 
    {
      if (0 == strcmp(arg.c_str(), name)) { val = true; return; }
    }
  }


  template <typename T>
  inline bool parse_optional_arg(const char* name, T& val, const std::vector<std::string>& argv, char delim = '=')
  {
    for (const auto& arg : argv) 
    {
      auto sarg = split_arg(arg.c_str(), delim);
      if (sarg.first == name) { convert_arg(sarg, val); return true; }
    }
    return false;
  }


  template <typename T>
  inline void parse_required_arg(const char* name, T& val, const std::vector<std::string>& argv, char delim = '=')
  {
    for (const auto& arg : argv) 
    {
      auto sarg = split_arg(arg.c_str(), delim);
      if (sarg.first == name) { convert_arg(sarg, val); return; }
    }
    throw parse_error(std::string("missing argument '") + name + '\'');
  }


  inline bool cmd_line_parser::flag(const char* name) const
  {
    memoize(name, 0);
    bool flag = false;
    parse_cmd_flag(name, flag, argv_);
    return flag;
  }


  template <typename T>
  inline bool cmd_line_parser::optional(const char* name, T& val, char delim) const
  {
    memoize(name,delim);
    return parse_optional_arg(name, val, argv_, delim);
  }


  template <typename T>
  inline T cmd_line_parser::optional_val(const char* name, T val, char delim) const
  {
    memoize(name,delim);
    T nVal = val;
    return parse_optional_arg(name, nVal, argv_, delim) ? nVal : val;
  }


  template <typename T>
  inline T cmd_line_parser::required(const char* name, char delim) const
  {
    memoize(name,delim);
    T val;
    parse_required_arg(name, val, argv_, delim);
    return val;
  }


  template <typename C>
  inline bool cmd_line_parser::optional_vec(const char* name, C& val, char delim) const
  {
    parse_vector<typename C::value_type> tmp;
    if (optional(name, tmp, delim)) {
      std::copy(tmp.res_.cbegin(), tmp.res_.cend(), val.begin());
      return true;
    }
    return false;
  }


  inline std::vector<std::string> cmd_line_parser::unrecognized() const
  {
    std::vector<std::string> tmp;
    for (size_t i=1; i<argv_.size(); ++i) 
    {
      bool known = false;
      for (const auto& a : memoized_) 
      {
        auto sarg = split_arg(argv_[i].c_str(), a.second);
        if (sarg.first == a.first)
        {
          known = true;
          break;
        }
      }
      if (!known) tmp.emplace_back(argv_[i]);
    }
    return tmp;
  }


  //! \brief returns command line [1,end]
  inline std::string cmd_line_parser::argv() const
  {
    std::string res;
    for (size_t i=1; i<argv_.size(); ++i)
    {
      res += argv_[i];
      res += ' ';
    }
    return res;
  }


  inline void cmd_line_parser::memoize(const char* name, char delim) const
  {
    memoized_.emplace(name, delim);
  }


}

#endif
