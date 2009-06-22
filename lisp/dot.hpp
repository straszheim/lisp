//
// Copyright Troy D. Straszheim 2009
//
// Distributed under the Boost Software License, Version 1.0
// See http://www.boost.org/LICENSE_1.0.txt
//

#ifndef LISP_DOT_HPP_INCLUDED
#define LISP_DOT_HPP_INCLUDED

#include <fstream>
#include <string>

#include "types.hpp"

namespace lisp {

  struct dot
  {
    typedef void* result_type;
    std::ofstream* os;

    dot(std::string prefix, unsigned n);
    ~dot();

    void* operator()(const double &d);
    void* operator()(const std::string& s);
    void* operator()(const symbol& s);
    void* operator()(const cons_ptr& p);
    void* operator()(const function& f);
    void* operator()(const variant& v);
    template <typename T> void* operator()(const special<T>& s) { return 0; }

  };

  void dout(std::string name, const variant& v);

}

#endif
