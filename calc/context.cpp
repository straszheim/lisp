#include "types.hpp"
#include "context.hpp"

namespace lisp 
{
  context_ptr context::scope()
  {
    context_ptr ctx(new context);
    ctx->next = shared_from_this();
    return ctx;
  }

  context_ptr global(new context);
}

