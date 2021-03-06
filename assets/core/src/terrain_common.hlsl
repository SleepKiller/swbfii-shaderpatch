#ifndef TERRAIN_COMMON_INCLUDED
#define TERRAIN_COMMON_INCLUDED

#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_utilities.hlsl"

const static bool terrain_common_use_parallax_offset_mapping =
   TERRAIN_COMMON_USE_PARALLAX_OFFSET_MAPPING;
const static bool terrain_common_use_parallax_occlusion_mapping =
   TERRAIN_COMMON_USE_PARALLAX_OCCLUSION_MAPPING;
const static bool terrain_common_basic_blending = TERRAIN_COMMON_BASIC_BLENDING;
const static bool terrain_common_low_detail = TERRAIN_COMMON_LOW_DETAIL;

const static float terrain_low_detail_cull_dist_mult = 1.5;

struct Packed_terrain_vertex {
   int4 position : POSITION;
   unorm float4 normal : NORMAL;
   unorm float4 tangent : TANGENT;
};

struct Terrain_vertex {
   float3 positionWS;
   float3 normalWS;
   float3 color;
   uint3 texture_indices;
   float2 texture_blend;
};

Terrain_vertex unpack_vertex(Packed_terrain_vertex packed, const bool srgb_colors)
{
   Terrain_vertex unpacked;

   unpacked.positionWS = decompress_position((float3)packed.position.xyz);

   unpacked.texture_indices[0] = (packed.position.w) & 0xf;
   unpacked.texture_indices[1] = (packed.position.w >> 4) & 0xf;
   unpacked.texture_indices[2] = (packed.position.w >> 8) & 0xf;

   unpacked.texture_blend[0] = packed.normal.x;
   unpacked.texture_blend[1] = packed.normal.y;

   unpacked.normalWS.xz = packed.normal.zw * (255.0 / 127.0) - (128.0 / 127.0);
   unpacked.normalWS.y =
      sqrt(1.0 - saturate(dot(unpacked.normalWS.xz, unpacked.normalWS.xz)));

   unpacked.color = float3(packed.tangent.rgb);

   if (srgb_colors) unpacked.color = srgb_to_linear(unpacked.color);

   return unpacked;
}

float3x3 terrain_tangent_to_world(const float3 normalWS)
{
   const float3 tangentWS = normalize(cross(normalWS, float3(0.0, 0.0, 1.0)));
   const float3 bitangentWS = normalize(cross(normalWS, float3(-1.0, 0.0, 0.0)));

   return float3x3(tangentWS, bitangentWS, normalWS);
}

class Terrain_parallax_texture : Parallax_input_texture {
   float CalculateLevelOfDetail(SamplerState samp, float2 texcoords)
   {
      return height_maps.CalculateLevelOfDetail(samp, texcoords);
   }

   float SampleLevel(SamplerState samp, float2 texcoords, float mip)
   {
      return height_maps.SampleLevel(samp, float3(texcoords, index), mip);
   }

   float Sample(SamplerState samp, float2 texcoords)
   {
      return height_maps.Sample(samp, float3(texcoords, index));
   }

   float index;
   Texture2DArray<float1> height_maps;
};

void terrain_sample_heightmaps(Texture2DArray<float1> height_maps,
                               const float3 height_scales, const float3 unorm_viewTS,
                               const float3 vertex_texture_blend,
                               inout float3 texcoords[3], out float3 heights)
{
   Terrain_parallax_texture parallax_height_map;
   parallax_height_map.height_maps = height_maps;

   heights = 0.0;

   if (terrain_common_low_detail) return;

   [unroll] for (int i = 0; i < 3; ++i)
   {
      [branch] if (vertex_texture_blend[i] > 0.0 && height_scales[i] > 0.0)
      {
         parallax_height_map.index = texcoords[i].z;

         if (terrain_common_use_parallax_offset_mapping) {
            texcoords[i].xy =
               parallax_offset_map(parallax_height_map, height_scales[i],
                                   texcoords[i].xy, normalize(unorm_viewTS));
         }
         else if (terrain_common_use_parallax_occlusion_mapping) {
            texcoords[i].xy =
               parallax_occlusion_map(parallax_height_map, height_scales[i],
                                      texcoords[i].xy, unorm_viewTS);
         }
      }

      if (!terrain_common_basic_blending) {
         heights[i] = height_maps.Sample(aniso_wrap_sampler, texcoords[i]);
      }
   }
}

float3 terrain_blend_weights(const float3 heights, const float3 vertex_texture_blend)
{
   if (terrain_common_basic_blending) return vertex_texture_blend;

   const float offset = 0.2;

   const float3 h = heights + vertex_texture_blend;
   const float offset_max_h = max(max(h[0], h[1]), h[2]) - offset;

   const float3 b = max(h - offset_max_h, 0.0);

   return (1.0 * b) / max(b[0] + b[1] + b[2], 1e-5);
}

float terrain_far_scene_fade(float4 positionPS)
{
   // We use the previous frames fade values are used for calculating fade in
   // far scene when needed as the current frame's ones are not yet available.
   return -((positionPS.w * terrain_low_detail_cull_dist_mult) * prev_near_fade_scale +
            prev_near_fade_offset);
}

#endif