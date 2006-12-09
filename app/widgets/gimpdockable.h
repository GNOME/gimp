/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockable.h
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_DOCKABLE_H__
#define __GIMP_DOCKABLE_H__


#include <gtk/gtkbin.h>


#define GIMP_DOCKABLE_DRAG_OFFSET (-6)


#define GIMP_TYPE_DOCKABLE            (gimp_dockable_get_type ())
#define GIMP_DOCKABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DOCKABLE, GimpDockable))
#define GIMP_DOCKABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DOCKABLE, GimpDockableClass))
#define GIMP_IS_DOCKABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DOCKABLE))
#define GIMP_IS_DOCKABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DOCKABLE))
#define GIMP_DOCKABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DOCKABLE, GimpDockableClass))


typedef struct _GimpDockableClass GimpDockableClass;

struct _GimpDockable
{
  GtkBin        parent_instance;

  gchar        *name;
  gchar        *blurb;
  gchar        *stock_id;
  gchar        *help_id;
  GimpTabStyle  tab_style;

  GimpDockbook *dockbook;

  GimpContext  *context;

  PangoLayout  *title_layout;
  GdkWindow    *title_window;
  GtkWidget    *menu_button;

  guint         blink_timeout_id;
  gint          blink_counter;

  /*  drag icon hotspot  */
  gint          drag_x;
  gint          drag_y;
};

struct _GimpDockableClass
{
  GtkBinClass  parent_class;
};


GType           gimp_dockable_get_type       (void) G_GNUC_CONST;

GtkWidget     * gimp_dockable_new            (const gchar    *name,
                                              const gchar    *blurb,
                                              const gchar    *stock_id,
                                              const gchar    *help_id);

void            gimp_dockable_set_aux_info   (GimpDockable   *dockable,
                                              GList          *aux_info);
GList         * gimp_dockable_get_aux_info   (GimpDockable   *dockable);

void            gimp_dockable_set_tab_style  (GimpDockable   *dockable,
                                              GimpTabStyle    tab_style);
GtkWidget     * gimp_dockable_get_tab_widget (GimpDockable   *dockable,
                                              GimpContext    *context,
                                              GimpTabStyle    tab_style,
                                              GtkIconSize     size);
void            gimp_dockable_set_context    (GimpDockable   *dockable,
                                              GimpContext    *context);
GimpUIManager * gimp_dockable_get_menu       (GimpDockable   *dockable,
                                              const gchar   **ui_path,
                                              gpointer       *popup_data);

void            gimp_dockable_detach         (GimpDockable   *dockable);

void            gimp_dockable_blink          (GimpDockable   *dockable);
void            gimp_dockable_blink_cancel   (GimpDockable   *dockable);


#endif /* __GIMP_DOCKABLE_H__ */
