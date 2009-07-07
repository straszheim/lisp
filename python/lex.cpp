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

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
//  Helper function reading a file into a string
///////////////////////////////////////////////////////////////////////////////
inline std::string 
read_from_file(char const* infile)
{
    std::ifstream instream(infile);
    if (!instream.is_open()) {
        std::cerr << "Couldn't open file: " << infile << std::endl;
        exit(-1);
    }
    instream.unsetf(std::ios::skipws);      // No white space skipping!
    return std::string(std::istreambuf_iterator<char>(instream.rdbuf()),
                       std::istreambuf_iterator<char>());
}



#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <boost/spirit/home/lex/argument.hpp>
#include <boost/spirit/home/lex/lexer/pass_flags.hpp>
#include <boost/format.hpp>

#include <boost/iterator/iterator_facade.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <stack>

using namespace boost::spirit;
using namespace boost::spirit::qi;
using namespace boost::spirit::lex;

using boost::phoenix::val;
using boost::phoenix::ref;
using boost::phoenix::construct;


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

static std::map<std::size_t, std::string> names;

template <typename Lexer>
struct python_tokens : lexer<Lexer>
{
  python_tokens()
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
    whitespace = "\\x20*";
    dent = "\\x20*";

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
      | '(' | ')' | ':' | '=' | '+' | '>' | '<'
      | comment
      | newline[ _state = "DENT" ]
      | whitespace
      ;

    this->self("DENT") = dent [ _state = "INITIAL" ] 
      ;

    this->self("UNUSED") = indent | dedent;

    names[if_.id()] = "IF";
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

  token_def<> if_, else_, while_, def, newline, comment, whitespace, indent, dedent;

  token_def<std::string> identifier, dent;
  token_def<unsigned int> constant;
};



namespace python {
  typedef std::string::iterator base_iterator_type;

  typedef lexertl::token< 
  base_iterator_type, boost::mpl::vector<unsigned int, std::string>
    > token_type;

  //]
  // Here we use the lexertl based lexer engine.
  typedef lexertl::actor_lexer<token_type> lexer_type;

  // This is the token definition type (derived from the given lexer type).
  typedef python_tokens<lexer_type> python_tokens;

  // this is the iterator type exposed by the lexer 
  typedef python_tokens::iterator_type iterator_type;
  typedef std::vector<iterator_type::value_type>::iterator grammar_iterator_type;

  class dentfixing_iterator
    : public boost::iterator_facade< dentfixing_iterator,
				     iterator_type::value_type,
				     boost::forward_traversal_tag
				     >
  { 
  public:
    dentfixing_iterator(iterator_type& iter_) : iter(iter_) { }

    void increment() { 
      iter++; 
      if (*iter < 127)
	std::cout << *iter << "\n";
      else
	std::cout << names[*iter] << "\n";
    }
    
    bool equal(dentfixing_iterator const& other) const
    {
        return this->iter == other.iter;
    }

    iterator_type::value_type& dereference() const 
    { 
      return *iter; 
    }

    iterator_type iter;
  };



///////////////////////////////////////////////////////////////////////////////
//  GRAMMAR DEFINITION
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator>
struct python_grammar 
  : grammar<Iterator, std::string()>
{
  template <typename TokenDef>
  python_grammar(TokenDef const& tok)
    : python_grammar::base_type(start)
  {
    start = tok.identifier >> tok.whitespace >> '+' >> tok.whitespace >> tok.identifier 
			   >> tok.newline;

    start.name("start");

    debug(start);
    /*
    debug(skunk);
    debug(identifier);
    debug(dent);
    debug(newline);
    */
  }

  typedef boost::variant<unsigned int, std::string> expression_type;

  rule<Iterator, std::string()> start;

};




}
///////////////////////////////////////////////////////////////////////////////
int main()
{
  // now we use the types defined above to create the lexer and grammar
  // object instances needed to invoke the parsing process
  python::python_tokens tokens;                         // Our lexer

  std::string str (read_from_file("python.input"));

  // At this point we generate the iterator pair used to expose the
  // tokenized input stream.
  std::string::iterator it = str.begin();
  python::iterator_type iter = tokens.begin(it, str.end());
  python::iterator_type end = tokens.end();
        
  python::dentfixing_iterator dfiter(iter);
  python::dentfixing_iterator dfend(end);

  python::python_grammar<python::dentfixing_iterator> grammar(tokens);

  std::string result;
  bool r = phrase_parse(dfiter, dfend, grammar, result);
  std::cout << "r = " << r << "\n";
#if 0
  // 
  //  replace 'DENT' tokens with INDENT and DEDENT
  //
  std::vector<python::iterator_type::value_type> dented_tokens;
  std::stack<unsigned> stack;
  stack.push(0);

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
	      python::iterator_type::value_type newtoken = *iter;
	      newtoken.id(tokens.indent.id());
	      dented_tokens.push_back(newtoken);
	    }
	  else if (dist < stack.top())
	    {
	      while (dist < stack.top())
		{
		  stack.pop();
		  python::iterator_type::value_type newtoken = *iter;
		  newtoken.id(tokens.dedent.id());
		  dented_tokens.push_back(newtoken);
		}
	      if (dist != stack.top())
		throw "BAD DENT\n";
	    }
	}
      else if(*iter == tokens.whitespace.id())
	; // drop the token
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
    }

  // if the indentations match this stack size will be 1 (just the zero we started with)
  assert(stack.size() == 1);

  // print the postprocessed tokens
  std::cout << "\n\nAFTER PROCESSING:\n";
  
  for (std::vector<python::iterator_type::value_type>::iterator iter = dented_tokens.begin();
       iter != dented_tokens.end();
       iter++)
    {
      std::cout << "[" << *iter << "\t" << tokens.names[*iter] << "\t"
		<< iter->value() << "]\n";
    }
#endif


  return 0;
}
