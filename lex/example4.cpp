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

#include <iostream>
#include <fstream>
#include <string>

#include "example.hpp"

using namespace boost::spirit;
using namespace boost::spirit::qi;
using namespace boost::spirit::lex;

using boost::phoenix::val;
using boost::phoenix::ref;

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
struct set_state_
{
  template <typename T1=void, typename T2=void, typename T3=void, typename T4=void, typename T5=void>
  struct result
  {
    typedef void type;
  };

  typedef void result_type;

  std::string to_what;

  set_state_(const char* _to_what) 
    : to_what(_to_what) 
  { }

  set_state_(const std::string& _to_what) 
    : to_what(_to_what) 
  { }

  template <typename T1, typename T2, typename T3, typename T4, typename T5>
  void operator()(T1, T2, T3, T4, T5& thingy) const
  {
    std::cout << __PRETTY_FUNCTION__ << "\n SET STATE:" << to_what << "\n";
    thingy.set_state_name(to_what.c_str());
  }
};

// typedef boost::phoenix::function<set_state_> const setter;

template <typename Lexer>
struct example4_tokens : lexer<Lexer>
{
  typedef typename Lexer::token_set token_set;

  example4_tokens()
  {
    using boost::spirit::_state;
    using boost::phoenix::val;
    using boost::phoenix::ref;

    // define the tokens to match
    identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
    constant = "[0-9]+";
    if_ = "if";
    else_ = "else";
    while_ = "while";
    newline = "\\n";
    comment = "#[^\\n]*";
    def = "def";
    whitespace = "[ ]+"; 

    //   token_def<>("[ \\t\\n]+")[ ref(dentlevel) = distance(_start, _end) ]
    //      |   "\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/"
    //      ;
    // associate the tokens and the token set with the lexer
    //    this->self = token_def<>('(') | ')' | '{' | '}' | '=' | ';' | constant;

    this->self = 
      if_
      | def
      | else_ 
      | while_ 
      | identifier
      | constant
      | '(' | ')' | ':' | '=' | '+'
      | comment
      | newline[ set_state_("WS") ]
      | whitespace [ set_state_("INITIAL") ]
      ;
    
    skip_set = "[ \\t\\n]+" ;

    this->self("WS") = skip_set;

    names[identifier.id()] = "IDENTIFIER";
    names[comment.id()] = "COMMENT";
    names[newline.id()] = "NEWLINE";
    names[whitespace.id()] = "WHITESPACE";
    names[def.id()] = "DEF";
    names[constant.id()] = "CONSTANT";
  }

  unsigned dentlevel;

  std::map<std::size_t, std::string> names;

  token_def<> if_, else_, while_, def, newline, comment, whitespace;

  token_def<std::string> identifier;
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
    : example4_grammar::base_type(program)
  {
    program =  +block
      ;

    block
      =   *statement
      ;

    comment = tok.comment
      ;

    newline = tok.newline
      ;

    statement 
      =   assignment
      | if_stmt
      | while_stmt
      | comment
      | tok.whitespace
      | newline
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

    program.name("program");
    debug(program);

    block.name("block");
    debug(block);

    statement.name("statement");
    debug(statement);

    comment.name("comment");
    debug(comment);

    newline.name("newline");
    debug(newline);

  }

  typedef typename Lexer::token_set token_set;
  typedef boost::variant<unsigned int, std::string> expression_type;

  rule<Iterator, in_state_skipper<token_set> > program, block, statement,
						 assignment, if_stmt,
						 while_stmt, comment,
						 newline
						 ;



  //  the expression is the only rule having a return value
  rule<Iterator, expression_type(), in_state_skipper<token_set> >  expression;
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
  // iterator type used to expose the underlying input stream
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

  // this is the type of the grammar to parse
  typedef example4_grammar<iterator_type, lexer_type> example4_grammar;

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
        
  unsigned z = 0;
  while(*iter != 0 && z < 50)
    {
      std::cout << "[" << *iter << " ";
      if (*iter < 127)
	{
	  char c = *iter;
	  std::cout << c;
	}
      else
	{
	  std::cout << tokens.names[*iter];
	}
      std::cout << "]\n";
      
      iter++;
      z++;
    }
  return 0;
  // Parsing is done based on the the token stream, not the character 
  // stream read from the input.
  // Note, how we use the token_set defined above as the skip parser. It must
  // be explicitly wrapped inside a state directive, switching the lexer 
  // state for the duration of skipping whitespace.
  bool r = phrase_parse(iter, end, calc, in_state("WS")[tokens.skip_set]);

  std::cout << "r=" << r << "\n";

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
