/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpselectbutton.h
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

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_SELECT_BUTTON_H__
#define __GIMP_SELECT_BUTTON_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_SELECT_BUTTON            (gimp_select_button_get_type ())
#define GIMP_SELECT_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SELECT_BUTTON, GimpSelectButton))
#define GIMP_SELECT_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SELECT_BUTTON, GimpSelectButtonClass))
#define GIMP_IS_SELECT_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SELECT_BUTTON))
#define GIMP_IS_SELECT_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SELECT_BUTTON))
#define GIMP_SELECT_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SELECT_BUTTON, GimpSelectButtonClass))


typedef struct _GimpSelectButtonClass   GimpSelectButtonClass;

struct _GimpSelectButton
{
  GtkBox       parent_instance;

  const gchar *temp_callback;
};

struct _GimpSelectButtonClass
{
  GtkBoxClass  parent_class;

  gchar       *default_title;

  void (*select_destroy) (const gchar *callback);

  /* Padding for future expansion */
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
  void (*_gimp_reserved5) (void);
  void (*_gimp_reserved6) (void);
  void (*_gimp_reserved7) (void);
};


GType       gimp_select_button_get_type    (void) G_GNUC_CONST;

void        gimp_select_button_close_popup (GimpSelectButton *button);


G_END_DECLS

#endif /* __GIMP_SELECT_BUTTON_H__ */
