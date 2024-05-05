/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "config/gimpgeglconfig.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpdatafactory.h"
#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-private.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimppalette.h"

#include "gimp-intl.h"


typedef struct
{
  GeglBuffer *buffer;
  const Babl *format;

  /* Shared by jobs. */
  gboolean   *found;
  GRWLock    *lock;
} IndexUsedJobData;


/*  local function prototype  */

static void   gimp_image_colormap_set_palette_entry    (GimpImage        *image,
                                                        GeglColor        *color,
                                                        gint              index);
static void   gimp_image_colormap_thread_is_index_used (IndexUsedJobData *data,
                                                        gint              index);


/*  public functions  */

void
gimp_image_colormap_init (GimpImage *image)
{
  GimpImagePrivate *private;
  GimpContainer    *palettes;
  gchar            *palette_name;
  gchar            *palette_id;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette == NULL);

  palette_name = g_strdup_printf (_("Colormap of Image #%d (%s)"),
                                  gimp_image_get_id (image),
                                  gimp_image_get_display_name (image));
  palette_id = g_strdup_printf ("gimp-indexed-image-palette-%d",
                                gimp_image_get_id (image));

  private->palette  = GIMP_PALETTE (gimp_palette_new (NULL, palette_name));

  gimp_image_colormap_update_formats (image);

  gimp_palette_set_columns (private->palette, 16);

  gimp_data_set_image (GIMP_DATA (private->palette), image, TRUE, FALSE);

  palettes = gimp_data_factory_get_container (image->gimp->palette_factory);

  gimp_container_add (palettes, GIMP_OBJECT (private->palette));

  g_free (palette_name);
  g_free (palette_id);
}

void
gimp_image_colormap_dispose (GimpImage *image)
{
  GimpImagePrivate *private;
  GimpContainer    *palettes;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (GIMP_IS_PALETTE (private->palette));

  palettes = gimp_data_factory_get_container (image->gimp->palette_factory);

  gimp_container_remove (palettes, GIMP_OBJECT (private->palette));
}

void
gimp_image_colormap_free (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (GIMP_IS_PALETTE (private->palette));

  g_clear_object (&private->palette);

  /* don't touch the image's babl_palettes because we might still have
   * buffers with that palette on the undo stack, and on undoing the
   * image back to indexed, we must have exactly these palettes around
   */
}

void
gimp_image_colormap_update_formats (GimpImage *image)
{
  GimpImagePrivate *private;
  const Babl       *space;
  gchar            *format_name;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  space = gimp_image_get_layer_space (image);

  format_name = g_strdup_printf ("-gimp-indexed-format-%d",
                                 gimp_image_get_id (image));

  babl_new_palette_with_space (format_name, space,
                               &private->babl_palette_rgb,
                               &private->babl_palette_rgba);

  g_free (format_name);

  if (private->palette && gimp_palette_get_n_colors (private->palette) > 0)
    {
      guchar *colormap;
      gint    n_colors;

      colormap = _gimp_image_get_colormap (image, &n_colors);

      babl_palette_set_palette (private->babl_palette_rgb,
                                gimp_babl_format (GIMP_RGB,
                                                  private->precision, FALSE,
                                                  space),
                                colormap,
                                n_colors);
      babl_palette_set_palette (private->babl_palette_rgba,
                                gimp_babl_format (GIMP_RGB,
                                                  private->precision, FALSE,
                                                  space),
                                colormap,
                                n_colors);

      g_free (colormap);
    }
}

const Babl *
gimp_image_colormap_get_rgb_format (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->babl_palette_rgb;
}

const Babl *
gimp_image_colormap_get_rgba_format (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->babl_palette_rgba;
}

GimpPalette *
gimp_image_get_colormap_palette (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->palette;
}

void
gimp_image_set_colormap_palette (GimpImage   *image,
                                 GimpPalette *palette,
                                 gboolean     push_undo)
{
  GimpImagePrivate *private;
  GimpPaletteEntry *entry;
  gint              n_colors, i;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (palette != NULL);
  n_colors = gimp_palette_get_n_colors (palette);
  g_return_if_fail (n_colors >= 0 && n_colors <= 256);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    gimp_image_undo_push_image_colormap (image, C_("undo-type", "Set Colormap"));

  if (!private->palette)
    gimp_image_colormap_init (image);

  gimp_data_freeze (GIMP_DATA (private->palette));

  while ((entry = gimp_palette_get_entry (private->palette, 0)))
    gimp_palette_delete_entry (private->palette, entry);

  for (i = 0; i < n_colors; i++)
    {
      GimpPaletteEntry *entry = gimp_palette_get_entry (palette, i);

      gimp_image_colormap_set_palette_entry (image, entry->color, i);
    }

  gimp_data_thaw (GIMP_DATA (private->palette));

  gimp_image_colormap_changed (image, -1);
}

