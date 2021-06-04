#include <string>
#include <vector>
#include <optional>

#include <installation.hpp>

#include <drts/client.hpp>
#include <drts/drts.hpp>
#include <drts/scoped_rifd.hpp>

#include <util-generic/executable_path.hpp>
#include <util-generic/print_exception.hpp>
#include <util-generic/read_lines.hpp>

#include "Singular/libsingular.h"

#include "../../parallel/singular_functions.hpp"

namespace
{
  template<std::size_t arg_index>
  leftv to_nth_arg (leftv args)
  {
    for (std::size_t index (arg_index); args && index > 0; --index)
    {
      args = args->next;
    }

    return args ? args : throw std::invalid_argument ("too few arguments");
  }

  template<std::size_t arg_index, typename T>
  T require_argument (leftv args, int type, std::string type_name,
    std::string argument_name)
  try
  {
    leftv arg (to_nth_arg<arg_index> (args));
    if (arg->Typ() != type)
    {
      throw std::invalid_argument ("expected " + type_name);
    }
    return reinterpret_cast<T> (arg->Data());
  }
  catch (...)
  {
    std::throw_with_nested
      ( std::invalid_argument
         ( "argument " + std::to_string (arg_index)
         + " '" + argument_name + "'"
         )
      );
  }

  template<std::size_t first_optional>
    std::vector<std::string> remaining_string_arguments (leftv args)
  {
    std::vector<std::string> strings;

    leftv it (to_nth_arg<first_optional - 1> (args)->next);
    while (it)
    {
      if (it->Typ() != STRING_CMD)
      {
        throw std::invalid_argument ("expected string");
      }
      strings.emplace_back (reinterpret_cast<char*> (it->Data()));
      //std::cout << "extra arg: " << strings.back() << "\n";

      it = it->next;
    }

    return strings;
  }

  template<std::size_t arg_index, typename T>
    T nth_list_arg (lists l)
  {
    return reinterpret_cast<T> (l->m[arg_index].data);
  }

  class ArgumentState {
    public:
      ArgumentState (leftv args);

      int outToken() const;
      int procsPerNode() const;
      std::size_t numTasks() const;

      std::string tmpDir() const;
      std::string nodeFile() const;
      std::string showStrategy() const;
      std::string baseFileName() const;
      std::string inStructName() const;
      std::string inStructDesc() const;
      std::string outStructName() const;
      std::string outStructDesc() const;
      std::string neededLibrary() const;
      std::string functionName() const;

      singular_parallel::installation singPI() const;
      lists argList() const;
      lists addArgsList() const;
    private:
      lists arg_list;
      lists addargs_list;

      std::size_t num_tasks;
      int procspernode;

      std::string tmpdir;
      std::string nodefile;
      std::string strategy;

      std::string instructname;
      std::string instructdesc;
      std::string outstructname;
      std::string outstructdesc;
      std::string neededlibrary;
      std::string functionname;
      int out_token;
      std::string base_filename;

      singular_parallel::installation singular_parallel_installation;
  };


  int ArgumentState::outToken() const {
    return out_token;
  }

  int ArgumentState::procsPerNode() const {
    return procspernode;
  }

  std::size_t ArgumentState::numTasks() const {
    return num_tasks;
  }

  /*
  std::string ArgumentState::tmpDir() const {
    return tmpdir;
  }
  */

  std::string ArgumentState::nodeFile() const {
    return nodefile;
  }

  std::string ArgumentState::showStrategy() const {
    return strategy;
  }

  std::string ArgumentState::baseFileName() const {
    return base_filename;
  }

  std::string ArgumentState::inStructName() const {
    return instructname;
  }

  std::string ArgumentState::inStructDesc() const {
    return instructdesc;
  }

  std::string ArgumentState::outStructName() const {
    return outstructname;
  }

  std::string ArgumentState::outStructDesc() const {
    return outstructdesc;
  }

  std::string ArgumentState::neededLibrary() const {
    return neededlibrary;
  }

  std::string ArgumentState::functionName() const {
    return functionname;
  }

  singular_parallel::installation ArgumentState::singPI() const {
    return singular_parallel_installation;
  }

  lists ArgumentState::argList() const {
    return arg_list;
  }

  lists ArgumentState::addArgsList() const {
    return addargs_list;
  }

  int fetch_token_value_from_sing_scope (std::string token_s) {
    int token_v;
    blackboxIsCmd (token_s.c_str(), token_v);
    return token_v;
  }

