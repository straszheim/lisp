#include "types.hpp"
#include "debug.hpp"

namespace lisp {

  cons_debug::cons_debug(std::ostream& _os) 
    : os(_os) 
  { }

  void cons_debug::operator()(double d) const
  {
    os << "(double:" << d << ")";
  }
    
  void cons_debug::operator()(const std::string& s) const
  {
    os << "(string:\"" << s << "\")";
  }
    
  void cons_debug::operator()(const symbol& s) const
  {
    os << "(symbol:" << s << ")";
  }
    
  void cons_debug::operator()(const cons_ptr p) const
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

  void cons_debug::operator()(const function f) const
  {
    os << "(function@" << &f << " \"" << f.name << "\")\n";
  }

  void cons_debug::operator()(const variant v) const
  {
    boost::apply_visitor(*this, v);
  }

  
  std::ostream& operator<<(std::ostream& os,
			   const cons_ptr& cp)
  {
    cons_debug printer(os);
    printer(cp);
    return os;
  }

  std::ostream& operator<<(std::ostream& os,
			   const variant& v)
  {
    cons_debug printer(os);
    printer(v);
    return os;
  }

  void debug(const variant& v, std::ostream& os)
  {
    cons_debug dbg(os);
    dbg(v);
  }


}
