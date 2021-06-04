#include <util-generic/executable_path.hpp>

#include <boost/filesystem/path.hpp>

namespace config
{
  boost::filesystem::path const& library()
  {
    static boost::filesystem::path const sing_path
      ( fhg::util::executable_path (&siInit).parent_path().parent_path()
      );

    static boost::filesystem::path const library
      ( sing_path
      / "lib" / "libSingular.so"
      );

    return library;
  }
}