guchar *
_gimp_image_get_colormap (GimpImage *image,
                          gint      *n_colors)
{
  GimpImagePrivate *private;
  guchar           *colormap = NULL;
  const Babl       *space;
  const Babl       *format;
  gint              bpp;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->palette == NULL)
    return NULL;

  space  = gimp_image_get_layer_space (image);
  format = gimp_babl_format (GIMP_RGB, private->precision, FALSE, space);
  bpp    = babl_format_get_bytes_per_pixel (format);

  *n_colors = gimp_palette_get_n_colors (private->palette);

  if (*n_colors > 0)
    {
      colormap = g_new0 (guchar, GIMP_IMAGE_COLORMAP_SIZE);

      for (gint i = 0; i < *n_colors; i++)
        {
          GimpPaletteEntry *entry = gimp_palette_get_entry (private->palette, i);

          gegl_color_get_pixel (entry->color, format, &colormap[i * bpp]);
        }
    }

  return colormap;
}

void
_gimp_image_set_colormap (GimpImage    *image,
                          const guchar *colormap,
                          gint          n_colors,
                          gboolean      push_undo)
{
  GimpImagePrivate *private;
  GimpPaletteEntry *entry;
  const Babl       *space;
  const Babl       *format;
  GeglColor        *color;
  gint              bpp;
  gint              i;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (colormap != NULL || n_colors == 0);
  g_return_if_fail (n_colors >= 0 && n_colors <= 256);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    gimp_image_undo_push_image_colormap (image, C_("undo-type", "Set Colormap"));

  if (!private->palette)
    gimp_image_colormap_init (image);

  gimp_data_freeze (GIMP_DATA (private->palette));

  while ((entry = gimp_palette_get_entry (private->palette, 0)))
    gimp_palette_delete_entry (private->palette, entry);

  space  = gimp_image_get_layer_space (image);
  format = gimp_babl_format (GIMP_RGB, private->precision, FALSE, space);
  bpp    = babl_format_get_bytes_per_pixel (format);

  color = gegl_color_new (NULL);
  for (i = 0; i < n_colors; i++)
    {
      gegl_color_set_pixel (color, format, &colormap[i * bpp]);
      gimp_image_colormap_set_palette_entry (image, color, i);
    }
  g_object_unref (color);

  gimp_image_colormap_changed (image, -1);
  gimp_data_thaw (GIMP_DATA (private->palette));
}

void
gimp_image_unset_colormap (GimpImage *image,
                           gboolean   push_undo)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    gimp_image_undo_push_image_colormap (image,
                                         C_("undo-type", "Unset Colormap"));

  if (private->palette)
    {
      gimp_image_colormap_dispose (image);
      gimp_image_colormap_free (image);
    }

  gimp_image_colormap_changed (image, -1);
}

gboolean
gimp_image_colormap_is_index_used (GimpImage *image,
                                   gint       color_index)
{
  GList       *layers;
  GList       *iter;
  GThreadPool *pool;
  GRWLock      lock;
  gboolean     found = FALSE;
  gint         num_processors;

  g_rw_lock_init (&lock);
  num_processors = GIMP_GEGL_CONFIG (image->gimp->config)->num_processors;
  layers         = gimp_image_get_layer_list (image);

  pool = g_thread_pool_new_full ((GFunc) gimp_image_colormap_thread_is_index_used,
                                 GINT_TO_POINTER (color_index),
                                 (GDestroyNotify) g_free,
                                 num_processors, TRUE, NULL);
  for (iter = layers; iter; iter = g_list_next (iter))
    {
      IndexUsedJobData *job_data;

      job_data = g_malloc (sizeof (IndexUsedJobData ));
      job_data->buffer = gimp_drawable_get_buffer (iter->data);
      job_data->format = gimp_drawable_get_format_without_alpha (iter->data);
      job_data->found  = &found;
      job_data->lock   = &lock;

      g_thread_pool_push (pool, job_data, NULL);
    }

  g_thread_pool_free (pool, FALSE, TRUE);
  g_rw_lock_clear (&lock);
  g_list_free (layers);

  return found;
}

GeglColor *
gimp_image_get_colormap_entry (GimpImage *image,
                               gint       color_index)
{
  GimpImagePrivate *private;
  GimpPaletteEntry *entry;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  g_return_val_if_fail (private->palette != NULL, NULL);
  g_return_val_if_fail (color_index >= 0 &&
                        color_index < gimp_palette_get_n_colors (private->palette),
                        NULL);

  entry = gimp_palette_get_entry (private->palette, color_index);

  g_return_val_if_fail (entry != NULL, NULL);

  return entry->color;
}

