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

#include <iostream>
#include <string>
#include <map>

#include "types.hpp"
#include "ops.hpp"
#include "context.hpp"
#include "eval.hpp"

namespace lisp
{
  namespace qi = boost::spirit::qi;
  namespace phoenix = boost::phoenix;
  namespace ascii = boost::spirit::ascii;

  
  const static variant nil(cons_ptr(0));

  bool is_nil(const variant& v)
  {
    const cons_ptr p = boost::get<cons_ptr>(v);
    return p.get() == 0;
  }

  bool is_ptr(const variant& v)
  {
    return boost::get<cons_ptr>(&v);
  }


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

    // currently only used for quote
    variant operator()(char c, const variant& v) const
    {
      //      if (c != '\'')
      //	throw std::runtime_error("we shouldn't ever see this");
      std::cout << __PRETTY_FUNCTION__ << "\n";
      cons_ptr head = new cons;
      
      head->car = symbol("quote");
      head->cdr = v;

      return head;
    }

    // cons
    variant operator()(const variant& l, const variant& r) const
    {
      //      if (c != '\'')
      //	throw std::runtime_error("we shouldn't ever see this");
      std::cout << __PRETTY_FUNCTION__ << "\n";
      cons_ptr c = new cons;
      
      c->car = l;
      c->cdr = r;

      return c;
    }
  };

  struct parens
  {
    std::ostream& os;
    parens(std::ostream& _os) : os(_os) { os << "("; }
    ~parens() { os << ")"; }
  };

  struct cons_debug
  {
    typedef void result_type;
    
    std::ostream& os;

    cons_debug(std::ostream& _os) 
      : os(_os) 
    { }

    void operator()(double d)
    {
      os << "(double:" << d << ")";
    }
    
    void operator()(const std::string& s)
    {
      os << "\"" << s << "\"";
    }
    
    void operator()(const symbol& s)
    {
      os << "symbol:" << s;
    }
    
    void operator()(const cons_ptr p)
    {
      if (!p)
	{
	  os << "NIL";
	  return;
	}
      os << "(cons @" << p.get() << " car:";
      boost::apply_visitor(*this, p->car);

      os << " cdr:";
      boost::apply_visitor(*this, p->cdr);

      os << ")";
    }

    void operator()(const function f)
    {
      os << "function@" << &f << " \"" << f.name << "\"\n";
    }

    void operator()(const variant v)
    {
      boost::apply_visitor(*this, v);
    }
  };
  
  std::ostream& operator<<(std::ostream& os,
			   cons_ptr cp)
  {
    cons_debug printer(os);
    printer(cp);
    return os;
  }

  std::ostream& operator<<(std::ostream& os,
			   variant v)
  {
    cons_debug printer(os);
    printer(v);
    return os;
  }

  struct cons_print
  {
    typedef void result_type;
    
    std::ostream& os;

    cons_print(std::ostream& _os) 
      : os(_os) 
    { }

    void operator()(double d)
    {
      os << d;
    }
    
    void operator()(const std::string& s)
    {
      os << "\"" << s << "\"";
    }
    
    void operator()(const symbol& s)
    {
      os << s;
    }
    
    void operator()(const cons_ptr p)
    {
      if (!p) 
	{
	  os << "NIL";
	  return;
	}
      bool branch = is_ptr(p->car) && !is_nil(p->car) && is_ptr(p->cdr);
      if (branch)
	os << "(";
      boost::apply_visitor(*this, p->car);
      if (branch)
	os << ")";
      bool recur = false;
      if (is_ptr(p->cdr) && !is_nil(p->cdr))
	{
	  os << " ";
	  boost::apply_visitor(*this, p->cdr);
	}
      if (!is_ptr(p->cdr))
	{
	  os << " . ";
	  boost::apply_visitor(*this, p->cdr);
	}
    }

    void operator()(const function f)
    {
      os << "function@" << &f << "\n";
    }

    void operator()(const variant v)
    {
      if (is_ptr(v) && is_nil(v)) 
	{
	  os << "NIL";
	  return;
	}
      bool branch = is_ptr(v);
      if (branch)
	os << "(";
      boost::apply_visitor(*this, v);
      if (branch)
	os << ")";
    }
  };
  
  template <typename Iterator>
  struct interpreter 
    : qi::grammar<Iterator, variant(), ascii::space_type>
  {
    boost::phoenix::function<lisp::process> p;
    bool show_debug;
    interpreter(bool _show_debug) : interpreter::base_type(sexpr), show_debug(_show_debug)
    {
      using namespace qi::labels;
      using ascii::alpha;
      using ascii::alnum;
      using ascii::char_;
      using qi::double_;
      using qi::int_;
      using qi::uint_;
      using qi::lit;
      using qi::on_error;
      using qi::fail;
      using qi::debug;
      using boost::spirit::lexeme;
      using boost::spirit::raw;
      
      using namespace boost::phoenix;
      
      identifier = 
	  raw[lexeme[+(alnum | '+' | '-' | '*' | '/')]][_val = p(_1)];

      atom =
          double_      [ _val = p(_1) ]
	| identifier   [ _val = _1    ]
      	;
      
      nil = 
          (char_("(") >> char_(")"));

      sexpr =
	  atom                      [ _val = _1 ] 
	| nil                       [ _val = val(::lisp::nil) ]
	| (char_("'") >> sexpr)     [ _val = p(_1, _2) ]
	| quote                     [ _val = _1 ]
	| cons                      [ _val = _1 ]
	| (char_("(") >> (+sexpr)   [ _val = p(_1) ] >> char_(")"))
	;
      
      quote = 
	(char_("(") >> "quote" >> sexpr >> char_(")"))   [ _val = p(_1, _2) ]
	;

      cons = 
	(char_("(") >> sexpr >> char_(".") >> sexpr >> char_(")"))   [ _val = p(_2, _4) ]
	;

      on_error<fail>
	(
	 sexpr
	 , std::cout
	 << val("Error! Expecting ")
	 << _4                               // what failed?
	 << val(" here: \"")
	 << construct<std::string>(_3, _2)   // iterators to error-pos, end
	 << val("\"")
	 << std::endl
	 );
      
      if (show_debug)
	{
	  nil.name("nil");
	  atom.name("atom");
	  sexpr.name("sexpr");
	  identifier.name("identifier");
	  quote.name("quote");
	  cons.name("cons");

	  debug(nil);
	  debug(atom);
	  debug(sexpr);
	  debug(identifier);
	  debug(quote);
	  debug(cons);
	}
    }
    
    qi::rule<Iterator, variant(), ascii::space_type> 
    atom, sexpr, nil, identifier, quote, cons;

    //    qi::rule<Iterator, int(), ascii::space_type> 
    //    identifier;

  };

  context_ptr global;

}

