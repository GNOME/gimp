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

#include "appenv.h"
#include "gdisplay.h"
#include "resize.h"
#include "undo.h"
#include "gimprc.h"
#include "gimpui.h"

#include "libgimp/gimpchainbutton.h"
#include "libgimp/gimplimits.h"
#include "libgimp/gimpmath.h"
#include "libgimp/gimpsizeentry.h"

#include "libgimp/gimpintl.h"

#define EVENT_MASK        GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK
#define DRAWING_AREA_SIZE 200
#define TEXT_WIDTH        35

typedef struct _ResizePrivate ResizePrivate;

struct _ResizePrivate
{
  /*  size frame  */
  GtkWidget *orig_width_label;
  GtkWidget *orig_height_label;

  GtkWidget *size_se;

  GtkObject *ratio_x_adj;
  GtkObject *ratio_y_adj;
  GtkWidget *constrain;

  /*  offset frame  */
  GtkWidget *offset_se;
  GtkWidget *drawing_area;

  /*  resolution frame  */
  GtkWidget *printsize_se;
  GtkWidget *resolution_se;
  GtkWidget *equal_res;

  gdouble ratio;
  gint    old_width, old_height;
  gdouble old_res_x, old_res_y;
  gint    area_width, area_height;
  gint    start_x, start_y;
  gint    orig_x, orig_y;
};

static void  resize_draw                 (Resize *);
static void  unit_update                 (GtkWidget *, gpointer);
static gint  resize_bound_off_x          (Resize *, gint);
static gint  resize_bound_off_y          (Resize *, gint);
static void  orig_labels_update          (GtkWidget *, gpointer);
static void  size_callback               (GtkWidget *, gpointer);
static void  ratio_callback              (GtkWidget *, gpointer);
static void  size_update                 (Resize *, gdouble, gdouble, gdouble, gdouble);
static void  offset_update               (GtkWidget *, gpointer);
static gint  resize_events               (GtkWidget *, GdkEvent *);
static void  printsize_update            (GtkWidget *, gpointer);
static void  resolution_update           (GtkWidget *, gpointer);
static void  resize_scale_warn_callback  (GtkWidget *, gboolean, gpointer);

