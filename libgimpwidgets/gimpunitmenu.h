/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitmenu.h
 * Copyright (C) 1999 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version
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

#ifndef GIMP_DISABLE_DEPRECATED

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_UNIT_MENU_H__
#define __GIMP_UNIT_MENU_H__

#ifdef GTK_DISABLE_DEPRECATED
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtkoptionmenu.h>
#define GTK_DISABLE_DEPRECATED
#endif

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_UNIT_MENU            (gimp_unit_menu_get_type ())
#define GIMP_UNIT_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UNIT_MENU, GimpUnitMenu))
#define GIMP_UNIT_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_UNIT_MENU, GimpUnitMenuClass))
#define GIMP_IS_UNIT_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_UNIT_MENU))
#define GIMP_IS_UNIT_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_UNIT_MENU))
#define GIMP_UNIT_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_UNIT_MENU, GimpUnitMenuClass))


typedef struct _GimpUnitMenuClass  GimpUnitMenuClass;

struct _GimpUnitMenu
{
  GtkOptionMenu  parent_instance;

  /* public (read only) */
  gchar         *format;
  GimpUnit       unit;
  gint           pixel_digits;

  gboolean       show_pixels;
  gboolean       show_percent;

  /* private */
  GtkWidget     *selection;
  GtkWidget     *tv;
};

struct _GimpUnitMenuClass
{
  GtkOptionMenuClass  parent_class;

  void (* unit_changed) (GimpUnitMenu *menu);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_unit_menu_get_type         (void) G_GNUC_CONST;

GtkWidget * gimp_unit_menu_new              (const gchar  *format,
                                             GimpUnit      unit,
                                             gboolean      show_pixels,
                                             gboolean      show_percent,
                                             gboolean      show_custom);

void        gimp_unit_menu_set_unit         (GimpUnitMenu *menu,
                                             GimpUnit      unit);

GimpUnit    gimp_unit_menu_get_unit         (GimpUnitMenu *menu);

void        gimp_unit_menu_set_pixel_digits (GimpUnitMenu *menu,
                                             gint          digits);
gint        gimp_unit_menu_get_pixel_digits (GimpUnitMenu *menu);


G_END_DECLS

#endif /* __GIMP_UNIT_MENU_H__ */

#endif /* GIMP_DISABLE_DEPRECATED */
