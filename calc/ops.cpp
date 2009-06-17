#include "types.hpp"
#include "ops.hpp"
#include "context.hpp"
#include "eval.hpp"
#include "print.hpp"
#include "dot.hpp"

#include <iostream>
#include <vector>

#define SHOW std::cerr << __PRETTY_FUNCTION__ << "\n"
// #define SHOW 

namespace lisp {
  namespace ops {

    template <typename Op>
    variant op<Op>::operator()(context_ptr c, variant v)
    {
      SHOW;
      cons_ptr l = boost::get<cons_ptr>(v);
      while(l)
	{
	  l->car = eval(c, l->car);
	  l = boost::get<cons_ptr>(l->cdr);
	}

      double r = initial;

      l = boost::get<cons_ptr>(v);
      while(l)
	{
	  double n = boost::get<double>(l->car);
	  r = op_(r, n);
	  l = boost::get<cons_ptr>(l->cdr);
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
      cons_ptr a1 = boost::get<cons_ptr>(v);
      cons_ptr a2 = boost::get<cons_ptr>(a1->cdr);
      cons_ptr nc = new lisp::cons;
      nc->car = a1->car;
      nc->cdr = a2->car;
      return nc;
    }

    variant quote::operator()(context_ptr c, variant v)
    {
      SHOW;
      return v;
    }

    variant list::operator()(context_ptr c, variant v)
    {
      SHOW;
      cons_ptr l = boost::get<cons_ptr>(v);
      while(l) { 
	l->car = eval(c, l->car);
	l = boost::get<cons_ptr>(l->cdr);
      }
      return v;
    }

    variant defvar::operator()(context_ptr ctx, variant v)
    {
      SHOW;
      cons_ptr c = boost::get<cons_ptr>(v);
      symbol s = boost::get<symbol>(c->car);
      cons_ptr next = boost::get<cons_ptr>(c->cdr);
      variant result = eval(ctx, next->car);
      global->table[s] = result;
      return s;
    }

    variant print::operator()(context_ptr ctx, variant v)
    {
      SHOW;
      cons_ptr arg = boost::get<cons_ptr>(v);
      variant evalled = eval(ctx, arg->car);
      cons_print cp(std::cout);
      cp(evalled);
      std::cout << "\n";
      return evalled;
    }

    variant evaluate::operator()(context_ptr ctx, variant v)
    {
      SHOW;
      cons_ptr arg = boost::get<cons_ptr>(v);
      variant evalled = eval(ctx, arg->car);
      // now we've got what to evaulate, ie fetch fncall from variable
      evalled = eval(ctx, evalled);
      return evalled;
    }

    variant progn::operator()(context_ptr ctx, variant v)
    {
      cons_ptr l = boost::get<cons_ptr>(v);
      variant last;
      while (l)
	{
	  last = eval(ctx, l->car);
	  l = boost::get<cons_ptr>(l->cdr);
	}
      return last;
    }

    variant divides::operator()(context_ptr c, variant v)
    {
      SHOW;

      // the 
      cons_ptr l = boost::get<cons_ptr>(v);
      while(l)
	{
	  l->car = eval(c, l->car);
	  l = boost::get<cons_ptr>(l->cdr);
	}

      double r = boost::get<double>(l->car);
      l = boost::get<cons_ptr>(l->cdr);

      if (! l)
	return 1.0 / r;

      l = boost::get<cons_ptr>(v);
      while(l)
	{
	  double n = boost::get<double>(l->car);
	  r /= n;
	  l = boost::get<cons_ptr>(l->cdr);
	}
      return r;
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

      variant operator()(context_ptr c, variant v)
      {
	cons_ptr l = boost::get<cons_ptr>(v);
	std::cout << "old scope:";
	c->dump(std::cout);
	context_ptr scope = c->scope();
	std::cout << "new scope:";
	scope->dump(std::cout);
	std::cout << "adding " << args.size() << " args\n";
	for(unsigned u = 0; u<args.size(); u++)
	  {
	    variant evalled = eval(c, l->car);
	    std::cout << args[u] << "\n";
	    scope->table[args[u]] = evalled;
	    l = boost::get<cons_ptr>(l->cdr);
	  }
	scope->dump(std::cout);
	
	cons_ptr progn(new lisp::cons);
	progn->car = symbol("progn");
	progn->cdr = code;
	variant v2(progn);
	dot d("readytoeval", 0);
	d(v2);
	return eval(scope, v2);
      }
    };

    variant defun::operator()(context_ptr c, variant v)
    {
      SHOW;
      c->dump(std::cout);
      cons_ptr top = boost::get<cons_ptr>(v);
      symbol s = boost::get<symbol>(top->car);
      cons_ptr arglist = boost::get<cons_ptr>(top->cdr);
      cons_ptr l = boost::get<cons_ptr>(arglist->car);
      std::vector<symbol> args;
      while (l)
	{
	  symbol s = boost::get<symbol>(l->car);
	  args.push_back(s);
	  std::cout << "ARG: " << s << "\n";
	  l = boost::get<cons_ptr>(l->cdr);
	}
      dispatch<void> dispatcher(arglist->cdr);
      dispatcher.args = args;
      c->fns[s] = function(dispatcher);

      return s;
    }
  }
}
