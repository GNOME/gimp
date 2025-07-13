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

#pragma once

#include "gimpdata.h"


#define GIMP_TYPE_PALETTE            (gimp_palette_get_type ())
#define GIMP_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PALETTE, GimpPalette))
#define GIMP_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PALETTE, GimpPaletteClass))
#define GIMP_IS_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PALETTE))
#define GIMP_IS_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PALETTE))
#define GIMP_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PALETTE, GimpPaletteClass))


struct _GimpPaletteEntry
{
  GeglColor *color;
  gchar     *name;
};


typedef struct _GimpPaletteClass GimpPaletteClass;

struct _GimpPalette
{
  GimpData  parent_instance;

  /* When set to an image, we store the image we connect to, so that we
   * can correctly disconnect if parent GimpData's image changes (so the
   * GimpPalette and GimpData's images may be unsynced for a tiny span of
   * time).
   */
  GimpImage  *image;

  /* Palette colors can be restricted to a given format. If NULL, then the
   * palette can be a mix of color models and color spaces.
   */
  const Babl *format;

  GList      *colors;
  gint        n_colors;

  gint        n_columns;
};

struct _GimpPaletteClass
{
  GimpDataClass  parent_class;

  void (* entry_changed) (GimpPalette *palette,
                          gint         index);
};


GType              gimp_palette_get_type        (void) G_GNUC_CONST;

GimpData         * gimp_palette_new             (GimpContext      *context,
                                                 const gchar      *name);
GimpData         * gimp_palette_get_standard    (GimpContext      *context);

void               gimp_palette_restrict_format (GimpPalette      *palette,
                                                 const Babl       *format,
                                                 gboolean          push_undo_if_image);
const Babl       * gimp_palette_get_restriction (GimpPalette      *palette);

GList            * gimp_palette_get_colors      (GimpPalette      *palette);
gint               gimp_palette_get_n_colors    (GimpPalette      *palette);

void               gimp_palette_move_entry      (GimpPalette      *palette,
                                                 GimpPaletteEntry *entry,
                                                 gint              position);

GimpPaletteEntry * gimp_palette_add_entry       (GimpPalette      *palette,
                                                 gint              position,
                                                 const gchar      *name,
                                                 GeglColor        *color);
void               gimp_palette_delete_entry    (GimpPalette      *palette,
                                                 GimpPaletteEntry *entry);

gboolean           gimp_palette_set_entry       (GimpPalette      *palette,
                                                 gint              position,
                                                 const gchar      *name,
                                                 GeglColor        *color);
gboolean           gimp_palette_set_entry_color (GimpPalette      *palette,
                                                 gint              position,
                                                 GeglColor        *color,
                                                 gboolean          push_undo_if_image);
gboolean           gimp_palette_set_entry_name  (GimpPalette      *palette,
                                                 gint              position,
                                                 const gchar      *name);
GimpPaletteEntry * gimp_palette_get_entry       (GimpPalette      *palette,
                                                 gint              position);
gint               gimp_palette_get_entry_position (GimpPalette   *palette,
                                                 GimpPaletteEntry *entry);

void               gimp_palette_set_columns     (GimpPalette      *palette,
                                                 gint              columns);
gint               gimp_palette_get_columns     (GimpPalette      *palette);

GimpPaletteEntry * gimp_palette_find_entry      (GimpPalette      *palette,
                                                 GeglColor        *color,
                                                 GimpPaletteEntry *start_from);

guchar           * gimp_palette_get_colormap    (GimpPalette      *palette,
                                                 const Babl       *format,
                                                 gint             *n_colors);
void               gimp_palette_set_colormap    (GimpPalette      *palette,
                                                 const Babl       *format,
                                                 const guchar     *colormap,
                                                 gint              n_colors,
                                                 gboolean          push_undo_if_image);
