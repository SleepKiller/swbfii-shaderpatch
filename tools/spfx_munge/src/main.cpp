
#include "munge_spfx.hpp"
#include "string_utilities.hpp"
#include "synced_io.hpp"

#include <filesystem>
#include <future>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <clara.hpp>

using namespace std::literals;
using namespace sp;

namespace fs = std::filesystem;

int main(int arg_count, char* args[])
{
   using namespace clara;

   bool help = false;
   auto output_dir = "./"s;
   auto source_dir = "./"s;

   // clang-format off

   auto cli = Help{help}
      | Opt{output_dir, "output directory"s}
      ["--outputdir"s]["-o"s]
      ("Path to place munged files in."s)
      | Opt{source_dir, "source directory"s}
      ["--sourcedir"s]["-s"s]
      ("Path to search for input .spfx files."s);

   // clang-format on

   const auto result = cli.parse(Args{arg_count, args});

   if (!result) {
      synced_error_print("Commandline Error: "sv, result.errorMessage());

      return 1;
   }
   else if (help) {
      synced_print(cli);

      return 0;
   }

   if (!fs::exists(output_dir) && !fs::create_directories(output_dir)) {
      synced_error_print("Unable to create output directory "sv,
                         std::quoted(output_dir), "."sv);

      return 1;
   }

   if (!fs::exists(source_dir)) {
      synced_error_print("Source Directory "sv, std::quoted(source_dir),
                         " does not exist!"sv);

      return 1;
   }

   for (const auto& entry : fs::recursive_directory_iterator{source_dir}) {
      if (!fs::is_regular_file(entry.path())) continue;
      if (entry.path().extension() != ".spfx"_svci) continue;

      try {
         munge_spfx(entry.path(), output_dir);
      }
      catch (std::exception& e) {
         synced_error_print("Error while munging "sv,
                            entry.path().filename().string(), " :"sv, e.what());
      }
   }
}
