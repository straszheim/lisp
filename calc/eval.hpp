#ifndef LISP_EVAL_HPP_INCLUDED
#define LISP_EVAL_HPP_INCLUDED

#include "types.hpp"
#include "context.hpp"

namespace lisp 
{
  struct eval
  {
    typedef variant result_type;

    context_ptr ctx;
    eval(context_ptr _ctx);

    variant operator()(double d);
    
    variant operator()(variant v);
    
    variant operator()(const std::string& s);
    
    variant operator()(const symbol& s);
    
    variant operator()(const function& p);

    variant operator()(const cons_ptr& p);
  };
}

#endif
