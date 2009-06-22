//
// Copyright Troy D. Straszheim 2009
//
// Distributed under the Boost Software License, Version 1.0
// See http://www.boost.org/LICENSE_1.0.txt
//

#ifndef LISP_EVAL_HPP_INCLUDED
#define LISP_EVAL_HPP_INCLUDED

#include "types.hpp"
#include "context.hpp"

namespace lisp 
{
  struct eval_visitor
  {
    typedef variant result_type;

    eval_visitor(context_ptr _ctx);

    variant operator()(double d);
    variant operator()(const std::string& s);
    variant operator()(const symbol& s);
    variant operator()(const function& p);
    variant operator()(const cons_ptr& p);
    variant operator()(const special<backquoted_>& s);
    variant operator()(const special<quoted_>& s);
    variant operator()(const special<comma_at_>& s);
    variant operator()(const special<comma_>& s);

  private:

    context_ptr ctx;

    template <typename T>
    variant visit(T const& t);

  };

  variant eval(context_ptr& ctx, const variant& v);

}

#endif
