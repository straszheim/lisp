#include <boost/intrusive_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/home/phoenix/core/value.hpp>
#include <boost/function.hpp>
#include <boost/format.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include "types.hpp"
#include "ops.hpp"
#include "context.hpp"
#include "eval.hpp"
#include "debug.hpp"
#include "print.hpp"
#include "dot.hpp"
#include "grammar.hpp"

using namespace boost::spirit;
using namespace boost::spirit::qi;
using namespace boost::spirit::ascii;

namespace lisp
{
  namespace phoenix = boost::phoenix;
  namespace ascii = boost::spirit::ascii;

  struct process
  {
    template <typename T = void, typename U = void, typename V = void>
    struct result
    {
      typedef variant type;
    };

    variant operator()(double d) const
    {
      return d;
    }
    
    variant operator()(boost::iterator_range<std::string::const_iterator> r) const
    {
      return symbol(std::string(r.begin(), r.end()));
    }

    template <typename T>
    variant operator()(const T& s) const
    {
      assert(0);
      // something fell through
      return std::string("oh noes");
    }

    template <typename T, typename U>
    variant operator()(const T& s, const U& t) const
    {
      assert(0);
      // something fell through
      return std::string("oh noes");
    }

    variant operator()(const std::vector<char>& v) const
    {
      return std::string(v.data(), v.data() + v.size());
    }

    variant operator()(boost::iterator_range<std::string::const_iterator> r, double) const
    {
      return std::string(r.begin(), r.end());
    }

    variant operator()(const std::vector<variant>& v) const
    {
      cons_ptr head = new cons;
      cons_ptr tmp = head;
      // chain them together
      if (v.size() == 0)
	return head;

      tmp->car = v[0];
      for (unsigned i=1; i<v.size(); i++)
	{
	  cons_ptr tail = new cons;
	  tmp->cdr = tail;
	  tmp = tail;
	  tmp->car = v[i];
	}
      return head;
    }

  };

  namespace {
    using boost::phoenix::function;
    function<lisp::process> p;
  }


  // cons
  struct make_cons_actor
  {
    template <typename T0 = void, typename T1 = void>
    struct result {
      typedef variant type;
    };

    variant operator()(const variant& car, const variant& cdr) const
    {
      //std::cout << __PRETTY_FUNCTION__ << "\n";
      return cons_ptr(new cons(car, cdr));
    }
  };

  namespace 
  {
    using boost::phoenix::function;
    function<make_cons_actor> make_cons;
  }

  

  //
  //  handles sugary constructs like comma, comma_at, backtick, quote
  //
  struct sugar_cons 
  {
    std::string name;

    template <typename T=void, typename U=void, typename V=void>
    struct result
    {
      typedef variant type;
    };

    sugar_cons(const std::string& _name) : name(_name) { }
    sugar_cons(const sugar_cons& rhs) : name(rhs.name) { }

    variant operator()(const variant& v) const
    {
      cons_ptr tail = new cons(v);
      cons_ptr head = new cons(symbol(name), tail);
      return head;
    }
  };

  namespace {
    using boost::phoenix::function;
    function<sugar_cons> quote(sugar_cons("quote"));
    function<sugar_cons> backtick(sugar_cons("backtick"));
    function<sugar_cons> comma_at(sugar_cons("comma_at"));
    function<sugar_cons> comma(sugar_cons("comma"));
  }

  template <typename Iterator>
  void error_handler_::operator()(info const& what, Iterator err_pos, Iterator last) const
  {
    std::cout
      << "Error! Expecting "
      << what                         // what failed?
      << " here: \""
      << std::string(err_pos, last)   // iterators to error-pos, end
      << "\""
      << std::endl
      ;
  }

  phoenix::function<error_handler_> const error_handler = error_handler_();

  template <typename Iterator>
  white_space<Iterator>::white_space() : white_space::base_type(start)
  {
    using namespace qi;
    
    start =
      space                            // tab/space/cr/lf
      | ";" >> *(char_ - eol) >> eol     // C-style comments
      ;
  }

  template <typename Iterator>
  interpreter<Iterator>::interpreter(bool _show_debug) : interpreter::base_type(sexpr), show_debug(_show_debug)
  {
    using namespace qi::labels;
    using ascii::alpha;
    using ascii::alnum;
    using ascii::char_;
    using boost::spirit::lexeme;
    using boost::spirit::raw;
      
    using namespace boost::phoenix;

    sexpr =
      atom                           [ _val = _1               ] 
      | "'" >> sexpr                 [ _val = quote(_1)        ]
      | "`" >> sexpr                 [ _val = backtick(_1)     ]
      | ",@" >> sexpr                [ _val = comma_at(_1)     ]
      | "," >> sexpr                 [ _val = comma(_1)        ]
      | cons                         [ _val = _1               ]
      | ( char_("(") >> ( +sexpr )   [ _val = p(_1)            ] 
          > char_(")"))
      ;
      
    identifier = 
      raw
      [
         lexeme
         [
            +(alnum | '+' | '-' | '*' | '/')
	  ]
      ][_val = p(_1)];

    escaped_char %= ('\\' >> char_) | (char_ - '"')
      ;

    quoted_string = 
      lexeme['"' >> +escaped_char >> '"'][ _val = p(_1) ]
      ;

    atom %=
      nil
      | double_     
      | identifier  
      | quoted_string
      ;
      
    nil = 
      ((char_("(") >> char_(")")) | "NIL" | "nil") [ _val = val(::lisp::nil) ];

    cons = 
      (char_("(") >> sexpr >> char_(".") >> sexpr >> char_(")"))   [ _val = make_cons(_2, _4) ]
      ;

    //on_error<fail>(sexpr, error_handler(_4, _3, _2));
    //on_error<retry>(sexpr, error_handler(_4, _3, _2));
    //on_error<accept>(sexpr, error_handler(_4, _3, _2));
    //on_error<rethrow>(sexpr, error_handler(_4, _3, _2));
      
    if (show_debug)
      {
	nil.name("nil");
	atom.name("atom");
	sexpr.name("sexpr");
	identifier.name("identifier");
	cons.name("cons");
	quoted_string.name("quoted_string");
	start.name("start");
	escaped_char.name("escaped_char");

	debug(nil);
	debug(atom);
	debug(sexpr);
	debug(identifier);
	debug(cons);
	debug(quoted_string);
	debug(start);
	debug(escaped_char);
      }
  }

  namespace {
    void instantiate()
    {
      interpreter_t interp(false);
      skipper_t skipper;
    }
  }
}