Resize *
resize_widget_new (ResizeType    type,
		   ResizeTarget  target,
		   GtkObject    *object,
		   gchar        *signal,
		   gint          width,
		   gint          height,
		   gdouble       resolution_x,
		   gdouble       resolution_y,
		   GimpUnit      unit,
		   gboolean      dot_for_dot,
		   GtkSignalFunc ok_cb,
		   GtkSignalFunc cancel_cb,
		   gpointer      user_data)
{
  Resize *resize;
  ResizePrivate *private;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *table;
  GtkWidget *table2;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *spinbutton;
  GtkWidget *alignment;
  GtkObject *adjustment;

  alignment = NULL;
  frame = NULL;

  private = g_new (ResizePrivate, 1);
  private->old_width  = width;
  private->old_height = height;
  private->old_res_x  = resolution_x;
  private->old_res_y  = resolution_y;

  resize = g_new (Resize, 1);
  resize->type         = type;
  resize->target       = target;
  resize->private_part = private;
  resize->width        = width;
  resize->height       = height;
  resize->resolution_x = resolution_x;
  resize->resolution_y = resolution_y;
  resize->unit         = unit;
  resize->ratio_x      = 1.0;
  resize->ratio_y      = 1.0;
  resize->offset_x     = 0;
  resize->offset_y     = 0;

  /*  Get the image width and height variables, based on the gimage  */
  if (width > height)
    private->ratio = (double) DRAWING_AREA_SIZE / (double) width;
  else
    private->ratio = (double) DRAWING_AREA_SIZE / (double) height;
  private->area_width = (int) (private->ratio * width);
  private->area_height = (int) (private->ratio * height);

  /*  dialog box  */
  {
    const gchar *wmclass = NULL;
    const gchar *window_title = NULL;
    gchar *help_page = NULL;

    switch (type)
      {
      case ScaleWidget:
	switch (target)
	  {
	  case ResizeLayer:
	    wmclass = "scale_layer";
	    window_title = _("Scale Layer");
	    help_page = "layers/dialogs/scale_layer.html";
	    frame = gtk_frame_new (_("Size"));
	    break;
	  case ResizeImage:
	    wmclass = "scale_image";
	    window_title = _("Scale Image");
	    help_page = "dialogs/scale_image.html";
	    frame = gtk_frame_new (_("Pixel Dimensions"));
	    break;
	  }
	break;

      case ResizeWidget:
	switch (target)
	  {
	  case ResizeLayer:
	    wmclass = "resize_layer";
	    window_title = _("Set Layer Boundary Size");
	    help_page = "layers/dialogs/resize_layer.html";
	    break;
	  case ResizeImage:
	    wmclass = "resize_image";
	    window_title = _("Set Canvas Size");
	    help_page = "dialogs/resize_image.html";
	    break;
	  }
	frame = gtk_frame_new (_("Size"));
	break;
      }	

    resize->resize_shell =
      gimp_dialog_new (window_title, wmclass,
		       gimp_standard_help_func, help_page,
		       GTK_WIN_POS_MOUSE,
		       FALSE, FALSE, TRUE,

		       _("OK"), ok_cb,
		       user_data, NULL, NULL, TRUE, FALSE,
		       _("Cancel"), cancel_cb ? cancel_cb : gtk_widget_destroy,
		       cancel_cb ? user_data : NULL,
		       cancel_cb ? NULL : (gpointer) 1,
		       NULL, FALSE, TRUE,

		       NULL);

    gtk_signal_connect_object (GTK_OBJECT (resize->resize_shell), "destroy",
			       GTK_SIGNAL_FUNC (g_free),
			       (GtkObject *) private);
    gtk_signal_connect_object (GTK_OBJECT (resize->resize_shell), "destroy",
			       GTK_SIGNAL_FUNC (g_free),
			       (GtkObject *) resize);
  }

  /*  handle the image disappearing under our feet  */
  if (object && signal)
    {
      if (cancel_cb)
	gtk_signal_connect (GTK_OBJECT (object), signal,
			    cancel_cb,
			    user_data);
      else
	gtk_signal_connect_object_while_alive
	  (GTK_OBJECT (object), signal,
	   GTK_SIGNAL_FUNC (gtk_widget_destroy),
	   GTK_OBJECT (resize->resize_shell));
    }

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (resize->resize_shell)->vbox),
		     vbox);

  /*  the pixel dimensions frame  */
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);

  table = gtk_table_new (6, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 3, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 4, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

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
  gtk_misc_set_alignment (GTK_MISC ( private->orig_width_label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table),  private->orig_width_label, 1, 2, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show ( private->orig_width_label);

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
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
                                   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);

  private->size_se =
    gimp_size_entry_new (1, unit, "%a", TRUE, TRUE, FALSE, 75,
			 GIMP_SIZE_ENTRY_UPDATE_SIZE);
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

  gtk_signal_connect (GTK_OBJECT (private->size_se), "value_changed",
		      (GtkSignalFunc) size_callback,
		      resize);
  gtk_signal_connect (GTK_OBJECT (private->size_se), "unit_changed",
		      (GtkSignalFunc) orig_labels_update,
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
  alignment = gtk_alignment_new (0.0, 0.5, 0.0, 1.0);
  table2 = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table2), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table2), 0, 2);
  gtk_container_add (GTK_CONTAINER (alignment), table2);
  gtk_table_attach (GTK_TABLE (table), alignment, 1, 2, 4, 6,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (alignment);
  
  /*  the scale ratio spinbuttons  */
  private->ratio_x_adj =
    gtk_adjustment_new (resize->ratio_x, 
			(double) GIMP_MIN_IMAGE_SIZE / (double) resize->width,
			(double) GIMP_MAX_IMAGE_SIZE / (double) resize->width,
			0.01, 0.1, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (private->ratio_x_adj), 1, 4);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
                                   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  gtk_table_attach_defaults (GTK_TABLE (table2), spinbutton, 0, 1, 0, 1);
  gtk_signal_connect (GTK_OBJECT ( private->ratio_x_adj), "value_changed",
		      (GtkSignalFunc) ratio_callback,
		      resize);
  gtk_widget_show (spinbutton);

  private->ratio_y_adj =
    gtk_adjustment_new (resize->ratio_y,
			(double) GIMP_MIN_IMAGE_SIZE / (double) resize->height,
			(double) GIMP_MAX_IMAGE_SIZE / (double) resize->height,
			0.01, 0.1, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (private->ratio_y_adj), 1, 4);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
                                   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  gtk_table_attach_defaults (GTK_TABLE (table2), spinbutton, 0, 1, 1, 2);
  gtk_signal_connect (GTK_OBJECT ( private->ratio_y_adj), "value_changed",
		      (GtkSignalFunc) ratio_callback,
		      resize);
  gtk_widget_show (spinbutton);

  /*  the constrain ratio chainbutton  */
  private->constrain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (private->constrain), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table2), private->constrain, 1, 2, 0, 2);
  gtk_widget_show (private->constrain);

  gtk_widget_show (table2);
  gtk_widget_show (table);
  gtk_widget_show (vbox2);

  /*  the offset frame  */
  if (type == ResizeWidget)
    {
      frame = gtk_frame_new (_("Offset"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      hbox = gtk_hbox_new (FALSE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
      gtk_container_add (GTK_CONTAINER (frame), hbox);

      vbox2 = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
      gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, FALSE, 0);

      table = gtk_table_new (2, 2, FALSE);
      gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
      gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
 
      /*  the x and y offset labels  */
      label = gtk_label_new (_("X:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Y:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);

      /*  the offset sizeentry  */
      adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
      spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				       GTK_SHADOW_NONE);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_widget_set_usize (spinbutton, 75, 0);

      private->offset_se =
	gimp_size_entry_new (1, unit, "%a", TRUE, FALSE, FALSE, 75,
			     GIMP_SIZE_ENTRY_UPDATE_SIZE);
      gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->offset_se),
				 GTK_SPIN_BUTTON (spinbutton), NULL);
      gtk_table_attach_defaults (GTK_TABLE (private->offset_se), spinbutton,
				 1, 2, 0, 1);
      gtk_widget_show (spinbutton);
      gtk_table_attach (GTK_TABLE (table), private->offset_se, 1, 2, 0, 2,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
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

      gtk_signal_connect (GTK_OBJECT (private->offset_se), "value_changed",
			  (GtkSignalFunc) offset_update, resize);

      gtk_widget_show (table);

      /*  frame to hold drawing area  */
      hbox2 = gtk_hbox_new (0, FALSE);
      gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_box_pack_start (GTK_BOX (hbox2), frame, TRUE, FALSE, 0);
      private->drawing_area = gtk_drawing_area_new ();
      gtk_drawing_area_size (GTK_DRAWING_AREA (private->drawing_area),
			     private->area_width, private->area_height);
      gtk_widget_set_events (private->drawing_area, EVENT_MASK);
      gtk_signal_connect (GTK_OBJECT (private->drawing_area), "event",
			  (GtkSignalFunc) resize_events,
			  NULL);
      gtk_object_set_user_data (GTK_OBJECT (private->drawing_area), resize);
      gtk_container_add (GTK_CONTAINER (frame), private->drawing_area);
      gtk_widget_show (private->drawing_area);
      gtk_widget_show (frame);

      gtk_widget_show (hbox2);
      gtk_widget_show (vbox2);
      gtk_widget_show (hbox);
    }

  /*  the resolution stuff  */
  if ((type == ScaleWidget) && (target == ResizeImage))
    {
      frame = gtk_frame_new (_("Print Size & Display Unit"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox2 = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox2);

      table = gtk_table_new (4, 2, FALSE);
      gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
      gtk_table_set_row_spacing (GTK_TABLE (table), 1, 4);
      gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

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
      alignment = gtk_alignment_new (0.0, 0.5, 0.0, 1.0);
      adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
      spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				       GTK_SHADOW_NONE);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_widget_set_usize (spinbutton, 75, 0);
      gtk_container_add (GTK_CONTAINER (alignment), spinbutton);
      gtk_widget_show (spinbutton);
      gtk_table_attach (GTK_TABLE (table), alignment, 1, 2, 0, 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (alignment);

      alignment = gtk_alignment_new (0.0, 0.5, 0.0, 1.0);
      private->printsize_se =
	gimp_size_entry_new (1, unit, "%a", FALSE, FALSE, FALSE, 75,
			     GIMP_SIZE_ENTRY_UPDATE_SIZE);
      gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->printsize_se),
				 GTK_SPIN_BUTTON (spinbutton), NULL);
      gtk_container_add (GTK_CONTAINER (alignment), private->printsize_se);
      gtk_table_attach (GTK_TABLE (table), alignment, 1, 2, 1, 2,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (private->printsize_se);
      gtk_widget_show (alignment);

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

      gtk_signal_connect (GTK_OBJECT (private->printsize_se), "value_changed",
			  (GtkSignalFunc) printsize_update,
			  resize);
      gtk_signal_connect (GTK_OBJECT (private->printsize_se), "unit_changed",
			  (GtkSignalFunc) unit_update,
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
      adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
      spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				       GTK_SHADOW_NONE);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_widget_set_usize (spinbutton, 75, 0);

      private->resolution_se =
	gimp_size_entry_new (1, default_resolution_units, _("pixels/%a"),
			     FALSE, FALSE, FALSE, 75,
			     GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
      gtk_table_set_col_spacing (GTK_TABLE (private->resolution_se), 1, 2);
      gtk_table_set_col_spacing (GTK_TABLE (private->resolution_se), 2, 2);
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

      gtk_signal_connect (GTK_OBJECT (private->resolution_se), "value_changed",
			  (GtkSignalFunc) resolution_update,
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
      gtk_widget_show (vbox2);
    }

  gtk_widget_show (vbox);

  /*  finally, activate the first entry  */
  gimp_size_entry_grab_focus (GIMP_SIZE_ENTRY (private->size_se));

  return resize;
}

static void
resize_draw (Resize *resize)
{
  GtkWidget *widget;
  ResizePrivate *private;
  gint aw, ah;
  gint x, y;
  gint w, h;

  /*  Only need to draw if it's a resize widget  */
  if (resize->type != ResizeWidget)
    return;

  private = (ResizePrivate *) resize->private_part;
  widget = private->drawing_area;

  /*  If we're making the size larger  */
  if (private->old_width <= resize->width)
    w = resize->width;
  /*  otherwise, if we're making the size smaller  */
  else
    w = private->old_width * 2 - resize->width;
  /*  If we're making the size larger  */
  if (private->old_height <= resize->height)
    h = resize->height;
  /*  otherwise, if we're making the size smaller  */
  else
    h = private->old_height * 2 - resize->height;

  if (w > h)
    private->ratio = (double) DRAWING_AREA_SIZE / (double) w;
  else
    private->ratio = (double) DRAWING_AREA_SIZE / (double) h;

  aw = (int) (private->ratio * w);
  ah = (int) (private->ratio * h);

  if (aw != private->area_width || ah != private->area_height)
    {
      private->area_width = aw;
      private->area_height = ah;
      gtk_widget_set_usize (private->drawing_area, aw, ah);
    }

  if (private->old_width <= resize->width)
    x = private->ratio * resize->offset_x;
  else
    x = private->ratio * (resize->offset_x + private->old_width - resize->width);
  if (private->old_height <= resize->height)
    y = private->ratio * resize->offset_y;
  else
    y = private->ratio * (resize->offset_y + private->old_height - resize->height);

  w = private->ratio * private->old_width;
  h = private->ratio * private->old_height;

  gdk_window_clear (private->drawing_area->window);
  gtk_draw_shadow (widget->style, widget->window,
		   GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		   x, y, w, h);

  /*  If we're making the size smaller  */
  if (private->old_width > resize->width ||
      private->old_height > resize->height)
    {
      if (private->old_width > resize->width)
	{
	  x = private->ratio * (private->old_width - resize->width);
	  w = private->ratio * resize->width;
	}
      else
	{
	  x = -1;
	  w = aw + 2;
	}
      if (private->old_height > resize->height)
	{
	  y = private->ratio * (private->old_height - resize->height);
	  h = private->ratio * resize->height;
	}
      else
	{
	  y = -1;
	  h = ah + 2;
	}

      gdk_draw_rectangle (private->drawing_area->window,
			  widget->style->black_gc, 0,
			  x, y, w, h);
    }
}

static gint
resize_bound_off_x (Resize *resize,
		    gint    off_x)
{
  ResizePrivate *private;

  private = (ResizePrivate *) resize->private_part;

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

  private = (ResizePrivate *) resize->private_part;

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

  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;

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
  Resize *resize;
  ResizePrivate *private;
  gint offset_x;
  gint offset_y;

  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;

  offset_x = (gint)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->offset_se), 0) + 0.5);
  offset_x = resize_bound_off_x (resize, offset_x);

  offset_y = (gint)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->offset_se), 1) + 0.5);
  offset_y = resize_bound_off_y (resize, offset_y);

  if ((offset_x != resize->offset_x) ||
      (offset_y != resize->offset_y))
    {
      resize->offset_x = offset_x;
      resize->offset_y = offset_y;
      resize_draw (resize);
    }
}

