#include <boost/intrusive_ptr.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix1_functions.hpp>

#include <iostream>
#include <string>

namespace client
{
  namespace qi = boost::spirit::qi;
  namespace phoenix = boost::phoenix;
  namespace ascii = boost::spirit::ascii;

  struct nil {};

  struct cons;

  typedef boost::intrusive_ptr<cons> cons_ptr;

  struct cons
  {
    unsigned count;

    typedef boost::variant<
      std::string,
      int,
      double,
      cons_ptr
      >
    type;

    type car, cdr;
    
    cons() : count(0),
	     car(cons_ptr(0)),
	     cdr(cons_ptr(0))
    { 
      std::cout << "  Creating cons @ " << this << "\n";
    }

    ~cons()
    {
      std::cout << "destroying cons @ " << this << "\n";
    }
    
  };

  void intrusive_ptr_add_ref(cons* c)
  {
    std::cout << "inc cons @ " << c << "\n";
    c->count++;
  }
  
  void intrusive_ptr_release(cons* c)
  {
    std::cout << "dec cons @ " << c << "\n";
    c->count--;
    if (c->count == 0)
      delete c;
  }
  
  struct process
  {
    template <typename T>
    struct result
    {
      typedef cons_ptr type;
    };

    cons_ptr operator()(double d) const
    {
      cons_ptr c = new cons;
      c->car = d;
      return c;
    }
    
    cons_ptr operator()(int i) const
    {
      cons_ptr c = new cons;
      c->car = i;
      return c;
    }

    cons_ptr operator()(boost::iterator_range<std::string::const_iterator> r) const
    {
      std::string s(r.begin(), r.end());
      cons_ptr c = new cons;
      c->car = s;
      std::cout << "returning cons @ " << c.get() << "\n";
      return c;
    }

    cons_ptr operator()(const std::vector<cons_ptr>& v) const
    {
      cons_ptr c = new cons;
      // chain them together
      for (int i=0; i<v.size()-1; i++)
	v[i]->cdr = v[i+1];
      // point our cons at it
      c->car = v[0];
      return c;
    }

  };

  struct cons_print
  {
    typedef void result_type;
    
    std::ostream& os;

    cons_print(std::ostream& _os) : os(_os) { }

    void operator()(double d)
    {
      os << d << " ";
    }
    
    void operator()(int i)
    {
      os << i << " ";
    }
    
    void operator()(const std::string& s)
    {
      os << "'" << s << "' ";
    }
    
    void operator()(const cons_ptr p)
    {
      os << "ptr:" << p.get() << "\n";
      if (!p) return;
      boost::apply_visitor(*this, p->car);
      boost::apply_visitor(*this, p->cdr);
    }
  };
  
  std::ostream& operator<<(std::ostream& os,
			   cons_ptr cp)
  {
    cons_print printer(os);
    printer(cp);
    return os;
  }

  
  ///////////////////////////////////////////////////////////////////////////////
  //  Our calculator grammar
  ///////////////////////////////////////////////////////////////////////////////
  template <typename Iterator>
  struct calculator : qi::grammar<Iterator, cons_ptr(), ascii::space_type>
  {
    boost::phoenix::function<client::process> p;

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
      

      atom = 
	(int_ >> !char_('.'))[_val = p(_1)]
	| double_[_val = p(_1)]
	| raw[lexeme[alpha >> *alnum >> !alnum]][_val = p(_1)]
	;
      
      sexpr = atom[_val = _1] 
	| (char_("(") >> (*sexpr)[_val = p(_1)] >> char_(")"))
	;
      
      atom.name("atom");
      sexpr.name("sexpr");
      
      on_error<fail>
	(
	 sexpr
	 , std::cout
	 << val("Error! Expecting ")
	 << _4                               // what failed?
	 << val(" here: \"")
	 << construct<std::string>(_3, _2)   // iterators to error-pos, end
	 << val("\"")
	 << std::endl
	 );
      
      debug(atom);
      debug(sexpr);
    }
    
    qi::rule<Iterator, 
	     cons_ptr(), 
	     ascii::space_type> 
    atom, sexpr;
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
  
  client::cons_ptr a, b;
  a = new client::cons; b = new client::cons;
  a->cdr = b;
  a->car = 3.14159;
  b->cdr = std::string("SCHWING");
  client::cons_print printer(std::cout);
  printer(a);
  std::cout << "----------------------------\n";

  while (std::getline(std::cin, str))
    {
      if (str.empty() || str[0] == 'q' || str[0] == 'Q')
	break;
      
      std::string::const_iterator iter = str.begin();
      std::string::const_iterator end = str.end();
      bool r = phrase_parse(iter, end, calc, space);
      
      if (r && iter == end)
        {
	  std::cout << "-------------------------\n";
	  std::cout << "Parsing succeeded\n";
	  client::cons_print printer(std::cout);
	  //r	  printer(bang);
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


