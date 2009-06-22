//
// Copyright Troy D. Straszheim 2009
//
// Distributed under the Boost Software License, Version 1.0
// See http://www.boost.org/LICENSE_1.0.txt
//

#ifndef LISP_CONFIG_HPP_INCLUDED
#define LISP_CONFIG_HPP_INCLUDED

#define DEBUG 0

#if DEBUG

#include <iostream>
#define SHOW std::cerr << __PRETTY_FUNCTION__ << "\n"

#else

#define SHOW 

#endif

#endif
