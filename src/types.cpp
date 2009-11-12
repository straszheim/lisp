//
// Copyright Troy D. Straszheim 2009
//
// Distributed under the Boost Software License, Version 1.0
// See http://www.boost.org/LICENSE_1.0.txt
//

#include "types.hpp"
#include "context.hpp"

namespace lisp
{
  variant function::operator()(context_ptr& ctx, variant& cns)
  {
    return f(ctx, cns);
  }

  const variant nil(cons_ptr(0));
  const variant t(symbol("t"));
}