static void
size_callback (GtkWidget *widget,
	       gpointer   data)
{
  Resize *resize;
  ResizePrivate *private;
  double width;
  double height;
  double ratio_x;
  double ratio_y;

  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;

  width = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->size_se), 0);
  height = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->size_se), 1);

  ratio_x = width / (double) private->old_width;
  ratio_y = height / (double) private->old_height;

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (private->constrain)))
    {
      if (ratio_x != resize->ratio_x)
	{
	  ratio_y = ratio_x;
	  height = (double) private->old_height * ratio_y;
	  height = CLAMP (height, GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);
	}
      else
	{
	  ratio_x = ratio_y;
	  width = (double) private->old_width * ratio_x;
	  width = CLAMP (width, GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);
	}
    }

  size_update (resize, width, height, ratio_x, ratio_y);
}

static void
ratio_callback (GtkWidget *widget,
		gpointer   data)
{
  Resize *resize;
  ResizePrivate *private;
  double width;
  double height;
  double ratio_x;
  double ratio_y;

  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;

  width = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->size_se), 0);
  height = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->size_se), 1);

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

  width = CLAMP (private->old_width * ratio_x,
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

  private = (ResizePrivate *) resize->private_part;

  resize->width = (gint) (width + 0.5);
  resize->height = (gint) (height + 0.5);

  resize->ratio_x = ratio_x;
  resize->ratio_y = ratio_y;

  gtk_signal_handler_block_by_data (GTK_OBJECT (private->size_se), resize);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->size_se),
			      0, width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->size_se),
			      1, height);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->size_se), resize);

  gtk_signal_handler_block_by_data (GTK_OBJECT (private->ratio_x_adj), resize);
  gtk_signal_handler_block_by_data (GTK_OBJECT (private->ratio_y_adj), resize);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (private->ratio_x_adj), ratio_x);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (private->ratio_y_adj), ratio_y);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->ratio_x_adj), resize);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->ratio_y_adj), resize);

  if (resize->type == ResizeWidget)
    {
      resize->offset_x = resize_bound_off_x (resize, resize->offset_x);
      resize->offset_y = resize_bound_off_y (resize, resize->offset_y);

      gimp_size_entry_set_refval_boundaries
	(GIMP_SIZE_ENTRY (private->offset_se), 0,
	 MIN (0, resize->width - private->old_width),
	 MAX (0, resize->width - private->old_width));
      gimp_size_entry_set_refval_boundaries
	(GIMP_SIZE_ENTRY (private->offset_se), 1,
	 MIN (0, resize->height - private->old_height),
	 MAX (0, resize->height - private->old_height));

      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se),
				  0, resize->offset_x);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se),
				  1, resize->offset_y);
    }

  if ((resize->type == ScaleWidget) && (resize->target == ResizeImage))
    {
      gtk_signal_handler_block_by_data (GTK_OBJECT (private->printsize_se),
					resize);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->printsize_se),
				  0, width);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->printsize_se),
				  1, height);
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->printsize_se),
					  resize);
    }

  resize_draw (resize);
}

