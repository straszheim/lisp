
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <string>
#include <iostream>
#include <stdexcept>
#include <boost/fusion/container/generation.hpp>
#include <boost/fusion/include/transform.hpp>
#include <boost/fusion/include/for_each.hpp>
#include "interpreter.hpp"

namespace ft = boost::function_types;

void echo(std::string s)
{
  std::cout << s << std::endl;
}

void add(int a, int b)
{
  std::cout << a + b << std::endl;
}

void repeat(std::string s, int n)
{
  while (--n >= 0) std::cout << s;
  std::cout << std::endl; 
}

struct printy
{
  typedef void result_type;

  template <typename T>
  struct result
  {
    typedef void type;
  };

  template <typename T>
  void operator()(T t) const
  {
    std::cout << "T=" << t << "\n";
  }
};

template <typename Sig>
struct doit
{
  typedef ft::parameter_types<Sig> blahbla;

  void operator()()
  {
    std::cout << __PRETTY_FUNCTION__ << "\n";
  }
};


typedef boost::variant<int, double, std::string> variant_t;

template <typename Function>
struct blang
{
  typedef typename ft::parameter_types<Function> params_t;

  struct checker
  {
    template <typename T>
    void operator()(T) const
    { 
      std::cout << __PRETTY_FUNCTION__ << "\n"; 
    }
  };

  checker check;

  blang() 
  { 
    std::cout << __PRETTY_FUNCTION__ << "\n";
    fusion::for_each(params_t(), check);
  }

};
	  


int main()
{

  doit<void(int, double, std::string)> d;
  d();

  blang<void(bool, short, int, float, double)> b; 

  return 0;
}

