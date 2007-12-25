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

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-undo-push.h"

#include "gimp-intl.h"


const guchar *
gimp_image_get_colormap (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return image->colormap;
}

gint
gimp_image_get_colormap_size (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return image->n_colors;
}

void
gimp_image_set_colormap (GimpImage    *image,
                         const guchar *colormap,
                         gint          n_colors,
                         gboolean      push_undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (colormap != NULL || n_colors == 0);
  g_return_if_fail (n_colors >= 0 && n_colors <= 256);

  if (push_undo)
    gimp_image_undo_push_image_colormap (image, _("Set Colormap"));

  if (image->colormap)
    memset (image->colormap, 0, GIMP_IMAGE_COLORMAP_SIZE);

  if (colormap)
    {
      if (! image->colormap)
        image->colormap = g_new0 (guchar, GIMP_IMAGE_COLORMAP_SIZE);

      memcpy (image->colormap, colormap, n_colors * 3);
    }
  else if (! gimp_image_base_type (image) == GIMP_INDEXED)
    {
      if (image->colormap)
        g_free (image->colormap);

      image->colormap = NULL;
    }

  image->n_colors = n_colors;

  gimp_image_colormap_changed (image, -1);
}

void
gimp_image_get_colormap_entry (GimpImage *image,
                               gint       color_index,
                               GimpRGB   *color)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (image->colormap != NULL);
  g_return_if_fail (color_index >= 0 && color_index < image->n_colors);
  g_return_if_fail (color != NULL);

  gimp_rgba_set_uchar (color,
                       image->colormap[color_index * 3],
                       image->colormap[color_index * 3 + 1],
                       image->colormap[color_index * 3 + 2],
                       OPAQUE_OPACITY);
}

void
gimp_image_set_colormap_entry (GimpImage     *image,
                               gint           color_index,
                               const GimpRGB *color,
                               gboolean       push_undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (image->colormap != NULL);
  g_return_if_fail (color_index >= 0 && color_index < image->n_colors);
  g_return_if_fail (color != NULL);

  if (push_undo)
    gimp_image_undo_push_image_colormap (image,
                                         _("Change Colormap entry"));

  gimp_rgb_get_uchar (color,
                      &image->colormap[color_index * 3],
                      &image->colormap[color_index * 3 + 1],
                      &image->colormap[color_index * 3 + 2]);

  gimp_image_colormap_changed (image, color_index);
}

void
gimp_image_add_colormap_entry (GimpImage     *image,
                               const GimpRGB *color)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (image->colormap != NULL);
  g_return_if_fail (image->n_colors < 256);
  g_return_if_fail (color != NULL);

  gimp_image_undo_push_image_colormap (image,
                                       _("Add Color to Colormap"));

  gimp_rgb_get_uchar (color,
                      &image->colormap[image->n_colors * 3],
                      &image->colormap[image->n_colors * 3 + 1],
                      &image->colormap[image->n_colors * 3 + 2]);

  image->n_colors++;

  gimp_image_colormap_changed (image, -1);
}
