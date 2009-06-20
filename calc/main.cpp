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

namespace lisp
{
  namespace qi = boost::spirit::qi;
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

    variant operator()(const std::vector<char>& v) const
    {
      return std::string(v.begin(), v.end());
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

    // currently only used for quote
    variant operator()(char c, const variant& v) const
    {
      // if (c != '\'')
      //   throw std::runtime_error("we shouldn't ever see this");
      //std::cout << __PRETTY_FUNCTION__ << "\n";
      cons_ptr head = new cons;
      cons_ptr tail = new cons;
      tail->car = v;
      
      head->car = symbol("quote");
      head->cdr = tail;
      return head;
    }

    // cons
    variant operator()(const variant& l, const variant& r) const
    {
      //std::cout << __PRETTY_FUNCTION__ << "\n";
      cons_ptr c = new cons;
      
      c->car = l;
      c->cdr = r;

      return c;
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
      using boost::phoenix::construct;
      using boost::spirit::lexeme;
      using boost::spirit::raw;
      
      using namespace boost::phoenix;
      
      identifier = 
	  raw[lexeme[+(alnum | '+' | '-' | '*' | '/')]][_val = p(_1)];

      quoted_string = 
	lexeme['"' >> +(char_ - '"') >> '"'][_val = p(_1)];

      atom %=
          double_     
	| identifier  
	| quoted_string
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
	  quoted_string.name("quoted_string");


	  debug(nil);
	  debug(atom);
	  debug(sexpr);
	  debug(identifier);
	  debug(quote);
	  debug(cons);
	  debug(quoted_string);
	}
    }
    
    qi::rule<Iterator, variant(), ascii::space_type> 
    atom, sexpr, nil, identifier, quote, cons, quoted_string;
    //    qi::rule<Iterator, std::string(), qi::locals<std::string>, ascii::space_type> 
    //    quoted_string;

  };

}

using namespace lisp;

void add_builtins()
{
  global->put("+", lisp::function(lisp::ops::op<std::plus<double> >(0)));
  global->put("*", lisp::function(lisp::ops::op<std::multiplies<double> >(1)));
  global->put("-", lisp::function(lisp::ops::minus()));
  global->put("/", lisp::function(lisp::ops::divides()));
  global->put("quote", lisp::function(lisp::ops::quote()));
  global->put("cons", lisp::function(lisp::ops::cons()));
  global->put("list", lisp::function(lisp::ops::list()));
  global->put("defvar", lisp::function(lisp::ops::defvar()));
  global->put("print", lisp::function(lisp::ops::print()));
  global->put("eval", lisp::function(lisp::ops::evaluate()));
  global->put("defun", lisp::function(lisp::ops::defun()));
  global->put("progn", lisp::function(lisp::ops::progn()));
  global->put("equal", lisp::function(lisp::ops::equal()));
  global->put("if", lisp::function(lisp::ops::if_clause()));
  global->put("setf", lisp::function(lisp::ops::setf()));

  global->put("t", t);
  global->put("nil",  nil);
}


int repl(bool debug, std::istream& is)
{
  using boost::spirit::ascii::space;
  typedef std::string::const_iterator iterator_type;
  typedef lisp::interpreter<iterator_type> interpreter_t;
  
  interpreter_t lispi(debug); // Our grammar
  
  std::string str;
  
  std::cout << "A lisp interpreting plaything\n(c) 2009 Troy D. Straszheim\n";
  
  std::cout << "----------------------------\n> ";

  unsigned i = 0;

  while (std::getline(is, str))
    {
      i++;
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
	      lisp::dot d("parsed", i);
	      d(result);

	      std::cout << "\nparsed as> ";
	      repr(result);
	      std::cout << "\n";
	    }

	  try {
	    variant out = eval(global, result);
	    if (debug)
	      {
		std::cout << "\nevalled to> ";
		dbg(out);
		std::cout << "\n";
		lisp::dot d("result", i);
		d(out);
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


int offline(bool debug, std::istream& is)
{
  using boost::spirit::ascii::space;
  typedef std::string::const_iterator iterator_type;
  typedef lisp::interpreter<iterator_type> interpreter_t;
  
  interpreter_t lispi(debug); // Our grammar
  
  std::string code;
  
  unsigned i = 0;

  do {
    char c = is.get();
    if (! is.eof())
      code += c;
  } while (! is.eof());

  std::string::const_iterator pos = code.begin(), end = code.end();

  while (pos < end)
    {
      i++;
      lisp::variant result;
      bool r = phrase_parse(pos, end, lispi, space, result);

      lisp::cons_debug dbg(std::cout);
      lisp::cons_print repr(std::cout);

      if (r)
        {
	  cons_ptr c = new cons(result);

	  if (debug)
	    {
	      std::cout << "\nparsed as> ";
	      dbg(result);
	      lisp::dot d("parsed", i);
	      d(result);

	      std::cout << "\nparsed as> ";
	      repr(result);
	      std::cout << "\n";
	    }

	  try {
	    variant out = eval(global, result);
	    if (debug)
	      {
		std::cout << "\nevalled to> ";
		dbg(out);
		std::cout << "\n";
		lisp::dot d("result", i);
		d(out);
	      }
	  } catch (const std::exception& e) {
	    std::cout << "*** - EVAL exception caught: " << e.what() << "\n";
	  }
        }
      else
        {
	  throw std::runtime_error("parsing failed");
        }
    }
  return 0;
}


///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int
main(int argc, char** argv)
{
  add_builtins();

  std::list<std::string> args(argv+1, argv+argc);

  bool debug;
  if(args.front() == "-d")
    {
      debug = true;
      args.pop_front();
    }
  if (args.size() == 1)
    {
      std::ifstream is(args.front().c_str());
      offline(debug, is);
    }
  else
    repl(debug, std::cin);
}

