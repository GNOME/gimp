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

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimptemplate.h"

#include "widgets/gimpenummenu.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "resize-dialog.h"

#include "gimp-intl.h"


#define RESIZE_RESPONSE_RESET 1
#define SB_WIDTH              10


typedef struct _ResizePrivate ResizePrivate;

struct _ResizePrivate
{
  Resize     resize;

  GtkCallback  ok_callback;
  gpointer     ok_data;

  /*  size frame  */
  GtkWidget *orig_width_label;
  GtkWidget *orig_height_label;

  GtkWidget *size_se;

  GtkObject *ratio_x_adj;
  GtkObject *ratio_y_adj;
  GtkWidget *constrain;

  /*  offset frame  */
  GtkWidget *offset_se;
  GtkWidget *offset_area;

  /*  resolution frame  */
  GtkWidget *printsize_se;
  GtkWidget *resolution_se;
  GtkWidget *equal_res;

  /*  interpolation menu  */
  GtkWidget *interpolation_menu;

  gint       old_width, old_height;
  gdouble    old_res_x, old_res_y;
};

static void  unit_update                 (GtkWidget *widget,
					  gpointer   data);
static gint  resize_bound_off_x          (Resize    *resize,
					  gint       off_x);
static gint  resize_bound_off_y          (Resize    *resize,
					  gint       off_y);
static void  orig_labels_update          (GtkWidget *widget,
					  gpointer   data);
static void  response_callback           (GtkWidget *widget,
                                          gint       response_id,
					  Resize    *resize);
static void  size_callback               (GtkWidget *widget,
					  Resize    *resize);
static void  ratio_callback              (GtkWidget *widget,
					  Resize    *resize);
static void  size_update                 (Resize    *widget,
					  gdouble    width,
					  gdouble    height,
					  gdouble    ratio_x,
					  gdouble    ratio_y);
static void  offset_area_offsets_changed (GtkWidget *offset_area,
                                          gint       offset_x,
                                          gint       offset_y,
                                          gpointer   data);
static void  offset_update               (GtkWidget *widget,
					  gpointer   data);
static void  offset_center_clicked       (GtkWidget *widget,
                                          gpointer   data);
static void  printsize_update            (GtkWidget *widget,
					  gpointer   data);
static void  resolution_callback         (GtkWidget *widget,
					  gpointer   data);
static void  resolution_update           (Resize    *resize,
					  gdouble    res_x,
					  gdouble    res_y);


