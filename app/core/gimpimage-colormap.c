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

#include "gegl/gimp-babl.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpdatafactory.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-private.h"
#include "gimpimage-undo-push.h"
#include "gimppalette.h"

#include "gimp-intl.h"


/*  local function prototype  */

static void   gimp_image_colormap_set_palette_entry (GimpImage     *image,
                                                     const GimpRGB *color,
                                                     gint           index);


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

  gimp_data_make_internal (GIMP_DATA (private->palette), palette_id);

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

  if (private->palette)
    {
      guchar *colormap;
      gint    n_colors;

      colormap = gimp_image_get_colormap (image);
      n_colors = gimp_image_get_colormap_size (image);

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
      gimp_image_colormap_set_palette_entry (image, &entry->color, i);
    }

  gimp_data_thaw (GIMP_DATA (private->palette));

  gimp_image_colormap_changed (image, -1);
}

guchar *
gimp_image_get_colormap (GimpImage *image)
{
  GimpImagePrivate *private;
  guchar           *colormap = NULL;
  gint              n_colors, i;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->palette == NULL)
    return NULL;

  n_colors = gimp_palette_get_n_colors (private->palette);

  if (n_colors > 0)
    {
      colormap = g_new0 (guchar, GIMP_IMAGE_COLORMAP_SIZE);

      for (i = 0; i < n_colors; i++)
        {
          GimpPaletteEntry *entry = gimp_palette_get_entry (private->palette, i);
          gimp_rgb_get_uchar (&entry->color,
                              &colormap[i * 3],
                              &colormap[i * 3 + 1],
                              &colormap[i * 3 + 2]);
        }
    }

  return colormap;
}

gint
gimp_image_get_colormap_size (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);
  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->palette == NULL)
    return 0;

  return gimp_palette_get_n_colors (private->palette);
}

void
gimp_image_set_colormap (GimpImage    *image,
                         const guchar *colormap,
                         gint          n_colors,
                         gboolean      push_undo)
{
  GimpImagePrivate *private;
  GimpPaletteEntry *entry;
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

  for (i = 0; i < n_colors; i++)
    {
      GimpRGB color;
      gimp_rgba_set_uchar (&color,
                           colormap[3 * i + 0],
                           colormap[3 * i + 1],
                           colormap[3 * i + 2],
                           255);
      gimp_image_colormap_set_palette_entry (image, &color, i);
    }

  gimp_data_thaw (GIMP_DATA (private->palette));

  gimp_image_colormap_changed (image, -1);
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

void
gimp_image_get_colormap_entry (GimpImage *image,
                               gint       color_index,
                               GimpRGB   *color)
{
  GimpImagePrivate *private;
  GimpPaletteEntry *entry;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette != NULL);
  g_return_if_fail (color_index >= 0 &&
                    color_index < gimp_palette_get_n_colors (private->palette));
  g_return_if_fail (color != NULL);

  entry = gimp_palette_get_entry (private->palette, color_index);

  g_return_if_fail (entry != NULL);

  *color = entry->color;
}

void
gimp_image_set_colormap_entry (GimpImage     *image,
                               gint           color_index,
                               const GimpRGB *color,
                               gboolean       push_undo)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette != NULL);
  g_return_if_fail (color_index >= 0 &&
                    color_index < gimp_palette_get_n_colors (private->palette));
  g_return_if_fail (color != NULL);

  if (push_undo)
    gimp_image_undo_push_image_colormap (image,
                                         C_("undo-type", "Change Colormap entry"));

  gimp_image_colormap_set_palette_entry (image, color, color_index);

  gimp_image_colormap_changed (image, color_index);
}

void
gimp_image_add_colormap_entry (GimpImage     *image,
                               const GimpRGB *color)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette != NULL);
  g_return_if_fail (gimp_palette_get_n_colors (private->palette) < 256);
  g_return_if_fail (color != NULL);

  gimp_image_undo_push_image_colormap (image,
                                       C_("undo-type", "Add Color to Colormap"));

  gimp_image_colormap_set_palette_entry (image, color,
                                         gimp_palette_get_n_colors (private->palette));

  gimp_image_colormap_changed (image, -1);
}


/*  private functions  */

static void
gimp_image_colormap_set_palette_entry (GimpImage     *image,
                                       const GimpRGB *color,
                                       gint           index)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
  GimpRGB           black;
  gchar             name[64];

  g_return_if_fail (color != NULL);

  gimp_rgb_set (&black, 0, 0, 0);
  while (gimp_palette_get_n_colors (private->palette) <= index)
    gimp_palette_add_entry (private->palette, index, name, &black);

  g_snprintf (name, sizeof (name), "#%d", index);

  gimp_palette_set_entry (private->palette, index, name, color);
}
