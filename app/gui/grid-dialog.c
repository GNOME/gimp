/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include "core/gimpgrid.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "widgets/gimpviewabledialog.h"
#include "widgets/gimppropwidgets.h"

#include "grid-dialog.h"

#include "gimp-intl.h"


#define GRID_COLOR_SIZE 20


/*  local functions  */


static void reset_callback  (GtkWidget  *widget,
                             GtkWidget  *dialog);
static void remove_callback (GtkWidget  *widget,
                             GtkWidget  *dialog);
static void cancel_callback (GtkWidget  *widget,
                             GtkWidget  *dialog);
static void ok_callback     (GtkWidget  *widget,
                             GtkWidget  *dialog);


/*  public function  */


GtkWidget *
grid_dialog_new (GimpDisplay *gdisp)
{
  GimpImage        *gimage;
  GimpDisplayShell *shell;
  GimpGrid         *grid_orig;
  GimpGrid         *grid;

  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *type;
  GtkWidget *color_button;
  GtkWidget *sizeentry;

  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), NULL);

  gimage    = GIMP_IMAGE (gdisp->gimage);
  shell     = GIMP_DISPLAY_SHELL (gdisp->shell);

  grid_orig = gimp_image_get_grid (GIMP_IMAGE (gimage));
  grid      = g_object_new (GIMP_TYPE_GRID, NULL);

  g_object_ref (G_OBJECT (grid));

  /* the dialog */
  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (gimage),
                                     _("Configure Grid"), "configure_grid",
                                     GIMP_STOCK_GRID, _("Configure Image Grid"),
                                     gimp_standard_help_func,
                                     "dialogs/configure_grid.html",

                                     GIMP_STOCK_RESET, reset_callback,
                                     NULL, NULL, NULL, FALSE, FALSE,

                                     GTK_STOCK_REMOVE, remove_callback,
                                     NULL, NULL, NULL, FALSE, FALSE,

                                     GTK_STOCK_CANCEL, cancel_callback,
                                     NULL, NULL, NULL, FALSE, TRUE,

                                     GTK_STOCK_OK, ok_callback,
                                     NULL, NULL, NULL, TRUE, FALSE,

                                     NULL);

  /* the main vbox */
  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
		     main_vbox);

  /* the appearance frame */
  frame = gtk_frame_new (_("Appearance"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  type = gimp_prop_enum_option_menu_new (G_OBJECT (grid), "type",
                                         GIMP_GRID_TYPE_INTERSECTION,
                                         GIMP_GRID_TYPE_SOLID);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Line _Style:"), 1.0, 0.5,
                             type, 1, TRUE);

  color_button = gimp_prop_color_button_new (G_OBJECT (grid), "fgcolor",
                                             _("Change Grid Foreground Color"),
                                             GRID_COLOR_SIZE, GRID_COLOR_SIZE,
                                             GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("_Foreground Color:"), 1.0, 0.5,
                             color_button, 1, TRUE);

  color_button = gimp_prop_color_button_new (G_OBJECT (grid), "bgcolor",
                                             _("Change Grid Background Color"),
                                             GRID_COLOR_SIZE, GRID_COLOR_SIZE,
                                             GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("_Background Color:"), 1.0, 0.5,
                             color_button, 1, TRUE);

  gtk_widget_show (table);

  /* the spacing frame */
  frame = gtk_frame_new (_("Spacing"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  sizeentry = gimp_prop_coordinates_new (G_OBJECT (grid),
                                         "xspacing",
                                         "yspacing",
                                         "spacing-unit",
                                         "%a",
                                         GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                         gimage->xresolution,
                                         gimage->yresolution,
                                         TRUE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry),
                                         0, 1.0, GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry),
                                         1, 1.0, GIMP_MAX_IMAGE_SIZE);

  gtk_table_set_col_spacings (GTK_TABLE (sizeentry), 2);
  gtk_table_set_row_spacings (GTK_TABLE (sizeentry), 2);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Width"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Height"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Pixels"), 1, 4, 0.0);

  gtk_box_pack_start (GTK_BOX (hbox), sizeentry, FALSE, FALSE, 0);
  gtk_widget_show (sizeentry);

  gtk_widget_show (hbox);

  /* the offset frame */
  frame = gtk_frame_new (_("Offset"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  sizeentry =  gimp_prop_coordinates_new (G_OBJECT (grid),
                                          "xoffset",
                                          "yoffset",
                                          "offset-unit",
                                          "%a",
                                          GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                          gimage->xresolution,
                                          gimage->yresolution,
                                          TRUE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry),
                                         0, - GIMP_MAX_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry),
                                         1, - GIMP_MAX_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gtk_table_set_col_spacings (GTK_TABLE (sizeentry), 2);
  gtk_table_set_row_spacings (GTK_TABLE (sizeentry), 2);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Width"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Height"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Pixels"), 1, 4, 0.0);

  gtk_box_pack_start (GTK_BOX (hbox), sizeentry, FALSE, FALSE, 0);
  gtk_widget_show (sizeentry);

  gtk_widget_show (hbox);

  if (grid_orig)
    {
      gimp_config_copy_properties (G_OBJECT (grid_orig), G_OBJECT (grid));
    }
  else
    {
      gimp_config_reset_properties (G_OBJECT (grid));
      g_object_set (G_OBJECT (grid),
                    "spacing-unit", gimage->unit,
                    NULL);
      g_object_set (G_OBJECT (grid),
                    "offset-unit", gimage->unit,
                    NULL);
    }

  gtk_widget_show (main_vbox);

  g_object_set_data (G_OBJECT (dialog), "gimage", gimage);
  g_object_set_data (G_OBJECT (dialog), "shell", shell);

  g_object_set_data_full (G_OBJECT (dialog), "grid", grid,
                          (GDestroyNotify) g_object_unref);

  return dialog;
}


/*  local functions  */


static void
reset_callback (GtkWidget  *widget,
                GtkWidget  *dialog)
{
  GimpImage        *gimage;
  GimpDisplayShell *shell;
  GimpGrid         *grid_orig;
  GimpGrid         *grid;
  GimpUnit          unit_orig;

  gimage      = g_object_get_data (G_OBJECT (dialog), "gimage");
  grid        = g_object_get_data (G_OBJECT (dialog), "grid");
  shell       = g_object_get_data (G_OBJECT (dialog), "shell");

  grid_orig   = gimp_image_get_grid (GIMP_IMAGE (gimage));
  unit_orig   = gimp_image_get_unit (GIMP_IMAGE (gimage));

  if (grid_orig)
    {
      gimp_config_copy_properties (G_OBJECT (grid_orig),
                                   G_OBJECT (grid));
    }
  else
    {
      g_object_freeze_notify (G_OBJECT (grid));
      gimp_config_reset_properties (G_OBJECT (grid));

      g_object_set (G_OBJECT (grid),
                    "spacing-unit", unit_orig,
                    NULL);
      g_object_set (G_OBJECT (grid),
                    "offset-unit", unit_orig,
                    NULL);

      g_object_thaw_notify (G_OBJECT (grid));
    }
}

static void
remove_callback (GtkWidget  *widget,
                 GtkWidget  *dialog)
{
  GimpImage        *gimage;
  GimpDisplayShell *shell;

  gimage = g_object_get_data (G_OBJECT (dialog), "gimage");
  shell  = g_object_get_data (G_OBJECT (dialog), "shell");

  gimp_image_set_grid (gimage, NULL, TRUE);

  gtk_widget_destroy (dialog);
}


static void
cancel_callback (GtkWidget  *widget,
                 GtkWidget  *dialog)
{
  gtk_widget_destroy (dialog);
}

static void
ok_callback (GtkWidget  *widget,
             GtkWidget  *dialog)
{
  GimpImage        *gimage;
  GimpDisplayShell *shell;
  GimpGrid         *grid;
  GimpGrid         *grid_orig;

  gimage      = g_object_get_data (G_OBJECT (dialog), "gimage");
  grid        = g_object_get_data (G_OBJECT (dialog), "grid");
  shell       = g_object_get_data (G_OBJECT (dialog), "shell");

  grid_orig   = gimp_image_get_grid (GIMP_IMAGE (gimage));

  if (grid_orig == NULL || ! gimp_config_is_equal_to (G_OBJECT (grid_orig),
                                                      G_OBJECT (grid)))
    gimp_image_set_grid (GIMP_IMAGE (gimage), grid, TRUE);

  gtk_widget_destroy (dialog);
}
