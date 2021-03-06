#include "../game_support/munged_shader_declarations.hpp"
#include "helpers.hpp"
#include "texture_stage_state_manager.hpp"

#include <algorithm>
#include <gsl/gsl>
#include <iterator>
#include <ranges>
#include <tuple>

namespace sp::d3d9 {

void Texture_stage_state_manager::set(const UINT stage,
                                      const D3DTEXTURESTAGESTATETYPE state,
                                      const DWORD value) noexcept
{
   Expects(stage < _stages.size());

   switch (state) {
   case D3DTSS_COLOROP:
      _stages[stage].colorop = value;
      break;
   case D3DTSS_COLORARG1:
      _stages[stage].colorarg1 = value;
      break;
   case D3DTSS_COLORARG2:
      _stages[stage].colorarg2 = value;
      break;
   case D3DTSS_ALPHAOP:
      _stages[stage].alphaop = value;
      break;
   case D3DTSS_ALPHAARG1:
      _stages[stage].alphaarg1 = value;
      break;
   case D3DTSS_ALPHAARG2:
      _stages[stage].alphaarg2 = value;
      break;
   case D3DTSS_BUMPENVMAT00:
      _stages[stage].bumpenvmat00 = value;
      break;
   case D3DTSS_BUMPENVMAT01:
      _stages[stage].bumpenvmat01 = value;
      break;
   case D3DTSS_BUMPENVMAT10:
      _stages[stage].bumpenvmat10 = value;
      break;
   case D3DTSS_BUMPENVMAT11:
      _stages[stage].bumpenvmat11 = value;
      break;
   case D3DTSS_TEXCOORDINDEX:
      _stages[stage].texcoord_index = value;
      break;
   case D3DTSS_BUMPENVLSCALE:
      _stages[stage].bumpenv_lscale = value;
      break;
   case D3DTSS_BUMPENVLOFFSET:
      _stages[stage].bumpenv_loffset = value;
      break;
   case D3DTSS_TEXTURETRANSFORMFLAGS:
      _stages[stage].texture_transform_flags = value;
      break;
   case D3DTSS_COLORARG0:
      _stages[stage].colorarg0 = value;
      break;
   case D3DTSS_ALPHAARG0:
      _stages[stage].alphaarg0 = value;
      break;
   case D3DTSS_RESULTARG:
      _stages[stage].resultarg = value;
      break;
   case D3DTSS_CONSTANT:
      _stages[stage].constant = value;
      break;
   }
}

DWORD Texture_stage_state_manager::get(const UINT stage,
                                       const D3DTEXTURESTAGESTATETYPE state) const noexcept
{
   Expects(stage < _stages.size());

   switch (state) {
   case D3DTSS_COLOROP:
      return _stages[stage].colorop;
   case D3DTSS_COLORARG1:
      return _stages[stage].colorarg1;
   case D3DTSS_COLORARG2:
      return _stages[stage].colorarg2;
   case D3DTSS_ALPHAOP:
      return _stages[stage].alphaop;
   case D3DTSS_ALPHAARG1:
      return _stages[stage].alphaarg1;
   case D3DTSS_ALPHAARG2:
      return _stages[stage].alphaarg2;
   case D3DTSS_BUMPENVMAT00:
      return _stages[stage].bumpenvmat00;
   case D3DTSS_BUMPENVMAT01:
      return _stages[stage].bumpenvmat01;
   case D3DTSS_BUMPENVMAT10:
      return _stages[stage].bumpenvmat10;
   case D3DTSS_BUMPENVMAT11:
      return _stages[stage].bumpenvmat11;
   case D3DTSS_TEXCOORDINDEX:
      return _stages[stage].texcoord_index;
   case D3DTSS_BUMPENVLSCALE:
      return _stages[stage].bumpenv_lscale;
   case D3DTSS_BUMPENVLOFFSET:
      return _stages[stage].bumpenv_loffset;
   case D3DTSS_TEXTURETRANSFORMFLAGS:
      return _stages[stage].texture_transform_flags;
   case D3DTSS_COLORARG0:
      return _stages[stage].colorarg0;
   case D3DTSS_ALPHAARG0:
      return _stages[stage].alphaarg0;
   case D3DTSS_RESULTARG:
      return _stages[stage].resultarg;
   case D3DTSS_CONSTANT:
      return _stages[stage].constant;
   }

   return 0;
}

void Texture_stage_state_manager::update(core::Shader_patch& shader_patch,
                                         const DWORD texture_factor,
                                         const D3DVIEWPORT9& viewport) const noexcept
{
   if (is_color_fill_state()) {
      shader_patch.set_game_shader(_color_fill_shader);
   }
   else if (is_damage_overlay_state(texture_factor)) {
      shader_patch.set_game_shader(_damage_overlay_shader);
   }
   else if (is_plain_texture_state(texture_factor)) {
      shader_patch.set_game_shader(_plain_texture_shader);
   }
   else if (is_scene_blur_state()) {
      shader_patch.set_game_shader(_scene_blur_shader);
   }
   else if (is_zoom_blur_state()) { // This state is also used for the endgame screen fade.
      shader_patch.set_game_shader(_zoom_blur_shader);
   }
   else {
      log_and_terminate("Unexpected fixed function texture state!");
   }

   shader_patch.set_constants(core::cb::fixedfunction,
                              {.texture_factor = unpack_d3dcolor(texture_factor),
                               .inv_resolution = {1.0f / viewport.Width,
                                                  1.0f / viewport.Height}});
}

void Texture_stage_state_manager::reset() noexcept
{
   _stages = default_stages_state();
}

bool Texture_stage_state_manager::is_scene_blur_state() const noexcept
{
   if (_stages[1].colorop != D3DTOP_DISABLE) return false;
   if (_stages[1].alphaop != D3DTOP_DISABLE) return false;
   if (_stages[0].colorop != D3DTOP_MODULATE) return false;
   if (_stages[0].colorarg1 != D3DTA_TFACTOR) return false;
   if (_stages[0].colorarg2 != D3DTA_TEXTURE) return false;
   if (_stages[0].alphaop != D3DTOP_SELECTARG1) return false;
   if (_stages[0].alphaarg1 != D3DTA_TFACTOR) return false;
   if (_stages[0].alphaarg2 != D3DTA_TEXTURE) return false;

   return true;
}

bool Texture_stage_state_manager::is_zoom_blur_state() const noexcept
{
   if (_stages[2].colorop != D3DTOP_DISABLE) return false;
   if (_stages[2].alphaop != D3DTOP_DISABLE) return false;
   if (_stages[1].colorop != D3DTOP_SELECTARG1) return false;
   if (_stages[1].colorarg1 != D3DTA_CURRENT) return false;
   if (_stages[1].colorarg2 != D3DTA_CURRENT) return false;
   if (_stages[1].alphaop != D3DTOP_MODULATE) return false;
   if (_stages[1].alphaarg1 != D3DTA_TFACTOR) return false;
   if (_stages[1].alphaarg2 != D3DTA_TEXTURE) return false;
   if (_stages[0].colorop != D3DTOP_MODULATE) return false;
   if (_stages[0].colorarg1 != D3DTA_TFACTOR) return false;
   if (_stages[0].colorarg2 != D3DTA_TEXTURE) return false;
   if (_stages[0].alphaop != D3DTOP_MODULATE) return false;
   if (_stages[0].alphaarg1 != D3DTA_TFACTOR) return false;
   if (_stages[0].alphaarg2 != D3DTA_TEXTURE) return false;

   return true;
}

bool Texture_stage_state_manager::is_damage_overlay_state(const DWORD texture_factor) const noexcept
{
   constexpr auto damage_color = 0xdf2020u;

   if (_stages[1].colorop != D3DTOP_DISABLE) return false;
   if (_stages[1].alphaop != D3DTOP_DISABLE) return false;
   if (_stages[0].colorop != D3DTOP_MODULATE) return false;
   if (_stages[0].colorarg1 != D3DTA_TFACTOR) return false;
   if (_stages[0].colorarg2 != D3DTA_TEXTURE) return false;
   if (_stages[0].alphaop != D3DTOP_MODULATE) return false;
   if (_stages[0].alphaarg1 != D3DTA_TFACTOR) return false;
   if (_stages[0].alphaarg2 != D3DTA_TEXTURE) return false;
   if ((texture_factor & 0xffffffu) != damage_color) return false;

   return true;
}

bool Texture_stage_state_manager::is_plain_texture_state(const DWORD texture_factor) const noexcept
{
   if (_stages[1].colorop != D3DTOP_DISABLE) return false;
   if (_stages[1].alphaop != D3DTOP_DISABLE) return false;
   if (_stages[0].colorop != D3DTOP_MODULATE) return false;
   if (_stages[0].colorarg1 != D3DTA_TFACTOR) return false;
   if (_stages[0].colorarg2 != D3DTA_TEXTURE) return false;
   if (_stages[0].alphaop != D3DTOP_MODULATE) return false;
   if (_stages[0].alphaarg1 != D3DTA_TFACTOR) return false;
   if (_stages[0].alphaarg2 != D3DTA_TEXTURE) return false;
   if (texture_factor != D3DCOLOR_ARGB(0xff, 0xff, 0xff, 0xff)) return false;

   return true;
}

bool Texture_stage_state_manager::is_color_fill_state() const noexcept
{
   if (_stages[1].colorop != D3DTOP_DISABLE) return false;
   if (_stages[1].alphaop != D3DTOP_DISABLE) return false;
   if (_stages[0].colorop != D3DTOP_SELECTARG1) return false;
   if (_stages[0].colorarg1 != D3DTA_TFACTOR) return false;
   if (_stages[0].colorarg2 != D3DTA_TEXTURE) return false;
   if (_stages[0].alphaop != D3DTOP_SELECTARG1) return false;
   if (_stages[0].alphaarg1 != D3DTA_TFACTOR) return false;
   if (_stages[0].alphaarg2 != D3DTA_TEXTURE) return false;

   return true;
}

auto Texture_stage_state_manager::get_game_shader_index(Rendertype rendertype,
                                                        const std::string_view shader_name)
   -> std::uint32_t
{
   auto shader_pool =
      game_support::munged_shader_declarations().shader_pool | std::views::reverse |
      std::views::transform([](const game_support::Shader_metadata& metadata) {
         return std::tie(metadata.rendertype, metadata.shader_name);
      });

   if (auto it = std::ranges::find(shader_pool, std::tie(rendertype, shader_name));
       it != shader_pool.end()) {
      const auto reverse_index =
         static_cast<std::uint32_t>(std::distance(shader_pool.begin(), it));

      return shader_pool.size() - 1 - reverse_index;
   }

   std::terminate();
}

}
