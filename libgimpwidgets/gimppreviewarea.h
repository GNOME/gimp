/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_PREVIEW_AREA_H__
#define __GIMP_PREVIEW_AREA_H__

#include <gtk/gtkdrawingarea.h>


#define GIMP_TYPE_PREVIEW_AREA            (gimp_preview_area_get_type ())
#define GIMP_PREVIEW_AREA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PREVIEW_AREA, GimpPreviewArea))
#define GIMP_PREVIEW_AREA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PREVIEW_AREA, GimpPreviewAreaClass))
#define GIMP_IS_PREVIEW_AREA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PREVIEW_AREA))
#define GIMP_IS_PREVIEW_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PREVIEW_AREA))
#define GIMP_PREVIEW_AREA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PREVIEW_AREA, GimpPreviewArea))


typedef struct _GimpPreviewAreaClass  GimpPreviewAreaClass;

struct _GimpPreviewArea
{
  GtkDrawingArea   parent_instance;

  gint             width;
  gint             height;
  gint             rowstride;
  gint             dither_x;
  gint             dither_y;
  guchar          *buf;
  guchar          *cmap;
};

struct _GimpPreviewAreaClass
{
  GtkDrawingAreaClass  parent_class;
};


GType       gimp_preview_area_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_preview_area_new      (void);

void        gimp_preview_area_draw     (GimpPreviewArea *area,
                                        gint             x,
                                        gint             y,
                                        gint             width,
                                        gint             height,
                                        GimpImageType    type,
                                        const guchar    *buf,
                                        gint             rowstride);

void        gimp_preview_area_set_cmap (GimpPreviewArea *area,
                                        const guchar    *cmap,
                                        gint             num_colors);


#endif /* __GIMP_PREVIEW_AREA_H__ */
