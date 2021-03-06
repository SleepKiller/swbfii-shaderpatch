#pragma once

#include "../core/d3d11_helpers.hpp"
#include "../core/game_rendertarget.hpp"
#include "../core/texture_database.hpp"
#include "../shader/database.hpp"
#include "../user_config.hpp"
#include "cmaa2.hpp"
#include "color_grading_lut_baker.hpp"
#include "color_grading_regions_blender.hpp"
#include "color_grading_regions_io.hpp"
#include "com_ptr.hpp"
#include "ffx_cas.hpp"
#include "helpers.hpp"
#include "postprocess_params.hpp"
#include "profiler.hpp"
#include "rendertarget_allocator.hpp"

#include <array>
#include <limits>
#include <optional>
#include <random>
#include <string>

#include <glm/glm.hpp>

#include <d3d11_1.h>

namespace sp::effects {

struct Postprocess_input {
   ID3D11ShaderResourceView& srv;

   DXGI_FORMAT format;
   UINT width;
   UINT height;
   UINT sample_count;
};

struct Postprocess_output {
   ID3D11RenderTargetView& rtv;

   UINT width;
   UINT height;
};

class Postprocess {
public:
   Postprocess(Com_ptr<ID3D11Device1> device, shader::Database& shaders);

   void bloom_params(const Bloom_params& params) noexcept;

   auto bloom_params() const noexcept -> Bloom_params
   {
      return _color_grading_regions_blender.global_bloom_params();
   }

   void vignette_params(const Vignette_params& params) noexcept;

   auto vignette_params() const noexcept -> const Vignette_params&
   {
      return _vignette_params;
   }

   void color_grading_params(const Color_grading_params& params) noexcept;

   auto color_grading_params() const noexcept -> Color_grading_params
   {
      return _color_grading_regions_blender.global_cg_params();
   }

   void film_grain_params(const Film_grain_params& params) noexcept;

   auto film_grain_params() const noexcept -> const Film_grain_params&
   {
      return _film_grain_params;
   }

   void color_grading_regions(const Color_grading_regions& colorgrading_regions) noexcept;

   void show_color_grading_regions_imgui(
      const HWND game_window,
      Small_function<Color_grading_params(Color_grading_params) noexcept> show_cg_params_imgui,
      Small_function<Bloom_params(Bloom_params) noexcept> show_bloom_params_imgui) noexcept
   {
      _color_grading_regions_blender.show_imgui(game_window,
                                                std::move(show_cg_params_imgui),
                                                std::move(show_bloom_params_imgui));
   }

   void apply(ID3D11DeviceContext1& dc, Rendertarget_allocator& rt_allocator,
              Profiler& profiler, const core::Shader_resource_database& textures,
              const glm::vec3 camera_position, FFX_cas& ffx_cas,
              const Postprocess_input input, const Postprocess_output output) noexcept;

   void apply(ID3D11DeviceContext1& dc, Rendertarget_allocator& rt_allocator,
              Profiler& profiler, const core::Shader_resource_database& textures,
              const glm::vec3 camera_position, FFX_cas& ffx_cas, CMAA2& cmaa2,
              const Postprocess_input input, const Postprocess_output output) noexcept;

   void hdr_state(Hdr_state state) noexcept;

private:
   void msaa_resolve_input(ID3D11DeviceContext1& dc, const Postprocess_input& input,
                           Rendertarget_allocator::Handle& output,
                           Profiler& profiler) noexcept;

   void linearize_input(ID3D11DeviceContext1& dc, const Postprocess_input& input,
                        Rendertarget_allocator::Handle& output,
                        Profiler& profiler) noexcept;

   void do_bloom_and_color_grading(ID3D11DeviceContext1& dc,
                                   Rendertarget_allocator& allocator,
                                   const core::Shader_resource_database& textures,
                                   const Postprocess_input& input,
                                   const Postprocess_output& output, Profiler& profiler,
                                   ID3D11PixelShader& postprocess_shader,
                                   ID3D11RenderTargetView* luma_rtv = nullptr) noexcept;

   void do_color_grading(ID3D11DeviceContext1& dc,
                         const core::Shader_resource_database& textures,
                         const Postprocess_input& input,
                         const Postprocess_output& output, Profiler& profiler,
                         ID3D11PixelShader& postprocess_shader,
                         ID3D11RenderTargetView* luma_rtv = nullptr) noexcept;

   auto select_msaa_resolve_shader(const Postprocess_input& input) const noexcept
      -> ID3D11PixelShader*;

   void update_and_bind_cb(ID3D11DeviceContext1& dc, const UINT width,
                           const UINT height) noexcept;

   void bind_bloom_cb(ID3D11DeviceContext1& dc, const UINT index) noexcept;

   auto blue_noise_srv(const core::Shader_resource_database& textures) noexcept
      -> ID3D11ShaderResourceView*;

   void update_colorgrading_bloom(ID3D11DeviceContext1& dc,
                                  const glm::vec3 camera_position) noexcept;

   void update_randomness() noexcept;

   void update_shaders() noexcept;

   struct Resolve_constants {
      glm::ivec2 input_range;
      float exposure;
      float _padding{};
   };

   static_assert(sizeof(Resolve_constants) == 16);

   struct Global_constants {
      glm::vec2 scene_pixel_size;

      float vignette_end;
      float vignette_start;

