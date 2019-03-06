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

  g_return_if_fail (private->colormap == NULL);
  g_return_if_fail (private->palette == NULL);

  palette_name = g_strdup_printf (_("Colormap of Image #%d (%s)"),
                                  gimp_image_get_ID (image),
                                  gimp_image_get_display_name (image));
  palette_id = g_strdup_printf ("gimp-indexed-image-palette-%d",
                                gimp_image_get_ID (image));

  private->n_colors = 0;
  private->colormap = g_new0 (guchar, GIMP_IMAGE_COLORMAP_SIZE);
  private->palette  = GIMP_PALETTE (gimp_palette_new (NULL, palette_name));

  if (! private->babl_palette_rgb)
    {
      gchar *format_name = g_strdup_printf ("-gimp-indexed-format-%d",
                                            gimp_image_get_ID (image));

      babl_new_palette (format_name,
                        &private->babl_palette_rgb,
                        &private->babl_palette_rgba);

      g_free (format_name);
    }

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

  g_return_if_fail (private->colormap != NULL);
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

  g_return_if_fail (private->colormap != NULL);
  g_return_if_fail (GIMP_IS_PALETTE (private->palette));

  g_clear_pointer (&private->colormap, g_free);
  g_clear_object (&private->palette);

  /* don't touch the image's babl_palettes because we might still have
   * buffers with that palette on the undo stack, and on undoing the
   * image back to indexed, we must have exactly these palettes around
   */
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

const guchar *
gimp_image_get_colormap (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->colormap;
}

gint
gimp_image_get_colormap_size (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return GIMP_IMAGE_GET_PRIVATE (image)->n_colors;
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

  if (private->colormap)
    memset (private->colormap, 0, GIMP_IMAGE_COLORMAP_SIZE);
  else
    gimp_image_colormap_init (image);

  if (colormap)
    memcpy (private->colormap, colormap, n_colors * 3);

  /* make sure the image's colormap always has at least one color.  when
   * n_colors == 0, use black.
   */
  private->n_colors = MAX (n_colors, 1);

  gimp_data_freeze (GIMP_DATA (private->palette));

  while ((entry = gimp_palette_get_entry (private->palette, 0)))
    gimp_palette_delete_entry (private->palette, entry);

  for (i = 0; i < private->n_colors; i++)
    gimp_image_colormap_set_palette_entry (image, NULL, i);

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

  if (private->colormap)
    {
      gimp_image_colormap_dispose (image);
      gimp_image_colormap_free (image);
    }

  private->n_colors = 0;

  gimp_image_colormap_changed (image, -1);
}

void
gimp_image_get_colormap_entry (GimpImage *image,
                               gint       color_index,
                               GimpRGB   *color)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->colormap != NULL);
  g_return_if_fail (color_index >= 0 && color_index < private->n_colors);
  g_return_if_fail (color != NULL);

  gimp_rgba_set_uchar (color,
                       private->colormap[color_index * 3],
                       private->colormap[color_index * 3 + 1],
                       private->colormap[color_index * 3 + 2],
                       255);
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

  g_return_if_fail (private->colormap != NULL);
  g_return_if_fail (color_index >= 0 && color_index < private->n_colors);
  g_return_if_fail (color != NULL);

  if (push_undo)
    gimp_image_undo_push_image_colormap (image,
                                         C_("undo-type", "Change Colormap entry"));

  gimp_rgb_get_uchar (color,
                      &private->colormap[color_index * 3],
                      &private->colormap[color_index * 3 + 1],
                      &private->colormap[color_index * 3 + 2]);

  if (private->palette)
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

  g_return_if_fail (private->colormap != NULL);
  g_return_if_fail (private->n_colors < 256);
  g_return_if_fail (color != NULL);

  gimp_image_undo_push_image_colormap (image,
                                       C_("undo-type", "Add Color to Colormap"));

  gimp_rgb_get_uchar (color,
                      &private->colormap[private->n_colors * 3],
                      &private->colormap[private->n_colors * 3 + 1],
                      &private->colormap[private->n_colors * 3 + 2]);

  private->n_colors++;

  if (private->palette)
    gimp_image_colormap_set_palette_entry (image, color, private->n_colors - 1);

  gimp_image_colormap_changed (image, -1);
}


/*  private functions  */

static void
gimp_image_colormap_set_palette_entry (GimpImage     *image,
                                       const GimpRGB *c,
                                       gint           index)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
  GimpRGB           color;
  gchar             name[64];

  /* Avoid converting to char then back to double if we have the
   * original GimpRGB color.
   */
  if (c)
    color = *c;
  else
    gimp_rgba_set_uchar (&color,
                         private->colormap[3 * index + 0],
                         private->colormap[3 * index + 1],
                         private->colormap[3 * index + 2],
                         255);

  g_snprintf (name, sizeof (name), "#%d", index);

  if (gimp_palette_get_n_colors (private->palette) < private->n_colors)
    gimp_palette_add_entry (private->palette, index, name, &color);
  else
    gimp_palette_set_entry (private->palette, index, name, &color);
}
