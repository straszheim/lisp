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
#include <queue>

namespace python {

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
  struct lexer : boost::spirit::lex::lexer<Lexer>
  {
    lexer()
    {
      using boost::phoenix::val;
      using boost::phoenix::ref;
      using boost::spirit::lex::_val;

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

    token_def<> if_, else_, while_, return_, def, newline, comment, indent, dedent, tab;

    token_def<std::string> identifier;
    token_def<unsigned int> constant, whitespace;
  };

  typedef std::string::iterator base_iterator_type;

  typedef lexertl::token<base_iterator_type, 
			 boost::mpl::vector<unsigned int, std::string>
			 > 
  token_type;

  typedef lexertl::actor_lexer<token_type> lexer_type;

  typedef lexer<lexer_type> lexer_t;

  python::lexer_t lexer_;            

  typedef lexer_t::iterator_type iterator_type;
  typedef std::vector<iterator_type::value_type>::iterator grammar_iterator_type;

  class dentfixing_iterator
    : public boost::iterator_facade< dentfixing_iterator,
				     iterator_type::value_type,
				     boost::forward_traversal_tag
				     >
  { 
  public:
    dentfixing_iterator(iterator_type& iter_) 
      : iter(iter_), n_dedents(0), prev_was_newline(false), prev_was_indent(false)
    { 
      stack.push(0);
      indent_token.id(lexer_.indent.id());
      dedent_token.id(lexer_.dedent.id());
    }

    void increment() 
    { 
      if (queue.size())
	{
	  queue.pop();
	  return;
	}

      iter++; 

      if (*iter == lexer_.newline.id())
	{
	  prev_was_newline = true;
	  return;
	}

      if (prev_was_newline)
	{
	  unsigned dentlevel = *iter == lexer_.whitespace.id() ?
	    boost::get<unsigned>(iter->value()) : 0;
	  
	  prev_was_newline = false;

	  if (dentlevel > stack.top())
	    {
	      stack.push(dentlevel);
	      queue.push(indent_token);
	    }

	  if (dentlevel < stack.top())
	    {
	      while (dentlevel < stack.top())
		{
		  stack.pop();
		  queue.push(dedent_token);
		}
	      if (dentlevel != stack.top())
		throw "BAD DEDENT";
	    }
	}

      if (*iter == lexer_.whitespace.id()
	  || *iter == lexer_.comment.id())
	iter++;
    }
    
    bool equal(dentfixing_iterator const& other) const
    {
      return this->iter == other.iter;
    }

    iterator_type::value_type& dereference() const 
    { 
      if (queue.size())
	return queue.front();
      else
	return *iter;
    }

    iterator_type iter;
    std::stack<unsigned> stack;
    mutable std::queue<token_type> queue;

    mutable unsigned n_dedents;
    bool prev_was_newline;
    bool prev_was_indent;
    mutable token_type indent_token;
    mutable token_type dedent_token;
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
  std::string str (read_from_file("python.input"));

  // add a comment at the end so the lexer generates any needed dedents
  str += "\n#flush\n";

  // At this point we generate the iterator pair used to expose the
  // tokenized input stream.
  std::string::iterator it = str.begin();
  python::iterator_type iter = python::lexer_.begin(it, str.end());
  python::iterator_type end = python::lexer_.end();
        
  python::dentfixing_iterator dfiter(iter), dfend(end);

  python::python_grammar<python::dentfixing_iterator> grammar(python::lexer_);

  if (true)
    {
      while(dfiter != dfend)
	{
	  std::cout << boost::format("%-5u %10s [ %s ]\n") % *dfiter % python::names[*dfiter] % dfiter->value();
	  ++dfiter;
	}
      exit(0);
    }


  std::string result;
  bool r = phrase_parse(dfiter, dfend, grammar, result);
  std::cout << "r = " << r << "\n";

  return 0;
}
