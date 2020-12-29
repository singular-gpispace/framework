#include <ctime>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#include <installation.hpp>

#include <drts/client.hpp>
#include <drts/drts.hpp>
#include <drts/scoped_rifd.hpp>

#include <we/type/value/show.hpp>

#include <util-generic/executable_path.hpp>
#include <util-generic/print_exception.hpp>
#include <util-generic/read_lines.hpp>

#include "Singular/libsingular.h"

#include "smoothness/singular_commands.hpp"

/* Usage in Singular

LIB "${install}/lib/libSINGULAR-smoothness-sminterface.so";
ring R = 30013,(x,y,z,w,s1,s2,s3),dp;
ideal I = x^3+y^3+z^3+w^3, x^2-2042*x*y-2043*y^2+z*s1-2043*w*s1, -z^2-2043*z*w-2042*w^2+x*s1+2042*y*s1, -11366*x*z-11366*y*z-11366*x*w-11366*y*w-11366*s1^2+s1*s2, x*y+y^2+w*s1+z*s2+2042*w*s2, 2042*z^2-2042*w^2-2042*x*s1+2042*y*s1+x*s2-2043*y*s2, 681*x*s1+681*y*s1-10004*z*s1-10004*w*s1-10685*s1^2+11366*x*s2+11366*y*s2-9323*z*s2-9324*w*s2-1361*s1*s2+9324*s2^2+s2*s3, -9324*x*s1-9324*y*s1-10685*z*s1-12727*w*s1+10005*s1^2+9324*z*s2+9324*w*s2+11367*s1*s2+z*s3+w*s3+2042*s1*s3, -2042*z^2-x*w-y*w-8643*x*s1-2723*y*s1-681*z*s1-9323*w*s1+10004*s1^2+9323*x*s2-9323*y*s2-11366*z*s2+2043*w*s2-11366*s1*s2+y*s3+2042*w*s3, 2042*z^2-2042*x*s1-7962*y*s1-10685*z*s1-2043*w*s1-1361*s1^2+x*s2-11366*y*s2+9324*z*s2+s1*s2+x*s3-2042*w*s3+2043*s1*s3;
smoothtest ( I # ideal
           , 1 # is_projective: > 0
           , "/path/to/temporary/file/visible/from/every/node"
           , 4 # procs per node
           , "/path/to/nodefile" # usually $PBS_NODEFILE
           , "bootstrap strategy" # ssh or pbsdsh, usually ssh
           , 2 # codimension limit
           [, "extra", "arguments"] # strings that are passed to gpispace
           ); # TODO outdated
 */

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

  template<std::size_t arg_index, typename T, int type>
    T require_argument (leftv args, std::string type_name, std::string argument_name)
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

  template<std::size_t arg_index, typename T>
    T nth_list_arg (lists l)
  {
    return reinterpret_cast<T> (l->m[arg_index].data);
  }
}

