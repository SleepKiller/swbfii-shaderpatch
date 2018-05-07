#pragma once

#include "../shader_database.hpp"
#include "rendertarget_allocator.hpp"

#include <optional>
#include <string>

#include <glm/glm.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

#include <d3d9.h>

namespace sp::effects {

struct Bloom_params {
   bool enabled = true;

   float threshold = 1.0f;

   float intensity = 1.0f;
   glm::vec3 tint{1.0f, 1.0f, 1.0f};

   float inner_scale = 1.0f;
   glm::vec3 inner_tint{1.0f, 1.0f, 1.0f};

   float inner_mid_scale = 1.0f;
   glm::vec3 inner_mid_tint{1.0f, 1.0f, 1.0f};

   float mid_scale = 1.0f;
   glm::vec3 mid_tint{1.0f, 1.0f, 1.0f};

   float outer_mid_scale = 1.0f;
   glm::vec3 outer_mid_tint{1.0f, 1.0f, 1.0f};

   float outer_scale = 1.0f;
   glm::vec3 outer_tint{1.0f, 1.0f, 1.0f};
};

class Bloom {
public:
   Bloom(Com_ptr<IDirect3DDevice9> device);

   void params(const Bloom_params& params) noexcept;

   auto params() const noexcept -> const Bloom_params&
   {
      return _user_params;
   }

   void apply(const Shader_group& shaders, Rendertarget_allocator& allocator,
              IDirect3DTexture9& from_to) noexcept;

   void drop_device_resources() noexcept;

   void show_imgui() noexcept;

private:
   void do_pass(IDirect3DSurface9& dest, IDirect3DTexture9& source,
                const Shader_variations& state) const noexcept;

   void set_bloom_pass_state(IDirect3DTexture9& source, IDirect3DSurface9& dest) const
      noexcept;

   void downsample(IDirect3DSurface9& dest, IDirect3DSurface9& source) const noexcept;

   void set_bloom_constants() const noexcept;

   void set_scale_constant(const glm::vec3 scale) const noexcept;

   void setup_buffer_sampler(int index) const noexcept;

   void setup_vetrex_stream() noexcept;

   void setup_blur_stage_blendstate() const noexcept;

   void setup_combine_stage_blendstate() const noexcept;

   void create_resources() noexcept;

   Com_ptr<IDirect3DDevice9> _device;
   Com_ptr<IDirect3DVertexDeclaration9> _vertex_decl;
   Com_ptr<IDirect3DVertexBuffer9> _vertex_buffer;

   glm::vec3 _global_scale{1.0f, 1.0f, 1.0f};
   glm::vec3 _inner_scale{1.0f, 1.0f, 1.0f};
   glm::vec3 _inner_mid_scale{1.0f, 1.0f, 1.0f};
   glm::vec3 _mid_scale{1.0f, 1.0f, 1.0f};
   glm::vec3 _outer_mid_scale{1.0f, 1.0f, 1.0f};
   glm::vec3 _outer_scale{1.0f, 1.0f, 1.0f};

   Bloom_params _user_params{};
};

}

namespace YAML {
template<>
struct convert<sp::effects::Bloom_params> {
   static Node encode(const sp::effects::Bloom_params& params)
   {
      using namespace std::literals;

      YAML::Node node;

      node["Enable"s] = params.enabled;

      node["Threshold"s] = params.threshold;
      node["Intensity"s] = params.intensity;

      node["Tint"s].push_back(params.tint.r);
      node["Tint"s].push_back(params.tint.g);
      node["Tint"s].push_back(params.tint.b);

      node["InnerScale"s] = params.inner_scale;
      node["InnerTint"s].push_back(params.inner_tint.r);
      node["InnerTint"s].push_back(params.inner_tint.g);
      node["InnerTint"s].push_back(params.inner_tint.b);

      node["InnerMidScale"s] = params.inner_mid_scale;
      node["InnerMidTint"s].push_back(params.inner_mid_tint.r);
      node["InnerMidTint"s].push_back(params.inner_mid_tint.g);
      node["InnerMidTint"s].push_back(params.inner_mid_tint.b);

      node["MidScale"s] = params.mid_scale;
      node["MidTint"s].push_back(params.mid_tint.r);
      node["MidTint"s].push_back(params.mid_tint.g);
      node["MidTint"s].push_back(params.mid_tint.b);

      node["OuterMidScale"s] = params.outer_mid_scale;
      node["OuterMidTint"s].push_back(params.outer_mid_tint.r);
      node["OuterMidTint"s].push_back(params.outer_mid_tint.g);
      node["OuterMidTint"s].push_back(params.outer_mid_tint.b);

      node["OuterScale"s] = params.outer_scale;
      node["OuterTint"s].push_back(params.outer_tint.r);
      node["OuterTint"s].push_back(params.outer_tint.g);
      node["OuterTint"s].push_back(params.outer_tint.b);

      return node;
   }

   static bool decode(const Node& node, sp::effects::Bloom_params& params)
   {
      using namespace std::literals;

      params = sp::effects::Bloom_params{};

      params.enabled = node["Enable"s];

      params.threshold = node["Threshold"s].as<float>();
      params.intensity = node["Intensity"s].as<float>();

      params.tint[0] = node["Tint"s][0].as<float>();
      params.tint[1] = node["Tint"s][1].as<float>();
      params.tint[2] = node["Tint"s][2].as<float>();

      params.inner_scale = node["InnerScale"s].as<float>();
      params.inner_tint[0] = node["InnerTint"s][0].as<float>();
      params.inner_tint[1] = node["InnerTint"s][1].as<float>();
      params.inner_tint[2] = node["InnerTint"s][2].as<float>();

      params.inner_mid_scale = node["InnerMidScale"s].as<float>();
      params.inner_mid_tint[0] = node["InnerMidTint"s][0].as<float>();
      params.inner_mid_tint[1] = node["InnerMidTint"s][1].as<float>();
      params.inner_mid_tint[2] = node["InnerMidTint"s][2].as<float>();

      params.mid_scale = node["MidScale"s].as<float>();
      params.mid_tint[0] = node["MidTint"s][0].as<float>();
      params.mid_tint[1] = node["MidTint"s][1].as<float>();
      params.mid_tint[2] = node["MidTint"s][2].as<float>();

      params.outer_mid_scale = node["OuterMidScale"s].as<float>();
      params.outer_mid_tint[0] = node["OuterMidTint"s][0].as<float>();
      params.outer_mid_tint[1] = node["OuterMidTint"s][1].as<float>();
      params.outer_mid_tint[2] = node["OuterMidTint"s][2].as<float>();

      params.outer_scale = node["OuterScale"s].as<float>();
      params.outer_tint[0] = node["OuterTint"s][0].as<float>();
      params.outer_tint[1] = node["OuterTint"s][1].as<float>();
      params.outer_tint[2] = node["OuterTint"s][2].as<float>();

      return true;
   }
};
}