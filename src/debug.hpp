//
// Copyright Troy D. Straszheim 2009
//
// Distributed under the Boost Software License, Version 1.0
// See http://www.boost.org/LICENSE_1.0.txt
//

#ifndef LISP_DEBUG_HPP_INCLUDED
#define LISP_DEBUG_HPP_INCLUDED

#include <iostream>

namespace lisp {

  struct cons_debug
  {
    typedef void result_type;
    
    std::ostream& os;

    cons_debug(std::ostream& _os);

    void operator()(double d) const;
    void operator()(const std::string& s) const;
    void operator()(const symbol& s) const;
    void operator()(const cons_ptr p) const;
    void operator()(const function f) const;
    void operator()(const variant v) const;
    void operator()(const special<backquoted_>& s) const;
    void operator()(const special<quoted_>& s) const;
    void operator()(const special<comma_at_>& s) const;
    void operator()(const special<comma_>& s) const;
  };
  
  std::ostream& operator<<(std::ostream& os, const cons_ptr& cp);
  std::ostream& operator<<(std::ostream& os, const variant& v);

  void debug(const variant& v, std::ostream& os = std::cout);

}

#endif
