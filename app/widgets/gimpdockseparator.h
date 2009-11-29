/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockseparator.h
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_DOCK_SEPARATOR_H__
#define __GIMP_DOCK_SEPARATOR_H__


#define GIMP_TYPE_DOCK_SEPARATOR            (gimp_dock_separator_get_type ())
#define GIMP_DOCK_SEPARATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DOCK_SEPARATOR, GimpDockSeparator))
#define GIMP_DOCK_SEPARATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DOCK_SEPARATOR, GimpDockSeparatorClass))
#define GIMP_IS_DOCK_SEPARATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DOCK_SEPARATOR))
#define GIMP_IS_DOCK_SEPARATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DOCK_SEPARATOR))
#define GIMP_DOCK_SEPARATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DOCK_SEPARATOR, GimpDockSeparatorClass))


typedef struct _GimpDockSeparatorPrivate GimpDockSeparatorPrivate;
typedef struct _GimpDockSeparatorClass   GimpDockSeparatorClass;

/**
 * GimpDockSeparator:
 *
 * Separates dockable and acts as drop-targets to allow rearrangements
 * of them.
 */
struct _GimpDockSeparator
{
  GtkEventBox  parent_instance;

  GimpDockSeparatorPrivate  *p;
};

struct _GimpDockSeparatorClass
{
  GtkEventBoxClass  parent_class;
};


GType          gimp_dock_separator_get_type       (void) G_GNUC_CONST;
GtkWidget    * gimp_dock_separator_new            (GtkAnchorType            anchor);
void           gimp_dock_separator_set_dropped_cb (GimpDockSeparator       *separator,
                                                   GimpPanedBoxDroppedFunc  dropped_cb,
                                                   gpointer                 dropped_cb_data);
gint           gimp_dock_separator_get_insert_pos (GimpDockSeparator       *separator);
void           gimp_dock_separator_set_show_label (GimpDockSeparator       *separator,
                                                   gboolean                 show);


#endif /* __GIMP_DOCK_SEPARATOR_H__ */
