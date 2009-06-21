#ifndef LISP_CONFIG_HPP_INCLUDED
#define LISP_CONFIG_HPP_INCLUDED

#ifdef DEBUG

#include <iostream>
#define SHOW std::cerr << __PRETTY_FUNCTION__ << "\n"

#else

#define SHOW 

#endif

#endif
