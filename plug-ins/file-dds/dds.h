/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __DDS_H__
#define __DDS_H__

#define FOURCC(a, b, c, d) \
         ((unsigned int)((unsigned int)(a)      ) | \
                        ((unsigned int)(b) <<  8) | \
                        ((unsigned int)(c) << 16) | \
                        ((unsigned int)(d) << 24))

typedef enum
{
  DDS_COMPRESS_NONE = 0,
  DDS_COMPRESS_BC1,        /* DXT1  */
  DDS_COMPRESS_BC2,        /* DXT3  */
  DDS_COMPRESS_BC3,        /* DXT5  */
  DDS_COMPRESS_BC3N,       /* DXT5n */
  DDS_COMPRESS_BC4,        /* ATI1  */
  DDS_COMPRESS_BC5,        /* ATI2  */
  DDS_COMPRESS_RXGB,       /* DXT5  */
  DDS_COMPRESS_AEXP,       /* DXT5  */
  DDS_COMPRESS_YCOCG,      /* DXT5  */
  DDS_COMPRESS_YCOCGS,     /* DXT5  */
  DDS_COMPRESS_MAX
} DDS_COMPRESSION_TYPE;

typedef enum
{
  DDS_SAVE_SELECTED_LAYER = 0,
  DDS_SAVE_CUBEMAP,
  DDS_SAVE_VOLUMEMAP,
  DDS_SAVE_ARRAY,
  DDS_SAVE_MAX
} DDS_SAVE_TYPE;

typedef enum
{
  DDS_FORMAT_DEFAULT = 0,
  DDS_FORMAT_RGB8,
  DDS_FORMAT_RGBA8,
  DDS_FORMAT_BGR8,
  DDS_FORMAT_ABGR8,
  DDS_FORMAT_R5G6B5,
  DDS_FORMAT_RGBA4,
  DDS_FORMAT_RGB5A1,
  DDS_FORMAT_RGB10A2,
  DDS_FORMAT_R3G3B2,
  DDS_FORMAT_A8,
  DDS_FORMAT_L8,
  DDS_FORMAT_L8A8,
  DDS_FORMAT_AEXP,
  DDS_FORMAT_YCOCG,
  DDS_FORMAT_MAX
} DDS_FORMAT_TYPE;

typedef enum
{
  DDS_MIPMAP_NONE = 0,
  DDS_MIPMAP_GENERATE,
  DDS_MIPMAP_EXISTING,
  DDS_MIPMAP_MAX
} DDS_MIPMAP;

typedef enum
{
  DDS_MIPMAP_FILTER_DEFAULT = 0,
  DDS_MIPMAP_FILTER_NEAREST,
  DDS_MIPMAP_FILTER_BOX,
  DDS_MIPMAP_FILTER_TRIANGLE,
  DDS_MIPMAP_FILTER_QUADRATIC,
  DDS_MIPMAP_FILTER_BSPLINE,
  DDS_MIPMAP_FILTER_MITCHELL,
  DDS_MIPMAP_FILTER_LANCZOS,
  DDS_MIPMAP_FILTER_KAISER,
  DDS_MIPMAP_FILTER_MAX
} DDS_MIPMAP_FILTER;

typedef enum
{
  DDS_MIPMAP_WRAP_DEFAULT = 0,
  DDS_MIPMAP_WRAP_MIRROR,
  DDS_MIPMAP_WRAP_REPEAT,
  DDS_MIPMAP_WRAP_CLAMP,
  DDS_MIPMAP_WRAP_MAX
} DDS_MIPMAP_WRAP;

#define DDS_HEADERSIZE             128
#define DDS_HEADERSIZE_DX10        20

#define DDSD_CAPS                  0x00000001
#define DDSD_HEIGHT                0x00000002
#define DDSD_WIDTH                 0x00000004
#define DDSD_PITCH                 0x00000008
#define DDSD_PIXELFORMAT           0x00001000
#define DDSD_MIPMAPCOUNT           0x00020000
#define DDSD_LINEARSIZE            0x00080000
#define DDSD_DEPTH                 0x00800000

#define DDPF_ALPHAPIXELS           0x00000001
#define DDPF_ALPHA                 0x00000002
#define DDPF_FOURCC                0x00000004
#define DDPF_PALETTEINDEXED8       0x00000020
#define DDPF_RGB                   0x00000040
#define DDPF_LUMINANCE             0x00020000
#define DDPF_NORMAL                0x80000000  // nvidia specific

#define DDSCAPS_COMPLEX            0x00000008
#define DDSCAPS_TEXTURE            0x00001000
#define DDSCAPS_MIPMAP             0x00400000

#define DDSCAPS2_CUBEMAP           0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX 0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX 0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY 0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY 0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ 0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ 0x00008000
#define DDSCAPS2_CUBEMAP_ALL_FACES \
   (DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX | \
    DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGATIVEY | \
    DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ)

#define DDSCAPS2_VOLUME            0x00200000

