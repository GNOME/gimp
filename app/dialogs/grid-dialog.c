/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2003  Henrik Brix Andersen <brix@ligma.org>
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
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-grid.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaimage-undo-push.h"
#include "core/ligmagrid.h"

#include "widgets/ligmagrideditor.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaviewabledialog.h"

#include "grid-dialog.h"

#include "ligma-intl.h"


#define GRID_RESPONSE_RESET 1


typedef struct _GridDialog GridDialog;

struct _GridDialog
{
  LigmaImage *image;
  LigmaGrid  *grid;
  LigmaGrid  *grid_backup;
};


/*  local functions  */

static void   grid_dialog_free     (GridDialog *private);
static void   grid_dialog_response (GtkWidget  *dialog,
                                    gint        response_id,
                                    GridDialog *private);


/*  public function  */

GtkWidget *
grid_dialog_new (LigmaImage   *image,
                 LigmaContext *context,
                 GtkWidget   *parent)
{
  GridDialog *private;
  GtkWidget  *dialog;
  GtkWidget  *editor;
  gdouble     xres;
  gdouble     yres;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  private = g_slice_new0 (GridDialog);

  private->image       = image;
  private->grid        = ligma_image_get_grid (image);
  private->grid_backup = ligma_config_duplicate (LIGMA_CONFIG (private->grid));

  dialog = ligma_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                     _("Configure Grid"), "ligma-grid-configure",
                                     LIGMA_ICON_GRID, _("Configure Image Grid"),
                                     parent,
                                     ligma_standard_help_func,
                                     LIGMA_HELP_IMAGE_GRID,

                                     _("_Reset"),  GRID_RESPONSE_RESET,
                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_OK"),     GTK_RESPONSE_OK,

                                     NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GRID_RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) grid_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (grid_dialog_response),
                    private);

  ligma_image_get_resolution (image, &xres, &yres);

  editor = ligma_grid_editor_new (private->grid, context, xres, yres);
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      editor, TRUE, TRUE, 0);

  gtk_widget_show (editor);

  return dialog;
}


/*  local functions  */

static void
grid_dialog_free (GridDialog *private)
{
  g_object_unref (private->grid_backup);

  g_slice_free (GridDialog, private);
}

static void
grid_dialog_response (GtkWidget  *dialog,
                      gint        response_id,
                      GridDialog *private)
{
  switch (response_id)
    {
    case GRID_RESPONSE_RESET:
      ligma_config_sync (G_OBJECT (private->image->ligma->config->default_grid),
                        G_OBJECT (private->grid), 0);
      break;

    case GTK_RESPONSE_OK:
      if (! ligma_config_is_equal_to (LIGMA_CONFIG (private->grid_backup),
                                     LIGMA_CONFIG (private->grid)))
        {
          ligma_image_undo_push_image_grid (private->image, _("Grid"),
                                           private->grid_backup);
          ligma_image_flush (private->image);
        }

      gtk_widget_destroy (dialog);
      break;

    default:
      ligma_image_set_grid (private->image, private->grid_backup, FALSE);
      gtk_widget_destroy (dialog);
    }
}