static void
printsize_update (GtkWidget *widget,
		  gpointer   data)
{
  Resize *resize;
  ResizePrivate *private;
  double width;
  double height;
  double print_width;
  double print_height;
  double res_x;
  double res_y;

  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;

  width = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->size_se), 0);
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

  gtk_signal_handler_block_by_data (GTK_OBJECT (private->resolution_se), resize);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->resolution_se),
			      0, res_x);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->resolution_se),
			      1, res_y);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->resolution_se),
				      resize);

  gtk_signal_handler_block_by_data (GTK_OBJECT (private->size_se), resize);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se),
				  0, res_x, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se),
				  1, res_y, TRUE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->size_se),
			      0, width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->size_se),
			      1, height);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->size_se), resize);

  gtk_signal_handler_block_by_data (GTK_OBJECT (private->printsize_se), resize);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->printsize_se),
				  0, res_x, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->printsize_se),
				  1, res_y, TRUE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->printsize_se),
			      0, width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->printsize_se),
			      1, height);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->printsize_se),
				      resize);
}

static void
resolution_update (GtkWidget *widget,
		   gpointer   data)
{
  Resize *resize;
  ResizePrivate *private;
  double res_x;
  double res_y;
  
  resize = (Resize *) data;
  private = (ResizePrivate *) resize->private_part;

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

  resize->resolution_x = res_x;
  resize->resolution_y = res_y;

  gtk_signal_handler_block_by_data (GTK_OBJECT (private->resolution_se), resize);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->resolution_se),
			      0, res_x);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->resolution_se),
			      1, res_y);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->resolution_se),
				      resize);

  gtk_signal_handler_block_by_data (GTK_OBJECT (private->size_se), resize);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se),
				  0, res_x, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se),
				  1, res_y, TRUE);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->size_se), resize);

  gtk_signal_handler_block_by_data (GTK_OBJECT (private->printsize_se), resize);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->printsize_se),
				  0, res_x, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->printsize_se),
				  1, res_y, TRUE);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (private->printsize_se),
				      resize);
}

