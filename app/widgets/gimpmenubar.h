/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenubar.h
 * Copyright (C) 2023 Jehan
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

#ifndef __GIMP_MENU_BAR_H__
#define __GIMP_MENU_BAR_H__


#define GIMP_TYPE_MENU_BAR            (gimp_menu_bar_get_type ())
#define GIMP_MENU_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MENU_BAR, GimpMenuBar))
#define GIMP_MENU_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MENU_BAR, GimpMenuBarClass))
#define GIMP_IS_MENU_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_MENU_BAR))
#define GIMP_IS_MENU_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MENU_BAR))
#define GIMP_MENU_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MENU_BAR, GimpMenuBarClass))


typedef struct _GimpMenuBarPrivate GimpMenuBarPrivate;
typedef struct _GimpMenuBarClass   GimpMenuBarClass;

struct _GimpMenuBar
{
  GtkMenuBar           parent_instance;

  GimpMenuBarPrivate  *priv;
};

struct _GimpMenuBarClass
{
  GtkMenuBarClass  parent_class;
};


GType        gimp_menu_bar_get_type   (void) G_GNUC_CONST;

GtkWidget  * gimp_menu_bar_new        (GimpMenuModel *model,
                                       GimpUIManager *manager);


#endif /* __GIMP_MENU_BAR_H__ */
