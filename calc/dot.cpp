#include "dot.hpp"

#include <boost/format.hpp>

#include <iostream>
#include <fstream>


//#define SHOW std::cerr << __PRETTY_FUNCTION__ << "\n"

#define SHOW

namespace lisp 
{
  dot::dot(std::string prefix, unsigned n)
  { 
    std::string fname = (boost::format("%s%u.dot") % prefix % n).str();
    os = new std::ofstream(fname.c_str());
    std::cout << "opened file " << fname << "\n";
    
    *os << "digraph g {";
  }

  void* dot::operator()(const double &d)
  {
    SHOW;
    *os << "\"" << &d << "\" [ label = \"double " << d << "\" ];\n";
    return (void*)&d;
  }
    
  void* dot::operator()(const std::string& s)
  {
    SHOW;
    *os << "\"" << &s << "\" [ label = \"string " << s << "\" ];\n";
    return (void*)&s;
  }
    
  void* dot::operator()(const symbol& s)
  {
    SHOW;
    *os << "\"" << &s << "\" [ label = \"symbol " << s << "\" ];\n";
    return (void*)&s;
  }
    
  void* dot::operator()(const cons_ptr& p)
  {
    SHOW;
    if (!p) 
      return (void*)0;

    void *carp = boost::apply_visitor(*this, p->car);
    void *cdrp = boost::apply_visitor(*this, p->cdr);

    *os << "\"" << &p << "\" [ label =\"<car>car|<cdr>cdr\"\n shape = record ];";
    *os << "\"" << &p << "\":car -> \"" << carp << "\"\n";
    *os << "\"" << &p << "\":cdr -> \"" << cdrp << "\"\n";
    return (void*)&p;

  }

  void* dot::operator()(const function& f)
  {
    SHOW;
    *os << "\"" << &f << "\" [ label = \"function\" ];\n";
    return (void*)&f;
  }

  void* dot::operator()(const variant& v)
  {
    SHOW;
    return boost::apply_visitor(*this, v);
  }

  dot::~dot()
  {
    *os << "};\n";
    delete os;
  }

  void dout(std::string name, const variant& v)
  {
    dot d(name, 0);
    d(v);
  }

}
