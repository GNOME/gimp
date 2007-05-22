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

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"

#include "gimpchannel.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimppalette.h"
#include "gimppalette-import.h"
#include "gimppalette-load.h"
#include "gimppickable.h"

#include "gimp-intl.h"


#define MAX_IMAGE_COLORS (10000 * 2)


/*  create a palette from a gradient  ****************************************/

GimpPalette *
gimp_palette_import_from_gradient (GimpGradient *gradient,
                                   GimpContext  *context,
                                   gboolean      reverse,
                                   const gchar  *palette_name,
                                   gint          n_colors)
{
  GimpPalette         *palette;
  GimpGradientSegment *seg = NULL;
  gdouble              dx, cur_x;
  GimpRGB              color;
  gint                 i;

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);
  g_return_val_if_fail (n_colors > 1, NULL);

  palette = GIMP_PALETTE (gimp_palette_new (palette_name));

  dx = 1.0 / (n_colors - 1);

  for (i = 0, cur_x = 0; i < n_colors; i++, cur_x += dx)
    {
      seg = gimp_gradient_get_color_at (gradient, context,
                                        seg, cur_x, reverse, &color);
      gimp_palette_add_entry (palette, -1, NULL, &color);
    }

  return palette;
}


/*  create a palette from a non-indexed image  *******************************/

typedef struct _ImgColors ImgColors;

struct _ImgColors
{
  guint  count;
  guint  r_adj;
  guint  g_adj;
  guint  b_adj;
  guchar r;
  guchar g;
  guchar b;
};

static gint count_color_entries = 0;

static GHashTable *
gimp_palette_import_store_colors (GHashTable *table,
                                  guchar     *colors,
                                  guchar     *colors_real,
                                  gint        n_colors)
{
  gpointer   found_color = NULL;
  ImgColors *new_color;
  guint      key_colors = colors[0] * 256 * 256 + colors[1] * 256 + colors[2];

  if (table == NULL)
    {
      table = g_hash_table_new (g_direct_hash, g_direct_equal);
      count_color_entries = 0;
    }
  else
    {
      found_color = g_hash_table_lookup (table, GUINT_TO_POINTER (key_colors));
    }

  if (found_color == NULL)
    {
      if (count_color_entries > MAX_IMAGE_COLORS)
        {
          /* Don't add any more new ones */
          return table;
        }

      count_color_entries++;

      new_color = g_slice_new (ImgColors);

      new_color->count = 1;
      new_color->r_adj = 0;
      new_color->g_adj = 0;
      new_color->b_adj = 0;
      new_color->r     = colors[0];
      new_color->g     = colors[1];
      new_color->b     = colors[2];

      g_hash_table_insert (table, GUINT_TO_POINTER (key_colors), new_color);
    }
  else
    {
      new_color = found_color;

      if (new_color->count < (G_MAXINT - 1))
        new_color->count++;

      /* Now do the adjustments ...*/
      new_color->r_adj += (colors_real[0] - colors[0]);
      new_color->g_adj += (colors_real[1] - colors[1]);
      new_color->b_adj += (colors_real[2] - colors[2]);

      /* Boundary conditions */
      if(new_color->r_adj > (G_MAXINT - 255))
        new_color->r_adj /= new_color->count;

      if(new_color->g_adj > (G_MAXINT - 255))
        new_color->g_adj /= new_color->count;

      if(new_color->b_adj > (G_MAXINT - 255))
        new_color->b_adj /= new_color->count;
    }

  return table;
}

static void
gimp_palette_import_create_list (gpointer key,
                                 gpointer value,
                                 gpointer user_data)
{
  GSList    **list      = user_data;
  ImgColors  *color_tab = value;

  *list = g_slist_prepend (*list, color_tab);
}

static gint
gimp_palette_import_sort_colors (gconstpointer a,
                                 gconstpointer b)
{
  const ImgColors *s1 = a;
  const ImgColors *s2 = b;

  if(s1->count > s2->count)
    return -1;
  if(s1->count < s2->count)
    return 1;

  return 0;
}

