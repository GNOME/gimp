/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "gegl/ligma-babl.h"

#include "ligma.h"
#include "ligmacontainer.h"
#include "ligmadatafactory.h"
#include "ligmaimage.h"
#include "ligmaimage-colormap.h"
#include "ligmaimage-private.h"
#include "ligmaimage-undo-push.h"
#include "ligmapalette.h"

#include "ligma-intl.h"


/*  local function prototype  */

static void   ligma_image_colormap_set_palette_entry (LigmaImage     *image,
                                                     const LigmaRGB *color,
                                                     gint           index);


/*  public functions  */

void
ligma_image_colormap_init (LigmaImage *image)
{
  LigmaImagePrivate *private;
  LigmaContainer    *palettes;
  gchar            *palette_name;
  gchar            *palette_id;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette == NULL);

  palette_name = g_strdup_printf (_("Colormap of Image #%d (%s)"),
                                  ligma_image_get_id (image),
                                  ligma_image_get_display_name (image));
  palette_id = g_strdup_printf ("ligma-indexed-image-palette-%d",
                                ligma_image_get_id (image));

  private->palette  = LIGMA_PALETTE (ligma_palette_new (NULL, palette_name));

  ligma_image_colormap_update_formats (image);

  ligma_palette_set_columns (private->palette, 16);

  ligma_data_make_internal (LIGMA_DATA (private->palette), palette_id);

  palettes = ligma_data_factory_get_container (image->ligma->palette_factory);

  ligma_container_add (palettes, LIGMA_OBJECT (private->palette));

  g_free (palette_name);
  g_free (palette_id);
}

void
ligma_image_colormap_dispose (LigmaImage *image)
{
  LigmaImagePrivate *private;
  LigmaContainer    *palettes;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (LIGMA_IS_PALETTE (private->palette));

  palettes = ligma_data_factory_get_container (image->ligma->palette_factory);

  ligma_container_remove (palettes, LIGMA_OBJECT (private->palette));
}

void
ligma_image_colormap_free (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (LIGMA_IS_PALETTE (private->palette));

  g_clear_object (&private->palette);

  /* don't touch the image's babl_palettes because we might still have
   * buffers with that palette on the undo stack, and on undoing the
   * image back to indexed, we must have exactly these palettes around
   */
}

void
ligma_image_colormap_update_formats (LigmaImage *image)
{
  LigmaImagePrivate *private;
  const Babl       *space;
  gchar            *format_name;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  space = ligma_image_get_layer_space (image);

  format_name = g_strdup_printf ("-ligma-indexed-format-%d",
                                 ligma_image_get_id (image));

  babl_new_palette_with_space (format_name, space,
                               &private->babl_palette_rgb,
                               &private->babl_palette_rgba);

  g_free (format_name);

  if (private->palette && ligma_palette_get_n_colors (private->palette) > 0)
    {
      guchar *colormap;
      gint    n_colors;

      colormap = ligma_image_get_colormap (image);
      n_colors = ligma_image_get_colormap_size (image);

      babl_palette_set_palette (private->babl_palette_rgb,
                                ligma_babl_format (LIGMA_RGB,
                                                  private->precision, FALSE,
                                                  space),
                                colormap,
                                n_colors);
      babl_palette_set_palette (private->babl_palette_rgba,
                                ligma_babl_format (LIGMA_RGB,
                                                  private->precision, FALSE,
                                                  space),
                                colormap,
                                n_colors);

      g_free (colormap);
    }
}

const Babl *
ligma_image_colormap_get_rgb_format (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->babl_palette_rgb;
}

const Babl *
ligma_image_colormap_get_rgba_format (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->babl_palette_rgba;
}

LigmaPalette *
ligma_image_get_colormap_palette (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->palette;
}

void
ligma_image_set_colormap_palette (LigmaImage   *image,
                                 LigmaPalette *palette,
                                 gboolean     push_undo)
{
  LigmaImagePrivate *private;
  LigmaPaletteEntry *entry;
  gint              n_colors, i;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (palette != NULL);
  n_colors = ligma_palette_get_n_colors (palette);
  g_return_if_fail (n_colors >= 0 && n_colors <= 256);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    ligma_image_undo_push_image_colormap (image, C_("undo-type", "Set Colormap"));

  if (!private->palette)
    ligma_image_colormap_init (image);

  ligma_data_freeze (LIGMA_DATA (private->palette));

  while ((entry = ligma_palette_get_entry (private->palette, 0)))
    ligma_palette_delete_entry (private->palette, entry);

  for (i = 0; i < n_colors; i++)
    {
      LigmaPaletteEntry *entry = ligma_palette_get_entry (palette, i);
      ligma_image_colormap_set_palette_entry (image, &entry->color, i);
    }

  ligma_data_thaw (LIGMA_DATA (private->palette));

  ligma_image_colormap_changed (image, -1);
}

