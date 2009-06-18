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
  };
  
  std::ostream& operator<<(std::ostream& os, const cons_ptr& cp);
  std::ostream& operator<<(std::ostream& os, const variant& v);

  void debug(const variant& v, std::ostream& os = std::cout);

}

#endif
