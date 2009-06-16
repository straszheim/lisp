#include "types.hpp"
#include "ops.hpp"
#include "context.hpp"
#include "eval.hpp"

namespace lisp {
  namespace ops {
    /*
    */

    template <typename Op>
    variant op<Op>::operator()(context_ptr c, variant v)
    {
      cons_ptr l = boost::get<cons_ptr>(v);
      while(l)
	{
	  l->car = eval(c, l->car);
	  l = boost::get<cons_ptr>(l->cdr);
	}

      double r = initial;

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
      cons_ptr a1 = boost::get<cons_ptr>(v);
      cons_ptr a2 = boost::get<cons_ptr>(a1->cdr);
      cons_ptr nc = new lisp::cons;
      nc->car = a1->car;
      nc->cdr = a2->car;
      return nc;
    }

    variant quote::operator()(context_ptr c, variant v)
    {
      return v;
    }

    variant list::operator()(context_ptr c, variant v)
    {
      cons_ptr l = boost::get<cons_ptr>(v);
      while(l) { 
	l->car = eval(c, l->car);
	l = boost::get<cons_ptr>(l->cdr);
      }
      return v;
    }

    variant defvar::operator()(context_ptr c, variant v)
    {
      return v;
    }

    variant divides::operator()(context_ptr c, variant v)
    {
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

      while(l)
	{
	  double n = boost::get<double>(l->car);
	  r /= n;
	  l = boost::get<cons_ptr>(l->cdr);
	}
      return r;
    }

  }
}