static gint
resize_events (GtkWidget *widget,
	       GdkEvent  *event)
{
  Resize *resize;
  ResizePrivate *private;
  int dx, dy;
  int off_x, off_y;

  resize = (Resize *) gtk_object_get_user_data (GTK_OBJECT (widget));
  private = (ResizePrivate *) resize->private_part;

  switch (event->type)
    {
    case GDK_EXPOSE:
      resize_draw (resize);
      break;
    case GDK_BUTTON_PRESS:
      gdk_pointer_grab (private->drawing_area->window, FALSE,
			(GDK_BUTTON1_MOTION_MASK |
			 GDK_BUTTON_RELEASE_MASK),
			NULL, NULL, event->button.time);
      private->orig_x = resize->offset_x;
      private->orig_y = resize->offset_y;
      private->start_x = event->button.x;
      private->start_y = event->button.y;
      break;
    case GDK_MOTION_NOTIFY:
      /*  X offset  */
      dx = event->motion.x - private->start_x;
      off_x = private->orig_x + dx / private->ratio;
      off_x = resize_bound_off_x (resize, off_x);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se),
				  0, off_x);

      /*  Y offset  */
      dy = event->motion.y - private->start_y;
      off_y = private->orig_y + dy / private->ratio;
      off_y = resize_bound_off_y (resize, off_y);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se),
				  1, off_y);

      gtk_signal_emit_by_name (GTK_OBJECT (private->offset_se), "value_changed",
			       resize);
      break;
    case GDK_BUTTON_RELEASE:
      gdk_pointer_ungrab (event->button.time);
      break;
    default:
      break;
    }

  return FALSE;
}

