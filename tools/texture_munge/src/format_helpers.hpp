#pragma once

#include <cstddef>
#include <string_view>
#include <tuple>

#include <d3d9.h>
#include <dxgiformat.h>

#include <Compressonator.h>
#include <boost/algorithm/string.hpp>

namespace sp {

inline CMP_FORMAT dxgi_format_to_cmp_format(DXGI_FORMAT dxgi_format)
{
   switch (dxgi_format) {
   case DXGI_FORMAT_R16G16B16A16_TYPELESS:
   case DXGI_FORMAT_R16G16B16A16_FLOAT:
   case DXGI_FORMAT_R16G16B16A16_UNORM:
   case DXGI_FORMAT_R16G16B16A16_UINT:
   case DXGI_FORMAT_R16G16B16A16_SNORM:
   case DXGI_FORMAT_R16G16B16A16_SINT:
      return CMP_FORMAT_RGBA_16;
   case DXGI_FORMAT_R8G8B8A8_TYPELESS:
   case DXGI_FORMAT_R8G8B8A8_UNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
   case DXGI_FORMAT_R8G8B8A8_UINT:
   case DXGI_FORMAT_R8G8B8A8_SNORM:
   case DXGI_FORMAT_R8G8B8A8_SINT:
      return CMP_FORMAT_RGBA_8888;
   case DXGI_FORMAT_R16G16_TYPELESS:
   case DXGI_FORMAT_R16G16_FLOAT:
   case DXGI_FORMAT_R16G16_UNORM:
   case DXGI_FORMAT_R16G16_UINT:
   case DXGI_FORMAT_R16G16_SNORM:
   case DXGI_FORMAT_R16G16_SINT:
      return CMP_FORMAT_RG_16;
   case DXGI_FORMAT_R8G8_TYPELESS:
   case DXGI_FORMAT_R8G8_UNORM:
   case DXGI_FORMAT_R8G8_UINT:
   case DXGI_FORMAT_R8G8_SNORM:
   case DXGI_FORMAT_R8G8_SINT:
      return CMP_FORMAT_RG_8;
   case DXGI_FORMAT_R16_TYPELESS:
   case DXGI_FORMAT_R16_FLOAT:
   case DXGI_FORMAT_D16_UNORM:
   case DXGI_FORMAT_R16_UNORM:
   case DXGI_FORMAT_R16_UINT:
   case DXGI_FORMAT_R16_SNORM:
   case DXGI_FORMAT_R16_SINT:
      return CMP_FORMAT_R_16;
   case DXGI_FORMAT_R8_TYPELESS:
   case DXGI_FORMAT_R8_UNORM:
   case DXGI_FORMAT_R8_UINT:
   case DXGI_FORMAT_R8_SNORM:
   case DXGI_FORMAT_R8_SINT:
   case DXGI_FORMAT_A8_UNORM:
      return CMP_FORMAT_R_8;
   case DXGI_FORMAT_BC1_TYPELESS:
   case DXGI_FORMAT_BC1_UNORM:
   case DXGI_FORMAT_BC1_UNORM_SRGB:
      return CMP_FORMAT_BC1;
   case DXGI_FORMAT_BC2_TYPELESS:
   case DXGI_FORMAT_BC2_UNORM:
   case DXGI_FORMAT_BC2_UNORM_SRGB:
      return CMP_FORMAT_BC2;
   case DXGI_FORMAT_BC3_TYPELESS:
   case DXGI_FORMAT_BC3_UNORM:
   case DXGI_FORMAT_BC3_UNORM_SRGB:
      return CMP_FORMAT_BC3;
   case DXGI_FORMAT_BC4_TYPELESS:
   case DXGI_FORMAT_BC4_UNORM:
   case DXGI_FORMAT_BC4_SNORM:
      return CMP_FORMAT_BC4;
   case DXGI_FORMAT_BC5_TYPELESS:
   case DXGI_FORMAT_BC5_UNORM:
   case DXGI_FORMAT_BC5_SNORM:
      return CMP_FORMAT_BC5;
   case DXGI_FORMAT_B8G8R8A8_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM:
   case DXGI_FORMAT_B8G8R8A8_TYPELESS:
   case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
   case DXGI_FORMAT_B8G8R8X8_TYPELESS:
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return CMP_FORMAT_BGRA_8888;
   case DXGI_FORMAT_BC6H_TYPELESS:
   case DXGI_FORMAT_BC6H_UF16:
      return CMP_FORMAT_BC6H;
   case DXGI_FORMAT_BC6H_SF16:
      return CMP_FORMAT_BC6H_SF;
   case DXGI_FORMAT_BC7_TYPELESS:
   case DXGI_FORMAT_BC7_UNORM:
   case DXGI_FORMAT_BC7_UNORM_SRGB:
      return CMP_FORMAT_BC7;
   case DXGI_FORMAT_UNKNOWN:
   case DXGI_FORMAT_R32G32B32A32_TYPELESS:
   case DXGI_FORMAT_R32G32B32A32_FLOAT:
   case DXGI_FORMAT_R32G32B32A32_UINT:
   case DXGI_FORMAT_R32G32B32A32_SINT:
   case DXGI_FORMAT_R32G32B32_TYPELESS:
   case DXGI_FORMAT_R32G32B32_FLOAT:
   case DXGI_FORMAT_R32G32B32_UINT:
   case DXGI_FORMAT_R32G32B32_SINT:
   case DXGI_FORMAT_R32G32_TYPELESS:
   case DXGI_FORMAT_R32G32_FLOAT:
   case DXGI_FORMAT_R32G32_UINT:
   case DXGI_FORMAT_R32G32_SINT:
   case DXGI_FORMAT_R32G8X24_TYPELESS:
   case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
   case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
   case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
   case DXGI_FORMAT_R10G10B10A2_TYPELESS:
   case DXGI_FORMAT_R10G10B10A2_UNORM:
   case DXGI_FORMAT_R10G10B10A2_UINT:
   case DXGI_FORMAT_R11G11B10_FLOAT:
   case DXGI_FORMAT_R32_TYPELESS:
   case DXGI_FORMAT_D32_FLOAT:
   case DXGI_FORMAT_R32_FLOAT:
   case DXGI_FORMAT_R32_UINT:
   case DXGI_FORMAT_R32_SINT:
   case DXGI_FORMAT_R24G8_TYPELESS:
   case DXGI_FORMAT_D24_UNORM_S8_UINT:
   case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
   case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
   case DXGI_FORMAT_R1_UNORM:
   case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
   case DXGI_FORMAT_R8G8_B8G8_UNORM:
   case DXGI_FORMAT_G8R8_G8B8_UNORM:
   case DXGI_FORMAT_B5G6R5_UNORM:
   case DXGI_FORMAT_B5G5R5A1_UNORM:
   case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
   case DXGI_FORMAT_AYUV:
   case DXGI_FORMAT_Y410:
   case DXGI_FORMAT_Y416:
   case DXGI_FORMAT_NV12:
   case DXGI_FORMAT_P010:
   case DXGI_FORMAT_P016:
   case DXGI_FORMAT_420_OPAQUE:
   case DXGI_FORMAT_YUY2:
   case DXGI_FORMAT_Y210:
   case DXGI_FORMAT_Y216:
   case DXGI_FORMAT_NV11:
   case DXGI_FORMAT_AI44:
   case DXGI_FORMAT_IA44:
   case DXGI_FORMAT_P8:
   case DXGI_FORMAT_A8P8:
   case DXGI_FORMAT_B4G4R4A4_UNORM:
   case DXGI_FORMAT_P208:
   case DXGI_FORMAT_V208:
   case DXGI_FORMAT_V408:
   default:
      return CMP_FORMAT_Unknown;
   }
}

inline D3DFORMAT dxgi_format_to_d3dformat(DXGI_FORMAT dxgi_format)
{
   switch (dxgi_format) {
   case DXGI_FORMAT_B8G8R8A8_UNORM:
   case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      return D3DFMT_A8R8G8B8;
   case DXGI_FORMAT_B8G8R8X8_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return D3DFMT_X8R8G8B8;
   case DXGI_FORMAT_B5G6R5_UNORM:
      return D3DFMT_R5G6B5;
   case DXGI_FORMAT_B5G5R5A1_UNORM:
      return D3DFMT_A1R5G5B5;
   case DXGI_FORMAT_B4G4R4A4_UNORM:
      return D3DFMT_A4R4G4B4;
   case DXGI_FORMAT_A8_UNORM:
      return D3DFMT_A8;
   case DXGI_FORMAT_R10G10B10A2_UNORM:
      return D3DFMT_A2B10G10R10;
   case DXGI_FORMAT_R8G8B8A8_UNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      return D3DFMT_A8B8G8R8;
   case DXGI_FORMAT_R16G16_UNORM:
      return D3DFMT_G16R16;
   case DXGI_FORMAT_R16G16B16A16_UNORM:
      return D3DFMT_A16B16G16R16;
   case DXGI_FORMAT_R8_UNORM:
      return D3DFMT_L8;
   case DXGI_FORMAT_R8G8_SNORM:
      return D3DFMT_V8U8;
   case DXGI_FORMAT_R8G8B8A8_SNORM:
      return D3DFMT_Q8W8V8U8;
   case DXGI_FORMAT_R16G16_SNORM:
      return D3DFMT_V16U16;
   case DXGI_FORMAT_G8R8_G8B8_UNORM:
      return D3DFMT_R8G8_B8G8;
   case DXGI_FORMAT_R8G8_B8G8_UNORM:
      return D3DFMT_G8R8_G8B8;
   case DXGI_FORMAT_BC1_UNORM:
   case DXGI_FORMAT_BC1_UNORM_SRGB:
      return D3DFMT_DXT1;
   case DXGI_FORMAT_BC2_UNORM:
   case DXGI_FORMAT_BC2_UNORM_SRGB:
      return D3DFMT_DXT3;
   case DXGI_FORMAT_BC3_UNORM:
   case DXGI_FORMAT_BC3_UNORM_SRGB:
      return D3DFMT_DXT5;
   case DXGI_FORMAT_BC4_UNORM:
      return static_cast<D3DFORMAT>(MAKEFOURCC('A', 'T', 'I', '1'));
   case DXGI_FORMAT_R16_UNORM:
      return D3DFMT_L16;
   case DXGI_FORMAT_R16G16B16A16_SNORM:
      return D3DFMT_Q16W16V16U16;
   case DXGI_FORMAT_R16_FLOAT:
      return D3DFMT_R16F;
   case DXGI_FORMAT_R16G16_FLOAT:
      return D3DFMT_G16R16F;
   case DXGI_FORMAT_R16G16B16A16_FLOAT:
      return D3DFMT_A16B16G16R16F;
   case DXGI_FORMAT_R32_FLOAT:
      return D3DFMT_R32F;
   case DXGI_FORMAT_R32G32_FLOAT:
      return D3DFMT_G32R32F;
   case DXGI_FORMAT_R32G32B32A32_FLOAT:
      return D3DFMT_A32B32G32R32F;
   default:
      return D3DFMT_UNKNOWN;
   }
}

inline auto read_config_format(std::string_view format)
   -> std::tuple<CMP_FORMAT, D3DFORMAT>
{
   using namespace std::literals;

   if (boost::iequals(format, "BC1"sv)) return {CMP_FORMAT_BC1, D3DFMT_DXT1};
   if (boost::iequals(format, "BC3"sv)) return {CMP_FORMAT_BC3, D3DFMT_DXT5};
   if (boost::iequals(format, "BC4"sv)) {
      return {CMP_FORMAT_BC3, static_cast<D3DFORMAT>(MAKEFOURCC('A', 'T', 'I', '1'))};
   }
   if (boost::iequals(format, "ATI2"sv)) {
      return {CMP_FORMAT_ATI2N,
              static_cast<D3DFORMAT>(MAKEFOURCC('A', 'T', 'I', '2'))};
   }

   throw std::invalid_argument{"Invalid config format."s};
}
}
