#include "types.hpp"
#include "context.hpp"
#include "print.hpp"

#include <iostream>

namespace lisp 
{
  context_ptr context::scope()
  {
    context* newctx = new context;
    context_ptr newscope(newctx);
    //    std::cout << "current ";
    //    dump(std::cout);
    context_ptr this_ptr = shared_from_this();
    assert(this_ptr.get() == this);
    newscope->next_ = this_ptr;
    //    std::cout << "new ";
    //    newscope->dump(std::cout);
    return newscope;
  }

  template <typename T>
  T& context::convert(variant& v)
  {
    return boost::get<T>(v);
  }

  template <>
  variant& 
  context::convert<variant>(variant& v)
  {
    return v;
  }

  template <typename T>
  T& context::get(const std::string& s)
  {
    //    std::cout << "looking for " << s << "\n";
    context_ptr ctx = shared_from_this();
    //    ctx->dump(std::cerr);
    while (ctx)
      {
	std::map<std::string, variant>::iterator iter = ctx->m_.find(s);
	if (iter != ctx->m_.end())
	  return convert<T>(iter->second);
	ctx = ctx->next_;
      }
    throw std::runtime_error("symbol not found");
  }

  void context::put(const std::string& s, variant v)
  {
    m_[s] = v;
  }

  template variant& context::get(const std::string&);
  template function& context::get(const std::string&);

  void context::dump(std::ostream& os) const
  {
    boost::shared_ptr<const context> ctx(shared_from_this());

    while (ctx)
      {
	std::cout << "[ ";
	for (std::map<std::string, variant>::const_iterator iter = ctx->m_.begin();
	     iter != ctx->m_.end();
	     iter++)
	  {
	    os << "\t" << iter->first << " ";
	    print(os, iter->second);
	    os << "\n";
	  }
	os << "]\n";
	ctx = ctx->next_;
      }
  }

  context_ptr global(new context);
}

