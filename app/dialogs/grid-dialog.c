/* The GIMP -- an image manipulation program
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

#include "libgimpbase/gimplimits.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-types.h"
#include "config/gimpconfig-utils.h"

#include "core/gimp.h"
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


#define GRID_COLOR_SIZE 20


/*  local functions  */


static void grid_changed_cb (GtkWidget *widget,
                             gpointer   data);
static void reset_callback  (GtkWidget  *widget,
                             GtkWidget  *dialog);
static void cancel_callback (GtkWidget  *widget,
                             GtkWidget  *dialog);
static void ok_callback     (GtkWidget  *widget,
                             GtkWidget  *dialog);


/*  public function  */


GtkWidget *
grid_dialog_new (GimpImage *gimage)
{
  GimpGrid  *grid;
  GimpGrid  *grid_backup;
  GtkWidget *dialog;
  GtkWidget *editor;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  grid = gimp_image_get_grid (GIMP_IMAGE (gimage));

  grid_backup = g_object_new (GIMP_TYPE_GRID, NULL);
  gimp_config_copy_properties (G_OBJECT (grid),
                               G_OBJECT (grid_backup));

  /* dialog */
  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (gimage),
                                     _("Configure Grid"), "configure_grid",
                                     GIMP_STOCK_GRID, _("Configure Image Grid"),
                                     gimp_standard_help_func,
                                     GIMP_HELP_IMAGE_GRID,

                                     GIMP_STOCK_RESET, reset_callback,
                                     NULL, NULL, NULL, FALSE, FALSE,

                                     GTK_STOCK_CANCEL, cancel_callback,
                                     NULL, NULL, NULL, FALSE, TRUE,

                                     GTK_STOCK_OK, ok_callback,
                                     NULL, NULL, NULL, TRUE, FALSE,

                                     NULL);

  editor = gimp_grid_editor_new (grid,
                                 gimage->xresolution,
                                 gimage->yresolution);
  gtk_container_set_border_width (GTK_CONTAINER (editor), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
		     editor);

  g_signal_connect (editor, "grid_changed",
                    G_CALLBACK (grid_changed_cb),
                    gimage);

  gtk_widget_show (editor);

  g_object_set_data (G_OBJECT (dialog), "gimage", gimage);
  g_object_set_data (G_OBJECT (dialog), "grid", grid);

  g_object_set_data_full (G_OBJECT (dialog), "grid-backup", grid_backup,
                          (GDestroyNotify) g_object_unref);

  return dialog;
}


/*  local functions  */


static void
grid_changed_cb (GtkWidget *widget,
                 gpointer   data)
{
  g_return_if_fail (GIMP_IS_IMAGE (data));

  gimp_image_grid_changed (GIMP_IMAGE (data));
}

static void
reset_callback (GtkWidget  *widget,
                GtkWidget  *dialog)
{
  GimpImage *gimage;
  GimpImage *grid;
  GimpGrid  *grid_backup;

  gimage      = g_object_get_data (G_OBJECT (dialog), "gimage");
  grid        = g_object_get_data (G_OBJECT (dialog), "grid");
  grid_backup = g_object_get_data (G_OBJECT (dialog), "grid-backup");

  gimp_config_copy_properties (G_OBJECT (grid_backup),
                               G_OBJECT (grid));
  gimp_image_grid_changed (GIMP_IMAGE (gimage));
}

static void
cancel_callback (GtkWidget  *widget,
                 GtkWidget  *dialog)
{
  GimpImage *gimage;
  GimpGrid  *grid_backup;

  gimage      = g_object_get_data (G_OBJECT (dialog), "gimage");
  grid_backup = g_object_get_data (G_OBJECT (dialog), "grid-backup");

  gimp_image_set_grid (GIMP_IMAGE (gimage), grid_backup, FALSE);

  gtk_widget_destroy (dialog);
}

static void
ok_callback (GtkWidget  *widget,
             GtkWidget  *dialog)
{
  GimpImage *gimage;
  GimpGrid  *grid;
  GimpGrid  *grid_backup;

  gimage      = g_object_get_data (G_OBJECT (dialog), "gimage");
  grid        = g_object_get_data (G_OBJECT (dialog), "grid");
  grid_backup = g_object_get_data (G_OBJECT (dialog), "grid-backup");

  if (! gimp_config_is_equal_to (G_OBJECT (grid_backup), G_OBJECT (grid)))
    gimp_image_undo_push_image_grid (gimage, _("Grid"), grid_backup);

  gtk_widget_destroy (dialog);
}
