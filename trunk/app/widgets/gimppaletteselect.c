/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppaletteselect.c
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
#include "core/gimppalette.h"
#include "core/gimpparamspecs.h"

#include "pdb/gimppdb.h"

#include "gimpcontainerbox.h"
#include "gimpdatafactoryview.h"
#include "gimppaletteselect.h"


static GObject     * gimp_palette_select_constructor  (GType          type,
                                                       guint          n_params,
                                                       GObjectConstructParam *params);

static GValueArray * gimp_palette_select_run_callback (GimpPdbDialog *dialog,
                                                       GimpObject    *object,
                                                       gboolean       closing);


G_DEFINE_TYPE (GimpPaletteSelect, gimp_palette_select, GIMP_TYPE_PDB_DIALOG)

#define parent_class gimp_palette_select_parent_class


static void
gimp_palette_select_class_init (GimpPaletteSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GimpPdbDialogClass *pdb_class    = GIMP_PDB_DIALOG_CLASS (klass);

  object_class->constructor = gimp_palette_select_constructor;

  pdb_class->run_callback   = gimp_palette_select_run_callback;
}

static void
gimp_palette_select_init (GimpPaletteSelect *dialog)
{
}

static GObject *
gimp_palette_select_constructor (GType                  type,
                                 guint                  n_params,
                                 GObjectConstructParam *params)
{
  GObject       *object;
  GimpPdbDialog *dialog;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  dialog = GIMP_PDB_DIALOG (object);

  dialog->view =
    gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                dialog->context->gimp->palette_factory,
                                dialog->context,
                                GIMP_VIEW_SIZE_MEDIUM, 1,
                                dialog->menu_factory, "<Palettes>",
                                "/palettes-popup",
                                "palettes");

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (GIMP_CONTAINER_EDITOR (dialog->view)->view),
                                       5 * (GIMP_VIEW_SIZE_MEDIUM + 2),
                                       8 * (GIMP_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), dialog->view);
  gtk_widget_show (dialog->view);

  return object;
}

static GValueArray *
gimp_palette_select_run_callback (GimpPdbDialog *dialog,
                                  GimpObject    *object,
                                  gboolean       closing)
{
  GimpPalette *palette = GIMP_PALETTE (object);

  return gimp_pdb_execute_procedure_by_name (dialog->pdb,
                                             dialog->caller_context,
                                             NULL,
                                             dialog->callback_name,
                                             G_TYPE_STRING,   object->name,
                                             GIMP_TYPE_INT32, palette->n_colors,
                                             GIMP_TYPE_INT32, closing,
                                             G_TYPE_NONE);
}
