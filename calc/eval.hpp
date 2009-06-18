#ifndef LISP_EVAL_HPP_INCLUDED
#define LISP_EVAL_HPP_INCLUDED

#include "types.hpp"
#include "context.hpp"

namespace lisp 
{
  struct eval_visitor
  {
    typedef variant result_type;

    context_ptr ctx;
    eval_visitor(context_ptr _ctx);

    variant operator()(double d);
    
    variant operator()(const variant& v);
    
    variant operator()(const std::string& s);
    
    variant operator()(const symbol& s);
    
    variant operator()(const function& p);

    variant operator()(const cons_ptr& p);
  };

  variant eval(context_ptr& ctx, const variant& v);

}

#endif