static bool call_gspc_smoothtest ( ideal input_ideal
                                 , bool projective
                                 , int codimension_limit
                                 , std::string const& tmpdir
                                 , std::string const& nodefile
                                 , unsigned int procs_per_node
                                 , std::string const& rif_strategy
                                 , int desc_rnd_coeff_max
                                 , unsigned long desc_rnd_tries
                                 , unsigned long desc_max_zero_tries
                                 , unsigned long split_heuristics_options
                                 , int logging_level
                                 , std::vector<std::string> const& options
                                 , boost::filesystem::path installation_path
                                 )
{
  std::string const topology_description
    ( "compute:" + std::to_string (procs_per_node)
    + " administration:1x1"
    );

  std::string ideal_filename (tmpdir);
  if (ideal_filename.at (ideal_filename.length()-1) != '/')
  {
    ideal_filename += '/';
  }
  std::mt19937 generator {std::random_device{}()};
  std::uniform_int_distribution<int> distribution {'a', 'z'};
  std::string rand_str (8, '\0');
  for (char& d : rand_str)
  {
    d = distribution (generator);
  }
  ideal_filename = ideal_filename + "ideal-" + std::to_string (time (NULL)) + "-"
    + rand_str + ".ssi";

  idhdl input_ideal_handle = enterid ("SMinputideal", 0, IDEAL_CMD, &(currRing->idroot), FALSE);
  IDIDEAL (input_ideal_handle) = input_ideal;
  singular_write_ssi ("SMinputideal", ideal_filename);
  dechain_handle (input_ideal_handle, &(currRing->idroot));

  singular_smoothness::installation const singular_smoothness_installation (installation_path);

  boost::filesystem::path const implementation
    (singular_smoothness_installation.workflow_dir() / "libsmoothness_implementation.so");

  boost::program_options::options_description options_description;
  options_description.add_options() ("help", "Display this message");
  options_description.add (gspc::options::logging());
  options_description.add (gspc::options::scoped_rifd (gspc::options::rifd::rif_port));
  options_description.add (gspc::options::drts());

  boost::program_options::variables_map vm;
  boost::program_options::store
    ( boost::program_options::command_line_parser (options)
    . options (options_description).run()
    , vm
    );

  if (vm.count ("help"))
  {
    std::cout << options_description << "\n";
    return false;
  }

  vm.notify();

  gspc::installation const gspc_installation
    (singular_smoothness_installation.gspc_installation (vm));

  gspc::scoped_rifds const scoped_rifd
    ( gspc::rifd::strategy
        { [&]
          {
            using namespace boost::program_options;
            variables_map vm;
            vm.emplace ("rif-strategy", variable_value (rif_strategy, false));
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
              return fhg::util::read_lines (nodefile);
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

  std::multimap<std::string, pnet::type::value::value_type> const result
    ( gspc::client (drts).put_and_run
      ( gspc::workflow (singular_smoothness_installation.workflow())
      , { {"implementation", implementation.string()}
        , {"input_ideal", ideal_filename}
        , {"is_projective", projective}
        , {"codimension_limit", codimension_limit}
        , {"desc_rnd_coeff_max", desc_rnd_coeff_max}
        , {"desc_rnd_tries", desc_rnd_tries}
        , {"desc_max_zero_tries", desc_max_zero_tries}
        , {"split_heuristics_options", split_heuristics_options}
        , {"logging_level", logging_level}
        }
      )
    );

  for ( std::pair<std::string, pnet::type::value::value_type> const& kv
      : result
      )
  {
    std::cout
      << kv.first << " := " << pnet::type::value::show (kv.second) << '\n';
  }

  std::multimap<std::string, pnet::type::value::value_type>::const_iterator
    sm_result_it (result.find ("result"));
  if (sm_result_it == result.end())
  {
    throw std::runtime_error ("no result has been returned");
  }
  return boost::get<bool> ((*sm_result_it).second);
}

BOOLEAN smoothtest (leftv res, leftv args)
try
{
  ideal input_ideal = require_argument<0, ideal, IDEAL_CMD> (args, "ideal", "input ideal");
  bool projective = require_argument<1, long, INT_CMD> (args, "integer", "is projective") > 0;
  int codimension_limit = require_argument<2, long, INT_CMD> (args, "integer", "codimension limit");
  std::string const tmpdir (require_argument<3, char*, STRING_CMD> (args, "string", "temporary directory"));
  std::string const nodefile (require_argument<4, char*, STRING_CMD> (args, "string", "nodefile"));
  unsigned int const procs_per_node = require_argument<5, long, INT_CMD> (args, "integer", "processes per node");
  std::string const rif_strategy (require_argument<6, char*, STRING_CMD> (args, "string", "rif strategy"));
  int const desc_rnd_coeff_max = require_argument<7, long, INT_CMD> (args, "integer", "max. random coefficient");
  unsigned long const desc_rnd_tries = require_argument<8, long, INT_CMD> (args, "integer", "max. number of random tries");
  unsigned long const desc_max_zero_tries = require_argument<9, long, INT_CMD> (args, "integer", "max. number of zero tries");
  unsigned long const split_heuristics_options = require_argument<10, long, INT_CMD> (args, "integer", "split heuristics options");
  int const logging_level = require_argument<11, long, INT_CMD> (args, "integer", "logging level");

  lists addargs_list = require_argument<12, lists, LIST_CMD> (args, "list", "additional options");
  std::size_t num_addargs = addargs_list->nr + 1;
  std::vector<std::string> options;
  for (std::size_t i = 0; i < num_addargs; ++i)
  {
    int arg_type = addargs_list->m[i].rtyp;
    if (arg_type != STRING_CMD)
    {
      throw std::invalid_argument ("wrong type of additional option "
        + std::to_string (i) + ", expected string got "
        + std::to_string (arg_type));
    }
    const std::string addarg_str
      (static_cast<char*> (addargs_list->m[i].data));
    options.push_back (addarg_str);
  }

  bool result = call_gspc_smoothtest (input_ideal, projective,
    codimension_limit, tmpdir, nodefile, procs_per_node, rif_strategy,
    desc_rnd_coeff_max, desc_rnd_tries, desc_max_zero_tries,
    split_heuristics_options, logging_level, options,
    fhg::util::executable_path (&smoothtest).parent_path());
  res->rtyp = INT_CMD;
  res->data = reinterpret_cast<void*> (static_cast<int> (result));
  return FALSE;
}
catch (...)
{
  WerrorS (("smoothtest: " + fhg::util::current_exception_printer (": ").string()).c_str());
  return TRUE;
}

extern "C" int mod_init (SModulFunctions* psModulFunctions)
{
  psModulFunctions->iiAddCproc
    ((currPack->libname ? currPack->libname : ""),
      "gspc_smoothtest", FALSE, smoothtest);

  return MAX_TOK;
}
