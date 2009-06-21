#ifndef LISP_BACKQUOTE_HPP_INCLUDED
#define LISP_BACKQUOTE_HPP_INCLUDED

#include "types.hpp"
#include "context.hpp"

namespace lisp 
{
  struct backquote_visitor
  {
    typedef variant result_type;

    context_ptr ctx;
    backquote_visitor(context_ptr _ctx);

    variant operator()(double d);
    
    variant operator()(const variant& v);
    
    variant operator()(const std::string& s);
    
    variant operator()(const symbol& s);
    
    variant operator()(const function& p);

    variant operator()(const cons_ptr& p);
  };

  variant backquote(context_ptr& ctx, const variant& v);

}

#endif
