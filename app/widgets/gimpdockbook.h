/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockbook.h
 * Copyright (C) 2001-2007 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_DOCKBOOK_H__
#define __GIMP_DOCKBOOK_H__


#define GIMP_TYPE_DOCKBOOK            (gimp_dockbook_get_type ())
#define GIMP_DOCKBOOK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DOCKBOOK, GimpDockbook))
#define GIMP_DOCKBOOK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DOCKBOOK, GimpDockbookClass))
#define GIMP_IS_DOCKBOOK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DOCKBOOK))
#define GIMP_IS_DOCKBOOK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DOCKBOOK))
#define GIMP_DOCKBOOK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DOCKBOOK, GimpDockbookClass))


typedef struct _GimpDockbookClass    GimpDockbookClass;
typedef struct _GimpDockbookPrivate  GimpDockbookPrivate;

/**
 * GimpDockbook:
 *
 * Holds GimpDockables which are presented on different tabs using
 * GtkNotebook.
 */
struct _GimpDockbook
{
  GtkNotebook parent_instance;

  GimpDockbookPrivate *p;
};

struct _GimpDockbookClass
{
  GtkNotebookClass parent_class;

  void (* dockable_added)     (GimpDockbook *dockbook,
                               GimpDockable *dockable);
  void (* dockable_removed)   (GimpDockbook *dockbook,
                               GimpDockable *dockable);
  void (* dockable_reordered) (GimpDockbook *dockbook,
                               GimpDockable *dockable);
};


GType           gimp_dockbook_get_type                  (void) G_GNUC_CONST;
GtkWidget     * gimp_dockbook_new                       (GimpMenuFactory *menu_factory);
GimpDock      * gimp_dockbook_get_dock                  (GimpDockbook    *dockbook);
void            gimp_dockbook_set_dock                  (GimpDockbook    *dockbook,
                                                         GimpDock        *dock);
GimpUIManager * gimp_dockbook_get_ui_manager            (GimpDockbook    *dockbook);
void            gimp_dockbook_add                       (GimpDockbook    *dockbook,
                                                         GimpDockable    *dockable,
                                                         gint             position);
GtkWidget     * gimp_dockbook_add_from_dialog_factory   (GimpDockbook    *dockbook,
                                                         const gchar     *identifiers,
                                                         gint             position);
void            gimp_dockbook_remove                    (GimpDockbook    *dockbook,
                                                         GimpDockable    *dockable);
void            gimp_dockbook_update_with_context       (GimpDockbook    *dockbook,
                                                         GimpContext     *context);
GtkWidget    *  gimp_dockbook_create_tab_widget         (GimpDockbook    *dockbook,
                                                         GimpDockable    *dockable);
void            gimp_dockbook_update_auto_tab_style     (GimpDockbook    *dockbook);
gboolean        gimp_dockbook_drop_dockable             (GimpDockbook    *dockbook,
                                                         GtkWidget       *drag_source);
void            gimp_dockbook_set_drag_handler          (GimpDockbook    *dockbook,
                                                         GimpPanedBox    *drag_handler);
GimpDockable *  gimp_dockbook_drag_source_to_dockable   (GtkWidget       *drag_source);


#endif /* __GIMP_DOCKBOOK_H__ */
