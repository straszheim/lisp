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

struct distance_impl
{
  template <typename T0 = void, typename T1 = void>
  struct result {
    typedef unsigned type;
  };
  
  template <typename Iter>
  unsigned 
  operator()(const Iter& l, const Iter& r) const
  {
    return std::distance(l, r);
  }
};

boost::phoenix::function<distance_impl> const distance = distance_impl();

static std::map<std::size_t, std::string> names;

template <typename Lexer>
struct python_tokens : lexer<Lexer>
{
  python_tokens()
  {
    using boost::phoenix::val;
    using boost::phoenix::ref;
    using boost::spirit::lex::_val;

    dentlevel = 0;

    // define the tokens to match
    identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
    constant = "[0-9]+";
    if_ = "if";
    else_ = "else";
    while_ = "while";
    return_ = "return";
    def = "def";

    comment = "#[^\\n]*";

    tab = "\\t";
    newline = "\\n";
    whitespace = "\\x20+";

    this->self("INITIAL") = 
      if_
      | def
      | else_ 
      | while_ 
      | return_ 
      | identifier
      | constant
      | '(' | ')' | ':' | '=' | '+' | '>' | '<'
      | comment
      | newline
      | whitespace [ _val = distance(_start, _end) ]
      | tab
      ;

    this->self("UNUSED") = indent | dedent;

    names[if_.id()] = "IF";
    names[identifier.id()] = "IDENTIFIER";
    names[comment.id()] = "COMMENT";
    names[newline.id()] = "NEWLINE";
    names[whitespace.id()] = "WHITESPACE";
    names[tab.id()] = "TAB";
    names[def.id()] = "DEF";
    names[constant.id()] = "CONSTANT";
    names[indent.id()] = "INDENT";
    names[dedent.id()] = "DEDENT";
    names[return_.id()] = "RETURN";

  }

  unsigned dentlevel;

  token_def<> if_, else_, while_, return_, def, newline, comment, indent, dedent, tab;

  token_def<std::string> identifier;
  token_def<unsigned int> constant, whitespace;
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
    dentfixing_iterator(iterator_type& iter_) 
      : iter(iter_), n_dedents(0), prev_was_newline(false)
    { }

    void increment() 
    { 
      std::cout << *iter << "\t" << names[*iter] << "\t<<<" << iter->value() << ">>>\n";
      iter++; 
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
    std::stack<unsigned> stack;
    unsigned n_dedents;
    bool prev_was_newline;
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
    plus = tok.identifier >> tok.whitespace >> '+' >> tok.whitespace >> tok.identifier 
			  >> tok.newline;
    start = +(plus | tok.newline) ;

    start.name("start");
    plus.name("plus");

    debug(start);
    debug(plus);
  }

  typedef boost::variant<unsigned int, std::string> expression_type;

  rule<Iterator, std::string()> start, plus;

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
        
  python::dentfixing_iterator dfiter(iter), dfend(end);

  python::python_grammar<python::dentfixing_iterator> grammar(tokens);

  if (true)
    {
      for (;dfiter != dfend; dfiter++)
	dfiter++;
      exit(0);
    }


  std::string result;
  bool r = phrase_parse(dfiter, dfend, grammar, result);
  std::cout << "r = " << r << "\n";

  return 0;

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

}
