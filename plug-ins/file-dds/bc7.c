/*
 * DDS GIMP plugin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1301, USA.
 */

/* ImageMagick's implementation of BC7 was referenced for our implementation.
 * The relevant commit: https://github.com/ImageMagick/ImageMagick/pull/4126/files
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#include <libgimp/gimp.h>

#include "bc7.h"

#define SWAP(a, b)  do { typeof(a) t; t = a; a = b; b = t; } while(0)

static guchar    get_bits             (const guchar *block,
                                       guchar       *start_bit,
                                       guchar        length);

static void      decode_rgba_channels (BC7_colors   *colors,
                                       guchar       *src,
                                       guint         mode,
                                       guchar       *start);

static gboolean  is_pixel_anchor      (guchar        subset_index,
                                       guint         precision,
                                       guchar        pixel_index,
                                       guint         partition_id);


gint
bc7_decompress (guchar *src,
                guint   size,
                guchar *block)
{
  /* BC7 blocks are always 16 bytes */
  guchar     s[16];
  BC7_colors colors;
  guchar     rgb_indexes[16];
  guchar     alpha_indexes[16];
  guchar     subset_indexes[16];
  guchar     rgba[4];
  guint      mode             = 0;
  guint      subset_count     = 0;
  guint      partition_id     = 0;
  guint      swap             = 0;
  guint      selector         = 0;
  guchar     current_bit      = 0;
  guint      i_precision      = 0;
  guint      no_sel_precision = 0;

  for (gint i = 0; i < 16; i++)
    s[i] = src[i];

  /* BC7 blocks are read from the last bit of the last byte, backwards.
   * The mode is determined by the first 1 bit encountered. For instance,
   * 1 is mode 0, 10 is mode 1, 100 is mode 2, and so on. */
  while (current_bit <= 8 && ! get_bits (s, &current_bit, 1))
    {
      continue;
    }
  mode = current_bit - 1;

  if (mode > 7)
    return 0;

  /* For modes that support partitions, we start counting
   * from left of the mode bit, and get the partition ID.
   * The size of the partition ID is defined by the mode. */
  subset_count = mode_info[mode].subset_count;
  if (subset_count > 1)
    {
      partition_id = get_bits (s, &current_bit,
                               mode_info[mode].partition_bits);

      if (partition_id > 63)
        return 0;
    }

  /* Mode 4 and 5 might swap channels for better compression. 2 bits after the mode bit
   * indicate which were swapped (if any) */
  if (mode == 4 || mode == 5)
    swap = get_bits (s, &current_bit, 2);

  /* Mode 4 also has a single selector bit, to the left of its swap bits.
   * This bit determines where the alpha channel indexes are located. */
  if (mode == 4)
    selector = get_bits (s, &current_bit, 1);

  /* Channel values are stored in RnGnBn(An) format, where the mode determines
   * how many bits of precision and how many values are stored. */
  decode_rgba_channels (&colors, s, mode, &current_bit);

  /* Next, we get the indexes to assemble pixels from the channel values. */
  i_precision      = mode_info[mode].index_precision;
  no_sel_precision = mode_info[mode].no_sel_precision;

  if (mode == 4 && selector)
    {
      i_precision      = 3;
      alpha_indexes[0] = get_bits (s, &current_bit, 1);

      for (gint i = 1; i < 16; i++)
        alpha_indexes[i] = get_bits (s, &current_bit, 2);
    }

  /* First, calculate the RGB channel indexes based on the partition ID
   * and the number of subsets.  */
  for (gint i = 0; i < 16; i++)
    {
      guint precision = i_precision;

      if (subset_count == 2)
        subset_indexes[i] = partition_table[0][partition_id][i];
      else if (subset_count == 3)
        subset_indexes[i] = partition_table[1][partition_id][i];
      else
        subset_indexes[i] = 0;

      if (is_pixel_anchor (subset_indexes[i], subset_count, i, partition_id))
        precision--;

      rgb_indexes[i]= get_bits (s, &current_bit, precision);
    }

  if (mode == 5 || (mode == 4 && ! selector))
    {
      alpha_indexes[0] = get_bits (s, &current_bit, no_sel_precision - 1);

      for (gint i = 1; i < 16; i++)
        alpha_indexes[i] = get_bits (s, &current_bit, no_sel_precision);
    }

  /* Create pixels from subset indexes */
  for (gint i = 0; i < 16; i++)
    {
      guint  weight;
      guint  c0 = 2 * subset_indexes[i];
      guint  c1 = (2 * subset_indexes[i]) + 1;

      if (i_precision == 2)
        weight = weight_2[rgb_indexes[i]];
      else if (i_precision == 3)
        weight = weight_3[rgb_indexes[i]];
      else
        weight = weight_4[rgb_indexes[i]];

      rgba[0] = ((64 - weight) * colors.r[c0] + weight * colors.r[c1] + 32) >> 6;
      rgba[1] = ((64 - weight) * colors.g[c0] + weight * colors.g[c1] + 32) >> 6;
      rgba[2] = ((64 - weight) * colors.b[c0] + weight * colors.b[c1] + 32) >> 6;

      if (mode == 4 || mode == 5)
        {
          weight = weight_2[alpha_indexes[i]];

          if (mode == 4 && ! selector)
            weight = weight_3[alpha_indexes[i]];
        }
      rgba[3] = ((64 - weight) * colors.a[c0] + weight * colors.a[c1] + 32) >> 6;

      switch (swap)
        {
          case 1:
            SWAP (rgba[3], rgba[0]);
            break;
          case 2:
            SWAP (rgba[3], rgba[1]);
            break;
          case 3:
            SWAP (rgba[3], rgba[2]);
            break;
          default:
            break;
         }

      for (gint j = 0; j < 4; j++)
        block[(i * 4) + j] = rgba[j];
    }

  return 1;
}


