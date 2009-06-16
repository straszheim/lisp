#include "types.hpp"
#include "ops.hpp"
#include "context.hpp"
#include "eval.hpp"

namespace lisp {
  namespace ops {

    template <typename Op>
    variant op<Op>::operator()(context_ptr c, variant v)
    {
      cons_ptr l = boost::get<cons_ptr>(v);

      double r = initial;
      eval e(c);
      while(l)
	{
	  variant evalled = boost::apply_visitor(e, l->car);
	  double n = boost::get<double>(evalled);
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


    variant quote::operator()(context_ptr c, variant l)
    {
      return l; // no-op.  the 'quote' has been removed from the list already.
    }

    variant cons::operator()(context_ptr c, variant v)
    {
      cons_ptr a1 = boost::get<cons_ptr>(v);
      cons_ptr a2 = boost::get<cons_ptr>(a1->cdr);
      cons_ptr nc = new lisp::cons;
      nc->car = a1->car;
      nc->cdr = a2->car;
      return nc;
    }

    variant list::operator()(context_ptr c, variant v)
    {
      /*
      cons_ptr a1 = boost::get<cons_ptr>(v);
      cons_ptr a2 = boost::get<cons_ptr>(a1->cdr);
      cons_ptr nc = new lisp::cons;
      nc->car = a1->car;
      nc->cdr = a2->car;
      return nc;
      */
      assert(0);
    }

    variant divides::operator()(context_ptr c, variant v)
    {
      cons_ptr l = boost::get<cons_ptr>(v);

      eval e(c);
      variant evalled = boost::apply_visitor(e, l->car);
      double r = boost::get<double>(evalled);
      l = boost::get<cons_ptr>(l->cdr);

      if (! l)
	return 1.0 / r;

      while(l)
	{
	  variant evalled = boost::apply_visitor(e, l->car);
	  double n = boost::get<double>(evalled);
	  r /= n;
	  l = boost::get<cons_ptr>(l->cdr);
	}
      return r;
    }

  }
}
