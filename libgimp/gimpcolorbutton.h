/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * Gimp Color Button
 * Copyright (C) 1999 Sven Neumann
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

/* This provides a button with a color preview. The preview
 * can handle transparency by showing the checkerboard.
 * On click, a color selector is opened, which is already
 * fully functional wired to the preview button.
 */

#ifndef __GIMP_COLOR_BUTTON_H__
#define __GIMP_COLOR_BUTTON_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GIMP_TYPE_COLOR_BUTTON            (gimp_color_button_get_type ())
#define GIMP_COLOR_BUTTON(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_COLOR_BUTTON, GimpColorButton))
#define GIMP_COLOR_BUTTON_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_BUTTON, GimpColorButtonClass))
#define GIMP_IS_COLOR_BUTTON(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_COLOR_BUTTON))
#define GIMP_IS_COLOR_BUTTON_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_BUTTON))

typedef struct _GimpColorButton       GimpColorButton;
typedef struct _GimpColorButtonClass  GimpColorButtonClass;

struct _GimpColorButtonClass
{
  GtkButtonClass parent_class;

  void (* color_changed) (GimpColorButton *gcb);
};


GtkType    gimp_color_button_get_type   (void);
GtkWidget* gimp_color_button_new        (gchar   *title,
					 gint     width,
					 gint     height,
					 guchar  *color,
					 gint     bpp);
GtkWidget* gimp_color_button_double_new (gchar   *title,
					 gint     width,
					 gint     height,
					 gdouble *color,
					 gint     bpp);
void       gimp_color_button_update     (GimpColorButton *gcb);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __COLOR_BUTTON_H__ */
