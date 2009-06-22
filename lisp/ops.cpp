//
// Copyright Troy D. Straszheim 2009
//
// Distributed under the Boost Software License, Version 1.0
// See http://www.boost.org/LICENSE_1.0.txt
//

//
// Copyright Troy D. Straszheim 2009
//
// Distributed under the Boost Software License, Version 1.0
// See http://www.boost.org/LICENSE_1.0.txt
//

#include "config.hpp"
#include "types.hpp"
#include "ops.hpp"
#include "context.hpp"
#include "eval.hpp"
#include "print.hpp"
#include "dot.hpp"
#include "debug.hpp"
#include "backquote.hpp"

#include <iostream>
#include <vector>

using boost::get;

namespace lisp {
  namespace ops {

    template <typename Op>
    variant 
    op<Op>::operator()(context_ptr c, variant v)
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

      std::vector<variant> args;
      while(!is_nil(v))
	{
	  args.push_back(eval(c, v >> car));
	  v = v >> cdr;
	}

      double d = get<double>(args[0]);
      if (args.size() == 1)
	return 1.0 / d;
      for (unsigned i=1; i<args.size(); i++)
	d /= get<double>(args[i]);

      return d;
    }

    variant minus::operator()(context_ptr c, variant v)
    {
      SHOW;

      std::vector<variant> args;
      while(!is_nil(v))
	{
	  args.push_back(eval(c, v >> car));
	  v = v >> cdr;
	}

      double d = get<double>(args[0]);
      if (args.size() == 1)
	return -d;
      for (unsigned i=1; i<args.size(); i++)
	d -= get<double>(args[i]);

      return d;
    }

    template <typename Op>
    op<Op>::op(double _initial) : initial(_initial) { }

    template struct op<std::plus<double> >;
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

    variant backquote::operator()(context_ptr c, variant v)
    {
      variant result = lisp::backquote(c, v);
      return result >> car;
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
      lisp::print(std::cout, evalled);
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
      SHOW;
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
	SHOW;
	cons_ptr l = get<cons_ptr>(v);
	context_ptr scope = c->scope();
	for(unsigned u = 0; u<args.size(); u++)
	  {
	    variant evalled = eval(c, l->car);
	    scope->put(args[u], evalled);
	    l = get<cons_ptr>(l->cdr);
	  }
	
	cons_ptr progn(new lisp::cons);
	progn->car = symbol("progn");
	progn->cdr = code;
	variant v2(progn);
	variant result = eval(scope, v2);
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

    variant lambda::operator()(context_ptr c, variant v)
    {
      SHOW;

      variant l = v >> car;
      std::vector<symbol> args;
      while (! is_nil(l))
	{
	  symbol s = get<symbol>(l >> car);
	  args.push_back(s);
	  l = l >> cdr;
	}
      dispatch<void> dispatcher(v >> cdr);
      dispatcher.args = args;
      return function(dispatcher);
    }

    struct reexec 
    {
      function f;

      variant operator()(context_ptr c, variant v)
      {
	return f(c, v);
      }
    };


    struct macroexpand_dispatch
    {
      variant code;
      std::vector<symbol> args;

      variant operator()(context_ptr c, variant v)
      {
	SHOW;	
	cons_ptr l = get<cons_ptr>(v);
	context_ptr scope = c->scope();
	for(unsigned u = 0; u<args.size(); u++)
	  {
	    scope->put(args[u], l->car);
	    l = get<cons_ptr>(l->cdr);
	  }
	
	cons_ptr progn(new lisp::cons);
	progn->car = symbol("progn");
	progn->cdr = code;
	variant v2(progn);
	variant result = eval(scope, v2);
	return result;
      }
    };

    struct macroexec_dispatch
    {
      variant code;
      std::vector<symbol> args;

      variant operator()(context_ptr c, variant v)
      {
	SHOW;	
	cons_ptr l = get<cons_ptr>(v);
	context_ptr scope = c->scope();
	for(unsigned u = 0; u<args.size(); u++)
	  {
	    scope->put(args[u], l->car);
	    l = get<cons_ptr>(l->cdr);
	  }
	
	cons_ptr progn(new lisp::cons);
	progn->car = symbol("progn");
	progn->cdr = code;
	variant v2(progn);
	variant yay = eval(scope, v2);
	variant result = eval(c, yay);
	return result;
      }
    };

    variant defmacro::operator()(context_ptr c, variant v)
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
      macroexec_dispatch dispatcher;
      dispatcher.code = v >> cdr >> cdr;
      dispatcher.args = args;
      c->put(s, function(dispatcher));

      return s;
    }
  }
}