#define D3D10_RESOURCE_MISC_TEXTURECUBE    0x04
#define D3D10_RESOURCE_DIMENSION_BUFFER    1
#define D3D10_RESOURCE_DIMENSION_TEXTURE1D 2
#define D3D10_RESOURCE_DIMENSION_TEXTURE2D 3
#define D3D10_RESOURCE_DIMENSION_TEXTURE3D 4

typedef struct
{
  unsigned int size;
  unsigned int flags;
  char fourcc[4];
  unsigned int bpp;
  unsigned int rmask;
  unsigned int gmask;
  unsigned int bmask;
  unsigned int amask;
} dds_pixel_format_t;

typedef struct
{
  unsigned int caps1;
  unsigned int caps2;
  unsigned int reserved[2];
} dds_caps_t;

typedef struct
{
  unsigned int magic;
  unsigned int size;
  unsigned int flags;
  unsigned int height;
  unsigned int width;
  unsigned int pitch_or_linsize;
  unsigned int depth;
  unsigned int num_mipmaps;
  union
  {
    struct
    {
      unsigned int magic1;   // FOURCC "GIMP"
      unsigned int magic2;   // FOURCC "-DDS"
      unsigned int version;
      unsigned int extra_fourcc;
    } gimp_dds_special;
    unsigned char pad[4 * 11];
  } reserved;
  dds_pixel_format_t pixelfmt;
  dds_caps_t caps;
  unsigned int reserved2;
} dds_header_t;

