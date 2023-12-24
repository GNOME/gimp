/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppaletteview.h
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PALETTE_VIEW_H__
#define __GIMP_PALETTE_VIEW_H__

#include "gimpview.h"


#define GIMP_TYPE_PALETTE_VIEW            (gimp_palette_view_get_type ())
#define GIMP_PALETTE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PALETTE_VIEW, GimpPaletteView))
#define GIMP_PALETTE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PALETTE_VIEW, GimpPaletteViewClass))
#define GIMP_IS_PALETTE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_PALETTE_VIEW))
#define GIMP_IS_PALETTE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PALETTE_VIEW))
#define GIMP_PALETTE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PALETTE_VIEW, GimpPaletteViewClass))


typedef struct _GimpPaletteViewClass  GimpPaletteViewClass;

struct _GimpPaletteView
{
  GimpView          parent_instance;

  GimpPaletteEntry *selected;
  GimpPaletteEntry *dnd_entry;
};

struct _GimpPaletteViewClass
{
  GimpViewClass  parent_class;

  void (* entry_clicked)   (GimpPaletteView  *view,
                            GimpPaletteEntry *entry,
                            GdkModifierType   state);
  void (* entry_selected)  (GimpPaletteView  *view,
                            GimpPaletteEntry *entry);
  void (* entry_activated) (GimpPaletteView  *view,
                            GimpPaletteEntry *entry);
  void (* entry_context)   (GimpPaletteView  *view,
                            GimpPaletteEntry *entry);
  void (* color_dropped)   (GimpPaletteView  *view,
                            GimpPaletteEntry *entry,
                            GeglColor        *color);
};


GType   gimp_palette_view_get_type     (void) G_GNUC_CONST;

void    gimp_palette_view_select_entry (GimpPaletteView  *view,
                                        GimpPaletteEntry *entry);

GimpPaletteEntry * gimp_palette_view_get_selected_entry (GimpPaletteView *view);

void               gimp_palette_view_get_entry_rect     (GimpPaletteView  *view,
                                                         GimpPaletteEntry *entry,
                                                         GdkRectangle     *rect);


#endif /* __GIMP_PALETTE_VIEW_H__ */
