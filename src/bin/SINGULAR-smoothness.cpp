#include <installation.hpp>

#include <drts/client.hpp>
#include <drts/drts.hpp>
#include <drts/scoped_rifd.hpp>

#include <we/type/value/show.hpp>

#include <fhg/util/boost/program_options/validators/nonempty_file.hpp>
#include <fhg/util/boost/program_options/validators/positive_integral.hpp>
#include <fhg/util/boost/program_options/validators/nonempty_string.hpp>
#include <fhg/util/boost/program_options/validators/existing_directory.hpp>
#include <fhg/util/boost/program_options/validators/nonexisting_path_in_existing_directory.hpp>

#include <util-generic/executable_path.hpp>
#include <util-generic/print_exception.hpp>

namespace
{
  namespace option
  {
    namespace name
    {
      constexpr char const* const file_with_ideal {"file-with-ideal"};
      constexpr char const* const topology_description {"topology-description"};
      constexpr char const* const projective {"projective"};
      constexpr char const* const hybrid_codimension {"hybrid-codimension"};
      constexpr char const* const rnd_coeff_max {"rnd-coeff-max"};
      constexpr char const* const rnd_tries {"rnd-tries"};
      constexpr char const* const max_zero_tries {"max-zero-tries"};
      constexpr char const* const heuristics_options {"heuristics-options"};
      constexpr char const* const sg_log_level {"sg-log-level"};
      constexpr char const* const help {"help"};
    }
  }
}

int main (int argc, char** argv)
try
{
  singular_smoothness::installation const singular_smoothness_installation;

  namespace validators = fhg::util::boost::program_options;

  boost::program_options::options_description options_general ("General");

  options_general.add_options() (option::name::help, "print help");

  boost::program_options::options_description options_job ("Job");

  options_job.add_options()
    ( option::name::file_with_ideal
    , boost::program_options::value<validators::nonempty_file>()->required()
    , "file with input ideal IX"
    )
    ( option::name::topology_description
    , boost::program_options::value<validators::nonempty_string>()->required()
    , "topology description"
    )
    ( option::name::projective
    , boost::program_options::value<bool>()->default_value (false)->implicit_value (true)
    , "pass to affine charts of projective variety"
    )
    ( option::name::hybrid_codimension
    , boost::program_options::value<int>()->default_value (0)
    , "pass to Jacobi criterion if this codimension has been reached"
    )
    ( option::name::rnd_coeff_max
    , boost::program_options::value<int>()->default_value (10)
    , "max. absolute value for random coefficients in descent"
    )
    ( option::name::rnd_tries
    , boost::program_options::value<unsigned long>()->default_value (0UL)
    , "number of random linear combinations to be tested in descent"
    )
    ( option::name::max_zero_tries
    , boost::program_options::value<unsigned long>()->default_value (3UL)
    , "maximal number of combinations reducing to zero in descent"
    )
    ( option::name::heuristics_options
    , boost::program_options::value<unsigned long>()->default_value (3UL)
    , "options for heuristics in jacobi split"
    )
    ( option::name::sg_log_level
    , boost::program_options::value<int>()->default_value (3)
    , "log level for application"
    )
    ;

  boost::program_options::options_description options_description;

  options_description.add (options_general);
  options_description.add (options_job);

  options_description.add (gspc::options::logging());
  options_description.add (gspc::options::scoped_rifd());
  options_description.add (gspc::options::drts());

  boost::program_options::variables_map vm;
  boost::program_options::store
    ( boost::program_options::command_line_parser (argc, argv)
      . options (options_description).run()
      , vm
      );

  if (vm.count (option::name::help))
  {
    std::cout << options_description << std::endl;

    return 0;
  }

  vm.notify();

  boost::filesystem::path const installation_path
    (fhg::util::executable_path().parent_path().parent_path());
  boost::filesystem::path const implementation //! \todo: configure
    (installation_path / "libexec" / "workflow" / "libsmoothness_implementation.so");
  boost::filesystem::path const file_with_ideal
    (vm.at (option::name::file_with_ideal).as<validators::nonempty_file>());
  bool const is_projective
    (vm.at (option::name::projective).as<bool>());
  int const codimension_limit
    (vm.at (option::name::hybrid_codimension).as<int>());
  int const desc_rnd_coeff_max
    (vm.at (option::name::rnd_coeff_max).as<int>());
  unsigned long const desc_rnd_tries
    (vm.at (option::name::rnd_tries).as<unsigned long>());
  unsigned long const desc_max_zero_tries
    (vm.at (option::name::max_zero_tries).as<unsigned long>());
  unsigned long const split_heuristics_options
    (vm.at (option::name::heuristics_options).as<unsigned long>());
  int const logging_level
    (vm.at (option::name::sg_log_level).as<int>());
  gspc::installation const gspc_installation
    (singular_smoothness_installation.gspc_installation (vm));
  //std::cout << "DEBUG: will now set up scoped rif" << std::endl;
  gspc::scoped_rifds const scoped_rifd ( gspc::rifd::strategy {vm}
                                      , gspc::rifd::hostnames {vm}
                                      , gspc::rifd::port {vm}
                                      , gspc_installation
                                      );

  std::string const topology_description
    (vm.at (option::name::topology_description).as
    <validators::nonempty_string>());
  //std::cout << "DEBUG: scoped rif set up, will now set up drts" << std::endl;
  gspc::scoped_runtime_system drts ( vm
                                   , gspc_installation
                                   , topology_description
                                   , scoped_rifd.entry_points()
                                   );

  //std::cout << "DEBUG: drts set up, will now call put_and_run" << std::endl;
  std::multimap<std::string, pnet::type::value::value_type> const result
    ( gspc::client (drts).put_and_run
      ( gspc::workflow (singular_smoothness_installation.workflow())
      , { {"implementation", implementation.string()}
        , {"input_ideal", file_with_ideal.string()}
        , {"is_projective", is_projective}
        , {"codimension_limit", codimension_limit}
        , {"desc_rnd_coeff_max", desc_rnd_coeff_max}
        , {"desc_rnd_tries", desc_rnd_tries}
        , {"desc_max_zero_tries", desc_max_zero_tries}
        , {"split_heuristics_options", split_heuristics_options}
        , {"logging_level", logging_level}
        }
      )
    );
  //std::cout << "DEBUG: put_and_run finished, will now return result" << std::endl;
  for ( std::pair<std::string, pnet::type::value::value_type> const& kv
      : result
      )
  {
    std::cout
      << kv.first << " := " << pnet::type::value::show (kv.second) << '\n';
  }

  return 0;
}
catch (...)
{
  std::cerr << fhg::util::current_exception_printer() << std::endl;

  return 1;
}