/* Private Functions */

static guchar
get_bits (const guchar *block,
          guchar       *start_bit,
          guchar        length)
{
  guchar bits;
  guint  index;
  guint  base;
  guint  first_bits;
  guint  next_bits;

  index = (*start_bit) >> 3;
  base  = (*start_bit) - (index << 3);

  if (index > 15)
    return 0;

  if (base + length > 8)
    {
      first_bits = 8 - base;
      next_bits  = length - first_bits;

      bits = block[index] >> base;
      bits |= (block[index + 1] & ((1 << next_bits) - 1)) << first_bits;
    }
  else
    {
      bits = (block[index] >> base) & ((1 << length) - 1);
    }
  (*start_bit) += length;

  return bits;
}

static void
decode_rgba_channels (BC7_colors *colors,
                      guchar     *src,
                      guint       mode,
                      guchar     *start)
{
  guint channel_count   = mode_info[mode].subset_count * 2;
  guint rgb_precision   = mode_info[mode].rgb_precision;
  guint alpha_precision = mode_info[mode].alpha_precision;

  /* Get RGB channel values */
  for (gint i = 0; i < channel_count; i++)
    colors->r[i] = get_bits (src, start, rgb_precision);

  for (gint i = 0; i < channel_count; i++)
    colors->g[i] = get_bits (src, start, rgb_precision);

  for (gint i = 0; i < channel_count; i++)
    colors->b[i] = get_bits (src, start, rgb_precision);

  /* Modes 4 - 7 also have alpha values */
  if (alpha_precision)
    {
      for (gint i = 0; i < channel_count; i++)
        colors->a[i] = get_bits (src, start, alpha_precision);
    }
  else
    {
      for (gint i = 0; i < channel_count; i++)
        colors->a[i] = 255;
    }

  /* Modes 0, 1, 3, 6, and 7 have P-bits, which increase the precision
   * by 1. */
  if (mode_info[mode].p_bit_count)
    {
      rgb_precision++;
      if (alpha_precision)
        alpha_precision++;

      /* P-bits are added at the least significant bit, so
       * we have to shift existing channel values over by 1 */
      for (gint i = 0; i < channel_count; i++)
        {
          colors->r[i] <<= 1;
          colors->g[i] <<= 1;
          colors->b[i] <<= 1;
          if (alpha_precision)
            colors->a[i] <<= 1;
        }

      if (mode == 1)
        {
          guint p_bit_1 = get_bits (src, start, 1);
          guint p_bit_2 = get_bits (src, start, 1);

          for (gint i = 0; i < 2; i++)
            {
              colors->r[i] |= p_bit_1;
              colors->g[i] |= p_bit_1;
              colors->b[i] |= p_bit_1;

              colors->r[i + 2] |= p_bit_2;
              colors->g[i + 2] |= p_bit_2;
              colors->b[i + 2] |= p_bit_2;
            }
        }
      else
        {
          for (gint i = 0; i < channel_count; i++)
            {
              guint p_bit = get_bits (src, start, 1);

              colors->r[i] |= p_bit;
              colors->g[i] |= p_bit;
              colors->b[i] |= p_bit;
              if (alpha_precision)
                colors->a[i] |= p_bit;
            }
        }
    }

  /* Normalize all channel values to 8 bits */
  for (gint i = 0; i < channel_count; i++)
    {
      colors->r[i] <<= (8 - rgb_precision);
      colors->g[i] <<= (8 - rgb_precision);
      colors->b[i] <<= (8 - rgb_precision);

      colors->r[i] = colors->r[i] | (colors->r[i] >> rgb_precision);
      colors->g[i] = colors->g[i] | (colors->g[i] >> rgb_precision);
      colors->b[i] = colors->b[i] | (colors->b[i] >> rgb_precision);

      if (alpha_precision)
        {
          colors->a[i] <<= (8 - alpha_precision);
          colors->a[i] = colors->a[i] | (colors->a[i] >> alpha_precision);
        }
    }
}

static gboolean
is_pixel_anchor (guchar  subset_index,
                 guint   precision,
                 guchar  pixel_index,
                 guint   partition_id)
{
  guint table_index;

  if (subset_index == 0)
    table_index = 0;
  else if (subset_index == 1 && precision == 2)
    table_index = 1;
  else if (subset_index == 1 && precision == 3)
    table_index = 2;
  else
    table_index = 3;

  if (anchor_index_table[table_index][partition_id] == pixel_index)
    return TRUE;

  return FALSE;
}
