#include "types.hpp"
#include "eval.hpp"

//#define SHOW std::cout << __PRETTY_FUNCTION__ << "\n"
#define SHOW

namespace lisp 
{
  eval::eval(context_ptr _ctx) : ctx(_ctx) { }

  variant eval::operator()(double d)
  {
    SHOW;
    return d;
  }
    
  variant eval::operator()(variant v)
  {
    SHOW;
    eval eprime(ctx);
    return boost::apply_visitor(eprime, v);
  }
    
  variant eval::operator()(const std::string& s)
  {
    SHOW;
    return s;
  }
    
  variant eval::operator()(const symbol& s)
  {
    SHOW;
    return s;
  }
    
  variant eval::operator()(const function& p)
  {
    SHOW;
    /*
      if (!p) 
      return;
      if (boost::get<cons_ptr>(&p->car))
      {
      os << "(";
      }
      boost::apply_visitor(*this, p->car);
      if (boost::get<cons_ptr>(&p->car))
      os << ")";
      if (!cons::nil(p->cdr))
      os << " ";
      boost::apply_visitor(*this, p->cdr);
    */
    return 1313;
  }

  variant eval::operator()(const cons_ptr& p)
  {
    symbol sym = boost::get<symbol>(p->car);
    function f = ctx->fns[sym];
    return f(ctx, p->cdr);
  }

}
