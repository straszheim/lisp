//
// Copyright Troy D. Straszheim 2009
//
// Distributed under the Boost Software License, Version 1.0
// See http://www.boost.org/LICENSE_1.0.txt
//

#include <boost/intrusive_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix1_functions.hpp>

#include <iostream>
#include <string>
#include <map>

typedef boost::variant<int, double> variant_t;

struct plus
{
  int operator()(int i1, int i2) const
  {
    return i1+i2;
  }

  double operator()(int i, double d) const
  {
    return i+d;
  }

  double operator()(double d, int i) const
  {
    return d+i;
  }

  double operator()(double d1, double d2) const
  {
    return d+d;
  }
};

template <typename To>
struct dispatch
{
  //  template <typename T>
  

};

template <typename T, typename U, typename Op>
void double_dispatch

int
main()
{
  variant_t i, d;
  i = 3;
  d = 1.4159;

  double_dispatch(plus(), i, d);
  

  return 0;
}