Resize *
resize_widget_new (GimpViewable *viewable,
                   GtkWidget    *parent,
                   ResizeType    type,
		   gint          width,
		   gint          height,
		   gdouble       resolution_x,
		   gdouble       resolution_y,
		   GimpUnit      unit,
		   gboolean      dot_for_dot,
		   GCallback     ok_cb,
		   gpointer      user_data)
{
  Resize         *resize;
  ResizePrivate  *private;
  GimpCoreConfig *config;
  GtkWidget      *main_vbox;
  GtkWidget      *vbox;
  GtkWidget      *hbox;
  GtkWidget      *table;
  GtkWidget      *table2;
  GtkWidget      *label;
  GtkWidget      *frame = NULL;
  GtkWidget      *spinbutton;
  GtkObject      *adjustment;

  g_return_val_if_fail (GIMP_IS_ITEM (viewable) ||
                        GIMP_IS_IMAGE (viewable), NULL);
  g_return_val_if_fail (ok_cb != NULL, NULL);

  private = g_new0 (ResizePrivate, 1);

  private->ok_callback = (GtkCallback) ok_cb;
  private->ok_data     = user_data;
  private->old_width   = width;
  private->old_height  = height;
  private->old_res_x   = resolution_x;
  private->old_res_y   = resolution_y;

  resize = (Resize *) private;

  if (GIMP_IS_IMAGE (viewable))
    {
      resize->gimage = GIMP_IMAGE (viewable);
      resize->target = ResizeImage;
    }
  else
    {
      resize->gimage = gimp_item_get_image (GIMP_ITEM (viewable));
      resize->target = ResizeLayer;
    }

  config = resize->gimage->gimp->config;

  resize->type          = type;
  resize->width         = width;
  resize->height        = height;
  resize->resolution_x  = resolution_x;
  resize->resolution_y  = resolution_y;
  resize->unit          = unit;
  resize->ratio_x       = 1.0;
  resize->ratio_y       = 1.0;
  resize->offset_x      = 0;
  resize->offset_y      = 0;
  resize->interpolation = config->interpolation_type;

  /*  dialog box  */
  {
    const gchar *role         = NULL;
    const gchar *window_title = NULL;
    const gchar *stock_id     = NULL;
    const gchar *window_desc  = NULL;
    gchar *help_page          = NULL;

    switch (type)
      {
      case ScaleWidget:
        stock_id = GIMP_STOCK_SCALE;

	switch (resize->target)
	  {
	  case ResizeLayer:
	    role         = "gimp-layer-scale";
	    window_title = _("Scale Layer");
            window_desc  = _("Scale Layer Options");
	    help_page    = GIMP_HELP_LAYER_SCALE;
	    frame        = gtk_frame_new (_("Size"));
	    break;

	  case ResizeImage:
	    role         = "gimp-image-scale";
	    window_title = _("Scale Image");
            window_desc  = _("Scale Image Options");
	    help_page    = GIMP_HELP_IMAGE_SCALE;
	    frame        = gtk_frame_new (_("Pixel Dimensions"));
	    break;
	  }
	break;

      case ResizeWidget:
        stock_id = GIMP_STOCK_RESIZE;

	switch (resize->target)
	  {
	  case ResizeLayer:
	    role         = "gimp-layer-resize";
	    window_title = _("Layer Boundary Size");
            window_desc  = _("Set Layer Boundary Size");
	    help_page    = GIMP_HELP_LAYER_RESIZE;
	    break;

	  case ResizeImage:
	    role         = "gimp-image-resize";
	    window_title = _("Canvas Size");
            window_desc  = _("Set Image Canvas Size");
	    help_page    = GIMP_HELP_IMAGE_RESIZE;
	    break;
	  }
	frame = gtk_frame_new (_("Size"));
	break;
      }

    resize->resize_shell =
      gimp_viewable_dialog_new (viewable,
                                window_title, role,
                                stock_id, window_desc,
                                parent,
                                gimp_standard_help_func, help_page,

                                GIMP_STOCK_RESET, RESIZE_RESPONSE_RESET,
                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                NULL);

    g_signal_connect (resize->resize_shell, "response",
                      G_CALLBACK (response_callback),
                      resize);

    g_object_weak_ref (G_OBJECT (resize->resize_shell),
		       (GWeakNotify) g_free,
		       private);

    gtk_window_set_resizable (GTK_WINDOW (resize->resize_shell), FALSE);
  }

  /*  the main vbox  */
  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (resize->resize_shell)->vbox),
		     main_vbox);

  /*  the pixel dimensions frame  */
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  table = gtk_table_new (6, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 3, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  the original width & height labels  */
  label = gtk_label_new (_("Original Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  private->orig_width_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (private->orig_width_label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table),  private->orig_width_label, 1, 2, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (private->orig_width_label);

  private->orig_height_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (private->orig_height_label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), private->orig_height_label, 1, 2, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (private->orig_height_label);

  /*  the new size labels  */
  label = gtk_label_new (_("New Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  the new size sizeentry  */
  spinbutton = gimp_spin_button_new (&adjustment,
                                     1, 1, 1, 1, 10, 1,
                                     1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);

  private->size_se = gimp_size_entry_new (1, unit, "%a",
                                          TRUE, TRUE, FALSE, SB_WIDTH,
                                          GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_col_spacing (GTK_TABLE (private->size_se), 1, 4);
  gtk_table_set_row_spacing (GTK_TABLE (private->size_se), 0, 2);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->size_se),
			     GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (private->size_se), spinbutton,
			     1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  gtk_table_attach (GTK_TABLE (table), private->size_se, 1, 2, 2, 4,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (private->size_se);

  if (dot_for_dot)
    gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (private->size_se),
			      GIMP_UNIT_PIXEL);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se), 0,
				  resolution_x, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se), 1,
				  resolution_y, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (private->size_se), 0,
					 GIMP_MIN_IMAGE_SIZE,
					 GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (private->size_se), 1,
					 GIMP_MIN_IMAGE_SIZE,
					 GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (private->size_se), 0, 0, width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (private->size_se), 1, 0, height);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->size_se), 0, width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->size_se), 1, height);

  g_signal_connect (private->size_se, "value_changed",
		    G_CALLBACK (size_callback),
		    resize);
  g_signal_connect (private->size_se, "unit_changed",
		    G_CALLBACK (orig_labels_update),
		    resize);

  /*  initialize the original width & height labels  */
  orig_labels_update (private->size_se, resize);

  /*  the scale ratio labels  */
  label = gtk_label_new (_("Ratio X:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Y:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  a table for the spinbuttons and the chainbutton  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 4, 6,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  table2 = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table2), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table2), 0, 2);
  gtk_box_pack_start (GTK_BOX (hbox), table2, FALSE, FALSE, 0);
  gtk_widget_show (table2);

  /*  the scale ratio spinbuttons  */
  spinbutton =
    gimp_spin_button_new (&private->ratio_x_adj,
                          resize->ratio_x,
                          (double) GIMP_MIN_IMAGE_SIZE / (double) resize->width,
                          (double) GIMP_MAX_IMAGE_SIZE / (double) resize->width,
                          0.01, 0.1, 1,
                          0.01, 4);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);
  gtk_table_attach_defaults (GTK_TABLE (table2), spinbutton, 0, 1, 0, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (private->ratio_x_adj, "value_changed",
		    G_CALLBACK (ratio_callback),
		    resize);

  spinbutton =
    gimp_spin_button_new (&private->ratio_y_adj,
                          resize->ratio_y,
                          (double) GIMP_MIN_IMAGE_SIZE / (double) resize->height,
                          (double) GIMP_MAX_IMAGE_SIZE / (double) resize->height,
                          0.01, 0.1, 1,
                          0.01, 4);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);
  gtk_table_attach_defaults (GTK_TABLE (table2), spinbutton, 0, 1, 1, 2);
  gtk_widget_show (spinbutton);

  g_signal_connect (private->ratio_y_adj, "value_changed",
		    G_CALLBACK (ratio_callback),
		    resize);

  /*  the constrain ratio chainbutton  */
  private->constrain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (private->constrain), TRUE);
  gtk_table_attach (GTK_TABLE (table2), private->constrain, 1, 2, 0, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (private->constrain);

  gimp_help_set_help_data (GIMP_CHAIN_BUTTON (private->constrain)->button,
                           _("Constrain aspect ratio"), NULL);

  gtk_widget_show (vbox);

  /*  the offset frame  */
  if (type == ResizeWidget)
    {
      GtkWidget *button;
      GtkWidget *abox;

      frame = gtk_frame_new (_("Offset"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 4);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      /*  the offset sizeentry  */
      spinbutton = gimp_spin_button_new (&adjustment,
                                         1, 1, 1, 1, 10, 1,
                                         1, 2);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);

      private->offset_se = gimp_size_entry_new (1, unit, "%a",
                                                TRUE, FALSE, FALSE, SB_WIDTH,
                                                GIMP_SIZE_ENTRY_UPDATE_SIZE);
      gtk_table_set_col_spacing (GTK_TABLE (private->offset_se), 0, 4);
      gtk_table_set_col_spacing (GTK_TABLE (private->offset_se), 1, 4);
      gtk_table_set_col_spacing (GTK_TABLE (private->offset_se), 3, 8);
      gtk_table_set_row_spacing (GTK_TABLE (private->offset_se), 0, 2);

      gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->offset_se),
				 GTK_SPIN_BUTTON (spinbutton), NULL);
      gtk_table_attach_defaults (GTK_TABLE (private->offset_se), spinbutton,
				 1, 2, 0, 1);
      gtk_widget_show (spinbutton);

      gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (private->offset_se),
				    _("X:"), 0, 0, 1.0);
      gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (private->offset_se),
				    _("Y:"), 1, 0, 1.0);
      gtk_box_pack_start (GTK_BOX (vbox), private->offset_se, FALSE, FALSE, 0);
      gtk_widget_show (private->offset_se);

      if (dot_for_dot)
	gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (private->offset_se),
				  GIMP_UNIT_PIXEL);

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->offset_se), 0,
				      resolution_x, FALSE);
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->offset_se), 1,
				      resolution_y, FALSE);

      gimp_size_entry_set_refval_boundaries
	(GIMP_SIZE_ENTRY (private->offset_se), 0, 0, 0);
      gimp_size_entry_set_refval_boundaries
	(GIMP_SIZE_ENTRY (private->offset_se), 1, 0, 0);

      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se), 0, 0);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se), 1, 0);

      g_signal_connect (private->offset_se, "value_changed",
			G_CALLBACK (offset_update),
			resize);

      button = gtk_button_new_with_label (_("Center"));
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 4, 0);
      gtk_table_attach_defaults (GTK_TABLE (private->offset_se), button,
                                 4, 5, 1, 2);
      gtk_widget_show (button);

      g_signal_connect (button, "clicked",
			G_CALLBACK (offset_center_clicked),
			resize);

      /*  frame to hold GimpOffsetArea  */
      abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), abox, FALSE, FALSE, 0);
      gtk_widget_show (abox);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_container_add (GTK_CONTAINER (abox), frame);

      private->offset_area = gimp_offset_area_new (private->old_width,
                                                   private->old_height);
      gtk_container_add (GTK_CONTAINER (frame), private->offset_area);
      gtk_widget_show (private->offset_area);

      g_signal_connect (private->offset_area, "offsets_changed",
			G_CALLBACK (offset_area_offsets_changed),
			resize);

      gtk_widget_show (frame);
    }

  /*  the resolution stuff  */
  if ((type == ScaleWidget) && (resize->target == ResizeImage))
    {
      frame = gtk_frame_new (_("Print Size & Display Unit"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox);

      table = gtk_table_new (4, 2, FALSE);
      gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_table_set_row_spacing (GTK_TABLE (table), 1, 4);
      gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

      /*  the print size labels  */
      label = gtk_label_new (_("New Width:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Height:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);

      /*  the print size sizeentry  */
      hbox = gtk_hbox_new (FALSE, 0);
      gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 0, 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (hbox);

      spinbutton = gimp_spin_button_new (&adjustment,
                                         1, 1, 1, 1, 10, 1,
                                         1, 2);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);
      gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
      gtk_widget_show (spinbutton);

      hbox = gtk_hbox_new (FALSE, 0);
      gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 1, 2,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (hbox);

      private->printsize_se = gimp_size_entry_new (1, unit, "%a",
                                                   FALSE, FALSE, FALSE, SB_WIDTH,
                                                   GIMP_SIZE_ENTRY_UPDATE_SIZE);
      gtk_table_set_col_spacing (GTK_TABLE (private->printsize_se), 1, 4);

      gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->printsize_se),
				 GTK_SPIN_BUTTON (spinbutton), NULL);

      gtk_box_pack_start (GTK_BOX (hbox),
                          private->printsize_se, FALSE, FALSE, 0);
      gtk_widget_show (private->printsize_se);

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->printsize_se),
				      0, resolution_x, FALSE);
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->printsize_se),
				      1, resolution_y, FALSE);

      gimp_size_entry_set_refval_boundaries
	(GIMP_SIZE_ENTRY (private->printsize_se),
	 0, GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);
      gimp_size_entry_set_refval_boundaries
	(GIMP_SIZE_ENTRY (private->printsize_se),
	 1, GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);

      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->printsize_se),
				  0, resize->width);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->printsize_se),
				  1, resize->height);

      g_signal_connect (private->printsize_se, "value_changed",
			G_CALLBACK (printsize_update),
			resize);
      g_signal_connect (private->printsize_se, "unit_changed",
			G_CALLBACK (unit_update),
			resize);

      /*  the resolution labels  */
      label = gtk_label_new (_("Resolution X:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Y:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);

      /*  the resolution sizeentry  */
      spinbutton = gimp_spin_button_new (&adjustment,
                                         1, 1, 1, 1, 10, 1,
                                         1, 2);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);

      private->resolution_se =
	gimp_size_entry_new (1, config->default_image->resolution_unit,
			     _("pixels/%a"),
			     FALSE, FALSE, FALSE, SB_WIDTH,
			     GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
      gtk_table_set_col_spacing (GTK_TABLE (private->resolution_se), 1, 2);
      gtk_table_set_col_spacing (GTK_TABLE (private->resolution_se), 2, 2);
      gtk_table_set_row_spacing (GTK_TABLE (private->resolution_se), 0, 2);

      gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->resolution_se),
				 GTK_SPIN_BUTTON (spinbutton), NULL);
      gtk_table_attach_defaults (GTK_TABLE (private->resolution_se), spinbutton,
				 1, 2, 0, 1);
      gtk_widget_show (spinbutton);

      gtk_table_attach (GTK_TABLE (table), private->resolution_se, 1, 2, 2, 4,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (private->resolution_se);

      gimp_size_entry_set_refval_boundaries
	(GIMP_SIZE_ENTRY (private->resolution_se),
	 0, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);
      gimp_size_entry_set_refval_boundaries
	(GIMP_SIZE_ENTRY (private->resolution_se),
	 1, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);

      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->resolution_se),
				  0, resize->resolution_x);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->resolution_se),
				  1, resize->resolution_y);

      g_signal_connect (private->resolution_se, "value_changed",
			G_CALLBACK (resolution_callback),
			resize);

      /*  the resolution chainbutton  */
      private->equal_res = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
      gimp_chain_button_set_active
	(GIMP_CHAIN_BUTTON (private->equal_res),
	 ABS (resize->resolution_x - resize->resolution_y) < 1e-5);
      gtk_table_attach_defaults (GTK_TABLE (private->resolution_se),
				 private->equal_res, 2, 3, 0, 2);
      gtk_widget_show (private->equal_res);

      gtk_widget_show (table);
      gtk_widget_show (vbox);
    }

  /*  the interpolation menu  */
  if (type == ScaleWidget)
    {
      GtkWidget *hbox;
      GtkWidget *option_menu;

      hbox = gtk_hbox_new (FALSE, 4);
      gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new (_("Interpolation:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      private->interpolation_menu = option_menu =
        gimp_enum_option_menu_new (GIMP_TYPE_INTERPOLATION_TYPE,
                                   G_CALLBACK (gimp_menu_item_update),
                                   &resize->interpolation);
      gimp_int_option_menu_set_history (GTK_OPTION_MENU (option_menu),
                                        resize->interpolation);
      gtk_box_pack_start (GTK_BOX (hbox), option_menu,
                          FALSE, FALSE, 0);
      gtk_widget_show (option_menu);
    }

  gtk_widget_show (main_vbox);

  /*  finally, activate the first entry  */
  gimp_size_entry_grab_focus (GIMP_SIZE_ENTRY (private->size_se));

  return resize;
}

static gint
resize_bound_off_x (Resize *resize,
		    gint    off_x)
{
  ResizePrivate *private;

  private = (ResizePrivate *) resize;

  if (private->old_width <= resize->width)
    off_x = CLAMP (off_x, 0, (resize->width - private->old_width));
  else
    off_x = CLAMP (off_x, (resize->width - private->old_width), 0);

  return off_x;
}

static gint
resize_bound_off_y (Resize *resize,
		    gint    off_y)
{
  ResizePrivate *private;

  private = (ResizePrivate *) resize;

  if (private->old_height <= resize->height)
    off_y = CLAMP (off_y, 0, (resize->height - private->old_height));
  else
    off_y = CLAMP (off_y, (resize->height - private->old_height), 0);

  return off_y;
}

static void
orig_labels_update (GtkWidget *widget,
		    gpointer   data)
{
  Resize        *resize;
  ResizePrivate *private;
  GimpUnit       unit;
  gchar          format_buf[16];
  gchar          buf[32];

  static GimpUnit label_unit = GIMP_UNIT_PIXEL;

  resize  = (Resize *) data;
  private = (ResizePrivate *) resize;

  unit = gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (widget));

  if (unit != GIMP_UNIT_PERCENT)
    label_unit = unit;

  if (label_unit) /* unit != GIMP_UNIT_PIXEL */
    {
      gdouble unit_factor = gimp_unit_get_factor (label_unit);

      g_snprintf (format_buf, sizeof (format_buf), "%%.%df %s",
                  gimp_unit_get_digits (label_unit) + 1,
                  gimp_unit_get_symbol (label_unit));
      g_snprintf (buf, sizeof (buf), format_buf,
                  private->old_width * unit_factor / private->old_res_x);
      gtk_label_set_text (GTK_LABEL (private->orig_width_label), buf);
      g_snprintf (buf, sizeof (buf), format_buf,
                  private->old_height * unit_factor / private->old_res_y);
      gtk_label_set_text (GTK_LABEL (private->orig_height_label), buf);
    }
  else /* unit == GIMP_UNIT_PIXEL */
    {
      g_snprintf (buf, sizeof (buf), "%d", private->old_width);
      gtk_label_set_text (GTK_LABEL (private->orig_width_label), buf);
      g_snprintf (buf, sizeof (buf), "%d", private->old_height);
      gtk_label_set_text (GTK_LABEL (private->orig_height_label), buf);
    }
}

