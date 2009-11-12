//
// Copyright Troy D. Straszheim 2009
//
// Distributed under the Boost Software License, Version 1.0
// See http://www.boost.org/LICENSE_1.0.txt
//

#ifndef LISP_CONTEXT_HPP_INCLUDED
#define LISP_CONTEXT_HPP_INCLUDED

#include "types.hpp"
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>

namespace lisp {
  
  struct context;
  typedef boost::shared_ptr<context> context_ptr;

  struct context : boost::enable_shared_from_this<context>
  {
    template <typename T> 
    T& get(const std::string& name);

    void put(const std::string& name, variant what);

    context_ptr scope();

    void dump(std::ostream&) const;

  private:

    std::map<std::string, variant> m_;

    context_ptr next_;
    template <typename T> T& convert(variant&);


  };

  extern context_ptr global;
}

#endif

