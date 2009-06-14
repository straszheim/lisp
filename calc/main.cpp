#include <boost/intrusive_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix1_functions.hpp>
#include <boost/function.hpp>

#include <iostream>
#include <string>
#include <map>

namespace lisp
{
  namespace qi = boost::spirit::qi;
  namespace phoenix = boost::phoenix;
  namespace ascii = boost::spirit::ascii;

  struct nil {};

  struct symbol : std::string
  {
    symbol(const std::string& s) : std::string(s) { }
  };

  struct cons;
  typedef boost::intrusive_ptr<cons> cons_ptr;

  struct context;
  typedef boost::shared_ptr<context> context_ptr;

  struct function;

  typedef boost::variant<double,
			 std::string,
			 symbol,
			 boost::recursive_wrapper<function>,
			 cons_ptr>
  variant;

  struct function 
  { 
    typedef variant result_type;
    typedef boost::function<variant(context_ptr, cons_ptr)> bf_t;
    bf_t f;

    function(bf_t _f) : f(_f) { }

    variant operator()(context_ptr ctx, cons_ptr cns)
    {
      return f(ctx, cns);
    }
  };

  struct cons 
  {
    unsigned count;

    variant car, cdr;
    
    cons() : count(0),
	     car(cons_ptr(0)),
	     cdr(cons_ptr(0))
    { }

    template <typename T>
    cons(T t) : count(0),
		car(t),
		cdr(cons_ptr(0))
    { }

    ~cons()
    {
      //      std::cout << "destroying cons @ " << this << "\n";
    }
    
  };

  void intrusive_ptr_add_ref(cons* c)
  {
    //    std::cout << "inc cons @ " << c << "\n";
    c->count++;
  }
  
  void intrusive_ptr_release(cons* c)
  {
    //    std::cout << "dec cons @ " << c << "\n";
    c->count--;
    if (c->count == 0)
      delete c;
  }
  
  bool is_nil(const variant& v)
  {
    const cons_ptr p = boost::get<cons_ptr>(v);
    return !p;
  }

  bool is_ptr(const variant& v)
  {
    return boost::get<cons_ptr>(&v);
  }

  struct process
  {
    template <typename T>
    struct result
    {
      typedef variant type;
    };

    variant operator()(double d) const
    {
      return d;
    }
    
    variant operator()(boost::iterator_range<std::string::const_iterator> r) const
    {
      return symbol(std::string(r.begin(), r.end()));
    }

    variant operator()(const std::vector<variant>& v) const
    {
      std::cout << "**** BING " << v.size() << "\n";
      cons_ptr head = new cons;
      cons_ptr tmp = head;
      // chain them together
      if (v.size() == 0)
	return head;

      tmp->car = v[0];
      for (unsigned i=1; i<v.size(); i++)
	{
	  cons_ptr tail = new cons;
	  tmp->cdr = tail;
	  tmp = tail;
	  tmp->car = v[i];
	}
      return head;
    }

  };

  struct parens
  {
    std::ostream& os;
    parens(std::ostream& _os) : os(_os) { os << "("; }
    ~parens() { os << ")"; }
  };

  struct cons_debug
  {
    typedef void result_type;
    
    std::ostream& os;

    cons_debug(std::ostream& _os) 
      : os(_os) 
    { }

    void operator()(double d)
    {
      os << "(double:" << d << ")";
    }
    
    void operator()(const std::string& s)
    {
      os << "\"" << s << "\"";
    }
    
    void operator()(const symbol& s)
    {
      os << "symbol:" << s;
    }
    
    void operator()(const cons_ptr p)
    {
      if (!p)
	{
	  os << "NIL";
	  return;
	}
      os << "(cons @" << p.get() << " car:";
      boost::apply_visitor(*this, p->car);

      os << " cdr:";
      boost::apply_visitor(*this, p->cdr);

      os << ")";
    }

    void operator()(const function f)
    {
      os << "function@" << &f << "\n";
    }

    void operator()(const variant v)
    {
      boost::apply_visitor(*this, v);
    }
  };
  
  std::ostream& operator<<(std::ostream& os,
			   cons_ptr cp)
  {
    cons_debug printer(os);
    printer(cp);
    return os;
  }

  std::ostream& operator<<(std::ostream& os,
			   variant v)
  {
    cons_debug printer(os);
    printer(v);
    return os;
  }

  struct cons_print
  {
    typedef void result_type;
    
    std::ostream& os;

    cons_print(std::ostream& _os) 
      : os(_os) 
    { }

    void operator()(double d)
    {
      os << d;
    }
    
    void operator()(const std::string& s)
    {
      os << "\"" << s << "\"";
    }
    
    void operator()(const symbol& s)
    {
      os << s;
    }
    
    void operator()(const cons_ptr p)
    {
      if (!p) return;

      bool branch = is_ptr(p->car) && is_ptr(p->cdr);
      if (branch)
	os << "(";
      boost::apply_visitor(*this, p->car);
      if (branch)
	os << ")";
      if (is_ptr(p->cdr) && !is_nil(p->cdr))
	os << " ";
      boost::apply_visitor(*this, p->cdr);

    }

