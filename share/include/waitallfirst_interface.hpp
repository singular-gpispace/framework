#pragma once

#define NO_NAME_MANGLING extern "C"

#include <string>

NO_NAME_MANGLING
void singular_parallel_compute ( std::string const&
                               , std::string const&
                               , std::string const&
                               , std::string const&
                               , std::string const&
                               , std::string const&
                               , std::string const&
                               , std::string const&
                               );
