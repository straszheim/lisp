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
    bool branch = is_ptr(p->car) && !is_nil(p->car) && is_ptr(p->cdr);
    if (branch)
      os << "(";
    boost::apply_visitor(*this, p->car);
    if (branch)
      os << ")";
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

  void cons_print::operator()(const function& f) const
  {
    SHOW;
    os << "function@" << &f;
  }

  void cons_print::operator()(const special<backquoted_>& s) const
  {
    os << "`";
    bool branch = is_ptr(s.v);
    if (branch)
      os << "(";
    boost::apply_visitor(*this, s.v);
    if (branch)
      os << ")";
  }

  void cons_print::operator()(const special<quoted_>& s) const
  {
    os << "'";
    bool branch = is_ptr(s.v);
    if (branch)
      os << "(";
    boost::apply_visitor(*this, s.v);
    if (branch)
      os << ")";
  }
  void cons_print::operator()(const special<comma_at_>& s) const
  {
    os << ",@";
    bool branch = is_ptr(s.v);
    if (branch)
      os << "(";
    boost::apply_visitor(*this, s.v);
    if (branch)
      os << ")";
  }
  void cons_print::operator()(const special<comma_>& s) const
  {
    os << ",";
    bool branch = is_ptr(s.v);
    if (branch)
      os << "(";
    boost::apply_visitor(*this, s.v);
    if (branch)
      os << ")";
  }

  void print(std::ostream& os, const variant& v)
  {
    cons_print visitor(os);
    boost::apply_visitor(visitor, v);
  }

}
