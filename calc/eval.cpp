#include "types.hpp"
#include "eval.hpp"

//#define SHOW std::cout << __PRETTY_FUNCTION__ << "\n"
#define SHOW

namespace lisp 
{
  variant eval(context_ptr& ctx, variant& v)
  {
    eval_visitor e(ctx);
    return boost::apply_visitor(e, v);
  }
  

  eval_visitor::eval_visitor(context_ptr _ctx) : ctx(_ctx) { }

  variant eval_visitor::operator()(double d)
  {
    SHOW;
    return d;
  }
    
  variant eval_visitor::operator()(variant v)
  {
    SHOW;
    eval_visitor eprime(ctx);
    return boost::apply_visitor(eprime, v);
  }
    
  variant eval_visitor::operator()(const std::string& s)
  {
    SHOW;
    return s;
  }
    
  variant eval_visitor::operator()(const symbol& s)
  {
    SHOW;
    return s;
  }
    
  //
  //  don't remember why i put this here
  //
  variant eval_visitor::operator()(const function& p)
  {
    SHOW;
    return 1313;
  }

  variant eval_visitor::operator()(const cons_ptr& p)
  {
    symbol sym = boost::get<symbol>(p->car);
    if (sym == "quote")
      return p->cdr;
    function f = ctx->fns[sym];

    return f(ctx, p->cdr);
  }

}
