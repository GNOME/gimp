/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapanedbox.h
 * Copyright (C) 2001-2005 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PANED_BOX_H__
#define __LIGMA_PANED_BOX_H__


#define LIGMA_TYPE_PANED_BOX            (ligma_paned_box_get_type ())
#define LIGMA_PANED_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PANED_BOX, LigmaPanedBox))
#define LIGMA_PANED_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PANED_BOX, LigmaPanedBoxClass))
#define LIGMA_IS_PANED_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PANED_BOX))
#define LIGMA_IS_PANED_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PANED_BOX))
#define LIGMA_PANED_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PANED_BOX, LigmaPanedBoxClass))


typedef struct _LigmaPanedBoxClass    LigmaPanedBoxClass;
typedef struct _LigmaPanedBoxPrivate  LigmaPanedBoxPrivate;

/**
 * LigmaPanedBox:
 *
 * A #GtkBox with the children separated by #GtkPaned:s and basic
 * docking mechanisms.
 */
struct _LigmaPanedBox
{
  GtkBox parent_instance;

  LigmaPanedBoxPrivate *p;
};

struct _LigmaPanedBoxClass
{
  GtkBoxClass parent_class;
};


GType               ligma_paned_box_get_type              (void) G_GNUC_CONST;
GtkWidget         * ligma_paned_box_new                   (gboolean                 homogeneous,
                                                          gint                     spacing,
                                                          GtkOrientation           orientation);
void                ligma_paned_box_set_dropped_cb        (LigmaPanedBox            *paned_box,
                                                          LigmaPanedBoxDroppedFunc  dropped_cb,
                                                          gpointer                 dropped_cb_data);
void                ligma_paned_box_add_widget            (LigmaPanedBox            *paned_box,
                                                          GtkWidget               *widget,
                                                          gint                     index);
void                ligma_paned_box_remove_widget         (LigmaPanedBox            *paned_box,
                                                          GtkWidget               *widget);
gboolean            ligma_paned_box_will_handle_drag      (LigmaPanedBox            *paned_box,
                                                          GtkWidget               *widget,
                                                          GdkDragContext          *context,
                                                          gint                     x,
                                                          gint                     y,
                                                          gint                     time);
void                ligma_paned_box_set_drag_handler      (LigmaPanedBox            *paned_box,
                                                          LigmaPanedBox            *drag_handler);


#endif /* __LIGMA_PANED_BOX_H__ */
