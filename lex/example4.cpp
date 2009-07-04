//  Copyright (c) 2001-2009 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This example shows how to create a simple lexer recognizing a couple of 
//  different tokens aimed at a simple language and how to use this lexer with 
//  a grammar. It shows how to associate attributes to tokens and how to access 
//  the token attributes from inside the grammar.
//
//  We use explicit token attribute types, making the corresponding token instances
//  carry convert the matched input into an instance of that type. The token 
//  attribute is exposed as the parser attribute if this token is used as a 
//  parser component somewhere in a grammar.
//
//  Additionally, this example demonstrates, how to define a token set usable 
//  as the skip parser during parsing, allowing to define several tokens to be 
//  ignored.
//
//  This example recognizes a very simple programming language having 
//  assignment statements and if and while control structures. Look at the file
//  example4.input for an example.
//
// #define BOOST_SPIRIT_LEXERTL_DEBUG 1

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

#include <boost/spirit/include/lex_lexertl.hpp>

#include <boost/spirit/home/lex/argument.hpp>
#include <boost/spirit/home/lex/lexer/pass_flags.hpp>
#include <boost/format.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <stack>

#include "example.hpp"

using namespace boost::spirit;
using namespace boost::spirit::qi;
using namespace boost::spirit::lex;

using boost::phoenix::val;
using boost::phoenix::ref;
using boost::phoenix::construct;

struct distance_func
{
  template <typename Iterator1, typename Iterator2>
  struct result : boost::iterator_difference<Iterator1> {};

  template <typename Iterator1, typename Iterator2>
  typename result<Iterator1, Iterator2>::type 
  operator()(Iterator1& begin, Iterator2& end) const
  {
    return std::distance(begin, end);
  }
};
boost::phoenix::function<distance_func> const distance = distance_func();

///////////////////////////////////////////////////////////////////////////////
//  Token definition
///////////////////////////////////////////////////////////////////////////////

typedef boost::variant<unsigned, std::string> variant;

struct strlen_actor
{
  template <typename T>
  struct result {
    typedef unsigned type;
  };
  
  unsigned 
  operator()(const variant& v) const
  {
    const std::string& s = boost::get<std::string>(v);
    return s.size();
  }
};

boost::phoenix::function<strlen_actor> const strlen_ = strlen_actor();

template <typename Lexer>
struct example4_tokens : lexer<Lexer>
{
  typedef typename Lexer::token_set token_set;

  example4_tokens()
  {
    using boost::spirit::_state;
    using boost::phoenix::val;
    using boost::phoenix::ref;

    dentlevel = 0;

    // define the tokens to match
    identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
    constant = "[0-9]+";
    if_ = "if";
    else_ = "else";
    while_ = "while";
    newline = "\\n";
    comment = "#[^\\n]*";
    def = "def";
    whitespace = "[ ]*";
    dent = "[ ]*";

    //   token_def<>("[ \\t\\n]+")[ ref(dentlevel) = distance(_start, _end) ]
    //      |   "\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/"
    //      ;
    // associate the tokens and the token set with the lexer
    //    this->self = token_def<>('(') | ')' | '{' | '}' | '=' | ';' | constant;

    this->self("INITIAL") = 
      if_
      | def
      | else_ 
      | while_ 
      | identifier
      | constant
      | '(' | ')' | ':' | '=' | '+'
      | comment/*[ std::cout << _state ]*/
      | newline[ _state = "DENT" ]
      // | whitespace[ _state = "INITIAL", std::cout << _state ]
      ;

    this->self("DENT") = dent [ _state = "INITIAL" ] 
      ;

    skip_set = "[ \\t\\n]+" 
      ;

    this->self("WS") = skip_set
      ;

    this->self("UNUSED") = indent | dedent;

    names[identifier.id()] = "IDENTIFIER";
    names[comment.id()] = "COMMENT";
    names[newline.id()] = "NEWLINE";
    names[whitespace.id()] = "WHITESPACE";
    names[def.id()] = "DEF";
    names[constant.id()] = "CONSTANT";
    names[dent.id()] = "DENT";
    names[indent.id()] = "INDENT";
    names[dedent.id()] = "DEDENT";
  }

