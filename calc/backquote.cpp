#include "types.hpp"
#include "backquote.hpp"
#include "eval.hpp"
#include "config.hpp"

#include <iostream>


namespace lisp 
{
  variant backquote(context_ptr& ctx, const variant& v)
  {
    backquote_visitor e(ctx);
    return boost::apply_visitor(e, v);
  }
  
  backquote_visitor::backquote_visitor(context_ptr _ctx) : ctx(_ctx) { }

  variant backquote_visitor::operator()(const variant& v)
  {
    SHOW;
    return boost::apply_visitor(*this, v);
  }
    
  variant backquote_visitor::operator()(double d)
  {
    SHOW;
    return d;
  }
    
  variant backquote_visitor::operator()(const std::string& s)
  {
    SHOW;
    return s;
  }
    
  variant backquote_visitor::operator()(const symbol& s)
  {
    return s;
  }
    
  //
  //  don't remember why i put this here
  //
  variant backquote_visitor::operator()(const function& p)
  {
    assert(0);
    return 1313;
  }

  variant backquote_visitor::operator()(const cons_ptr& p)
  {
    SHOW;
    if (p == boost::get<cons_ptr>(nil))
      return p;
    //    ctx->dump(std::cout);
    symbol* sym = boost::get<symbol>(&(p->car));
    if (sym && *sym == "comma")
      {
	return eval(ctx, p->cdr >> car);
      }
    else
      return cons_ptr(new cons(boost::apply_visitor(*this, p->car),
			       boost::apply_visitor(*this, p->cdr)));
  }
}
