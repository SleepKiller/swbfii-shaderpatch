#include "framework.hpp"

#include "install_manifest.hpp"
#include "installer.hpp"
#include "user_config_loader.hpp"
#include "user_config_saver.hpp"
#include "xml_install_manifest.hpp"

using namespace std::literals;

installer::installer(std::filesystem::path install_path)
{
   install_future =
      std::async(std::launch::async, &installer::install, this, install_path);
}

auto installer::status() const noexcept -> installer_status
{
   return current_status;
}

auto installer::status_message() noexcept -> std::wstring
{
   std::scoped_lock lock{resource_mutex};

   return message;
}

auto installer::progress() const noexcept -> double
{
   return progress_percent;
}

auto installer::failed_to_remove_files() noexcept -> std::vector<std::filesystem::path>
{
   std::scoped_lock lock{resource_mutex};

   return std::exchange(remaining_files, {});
}

void installer::install(std::filesystem::path install_path) noexcept
{
   const auto v1_3_files = load_xml_install_manifest(install_path);
   const auto existing_files = load_install_manifest(install_path);

   std::vector<std::filesystem::path> paths_to_copy;

   for (const auto& entry : std::filesystem::recursive_directory_iterator{
           std::filesystem::current_path()}) {
      if (!entry.is_regular_file()) continue;
      if (entry.path().filename() == L"shader patch.yml"sv) continue;
      if (entry.path().filename() == L"Shader Patch Installer.exe"sv) continue;

      paths_to_copy.push_back(
         entry.path().lexically_relative(std::filesystem::current_path()));
   }

   const double total_work_items =
      v1_3_files.size() + existing_files.size() + paths_to_copy.size() + 3;
   double completed_work_items = 0;

   std::vector<std::filesystem::path> installed_files;

   const auto remove_existing = [&](const std::vector<std::filesystem::path>& files) {
      for (auto& file : files) {
         progress_percent.store((++completed_work_items / total_work_items) * 100.0);

         set_message(L"Removing "s + file.native());

         try {
            const auto full_path = install_path / file;

            if (std::filesystem::exists(full_path) &&
                std::filesystem::is_regular_file(full_path)) {
               std::filesystem::remove(full_path);
            }
         }
         catch (std::filesystem::filesystem_error&) {
            installed_files.emplace_back(file);
         }
      }
   };

   const auto user_config = load_user_config(install_path / L"shader patch.yml"sv);

   remove_existing(v1_3_files);
   remove_existing(existing_files);

   try {
      for (auto& path : paths_to_copy) {
         progress_percent.store((++completed_work_items / total_work_items) * 100.0);

         set_message(L"Copying "s + path.native());

         auto dest_path = install_path / path;

         std::filesystem::create_directories(dest_path.parent_path());
         std::filesystem::copy_file(path, install_path / path,
                                    std::filesystem::copy_options::overwrite_existing);

         installed_files.emplace_back(path);
      }

      progress_percent.store((++completed_work_items / total_work_items) * 100.0);

      set_message(L"Copying "s + L"shader patch.yml"s);

      save_user_config(install_path / L"shader patch.yml"sv, user_config);
      installed_files.emplace_back(L"shader patch.yml"sv);

      progress_percent.store((++completed_work_items / total_work_items) * 100.0);

      set_message(L"Copying "s + L"Shader Patch Settings.exe"s);

      progress_percent.store((++completed_work_items / total_work_items) * 100.0);

      std::filesystem::copy_file(L"Shader Patch Installer.exe"sv,
                                 install_path / L"Shader Patch Settings.exe"sv,
                                 std::filesystem::copy_options::overwrite_existing);
      installed_files.emplace_back(L"Shader Patch Settings.exe"sv);

      set_message(L"Saving install manifest"s);

      progress_percent.store((++completed_work_items / total_work_items) * 100.0);

      save_install_manifest(install_path, installed_files);
   }
   catch (std::filesystem::filesystem_error& e) {
      if (e.code().value() == ERROR_ACCESS_DENIED) {
         current_status = installer_status::permission_denied;
      }
      else {
         current_status = installer_status::failure;
      }

      set_message(L"Failure! Attempting to cleanup install..."s);

      failed_install_cleanup(install_path, installed_files);

      return;
   }

   set_message(L"Complete"s);
   current_status = installer_status::completed;
}

void installer::failed_install_cleanup(const std::filesystem::path& install_path,
                                       const std::vector<std::filesystem::path>& installed_files) noexcept
{
   for (auto& file : installed_files) {
      const auto full_path = install_path / file;

      try {
         if (std::filesystem::exists(full_path)) {
            std::filesystem::remove(full_path);
         }
      }
      catch (std::filesystem::filesystem_error&) {
         std::scoped_lock lock{resource_mutex};

         remaining_files.emplace_back(full_path);
      }
   }
}

void installer::set_message(std::wstring new_message) noexcept
{
   std::scoped_lock lock{resource_mutex};

   message = std::move(new_message);
}

[[noreturn]] void restart_and_install_as_admin(std::filesystem::path install_path) noexcept
{
   std::wstring executable_path;
   executable_path.resize(MAX_PATH);

   DWORD str_size = GetModuleFileNameW(nullptr, executable_path.data(),
                                       executable_path.size() + 1);

   while (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
      executable_path.resize(executable_path.size() * 2);
      str_size = GetModuleFileNameW(nullptr, executable_path.data(),
                                    executable_path.size() + 1);
   }

   executable_path.resize(str_size);

   const std::wstring commandline = L"install "s + install_path.native();

   ShellExecuteW(nullptr, L"runas", executable_path.data(), commandline.c_str(),
                 nullptr, SW_SHOWNORMAL);

   std::exit(0);
}
