
#include "bloom.hpp"
#include "../direct3d/shader_constant.hpp"
#include "../imgui/imgui.h"
#include "../shader_constants.hpp"
#include "helpers.hpp"

#include <tuple>

namespace sp::effects {

using namespace std::literals;

namespace {
auto get_texture_metrics(IDirect3DTexture9& from) -> std::tuple<D3DFORMAT, glm::ivec2>
{

   D3DSURFACE_DESC desc;
   from.GetLevelDesc(0, &desc);

   return std::make_tuple(desc.Format, glm::ivec2{static_cast<int>(desc.Width),
                                                  static_cast<int>(desc.Height)});
}

auto get_surface_metrics(IDirect3DSurface9& from) -> std::tuple<D3DFORMAT, glm::ivec2>
{

   D3DSURFACE_DESC desc;
   from.GetDesc(&desc);

   return std::make_tuple(desc.Format, glm::ivec2{static_cast<int>(desc.Width),
                                                  static_cast<int>(desc.Height)});
}

}

Bloom::Bloom(Com_ptr<IDirect3DDevice9> device) : _device{std::move(device)}
{
   params(Bloom_params{});
}

void Bloom::params(const Bloom_params& params) noexcept
{
   _user_params = params;

   _global_scale = params.tint * params.intensity;
   _inner_scale = params.inner_tint * params.inner_scale;
   _inner_mid_scale = (params.inner_mid_scale * params.inner_mid_tint) / _inner_scale;
   _mid_scale = (params.mid_scale * params.mid_tint) / _inner_mid_scale;
   _outer_mid_scale = (params.outer_mid_scale * params.outer_mid_tint) / _mid_scale;
   _outer_scale = (params.outer_scale * params.outer_tint) / _outer_mid_scale;
}

void Bloom::apply(const Shader_group& shaders, Rendertarget_allocator& allocator,
                  IDirect3DTexture9& from_to) noexcept
{
   setup_blur_stage_blendstate();
   set_bloom_constants();
   setup_vetrex_stream();

   if (!_user_params.enabled) return;

   auto [format, res] = get_texture_metrics(from_to);

   setup_buffer_sampler(0);

   auto rt_a_x = allocator.allocate(format, res);
   auto rt_a_y = allocator.allocate(format, res);

   do_pass(*rt_a_x.surface(), from_to, shaders.at("threshold and blur x 13"s));
   do_pass(*rt_a_y.surface(), *rt_a_x.texture(), shaders.at("blur y 13"s));

   auto rt_b_x = allocator.allocate(format, res / 2);
   auto rt_b_y = allocator.allocate(format, res / 2);

   downsample(*rt_b_y.surface(), *rt_a_y.surface());
   do_pass(*rt_b_x.surface(), *rt_b_y.texture(), shaders.at("blur x 13"s));
   do_pass(*rt_b_y.surface(), *rt_b_x.texture(), shaders.at("blur y 13"s));

   auto rt_c_x = allocator.allocate(format, res / 4);
   auto rt_c_y = allocator.allocate(format, res / 4);

   downsample(*rt_c_y.surface(), *rt_b_y.surface());
   do_pass(*rt_c_x.surface(), *rt_c_y.texture(), shaders.at("blur x 17"s));
   do_pass(*rt_c_y.surface(), *rt_c_x.texture(), shaders.at("blur y 17"s));

   auto rt_d_x = allocator.allocate(format, res / 8);
   auto rt_d_y = allocator.allocate(format, res / 8);

   downsample(*rt_d_y.surface(), *rt_c_y.surface());
   do_pass(*rt_d_x.surface(), *rt_d_y.texture(), shaders.at("blur x 25"s));
   do_pass(*rt_d_y.surface(), *rt_d_x.texture(), shaders.at("blur y 25"s));

   auto rt_e_x = allocator.allocate(format, res / 16);
   auto rt_e_y = allocator.allocate(format, res / 16);

   downsample(*rt_e_y.surface(), *rt_d_y.surface());
   do_pass(*rt_e_x.surface(), *rt_e_y.texture(), shaders.at("blur x 25"s));
   do_pass(*rt_e_y.surface(), *rt_e_x.texture(), shaders.at("blur y 25"s));

   setup_combine_stage_blendstate();

   set_scale_constant(_outer_scale);
   do_pass(*rt_d_y.surface(), *rt_e_y.texture(), shaders.at("combine"s));

   set_scale_constant(_outer_mid_scale);
   do_pass(*rt_c_y.surface(), *rt_d_y.texture(), shaders.at("combine"s));

   set_scale_constant(_mid_scale);
   do_pass(*rt_b_y.surface(), *rt_c_y.texture(), shaders.at("combine"s));

   set_scale_constant(_inner_mid_scale);
   do_pass(*rt_a_y.surface(), *rt_b_y.texture(), shaders.at("combine"s));

   Com_ptr<IDirect3DSurface9> target;
   from_to.GetSurfaceLevel(0, target.clear_and_assign());

   set_scale_constant(_inner_scale * _global_scale);
   do_pass(*target, *rt_a_y.texture(), shaders.at("apply"s));
}

void Bloom::drop_device_resources() noexcept
{
   _vertex_decl.reset(nullptr);
   _vertex_buffer.reset(nullptr);
}

void Bloom::show_imgui() noexcept
{
   ImGui::Begin("Bloom", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

   const auto max_float = std::numeric_limits<float>::max();

   if (ImGui::CollapsingHeader("Basic Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Checkbox("Enabled", &_user_params.enabled);

      ImGui::DragFloat("Threshold", &_user_params.threshold, 0.025f);
      ImGui::DragFloat("Intensity", &_user_params.intensity, 0.025f, 0.0f,
                       std::numeric_limits<float>::max());
      ImGui::ColorEdit3("Tint", &_user_params.tint.x, ImGuiColorEditFlags_Float);
   }

   if (ImGui::CollapsingHeader("Individual Scales & Tints")) {
      ImGui::DragFloat("Inner Scale", &_user_params.inner_scale, 0.025f, 0.0f,
                       std::numeric_limits<float>::max());
      ImGui::DragFloat("Inner Mid Scale", &_user_params.inner_mid_scale, 0.025f,
                       0.0f, std::numeric_limits<float>::max());
      ImGui::DragFloat("Mid Scale", &_user_params.mid_scale, 0.025f, 0.0f,
                       std::numeric_limits<float>::max());
      ImGui::DragFloat("Outer Mid Scale", &_user_params.outer_mid_scale, 0.025f,
                       0.0f, std::numeric_limits<float>::max());
      ImGui::DragFloat("Outer Scale", &_user_params.outer_scale, 0.025f, 0.0f,
                       std::numeric_limits<float>::max());

      ImGui::ColorEdit3("Inner Tint", &_user_params.inner_tint.x,
                        ImGuiColorEditFlags_Float);
      ImGui::ColorEdit3("Inner Mid Tint", &_user_params.inner_mid_tint.x,
                        ImGuiColorEditFlags_Float);
      ImGui::ColorEdit3("Mid Tint", &_user_params.mid_tint.x, ImGuiColorEditFlags_Float);
      ImGui::ColorEdit3("Outer Mid Tint", &_user_params.outer_mid_tint.x,
                        ImGuiColorEditFlags_Float);
      ImGui::ColorEdit3("Outer Tint", &_user_params.outer_tint.x,
                        ImGuiColorEditFlags_Float);
   }

   ImGui::Separator();

   if (ImGui::Button("Reset Settings")) _user_params = Bloom_params{};

   if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Reset bloom params to default settings.");
   }

   ImGui::End();

   params(_user_params);
}

void Bloom::do_pass(IDirect3DSurface9& dest, IDirect3DTexture9& source,
                    const Shader_variations& state) const noexcept
{
   _device->SetTexture(0, &source);
   _device->SetRenderTarget(0, &dest);
   set_bloom_pass_state(source, dest);

   state.bind(*_device);

   _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
}

void Bloom::set_bloom_pass_state(IDirect3DTexture9& source,
                                 IDirect3DSurface9& dest) const noexcept
{
   auto dest_size = std::get<glm::ivec2>(get_surface_metrics(dest));

   direct3d::Vs_2f_shader_constant<constants::vs::post_processing_start>{}
      .set(*_device, {-1.0f / dest_size.x, 1.0f / dest_size.y});

   D3DVIEWPORT9 viewport{0,
                         0,
                         static_cast<DWORD>(dest_size.x),
                         static_cast<DWORD>(dest_size.y),
                         0.0f,
                         1.0f};

   _device->SetViewport(&viewport);

   auto src_size =
      static_cast<glm::vec2>(std::get<glm::ivec2>(get_texture_metrics(source)));

   direct3d::Ps_4f_shader_constant<constants::ps::post_processing_start>{}
      .set(*_device, {1.0f / src_size, 0.5f / src_size});
}

void Bloom::downsample(IDirect3DSurface9& dest, IDirect3DSurface9& source) const noexcept
{
   _device->StretchRect(&source, nullptr, &dest, nullptr, D3DTEXF_LINEAR);
}

void Bloom::set_bloom_constants() const noexcept
{
   direct3d::Ps_1f_shader_constant<constants::ps::post_processing_start + 1>{}
      .set(*_device, _user_params.threshold);
}

void Bloom::set_scale_constant(const glm::vec3 scale) const noexcept
{
   direct3d::Ps_3f_shader_constant<constants::ps::post_processing_start + 2>{}.set(*_device,
                                                                                   scale);
}

void Bloom::setup_buffer_sampler(int index) const noexcept
{
   _device->SetSamplerState(index, D3DSAMP_ADDRESSU, D3DPTADDRESSCAPS_CLAMP);
   _device->SetSamplerState(index, D3DSAMP_ADDRESSV, D3DPTADDRESSCAPS_CLAMP);
   _device->SetSamplerState(index, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
   _device->SetSamplerState(index, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
   _device->SetSamplerState(index, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   _device->SetSamplerState(index, D3DSAMP_SRGBTEXTURE, false);
}

void Bloom::setup_blur_stage_blendstate() const noexcept
{
   _device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
   _device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
   _device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
   _device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
}

void Bloom::setup_combine_stage_blendstate() const noexcept
{
   _device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
   _device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
   _device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
   _device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
}

void Bloom::setup_vetrex_stream() noexcept
{
   if (!_vertex_decl || !_vertex_buffer) create_resources();

   _device->SetVertexDeclaration(_vertex_decl.get());
   _device->SetStreamSource(0, _vertex_buffer.get(), 0, fs_triangle_stride);
}

void Bloom::create_resources() noexcept
{
   _vertex_decl = create_fs_triangle_decl(*_device);
   _vertex_buffer = create_fs_triangle_buffer(*_device);
}
}