  unsigned dentlevel;

  std::map<std::size_t, std::string> names;

  token_def<> if_, else_, while_, def, newline, comment, whitespace, indent, dedent;

  token_def<std::string> identifier, dent;
  token_def<unsigned int> constant;

  // token set to be used as the skip parser
  token_set skip_set;

};

///////////////////////////////////////////////////////////////////////////////
//  Grammar definition
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator, typename Lexer>
struct example4_grammar 
  : grammar<Iterator, in_state_skipper<typename Lexer::token_set> >
{
  template <typename TokenDef>
  example4_grammar(TokenDef const& tok)
    : example4_grammar::base_type(skunk)
  {
    /*
    program 
      = +block
      ;

    block
      = *statement
      ;

    comment 
      = tok.comment
      ;

    newline 
      = tok.newline
      ;

    statement 
      =  assignment
      |  if_stmt
      |  while_stmt
      |  comment
      |  tok.whitespace
      |  newline
      ;

    assignment 
      =   (tok.identifier >> '=' >> expression >> ';')
      [
       std::cout << val("assignment statement to: ") << _1 << "\n"
       ]
      ;

    if_stmt
      =   (   tok.if_ >> '(' >> expression >> ')' >> block 
	      >> -(tok.else_ >> block) 
	      )
      [
       std::cout << val("if expression: ") << _2 << "\n"
       ]
      ;

    while_stmt 
      =   (tok.while_ >> '(' >> expression >> ')' >> block)
      [
       std::cout << val("while expression: ") << _2 << "\n"
       ]
      ;

    //  since expression has a variant return type accommodating for 
    //  std::string and unsigned integer, both possible values may be 
    //  returned to the calling rule
    expression 
      =   tok.identifier [ _val = _1 ]
      |   tok.constant   [ _val = _1 ]
      ;
    */

    identifier %= tok.identifier
      ;

    newline %= tok.newline
      ;

    dent = tok.dent [ _val = strlen_(_1) ]
      ;

    whitespace %= tok.whitespace
      ;

    skunk %= +(identifier | newline | dent | whitespace)[ std::cout << val("[[") << _1 << "]]\n" ];

    skunk.name("skunk");
    identifier.name("identifier");
    dent.name("dent");
    whitespace.name("whitespace");
    newline.name("newline");

    /*
    debug(whitespace);
    debug(skunk);
    debug(identifier);
    debug(dent);
    debug(newline);
    */

    dent_stack.push(0);
  }

  std::stack<unsigned> dent_stack;

  typedef typename Lexer::token_set token_set;
  typedef boost::variant<unsigned int, std::string> expression_type;

  rule<Iterator, std::string(), in_state_skipper<token_set> > identifier, newline, whitespace;

  rule<Iterator, unsigned(), in_state_skipper<token_set> > dent;

  /*
    rule<Iterator, expression_type(), in_state_skipper<token_set> >
    program, block, statement, assignment, if_stmt, while_stmt, comment,  expresssion;
  */

  //  the expression is the only rule having a return value
  rule<Iterator, in_state_skipper<token_set> > skunk;
};

struct dent_calculator
{
  std::stack<unsigned> stack;
};


template <typename T>
void boo(T const&)
{
  std::cout << __PRETTY_FUNCTION__ << "\n";
}


