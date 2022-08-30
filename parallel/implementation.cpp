#include <Singular/libsingular.h>

#include <share/include/waitallfirst_interface.hpp>

#include <iostream>
#include <stdexcept>
#include <unistd.h>

#include "config.hpp"
#include "singular_functions.hpp"

NO_NAME_MANGLING
void singular_parallel_compute ( std::string const& function_name
                               , std::string const& needed_library
                               , std::string const& in_filename
                               , std::string const& out_filename
                               , std::string const& in_struct_name
                               , std::string const& in_struct_desc
                               , std::string const& out_struct_name
                               , std::string const& out_struct_desc
                               )
{
  char hstn[65];
  gethostname (hstn, 64);
  hstn[64] = '\0';
  std::string ids (hstn);
  ids = ids + " " + std::to_string (getpid());
  //std::cout << ids << " in singular_..._compute" << std::endl;
  init_singular (config::library().string());

  load_singular_library (needed_library);

  if (!(register_struct (in_struct_name, in_struct_desc) &&
    register_struct (out_struct_name, out_struct_desc)))
  {
    throw std::runtime_error
      (ids + ": singular_parallel_all_compute: could not register structs");
  }
  int in_type;
  blackboxIsCmd (in_struct_name.c_str(), in_type);
  int out_type;
  blackboxIsCmd (out_struct_name.c_str(), out_type);

  si_link l = ssi_open_for_read (in_filename);
  lists in_lst = ssi_read_newstruct (l, in_struct_name);
  ssi_close_and_remove (l);

  std::pair<int, lists> out = call_user_proc
    (function_name, needed_library, in_type, in_lst);

  if (out.first != out_type)
  {
    throw std::runtime_error (ids + ": singular_parallel_all_compute: wrong type in "
    + out_filename + " from " + in_filename + ", expected " + std::to_string (out_type)
    + " got " + std::to_string (out.first));
  }

  l = ssi_open_for_write (out_filename);
  ssi_write_newstruct (l, out_struct_name, out.second);
  ssi_close_and_remove (l);
  //std::cout << ids << ": A" << std::endl;
  //in_lst->Clean(); // TODO needs repairing
  //std::cout << ids << ": B" << std::endl;
  //out.second->Clean(); // TODO needs repairing
  //std::cout << ids << ": end of singular_..._compute" << std::endl;
}
