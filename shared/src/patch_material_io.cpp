
#include "patch_material_io.hpp"
#include "compose_exception.hpp"
#include "game_rendertypes.hpp"
#include "material_rendertype_property_mappings.hpp"
#include "string_utilities.hpp"
#include "ucfb_reader.hpp"
#include "volume_resource.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

#include <gsl/gsl>

using namespace std::literals;

namespace sp {

namespace {
enum class Material_version : std::uint32_t { v_1, v_2, v_3, current = v_3 };

inline namespace v_3 {
auto read_patch_material_impl(ucfb::Reader reader) -> Material_config;
}

namespace v_2 {
auto read_patch_material_impl(ucfb::Reader reader) -> Material_config;
}

namespace v_1 {
auto read_patch_material_impl(ucfb::Reader reader) -> Material_config;
}
}

void write_patch_material(ucfb::File_writer& writer, const Material_config& config)
{
   std::ostringstream ostream;

   // write material chunk
   {
      ucfb::File_writer matl{"matl"_mn, ostream};

      matl.emplace_child("VER_"_mn).write(Material_version::current);

      matl.emplace_child("INFO"_mn).write(config.name, config.type,
                                          config.overridden_rendertype);

      // write properties
      {
         auto prps = matl.emplace_child("PRPS"_mn);

         prps.write(static_cast<std::uint32_t>(config.properties.size()));

         for (const auto& prop : config.properties) {
            if (prop.value.valueless_by_exception())
               throw std::runtime_error{"Invalid property value!"};

            prps.write(prop.name);
            prps.write(static_cast<std::uint32_t>(prop.value.index()));
            std::visit([&](const auto& v) { prps.write(v); }, prop.value);
         }
      }

      const auto write_resources =
         [&](const Magic_number mn,
             const absl::flat_hash_map<std::string, std::string>& resources) {
            auto texs = matl.emplace_child(mn);

            texs.write<std::uint32_t>(
               gsl::narrow_cast<std::uint32_t>(resources.size()));
            for (const auto& [key, value] : resources) texs.write(key, value);
         };

      write_resources("SR__"_mn, config.resources);
   }

   const auto matl_data = ostream.str();
   const auto matl_span = std::as_bytes(std::span{matl_data});

   write_volume_resource(writer, config.name, Volume_resource_type::material, matl_span);
}

void write_patch_material(const std::filesystem::path& save_path,
                          const Material_config& config)
{
   auto file = ucfb::open_file_for_output(save_path);

   ucfb::File_writer writer{"ucfb"_mn, file};

   write_patch_material(writer, config);
}

auto read_patch_material(ucfb::Reader_strict<"matl"_mn> reader) -> Material_config
{
   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Material_version>();

   reader.reset_head();

   switch (version) {
   case Material_version::current:
      return read_patch_material_impl(reader);
   case Material_version::v_1:
      return v_1::read_patch_material_impl(reader);
   case Material_version::v_2:
      return v_2::read_patch_material_impl(reader);
   default:
      throw std::runtime_error{"material has newer version than supported by "
                               "this version of Shader Patch!"};
   }
}

namespace {

const absl::flat_hash_map<std::string, std::vector<std::string>> resource_names_mappings{
   {"basic_unlit"s, {"ColorMap"s, "EmissiveMap"s}},

   {"normal_ext_terrain"s,
    {"height_textures"s, "diffuse_ao_textures"s, "normal_gloss_textures"s, "envmap"s}},

   {"normal_ext"s,
    {"DiffuseMap"s, "NormalMap"s, "HeightMap"s, "DetailMap"s, "DetailNormalMap"s,
     "EmissiveMap"s, "OverlayDiffuseMap"s, "OverlayNormalMap"s, "AOMap"s, "EnvMap"s}},

   {"normal_ext-tessellated"s,
    {"DiffuseMap"s, "NormalMap"s, "HeightMap"s, "DetailMap"s, "DetailNormalMap"s,
     "EmissiveMap"s, "OverlayDiffuseMap"s, "OverlayNormalMap"s, "AOMap"s, "EnvMap"s}},

   {"particle_ext"s, {"ColorMap"s}},

   {"pbr_terrain"s, {"height_textures"s, "albedo_ao_textures"s, "normal_mr_textures"s}},

   {"pbr"s,
    {"AlbedoMap"s, "NormalMap"s, "MetallicRoughnessMap"s, "AOMap"s, "EmissiveMap"s,
     // TODO: Remove these TEMP ones.
     "TEMP_ibl_dfg"s, "TEMP_ibl_specular"s, "TEMP_ibl_diffuse"}},

   {"skybox"s, {"Skybox"s, "SkyboxEmissive"s}},

   {"static_water"s,
    {"NormalMap"s, "ReflectionMap"s, "DepthBuffer"s, "RefractionMap"s}}};

template<Magic_number mn>
auto read_old_resources(ucfb::Reader_strict<mn> texs,
                        const std::vector<std::string>& resource_mappings)
   -> absl::flat_hash_map<std::string, std::string>
{
   const auto count = texs.read<std::uint32_t>();

   absl::flat_hash_map<std::string, std::string> resources;
   resources.reserve(count);

   const std::size_t read_count =
      std::min(std::size_t{count}, resource_mappings.size());

   for (auto i = 0; i < read_count; ++i) {
      auto resource = texs.read_string();

      if (resource.empty()) continue;

      resources[resource_mappings[i]] = resource;
   }

   return resources;
}

auto read_material_prop_var(ucfb::Reader_strict<"PRPS"_mn>& prps,
                            const std::uint32_t type_index) -> Material_property::Value
{
   switch (type_index) {
   case 0:
      return prps.read<std::variant_alternative_t<0, Material_property::Value>>();
   case 1:
      return prps.read<std::variant_alternative_t<1, Material_property::Value>>();
   case 2:
      return prps.read<std::variant_alternative_t<2, Material_property::Value>>();
   case 3:
      return prps.read<std::variant_alternative_t<3, Material_property::Value>>();
   case 4:
      return prps.read<std::variant_alternative_t<4, Material_property::Value>>();
   case 5:
      return prps.read<std::variant_alternative_t<5, Material_property::Value>>();
   case 6:
      return prps.read<std::variant_alternative_t<6, Material_property::Value>>();
   case 7:
      return prps.read<std::variant_alternative_t<7, Material_property::Value>>();
   case 8:
      return prps.read<std::variant_alternative_t<8, Material_property::Value>>();
   case 9:
      return prps.read<std::variant_alternative_t<9, Material_property::Value>>();
   case 10:
      return prps.read<std::variant_alternative_t<10, Material_property::Value>>();
   case 11:
      return prps.read<std::variant_alternative_t<11, Material_property::Value>>();
   case 12:
      return prps.read<std::variant_alternative_t<12, Material_property::Value>>();
   }

   static_assert(std::variant_size_v<Material_property::Value> == 13);

   std::terminate();
}

auto rename_tessellated_rendertype(std::string_view rendertype_view) -> std::string
{
   std::string rendertype{rendertype_view};

   if (rendertype.starts_with("normal_ext-tessellated"sv)) {
      rendertype.replace(0, "normal_ext-tessellated"sv.size(), "normal_ext"sv);
   }

   return rendertype;
}

void parse_rendertype(std::string_view rendertype,
                      std::vector<Material_property>& properties)
{
   const auto root_type = split_string_on(rendertype, "."sv)[0];

   if (!rendertype_property_mappings.contains(root_type)) return;

   for (const auto& [str, prop] : rendertype_property_mappings.at(root_type)) {
      if (!contains(rendertype, str)) continue;

      properties.push_back(prop);
   }
}

namespace v_3 {

auto read_patch_material_impl(ucfb::Reader reader) -> Material_config
{
   Material_config config{};

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Material_version>();

   Ensures(version == Material_version::v_3);

   {
      auto info = reader.read_child_strict<"INFO"_mn>();

      config.name = info.read_string();
      config.type = info.read_string();
      config.overridden_rendertype = info.read<Rendertype>();
   }

   {
      auto prps = reader.read_child_strict<"PRPS"_mn>();

      const auto count = prps.read<std::uint32_t>();

      config.properties.reserve(count);

      for (auto i = 0; i < count; ++i) {
         const auto name = prps.read_string();
         const auto type_index = prps.read<std::uint32_t>();

         config.properties.emplace_back(std::string{name},
                                        read_material_prop_var(prps, type_index));
      }
   }

   const auto read_resources =
      [&](auto sr, absl::flat_hash_map<std::string, std::string>& resources) {
         const auto count = sr.read<std::uint32_t>();
         resources.reserve(count);

         for (auto i = 0; i < count; ++i) {
            resources.emplace(std::pair{sr.read_string(), sr.read_string()});
         }
      };

   read_resources(reader.read_child_strict<"SR__"_mn>(), config.resources);

   return config;
}

}

namespace v_2 {

auto read_patch_material_impl(ucfb::Reader reader) -> Material_config
{
   Material_config config{};

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Material_version>();

   Ensures(version == Material_version::v_2);

   std::string rendertype;

   {
      auto info = reader.read_child_strict<"INFO"_mn>();

      config.name = info.read_string();
      rendertype = rename_tessellated_rendertype(info.read_string());
      config.overridden_rendertype = info.read<Rendertype>();
      [[maybe_unused]] auto cb_shader_stages = info.read<std::uint32_t>();
      [[maybe_unused]] auto cb_name = info.read_string();

      [[maybe_unused]] auto fail_safe_texture_index = info.read<std::uint32_t>();
      [[maybe_unused]] auto [tessellation, tessellation_primitive_topology] =
         info.read_multi<bool, std::uint32_t>();
   }

   {
      auto prps = reader.read_child_strict<"PRPS"_mn>();

      const auto count = prps.read<std::uint32_t>();

      config.properties.reserve(count);

      for (auto i = 0; i < count; ++i) {
         const auto name = prps.read_string();
         const auto type_index = prps.read<std::uint32_t>();

         config.properties.emplace_back(std::string{name},
                                        read_material_prop_var(prps, type_index));
      }
   }

   const auto material_type = split_string_on(rendertype, "."sv)[0];

   reader.read_child_strict<"VSSR"_mn>();
   reader.read_child_strict<"HSSR"_mn>();
   reader.read_child_strict<"DSSR"_mn>();
   reader.read_child_strict<"GSSR"_mn>();
   config.resources = read_old_resources(reader.read_child_strict<"PSSR"_mn>(),
                                         resource_names_mappings.at(material_type));

   parse_rendertype(rendertype, config.properties);
   config.type = material_type;

   return config;
}

}

namespace v_1 {

auto convert_v1_constant_buffer(const std::string_view rendertype,
                                const std::span<const std::byte> constant_buffer)
   -> std::vector<Material_property>
{
   Material_config config;

   std::vector<Material_property> properties;

   if (begins_with(rendertype, "pbr"sv)) {
      struct PBR_cb {
         glm::vec3 base_color = {1.0f, 1.0f, 1.0f};
         float base_metallicness = 1.0f;
         float base_roughness = 1.0f;
         float ao_strength = 1.0f;
         float emissive_power = 1.0f;
      };

      PBR_cb pbr_cb{};

      std::memcpy(&pbr_cb, constant_buffer.data(),
                  safe_min(sizeof(PBR_cb), constant_buffer.size()));

      properties.reserve(5);

      properties.emplace_back("BaseColor"s, pbr_cb.base_color, glm::vec3{0.0f},
                              glm::vec3{1.0f});
      properties.emplace_back("Metallicness"s, pbr_cb.base_metallicness, 0.0f, 1.0f);
      properties.emplace_back("Roughness"s, pbr_cb.base_roughness, 0.0f, 1.0f);
      properties.emplace_back("AOStrength"s, 1.0f / pbr_cb.ao_strength, 0.0f, 2048.0f);
      properties.emplace_back("EmissivePower"s,
                              std::log2(pbr_cb.emissive_power), 0.0f, 2048.0f);
   }
   else if (begins_with(rendertype, "normal_ext"sv)) {
      struct Normal_ext_cb {
         float disp_scale = 1.0f;
         float disp_offset = 0.5f;
         float material_tess_detail = 32.0f;
         float tess_smoothing_amount = 16.0f;

         glm::vec3 base_diffuse_color = {1.0f, 1.0f, 1.0f};
         float gloss_map_weight = 1.0f;

         glm::vec3 base_specular_color = {1.0f, 1.0f, 1.0f};
         float specular_exponent = 64.0f;

         std::uint32_t use_parallax_occlusion_mapping = false;
         float height_scale = 0.1f;
         std::uint32_t use_detail_textures = false;
         float detail_texture_scale = 1.0f;

         std::uint32_t use_overlay_textures = false;
         float overlay_texture_scale = 1.0f;
         std::uint32_t use_emissive_texture = false;
         float emissive_texture_scale = 1.0f;

         float emissive_power = 1.0f;
      };

      Normal_ext_cb normal_ext_cb{};

      std::memcpy(&normal_ext_cb, constant_buffer.data(),
                  safe_min(sizeof(Normal_ext_cb), constant_buffer.size()));

      properties.reserve(17);

      properties.emplace_back("DiffuseColor"s, normal_ext_cb.base_diffuse_color,
                              glm::vec3{0.0f}, glm::vec3{1.0f});
      properties.emplace_back("GlossMapWeight"s, normal_ext_cb.gloss_map_weight,
                              0.0f, 1.0f);
      properties.emplace_back("SpecularColor"s, normal_ext_cb.base_specular_color,
                              glm::vec3{0.0f}, glm::vec3{1.0f});
      properties.emplace_back("SpecularExponent"s,
                              normal_ext_cb.specular_exponent, 1.0f, 2048.0f);
      properties.emplace_back("UseParallaxOcclusionMapping"s,
                              normal_ext_cb.use_parallax_occlusion_mapping != 0,
                              false, true);
      properties.emplace_back("HeightScale"s, normal_ext_cb.height_scale, 0.0f, 2048.0f);
      properties.emplace_back("UseDetailMaps"s,
                              normal_ext_cb.use_detail_textures != 0, false, true);
      properties.emplace_back("DetailTextureScale"s,
                              normal_ext_cb.detail_texture_scale, 0.0f, 2048.0f);
      properties.emplace_back("UseOverlayMaps"s,
                              normal_ext_cb.use_overlay_textures != 0, false, true);
      properties.emplace_back("OverlayTextureScale"s,
                              normal_ext_cb.overlay_texture_scale, 0.0f, 2048.0f);
      properties.emplace_back("UseEmissiveMap"s,
                              normal_ext_cb.use_emissive_texture != 0, false, true);
      properties.emplace_back("EmissiveTextureScale"s,
                              normal_ext_cb.emissive_texture_scale, 0.0f, 2048.0f);
      properties.emplace_back("EmissivePower"s,
                              std::log2(normal_ext_cb.emissive_power), -2048.0f,
                              2048.0f);
   }
   else {
      throw std::runtime_error{"unexpected rendertype"};
   }

   return properties;
}

auto read_patch_material_impl(ucfb::Reader reader) -> Material_config
{
   Material_config config{};

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read<Material_version>();

   Ensures(version == Material_version::v_1);

   config.name = reader.read_child_strict<"NAME"_mn>().read_string();
   const std::string rendertype = rename_tessellated_rendertype(
      reader.read_child_strict<"RTYP"_mn>().read_string());
   config.overridden_rendertype =
      reader.read_child_strict<"ORTP"_mn>().read<Rendertype>();

   reader.read_child_strict<"CBST"_mn>();

   {
      auto cb__ = reader.read_child_strict<"CB__"_mn>();
      const auto constants = cb__.read_array<std::byte>(cb__.size());

      config.properties = convert_v1_constant_buffer(rendertype, constants);
   }

   const auto material_type = split_string_on(rendertype, "."sv)[0];

   reader.read_child_strict<"VSSR"_mn>();
   reader.read_child_strict<"HSSR"_mn>();
   reader.read_child_strict<"DSSR"_mn>();
   reader.read_child_strict<"GSSR"_mn>();
   config.resources = read_old_resources(reader.read_child_strict<"PSSR"_mn>(),
                                         resource_names_mappings.at(material_type));

   reader.read_child_strict<"FSTX"_mn>().read<std::uint32_t>();

   [[maybe_unused]] const auto [tessellation, tessellation_primitive_topology] =
      reader.read_child_strict<"TESS"_mn>().read_multi<bool, std::uint32_t>();

   parse_rendertype(rendertype, config.properties);
   config.type = material_type;

   return config;
}
}
}
}
