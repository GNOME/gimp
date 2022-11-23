/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapdbdialog.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PDB_DIALOG_H__
#define __LIGMA_PDB_DIALOG_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_PDB_DIALOG            (ligma_pdb_dialog_get_type ())
#define LIGMA_PDB_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PDB_DIALOG, LigmaPdbDialog))
#define LIGMA_PDB_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PDB_DIALOG, LigmaPdbDialogClass))
#define LIGMA_IS_PDB_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PDB_DIALOG))
#define LIGMA_IS_PDB_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PDB_DIALOG))
#define LIGMA_PDB_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PDB_DIALOG, LigmaPdbDialogClass))


typedef struct _LigmaPdbDialogClass  LigmaPdbDialogClass;

struct _LigmaPdbDialog
{
  LigmaDialog       parent_instance;

  LigmaPDB         *pdb;

  /*  The context we were created with. This is the context the plug-in
   *  exists in and must be used when calling the plug-in.
   */
  LigmaContext     *caller_context;

  /*  The dialog's private context, serves just as model for the
   *  select widgets and must not be used when calling the plug-in.
   */
  LigmaContext     *context;

  GType            select_type;
  LigmaObject      *initial_object;
  gchar           *callback_name;
  gboolean         callback_busy;

  LigmaMenuFactory *menu_factory;
  GtkWidget       *view;
};

struct _LigmaPdbDialogClass
{
  LigmaDialogClass  parent_class;

  GList           *dialogs;

  LigmaValueArray * (* run_callback) (LigmaPdbDialog  *dialog,
                                     LigmaObject     *object,
                                     gboolean        closing,
                                     GError        **error);
};


GType           ligma_pdb_dialog_get_type        (void) G_GNUC_CONST;

void            ligma_pdb_dialog_run_callback    (LigmaPdbDialog     **dialog,
                                                 gboolean            closing);

LigmaPdbDialog * ligma_pdb_dialog_get_by_callback (LigmaPdbDialogClass *klass,
                                                 const gchar        *callback_name);


G_END_DECLS

#endif /* __LIGMA_PDB_DIALOG_H__ */
