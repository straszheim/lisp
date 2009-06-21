#ifndef LISP_GRAMMAR_HPP_INCLUDED
#define LISP_GRAMMAR_HPP_INCLUDED

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

using namespace boost::spirit;
using namespace boost::spirit::qi;
using namespace boost::spirit::ascii;

namespace lisp
{
  namespace ascii = boost::spirit::ascii;

  struct error_handler_
  {
    template <typename, typename, typename>
    struct result { typedef void type; };

    template <typename Iterator>
    void operator()(info const& what, Iterator err_pos, Iterator last) const;
  };

  template <typename Iterator>
  struct white_space : boost::spirit::qi::grammar<Iterator>
  {
    white_space();

    rule<Iterator> start;
  };

  template <typename Iterator>
  struct interpreter 
    : grammar<Iterator, variant(), white_space<Iterator> >
  {
    bool show_debug;
    interpreter(bool _show_debug);
    
    qi::rule<Iterator, variant(), white_space<Iterator> > 
    start, atom, sexpr, nil, identifier, /*quote, */cons, quoted_string;

    qi::rule<Iterator, char()> escaped_char;
  };

  typedef std::string::const_iterator iterator_type;
  typedef lisp::interpreter<iterator_type> interpreter_t;
  typedef lisp::white_space<iterator_type> skipper_t;
}

#endif
