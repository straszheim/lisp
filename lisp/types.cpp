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
