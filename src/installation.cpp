#include <installation.hpp>

#include <util-generic/executable_path.hpp>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <stdexcept>

namespace singular_parallel
{
  namespace
  {
    void check ( boost::filesystem::path const& path
               , bool okay
               , std::string const& message
               )
    {
      if (!okay)
      {
        throw std::logic_error
          ( ( boost::format ("%1% %2%: Installation incomplete!?")
            % path
            % message
            ).str()
          );
      }
    }

    void check_is_directory (boost::filesystem::path const& path)
    {
      check ( path
            , boost::filesystem::is_directory (path)
            , "is not a directory"
            );
    }
    void check_is_file (boost::filesystem::path const& path)
    {
      check ( path
            , boost::filesystem::exists (path)
            , "does not exist"
            );
      check ( path
            , boost::filesystem::is_regular_file (path)
            , "is not a regular file"
            );
    }

    //! \todo configure
    boost::filesystem::path gspc_home
      (boost::filesystem::path const& gspc_path)
    {
      return gspc_path;
    }
    boost::filesystem::path workflow_path
      (boost::filesystem::path const& installation_path)
    {
      return installation_path / "libexec" / "workflow";
    }
    boost::filesystem::path libraries_path
      (boost::filesystem::path const& installation_path)
    {
      return installation_path / "libexec" / "workflow";
    }

    boost::filesystem::path workflow_all_file
      (boost::filesystem::path const& installation_path)
    {
      return workflow_path (installation_path) / "parallel_all.pnet";
    }
    boost::filesystem::path workflow_first_file
      (boost::filesystem::path const& installation_path)
    {
      return workflow_path (installation_path) / "parallel_first.pnet";
    }
    boost::filesystem::path workflow_smoothness_file
      (boost::filesystem::path const& installation_path)
    {
      return workflow_path (installation_path) / "smoothness.pnet";
    }
  }

  installation::installation (boost::program_options::variables_map const& vm)
    : _path (SP_INSTALL_PATH), _gspc_path (GSPC_HOME)
  {
    //! \todo more detailed tests!?
    check_is_directory (gspc_home (_gspc_path));
    check_is_directory (workflow_path (_path));
    check_is_file (workflow_all());
    check_is_file (workflow_first());
    check_is_file (workflow_smoothness());

    gspc::set_gspc_home
      ( const_cast<boost::program_options::variables_map&> (vm)
      , gspc_home (_gspc_path)
      );
    gspc::set_application_search_path
      ( const_cast<boost::program_options::variables_map&> (vm)
      , libraries_path (_path)
      );
  }

  boost::filesystem::path installation::workflow_all() const
  {
    return workflow_all_file (_path);
  }
  boost::filesystem::path installation::workflow_first() const
  {
    return workflow_first_file (_path);
  }
  boost::filesystem::path installation::workflow_smoothness() const
  {
    return workflow_smoothness_file (_path);
  }
  boost::filesystem::path installation::workflow_dir() const
  {
    return workflow_path (_path);
  }
  gspc::installation installation::gspc_installation() const
  {
    return {gspc_home (_gspc_path)};
  }
}
