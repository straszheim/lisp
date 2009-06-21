#include <iostream>
#include "print.hpp"
#include "config.hpp"

namespace lisp {

  cons_print::cons_print(std::ostream& _os)
    : os(_os)
  { }

  void cons_print::operator()(double d) const
  {
    SHOW;
    os << d;
  }
    
  void cons_print::operator()(const std::string& s) const
  {
    SHOW;
    os << "\"" << s << "\"";
  }
    
  void cons_print::operator()(const symbol& s) const
  {
    SHOW;
    os << s;
  }
    
  void cons_print::operator()(const cons_ptr& p) const
  {
    SHOW;
    if (!p) 
      {
	os << "NIL";
	return;
      }
    os << "(";
    boost::apply_visitor(*this, p->car);
    cons_ptr k = p;

    while(! is_nil(k->cdr))
      {
	os << " ";
	if (is_ptr(k->cdr))
	  {
	    k = boost::get<cons_ptr>(k->cdr);
	    boost::apply_visitor(*this, k->car);
	  }
	else
	  {
	    os << ". ";
	    boost::apply_visitor(*this, k->cdr);
	    break;
	  }
      }
    os << ")";
  }

  void cons_print::operator()(const function& f) const
  {
    SHOW;
    os << "function@" << &f;
  }

  void cons_print::operator()(const special<backquoted_>& s) const
  {
    os << "`";
    boost::apply_visitor(*this, s.v);
  }

  void cons_print::operator()(const special<quoted_>& s) const
  {
    os << "'";
    boost::apply_visitor(*this, s.v);
  }
  void cons_print::operator()(const special<comma_at_>& s) const
  {
    os << ",@";
    boost::apply_visitor(*this, s.v);
  }
  void cons_print::operator()(const special<comma_>& s) const
  {
    os << ",";
    boost::apply_visitor(*this, s.v);
  }

  void print(std::ostream& os, const variant& v)
  {
    cons_print visitor(os);
    boost::apply_visitor(visitor, v);
  }
}

/*

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/home/phoenix/core/value.hpp>

using namespace boost::spirit;
using namespace boost::spirit::ascii;
using namespace boost::spirit::karma;
using namespace boost::phoenix;

namespace lisp 
{
  typedef boost::variant<int, double> myv_t;


  template <typename T>
  struct get_element
  {
    template <typename T1>
    struct result { typedef T const& type; };

    T const& operator()(variant const& v) const
    {
      return boost::get<T>(v);
    }
  };

  boost::phoenix::function<get_element<double> > _double;
  boost::phoenix::function<get_element<std::string> >    _string;

  template <typename OutputIterator>
  struct dumper
    : karma::grammar<OutputIterator, variant(), space_type>
  {
    dumper() : dumper::base_type(start)
    {
      start = 
	double_ [ _1 = _double(_val) ]
	| str [ _1 = _string(_val) ]
	;
    }
    karma::rule<OutputIterator, variant(), space_type> start;
  };

  void flam()
  {
    variant v = 3.14159;

    boost::variant<int, double> myv;
    myv = 777.014;

    typedef std::back_insert_iterator<std::string> output_iterator_type;
    typedef dumper<output_iterator_type> dump_tree;
    dump_tree dump_grammar;
    std::string generated;

    output_iterator_type outit(generated);

    bool r = karma::generate_delimited(outit, dump_grammar, space, v);

    std::cout << ">>>" << r << "    " << generated << "<<<";

    std::cout << karma::format(int_ [ _1 = val(666)] | double_, myv) << "\n";


  }

}

*/