      glm::vec3 bloom_global_scale;
      float bloom_threshold;
      glm::vec3 bloom_dirt_scale;

      float exposure;

      float film_grain_amount;
      float film_grain_aspect;
      float film_grain_color_amount;
      float film_grain_luma_amount;

      glm::vec2 film_grain_size;
      std::array<float, 2> _padding0;

      glm::vec4 randomness_flt;
      glm::uvec4 randomness_uint;
   };

   static_assert(sizeof(Global_constants) == 112);

   struct Bloom_constants {
      glm::vec3 bloom_local_scale;
      float _padding0{};
      glm::vec2 bloom_texel_size;
      std::array<float, 2> _padding1{};
   };

   static_assert(sizeof(Bloom_constants) == 32);

   Global_constants _global_constants;
   std::array<Bloom_constants, 6> _bloom_constants;

   const Com_ptr<ID3D11Device1> _device;
   const Com_ptr<ID3D11BlendState> _additive_blend_state =
      create_additive_blend_state(*_device);
   const Com_ptr<ID3D11SamplerState> _linear_clamp_sampler = [this] {
      const CD3D11_SAMPLER_DESC desc{D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
                                     D3D11_TEXTURE_ADDRESS_CLAMP,
                                     D3D11_TEXTURE_ADDRESS_CLAMP,
                                     D3D11_TEXTURE_ADDRESS_CLAMP,
                                     0.0f,
                                     1,
                                     D3D11_COMPARISON_NEVER,
                                     nullptr,
                                     0.0f,
                                     std::numeric_limits<float>::max()};

      Com_ptr<ID3D11SamplerState> state;

      _device->CreateSamplerState(&desc, state.clear_and_assign());

      return state;
   }();

   const Com_ptr<ID3D11Buffer> _msaa_resolve_constant_buffer =
      core::create_dynamic_constant_buffer(*_device, sizeof(Resolve_constants));
   const Com_ptr<ID3D11Buffer> _global_constant_buffer =
      core::create_dynamic_constant_buffer(*_device, sizeof(Global_constants));
   const std::array<Com_ptr<ID3D11Buffer>, 6> _bloom_constant_buffers{
      core::create_dynamic_constant_buffer(*_device, sizeof(Bloom_constants)),
      core::create_dynamic_constant_buffer(*_device, sizeof(Bloom_constants)),
      core::create_dynamic_constant_buffer(*_device, sizeof(Bloom_constants)),
      core::create_dynamic_constant_buffer(*_device, sizeof(Bloom_constants)),
      core::create_dynamic_constant_buffer(*_device, sizeof(Bloom_constants)),
      core::create_dynamic_constant_buffer(*_device, sizeof(Bloom_constants))};

   shader::Group_pixel& _shaders;

   const Com_ptr<ID3D11VertexShader> _fullscreen_vs;
   const Com_ptr<ID3D11PixelShader> _stock_hdr_to_linear_ps;
   const Com_ptr<ID3D11PixelShader> _msaa_hdr_resolve_x2_ps;
   const Com_ptr<ID3D11PixelShader> _msaa_hdr_resolve_x4_ps;
   const Com_ptr<ID3D11PixelShader> _msaa_hdr_resolve_x8_ps;
   const Com_ptr<ID3D11PixelShader> _msaa_stock_hdr_resolve_x2_ps;
   const Com_ptr<ID3D11PixelShader> _msaa_stock_hdr_resolve_x4_ps;
   const Com_ptr<ID3D11PixelShader> _msaa_stock_hdr_resolve_x8_ps;
   const Com_ptr<ID3D11PixelShader> _bloom_downsample_ps;
   const Com_ptr<ID3D11PixelShader> _bloom_upsample_ps;
   const Com_ptr<ID3D11PixelShader> _bloom_threshold_ps;
   Com_ptr<ID3D11PixelShader> _postprocess_ps;
   Com_ptr<ID3D11PixelShader> _postprocess_cmaa2_pre_ps;
   Com_ptr<ID3D11PixelShader> _postprocess_cmaa2_post_ps;

   Color_grading_lut_baker _color_grading_lut_baker;
   Color_grading_regions_blender _color_grading_regions_blender;

   Vignette_params _vignette_params{};
   Film_grain_params _film_grain_params{};

   Hdr_state _hdr_state = Hdr_state::hdr;

   bool _config_changed = true;
   bool _bloom_enabled = true;
   bool _bloom_use_dirt = false;
   bool _vignette_enabled = true;
   bool _film_grain_enabled = true;
   bool _colored_film_grain_enabled = true;

   std::mt19937 _random_engine{std::random_device{}()};
   std::uniform_real_distribution<float> _random_real_dist{0.0f, 256.0f};
   std::uniform_int<int> _random_int_dist{0, 63};

   std::vector<Com_ptr<ID3D11ShaderResourceView>> _blue_noise_srvs;

   std::string _bloom_dirt_texture_name;

   constexpr static auto global_cb_slot = 0;
   constexpr static auto bloom_cb_slot = 1;

   constexpr static auto scene_texture_slot = 0;
   constexpr static auto bloom_texture_slot = 1;
   constexpr static auto dirt_texture_slot = 2;
   constexpr static auto lut_texture_slot = 3;
   constexpr static auto blue_noise_texture_slot = 4;
};
}
