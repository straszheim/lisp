#include <boost/program_options.hpp>

#include <iostream>

namespace opts = boost::program_options;

int
main(int argc, char* argv[])
{
  opts::options_description desc("Yes options work.");
  std::cout << desc << "\n";
  return 0;
}