static void
unit_update (GtkWidget *widget,
	     gpointer   data)
{
  Resize *resize;

  resize = (Resize *) data;
  resize->unit = gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (widget));
}

static void
offset_update (GtkWidget *widget,
	       gpointer   data)
{
  Resize        *resize;
  ResizePrivate *private;

  resize  = (Resize *) data;
  private = (ResizePrivate *) resize;

  resize->offset_x = resize_bound_off_x (resize,
                                         RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->offset_se), 0)));

  resize->offset_y = resize_bound_off_y (resize,
                                         RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->offset_se), 1)));

  gimp_offset_area_set_offsets (GIMP_OFFSET_AREA (private->offset_area),
                                resize->offset_x, resize->offset_y);
}

static void
offset_center_clicked (GtkWidget *widget,
                       gpointer   data)
{
  Resize        *resize;
  ResizePrivate *private;
  gint           off_x, off_y;

  resize  = (Resize *) data;
  private = (ResizePrivate *) resize;

  off_x = resize_bound_off_x (resize,
                              (resize->width - private->old_width)  / 2);

  off_y = resize_bound_off_y (resize,
                              (resize->height - private->old_height) / 2);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se), 0, off_x);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se), 1, off_y);

  g_signal_emit_by_name (private->offset_se, "value_changed", 0);
}

