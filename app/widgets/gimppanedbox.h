/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppanedbox.h
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
 * Copyright (C)      2009 Martin Nordholts <martinn@src.gnome.org>
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

#ifndef __GIMP_PANED_BOX_H__
#define __GIMP_PANED_BOX_H__


#define GIMP_TYPE_PANED_BOX            (gimp_paned_box_get_type ())
#define GIMP_PANED_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PANED_BOX, GimpPanedBox))
#define GIMP_PANED_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PANED_BOX, GimpPanedBoxClass))
#define GIMP_IS_PANED_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PANED_BOX))
#define GIMP_IS_PANED_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PANED_BOX))
#define GIMP_PANED_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PANED_BOX, GimpPanedBoxClass))


typedef struct _GimpPanedBoxClass    GimpPanedBoxClass;
typedef struct _GimpPanedBoxPrivate  GimpPanedBoxPrivate;

/**
 * GimpPanedBox:
 *
 * A #GtkBox with the children separated by #GtkPaned:s and basic
 * docking mechanisms.
 */
struct _GimpPanedBox
{
  GtkBox parent_instance;

  GimpPanedBoxPrivate *p;
};

struct _GimpPanedBoxClass
{
  GtkBoxClass parent_class;
};


GType               gimp_paned_box_get_type              (void) G_GNUC_CONST;
GtkWidget         * gimp_paned_box_new                   (gboolean                 homogeneous,
                                                          gint                     spacing,
                                                          GtkOrientation           orientation);
void                gimp_paned_box_set_dropped_cb        (GimpPanedBox            *paned_box,
                                                          GimpPanedBoxDroppedFunc  dropped_cb,
                                                          gpointer                 dropped_cb_data);
void                gimp_paned_box_add_widget            (GimpPanedBox            *paned_box,
                                                          GtkWidget               *widget,
                                                          gint                     index);
void                gimp_paned_box_remove_widget         (GimpPanedBox            *paned_box,
                                                          GtkWidget               *widget);
gboolean            gimp_paned_box_will_handle_drag      (GimpPanedBox            *paned_box,
                                                          GtkWidget               *widget,
                                                          GdkDragContext          *context,
                                                          gint                     x,
                                                          gint                     y,
                                                          gint                     time);
void                gimp_paned_box_set_drag_handler      (GimpPanedBox            *paned_box,
                                                          GimpPanedBox            *drag_handler);


#endif /* __GIMP_PANED_BOX_H__ */