static void
gimp_palette_import_create_image_palette (gpointer data,
                                          gpointer user_data)
{
  GimpPalette *palette   = user_data;
  ImgColors   *color_tab = data;
  gint         n_colors;
  gchar       *lab;
  GimpRGB      color;

  n_colors = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (palette),
                                                 "import-n-colors"));

  if (palette->n_colors >= n_colors)
    return;

  lab = g_strdup_printf ("%s (occurs %u)", _("Untitled"), color_tab->count);

  /* Adjust the colors to the mean of the the sample */
  gimp_rgba_set_uchar
    (&color,
     (guchar) color_tab->r + (color_tab->r_adj / color_tab->count),
     (guchar) color_tab->g + (color_tab->g_adj / color_tab->count),
     (guchar) color_tab->b + (color_tab->b_adj / color_tab->count),
     255);

  gimp_palette_add_entry (palette, -1, lab, &color);

  g_free (lab);
}

static GimpPalette *
gimp_palette_import_make_palette (GHashTable  *table,
                                  const gchar *palette_name,
                                  gint         n_colors)
{
  GimpPalette *palette = GIMP_PALETTE (gimp_palette_new (palette_name));
  GSList      *list    = NULL;
  GSList      *iter;

  if (! table)
    return palette;

  g_hash_table_foreach (table, gimp_palette_import_create_list, &list);
  list = g_slist_sort (list, gimp_palette_import_sort_colors);

  g_object_set_data (G_OBJECT (palette), "import-n-colors",
                     GINT_TO_POINTER (n_colors));

  g_slist_foreach (list, gimp_palette_import_create_image_palette, palette);

  g_object_set_data (G_OBJECT (palette), "import-n-colors", NULL);

  /*  Free up used memory
   *  Note the same structure is on both the hash list and the sorted
   *  list. So only delete it once.
   */
  g_hash_table_destroy (table);

  for (iter = list; iter; iter = iter->next)
    g_slice_free (ImgColors, iter->data);

  g_slist_free (list);

  return palette;
}

static GHashTable *
gimp_palette_import_extract (GimpImage     *image,
                             GimpPickable  *pickable,
                             gint           pickable_off_x,
                             gint           pickable_off_y,
                             gboolean       selection_only,
                             gint           x,
                             gint           y,
                             gint           width,
                             gint           height,
                             gint           n_colors,
                             gint           threshold)
{
  TileManager   *tiles;
  GimpImageType  type;
  PixelRegion    region;
  PixelRegion    mask_region;
  PixelRegion   *maskPR = NULL;
  gpointer       pr;
  GHashTable    *colors = NULL;

  tiles = gimp_pickable_get_tiles (pickable);
  type  = gimp_pickable_get_image_type (pickable);

  pixel_region_init (&region, tiles, x, y, width, height, FALSE);

  if (selection_only &&
      ! gimp_channel_is_empty (gimp_image_get_mask (image)))
    {
      pixel_region_init (&mask_region,
                         GIMP_DRAWABLE (gimp_image_get_mask (image))->tiles,
                         x + pickable_off_x, y + pickable_off_y,
                         width, height,
                         FALSE);

      maskPR = &mask_region;
    }

  for (pr = pixel_regions_register (maskPR ? 2 : 1, &region, maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *data      = region.data;
      const guchar *mask_data = NULL;
      gint          i, j;

      if (maskPR)
        mask_data = maskPR->data;

      for (i = 0; i < region.h; i++)
        {
          const guchar *idata = data;
          const guchar *mdata = mask_data;

          for (j = 0; j < region.w; j++)
            {
              if (! mdata || *mdata)
                {
                  guchar rgba[MAX_CHANNELS];

                  gimp_image_get_color (image, type, idata, rgba);

                  /*  ignore completely transparent pixels  */
                  if (rgba[ALPHA_PIX])
                    {
                      guchar rgb_real[MAX_CHANNELS];

                      memcpy (rgb_real, rgba, MAX_CHANNELS);

                      rgba[0] = (rgba[0] / threshold) * threshold;
                      rgba[1] = (rgba[1] / threshold) * threshold;
                      rgba[2] = (rgba[2] / threshold) * threshold;

                      colors = gimp_palette_import_store_colors (colors,
                                                                 rgba, rgb_real,
                                                                 n_colors);
                    }
                }

              idata += region.bytes;

              if (mdata)
                mdata += maskPR->bytes;
            }

          data += region.rowstride;

          if (mask_data)
            mask_data += maskPR->rowstride;
        }
    }

  return colors;
}

GimpPalette *
gimp_palette_import_from_image (GimpImage   *image,
                                const gchar *palette_name,
                                gint         n_colors,
                                gint         threshold,
                                gboolean     selection_only)
{
  GHashTable *colors;
  gint        x, y;
  gint        width, height;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);
  g_return_val_if_fail (n_colors > 1, NULL);
  g_return_val_if_fail (threshold > 0, NULL);

  gimp_pickable_flush (GIMP_PICKABLE (image->projection));

  if (selection_only)
    {
      gimp_channel_bounds (gimp_image_get_mask (image),
                           &x, &y, &width, &height);

      width  -= x;
      height -= y;
    }
  else
    {
      x      = 0;
      y      = 0;
      width  = image->width;
      height = image->height;
    }

  colors = gimp_palette_import_extract (image,
                                        GIMP_PICKABLE (image->projection),
                                        0, 0,
                                        selection_only,
                                        x, y, width, height,
                                        n_colors, threshold);

  return gimp_palette_import_make_palette (colors, palette_name, n_colors);
}