static void
response_callback (GtkWidget *widget,
                   gint       response_id,
                   Resize    *resize)
{
  ResizePrivate *private = (ResizePrivate *) resize;

  switch (response_id)
    {
    case RESIZE_RESPONSE_RESET:
      /* restore size and ratio settings */
      size_update (resize, private->old_width, private->old_height, 1.0, 1.0);

      if ((resize->type == ScaleWidget) && (resize->target == ResizeImage))
        {
          /* restore resolution settings */
          resolution_update (resize, private->old_res_x, private->old_res_y);
        }

      if (resize->type == ScaleWidget)
        {
          resize->interpolation =
            resize->gimage->gimp->config->interpolation_type;

          gimp_int_option_menu_set_history (GTK_OPTION_MENU (private->interpolation_menu),
                                            resize->interpolation);
        }
      break;

    case GTK_RESPONSE_OK:
      private->ok_callback (widget, private->ok_data);
      break;

    default:
      gtk_widget_destroy (widget);
      break;
    }
}

static void
size_callback (GtkWidget *widget,
               Resize    *resize)
{
  ResizePrivate *private = (ResizePrivate *) resize;
  gdouble        width;
  gdouble        height;
  gdouble        ratio_x;
  gdouble        ratio_y;

  width  = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->size_se), 0);
  height = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->size_se), 1);

  ratio_x = width  / (gdouble) private->old_width;
  ratio_y = height / (gdouble) private->old_height;

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (private->constrain)))
    {
      if (ratio_x != resize->ratio_x)
	{
	  ratio_y = ratio_x;
	  height = (gdouble) private->old_height * ratio_y;
	  height = CLAMP (height, GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);
	}
      else
	{
	  ratio_x = ratio_y;
	  width = (gdouble) private->old_width * ratio_x;
	  width = CLAMP (width, GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);
	}
    }

  size_update (resize, width, height, ratio_x, ratio_y);
}

