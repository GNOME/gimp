/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpcolorarea.h
 * Copyright (C) 2001 Sven Neumann
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


#include <gtk/gtkpreview.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GIMP_TYPE_COLOR_AREA            (gimp_color_area_get_type ())
#define GIMP_COLOR_AREA(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_COLOR_AREA, GimpColorArea))
#define GIMP_COLOR_AREA_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_AREA, GimpColorAreaClass))
#define GIMP_IS_COLOR_AREA(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_COLOR_AREA))
#define GIMP_IS_COLOR_AREA_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_AREA))


typedef struct _GimpColorAreaClass  GimpColorAreaClass;

struct _GimpColorAreaClass
{
  GtkPreviewClass parent_class;

  void (* color_changed) (GimpColorArea *gca);
};


GtkType     gimp_color_area_get_type   (void);
GtkWidget * gimp_color_area_new        (GimpRGB         *color,
					gboolean         alpha,
					GdkModifierType  drag_mask);
void        gimp_color_area_set_color  (GimpColorArea   *gca,
					GimpRGB         *color);
void        gimp_color_area_get_color  (GimpColorArea   *gca,
					GimpRGB         *color); /* returns */
gboolean    gimp_color_area_has_alpha  (GimpColorArea   *gca);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_COLOR_AREA_H__ */