  ArgumentState::ArgumentState (leftv args)
  : arg_list (require_argument<0, lists> (args,
                                          LIST_CMD,
                                          "list",
                                          "list of input structs"))
  , addargs_list (require_argument<5, lists> (args,
                                              LIST_CMD,
                                              "list",
                                              "additional arguments"))
  , num_tasks (arg_list->nr + 1)
  , procspernode (require_argument<3, long> (args,
                                            INT_CMD,
                                            "int",
                                            "processes per node"))
  , tmpdir (require_argument<1, char*> (args,
                                        STRING_CMD,
                                        "string",
                                        "temp directory"))
  , nodefile (require_argument<2, char*> (args,
                                          STRING_CMD,
                                          "string",
                                          "nodefile"))
  , strategy (require_argument<4, char*> (args,
                                          STRING_CMD,
                                          "string",
                                          "rif strategy"))
  , instructname (require_argument<6, char*> (args,
                                              STRING_CMD,
                                              "string",
                                              "input struct name"))
  , instructdesc (require_argument<7, char*> (args,
                                              STRING_CMD,
                                              "string",
                                              "input struct description"))
  , outstructname (require_argument<8, char*> (args,
                                               STRING_CMD,
                                               "string",
                                               "output struct name"))
  , outstructdesc (require_argument<9, char*> (args,
                                               STRING_CMD,
                                               "string",
                                               "output struct description"))
  , neededlibrary (require_argument<10, char*> (args,
                                                STRING_CMD,
                                                "string",
                                                "needed library"))
  , functionname (require_argument<11, char*> (args,
                                               STRING_CMD,
                                               "string",
                                               "function name"))
  , out_token (fetch_token_value_from_sing_scope (outstructname))
  , base_filename (tmpdir + "/sggspc")
  , singular_parallel_installation ()
  {
    if (out_token == 0)
    {
      throw std::invalid_argument ("struct does not exist");
    }
  }
}

void sggspc_print_current_exception (std::string s)
{
  WerrorS (("singular_parallel: (" + s + ") " +
            fhg::util::current_exception_printer (": ").string()).c_str());
}

std::optional<std::multimap<std::string, pnet::type::value::value_type>>
                    gpis_launch_with_workflow (boost::filesystem::path workflow,
                    ArgumentState const &as)
try
{
  /*
  std::string debugout = as.tmpDir() + " " + as.nodeFile() + " " +
    std::to_string (as.procsPerNode()) + " " + as.showStrategy() + "\n" +
    as.inStructName() + " " + as.inStructDesc() + " " +
    as.outStructName() + " " + as.outStructDesc() + " " +
    as.neededLibrary() + " " + as.functionName() + "\n";
  */

  std::vector<std::string> options;
  std::size_t num_addargs = as.addArgsList()->nr + 1;
  //PrintS ((std::to_string (num_addargs) + " additional arguments\n").c_str());
  for (std::size_t i = 0; i < num_addargs; ++i)
  {
    int arg_type = as.addArgsList()->m[i].rtyp;
    if (arg_type != STRING_CMD)
    {
      throw std::invalid_argument ("wrong type of additional option "
        + std::to_string (i) + ", expected string got "
        + std::to_string (arg_type));
    }
    const std::string addarg_str
      (static_cast<char*> (as.addArgsList()->m[i].data));
    //debugout += addarg_str + "\n";
    options.push_back (addarg_str);
  }

  //PrintS (debugout.c_str());
  //PrintS (("have " + std::to_string (as.numTasks()) + " tasks\n").c_str());

  int in_token, out_token;
  blackboxIsCmd ((as.inStructName()).c_str(), in_token);
  blackboxIsCmd ((as.outStructName()).c_str(), out_token);
  if ((in_token == 0) || (out_token == 0))
  {
    throw std::invalid_argument ("struct does not exist");
  }

  for (std::size_t i = 0; i < as.numTasks(); ++i)
  {
    int task_type = as.argList()->m[i].rtyp;
    if (task_type != in_token)
    {
      throw std::invalid_argument ("wrong type of argument "
        + std::to_string (i) + ", expected " + std::to_string (in_token)
        + " got " + std::to_string (task_type));
    }
  }

  for (std::size_t i = 0; i < as.numTasks(); ++i)
  {
    si_link l = ssi_open_for_write (as.baseFileName() + ".i"
      + std::to_string (i));
    ssi_write_newstruct (l, in_token,
      static_cast<lists> (as.argList()->m[i].data));
    ssi_close_and_remove (l);
  }

  // now prepare startup of GPI-Space
  // TODO: do this independent from actual call?

  std::string const topology_description
    ("worker:" + std::to_string (as.procsPerNode())); // here, only one type

  boost::filesystem::path const implementation
    (as.singPI().workflow_dir() /
     "libparallel_implementation.so");

  boost::program_options::options_description options_description;
  options_description.add_options() ("help", "Display this message");
  options_description.add (gspc::options::logging());
  options_description.add (gspc::options::scoped_rifd
                                      (gspc::options::rifd::rif_port));
  options_description.add (gspc::options::drts());

  boost::program_options::variables_map vm;
  boost::program_options::store
    ( boost::program_options::command_line_parser (options)
    . options (options_description).run()
    , vm
    );

  // help option left as is. If this is really to be used, it should be checked
  // earlier. Using std::cout is not really right, should rather write to
  // std::stringstream and use PrintS ...
  if (vm.count ("help"))
  {
    std::cout << options_description;
    return std::nullopt;
  }

  gspc::installation const gspc_installation
    (as.singPI().gspc_installation (vm));

  gspc::scoped_rifds const scoped_rifd
    ( gspc::rifd::strategy
        { [&]
          {
            using namespace boost::program_options;
            variables_map vm;
            vm.emplace ("rif-strategy"
                       , variable_value (as.showStrategy(), false));
            vm.emplace ( "rif-strategy-parameters"
                       , variable_value (std::vector<std::string>{}, true)
                       );
            return vm;
          }()
        }
    , gspc::rifd::hostnames
        { [&]
          {
            try
            {
              return fhg::util::read_lines (as.nodeFile());
            }
            catch (...)
            {
              std::throw_with_nested (std::runtime_error ("reading nodefile"));
            }
          }()
        }
    , gspc::rifd::port {vm}
    , gspc_installation
    );

  gspc::scoped_runtime_system drts ( vm
                                   , gspc_installation
                                   , topology_description
                                   , scoped_rifd.entry_points()
                                   );

  std::multimap<std::string, pnet::type::value::value_type> result
    ( gspc::client (drts).put_and_run
      ( gspc::workflow (workflow)
      , { {"implementation", implementation.string()}
        , {"base_filename", as.baseFileName()}
        , {"function_name", as.functionName()}
        , {"in_struct_name", as.inStructName()}
        , {"in_struct_desc", as.inStructDesc()}
        , {"out_struct_name", as.outStructName()}
        , {"out_struct_desc", as.outStructDesc()}
        , {"needed_library", as.neededLibrary()}
        , {"task_count", static_cast<unsigned int> (as.numTasks())}
        }
      )
    );
  return result;
}
catch (...)
{
  // need to check which resources must be tidied up
  sggspc_print_current_exception (std::string ("in gpis_launch_with_workflow"));
  return std::nullopt;
}