/*** Resize sanity checks ***/

void
resize_scale_implement (ImageResize *image_scale)
{
  GImage      *gimage        = NULL;
  gboolean     rulers_flush  = FALSE;
  gboolean     display_flush = FALSE;  /* this is a bit ugly: 
				          we hijack the flush variable 
					  to check if an undo_group was 
					  already started */
  g_assert(image_scale != NULL);
  gimage = image_scale->gimage;
  g_assert(gimage != NULL);

  if (image_scale->resize->resolution_x != gimage->xresolution ||
      image_scale->resize->resolution_y != gimage->yresolution)
    {
      undo_push_group_start (gimage, IMAGE_SCALE_UNDO);
	  
      gimage_set_resolution (gimage,
			     image_scale->resize->resolution_x,
			     image_scale->resize->resolution_y);

      rulers_flush = TRUE;
      display_flush = TRUE;
    }

  if (image_scale->resize->unit != gimage->unit)
    {
      if (!display_flush)
	undo_push_group_start (gimage, IMAGE_SCALE_UNDO);

      gimage_set_unit (gimage, image_scale->resize->unit);
      gdisplays_setup_scale (gimage);
      gdisplays_resize_cursor_label (gimage);

      rulers_flush = TRUE;
      display_flush = TRUE;
    }

  if (image_scale->resize->width != gimage->width ||
      image_scale->resize->height != gimage->height)
    {
      if (image_scale->resize->width > 0 &&
	  image_scale->resize->height > 0) 
	{
	  if (!display_flush)
	    undo_push_group_start (gimage, IMAGE_SCALE_UNDO);

	  gimage_scale (gimage,
			image_scale->resize->width,
			image_scale->resize->height);

	  display_flush = TRUE;
	}
      else
	{
	  g_message (_("Scale Error: Both width and height must be "
		       "greater than zero."));
	  return;
	}
    }

  if (rulers_flush)
    {
      gdisplays_setup_scale (gimage);
      gdisplays_resize_cursor_label (gimage);
    }
      
  if (display_flush)
    {
      undo_push_group_end (gimage);
      gdisplays_flush ();
    }
}

