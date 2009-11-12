
#include "lex.hpp"

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include <iostream>
#include <string>

#include <iostream>
#include <fstream>
#include <string>


using namespace boost::spirit::lex;

namespace lisp {

  template <typename Lexer>
  struct word_count_tokens : lexer<Lexer>
  {
    word_count_tokens()
    {
      /*
        // define tokens (the regular expression to match and the corresponding
        // token id) and add them to the lexer 
        this->self.add
	  ("\"[^\"]+\"", STRING)
	  ("[a-zA-Z0-9\\*]+", IDENTIFIER)
	  ("\\(", OPENPAREN)
	  ("\\)", CLOSEPAREN)
	  ("'", QUOTE)
	  ("`", BACKQUOTE)
	  (",", COMMA)
	  (",@", COMMA_AT)
	  (";.*\\n", COMMENT)
        ;
      */
    }
  };
}

int main(int argc, char* argv[])
{
  /*
    // these variables are used to count characters, words and lines
    std::size_t c = 0, w = 0, l = 0;

    // read input from the given file
    std::string str (read_from_file(1 == argc ? "word_count.input" : argv[1]));

    // create the token definition instance needed to invoke the lexical analyzer
    word_count_tokens<lexertl::lexer<> > word_count_functor;

    // tokenize the given string, the bound functor gets invoked for each of 
    // the matched tokens
    char const* first = str.c_str();
    char const* last = &first[str.size()];
    bool r = tokenize(first, last, word_count_functor, 
        boost::bind(counter(), _1, boost::ref(c), boost::ref(w), boost::ref(l)));

    // print results
    if (r) {
        std::cout << "lines: " << l << ", words: " << w 
                  << ", characters: " << c << "\n";
    }
    else {
        std::string rest(first, last);
        std::cout << "Lexical analysis failed\n" << "stopped at: \"" 
                  << rest << "\"\n";
    }
  */
    return 0;
}
//]

