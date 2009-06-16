#include <iostream>
#include "print.hpp"

#define SHOW std::cerr << __PRETTY_FUNCTION__ << "\n"

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
    
  void cons_print::operator()(const cons_ptr p) const
  {
    SHOW;
    if (!p) 
      {
	os << "NIL";
	return;
      }
    bool branch = is_ptr(p->car) && !is_nil(p->car) && is_ptr(p->cdr);
    if (branch)
      os << "(";
    boost::apply_visitor(*this, p->car);
    if (branch)
      os << ")";
    bool recur = false;
    if (is_ptr(p->cdr) && !is_nil(p->cdr))
      {
	os << " ";
	boost::apply_visitor(*this, p->cdr);
      }
    if (!is_ptr(p->cdr))
      {
	os << " . ";
	boost::apply_visitor(*this, p->cdr);
      }
  }

  void cons_print::operator()(const function f) const
  {
    SHOW;
    os << "function@" << &f << "\n";
  }

  void cons_print::operator()(const variant v) const
  {
    SHOW;
    if (is_ptr(v) && is_nil(v)) 
      {
	os << "NIL";
	return;
      }
    bool branch = is_ptr(v);
    if (branch)
      os << "(";
    boost::apply_visitor(*this, v);
    if (branch)
      os << ")";
  }
}
