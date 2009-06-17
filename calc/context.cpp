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
    std::cout << "current ";
    dump(std::cout);
    context_ptr this_ptr = shared_from_this();
    assert(this_ptr.get() == this);
    newscope->next = this_ptr;
    std::cout << "new ";
    newscope->dump(std::cout);
    return newscope;
  }

  function context::get_function(const std::string& s)
  {
    std::cout << "looking for " << s << "\n";
    context_ptr ctx = shared_from_this();
    ctx->dump(std::cerr);
    while (ctx)
      {
	std::map<std::string, function>::iterator iter = ctx->fns.find(s);
	if (iter != ctx->fns.end())
	  return iter->second;
	ctx = ctx->next;
      }
    throw std::runtime_error("function not found");
  }

  variant context::get_variable(const std::string& s)
  {
    std::cout << "looking for " << s << "\n";
    context_ptr ctx = shared_from_this();
    ctx->dump(std::cerr);
    while (ctx)
      {
	std::map<std::string, variant>::iterator iter = ctx->table.find(s);
	if (iter != ctx->table.end())
	  return iter->second;
	ctx = ctx->next;
      }
    throw std::runtime_error("variable not found");
  }

  void context::dump(std::ostream& os) const
  {
    boost::shared_ptr<const context> ctx(shared_from_this());

    while (ctx)
      {
	std::cout << "[fns ";
	for (std::map<std::string, function>::const_iterator iter = ctx->fns.begin();
	     iter != ctx->fns.end();
	     iter++)
	  {
	    os << iter->first << (!iter->second ? "!!!  " : "  ");
	  }
	os << "]\n[vars ";
	for (std::map<std::string, variant>::const_iterator iter = ctx->table.begin();
	     iter != ctx->table.end();
	     iter++)
	  {
	    os << "\t" << iter->first << " ";
	    cons_print p(os);
	    p(iter->second);
	    os << "\n";
	  }
	os << "]\n";
	ctx = ctx->next;
      }
  }

  context_ptr global(new context);
}