static void
ratio_callback (GtkWidget *widget,
                Resize    *resize)
{
  ResizePrivate *private = (ResizePrivate *) resize;
  gdouble        width;
  gdouble        height;
  gdouble        ratio_x;
  gdouble        ratio_y;

  ratio_x = GTK_ADJUSTMENT (private->ratio_x_adj)->value;
  ratio_y = GTK_ADJUSTMENT (private->ratio_y_adj)->value;

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (private->constrain)))
    {
      if (ratio_x != resize->ratio_x)
	{
	  ratio_y = ratio_x;
	}
      else
	{
	  ratio_x = ratio_y;
	}
    }

  width  = CLAMP (private->old_width * ratio_x,
		  GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);
  height = CLAMP (private->old_height * ratio_y,
		  GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);

  size_update (resize, width, height, ratio_x, ratio_y);
}

static void
size_update (Resize *resize,
	     double  width,
	     double  height,
	     double  ratio_x,
	     double  ratio_y)
{
  ResizePrivate *private;

  private = (ResizePrivate *) resize;

  resize->width  = (gint) (width  + 0.5);
  resize->height = (gint) (height + 0.5);

  resize->ratio_x = ratio_x;
  resize->ratio_y = ratio_y;

  if (private->offset_area)
    gimp_offset_area_set_size (GIMP_OFFSET_AREA (private->offset_area),
			       resize->width, resize->height);

  g_signal_handlers_block_by_func (private->size_se,
                                   size_callback,
                                   resize);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->size_se),
			      0, width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->size_se),
			      1, height);

  g_signal_handlers_unblock_by_func (private->size_se,
                                     size_callback,
                                     resize);

  g_signal_handlers_block_by_func (private->ratio_x_adj,
                                   ratio_callback,
                                   resize);
  g_signal_handlers_block_by_func (private->ratio_y_adj,
                                   ratio_callback,
                                   resize);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (private->ratio_x_adj), ratio_x);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (private->ratio_y_adj), ratio_y);

  g_signal_handlers_unblock_by_func (private->ratio_x_adj,
                                     ratio_callback,
                                     resize);
  g_signal_handlers_unblock_by_func (private->ratio_y_adj,
                                     ratio_callback,
                                     resize);

  if (resize->type == ResizeWidget)
    {
      gimp_size_entry_set_refval_boundaries
        (GIMP_SIZE_ENTRY (private->offset_se), 0,
         MIN (0, resize->width - private->old_width),
         MAX (0, resize->width - private->old_width));
      gimp_size_entry_set_refval_boundaries
        (GIMP_SIZE_ENTRY (private->offset_se), 1,
         MIN (0, resize->height - private->old_height),
         MAX (0, resize->height - private->old_height));
    }

  if ((resize->type == ScaleWidget) && (resize->target == ResizeImage))
    {
      g_signal_handlers_block_by_func (private->printsize_se,
                                       printsize_update,
                                       resize);

      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->printsize_se),
				  0, width);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->printsize_se),
				  1, height);

      g_signal_handlers_unblock_by_func (private->printsize_se,
                                         printsize_update,
                                         resize);
    }
}

