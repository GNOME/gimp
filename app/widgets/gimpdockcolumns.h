/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadockcolumns.h
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#ifndef __LIGMA_DOCK_COLUMNS_H__
#define __LIGMA_DOCK_COLUMNS_H__


#define LIGMA_TYPE_DOCK_COLUMNS            (ligma_dock_columns_get_type ())
#define LIGMA_DOCK_COLUMNS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DOCK_COLUMNS, LigmaDockColumns))
#define LIGMA_DOCK_COLUMNS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DOCK_COLUMNS, LigmaDockColumnsClass))
#define LIGMA_IS_DOCK_COLUMNS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DOCK_COLUMNS))
#define LIGMA_IS_DOCK_COLUMNS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DOCK_COLUMNS))
#define LIGMA_DOCK_COLUMNS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DOCK_COLUMNS, LigmaDockColumnsClass))


typedef struct _LigmaDockColumnsClass    LigmaDockColumnsClass;
typedef struct _LigmaDockColumnsPrivate  LigmaDockColumnsPrivate;

/**
 * LigmaDockColumns:
 *
 * A widget containing LigmaDocks so that dockables are arranged in
 * columns.
 */
struct _LigmaDockColumns
{
  GtkBox parent_instance;

  LigmaDockColumnsPrivate *p;
};

struct _LigmaDockColumnsClass
{
  GtkBoxClass parent_class;

  void (* dock_added)   (LigmaDockColumns *dock_columns,
                         LigmaDock        *dock);
  void (* dock_removed) (LigmaDockColumns *dock_columns,
                         LigmaDock        *dock);
};


GType               ligma_dock_columns_get_type           (void) G_GNUC_CONST;
GtkWidget         * ligma_dock_columns_new                (LigmaContext       *context,
                                                          LigmaDialogFactory *dialog_factory,
                                                          LigmaUIManager     *ui_manager);
void                ligma_dock_columns_add_dock           (LigmaDockColumns   *dock_columns,
                                                          LigmaDock          *dock,
                                                          gint               index);
void                ligma_dock_columns_prepare_dockbook   (LigmaDockColumns   *dock_columns,
                                                          gint               dock_index,
                                                          GtkWidget        **dockbook_p);
void                ligma_dock_columns_remove_dock        (LigmaDockColumns   *dock_columns,
                                                          LigmaDock          *dock);
GList             * ligma_dock_columns_get_docks          (LigmaDockColumns   *dock_columns);
LigmaContext       * ligma_dock_columns_get_context        (LigmaDockColumns   *dock_columns);
void                ligma_dock_columns_set_context        (LigmaDockColumns   *dock_columns,
                                                          LigmaContext       *context);
LigmaDialogFactory * ligma_dock_columns_get_dialog_factory (LigmaDockColumns   *dock_columns);
LigmaUIManager     * ligma_dock_columns_get_ui_manager     (LigmaDockColumns   *dock_columns);


#endif /* __LIGMA_DOCK_COLUMNS_H__ */