guchar *
ligma_image_get_colormap (LigmaImage *image)
{
  LigmaImagePrivate *private;
  guchar           *colormap = NULL;
  gint              n_colors, i;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->palette == NULL)
    return NULL;

  n_colors = ligma_palette_get_n_colors (private->palette);

  if (n_colors > 0)
    {
      colormap = g_new0 (guchar, LIGMA_IMAGE_COLORMAP_SIZE);

      for (i = 0; i < n_colors; i++)
        {
          LigmaPaletteEntry *entry = ligma_palette_get_entry (private->palette, i);
          ligma_rgb_get_uchar (&entry->color,
                              &colormap[i * 3],
                              &colormap[i * 3 + 1],
                              &colormap[i * 3 + 2]);
        }
    }

  return colormap;
}

gint
ligma_image_get_colormap_size (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);
  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->palette == NULL)
    return 0;

  return ligma_palette_get_n_colors (private->palette);
}

void
ligma_image_set_colormap (LigmaImage    *image,
                         const guchar *colormap,
                         gint          n_colors,
                         gboolean      push_undo)
{
  LigmaImagePrivate *private;
  LigmaPaletteEntry *entry;
  gint              i;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (colormap != NULL || n_colors == 0);
  g_return_if_fail (n_colors >= 0 && n_colors <= 256);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    ligma_image_undo_push_image_colormap (image, C_("undo-type", "Set Colormap"));

  if (!private->palette)
    ligma_image_colormap_init (image);

  ligma_data_freeze (LIGMA_DATA (private->palette));

  while ((entry = ligma_palette_get_entry (private->palette, 0)))
    ligma_palette_delete_entry (private->palette, entry);

  for (i = 0; i < n_colors; i++)
    {
      LigmaRGB color;
      ligma_rgba_set_uchar (&color,
                           colormap[3 * i + 0],
                           colormap[3 * i + 1],
                           colormap[3 * i + 2],
                           255);
      ligma_image_colormap_set_palette_entry (image, &color, i);
    }

  ligma_data_thaw (LIGMA_DATA (private->palette));

  ligma_image_colormap_changed (image, -1);
}

void
ligma_image_unset_colormap (LigmaImage *image,
                           gboolean   push_undo)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    ligma_image_undo_push_image_colormap (image,
                                         C_("undo-type", "Unset Colormap"));

  if (private->palette)
    {
      ligma_image_colormap_dispose (image);
      ligma_image_colormap_free (image);
    }

  ligma_image_colormap_changed (image, -1);
}

void
ligma_image_get_colormap_entry (LigmaImage *image,
                               gint       color_index,
                               LigmaRGB   *color)
{
  LigmaImagePrivate *private;
  LigmaPaletteEntry *entry;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette != NULL);
  g_return_if_fail (color_index >= 0 &&
                    color_index < ligma_palette_get_n_colors (private->palette));
  g_return_if_fail (color != NULL);

  entry = ligma_palette_get_entry (private->palette, color_index);

  g_return_if_fail (entry != NULL);

  *color = entry->color;
}

void
ligma_image_set_colormap_entry (LigmaImage     *image,
                               gint           color_index,
                               const LigmaRGB *color,
                               gboolean       push_undo)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette != NULL);
  g_return_if_fail (color_index >= 0 &&
                    color_index < ligma_palette_get_n_colors (private->palette));
  g_return_if_fail (color != NULL);

  if (push_undo)
    ligma_image_undo_push_image_colormap (image,
                                         C_("undo-type", "Change Colormap entry"));

  ligma_image_colormap_set_palette_entry (image, color, color_index);

  ligma_image_colormap_changed (image, color_index);
}

void
ligma_image_add_colormap_entry (LigmaImage     *image,
                               const LigmaRGB *color)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette != NULL);
  g_return_if_fail (ligma_palette_get_n_colors (private->palette) < 256);
  g_return_if_fail (color != NULL);

  ligma_image_undo_push_image_colormap (image,
                                       C_("undo-type", "Add Color to Colormap"));

  ligma_image_colormap_set_palette_entry (image, color,
                                         ligma_palette_get_n_colors (private->palette));

  ligma_image_colormap_changed (image, -1);
}


/*  private functions  */

static void
ligma_image_colormap_set_palette_entry (LigmaImage     *image,
                                       const LigmaRGB *color,
                                       gint           index)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);
  LigmaRGB           black;
  gchar             name[64];

  g_return_if_fail (color != NULL);

  ligma_rgb_set (&black, 0, 0, 0);
  while (ligma_palette_get_n_colors (private->palette) <= index)
    ligma_palette_add_entry (private->palette, index, name, &black);

  g_snprintf (name, sizeof (name), "#%d", index);

  ligma_palette_set_entry (private->palette, index, name, color);
}