static void
offset_area_offsets_changed (GtkWidget *offset_area,
                             gint       offset_x,
                             gint       offset_y,
                             gpointer   data)
{
  Resize        *resize;
  ResizePrivate *private;

  resize  = (Resize *) data;
  private = (ResizePrivate *) resize;

  resize->offset_x = offset_x;
  resize->offset_y = offset_y;

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se),
                              0, resize->offset_x);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se),
                              1, resize->offset_y);
}


static void
printsize_update (GtkWidget *widget,
		  gpointer   data)
{
  Resize        *resize;
  ResizePrivate *private;
  gdouble        width;
  gdouble        height;
  gdouble        print_width;
  gdouble        print_height;
  gdouble        res_x;
  gdouble        res_y;

  resize  = (Resize *) data;
  private = (ResizePrivate *) resize;

  width  = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->size_se), 0);
  height = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->size_se), 1);

  print_width =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->printsize_se), 0);
  print_height =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->printsize_se), 1);

  /*  this is tricky: we use the sizes in pixels (which is otherwise
   *  meaningless for the "print size" widgets) to calculate the new
   *  resolution.
   */
  res_x = CLAMP (resize->resolution_x * width / print_width,
		 GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);
  res_y = CLAMP (resize->resolution_y * height / print_height,
		 GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (private->equal_res)))
    {
      if (res_x != resize->resolution_x)
	{
	  res_y = res_x;
	}
      else
	{
	  res_x = res_y;
	}
    }

  resize->resolution_x = res_x;
  resize->resolution_y = res_y;

  g_signal_handlers_block_by_func (private->resolution_se,
                                   resolution_callback,
                                   resize);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->resolution_se),
			      0, res_x);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->resolution_se),
			      1, res_y);

  g_signal_handlers_unblock_by_func (private->resolution_se,
                                     resolution_callback,
                                     resize);

  g_signal_handlers_block_by_func (private->size_se,
                                   size_callback,
                                   resize);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se),
				  0, res_x, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se),
				  1, res_y, TRUE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->size_se),
			      0, width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->size_se),
			      1, height);

  g_signal_handlers_unblock_by_func (private->size_se,
                                     size_callback,
                                     resize);

  g_signal_handlers_block_by_func (private->printsize_se,
                                   printsize_update,
                                   resize);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->printsize_se),
				  0, res_x, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->printsize_se),
				  1, res_y, TRUE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->printsize_se),
			      0, width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->printsize_se),
			      1, height);

  g_signal_handlers_unblock_by_func (private->printsize_se,
                                     printsize_update,
                                     resize);
}