/*  create a palette from an indexed image  **********************************/

GimpPalette *
gimp_palette_import_from_indexed_image (GimpImage   *image,
                                        const gchar *palette_name)
{
  GimpPalette *palette;
  gint         count;
  GimpRGB      color;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (gimp_image_base_type (image) == GIMP_INDEXED, NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);

  palette = GIMP_PALETTE (gimp_palette_new (palette_name));

  for (count = 0; count < image->num_cols; ++count)
    {
      gchar name[256];

      g_snprintf (name, sizeof (name), _("Index %d"), count);

      gimp_rgba_set_uchar (&color,
                           image->cmap[count * 3 + 0],
                           image->cmap[count * 3 + 1],
                           image->cmap[count * 3 + 2],
                           255);

      gimp_palette_add_entry (palette, -1, name, &color);
    }

  return palette;
}


/*  create a palette from a drawable  ****************************************/

GimpPalette *
gimp_palette_import_from_drawable (GimpDrawable *drawable,
                                   const gchar  *palette_name,
                                   gint          n_colors,
                                   gint          threshold,
                                   gboolean      selection_only)
{
  GHashTable *colors = NULL;
  gint        x, y;
  gint        width, height;
  gint        off_x, off_y;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);
  g_return_val_if_fail (n_colors > 1, NULL);
  g_return_val_if_fail (threshold > 0, NULL);

  if (selection_only)
    {
      if (! gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
        return NULL;
    }
  else
    {
      x      = 0;
      y      = 0;
      width  = gimp_item_width (GIMP_ITEM (drawable));
      height = gimp_item_height (GIMP_ITEM (drawable));
    }

  gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

  colors =
    gimp_palette_import_extract (gimp_item_get_image (GIMP_ITEM (drawable)),
                                 GIMP_PICKABLE (drawable),
                                 off_x, off_y,
                                 selection_only,
                                 x, y, width, height,
                                 n_colors, threshold);

  return gimp_palette_import_make_palette (colors, palette_name, n_colors);
}


/*  create a palette from a file  **********************************/

GimpPalette *
gimp_palette_import_from_file (const gchar  *filename,
                               const gchar  *palette_name,
                               GError      **error)
{
  GList *palette_list = NULL;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  switch (gimp_palette_load_detect_format (filename))
    {
    case GIMP_PALETTE_FILE_FORMAT_GPL:
      palette_list = gimp_palette_load (filename, error);
      break;

    case GIMP_PALETTE_FILE_FORMAT_ACT:
      palette_list = gimp_palette_load_act (filename, error);
      break;

    case GIMP_PALETTE_FILE_FORMAT_RIFF_PAL:
      palette_list = gimp_palette_load_riff (filename, error);
      break;

    case GIMP_PALETTE_FILE_FORMAT_PSP_PAL:
      palette_list = gimp_palette_load_psp (filename, error);
      break;

    default:
      g_set_error (error,
                   GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Unknown type of palette file: %s"),
                   gimp_filename_to_utf8 (filename));
      break;
    }

  if (palette_list)
    {
      GimpPalette *palette = g_object_ref (palette_list->data);

      gimp_object_set_name (GIMP_OBJECT (palette), palette_name);

      g_list_foreach (palette_list, (GFunc) g_object_unref, NULL);
      g_list_free (palette_list);

      return palette;
    }

  return NULL;
}
