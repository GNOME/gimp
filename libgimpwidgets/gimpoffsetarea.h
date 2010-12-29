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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_OFFSET_AREA_H__
#define __GIMP_OFFSET_AREA_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */

#define GIMP_TYPE_OFFSET_AREA            (gimp_offset_area_get_type ())
#define GIMP_OFFSET_AREA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OFFSET_AREA, GimpOffsetArea))
#define GIMP_OFFSET_AREA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_OFFSET_AREA, GimpOffsetAreaClass))
#define GIMP_IS_OFFSET_AREA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OFFSET_AREA))
#define GIMP_IS_OFFSET_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_OFFSET_AREA))
#define GIMP_OFFSET_AREA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_OFFSET_AREA, GimpOffsetAreaClass))


typedef struct _GimpOffsetAreaClass  GimpOffsetAreaClass;

struct _GimpOffsetArea
{
  GtkDrawingArea  parent_instance;
};

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
};


GType       gimp_offset_area_get_type    (void) G_GNUC_CONST;

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
