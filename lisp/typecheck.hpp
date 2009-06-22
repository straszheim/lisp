//
// Copyright Troy D. Straszheim 2009
//
// Distributed under the Boost Software License, Version 1.0
// See http://www.boost.org/LICENSE_1.0.txt
//

#ifndef LISP_TYPECHECK_HPP_INCLUDED
#define LISP_TYPECHECK_HPP_INCLUDED

#include "types.hpp"

namespace lisp 
{
  template <typename Sig>
  struct typecheck 
  {
    typedef typename ft::parameter_types<Function> params_t;
    typedef typename ft::return_type<Function> op_t;

    struct checker
    {
      cons_ptr& cp;

      checker

      template <typename T>
      void operator()(T) const
      { 
	std::cout << __PRETTY_FUNCTION__ << "\n"; 
      }
    };

    checker check;

    typecheck(cons_ptr) 
    { 
      std::cout << __PRETTY_FUNCTION__ << "\n";
      fusion::for_each(params_t(), check);
    }

    variant operator()(context_ptr& ctx, variant& v)
    {
      return v;
    }

  };

}







#endif


#if 0
struct typechecked<eql(variant, variant)> 
{ 
  // ... 
};


#endif