///////////////////////////////////////////////////////////////////////////////
int main()
{
  typedef std::string::iterator base_iterator_type;

  //[example4_token
  // This is the lexer token type to use. The second template parameter lists 
  // all attribute types used for token_def's during token definition (see 
  // calculator_tokens<> above). Here we use the predefined lexertl token 
  // type, but any compatible token type may be used instead.
  //
  // If you don't list any token attribute types in the following declaration 
  // (or just use the default token type: lexertl_token<base_iterator_type>)  
  // it will compile and work just fine, just a bit less efficient. This is  
  // because the token attribute will be generated from the matched input  
  // sequence every time it is requested. But as soon as you specify at 
  // least one token attribute type you'll have to list all attribute types 
  // used for token_def<> declarations in the token definition class above, 
  // otherwise compilation errors will occur.
  typedef lexertl::token< 
  base_iterator_type, boost::mpl::vector<unsigned int, std::string>
    > token_type;

  //]
  // Here we use the lexertl based lexer engine.
  typedef lexertl::actor_lexer<token_type> lexer_type;

  // This is the token definition type (derived from the given lexer type).
  typedef example4_tokens<lexer_type> example4_tokens;

  // this is the iterator type exposed by the lexer 
  typedef example4_tokens::iterator_type iterator_type;
  typedef std::vector<iterator_type::value_type>::iterator grammar_iterator_type;

  // this is the type of the grammar to parse
  typedef example4_grammar<grammar_iterator_type, lexer_type> example4_grammar;

  // now we use the types defined above to create the lexer and grammar
  // object instances needed to invoke the parsing process
  example4_tokens tokens;                         // Our lexer
  example4_grammar calc(tokens);                  // Our parser

  std::string str (read_from_file("example6.input"));

  // At this point we generate the iterator pair used to expose the
  // tokenized input stream.
  std::string::iterator it = str.begin();
  iterator_type iter = tokens.begin(it, str.end());
  iterator_type end = tokens.end();
        
  std::vector<iterator_type::value_type> dented_tokens;
  std::stack<unsigned> stack;
  stack.push(0);

  unsigned z = 0;
  while(iter != end)
    {
      std::cout << "\t[" << *iter << " " << iter->value() << "\t";

      if (*iter == tokens.dent.id())
	{
	  //	  unsigned u = boost::get<unsigned>(iter->value());
	  //	  std::string s(iter->value().begin(), iter->value().end());
	  boost::iterator_range<std::string::iterator> pr 
	    = boost::get<boost::iterator_range<std::string::iterator> >(iter->value());

	  std::size_t dist = std::distance(pr.begin(), pr.end());

	  std::cout << "DENT[" << iter->value() << " " << dist << "]\n";
	  if (dist > stack.top())
	    {
	      stack.push(dist);
	      iterator_type::value_type newtoken = *iter;
	      newtoken.id(tokens.indent.id());
	      dented_tokens.push_back(newtoken);
	    }
	  else if (dist < stack.top())
	    {
	      while (dist < stack.top())
		{
		  stack.pop();
		  iterator_type::value_type newtoken = *iter;
		  newtoken.id(tokens.dedent.id());
		  dented_tokens.push_back(newtoken);
		}
	      if (dist != stack.top())
		throw "BAD DENT\n";
	    }
	}
      else
	dented_tokens.push_back(*iter);

      if (*iter < 127)
	{
	  char c = *iter;
	  std::cout << c;
	}
      else
	{
	  std::cout << tokens.names[*iter];
	}
      std::cout << "\t" << tokens.dentlevel << "]\n";
      
      iter++;
      z++;
    }
  assert(stack.size() == 1);

  std::cout << "----------------------------\n--------------------------------\n";
  
  for (std::vector<iterator_type::value_type>::iterator iter = dented_tokens.begin();
       iter != dented_tokens.end();
       iter++)
    {
      std::cout << "[" << *iter << "\t" << tokens.names[*iter] << "\t"
		<< iter->value() << "]\n";
    }

bool r = phrase_parse(dented_tokens.begin(), dented_tokens.end(), 
		      calc, in_state("WS")[tokens.skip_set]);

  std::cout << "\nsuccess=" << r << "\n";

  if (r && iter == end)
    {
      std::cout << "-------------------------\n";
      std::cout << "Parsing succeeded\n";
      std::cout << "-------------------------\n";
    }
  else
    {
      std::cout << "-------------------------\n";
      std::string s(it, str.end());
      std::cout << "Parsing failed at >>>" << s << "<<<\n";
      std::cout << "-------------------------\n";
    }

  std::cout << "Bye... :-) \n\n";
  return 0;
}
