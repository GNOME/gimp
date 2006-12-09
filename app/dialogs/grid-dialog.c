/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-grid.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpgrid.h"

#include "widgets/gimpgrideditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "grid-dialog.h"

#include "gimp-intl.h"


#define GRID_RESPONSE_RESET 1


/*  local functions  */

static void  grid_dialog_response (GtkWidget *widget,
                                   gint       response_id,
                                   GtkWidget *dialog);


/*  public function  */


GtkWidget *
grid_dialog_new (GimpImage   *image,
                 GimpContext *context,
                 GtkWidget   *parent)
{
  GimpGrid  *grid;
  GimpGrid  *grid_backup;
  GtkWidget *dialog;
  GtkWidget *editor;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  grid = gimp_image_get_grid (GIMP_IMAGE (image));
  grid_backup = gimp_config_duplicate (GIMP_CONFIG (grid));

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                                     _("Configure Grid"), "gimp-grid-configure",
                                     GIMP_STOCK_GRID, _("Configure Image Grid"),
                                     parent,
                                     gimp_standard_help_func,
                                     GIMP_HELP_IMAGE_GRID,

                                     GIMP_STOCK_RESET, GRID_RESPONSE_RESET,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                     NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GRID_RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (grid_dialog_response),
                    dialog);

  editor = gimp_grid_editor_new (grid, context,
                                 image->xresolution, image->yresolution);
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
                     editor);

  gtk_widget_show (editor);

  g_object_set_data (G_OBJECT (dialog), "image", image);
  g_object_set_data (G_OBJECT (dialog), "grid", grid);

  g_object_set_data_full (G_OBJECT (dialog), "grid-backup", grid_backup,
                          (GDestroyNotify) g_object_unref);

  return dialog;
}


/*  local functions  */

static void
grid_dialog_response (GtkWidget  *widget,
                      gint        response_id,
                      GtkWidget  *dialog)
{
  GimpImage *image;
  GimpImage *grid;
  GimpGrid  *grid_backup;

  image       = g_object_get_data (G_OBJECT (dialog), "image");
  grid        = g_object_get_data (G_OBJECT (dialog), "grid");
  grid_backup = g_object_get_data (G_OBJECT (dialog), "grid-backup");

  switch (response_id)
    {
    case GRID_RESPONSE_RESET:
      gimp_config_sync (G_OBJECT (image->gimp->config->default_grid),
                        G_OBJECT (grid), 0);
      break;

    case GTK_RESPONSE_OK:
      if (! gimp_config_is_equal_to (GIMP_CONFIG (grid_backup),
                                     GIMP_CONFIG (grid)))
        {
          gimp_image_undo_push_image_grid (image, _("Grid"), grid_backup);
        }

      gtk_widget_destroy (dialog);
      break;

    default:
      gimp_image_set_grid (GIMP_IMAGE (image), grid_backup, FALSE);
      gtk_widget_destroy (dialog);
    }
}
