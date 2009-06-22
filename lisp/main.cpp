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

using namespace lisp;

void add_builtins()
{
  global->put("+", lisp::function(lisp::ops::op<std::plus<double> >(0)));
  global->put("*", lisp::function(lisp::ops::op<std::multiplies<double> >(1)));
  global->put("-", lisp::function(lisp::ops::minus()));
  global->put("/", lisp::function(lisp::ops::divides()));
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
  global->put("defmacro", lisp::function(lisp::ops::defmacro()));
  global->put("lambda", lisp::function(lisp::ops::lambda()));
  //  global->put("macroexpand", lisp::function(lisp::ops::macroexpand()));

  global->put("t", t);
  global->put("nil",  nil);
}

skipper_t skipper;

int repl(bool debug, std::istream& is)
{
  interpreter_t lispi(debug); // Our grammar
  
  std::string str;
  
  std::cout << "A lisp interpreting plaything\n(c) 2009 Troy D. Straszheim\n";
  
  std::cout << "----------------------------\n> ";

  unsigned i = 0;

  while (std::getline(is, str))
    {
      i++;
      str += "\n";
      std::string::const_iterator iter = str.begin();
      std::string::const_iterator end = str.end();
      lisp::variant result;
      while (iter != end)
	{
	  bool r = phrase_parse(iter, end, lispi, skipper, result);

	  if (!r)
	    {
	      std::cout << "parsing failed.\n";
	      iter = end;
	    }
	  else
	    {
	      lisp::cons_debug dbg(std::cout);

	      cons_ptr c = new cons(result);

	      if (debug)
		{
		  std::cout << "\nparsed as> ";
		  //		  dbg(result);
		  //		  lisp::dot d("parsed", i);
		  //		  d(result);

		  std::cout << "\nparsed as> ";
		  lisp::print(std::cout, result);
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
		lisp::print(std::cout, out);
		std::cout << "\n";
	      } catch (const std::exception& e) {
		std::cout << "*** - EVAL exception caught: " << e.what() << "\n";
	      }
	    }
        }
      std::cout << "> ";
    }
  std::cout << "Ciao.\n";
  return 0;
}


int offline(bool debug, std::istream& is)
{
  interpreter_t lispi(debug); // Our grammar
  
  std::string code;
  
  unsigned i = 0;

  do {
    char c = is.get();
    if (! is.eof())
      code += c;
  } while (! is.eof());

  std::string::const_iterator pos = code.begin(), end = code.end();

  // if there's a hashbang,  strip her
  if (*pos == '#')
    while (*pos != '\n')
      pos++;

  while (pos < end)
    {
      i++;
      lisp::variant result;
      bool r = phrase_parse(pos, end, lispi, skipper, result);

      lisp::cons_debug dbg(std::cout);

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
	      lisp::print(std::cout, result);
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

  bool debug = false;
  if(args.size() > 0 && args.front() == "-d")
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