/* Callback for resolution change. */
static void
resolution_callback (GtkWidget *widget,
		     gpointer   data)
{
  Resize        *resize;
  ResizePrivate *private;
  gdouble        res_x;
  gdouble        res_y;

  resize  = (Resize *) data;
  private = (ResizePrivate *) resize;

  res_x =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->resolution_se), 0);
  res_y =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->resolution_se), 1);

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (private->equal_res)))
    {
      if (res_x != resize->resolution_x)
	{
	  res_y = res_x;
	}
      else
	{
	  res_x = res_y;
	}
    }

  resolution_update (resize, res_x, res_y);
}

/* Update widgets with resolution settings found in given Resize struct. */
static void
resolution_update (Resize  *resize,
		   gdouble  res_x,
		   gdouble  res_y)
{
  ResizePrivate *private;

  private = (ResizePrivate *) resize;

  resize->resolution_x = res_x;
  resize->resolution_y = res_y;

  g_signal_handlers_block_by_func (private->resolution_se,
                                   resolution_callback,
                                   resize);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->resolution_se),
			      0, res_x);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->resolution_se),
			      1, res_y);

  g_signal_handlers_unblock_by_func (private->resolution_se,
                                     resolution_callback,
                                     resize);

  g_signal_handlers_block_by_func (private->size_se,
                                   size_callback,
                                   resize);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se),
				  0, res_x, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se),
				  1, res_y, TRUE);

  g_signal_handlers_unblock_by_func (private->size_se,
                                     size_callback,
                                     resize);

  g_signal_handlers_block_by_func (private->printsize_se,
                                   printsize_update,
                                   resize);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->printsize_se),
				  0, res_x, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->printsize_se),
				  1, res_y, TRUE);

  g_signal_handlers_unblock_by_func (private->printsize_se,
                                     printsize_update,
                                     resize);
}
