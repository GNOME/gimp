/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolbar.h
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

#ifndef __GIMP_TOOLBAR_H__
#define __GIMP_TOOLBAR_H__


#define GIMP_TYPE_TOOLBAR            (gimp_toolbar_get_type ())
#define GIMP_TOOLBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOLBAR, GimpToolbar))
#define GIMP_TOOLBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOLBAR, GimpToolbarClass))
#define GIMP_IS_TOOLBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_TOOLBAR))
#define GIMP_IS_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOLBAR))
#define GIMP_TOOLBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOLBAR, GimpToolbarClass))


typedef struct _GimpToolbarPrivate GimpToolbarPrivate;
typedef struct _GimpToolbarClass   GimpToolbarClass;

struct _GimpToolbar
{
  GtkToolbar          parent_instance;

  GimpToolbarPrivate  *priv;
};

struct _GimpToolbarClass
{
  GtkToolbarClass  parent_class;
};


GType        gimp_toolbar_get_type   (void) G_GNUC_CONST;

GtkWidget  * gimp_toolbar_new        (GimpMenuModel *model,
                                      GimpUIManager *manager);


#endif /* __GIMP_TOOLBAR_H__ */
