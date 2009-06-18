#include "types.hpp"
#include "eval.hpp"

#include <iostream>

#define SHOW std::cout << __PRETTY_FUNCTION__ << "\n"
// #define SHOW

namespace lisp 
{
  variant eval(context_ptr& ctx, const variant& v)
  {
    eval_visitor e(ctx);
    return boost::apply_visitor(e, v);
  }
  
  eval_visitor::eval_visitor(context_ptr _ctx) : ctx(_ctx) { }

  variant eval_visitor::operator()(const variant& v)
  {
    SHOW;
    eval_visitor eprime(ctx);
    return boost::apply_visitor(eprime, v);
  }
    
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
    
  //
  //  don't remember why i put this here
  //
  variant eval_visitor::operator()(const function& p)
  {
    assert(0);
    return 1313;
  }

  variant eval_visitor::operator()(const cons_ptr& p)
  {
    SHOW;
    //    ctx->dump(std::cout);
    symbol sym = boost::get<symbol>(p->car);
    function f = ctx->get<function>(sym);

    return f(ctx, p->cdr);
  }

}
