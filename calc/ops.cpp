#include "types.hpp"
#include "ops.hpp"
#include "context.hpp"
#include "eval.hpp"
#include "print.hpp"
#include "dot.hpp"
#include "debug.hpp"

#include <iostream>
#include <vector>

// #define SHOW std::cerr << __PRETTY_FUNCTION__ << "\n"
#define SHOW 

using boost::get;

namespace lisp {
  namespace ops {

    template <typename Op>
    variant op<Op>::operator()(context_ptr c, variant v)
    {
      SHOW;
      double r = initial;

      std::vector<double> vd;
      while(!is_nil(v))
	{
	  variant result = eval(c, v >> car);
	  double d = get<double>(result);
	  r = op_(r, d);
	  v = v >> cdr;
	}
      return r;
    }

    variant divides::operator()(context_ptr c, variant v)
    {
      SHOW;

      cons_ptr l = get<cons_ptr>(v);
      while(l)
	{
	  l->car = eval(c, l->car);
	  l = get<cons_ptr>(l->cdr);
	}

      double r = get<double>(l->car);
      l = get<cons_ptr>(l->cdr);

      if (! l)
	return 1.0 / r;

      l = get<cons_ptr>(v);
      while(l)
	{
	  double n = get<double>(l->car);
	  r /= n;
	  l = get<cons_ptr>(l->cdr);
	}
      return r;
    }

    template <typename Op>
    op<Op>::op(double _initial) : initial(_initial) { }

    template struct op<std::plus<double> >;
    template struct op<std::minus<double> >;
    template struct op<std::multiplies<double> >;

    variant cons::operator()(context_ptr c, variant v)
    {
      SHOW;
      cons_ptr nc = new lisp::cons;
      nc->car = v >> car;
      nc->cdr = v >> cdr >> car;
      return nc;
    }

    variant quote::operator()(context_ptr c, variant v)
    {
      SHOW;
      if (! is_nil(v >> cdr))
	throw std::runtime_error("wrong number of args to quote");
      return v >> car;
    }

    variant list::operator()(context_ptr c, variant v)
    {
      SHOW;
      cons_ptr head = new lisp::cons, tail = head;

      while(true)
	{
	  tail->car = eval(c, v >> car);
	  if (! is_nil(v >> cdr))
	    {
	      v = v >> cdr;
	      cons_ptr tmp = new lisp::cons;
	      tail->cdr = tmp;
	      tail = tmp;
	    }
	  else
	    break;
	}

      return head;
    }

    struct equal_visitor
    : public boost::static_visitor<bool>
    {
      template <typename T, typename U>
      bool operator()( const T &, const U & ) const
      {
        return false; // cannot compare different types
      }

      template <typename T>
      bool operator()( const T & lhs, const T & rhs ) const
      {
        return lhs == rhs;
      }

      template <typename T>
      bool operator()( const lisp::function & lhs, const lisp::function & rhs ) const
      {
        return &lhs == &rhs;
      }

      bool operator()( const lisp::cons_ptr& lhs, const lisp::cons_ptr & rhs ) const
      {
	if (is_nil(lhs) && is_nil(rhs))
	  return true;
	if (is_nil(lhs) || is_nil(rhs))
	  return false;

        return boost::apply_visitor(*this, lhs->car, rhs->car)
	  && boost::apply_visitor(*this, lhs->cdr, rhs->cdr);
      }
    };

    //
    // this is the one where they're equal if their printed representations
    // are the same
    //
    variant equal::operator()(context_ptr ctx, variant v)
    {
      SHOW;

      variant lhs_evalled = eval(ctx, v >> car);
      variant rhs_evalled = eval(ctx, v >> cdr >> car);

      return boost::apply_visitor(equal_visitor(), lhs_evalled, rhs_evalled) ? t : nil;
    }

    variant if_clause::operator()(context_ptr ctx, variant v)
    {
      SHOW;
      variant cond_evalled = eval(ctx, v >> car);

      if (cond_evalled == t)
	return eval(ctx, v >> cdr >> car);
      else
	return eval(ctx, v >> cdr >> cdr >> car);
    }

    variant defvar::operator()(context_ptr ctx, variant v)
    {
      SHOW;
      symbol s = get<symbol>(v >> car);
      variant result = eval(ctx, v >> cdr >> car);
      global->put(s, result);
      return s;
    }

    variant setf::operator()(context_ptr ctx, variant v)
    {
      SHOW;
      symbol s = get<symbol>(v >> car);
      variant result = eval(ctx, v >> cdr >> car);
      try {
	variant& destination = ctx->get<variant>(s);
	destination = result;
      } catch (const std::exception&) {
	ctx->put(s, result);
      }
      return result;
    }

    variant print::operator()(context_ptr ctx, variant v)
    {
      SHOW;
      variant evalled = eval(ctx, v >> car);
      cons_print cp(std::cout);
      cp(evalled);
      std::cout << "\n";
      return evalled;
    }

    variant evaluate::operator()(context_ptr ctx, variant v)
    {
      SHOW;
      variant evalled = eval(ctx, v >> car);
      // now we've got what to evaulate, e.g. fetch fncall from variable
      evalled = eval(ctx, evalled);
      return evalled;
    }

    variant progn::operator()(context_ptr ctx, variant v)
    {
      variant last;
      while (! is_nil(v))
	{
	  last = eval(ctx, v >> car);
	  v = v >> cdr;
	}
      return last;
    }

    template <typename Signature>
    struct dispatch
    {
      variant code;
      std::vector<symbol> args;

      dispatch(variant _code) : code(_code) 
      { 
	dout("codeis", code);
      }

      variant operator()(context_ptr c, const variant v)
      {
	cons_ptr l = get<cons_ptr>(v);
	//	std::cout << "old scope:";
	//	c->dump(std::cout);
	context_ptr scope = c->scope();
	//	std::cout << "new scope:";
	//	scope->dump(std::cout);
	//	std::cout << "adding " << args.size() << " args\n";
	for(unsigned u = 0; u<args.size(); u++)
	  {
	    variant evalled = eval(c, l->car);
	    //	    std::cout << args[u] << "\n";
	    scope->put(args[u], evalled);
	    l = get<cons_ptr>(l->cdr);
	  }
	//	scope->dump(std::cout);
	
	cons_ptr progn(new lisp::cons);
	progn->car = symbol("progn");
	progn->cdr = code;
	variant v2(progn);
	//	dout("readytorun", v2);
	//	std::cout << "READY TO RUN:";
	//debug(v2);
	variant result = eval(scope, v2);
	//	std::cout << "\nNOW IT IS:";
	//	dout("afterrun", v2);
	//	debug(v2);
	return result;
      }
    };

    variant defun::operator()(context_ptr c, variant v)
    {
      SHOW;

      symbol s = get<symbol>(v >> car);
      variant l = v >> cdr >> car;
      std::vector<symbol> args;
      while (! is_nil(l))
	{
	  symbol s = get<symbol>(l >> car);
	  args.push_back(s);
	  l = l >> cdr;
	}
      dispatch<void> dispatcher(v >> cdr >> cdr);
      dispatcher.args = args;
      c->put(s, function(dispatcher));

      return s;
    }
  }
}
