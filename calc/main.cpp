/*=============================================================================
  Copyright (c) 2001-2009 Joel de Guzman

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
  =============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  Same as cal4, but with debugging enabled.
//
//  [ JDG June 29, 2002 ]   spirit1
//  [ JDG March 5, 2007 ]   spirit2
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>

#include <iostream>
#include <string>

namespace client
{
  namespace qi = boost::spirit::qi;
  namespace phoenix = boost::phoenix;
  namespace ascii = boost::spirit::ascii;

  namespace ast
  {
    struct nil {};

    struct lisp_ast;

    struct lisp_ast
    {
      typedef std::pair<boost::shared_ptr<lisp_ast>, 
			boost::shared_ptr<lisp_ast> > 
      cons_t;

      typedef boost::variant<
	std::string,
	int,
	double,
	boost::recursive_wrapper<cons_t>
	>
      type;

      type expr;

      friend std::ostream& operator<<(std::ostream& os, const lisp_ast& ast)
      {
	return os << "ast";
      }

      lisp_ast& operator=(double d)
      {
	std::cout << __PRETTY_FUNCTION__ << "\n";
	expr = d;
	return *this;
      }

      lisp_ast& operator=(int i)
      {
	std::cout << __PRETTY_FUNCTION__ << "\n";
	expr = i;
	return *this;
      }

      template <typename T>
      lisp_ast& operator=(T const& t)
      {
	std::cout << __PRETTY_FUNCTION__ << "\n";
	expr = std::string(t.begin(), t.end());
	return *this;
      }

      lisp_ast& operator=(const std::vector<lisp_ast>& v)
      {
	std::cout << __PRETTY_FUNCTION__ << "\n";
	
	cons_t cons;
	for (std::vector<lisp_ast>::const_iterator iter = v.begin();
	     iter != v.end();
	     iter++)
	  {

	  }

	return *this;
      }

    };
  }
  ///////////////////////////////////////////////////////////////////////////////
  //  Our calculator grammar
  ///////////////////////////////////////////////////////////////////////////////
  template <typename Iterator>
  struct calculator : qi::grammar<Iterator, ast::lisp_ast(), ascii::space_type>
  {
    calculator() : calculator::base_type(sexpr)
    {
      using namespace qi::labels;
      using ascii::alpha;
      using ascii::alnum;
      using ascii::char_;
      using qi::double_;
      using qi::int_;
      using qi::uint_;
      using qi::on_error;
      using qi::fail;
      using qi::debug;
      using boost::spirit::lexeme;
      using boost::spirit::raw;

      using phoenix::construct;
      using phoenix::val;

      atom = raw[lexeme[alpha >> *alnum >> !alnum]][_val = _1]
	| double_[_val = _1]
	| int_[_val = _1]
	;

      sexpr = atom 
	| (char_("(") >> (*sexpr)[_val = _1] >> char_(")"))
	;

      /*
      expression =
	term                            [_val = _1]
	>> *(   ('+' > term             [_val += _1])
		|   ('-' > term             [_val -= _1])
		) 
	| atom
	;

      term =
	factor                          [_val = _1]
	>> *(   ('*' > factor           [_val *= _1])
		|   ('/' > factor           [_val /= _1])
		)
	;

      factor =
	uint_                           [_val = _1]
	|   '(' > expression            [_val = _1] > ')'
	|   ('-' > factor               [_val = -_1])
	|   ('+' > factor               [_val = _1])
	;
      */
      expression.name("expression");
      term.name("term");
      factor.name("factor");
      atom.name("atom");
      sexpr.name("sexpr");

      on_error<fail>
	(
	 expression
	 , std::cout
	 << val("Error! Expecting ")
	 << _4                               // what failed?
	 << val(" here: \"")
	 << construct<std::string>(_3, _2)   // iterators to error-pos, end
	 << val("\"")
	 << std::endl
	 );

      //      debug(expression);
      //      debug(term);
      //      debug(factor);
      debug(atom);
      debug(sexpr);
    }

    qi::rule<Iterator, ast::lisp_ast(), ascii::space_type> expression, term, factor, atom,
	    sexpr;
  };
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int
main()
{
  std::cout << "/////////////////////////////////////////////////////////\n\n";
  std::cout << "Expression parser...\n\n";
  std::cout << "/////////////////////////////////////////////////////////\n\n";
  std::cout << "Type an expression...or [q or Q] to quit\n\n";

  using boost::spirit::ascii::space;
  typedef std::string::const_iterator iterator_type;
  typedef client::calculator<iterator_type> calculator;

  calculator calc; // Our grammar

  std::string str;
  client::ast::lisp_ast result;
  while (std::getline(std::cin, str))
    {
      if (str.empty() || str[0] == 'q' || str[0] == 'Q')
	break;

      std::string::const_iterator iter = str.begin();
      std::string::const_iterator end = str.end();
      bool r = phrase_parse(iter, end, calc, space, result);

      if (r && iter == end)
        {
	  std::cout << "-------------------------\n";
	  std::cout << "Parsing succeeded\n";
	  //	  std::cout << "result = " << result << std::endl;
	  std::cout << "-------------------------\n";
        }
      else
        {
	  std::cout << "-------------------------\n";
	  std::cout << "Parsing failed\n";
	  std::cout << "-------------------------\n";
        }
    }

  std::cout << "Bye... :-) \n\n";
  return 0;
}


