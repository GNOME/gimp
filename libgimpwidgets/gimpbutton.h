/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbutton.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_BUTTON_H__
#define __GIMP_BUTTON_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_BUTTON            (gimp_button_get_type ())
#define GIMP_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BUTTON, GimpButton))
#define GIMP_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BUTTON, GimpButtonClass))
#define GIMP_IS_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BUTTON))
#define GIMP_IS_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BUTTON))


typedef struct _GimpButtonClass  GimpButtonClass;

struct _GimpButton
{
  GtkButton  parent_instance;

  /*< private >*/
  guint      press_state;
};

struct _GimpButtonClass
{
  GtkButtonClass  parent_class;

  void (* extended_clicked) (GimpButton *preview,
			     guint       modifier_state);
};


GType       gimp_button_get_type (void);

GtkWidget * gimp_button_new      (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_BUTTON_H__ */
