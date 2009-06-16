#include <iostream>

#include "types.hpp"
#include "ops.hpp"
#include "context.hpp"
#include "eval.hpp"
#include "print.hpp"

// #define SHOW std::cerr << __PRETTY_FUNCTION__ << "\n"
#define SHOW 

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

    variant divides::operator()(context_ptr c, variant v)
    {
      SHOW;
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

    /*
    variant defun::operator()(context_ptr c, variant v)
    {
      
    }
    */
  }
}
