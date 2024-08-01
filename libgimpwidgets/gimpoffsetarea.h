/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpoffsetarea.h
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_OFFSET_AREA_H__
#define __GIMP_OFFSET_AREA_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */

#define GIMP_TYPE_OFFSET_AREA (gimp_offset_area_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpOffsetArea, gimp_offset_area, GIMP, OFFSET_AREA, GtkDrawingArea)

struct _GimpOffsetAreaClass
{
  GtkDrawingAreaClass  parent_class;

  void (* offsets_changed) (GimpOffsetArea *offset_area,
                            gint            offset_x,
                            gint            offset_y);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GtkWidget * gimp_offset_area_new         (gint            orig_width,
                                          gint            orig_height);
void        gimp_offset_area_set_pixbuf  (GimpOffsetArea *offset_area,
                                          GdkPixbuf      *pixbuf);

void        gimp_offset_area_set_size    (GimpOffsetArea *offset_area,
                                          gint            width,
                                          gint            height);
void        gimp_offset_area_set_offsets (GimpOffsetArea *offset_area,
                                          gint            offset_x,
                                          gint            offset_y);


G_END_DECLS

#endif /* __GIMP_OFFSET_AREA_H__ */