BOOLEAN sggspc_wait_all (leftv res, leftv args)
try {

  ArgumentState as (args);

  auto result = gpis_launch_with_workflow (as.singPI().workflow_all(), as);
  if (!result.has_value()) {
    res->rtyp = NONE;
    return FALSE;
  }

  std::multimap<std::string, pnet::type::value::value_type>::const_iterator
    sm_result_it (result.value().find ("output"));
  if (sm_result_it == result.value().end())
  {
    throw std::runtime_error ("Petri net has not finished correctly");
  }

  lists out_list = static_cast<lists> (omAlloc0Bin (slists_bin));
  out_list->Init (as.numTasks());

  for (std::size_t i = 0; i < as.numTasks(); i++)
  {
    si_link l = ssi_open_for_read (as.baseFileName() +
                                   ".o" +
                                   std::to_string (i));
    // later consider case of "wrong" output (and do not throw)
    lists entry = ssi_read_newstruct (l, as.outStructName());
    ssi_close_and_remove (l);
    out_list->m[i].rtyp = as.outToken();
    out_list->m[i].data = entry;
  }


  res->rtyp = LIST_CMD;
  res->data = out_list;

  return FALSE;
}
catch (...)
{
  // need to check which resources must be tidied up
  sggspc_print_current_exception (std::string ("in sggspc_wait_all"));
  return TRUE;
}

BOOLEAN sggspc_wait_first (leftv res, leftv args)
try
{
  ArgumentState as (args);

  auto result = gpis_launch_with_workflow (as.singPI().workflow_first(), as);
  if (!result.has_value()) {
    res->rtyp = NONE;
    return FALSE;
  }

  std::multimap<std::string, pnet::type::value::value_type>::const_iterator
                          sm_result_it (result.value().find ("output"));
  if (sm_result_it == result.value().end())
  {
    throw std::runtime_error ("Petri net has not finished correctly");
  }
  unsigned int i = boost::get<unsigned int> (sm_result_it->second);

  si_link l = ssi_open_for_read (as.baseFileName() +
                                 ".o" +
                                 std::to_string (i));

  lists entry = ssi_read_newstruct (l, as.outStructName());
  ssi_close_and_remove (l);

  res->rtyp = as.outToken();
  res->data = entry;

  return FALSE;
}
catch (...)
{
  // need to check which resources must be tidied up
  sggspc_print_current_exception (std::string ("in sggspc_wait_first"));
  return TRUE;
}

extern "C" int mod_init (SModulFunctions* psModulFunctions)
{
  // TODO: explicit check if module has already been loaded?
  //PrintS ("DEBUG: in mod_init\n");

  psModulFunctions->iiAddCproc
    ((currPack->libname ? currPack->libname : ""),
      "sggspc_wait_all", FALSE, sggspc_wait_all);

  psModulFunctions->iiAddCproc
    ((currPack->libname ? currPack->libname : ""),
      "sggspc_wait_first", FALSE, sggspc_wait_first);

  return MAX_TOK;
}
