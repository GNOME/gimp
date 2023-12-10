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

#include <libgimp/gimp.h>

#include "dds.h"
#include "formats.h"


/* Table that determines how uncompressed D3D9 and DXGI formats are read and parsed */
static const fmt_read_info_t format_read_info[] =
{/*|D3D Format           |DXGI Format                           |Order     |Channel Bits |bpp|Alpha |Float |Signed|GIMP Type    */
  { D3DFMT_R8G8B8,        DXGI_FORMAT_UNKNOWN,                  {2,1,0,3}, { 8, 8, 8, 0},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_A8R8G8B8,      DXGI_FORMAT_UNKNOWN,                  {2,1,0,3}, { 8, 8, 8, 8},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_X8R8G8B8,      DXGI_FORMAT_UNKNOWN,                  {2,1,0,3}, { 8, 8, 8, 8},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_R5G6B5,        DXGI_FORMAT_UNKNOWN,                  {2,1,0,3}, { 5, 6, 5, 0},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_X1R5G5B5,      DXGI_FORMAT_UNKNOWN,                  {2,1,0,3}, { 5, 5, 5, 1},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_A1R5G5B5,      DXGI_FORMAT_UNKNOWN,                  {2,1,0,3}, { 5, 5, 5, 1},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_A4R4G4B4,      DXGI_FORMAT_UNKNOWN,                  {2,1,0,3}, { 4, 4, 4, 4},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_R3G3B2,        DXGI_FORMAT_UNKNOWN,                  {2,1,0,3}, { 2, 3, 3, 0},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_A8,            DXGI_FORMAT_UNKNOWN,                  {3,2,1,0}, { 8, 0, 0, 0},  8, TRUE,  FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_A8R3G3B2,      DXGI_FORMAT_UNKNOWN,                  {2,1,0,3}, { 2, 3, 3, 8},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_X4R4G4B4,      DXGI_FORMAT_UNKNOWN,                  {2,1,0,3}, { 4, 4, 4, 4},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_A2B10G10R10,   DXGI_FORMAT_UNKNOWN,                  {2,1,0,3}, {10,10,10, 2}, 16, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_A8B8G8R8,      DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, { 8, 8, 8, 8},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_X8B8G8R8,      DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, { 8, 8, 8, 8},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_G16R16,        DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {16,16, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_A2R10G10B10,   DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {10,10,10, 2}, 16, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_A16B16G16R16,  DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {16,16,16,16}, 16, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_A8P8,          DXGI_FORMAT_UNKNOWN,                  {0,3,2,1}, { 8, 8, 0, 0},  8, TRUE,  FALSE, FALSE, GIMP_INDEXED },
  { D3DFMT_P8,            DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, { 8, 0, 0, 0},  8, FALSE, FALSE, FALSE, GIMP_INDEXED },
  { D3DFMT_L8,            DXGI_FORMAT_UNKNOWN,                  {0,3,2,1}, { 8, 0, 0, 0},  8, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_A8L8,          DXGI_FORMAT_UNKNOWN,                  {0,3,2,1}, { 8, 8, 0, 0},  8, TRUE,  FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_A4L4,          DXGI_FORMAT_UNKNOWN,                  {0,3,2,1}, { 4, 4, 0, 0},  8, TRUE,  FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_V8U8,          DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, { 8, 8, 0, 0},  8, FALSE, FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_L6V5U5,        DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, { 5, 5, 6, 0},  8, FALSE, FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_X8L8V8U8,      DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, { 8, 8, 8, 8},  8, FALSE, FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_Q8W8V8U8,      DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, { 8, 8, 8, 8},  8, TRUE,  FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_V16U16,        DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {16,16, 0, 0}, 16, FALSE, FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_A2W10V10U10,   DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {10,10,10, 2}, 16, TRUE,  FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_D16_LOCKABLE,  DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {16, 0, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_D32,           DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {32, 0, 0, 0}, 32, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_D15S1,         DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {15, 1, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_D24S8,         DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {24, 8, 0, 0}, 32, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_D24X8,         DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {24, 8, 0, 0}, 32, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_D24X4S4,       DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {24, 4, 4, 0}, 32, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_D16,           DXGI_FORMAT_UNKNOWN,                  {1,0,2,3}, {16, 0, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_L16,           DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {16, 0, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_D32F_LOCKABLE, DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {32, 0, 0, 0}, 32, FALSE, TRUE,  TRUE,  GIMP_GRAY    },
  { D3DFMT_D24FS8,        DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {24, 8, 0, 0}, 32, FALSE, TRUE,  TRUE,  GIMP_RGB     },
  { D3DFMT_D32_LOCKABLE,  DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {32, 0, 0, 0}, 32, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_S8_LOCKABLE,   DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, { 8, 0, 0, 0},  8, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_Q16W16V16U16,  DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {16,16,16,16}, 16, TRUE,  FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_R16F,          DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {16, 0, 0, 0}, 16, FALSE, TRUE,  TRUE,  GIMP_GRAY    },
  { D3DFMT_G16R16F,       DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {16,16, 0, 0}, 16, FALSE, TRUE,  TRUE,  GIMP_RGB     },
  { D3DFMT_A16B16G16R16F, DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {16,16,16,16}, 16, TRUE,  TRUE,  TRUE,  GIMP_RGB     },
  { D3DFMT_R32F,          DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {32, 0, 0, 0}, 32, FALSE, TRUE,  TRUE,  GIMP_GRAY    },
  { D3DFMT_G32R32F,       DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {32,32, 0, 0}, 32, FALSE, TRUE,  TRUE,  GIMP_RGB     },
  { D3DFMT_A32B32G32R32F, DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, {32,32,32,32}, 32, TRUE,  TRUE,  TRUE,  GIMP_RGB     },
  { D3DFMT_CxV8U8,        DXGI_FORMAT_UNKNOWN,                  {0,1,2,3}, { 8, 8, 0, 0},  8, FALSE, FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_A1,            DXGI_FORMAT_UNKNOWN,                  {3,2,1,0}, { 1, 0, 0, 0},  8, TRUE,  FALSE, FALSE, GIMP_GRAY    },

  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R32G32B32A32_FLOAT,       {0,1,2,3}, {32,32,32,32}, 32, TRUE,  TRUE,  TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R32G32B32A32_UINT,        {0,1,2,3}, {32,32,32,32}, 32, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R32G32B32A32_SINT,        {0,1,2,3}, {32,32,32,32}, 32, TRUE,  FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R32G32B32_FLOAT,          {0,1,2,3}, {32,32,32, 0}, 32, FALSE, TRUE,  TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R32G32B32_UINT,           {0,1,2,3}, {32,32,32, 0}, 32, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R32G32B32_SINT,           {0,1,2,3}, {32,32,32, 0}, 32, FALSE, FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16G16B16A16_FLOAT,       {0,1,2,3}, {16,16,16,16}, 16, TRUE,  TRUE,  TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16G16B16A16_UNORM,       {0,1,2,3}, {16,16,16,16}, 16, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16G16B16A16_UINT,        {0,1,2,3}, {16,16,16,16}, 16, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16G16B16A16_SNORM,       {0,1,2,3}, {16,16,16,16}, 16, TRUE,  FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16G16B16A16_SINT,        {0,1,2,3}, {16,16,16,16}, 16, TRUE,  FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R32G32_FLOAT,             {0,1,2,3}, {32,32, 0, 0}, 32, FALSE, TRUE,  TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R32G32_UINT,              {0,1,2,3}, {32,32, 0, 0}, 32, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R32G32_SINT,              {0,1,2,3}, {32,32, 0, 0}, 32, FALSE, FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_D32_FLOAT_S8X24_UINT,     {0,1,2,3}, {32, 8,24, 0}, 32, FALSE, TRUE,  TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R10G10B10A2_UNORM,        {0,1,2,3}, {10,10,10, 2}, 16, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R10G10B10A2_UINT,         {0,1,2,3}, {10,10,10, 2}, 16, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R11G11B10_FLOAT,          {0,1,2,3}, {11,11,10, 0}, 16, FALSE, TRUE,  FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8G8B8A8_UNORM,           {0,1,2,3}, { 8, 8, 8, 8},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,      {0,1,2,3}, { 8, 8, 8, 8},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8G8B8A8_UINT,            {0,1,2,3}, { 8, 8, 8, 8},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8G8B8A8_SNORM,           {0,1,2,3}, { 8, 8, 8, 8},  8, TRUE,  FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8G8B8A8_SINT,            {0,1,2,3}, { 8, 8, 8, 8},  8, TRUE,  FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16G16_FLOAT,             {0,1,2,3}, {16,16, 0, 0}, 16, FALSE, TRUE,  TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16G16_UNORM,             {0,1,2,3}, {16,16, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16G16_UINT,              {0,1,2,3}, {16,16, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16G16_SNORM,             {0,1,2,3}, {16,16, 0, 0}, 16, FALSE, FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16G16_SINT,              {0,1,2,3}, {16,16, 0, 0}, 16, FALSE, FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_D32_FLOAT,                {0,1,2,3}, {32, 0, 0, 0}, 32, FALSE, TRUE,  TRUE,  GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R32_FLOAT,                {0,1,2,3}, {32, 0, 0, 0}, 32, FALSE, TRUE,  TRUE,  GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R32_UINT,                 {0,1,2,3}, {32, 0, 0, 0}, 32, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R32_SINT,                 {0,1,2,3}, {32, 0, 0, 0}, 32, FALSE, FALSE, TRUE,  GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_D24_UNORM_S8_UINT,        {0,1,2,3}, {24, 8, 0, 0}, 32, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8G8_UNORM,               {0,1,2,3}, { 8, 8, 0, 0},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8G8_UINT,                {0,1,2,3}, { 8, 8, 0, 0},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8G8_SNORM,               {0,1,2,3}, { 8, 8, 0, 0},  8, FALSE, FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8G8_SINT,                {0,1,2,3}, { 8, 8, 0, 0},  8, FALSE, FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16_FLOAT,                {0,1,2,3}, {16, 0, 0, 0}, 16, FALSE, TRUE,  TRUE,  GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_D16_UNORM,                {0,1,2,3}, {16, 0, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16_UNORM,                {0,1,2,3}, {16, 0, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16_UINT,                 {0,1,2,3}, {16, 0, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16_SNORM,                {0,1,2,3}, {16, 0, 0, 0}, 16, FALSE, FALSE, TRUE,  GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16_SINT,                 {0,1,2,3}, {16, 0, 0, 0}, 16, FALSE, FALSE, TRUE,  GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8_UNORM,                 {0,1,2,3}, { 8, 0, 0, 0},  8, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8_UINT,                  {0,1,2,3}, { 8, 0, 0, 0},  8, FALSE, FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8_SNORM,                 {0,1,2,3}, { 8, 0, 0, 0},  8, FALSE, FALSE, TRUE,  GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R8_SINT,                  {0,1,2,3}, { 8, 0, 0, 0},  8, FALSE, FALSE, TRUE,  GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_A8_UNORM,                 {3,2,1,0}, { 8, 0, 0, 0},  8, TRUE,  FALSE, FALSE, GIMP_GRAY    },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R9G9B9E5_SHAREDEXP,       {0,1,2,3}, { 9, 9, 9, 5}, 32, FALSE, TRUE,  FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_B5G6R5_UNORM,             {0,1,2,3}, { 5, 6, 5, 0},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_B5G5R5A1_UNORM,           {0,1,2,3}, { 5, 5, 5, 1},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_B8G8R8A8_UNORM,           {2,1,0,3}, { 8, 8, 8, 8},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_B8G8R8X8_UNORM,           {2,1,0,3}, { 8, 8, 8, 8},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,      {2,1,0,3}, { 8, 8, 8, 8},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,      {2,1,0,3}, { 8, 8, 8, 8},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_P8,                       {0,1,2,3}, { 8, 0, 0, 0},  8, FALSE, FALSE, FALSE, GIMP_INDEXED },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_A8P8,                     {0,1,2,3}, { 8, 8, 0, 0},  8, TRUE,  FALSE, FALSE, GIMP_INDEXED },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_B4G4R4A4_UNORM,           {2,1,0,3}, { 4, 4, 4, 4},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT,   {0,1,2,3}, {10,10,10, 2}, 32, TRUE,  TRUE,  FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT,   {0,1,2,3}, {10,10,10, 2}, 32, TRUE,  TRUE,  FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_D16_UNORM_S8_UINT,        {0,1,2,3}, {16, 8, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R16_UNORM_X8_TYPELESS,    {0,1,2,3}, {16, 8, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_X16_TYPELESS_G8_UINT,     {0,1,2,3}, {16, 8, 0, 0}, 16, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM, {0,1,2,3}, {10,10,10, 2}, 16, TRUE,  FALSE, TRUE,  GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_R4G4_UNORM,               {0,1,2,3}, { 4, 4, 0, 0},  8, FALSE, FALSE, FALSE, GIMP_RGB     },
  { D3DFMT_UNKNOWN,       DXGI_FORMAT_A4B4G4R4_UNORM,           {3,2,1,0}, { 4, 4, 4, 4},  8, TRUE,  FALSE, FALSE, GIMP_RGB     },
};
#define FORMAT_READ_INFO_COUNT (sizeof (format_read_info) / sizeof (fmt_read_info_t))

/* Table for mapping bpp, mask, and flag values to D3D9 format codes */
static struct _FMT_MAP
{
  D3DFORMAT    d3d9_format;
  gint         bpp;
  guint        rmask;
  guint        gmask;
  guint        bmask;
  guint        amask;
  guint        flags;
} format_map[] =
{/*|D3D Format          |bpp|R Mask     |G Mask     |B Mask     |A Mask     |Flags                                   */
  { D3DFMT_R8G8B8,       24, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, DDPF_RGB                                },
  { D3DFMT_A8R8G8B8,     32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, DDPF_RGB | DDPF_ALPHAPIXELS             },
  { D3DFMT_X8R8G8B8,     32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, DDPF_RGB                                },
  { D3DFMT_R5G6B5,       16, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000, DDPF_RGB                                },
  { D3DFMT_X1R5G5B5,     16, 0x00007C00, 0x000003E0, 0x0000001F, 0x00000000, DDPF_RGB                                },
  { D3DFMT_A1R5G5B5,     16, 0x00007C00, 0x000003E0, 0x0000001F, 0x00008000, DDPF_RGB | DDPF_ALPHAPIXELS             },
  { D3DFMT_A4R4G4B4,     16, 0x00000F00, 0x000000F0, 0x0000000F, 0x0000F000, DDPF_RGB | DDPF_ALPHAPIXELS             },
  { D3DFMT_R3G3B2,        8, 0x000000E0, 0x0000001C, 0x00000003, 0x00000000, DDPF_RGB                                },
  { D3DFMT_A8,            8, 0x00000000, 0x00000000, 0x00000000, 0x000000FF, DDPF_ALPHA                              },
  { D3DFMT_A8R3G3B2,     16, 0x000000E0, 0x0000001C, 0x00000003, 0x0000FF00, DDPF_RGB | DDPF_ALPHAPIXELS             },
  { D3DFMT_X4R4G4B4,     16, 0x00000F00, 0x000000F0, 0x0000000F, 0x00000000, DDPF_RGB                                },
  { D3DFMT_A2B10G10R10,  32, 0x000003FF, 0x000FFC00, 0x3FF00000, 0xC0000000, DDPF_RGB | DDPF_ALPHAPIXELS             },
  { D3DFMT_A8B8G8R8,     32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000, DDPF_RGB | DDPF_ALPHAPIXELS             },
  { D3DFMT_X8B8G8R8,     32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, DDPF_RGB                                },
  { D3DFMT_G16R16,       32, 0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000, DDPF_RGB                                },
  { D3DFMT_A2R10G10B10,  32, 0x3FF00000, 0x000FFC00, 0x000003FF, 0xC0000000, DDPF_RGB | DDPF_ALPHAPIXELS             },
  { D3DFMT_A8P8,         16, 0x00000000, 0x00000000, 0x00000000, 0x0000FF00, DDPF_PALETTEINDEXED8 | DDPF_ALPHAPIXELS },
  { D3DFMT_P8,            8, 0x00000000, 0x00000000, 0x00000000, 0x00000000, DDPF_PALETTEINDEXED8                    },
  { D3DFMT_L8,            8, 0x000000FF, 0x00000000, 0x00000000, 0x00000000, DDPF_LUMINANCE                          },
  { D3DFMT_A8L8,         16, 0x000000FF, 0x00000000, 0x00000000, 0x0000FF00, DDPF_LUMINANCE | DDPF_ALPHAPIXELS       },
  { D3DFMT_A4L4,          8, 0x0000000F, 0x00000000, 0x00000000, 0x000000F0, DDPF_LUMINANCE | DDPF_ALPHAPIXELS       },
  { D3DFMT_UNKNOWN,      16, 0x000000FF, 0x0000FF00, 0x00000000, 0x00000000, DDPF_RGB                                },
  { D3DFMT_V8U8,         16, 0x000000FF, 0x0000FF00, 0x00000000, 0x00000000, DDPF_BUMPDUDV                           },
  { D3DFMT_L6V5U5,       16, 0x0000001F, 0x000003E0, 0x0000FC00, 0x00000000, DDPF_BUMPLUMINANCE                      },
  { D3DFMT_X8L8V8U8,     32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, DDPF_BUMPLUMINANCE                      },
  { D3DFMT_Q8W8V8U8,     32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000, DDPF_BUMPDUDV                           },
  { D3DFMT_V16U16,       32, 0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000, DDPF_BUMPDUDV                           },
  { D3DFMT_A2W10V10U10,  32, 0x3FF00000, 0x000FFC00, 0x000003FF, 0xC0000000, DDPF_BUMPDUDV | DDPF_ALPHAPIXELS        },
  { D3DFMT_D16_LOCKABLE, 16, 0x00000000, 0x0000FFFF, 0x00000000, 0x00000000, DDPF_ZBUFFER                            },
  { D3DFMT_L16,          16, 0x0000FFFF, 0x00000000, 0x00000000, 0x00000000, DDPF_LUMINANCE                          },

  /* NVTT v1 wrote these with DDPF_RGB instead of DDPF_LUMINANCE */
  { D3DFMT_L8,            8, 0x000000FF, 0x00000000, 0x00000000, 0x00000000, DDPF_RGB                                },
  { D3DFMT_A8L8,         16, 0x000000FF, 0x00000000, 0x00000000, 0x0000FF00, DDPF_RGB | DDPF_ALPHAPIXELS             },
  { D3DFMT_L16,          16, 0x0000FFFF, 0x00000000, 0x00000000, 0x00000000, DDPF_RGB                                },
};
#define FORMAT_MAP_COUNT  (sizeof (format_map) / sizeof (format_map[0]))


/*
 * Get D3DFORMAT code that matches input bpp, masks, and flags
 */
guint
get_d3d9format (guint  bpp,
                guint  rmask,
                guint  gmask,
                guint  bmask,
                guint  amask,
                guint  flags)
{
  for (gint i = 0; i < FORMAT_MAP_COUNT; i++)
    {
      if (format_map[i].bpp   == bpp   &&
          format_map[i].rmask == rmask &&
          format_map[i].gmask == gmask &&
          format_map[i].bmask == bmask &&
          format_map[i].amask == amask &&
          (format_map[i].flags & flags) == format_map[i].flags)
        {
          return format_map[i].d3d9_format;
        }
    }
  return D3DFMT_UNKNOWN;
}

/*
 * Check that uncompressed DXGI format is supported
 */
gboolean
dxgiformat_supported (guint hdr_dxgifmt)
{
  for (gint i = 0; i < FORMAT_READ_INFO_COUNT; i++)
    {
      if (format_read_info[i].dxgi_format == hdr_dxgifmt)
        {
          return TRUE;
        }
    }
  return FALSE;
}

/*
 * Get read info from D3D9 or DXGI format code
 */
fmt_read_info_t
get_format_read_info (guint d3d9_fmt,
                      guint dxgi_fmt)
{
  gint index = 0;
  for (gint i = 0; i < FORMAT_READ_INFO_COUNT; i++)
    {
      if ((d3d9_fmt && (format_read_info[i].d3d9_format == d3d9_fmt)) ||
          (dxgi_fmt && (format_read_info[i].dxgi_format == dxgi_fmt)))
        {
          index = i;
          break;
        }
    }
  return format_read_info[index];
}

/*
 * Convert integer component from input size to output size (up to 32 bits)
 * Brute-force high-precision float method for minimum possible error
 */
guint32
requantize_component (guint32 bits,
                      guchar  size_in,
                      guchar  size_out)
{
  if (size_in == size_out)
    return bits;

  return (guint) ((gdouble) bits * ((gdouble) ((1ULL << size_out) - 1)
                                  / (gdouble) ((1ULL << size_in ) - 1)) + 0.5);
}

/* Special float handling for R9G9B9E5
 * https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm
 */
void
float_from_9e5 (guint32 channels[4])
{
  for (gint ch = 0; ch < 3; ch++)
    {
      gfloat val_f = (gfloat) exp2 ((gdouble) ((gint32) channels[3] - 15))
                                 * ((gdouble) channels[ch] / exp2 (9.0));
      memcpy (&channels[ch], &val_f, 4);
    }
}

/* Special float handling for R10G10B10_7E3_A2
 * https://github.com/microsoft/DirectXTex/blob/main/DirectXTex/DirectXTexConvert.cpp
 */
void
float_from_7e3a2 (guint32 channels[4])
{
  gint64 mantissa;
  gint64 exponent;
  gfloat alpha;

  for (gint ch = 0; ch < 3; ch++)
    {
      mantissa = channels[ch] & 0x0000007F;
      exponent = channels[ch] & 0x00000380;

      if (exponent != 0)  /* The value is normalized */
        {
          exponent = (channels[ch] >> 7) & 0x00000007;
        }
      else if (mantissa != 0)  /* The value is denormalized */
        {
          /* Normalize the value in the resulting float */
          exponent = 1;

          do
            {
              exponent--;
              mantissa <<= 1;
            }
          while ((mantissa & 0x80) == 0);

          mantissa &= 0x7F;
        }
      else  /* The value is zero */
        {
          exponent = -124;
        }

      channels[ch] = (guint32) (((exponent + 124) << 23) | (mantissa << 16));
    }
  alpha = (gfloat) channels[3] * (1.0f / 3.0f);
  memcpy (&channels[3], &alpha, 4);
}

/* Special float handling for R10G10B10_6E4_A2
 * https://github.com/microsoft/DirectXTex/blob/main/DirectXTex/DirectXTexConvert.cpp
 */
void
float_from_6e4a2 (guint32 channels[4])
{
  gint64 mantissa;
  gint64 exponent;
  gfloat alpha;

  for (gint ch = 0; ch < 3; ch++)
    {
      mantissa = channels[ch] & 0x0000003F;
      exponent = channels[ch] & 0x000003C0;

      if (exponent != 0)  /* The value is normalized */
        {
          exponent = (channels[ch] >> 6) & 0x0000000F;
        }
      else if (mantissa != 0)  /* The value is denormalized */
        {
          /* Normalize the value in the resulting float */
          exponent = 1;

          do
            {
              exponent--;
              mantissa <<= 1;
            }
          while ((mantissa & 0x40) == 0);

          mantissa &= 0x3F;
        }
      else  /* The value is zero */
        {
          exponent = -120;
        }

      channels[ch] = (guint32) (((exponent + 120) << 23) | (mantissa << 17));
    }
  alpha = (gfloat) channels[3] * (1.0f / 3.0f);
  memcpy (&channels[3], &alpha, 4);
}

/* Special handling for CxV8U8
 * Z-component is reconstructed from X and Y
 */
void
reconstruct_z (guint32 channels[4])
{
  gchar  ch_u, ch_v, ch_c;
  gfloat ch_uf, ch_vf;

  memcpy (&ch_u, &channels[0], 1);
  memcpy (&ch_v, &channels[1], 1);

  ch_uf = (gfloat) ch_u / 127.0f;
  ch_vf = (gfloat) ch_v / 127.0f;
  ch_c =  (gchar) (sqrtf (1.0f - (ch_uf * ch_uf + ch_vf * ch_vf)) * 127.0f);

  memcpy (&channels[2], &ch_c, 1);
}

/*
 * Get input bits-per-pixel from D3D9 format
 */
gsize
get_bpp_d3d9 (guint fmt)
{
  switch (fmt)
    {
    case D3DFMT_A32B32G32R32F:
      return 128;

    case D3DFMT_A16B16G16R16:
    case D3DFMT_Q16W16V16U16:
    case D3DFMT_A16B16G16R16F:
    case D3DFMT_G32R32F:
      return 64;

    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_A2B10G10R10:
    case D3DFMT_A8B8G8R8:
    case D3DFMT_X8B8G8R8:
    case D3DFMT_G16R16:
    case D3DFMT_A2R10G10B10:
    case D3DFMT_Q8W8V8U8:
    case D3DFMT_V16U16:
    case D3DFMT_X8L8V8U8:
    case D3DFMT_A2W10V10U10:
    case D3DFMT_D32:
    case D3DFMT_D24S8:
    case D3DFMT_D24X8:
    case D3DFMT_D24X4S4:
    case D3DFMT_D32F_LOCKABLE:
    case D3DFMT_D24FS8:
    case D3DFMT_INDEX32:
    case D3DFMT_G16R16F:
    case D3DFMT_R32F:
    case D3DFMT_D32_LOCKABLE:
      return 32;

    case D3DFMT_R8G8B8:
      return 24;

    case D3DFMT_A4R4G4B4:
    case D3DFMT_X4R4G4B4:
    case D3DFMT_R5G6B5:
    case D3DFMT_L16:
    case D3DFMT_A8L8:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
    case D3DFMT_A8R3G3B2:
    case D3DFMT_V8U8:
    case D3DFMT_CxV8U8:
    case D3DFMT_L6V5U5:
    case D3DFMT_D16_LOCKABLE:
    case D3DFMT_D15S1:
    case D3DFMT_D16:
    case D3DFMT_INDEX16:
    case D3DFMT_R16F:
      return 16;

    case D3DFMT_R3G3B2:
    case D3DFMT_A8:
    case D3DFMT_A8P8:
    case D3DFMT_P8:
    case D3DFMT_L8:
    case D3DFMT_A4L4:
    case D3DFMT_S8_LOCKABLE:
      return 8;

    case D3DFMT_A1:
      return 1;

    default:
      return 0;
  }
}

/*
 * Get input bits-per-pixel from DXGI format
 */
gsize
get_bpp_dxgi (guint fmt)
{
  switch (fmt)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
      return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
      return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
      return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
    case DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT:
    case DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:
    case DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
      return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
    case DXGI_FORMAT_D16_UNORM_S8_UINT:
    case DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X16_TYPELESS_G8_UINT:
    case DXGI_FORMAT_V408:
      return 24;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
    case DXGI_FORMAT_P208:
    case DXGI_FORMAT_V208:
    case DXGI_FORMAT_A4B4G4R4_UNORM:
      return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
      return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
    case DXGI_FORMAT_R4G4_UNORM:
      return 8;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
      return 4;

    case DXGI_FORMAT_R1_UNORM:
      return 1;

    default:
      return 0;
    }
}
