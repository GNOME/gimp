/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapaletteselect.c
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmapalette.h"
#include "core/ligmaparamspecs.h"

#include "pdb/ligmapdb.h"

#include "ligmacontainerbox.h"
#include "ligmadatafactoryview.h"
#include "ligmapaletteselect.h"


static void             ligma_palette_select_constructed  (GObject        *object);

static LigmaValueArray * ligma_palette_select_run_callback (LigmaPdbDialog  *dialog,
                                                          LigmaObject     *object,
                                                          gboolean        closing,
                                                          GError        **error);


G_DEFINE_TYPE (LigmaPaletteSelect, ligma_palette_select, LIGMA_TYPE_PDB_DIALOG)

#define parent_class ligma_palette_select_parent_class


static void
ligma_palette_select_class_init (LigmaPaletteSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  LigmaPdbDialogClass *pdb_class    = LIGMA_PDB_DIALOG_CLASS (klass);

  object_class->constructed = ligma_palette_select_constructed;

  pdb_class->run_callback   = ligma_palette_select_run_callback;
}

static void
ligma_palette_select_init (LigmaPaletteSelect *dialog)
{
}

static void
ligma_palette_select_constructed (GObject *object)
{
  LigmaPdbDialog *dialog = LIGMA_PDB_DIALOG (object);
  GtkWidget     *content_area;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  dialog->view =
    ligma_data_factory_view_new (LIGMA_VIEW_TYPE_LIST,
                                dialog->context->ligma->palette_factory,
                                dialog->context,
                                LIGMA_VIEW_SIZE_MEDIUM, 1,
                                dialog->menu_factory, "<Palettes>",
                                "/palettes-popup",
                                "palettes");

  ligma_container_box_set_size_request (LIGMA_CONTAINER_BOX (LIGMA_CONTAINER_EDITOR (dialog->view)->view),
                                       5 * (LIGMA_VIEW_SIZE_MEDIUM + 2),
                                       8 * (LIGMA_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (content_area), dialog->view, TRUE, TRUE, 0);
  gtk_widget_show (dialog->view);
}

static LigmaValueArray *
ligma_palette_select_run_callback (LigmaPdbDialog  *dialog,
                                  LigmaObject     *object,
                                  gboolean        closing,
                                  GError        **error)
{
  LigmaPalette *palette = LIGMA_PALETTE (object);

  return ligma_pdb_execute_procedure_by_name (dialog->pdb,
                                             dialog->caller_context,
                                             NULL, error,
                                             dialog->callback_name,
                                             G_TYPE_STRING,  ligma_object_get_name (object),
                                             G_TYPE_INT,     ligma_palette_get_n_colors (palette),
                                             G_TYPE_BOOLEAN, closing,
                                             G_TYPE_NONE);
}
