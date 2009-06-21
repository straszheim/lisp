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
    if (is_nil(p))
      return p;

    variant car_result = visit(p->car);
    variant cdr_result = visit(p->cdr);

    if (boost::get<special<comma_at_> >(&(p->car)))
      {
	last(car_result)->cdr = cdr_result;
	return car_result;
      }
    else
      return cons_ptr(new cons(car_result, cdr_result));
  }

  variant backquote_visitor::operator()(const special<backquoted_>& s)
  {
    return visit(s.v);
  }

  variant backquote_visitor::operator()(const special<quoted_>& s)
  {
    return s.v;
  }

  variant backquote_visitor::operator()(const special<comma_at_>& s)
  {
    variant result = eval(ctx, s.v);
    return result;
  }

  variant backquote_visitor::operator()(const special<comma_>& s)
  {
    return eval(ctx, s.v);
  }


}
