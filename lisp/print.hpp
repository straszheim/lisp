#ifndef LISP_PRINT_HPP_INCLUDED
#define LISP_PRINT_HPP_INCLUDED

#include "types.hpp"

namespace lisp {

  struct cons_print
  {
    typedef void result_type;
    std::ostream& os;

    cons_print(std::ostream& _os);

    void operator()(double d) const;
    void operator()(const std::string& s) const;
    void operator()(const symbol& s) const;
    void operator()(const cons_ptr& p) const;
    void operator()(const function& f) const;
    void operator()(const special<backquoted_>& s) const;
    void operator()(const special<quoted_>& s) const;
    void operator()(const special<comma_at_>& s) const;
    void operator()(const special<comma_>& s) const;

  private:

    template <typename T>
    void 
    visit(T const& t) const;
  };

  void print(std::ostream&, const variant&);
}

#endif
