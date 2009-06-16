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
    std::map<std::string, variant> table;
    std::map<std::string, function> fns;

    context_ptr next;

    context_ptr scope();
  };

  extern context_ptr global;
}

#endif

