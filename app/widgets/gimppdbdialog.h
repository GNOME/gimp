/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppdbdialog.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#pragma once


#define GIMP_TYPE_PDB_DIALOG            (gimp_pdb_dialog_get_type ())
#define GIMP_PDB_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PDB_DIALOG, GimpPdbDialog))
#define GIMP_PDB_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PDB_DIALOG, GimpPdbDialogClass))
#define GIMP_IS_PDB_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PDB_DIALOG))
#define GIMP_IS_PDB_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PDB_DIALOG))
#define GIMP_PDB_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PDB_DIALOG, GimpPdbDialogClass))


typedef struct _GimpPdbDialogClass  GimpPdbDialogClass;

struct _GimpPdbDialog
{
  GimpDialog       parent_instance;

  GimpPDB         *pdb;

  /*  The context we were created with. This is the context the plug-in
   *  exists in and must be used when calling the plug-in.
   */
  GimpContext     *caller_context;

  /*  The dialog's private context, serves just as model for the
   *  select widgets and must not be used when calling the plug-in.
   */
  GimpContext     *context;

  GType            select_type;
  GimpObject      *initial_object;
  gchar           *callback_name;
  gboolean         callback_busy;

  GimpMenuFactory *menu_factory;
  GtkWidget       *view;
};

struct _GimpPdbDialogClass
{
  GimpDialogClass  parent_class;

  GList           *dialogs;

  GimpValueArray * (* run_callback) (GimpPdbDialog  *dialog,
                                     GimpObject     *object,
                                     gboolean        closing,
                                     GError        **error);
  GimpObject     * (* get_object)   (GimpPdbDialog  *dialog);
  void             (* set_object)   (GimpPdbDialog  *dialog,
                                     GimpObject     *object);
};


GType           gimp_pdb_dialog_get_type        (void) G_GNUC_CONST;

void            gimp_pdb_dialog_run_callback    (GimpPdbDialog     **dialog,
                                                 gboolean            closing);

GimpPdbDialog * gimp_pdb_dialog_get_by_callback (GimpPdbDialogClass *klass,
                                                 const gchar        *callback_name);
