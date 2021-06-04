#pragma once

#define NO_NAME_MANGLING extern "C"

#include <string>

NO_NAME_MANGLING
void singular_parallel_all_compute ( std::string const&
                                   , unsigned int const&
                                   , std::string const&
                                   , std::string const&
                                   , std::string const&
                                   , std::string const&
                                   , std::string const&
                                   , std::string const&
                                   );
