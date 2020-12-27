#pragma once

#include <drts/drts.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

namespace singular_smoothness
{
  //! \note collects information relative to the path of the executable
  class installation
  {
  public:
    installation();
    installation (boost::filesystem::path const&);
    installation (boost::filesystem::path const&,
      boost::filesystem::path const&);

    boost::filesystem::path workflow() const;
    boost::filesystem::path workflow_dir() const;
    gspc::installation gspc_installation
      (boost::program_options::variables_map&) const;

  private:
    boost::filesystem::path const _path;
    boost::filesystem::path const _gspc_path;
  };
}