    void operator()(const function f)
    {
      os << "function@" << &f << "\n";
    }

    void operator()(const variant v)
    {
      bool branch = is_ptr(v);
      if (branch)
	os << "(";
      boost::apply_visitor(*this, v);
      if (branch)
	os << ")";
    }
  };
  
  template <typename Iterator>
  struct interpreter 
    : qi::grammar<Iterator, variant(), ascii::space_type>
  {
    boost::phoenix::function<lisp::process> p;

    interpreter() : interpreter::base_type(sexpr)
    {
      using namespace qi::labels;
      using ascii::alpha;
      using ascii::alnum;
      using ascii::char_;
      using qi::double_;
      using qi::int_;
      using qi::uint_;
      using qi::lit;
      using qi::on_error;
      using qi::fail;
      using qi::debug;
      using boost::spirit::lexeme;
      using boost::spirit::raw;
      
      using phoenix::construct;
      using phoenix::val;
      
      // reserved = lit("setq")[_val 
      //   = p(val(std::string("SETQ")))]
      //;

      atom = 
	//(int_ >> !char_('.'))[_val = p(_1)]
	double_[_val = p(_1)]
	| raw[lexeme[alpha >> *alnum >> !alnum]][_val = p(_1)]
      	;
      
      sexpr = // reserved[_val = _1] | 
	atom[_val = _1] 
	| (char_("(") >> (*sexpr)[_val = p(_1)] >> char_(")"))
	;
      
      atom.name("atom");
      sexpr.name("sexpr");
      //      reserved.name("reserved");
      
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
    
    qi::rule<Iterator, variant(), ascii::space_type> 
    atom, sexpr; //, reserved;
  };

  //  std::hash_map<std::string, cons_ptr> table;

  struct eval
  {
    typedef variant result_type;

    context_ptr ctx;
    eval(context_ptr _ctx) : ctx(_ctx) { }

    variant operator()(double d)
    {
      return d;
    }
    
    variant operator()(variant v)
    {
      eval eprime(ctx);
      boost::apply_visitor(eprime, v);
    }
    
    variant operator()(const std::string& s)
    {
      return s;
    }
    
    variant operator()(const symbol& s)
    {
      return s;
    }
    
    variant operator()(const function& p)
    {
      /*
      if (!p) 
	return;
      if (boost::get<cons_ptr>(&p->car))
	{
	  os << "(";
	}
      boost::apply_visitor(*this, p->car);
      if (boost::get<cons_ptr>(&p->car))
	os << ")";
      if (!cons::nil(p->cdr))
	os << " ";
      boost::apply_visitor(*this, p->cdr);
      */
    }

    variant operator()(const cons_ptr& p)
    {
      function* f = boost::get<function>(&p->car);
      if (f)
	{
	  cons_ptr args = boost::get<cons_ptr>(p->cdr);
	  (*f)(ctx, args);
	}
      return p;
    }
  };
  
  struct context : boost::enable_shared_from_this<context>
  {
    std::map<std::string, variant> table;
    context_ptr next;

    context_ptr scope()
    {
      context_ptr ctx(new context);
      ctx->next = shared_from_this();
    }

  };

  context_ptr global;

  struct plus
  {
    cons_ptr operator()(context_ptr c, cons_ptr l)
    {
      double sum = 0;
      while(l)
	{
	  double& d = boost::get<double>(l->car);
	  sum += d;
	  l = boost::get<cons_ptr>(l->cdr);
	}
      return new cons(sum);
    }
  };

  
}

using namespace lisp;

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
  
  global = context_ptr(new context);
  global->table["add"] = function(plus());

  using boost::spirit::ascii::space;
  typedef std::string::const_iterator iterator_type;
  typedef lisp::interpreter<iterator_type> interpreter_t;
  
  interpreter_t lispi; // Our grammar
  
  std::string str;
  
  std::cout << "----------------------------\n> ";

  while (std::getline(std::cin, str))
    {
      std::string::const_iterator iter = str.begin();
      std::string::const_iterator end = str.end();
      lisp::variant result;
      bool r = phrase_parse(iter, end, lispi, space, result);

      if (r && iter == end)
        {
	  std::cout << "-------------------------\n";
	  std::cout << "Parsing succeeded: ";
	  cons_ptr c = new cons(result);

	  lisp::cons_debug printer(std::cout);
	  printer(result);
	  std::cout << "\n" << str << "\n";
	  lisp::cons_print pretty(std::cout);
	  pretty(result);
	  //	  try {
	  //	    eval e(global);
	  //	    variant out = e(result);
	  //	    printer(out);
	  //	  } catch (const std::exception& e) {
	  //	    std::cout << "*** - EVAL exception caught: " << e.what() << "\n";
	  //	  }
	  std::cout << "\n-------------------------\n";
        }
      else
        {
	  std::cout << "-------------------------\n";
	  std::cout << "Parsing failed\n";
	  std::cout << "-------------------------\n";
        }
      std::cout << "> ";
    }
  
  return 0;
}


