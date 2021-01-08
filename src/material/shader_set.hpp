#pragma once

#include "../core/input_layout_descriptions.hpp"
#include "../core/shader_input_layouts.hpp"
#include "../shader/database.hpp"
#include "com_ptr.hpp"

#include <unordered_map>

#include <d3d11_1.h>

namespace sp::material {

class Shader_set {
public:
   Shader_set(Com_ptr<ID3D11Device1> device, shader::Rendertype& rendertype,
              std::string name) noexcept;

   void update(ID3D11DeviceContext1& dc,
               const core::Input_layout_descriptions& layout_descriptions,
               const std::uint16_t layout_index, const std::string& state_name,
               const shader::Vertex_shader_flags vertex_shader_flags,
               const bool oit_active) noexcept;

private:
   struct Material_vertex_shader {
      Com_ptr<ID3D11VertexShader> vs;

      core::Shader_input_layouts input_layouts;
   };

   struct Material_shader_state {
      std::unordered_map<shader::Vertex_shader_flags, Material_vertex_shader> vertex;
      Com_ptr<ID3D11PixelShader> pixel;
      Com_ptr<ID3D11PixelShader> pixel_oit;

      auto get_vs(const shader::Vertex_shader_flags flags,
                  const std::string& state_name, const std::string& shader_name) noexcept
         -> Material_vertex_shader&;
   };

   using Shaders = std::unordered_map<std::string, Material_shader_state>;

   auto get_state(const std::string& state_name) noexcept -> Material_shader_state&;

   static auto init_shaders(shader::Rendertype& rendertype) noexcept -> Shaders;

   static auto init_state(shader::Rendertype_state& state) noexcept
      -> Material_shader_state;

   static auto init_vs_entrypoint(shader::Rendertype_state& state) noexcept
      -> std::unordered_map<shader::Vertex_shader_flags, Material_vertex_shader>;

   const Com_ptr<ID3D11Device1> _device;

   Shaders _shaders;
   std::string _name;
};

}