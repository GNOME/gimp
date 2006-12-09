/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "core-types.h"

#include "gimpimage.h"
#include "gimpimage-colorhash.h"


#define MAXDIFF         195076
#define HASH_TABLE_SIZE 1021


typedef struct _ColorHash ColorHash;

struct _ColorHash
{
  gint       pixel;   /*  R << 16 | G << 8 | B  */
  gint       index;   /*  colormap index        */
  GimpImage *image;
};


static ColorHash  color_hash_table[HASH_TABLE_SIZE];
static gint       color_hash_misses;
static gint       color_hash_hits;


void
gimp_image_color_hash_init (void)
{
  gint i;

  /*  initialize the color hash table--invalidate all entries  */
  for (i = 0; i < HASH_TABLE_SIZE; i++)
    {
      color_hash_table[i].pixel  = 0;
      color_hash_table[i].index  = 0;
      color_hash_table[i].image = NULL;
    }

  color_hash_misses = 0;
  color_hash_hits   = 0;
}

void
gimp_image_color_hash_exit (void)
{
#if 0
  /*  print out the hash table statistics  */
  g_print ("RGB->indexed hash table lookups: %d\n",
           color_hash_hits + color_hash_misses);
  g_print ("RGB->indexed hash table hits: %d\n", color_hash_hits);
  g_print ("RGB->indexed hash table misses: %d\n", color_hash_misses);
  g_print ("RGB->indexed hash table hit rate: %f\n",
           100.0 * color_hash_hits / (color_hash_hits + color_hash_misses));
#endif
}

void
gimp_image_color_hash_invalidate (GimpImage *image,
                                  gint       index)
{
  gint i;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (index == -1) /* invalidate all entries */
    {
      for (i = 0; i < HASH_TABLE_SIZE; i++)
        if (color_hash_table[i].image == image)
          {
            color_hash_table[i].pixel  = 0;
            color_hash_table[i].index  = 0;
            color_hash_table[i].image = NULL;
          }
    }
  else
    {
      for (i = 0; i < HASH_TABLE_SIZE; i++)
        if (color_hash_table[i].image == image &&
            color_hash_table[i].index  == index)
          {
            color_hash_table[i].pixel  = 0;
            color_hash_table[i].index  = 0;
            color_hash_table[i].image = NULL;
          }
    }
}

gint
gimp_image_color_hash_rgb_to_indexed (const GimpImage *image,
                                      gint             r,
                                      gint             g,
                                      gint             b)
{
  guchar *cmap;
  gint    num_cols;
  guint   pixel;
  gint    hash_index;
  gint    cmap_index;

  cmap       = image->cmap;
  num_cols   = image->num_cols;
  pixel      = (r << 16) | (g << 8) | b;
  hash_index = pixel % HASH_TABLE_SIZE;

  if (color_hash_table[hash_index].image == image &&
      color_hash_table[hash_index].pixel  == pixel)
    {
      /*  Hash table lookup hit  */

      cmap_index = color_hash_table[hash_index].index;
      color_hash_hits++;
    }
  else
    {
      /*  Hash table lookup miss  */

      const guchar *col;
      gint          diff, sum, max;
      gint          i;

      max        = MAXDIFF;
      cmap_index = 0;
      col        = cmap;

      for (i = 0; i < num_cols; i++)
        {
          diff = r - *col++;
          sum  = diff * diff;

          diff = g - *col++;
          sum += diff * diff;

          diff = b - *col++;
          sum += diff * diff;

          if (sum < max)
            {
              cmap_index = i;
              max = sum;
            }
        }

      /*  update the hash table  */
      color_hash_table[hash_index].pixel  = pixel;
      color_hash_table[hash_index].index  = cmap_index;
      color_hash_table[hash_index].image = (GimpImage *) image;
      color_hash_misses++;
    }

  return cmap_index;
}
