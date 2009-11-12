//
// Copyright Troy D. Straszheim 2009
//
// Distributed under the Boost Software License, Version 1.0
// See http://www.boost.org/LICENSE_1.0.txt
//

#include "config.hpp"
#include "types.hpp"
#include "eval.hpp"
#include "print.hpp"
#include "backquote.hpp"

#include <iostream>

namespace lisp 
{
  variant eval(context_ptr& ctx, const variant& v)
  {
    eval_visitor e(ctx);
    return boost::apply_visitor(e, v);
  }

  template <typename T>
  variant eval_visitor::visit(T const& t)
  {
    return boost::apply_visitor(*this, t);
  }
  
  eval_visitor::eval_visitor(context_ptr _ctx) : ctx(_ctx) { }

  variant eval_visitor::operator()(double d)
  {
    SHOW;
    return d;
  }
    
  variant eval_visitor::operator()(const std::string& s)
  {
    SHOW;
    return s;
  }
    
  variant eval_visitor::operator()(const symbol& s)
  {
    return ctx->get<variant>(s);
  }

  variant eval_visitor::operator()(const function& p)
  {
    return p;
  }

  variant eval_visitor::operator()(const cons_ptr& p)
  {
    SHOW;
    if (p == boost::get<cons_ptr>(nil))
      return p;
    // ctx->dump(std::cout);
    variant v = visit(p->car);
    function f = boost::get<function>(v);

    return f(ctx, p->cdr);
  }

  variant eval_visitor::operator()(const special<backquoted_>& s)
  {
    return lisp::backquote(ctx, s.v);
  }

  variant eval_visitor::operator()(const special<quoted_>& s)
  {
    return s.v;
  }

  variant eval_visitor::operator()(const special<comma_at_>& s)
  {
    return s.v;
  }

  variant eval_visitor::operator()(const special<comma_>& s)
  {
    return s.v;
  }


}
