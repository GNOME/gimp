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

#ifndef __GIMP_PALETTE_H__
#define __GIMP_PALETTE_H__


#include "gimpdata.h"


#define GIMP_TYPE_PALETTE            (gimp_palette_get_type ())
#define GIMP_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PALETTE, GimpPalette))
#define GIMP_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PALETTE, GimpPaletteClass))
#define GIMP_IS_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PALETTE))
#define GIMP_IS_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PALETTE))
#define GIMP_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PALETTE, GimpPaletteClass))


struct _GimpPaletteEntry
{
  GimpRGB  color;
  gchar   *name;

  /* EEK */
  gint     position;
};


typedef struct _GimpPaletteClass GimpPaletteClass;

struct _GimpPalette
{
  GimpData  parent_instance;

  GList    *colors;
  gint      n_colors;

  gint      n_columns;
};

struct _GimpPaletteClass
{
  GimpDataClass  parent_class;
};


GType              gimp_palette_get_type        (void) G_GNUC_CONST;

GimpData         * gimp_palette_new             (GimpContext      *context,
                                                 const gchar      *name);
GimpData         * gimp_palette_get_standard    (GimpContext      *context);

GList            * gimp_palette_get_colors      (GimpPalette      *palette);
gint               gimp_palette_get_n_colors    (GimpPalette      *palette);

void               gimp_palette_move_entry      (GimpPalette      *palette,
                                                 GimpPaletteEntry *entry,
                                                 gint              position);

GimpPaletteEntry * gimp_palette_add_entry       (GimpPalette      *palette,
                                                 gint              position,
                                                 const gchar      *name,
                                                 const GimpRGB    *color);
void               gimp_palette_delete_entry    (GimpPalette      *palette,
                                                 GimpPaletteEntry *entry);

gboolean           gimp_palette_set_entry       (GimpPalette      *palette,
                                                 gint              position,
                                                 const gchar      *name,
                                                 const GimpRGB    *color);
gboolean           gimp_palette_set_entry_color (GimpPalette      *palette,
                                                 gint              position,
                                                 const GimpRGB    *color);
gboolean           gimp_palette_set_entry_name  (GimpPalette      *palette,
                                                 gint              position,
                                                 const gchar      *name);
GimpPaletteEntry * gimp_palette_get_entry       (GimpPalette      *palette,
                                                 gint              position);

void               gimp_palette_set_columns     (GimpPalette      *palette,
                                                 gint              columns);
gint               gimp_palette_get_columns     (GimpPalette      *palette);

GimpPaletteEntry * gimp_palette_find_entry      (GimpPalette      *palette,
                                                 const GimpRGB    *color,
                                                 GimpPaletteEntry *start_from);


#endif /* __GIMP_PALETTE_H__ */
