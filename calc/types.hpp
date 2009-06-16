#ifndef LISP_TYPES_HPP_INCLUDED
#define LISP_TYPES_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/variant.hpp>
#include <boost/intrusive_ptr.hpp>
#include <string>

namespace lisp {
  struct symbol : std::string
  {
    symbol(const std::string& s) : std::string(s) { }
  };

  struct cons;
  typedef boost::intrusive_ptr<cons> cons_ptr;

  struct context;
  typedef boost::shared_ptr<context> context_ptr;

  struct function;

  typedef boost::variant<double,
			 std::string,
			 symbol,
			 boost::recursive_wrapper<function>,
			 cons_ptr>
  variant;

  struct function 
  { 
    typedef variant result_type;
    typedef boost::function<variant(context_ptr, variant)> bf_t;
    bf_t f;

    std::string name;

    function() { }
    function(bf_t _f) : f(_f) { }

    variant operator()(context_ptr& ctx, variant& cns);
    //    {
    //      return f(ctx, cns);
    //    }
  };

  struct cons 
  {
    unsigned count;

    variant car, cdr;
    
    cons() : count(0),
	     car(cons_ptr(0)),
	     cdr(cons_ptr(0))
    { }

    template <typename T>
    cons(T t) : count(0),
		car(t),
		cdr(cons_ptr(0))
    { }

    ~cons() { }
    
  };

  inline void intrusive_ptr_add_ref(cons* c)
  {
    //    std::cout << "inc cons @ " << c << "\n";
    c->count++;
  }
  
  inline void intrusive_ptr_release(cons* c)
  {
    //    std::cout << "dec cons @ " << c << "\n";
    c->count--;
    if (c->count == 0)
      delete c;
  }
}

#endif
