/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfontselect.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "text/gimpfont.h"

#include "pdb/procedural_db.h"

#include "gimpcontainerbox.h"
#include "gimpfontselect.h"
#include "gimpfontview.h"


static void       gimp_font_select_class_init   (GimpFontSelectClass *klass);

static GObject  * gimp_font_select_constructor  (GType          type,
                                                 guint          n_params,
                                                 GObjectConstructParam *params);

static Argument * gimp_font_select_run_callback (GimpPdbDialog *dialog,
                                                 GimpObject    *object,
                                                 gboolean       closing,
                                                 gint          *n_return_vals);


static GimpPdbDialogClass *parent_class = NULL;


GType
gimp_font_select_get_type (void)
{
  static GType dialog_type = 0;

  if (! dialog_type)
    {
      static const GTypeInfo dialog_info =
      {
        sizeof (GimpFontSelectClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_font_select_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpFontSelect),
        0,              /* n_preallocs    */
        NULL            /* instance_init  */
      };

      dialog_type = g_type_register_static (GIMP_TYPE_PDB_DIALOG,
                                            "GimpFontSelect",
                                            &dialog_info, 0);
    }

  return dialog_type;
}

static void
gimp_font_select_class_init (GimpFontSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GimpPdbDialogClass *pdb_class    = GIMP_PDB_DIALOG_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gimp_font_select_constructor;

  pdb_class->run_callback   = gimp_font_select_run_callback;
}

static GObject *
gimp_font_select_constructor (GType                  type,
                              guint                  n_params,
                              GObjectConstructParam *params)
{
  GObject       *object;
  GimpPdbDialog *dialog;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  dialog = GIMP_PDB_DIALOG (object);

  dialog->view = gimp_font_view_new (GIMP_VIEW_TYPE_LIST,
                                     dialog->context->gimp->fonts,
                                     dialog->context,
                                     GIMP_VIEW_SIZE_MEDIUM, 1,
                                     dialog->menu_factory);

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (GIMP_CONTAINER_EDITOR (dialog->view)->view),
                                       6 * (GIMP_VIEW_SIZE_MEDIUM + 2),
                                       6 * (GIMP_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), dialog->view);
  gtk_widget_show (dialog->view);

  return object;
}

static Argument *
gimp_font_select_run_callback (GimpPdbDialog *dialog,
                               GimpObject    *object,
                               gboolean       closing,
                               gint          *n_return_vals)
{
  return procedural_db_run_proc (dialog->caller_context->gimp,
                                 dialog->caller_context,
                                 NULL,
                                 dialog->callback_name,
                                 n_return_vals,
                                 GIMP_PDB_STRING, object->name,
                                 GIMP_PDB_INT32,  closing,
                                 GIMP_PDB_END);
}