void
gimp_image_set_colormap_entry (GimpImage *image,
                               gint       color_index,
                               GeglColor *color,
                               gboolean   push_undo)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette != NULL);
  g_return_if_fail (color_index >= 0 &&
                    color_index < gimp_palette_get_n_colors (private->palette));
  g_return_if_fail (GEGL_IS_COLOR (color));

  if (push_undo)
    gimp_image_undo_push_image_colormap (image,
                                         C_("undo-type", "Change Colormap entry"));

  gimp_image_colormap_set_palette_entry (image, color, color_index);

  gimp_image_colormap_changed (image, color_index);
}

void
gimp_image_add_colormap_entry (GimpImage *image,
                               GeglColor *color)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette != NULL);
  g_return_if_fail (gimp_palette_get_n_colors (private->palette) < 256);
  g_return_if_fail (GEGL_IS_COLOR (color));

  gimp_image_undo_push_image_colormap (image,
                                       C_("undo-type", "Add Color to Colormap"));

  gimp_image_colormap_set_palette_entry (image, color,
                                         gimp_palette_get_n_colors (private->palette));

  gimp_image_colormap_changed (image, -1);
}

gboolean
gimp_image_delete_colormap_entry (GimpImage *image,
                                  gint       color_index,
                                  gboolean   push_undo)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  if (! gimp_image_colormap_is_index_used (image, color_index))
    {
      GimpImagePrivate *private;
      GimpPaletteEntry *entry;
      GList            *layers;
      GList            *iter;

      if (push_undo)
        {
          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_COLORMAP_REMAP,
                                       C_("undo-type", "Delete Colormap entry"));

          gimp_image_undo_push_image_colormap (image, NULL);
        }

      private = GIMP_IMAGE_GET_PRIVATE (image);
      layers  = gimp_image_get_layer_list (image);

      for (iter = layers; iter; iter = g_list_next (iter))
        {
          if (push_undo)
            gimp_image_undo_push_drawable_mod (image, NULL, iter->data, TRUE);

          gimp_gegl_shift_index (gimp_drawable_get_buffer (iter->data), NULL,
                                 gimp_drawable_get_format (iter->data),
                                 color_index, -1);
        }

      entry = gimp_palette_get_entry (private->palette, color_index);
      gimp_palette_delete_entry (private->palette, entry);

      g_list_free (layers);

      if (push_undo)
        gimp_image_undo_group_end (image);

      gimp_image_colormap_changed (image, -1);

      return TRUE;
    }

  return FALSE;
}


/*  private functions  */

static void
gimp_image_colormap_set_palette_entry (GimpImage *image,
                                       GeglColor *color,
                                       gint       index)
{
  GimpImagePrivate *private   = GIMP_IMAGE_GET_PRIVATE (image);
  GeglColor        *new_color = gegl_color_new ("black");
  const Babl       *space;
  const Babl       *format;
  guint8            rgb[3];
  gchar             name[64];
  gint              i;

  g_return_if_fail (GEGL_IS_COLOR (color));

  /* Adding black entries if needed. */
  for (i = gimp_palette_get_n_colors (private->palette);
       i <= index;
       i++)
    {
      g_snprintf (name, sizeof (name), "#%d", i);
      gimp_palette_add_entry (private->palette, index, name, new_color);
    }

  space  = gimp_image_get_layer_space (image);
  format = gimp_babl_format (GIMP_RGB, private->precision, FALSE, space);

  /* For image colormap, we force the color to be in the target format (which at
   * time of writing is only "R'G'B' u8" for the target space).
   */
  gegl_color_get_pixel (color, format, rgb);
  gegl_color_set_pixel (new_color, format, rgb);
  gimp_palette_set_entry (private->palette, index, name, new_color);
  g_object_unref (new_color);
}

static void
gimp_image_colormap_thread_is_index_used (IndexUsedJobData *data,
                                          gint              index)
{
  g_rw_lock_reader_lock (data->lock);
  if (*data->found)
    {
      g_rw_lock_reader_unlock (data->lock);
      return;
    }
  g_rw_lock_reader_unlock (data->lock);
  if (gimp_gegl_is_index_used (data->buffer, NULL, data->format, index))
    {
      g_rw_lock_writer_lock (data->lock);
      *data->found = TRUE;
      g_rw_lock_writer_unlock (data->lock);
    }
}