static void
resize_scale_warn_callback (GtkWidget *widget,
			    gboolean   do_scale,
			    gpointer   client_data)
{
  ImageResize *image_scale = NULL;
  GImage      *gimage      = NULL;

  g_assert (client_data != NULL);
  image_scale = (ImageResize *) client_data;
  gimage      = image_scale->gimage;
  g_assert (gimage != NULL);

  if (do_scale == TRUE) /* User doesn't mind losing layers... */
    {
      resize_scale_implement (image_scale);
      gtk_widget_destroy (image_scale->resize->resize_shell);
    }
  else
    {
      gtk_widget_set_sensitive (image_scale->resize->resize_shell, TRUE);
    }
}

gboolean 
resize_check_layer_scaling (ImageResize *image_scale)
{
  /* Inventory the layer list in gimage and return TRUE if, after
   * scaling, they all retain positive x and y pixel dimensions.
   * Otherwise, put up a modal boolean dialog box and ask the user if
   * she wishes to proceed. Return FALSE in the dialog case; the dialog box
   * callback will complete the job if the user really wants to
   * proceed. <02/22/2000 gosgood@idt.net>
   */

  gboolean   success = FALSE;
  GImage    *gimage  = NULL;
  GSList    *list    = NULL;
  Layer     *layer   = NULL;
  GtkWidget *dialog  = NULL;

  g_assert (image_scale != NULL);

  if (NULL != (gimage = image_scale->gimage))
    {
      /* Step through layers; test scaled dimensions. */

      success = TRUE;
      list    = gimage->layers;
      while (list && success == TRUE)
	{
	  layer   = (Layer *)list->data;
	  success = layer_check_scaling (layer, 
					 image_scale->resize->width,
					 image_scale->resize->height);
	  list   = g_slist_next (list);
	  
	}
      /* Warn user on failure */
      if (success == FALSE)
	{
	  dialog =
	    gimp_query_boolean_box (_("Layer Too Small"),
				    gimp_standard_help_func,
				    "dialogs/scale_layer_warn.html",
				    FALSE,
				    _("The chosen image size will shrink\n"
				      "some layers completely away.\n"
				      "Is this what you want?"),
				    _("OK"), _("Cancel"),
				    GTK_OBJECT (image_scale->resize->resize_shell),
				    "destroy",
				    resize_scale_warn_callback,
				    image_scale);
	  gtk_widget_show (dialog);
	}
    }
  return success;
}

