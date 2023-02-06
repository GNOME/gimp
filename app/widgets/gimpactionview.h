/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpactionview.h
 * Copyright (C) 2004-2005  Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_ACTION_VIEW_H__
#define __GIMP_ACTION_VIEW_H__


enum
{
  GIMP_ACTION_VIEW_COLUMN_VISIBLE,
  GIMP_ACTION_VIEW_COLUMN_ACTION,
  GIMP_ACTION_VIEW_COLUMN_ICON_NAME,
  GIMP_ACTION_VIEW_COLUMN_LABEL,
  GIMP_ACTION_VIEW_COLUMN_LABEL_CASEFOLD,
  GIMP_ACTION_VIEW_COLUMN_NAME,
  GIMP_ACTION_VIEW_COLUMN_ACCEL_KEY,
  GIMP_ACTION_VIEW_COLUMN_ACCEL_MASK,
  GIMP_ACTION_VIEW_N_COLUMNS
};


#define GIMP_TYPE_ACTION_VIEW            (gimp_action_view_get_type ())
#define GIMP_ACTION_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ACTION_VIEW, GimpActionView))
#define GIMP_ACTION_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ACTION_VIEW, GimpActionViewClass))
#define GIMP_IS_ACTION_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ACTION_VIEW))
#define GIMP_IS_ACTION_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ACTION_VIEW))
#define GIMP_ACTION_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ACTION_VIEW, GimpActionViewClass))


typedef struct _GimpActionViewClass GimpActionViewClass;

struct _GimpActionView
{
  GtkTreeView    parent_instance;

  Gimp          *gimp;
  gboolean       show_shortcuts;

  gchar         *filter;
};

struct _GimpActionViewClass
{
  GtkTreeViewClass  parent_class;
};


GType       gimp_action_view_get_type   (void) G_GNUC_CONST;

GtkWidget * gimp_action_view_new        (Gimp           *gimp,
                                         const gchar    *select_action,
                                         gboolean        show_shortcuts);

void        gimp_action_view_set_filter (GimpActionView *view,
                                         const gchar    *filter);


#endif  /*  __GIMP_ACTION_VIEW_H__  */
