#ifndef LISP_TYPES_HPP_INCLUDED
#define LISP_TYPES_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/variant.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/serialization/strong_typedef.hpp>
#include <string>
#include <vector>

namespace lisp 
{
  struct symbol : std::string
  {
    symbol(const std::string& s) : std::string(s) { }
    symbol(const char* s, unsigned len) : std::string(s, len) { }
    symbol(const std::vector<char>& s) 
      : std::string(s.data(), s.size()) 
    { }
  };

  struct cons;
  typedef boost::intrusive_ptr<cons> cons_ptr;

  struct context;
  typedef boost::shared_ptr<context> context_ptr;

  struct function;
  struct quoted_ {};
  struct backquoted_ {};
  struct comma_ {};
  struct comma_at_ {};

  template <typename T> struct special;

  typedef boost::variant<double,
			 std::string,
			 symbol,
			 boost::recursive_wrapper<function>,
			 cons_ptr,
			 boost::recursive_wrapper<special<quoted_> >, 
			 boost::recursive_wrapper<special<backquoted_> >, 
			 boost::recursive_wrapper<special<comma_> >,
			 boost::recursive_wrapper<special<comma_at_> >
			 > variant;

  extern const variant nil;
}

namespace lisp 
{   
  template <typename T>
  struct special 
  {
    special(const variant& _v) : v(_v) { }
    variant v;
  };

  template <typename T>
  inline bool operator==(const special<T>& lhs, const special<T>& rhs)
  {
    return lhs.v == rhs.v;
  }

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

  extern const variant nil;
  extern const variant t;

  inline bool is_nil(const variant& v)
  {
    const cons_ptr p = boost::get<cons_ptr>(v);
    return p.get() == 0;
  }

  inline bool is_ptr(const variant& v)
  {
    return v.which() == 4;
  }
  
  inline cons_ptr last(const variant& v)
  {
    cons_ptr tmp = boost::get<cons_ptr>(v);
    while (!is_nil(tmp->cdr))
      {
	tmp = boost::get<cons_ptr>(tmp->cdr);
      }
    return tmp;
  }


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
