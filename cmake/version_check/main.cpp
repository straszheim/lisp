#include <boost/version.hpp>
#include <iostream>

int main(int argc, char** argv)
{
  std::cout << "Found boost version " 
	    << (int) BOOST_VERSION / 100000
	    << "."
	    << (int) BOOST_VERSION / 100 % 1000
	    << "."
	    << (int) BOOST_VERSION % 1000
	    << "\n";
  return 0;
}
