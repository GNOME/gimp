/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapatheditor.h
 * Copyright (C) 1999-2004 Michael Natterer <mitch@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_PATH_EDITOR_H__
#define __LIGMA_PATH_EDITOR_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_PATH_EDITOR            (ligma_path_editor_get_type ())
#define LIGMA_PATH_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PATH_EDITOR, LigmaPathEditor))
#define LIGMA_PATH_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PATH_EDITOR, LigmaPathEditorClass))
#define LIGMA_IS_PATH_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_PATH_EDITOR))
#define LIGMA_IS_PATH_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PATH_EDITOR))
#define LIGMA_PATH_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PATH_EDITOR, LigmaPathEditorClass))


typedef struct _LigmaPathEditorPrivate LigmaPathEditorPrivate;
typedef struct _LigmaPathEditorClass   LigmaPathEditorClass;

struct _LigmaPathEditor
{
  GtkBox                 parent_instance;

  LigmaPathEditorPrivate *priv;

  /* FIXME MOVE TO PRIVATE */
  GtkWidget         *upper_hbox;

  GtkWidget         *new_button;
  GtkWidget         *up_button;
  GtkWidget         *down_button;
  GtkWidget         *delete_button;

  GtkWidget         *file_entry;

  GtkListStore      *dir_list;

  GtkTreeSelection  *sel;
  GtkTreePath       *sel_path;

  GtkTreeViewColumn *writable_column;

  gint               num_items;
};

struct _LigmaPathEditorClass
{
  GtkBoxClass  parent_class;

  void (* path_changed)     (LigmaPathEditor *editor);
  void (* writable_changed) (LigmaPathEditor *editor);

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


/* For information look into the C source or the html documentation */

GType       ligma_path_editor_get_type          (void) G_GNUC_CONST;

GtkWidget * ligma_path_editor_new               (const gchar    *title,
                                                const gchar    *path);

gchar     * ligma_path_editor_get_path          (LigmaPathEditor *editor);
void        ligma_path_editor_set_path          (LigmaPathEditor *editor,
                                                const gchar    *path);

gchar     * ligma_path_editor_get_writable_path (LigmaPathEditor *editor);
void        ligma_path_editor_set_writable_path (LigmaPathEditor *editor,
                                                const gchar    *path);

gboolean    ligma_path_editor_get_dir_writable  (LigmaPathEditor *editor,
                                                const gchar    *directory);
void        ligma_path_editor_set_dir_writable  (LigmaPathEditor *editor,
                                                const gchar    *directory,
                                                gboolean        writable);

G_END_DECLS

#endif /* __LIGMA_PATH_EDITOR_H__ */
