/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorarea.h
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

/* This provides a color preview area. The preview
 * can handle transparency by showing the checkerboard and
 * handles drag'n'drop.
 */

#ifndef __GIMP_COLOR_AREA_H__
#define __GIMP_COLOR_AREA_H__

#include <gtk/gtkdrawingarea.h>

G_BEGIN_DECLS


#define GIMP_TYPE_COLOR_AREA            (gimp_color_area_get_type ())
#define GIMP_COLOR_AREA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_AREA, GimpColorArea))
#define GIMP_COLOR_AREA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_AREA, GimpColorAreaClass))
#define GIMP_IS_COLOR_AREA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_AREA))
#define GIMP_IS_COLOR_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_AREA))
#define GIMP_COLOR_AREA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_AREA, GimpColorAreaClass))


typedef struct _GimpColorAreaClass  GimpColorAreaClass;

struct _GimpColorArea
{
  GtkDrawingArea       parent_instance;

  /*< private >*/
  guchar              *buf;
  guint                width;
  guint                height;
  guint                rowstride;

  GimpColorAreaType    type;
  GimpRGB              color;
  guint                draw_border  : 1;
  guint                needs_render : 1;
};

struct _GimpColorAreaClass
{
  GtkDrawingAreaClass  parent_class;

  void (* color_changed) (GimpColorArea *area);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_color_area_get_type        (void) G_GNUC_CONST;

GtkWidget * gimp_color_area_new             (const GimpRGB     *color,
                                             GimpColorAreaType  type,
                                             GdkModifierType    drag_mask);

void        gimp_color_area_set_color       (GimpColorArea     *area,
                                             const GimpRGB     *color);
void        gimp_color_area_get_color       (GimpColorArea     *area,
                                             GimpRGB           *color);
gboolean    gimp_color_area_has_alpha       (GimpColorArea     *area);
void        gimp_color_area_set_type        (GimpColorArea     *area,
                                             GimpColorAreaType  type);
void        gimp_color_area_set_draw_border (GimpColorArea     *area,
                                             gboolean           draw_border);

/*  only for private use in libgimpwidgets  */
G_GNUC_INTERNAL void _gimp_color_area_render_buf (GtkWidget         *widget,
                                                  gboolean           insensitive,
                                                  GimpColorAreaType  type,
                                                  guchar            *buf,
                                                  guint              width,
                                                  guint              height,
                                                  guint              rowstride,
                                                  GimpRGB           *color);


G_END_DECLS

#endif /* __GIMP_COLOR_AREA_H__ */
