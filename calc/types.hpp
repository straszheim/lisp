#ifndef LISP_TYPES_HPP_INCLUDED
#define LISP_TYPES_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/variant.hpp>
#include <boost/intrusive_ptr.hpp>
#include <string>
#include <vector>

namespace lisp {
  struct symbol : std::string
  {
    symbol(const std::string& s) : std::string(s) { }
    symbol(const char* s, unsigned len) : std::string(s, len) { }
    symbol(const std::vector<char>& s) 
      : std::string(s.data(), s.size()) { }
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
			 cons_ptr> variant;

  struct function 
  { 
    typedef variant result_type;
    typedef boost::function<variant(context_ptr, variant)> bf_t;
    bf_t f;

    std::string name;

    function() { }
    function(bf_t _f) : f(_f) { }

    variant operator()(context_ptr& ctx, variant& cns);
    bool operator!() const { return !f; }
  };

  inline bool operator==(const function& lhs, const function& rhs)
  {
    return &lhs == &rhs;
  }

  struct cons 
  {
    unsigned count;

    variant car, cdr;
    
    cons() : count(0),
	     car(cons_ptr(0)),
	     cdr(cons_ptr(0))
    { }

    cons(const variant& v) : count(0),
			     car(v),
			     cdr(cons_ptr(0))
    { }

    cons(const variant& v, const variant& w) : count(0),
					       car(v),
					       cdr(w)
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

  inline bool is_nil(const variant& v)
  {
    const cons_ptr p = boost::get<cons_ptr>(v);
    return p.get() == 0;
  }

  inline bool is_ptr(const variant& v)
  {
    return boost::get<cons_ptr>(&v);
  }
  
  extern const variant nil;
  extern const variant t;


  //
  //  some syntactic sugar for dealing with variants that are conses
  //
  namespace tag 
  {
    struct car {};
    struct cdr {};
  }

  const static tag::car car = {};
  const static tag::cdr cdr = {};

  inline variant& operator>>(variant& v, tag::car)
  {
    return boost::get<cons_ptr>(v)->car;
  };

  inline variant& operator>>(variant& v, tag::cdr)
  {
    return boost::get<cons_ptr>(v)->cdr;
  };

}

#endif
