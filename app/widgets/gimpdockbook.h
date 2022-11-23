/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadockbook.h
 * Copyright (C) 2001-2007 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DOCKBOOK_H__
#define __LIGMA_DOCKBOOK_H__


#define LIGMA_TYPE_DOCKBOOK            (ligma_dockbook_get_type ())
#define LIGMA_DOCKBOOK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DOCKBOOK, LigmaDockbook))
#define LIGMA_DOCKBOOK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DOCKBOOK, LigmaDockbookClass))
#define LIGMA_IS_DOCKBOOK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DOCKBOOK))
#define LIGMA_IS_DOCKBOOK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DOCKBOOK))
#define LIGMA_DOCKBOOK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DOCKBOOK, LigmaDockbookClass))


typedef void (* LigmaDockbookDragCallback) (GdkDragContext *context,
                                           gboolean        begin,
                                           gpointer        data);


typedef struct _LigmaDockbookClass    LigmaDockbookClass;
typedef struct _LigmaDockbookPrivate  LigmaDockbookPrivate;

/**
 * LigmaDockbook:
 *
 * Holds LigmaDockables which are presented on different tabs using
 * GtkNotebook.
 */
struct _LigmaDockbook
{
  GtkNotebook parent_instance;

  LigmaDockbookPrivate *p;
};

struct _LigmaDockbookClass
{
  GtkNotebookClass parent_class;

  void (* dockable_added)     (LigmaDockbook *dockbook,
                               LigmaDockable *dockable);
  void (* dockable_removed)   (LigmaDockbook *dockbook,
                               LigmaDockable *dockable);
  void (* dockable_reordered) (LigmaDockbook *dockbook,
                               LigmaDockable *dockable);
};


GType           ligma_dockbook_get_type                (void) G_GNUC_CONST;
GtkWidget     * ligma_dockbook_new                     (LigmaMenuFactory          *menu_factory);

void            ligma_dockbook_set_dock                (LigmaDockbook             *dockbook,
                                                       LigmaDock                 *dock);
LigmaDock      * ligma_dockbook_get_dock                (LigmaDockbook             *dockbook);

LigmaUIManager * ligma_dockbook_get_ui_manager          (LigmaDockbook             *dockbook);

GtkWidget     * ligma_dockbook_add_from_dialog_factory (LigmaDockbook             *dockbook,
                                                       const gchar              *identifiers);

void            ligma_dockbook_update_with_context     (LigmaDockbook             *dockbook,
                                                       LigmaContext              *context);
GtkWidget    *  ligma_dockbook_create_tab_widget       (LigmaDockbook             *dockbook,
                                                       LigmaDockable             *dockable);
void            ligma_dockbook_set_drag_handler        (LigmaDockbook             *dockbook,
                                                       LigmaPanedBox             *drag_handler);

void            ligma_dockbook_add_drag_callback       (LigmaDockbookDragCallback  callback,
                                                       gpointer                  data);
void            ligma_dockbook_remove_drag_callback    (LigmaDockbookDragCallback  callback,
                                                       gpointer                  data);


#endif /* __LIGMA_DOCKBOOK_H__ */
