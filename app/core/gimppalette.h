/* The GIMP -- an image manipulation program
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

#ifndef __GIMP_PALETTE_H__
#define __GIMP_PALETTE_H__


#include "gimpobject.h"


#define GIMP_TYPE_PALETTE            (gimp_palette_get_type ())
#define GIMP_PALETTE(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_PALETTE, GimpPalette))
#define GIMP_IS_PALETTE(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_PALETTE))
#define GIMP_PALETTE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PALETTE, GimpPaletteClass))
#define GIMP_IS_PALETTE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PALETTE))


typedef struct _GimpPaletteEntry GimpPaletteEntry;

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
  GimpObject  parent_instance;

  gchar      *filename;

  GList      *colors;
  gint        n_colors;

  gboolean    changed;

  /* EEK */
  GdkPixmap  *pixmap;
};

struct _GimpPaletteClass
{
  GimpObjectClass  parent_class;

  void (* changed) (GimpPalette *palette);
};


GtkType            gimp_palette_get_type       (void);
GimpPalette      * gimp_palette_new            (const gchar      *name);

GimpPalette      * gimp_palette_new_from_file  (const gchar      *filename);
gboolean           gimp_palette_save           (GimpPalette      *palette);

GimpPaletteEntry * gimp_palette_add_entry      (GimpPalette      *palette,
					        const gchar      *name,
					        GimpRGB          *color);
void               gimp_palette_delete_entry   (GimpPalette      *palette,
					        GimpPaletteEntry *entry);

void               gimp_palette_delete         (GimpPalette      *palette);

void               gimp_palette_update_preview (GimpPalette      *palette,
						GdkGC            *gc);


#endif /* __GIMP_PALETTE_H__ */
