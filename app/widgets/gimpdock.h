/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadock.h
 * Copyright (C) 2001-2005 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DOCK_H__
#define __LIGMA_DOCK_H__


#define LIGMA_TYPE_DOCK            (ligma_dock_get_type ())
#define LIGMA_DOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DOCK, LigmaDock))
#define LIGMA_DOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DOCK, LigmaDockClass))
#define LIGMA_IS_DOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DOCK))
#define LIGMA_IS_DOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DOCK))
#define LIGMA_DOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DOCK, LigmaDockClass))


/* String used to separate dockables, e.g. "Tool Options, Layers" */
#define LIGMA_DOCK_DOCKABLE_SEPARATOR C_("dock", ", ")

/* String used to separate books (GtkNotebooks) within a dock,
   e.g. "Tool Options, Layers - Brushes"
 */
#define LIGMA_DOCK_BOOK_SEPARATOR C_("dock", " - ")

/* String used to separate dock columns,
   e.g. "Tool Options, Layers - Brushes | Gradients"
 */
#define LIGMA_DOCK_COLUMN_SEPARATOR C_("dock", " | ")


typedef struct _LigmaDockClass    LigmaDockClass;
typedef struct _LigmaDockPrivate  LigmaDockPrivate;

/**
 * LigmaDock:
 *
 * Contains a column of LigmaDockbooks.
 */
struct _LigmaDock
{
  GtkBox           parent_instance;

  LigmaDockPrivate *p;
};

struct _LigmaDockClass
{
  GtkBoxClass  parent_class;

  /*  virtual functions  */
  gchar * (* get_description)         (LigmaDock       *dock,
                                       gboolean        complete);
  void    (* set_host_geometry_hints) (LigmaDock       *dock,
                                       GtkWindow      *window);

  /*  signals  */
  void    (* book_added)              (LigmaDock       *dock,
                                       LigmaDockbook   *dockbook);
  void    (* book_removed)            (LigmaDock       *dock,
                                       LigmaDockbook   *dockbook);
  void    (* description_invalidated) (LigmaDock       *dock);
  void    (* geometry_invalidated)    (LigmaDock       *dock);
};


GType               ligma_dock_get_type                (void) G_GNUC_CONST;

gchar             * ligma_dock_get_description         (LigmaDock       *dock,
                                                       gboolean        complete);
void                ligma_dock_set_host_geometry_hints (LigmaDock       *dock,
                                                       GtkWindow      *window);
void                ligma_dock_invalidate_geometry     (LigmaDock       *dock);
void                ligma_dock_update_with_context     (LigmaDock       *dock,
                                                       LigmaContext    *context);
LigmaContext       * ligma_dock_get_context             (LigmaDock       *dock);
LigmaDialogFactory * ligma_dock_get_dialog_factory      (LigmaDock       *dock);
LigmaUIManager     * ligma_dock_get_ui_manager          (LigmaDock       *dock);
GList             * ligma_dock_get_dockbooks           (LigmaDock       *dock);
gint                ligma_dock_get_n_dockables         (LigmaDock       *dock);
GtkWidget         * ligma_dock_get_main_vbox           (LigmaDock       *dock);
GtkWidget         * ligma_dock_get_vbox                (LigmaDock       *dock);

void                ligma_dock_set_id                  (LigmaDock       *dock,
                                                       gint            ID);
gint                ligma_dock_get_id                  (LigmaDock       *dock);

void                ligma_dock_add_book                (LigmaDock       *dock,
                                                       LigmaDockbook   *dockbook,
                                                       gint            index);
void                ligma_dock_remove_book             (LigmaDock       *dock,
                                                       LigmaDockbook   *dockbook);


#endif /* __LIGMA_DOCK_H__ */
