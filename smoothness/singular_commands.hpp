#include <sstream>
#include <stdexcept>
#include <string>

#include <Singular/libsingular.h>
#include <Singular/newstruct.h>

//namespace
//{
  void init_singular (std::string const& library_path);

  void call_singular (std::string const& command);

  void call_singular_and_discard (std::string const& command);

  std::string get_singular_result (std::string const& command);

  void dechain_handle (idhdl h, idhdl* ih);

  poly singular_determinant (matrix m);

  bool singular_check_proc (std::string const& proc_name);

  void singular_load_library (std::string const& library_name);

  void singular_load_ssi (std::string const& symbol_name, std::string const& file_name);

  void singular_write_ssi (std::string const& symbol_name, std::string const& file_name);

  void singular_load_rms();

  BOOLEAN idInsertPoly_inc1 (ideal h1, poly h2);

  bool is_ideal_zero (ideal id);

  lists delete_from_list (lists l, int index);

  matrix jacobi_matrix_of_ideal (ideal id, ring R);

  int draw_random_integer (const int max_abs);
//}
