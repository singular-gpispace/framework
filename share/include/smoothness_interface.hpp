#pragma once

#define NO_NAME_MANGLING extern "C"

#include <map>
#include <string>
#include <list>

#include <gen_smoothness.pnet/pnetc/type/smoothness_task.hpp>
#include <gen_smoothness.pnet/pnetc/type/jacobi_task.hpp>
#include <gen_smoothness.pnet/pnetc/type/jacobi_list.hpp>

NO_NAME_MANGLING
void singular_smoothness_trivial ( pnetc::type::smoothness_task::smoothness_task&
                                 , int const&
                                 );

NO_NAME_MANGLING
void singular_smoothness_delta ( pnetc::type::smoothness_task::smoothness_task&
                               , std::string const&
                               , int const&
                               );

NO_NAME_MANGLING
void singular_smoothness_jacobi ( pnetc::type::jacobi_task::jacobi_task&
                                , std::string const&
                                , int const&
                                );

NO_NAME_MANGLING
void singular_smoothness_descent ( pnetc::type::smoothness_task::smoothness_task const&
                                 , std::list<pnetc::type::smoothness_task::smoothness_task>&
                                 , std::string const&
                                 , int const&
                                 , unsigned long const&
                                 , unsigned long const&
                                 , int const&
                                 );

NO_NAME_MANGLING
void singular_smoothness_init ( std::string const&
                              , bool const&
                              , std::list<pnetc::type::smoothness_task::smoothness_task>&
                              , std::map<int, unsigned long>&
                              , std::string&
                              , std::string&
                              , int const&
                              , int const&
                              );

NO_NAME_MANGLING
void singular_smoothness_jacobisplit ( pnetc::type::smoothness_task::smoothness_task const&
                                     , pnetc::type::jacobi_list::jacobi_list&
                                     , unsigned long const&
                                     , int const&
                                     );

NO_NAME_MANGLING
void singular_smoothness_checkcover ( std::string const&
                                    , int const&
                                    , bool&
                                    , int const&
                                    );
