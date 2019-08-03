/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * jpeg-quality.c
 * Copyright (C) 2007 Raphaël Quinet <raphael@gimp.org>
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

#include "config.h"

#include <glib/gstdio.h>

#include <jpeglib.h>

#include "jpeg-quality.h"

/*
 * Note that although 0 is a valid quality for IJG's JPEG library, the
 * baseline quantization tables for quality 0 and 1 are identical (all
 * divisors are set to the maximum value 255).  So 0 can be used here
 * to flag unusual settings.
 */

/* sum of the luminance divisors for quality = 0..100 (IJG standard tables) */
static gint std_luminance_sum[101] =
{
  G_MAXINT,
  16320, 16315, 15946, 15277, 14655, 14073, 13623, 13230, 12859, 12560, 12240,
  11861, 11456, 11081, 10714, 10360, 10027,  9679,  9368,  9056,  8680,  8331,
   7995,  7668,  7376,  7084,  6823,  6562,  6345,  6125,  5939,  5756,  5571,
   5421,  5240,  5086,  4976,  4829,  4719,  4616,  4463,  4393,  4280,  4166,
   4092,  3980,  3909,  3835,  3755,  3688,  3621,  3541,  3467,  3396,  3323,
   3247,  3170,  3096,  3021,  2952,  2874,  2804,  2727,  2657,  2583,  2509,
   2437,  2362,  2290,  2211,  2136,  2068,  1996,  1915,  1858,  1773,  1692,
   1620,  1552,  1477,  1398,  1326,  1251,  1179,  1109,  1031,   961,   884,
    814,   736,   667,   592,   518,   441,   369,   292,   221,   151,    86,
     64
};

/* sum of the chrominance divisors for quality = 0..100 (IJG standard tables) */
static gint std_chrominance_sum[101] =
{
  G_MAXINT,
  16320, 16320, 16320, 16218, 16010, 15731, 15523, 15369, 15245, 15110, 14985,
  14864, 14754, 14635, 14526, 14429, 14346, 14267, 14204, 13790, 13121, 12511,
  11954, 11453, 11010, 10567, 10175,  9787,  9455,  9122,  8844,  8565,  8288,
   8114,  7841,  7616,  7447,  7227,  7060,  6897,  6672,  6562,  6396,  6226,
   6116,  5948,  5838,  5729,  5614,  5505,  5396,  5281,  5172,  5062,  4947,
   4837,  4726,  4614,  4506,  4395,  4282,  4173,  4061,  3950,  3839,  3727,
   3617,  3505,  3394,  3284,  3169,  3060,  2949,  2836,  2780,  2669,  2556,
   2445,  2336,  2221,  2111,  2000,  1888,  1778,  1666,  1555,  1444,  1332,
   1223,  1110,   999,   891,   779,   668,   558,   443,   333,   224,   115,
     64
};

/**
 * jpeg_detect_quality:
 * @cinfo: a pointer to a JPEG decompressor info.
 *
 * Returns the exact or estimated quality value that was used to save
 * the JPEG image by analyzing the quantization table divisors.
 *
 * If an exact match for the IJG quantization tables is found, then a
 * quality setting in the range 1..100 is returned.  If the quality
 * can only be estimated, then a negative number in the range -1..-100
 * is returned; its absolute value represents the maximum IJG quality
 * setting to use.  If the quality cannot be reliably determined, then
 * 0 is returned.
 *
 * This function must be called after jpeg_read_header() so that
 * @cinfo contains the quantization tables read from the DQT markers
 * in the file.
 *
 * Returns: the JPEG quality setting in the range 1..100, -1..-100 or 0.
 */
gint
jpeg_detect_quality (struct jpeg_decompress_struct *cinfo)
{
  gint t;
  gint i;
  gint sum[3];
  gint q;

  /* files using CMYK or having 4 quantization tables are unusual */
  if (!cinfo || cinfo->output_components > 3 || cinfo->quant_tbl_ptrs[3])
    return 0;

  /* Most files use table 0 for luminance divisors (Y) and table 1 for
   * chrominance divisors (Cb and Cr).  Some files use separate tables
   * for Cb and Cr, so table 2 may also be used.
   */
  for (t = 0; t < 3; t++)
    {
      sum[t] = 0;
      if (cinfo->quant_tbl_ptrs[t])
        for (i = 0; i < DCTSIZE2; i++)
          sum[t] += cinfo->quant_tbl_ptrs[t]->quantval[i];
    }

  if (cinfo->output_components > 1)
    {
      gint sums;

      if (sum[0] < 64 || sum[1] < 64)
        return 0;

      /* compare with the chrominance table having the lowest sum */
      if (sum[1] < sum[2] || sum[2] <= 0)
        sums = sum[0] + sum[1];
      else
        sums = sum[0] + sum[2];

      q = 100;
      while (sums > std_luminance_sum[q] + std_chrominance_sum[q])
        q--;

      if (sum[0] == std_luminance_sum[q] && sum[1] == std_chrominance_sum[q])
        return q;
      else
        return -q;
    }
  else
    {
      if (sum[0] < 64)
        return 0;

      q = 100;
      while (sum[0] > std_luminance_sum[q])
        q--;

      if (sum[0] == std_luminance_sum[q])
        return q;
      else
        return -q;
    }
}