using namespace lisp;

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int
main(int argc, char** argv)
{
  std::cout << "A lisp interpreting plaything\n(c) 2009 Troy D. Straszheim\n";
  
  std::vector<std::string> args(argv, argv+argc);
  bool debug = false;

  if (argc > 1)
    {
      if (args[1] == "-d")
	debug = true;
    }

  global = context_ptr(new context);

  global->fns["+"] = lisp::function(lisp::ops::op<std::plus<double> >(0));
  global->fns["-"] = lisp::function(lisp::ops::op<std::minus<double> >(0));
  global->fns["*"] = lisp::function(lisp::ops::op<std::multiplies<double> >(1));
  global->fns["/"] = lisp::function(lisp::ops::divides());
  global->fns["quote"] = lisp::function(lisp::ops::quote());
  global->fns["cons"] = lisp::function(lisp::ops::cons());
  global->fns["list"] = lisp::function(lisp::ops::list());

  using boost::spirit::ascii::space;
  typedef std::string::const_iterator iterator_type;
  typedef lisp::interpreter<iterator_type> interpreter_t;
  
  interpreter_t lispi(debug); // Our grammar
  
  std::string str;
  
  std::cout << "----------------------------\n> ";

  while (std::getline(std::cin, str))
    {
      std::string::const_iterator iter = str.begin();
      std::string::const_iterator end = str.end();
      lisp::variant result;
      bool r = phrase_parse(iter, end, lispi, space, result);

      lisp::cons_debug dbg(std::cout);
      lisp::cons_print repr(std::cout);

      if (r && iter == end)
        {
	  cons_ptr c = new cons(result);

	  if (debug)
	    {
	      std::cout << "\nparsed as> ";
	      dbg(result);
	      std::cout << "\nparsed as> ";
	      repr(result);
	      std::cout << "\n";
	    }

	  try {
	    eval e(global);
	    variant out = e(result);
	    if (debug)
	      {
		std::cout << "\nevalled to> ";
		dbg(out);
		std::cout << "\n";
	      }
	    repr(out);
	    std::cout << "\n";
	  } catch (const std::exception& e) {
	    std::cout << "*** - EVAL exception caught: " << e.what() << "\n";
	  }
        }
      else
        {
	  std::cout << "Error, parsing failed\n";
        }
      std::cout << "> ";
    }
  std::cout << "Ciao.\n";
  return 0;
}


