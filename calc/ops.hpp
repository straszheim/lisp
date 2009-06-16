#ifndef LISP_OPS_HPP_INCLUDED
#define LISP_OPS_HPP_INCLUDED

#define OP_FWD_DECL(T) struct T { variant operator()(context_ptr, variant); }; 

namespace lisp {
  namespace ops {
    OP_FWD_DECL(cons);
    OP_FWD_DECL(divides);
    OP_FWD_DECL(list);

    template <typename Op>
    struct op 
    { 
      const double initial;
      Op op_;

      op(double);
      variant operator()(context_ptr, variant); 
    };
  }
}


#endif
