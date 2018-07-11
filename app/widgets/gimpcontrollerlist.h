/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontrollerlist.h
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_CONTROLLER_LIST_H__
#define __GIMP_CONTROLLER_LIST_H__


#define GIMP_TYPE_CONTROLLER_LIST            (gimp_controller_list_get_type ())
#define GIMP_CONTROLLER_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTROLLER_LIST, GimpControllerList))
#define GIMP_CONTROLLER_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTROLLER_LIST, GimpControllerListClass))
#define GIMP_IS_CONTROLLER_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTROLLER_LIST))
#define GIMP_IS_CONTROLLER_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTROLLER_LIST))
#define GIMP_CONTROLLER_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTROLLER_LIST, GimpControllerListClass))


typedef struct _GimpControllerListClass GimpControllerListClass;

struct _GimpControllerList
{
  GtkBox              parent_instance;

  Gimp               *gimp;

  GtkWidget          *hbox;

  GtkListStore       *src;
  GtkTreeSelection   *src_sel;
  GType               src_gtype;

  GtkWidget          *dest;
  GimpControllerInfo *dest_info;

  GtkWidget          *add_button;
  GtkWidget          *remove_button;
  GtkWidget          *edit_button;
  GtkWidget          *up_button;
  GtkWidget          *down_button;
};

struct _GimpControllerListClass
{
  GtkBoxClass   parent_class;
};


GType       gimp_controller_list_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_controller_list_new      (Gimp *gimp);


#endif  /*  __GIMP_CONTROLLER_LIST_H__  */
