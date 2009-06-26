//
// Copyright Troy D. Straszheim 2009
//
// Distributed under the Boost Software License, Version 1.0
// See http://www.boost.org/LICENSE_1.0.txt
//

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

  struct variant_maker
  {
    template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
    struct result
    {
      typedef variant type;
    };
  };

  struct make_list_actor : variant_maker
  {
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
  namespace 
  {
    phoenix::function<make_list_actor> make_list;
  }

  // cons
  struct make_cons_actor : variant_maker
  {
    variant operator()(const variant& car, const variant& cdr) const
    {
      return cons_ptr(new cons(car, cdr));
    }
  };

  namespace 
  {
    phoenix::function<make_cons_actor> make_cons;
  }

  template <typename T>
  struct sugar_ : variant_maker
  {
    variant operator()(const variant& v) const
    {
      return special<T>(v);
    }
  };

  template struct sugar_<backquoted_>;

  namespace {
    phoenix::function<sugar_<backquoted_> > backquote;
    phoenix::function<sugar_<quoted_> > quote;
    phoenix::function<sugar_<comma_at_> > comma_at;
    phoenix::function<sugar_<comma_> > comma;
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

  namespace {
    phoenix::function<error_handler_> const error_handler = error_handler_();
  }


  template <typename Iterator>
  white_space<Iterator>::white_space() : white_space::base_type(start)
  {
    using namespace qi;
    
    start =
      space                            // tab/space/cr/lf
      | ";" >> *(char_ - eol) >> eol     // C-style comments
      | "#|" >> comment_end
      ;

    comment_end = (char_ >> "|#") || (char_ >> comment_end)
      ;

  }

  template <typename Iterator>
  interpreter<Iterator>::interpreter(bool _show_debug) 
  : interpreter::base_type(sexpr), show_debug(_show_debug)
  {
    using namespace qi::labels;
    using ascii::alpha;
    using ascii::alnum;
    using ascii::char_;
    using boost::spirit::lexeme;
    using boost::spirit::raw;
      
    using namespace boost::phoenix;

    backquote_depth = 0;

    sexpr =
      atom                           [ _val = _1               ] 
      | "'" >> sexpr                 [ _val = quote(_1)        ]
      | "`" >> sexpr                 [ _val = backquote(_1)    ]
      | ",@" >> sexpr                [ _val = comma_at(_1)     ]
      | "," >> sexpr                 [ _val = comma(_1)        ]  
      | cons                         [ _val = _1               ]
      | ( char_("(") >> ( +sexpr )   [ _val = make_list(_1)    ]
          > char_(")"))
      ;
      
    identifier = 
      lexeme[
	     +(alnum | char_("+") | char_("-") | char_("*") | char_("/"))
	     ]
      [ 
       _val = construct<symbol>(_1) 
	];

    escaped_char %= ('\\' >> char_) | (char_ - '"')
      ;

    quoted_string = 
      lexeme[ '"' >> *escaped_char >> '"' ]
      [ _val = construct<std::string>(&front(_1), size(_1)) ]
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