typedef enum
{
  DXGI_FORMAT_UNKNOWN                      = 0,
  DXGI_FORMAT_R32G32B32A32_TYPELESS        = 1,
  DXGI_FORMAT_R32G32B32A32_FLOAT           = 2,
  DXGI_FORMAT_R32G32B32A32_UINT            = 3,
  DXGI_FORMAT_R32G32B32A32_SINT            = 4,
  DXGI_FORMAT_R32G32B32_TYPELESS           = 5,
  DXGI_FORMAT_R32G32B32_FLOAT              = 6,
  DXGI_FORMAT_R32G32B32_UINT               = 7,
  DXGI_FORMAT_R32G32B32_SINT               = 8,
  DXGI_FORMAT_R16G16B16A16_TYPELESS        = 9,
  DXGI_FORMAT_R16G16B16A16_FLOAT           = 10,
  DXGI_FORMAT_R16G16B16A16_UNORM           = 11,
  DXGI_FORMAT_R16G16B16A16_UINT            = 12,
  DXGI_FORMAT_R16G16B16A16_SNORM           = 13,
  DXGI_FORMAT_R16G16B16A16_SINT            = 14,
  DXGI_FORMAT_R32G32_TYPELESS              = 15,
  DXGI_FORMAT_R32G32_FLOAT                 = 16,
  DXGI_FORMAT_R32G32_UINT                  = 17,
  DXGI_FORMAT_R32G32_SINT                  = 18,
  DXGI_FORMAT_R32G8X24_TYPELESS            = 19,
  DXGI_FORMAT_D32_FLOAT_S8X24_UINT         = 20,
  DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS     = 21,
  DXGI_FORMAT_X32_TYPELESS_G8X24_UINT      = 22,
  DXGI_FORMAT_R10G10B10A2_TYPELESS         = 23,
  DXGI_FORMAT_R10G10B10A2_UNORM            = 24,
  DXGI_FORMAT_R10G10B10A2_UINT             = 25,
  DXGI_FORMAT_R11G11B10_FLOAT              = 26,
  DXGI_FORMAT_R8G8B8A8_TYPELESS            = 27,
  DXGI_FORMAT_R8G8B8A8_UNORM               = 28,
  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB          = 29,
  DXGI_FORMAT_R8G8B8A8_UINT                = 30,
  DXGI_FORMAT_R8G8B8A8_SNORM               = 31,
  DXGI_FORMAT_R8G8B8A8_SINT                = 32,
  DXGI_FORMAT_R16G16_TYPELESS              = 33,
  DXGI_FORMAT_R16G16_FLOAT                 = 34,
  DXGI_FORMAT_R16G16_UNORM                 = 35,
  DXGI_FORMAT_R16G16_UINT                  = 36,
  DXGI_FORMAT_R16G16_SNORM                 = 37,
  DXGI_FORMAT_R16G16_SINT                  = 38,
  DXGI_FORMAT_R32_TYPELESS                 = 39,
  DXGI_FORMAT_D32_FLOAT                    = 40,
  DXGI_FORMAT_R32_FLOAT                    = 41,
  DXGI_FORMAT_R32_UINT                     = 42,
  DXGI_FORMAT_R32_SINT                     = 43,
  DXGI_FORMAT_R24G8_TYPELESS               = 44,
  DXGI_FORMAT_D24_UNORM_S8_UINT            = 45,
  DXGI_FORMAT_R24_UNORM_X8_TYPELESS        = 46,
  DXGI_FORMAT_X24_TYPELESS_G8_UINT         = 47,
  DXGI_FORMAT_R8G8_TYPELESS                = 48,
  DXGI_FORMAT_R8G8_UNORM                   = 49,
  DXGI_FORMAT_R8G8_UINT                    = 50,
  DXGI_FORMAT_R8G8_SNORM                   = 51,
  DXGI_FORMAT_R8G8_SINT                    = 52,
  DXGI_FORMAT_R16_TYPELESS                 = 53,
  DXGI_FORMAT_R16_FLOAT                    = 54,
  DXGI_FORMAT_D16_UNORM                    = 55,
  DXGI_FORMAT_R16_UNORM                    = 56,
  DXGI_FORMAT_R16_UINT                     = 57,
  DXGI_FORMAT_R16_SNORM                    = 58,
  DXGI_FORMAT_R16_SINT                     = 59,
  DXGI_FORMAT_R8_TYPELESS                  = 60,
  DXGI_FORMAT_R8_UNORM                     = 61,
  DXGI_FORMAT_R8_UINT                      = 62,
  DXGI_FORMAT_R8_SNORM                     = 63,
  DXGI_FORMAT_R8_SINT                      = 64,
  DXGI_FORMAT_A8_UNORM                     = 65,
  DXGI_FORMAT_R1_UNORM                     = 66,
  DXGI_FORMAT_R9G9B9E5_SHAREDEXP           = 67,
  DXGI_FORMAT_R8G8_B8G8_UNORM              = 68,
  DXGI_FORMAT_G8R8_G8B8_UNORM              = 69,
  DXGI_FORMAT_BC1_TYPELESS                 = 70,
  DXGI_FORMAT_BC1_UNORM                    = 71,
  DXGI_FORMAT_BC1_UNORM_SRGB               = 72,
  DXGI_FORMAT_BC2_TYPELESS                 = 73,
  DXGI_FORMAT_BC2_UNORM                    = 74,
  DXGI_FORMAT_BC2_UNORM_SRGB               = 75,
  DXGI_FORMAT_BC3_TYPELESS                 = 76,
  DXGI_FORMAT_BC3_UNORM                    = 77,
  DXGI_FORMAT_BC3_UNORM_SRGB               = 78,
  DXGI_FORMAT_BC4_TYPELESS                 = 79,
  DXGI_FORMAT_BC4_UNORM                    = 80,
  DXGI_FORMAT_BC4_SNORM                    = 81,
  DXGI_FORMAT_BC5_TYPELESS                 = 82,
  DXGI_FORMAT_BC5_UNORM                    = 83,
  DXGI_FORMAT_BC5_SNORM                    = 84,
  DXGI_FORMAT_B5G6R5_UNORM                 = 85,
  DXGI_FORMAT_B5G5R5A1_UNORM               = 86,
  DXGI_FORMAT_B8G8R8A8_UNORM               = 87,
  DXGI_FORMAT_B8G8R8X8_UNORM               = 88,
  DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM   = 89,
  DXGI_FORMAT_B8G8R8A8_TYPELESS            = 90,
  DXGI_FORMAT_B8G8R8A8_UNORM_SRGB          = 91,
  DXGI_FORMAT_B8G8R8X8_TYPELESS            = 92,
  DXGI_FORMAT_B8G8R8X8_UNORM_SRGB          = 93,
  DXGI_FORMAT_BC6H_TYPELESS                = 94,
  DXGI_FORMAT_BC6H_UF16                    = 95,
  DXGI_FORMAT_BC6H_SF16                    = 96,
  DXGI_FORMAT_BC7_TYPELESS                 = 97,
  DXGI_FORMAT_BC7_UNORM                    = 98,
  DXGI_FORMAT_BC7_UNORM_SRGB               = 99,
  DXGI_FORMAT_AYUV                         = 100,
  DXGI_FORMAT_Y410                         = 101,
  DXGI_FORMAT_Y416                         = 102,
  DXGI_FORMAT_NV12                         = 103,
  DXGI_FORMAT_P010                         = 104,
  DXGI_FORMAT_P016                         = 105,
  DXGI_FORMAT_420_OPAQUE                   = 106,
  DXGI_FORMAT_YUY2                         = 107,
  DXGI_FORMAT_Y210                         = 108,
  DXGI_FORMAT_Y216                         = 109,
  DXGI_FORMAT_NV11                         = 110,
  DXGI_FORMAT_AI44                         = 111,
  DXGI_FORMAT_IA44                         = 112,
  DXGI_FORMAT_P8                           = 113,
  DXGI_FORMAT_A8P8                         = 114,
  DXGI_FORMAT_B4G4R4A4_UNORM               = 115,
  DXGI_FORMAT_FORCE_UINT                   = 0xffffffffUL
} DXGI_FORMAT;

typedef struct
{
  DXGI_FORMAT dxgiFormat;
  unsigned int resourceDimension;
  unsigned int miscFlag;
  unsigned int arraySize;
  unsigned int reserved;
} dds_header_dx10_t;

#endif /* __DDS_H__ */
