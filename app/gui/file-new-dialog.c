/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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
#include "file_new_dialog.h"
#include "gimprc.h"
#include "gimpui.h"
#include "gdisplay.h"

#include "libgimp/gimpchainbutton.h"
#include "libgimp/gimplimits.h"
#include "libgimp/gimpsizeentry.h"
#include "libgimp/gimpintl.h"

typedef struct
{
  GtkWidget *dlg;

  GtkWidget *confirm_dlg;

  GtkWidget *size_frame;
  GtkWidget *size_se;
  GtkWidget *resolution_se;
  GtkWidget *couple_resolutions;

  /* this should be a list */
  GtkWidget *type_w[2];
  GtkWidget *fill_type_w[4];

  GimpImageNewValues *values;
  gdouble size;
} NewImageInfo;

/*  new image local functions  */
static void file_new_confirm_dialog (NewImageInfo *);

static void file_new_ok_callback (GtkWidget *, gpointer);
static void file_new_reset_callback (GtkWidget *, gpointer);
static void file_new_cancel_callback (GtkWidget *, gpointer);
static void file_new_toggle_callback (GtkWidget *, gpointer);
static void file_new_resolution_callback (GtkWidget *, gpointer);
static void file_new_image_size_callback (GtkWidget *, gpointer);

static void
file_new_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  NewImageInfo *info;
  GimpImageNewValues *values;

  info = (NewImageInfo*) data;
  values = info->values;

  /* get the image size in pixels */
  values->width = (int)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (info->size_se), 0) + 0.5);
  values->height = (int)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (info->size_se), 1) + 0.5);

  /* get the resolution in dpi */
  values->xresolution =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (info->resolution_se), 0);
  values->yresolution =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (info->resolution_se), 1);

  /* get the units */
  values->unit =
    gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (info->size_se));
  values->res_unit =
    gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (info->resolution_se));

  if (info->size > max_new_image_size)
    {
      file_new_confirm_dialog (info);
    }
  else
    {
      gtk_widget_destroy (info->dlg);
      image_new_create_image (values);
      image_new_values_free (values);
      g_free (info);
    }
}

static void
file_new_reset_callback (GtkWidget *widget,
			 gpointer   data)
{
  NewImageInfo *info;

  info = (NewImageInfo*) data;

  gtk_signal_handler_block_by_data (GTK_OBJECT (info->resolution_se), info);

  gimp_chain_button_set_active
    (GIMP_CHAIN_BUTTON (info->couple_resolutions),
     ABS (default_xresolution - default_yresolution) < GIMP_MIN_RESOLUTION);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->resolution_se),
			      0, default_xresolution);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->resolution_se),
			      1, default_yresolution);
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (info->resolution_se),
			    default_resolution_units);

  gtk_signal_handler_unblock_by_data (GTK_OBJECT (info->resolution_se), info);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se),
				  0, default_xresolution, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se),
				  1, default_yresolution, TRUE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->size_se),
			      0, default_width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->size_se),
			      1, default_height);
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (info->size_se),
			    default_units);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (info->type_w[default_type]),
				TRUE);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (info->fill_type_w[BACKGROUND_FILL]), TRUE);
}

static void
file_new_cancel_callback (GtkWidget *widget,
			  gpointer   data)
{
  NewImageInfo *info;

  info = (NewImageInfo*) data;

  gtk_widget_destroy (info->dlg);
  image_new_values_free(info->values);
  g_free (info);
}

/*  local callbacks of file_new_confirm_dialog()  */
static void
file_new_confirm_dialog_ok_callback (GtkWidget *widget,
                                     gpointer  data)
{
  NewImageInfo *info;

  info = (NewImageInfo*) data;

  gtk_widget_destroy (info->confirm_dlg);
  gtk_widget_destroy (info->dlg);
  image_new_create_image (info->values);
  image_new_values_free (info->values);
  g_free (info);
}

static void
file_new_confirm_dialog_cancel_callback (GtkWidget *widget,
					 gpointer  data)
{
  NewImageInfo *info;

  info = (NewImageInfo*) data;

  gtk_widget_destroy (info->confirm_dlg);
  info->confirm_dlg = NULL;
  gtk_widget_set_sensitive (info->dlg, TRUE);
}

static void
file_new_confirm_dialog (NewImageInfo *info)
{
  GtkWidget *label;
  gchar *size;
  gchar *max_size;
  gchar *text;

  gtk_widget_set_sensitive (info->dlg, FALSE);

  info->confirm_dlg =
    gimp_dialog_new (_("Confirm Image Size"), "confirm_size",
		     gimp_standard_help_func,
		     "dialogs/file_new.html#confirm_size",
		     GTK_WIN_POS_MOUSE,
		     FALSE, FALSE, FALSE,

		     _("OK"), file_new_confirm_dialog_ok_callback,
		     info, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), file_new_confirm_dialog_cancel_callback,
		     info, NULL, NULL, FALSE, TRUE,

		     NULL);

  size = image_new_get_size_string (info->size);
  max_size = image_new_get_size_string (max_new_image_size);

  /* xgettext:no-c-format */
	    
  text = g_strdup_printf (_("You are trying to create an image which\n"
			    "has an initial size of %s.\n\n"
			    "Choose OK to create this image anyway.\n"
			    "Choose Cancel if you didn't mean to\n"
			    "create such a large image.\n\n"
			    "To prevent this dialog from appearing,\n"
			    "increase the \"Maximum Image Size\"\n"
			    "setting (currently %s) in the\n"
			    "preferences dialog."),
                          size, max_size);
  label = gtk_label_new (text);
  gtk_misc_set_padding (GTK_MISC (label), 6, 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info->confirm_dlg)->vbox), label,
		      TRUE, TRUE, 0);
  gtk_widget_show (label);

  g_free (text);
  g_free (max_size);
  g_free (size);

  gtk_widget_show (info->confirm_dlg);
}

static void
file_new_toggle_callback (GtkWidget *widget,
			  gpointer   data)
{
  gint *val;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      val = data;
      *val = (long) gtk_object_get_user_data (GTK_OBJECT (widget));
    }
}

static void
file_new_resolution_callback (GtkWidget *widget,
			      gpointer   data)
{
  NewImageInfo *info;

  static gdouble xres = 0.0;
  static gdouble yres = 0.0;
  gdouble new_xres;
  gdouble new_yres;

  info = (NewImageInfo*) data;

  new_xres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_yres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if (gimp_chain_button_get_active
      (GIMP_CHAIN_BUTTON (info->couple_resolutions)))
    {
      gtk_signal_handler_block_by_data
	(GTK_OBJECT (info->resolution_se), info);

      if (new_xres != xres)
	{
	  yres = new_yres = xres = new_xres;
	  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 1, yres);
	}

      if (new_yres != yres)
	{
	  xres = new_xres = yres = new_yres;
	  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 0, xres);
	}

      gtk_signal_handler_unblock_by_data
	(GTK_OBJECT (info->resolution_se), info);
    }
  else
    {
      if (new_xres != xres)
	xres = new_xres;
      if (new_yres != yres)
	yres = new_yres;
    }

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se), 0,
				  xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se), 1,
				  yres, FALSE);

  file_new_image_size_callback (widget, data);
}

static void
file_new_image_size_callback (GtkWidget *widget,
			      gpointer   data)
{
  NewImageInfo *info;
  gchar *text;
  gchar *label;

  info = (NewImageInfo*) data;

  info->values->width = (gdouble) (gint)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (info->size_se), 0) + 0.5);
  info->values->height = (gdouble) (gint)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (info->size_se), 1) + 0.5);

  info->size = image_new_calculate_size (info->values);

  label = g_strdup_printf (_("Image Size: %s"),
			   text = image_new_get_size_string (info->size));
  gtk_frame_set_label (GTK_FRAME (info->size_frame), label);

  g_free (label);
  g_free (text);
}

void
file_new_cmd_callback (GtkWidget *widget,
		       gpointer   callback_data,
		       guint      callback_action)
{
  GDisplay *gdisp;
  GimpImage *image = NULL;

  /*  Before we try to determine the responsible gdisplay,
   *  make sure this wasn't called from the toolbox
   */
  if (callback_action)
    {
      gdisp = gdisplay_active ();

      if (gdisp)
        image = gdisp->gimage;
    }

  image_new_create_window (NULL, image);
}

void
ui_new_image_window_create (const GimpImageNewValues *values_orig)
{
  NewImageInfo       *info;
  GimpImageNewValues *values;

  GtkWidget *top_vbox;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *separator;
  GtkWidget *label;
  GtkWidget *button;
  GtkObject *adjustment;
  GtkWidget *spinbutton;
  GtkWidget *spinbutton2;
  GtkWidget *radio_box;
  GSList *group;
  GList *list;

  info = g_new (NewImageInfo, 1);
  info->values = values = image_new_values_new (values_orig);

  info->confirm_dlg = NULL;
  info->size = 0.0;

  info->dlg = gimp_dialog_new (_("New Image"), "new_image",
			       gimp_standard_help_func,
			       "dialogs/file_new.html",
			       GTK_WIN_POS_MOUSE,
			       FALSE, FALSE, TRUE,

			       _("OK"), file_new_ok_callback,
			       info, NULL, NULL, TRUE, FALSE,
			       _("Reset"), file_new_reset_callback,
			       info, NULL, NULL, FALSE, FALSE,
			       _("Cancel"), file_new_cancel_callback,
			       info, NULL, NULL, FALSE, TRUE,

			       NULL);

  /*  vbox holding the rest of the dialog  */
  top_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (top_vbox), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info->dlg)->vbox),
		      top_vbox, TRUE, TRUE, 0);
  gtk_widget_show (top_vbox);

  /*  Image size frame  */
  info->size_frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (top_vbox), info->size_frame, FALSE, FALSE, 0);
  gtk_widget_show (info->size_frame);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (info->size_frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (7, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 4, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  the pixel size labels  */
  label = gtk_label_new (_("Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  a separator after the pixel section  */
  separator = gtk_hseparator_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), separator, 0, 2, 2, 3);
  gtk_widget_show (separator);

  /*  the unit size labels  */
  label = gtk_label_new (_("Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  create the sizeentry which keeps it all together  */
  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 2, 3, 5);
  info->size_se =
    gimp_size_entry_new (0, values->unit, "%a", FALSE, FALSE, TRUE, 75,
			 GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_col_spacing (GTK_TABLE (info->size_se), 1, 2);
  gtk_container_add (GTK_CONTAINER (abox), info->size_se);
  gtk_widget_show (info->size_se);
  gtk_widget_show (abox);

  /*  height in units  */
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
                                   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  /*  add the "height in units" spinbutton to the sizeentry  */
  gtk_table_attach_defaults (GTK_TABLE (info->size_se), spinbutton,
			     0, 1, 2, 3);
  gtk_widget_show (spinbutton);

  /*  height in pixels  */
  hbox = gtk_hbox_new (FALSE, 2);
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton2 = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton2),
                                   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton2), TRUE);
  gtk_widget_set_usize (spinbutton2, 75, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton2, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton2);

  label = gtk_label_new (_("Pixels"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  add the "height in pixels" spinbutton to the main table  */
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 1, 2);
  gtk_widget_show (hbox);

  /*  register the height spinbuttons with the sizeentry  */
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (info->size_se),
                             GTK_SPIN_BUTTON (spinbutton),
			     GTK_SPIN_BUTTON (spinbutton2));

  /*  width in units  */
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
                                   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  /*  add the "width in units" spinbutton to the sizeentry  */
  gtk_table_attach_defaults (GTK_TABLE (info->size_se), spinbutton,
			     0, 1, 1, 2);
  gtk_widget_show (spinbutton);

  /*  width in pixels  */
  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 2, 0, 1);
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton2 = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton2),
                                   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton2), TRUE);
  gtk_widget_set_usize (spinbutton2, 75, 0);
  /*  add the "width in pixels" spinbutton to the main table  */
  gtk_container_add (GTK_CONTAINER (abox), spinbutton2);
  gtk_widget_show (spinbutton2);
  gtk_widget_show (abox);

  /*  register the width spinbuttons with the sizeentry  */
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (info->size_se),
                             GTK_SPIN_BUTTON (spinbutton),
			     GTK_SPIN_BUTTON (spinbutton2));

  /*  initialize the sizeentry  */
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se), 0,
				  values->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se), 1,
				  values->yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (info->size_se), 0,
					 GIMP_MIN_IMAGE_SIZE,
					 GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (info->size_se), 1,
					 GIMP_MIN_IMAGE_SIZE,
					 GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->size_se), 0, values->width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->size_se), 1, values->height);

  gtk_signal_connect (GTK_OBJECT (info->size_se), "refval_changed",
		      (GtkSignalFunc) file_new_image_size_callback, info);
  gtk_signal_connect (GTK_OBJECT (info->size_se), "value_changed",
		      (GtkSignalFunc) file_new_image_size_callback, info);

  /*  initialize the size label  */
  file_new_image_size_callback (info->size_se, info);

  /*  the resolution labels  */
  label = gtk_label_new (_("Resolution X:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Y:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 6, 7,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  the resolution sizeentry  */
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);

  info->resolution_se =
    gimp_size_entry_new (1, default_resolution_units, _("pixels/%a"),
		         FALSE, FALSE, FALSE, 75,
		         GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
  gtk_table_set_col_spacing (GTK_TABLE (info->resolution_se), 1, 2);
  gtk_table_set_col_spacing (GTK_TABLE (info->resolution_se), 2, 2);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (info->resolution_se),
			     GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (info->resolution_se), spinbutton,
			     1, 2, 0, 1);
  gtk_widget_show (spinbutton);
  gtk_table_attach (GTK_TABLE (table), info->resolution_se, 1, 2, 5, 7,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (info->resolution_se);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (info->resolution_se),
					 0, GIMP_MIN_RESOLUTION,
					 GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (info->resolution_se),
					 1, GIMP_MIN_RESOLUTION,
					 GIMP_MAX_RESOLUTION);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->resolution_se),
			      0, values->xresolution);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->resolution_se),
			      1, values->yresolution);

  gtk_signal_connect (GTK_OBJECT (info->resolution_se), "value_changed",
		      (GtkSignalFunc) file_new_resolution_callback, info);

  /*  the resolution chainbutton  */
  info->couple_resolutions = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active
    (GIMP_CHAIN_BUTTON (info->couple_resolutions),
     ABS (values->xresolution - values->yresolution) < GIMP_MIN_RESOLUTION);
  gtk_table_attach_defaults (GTK_TABLE (info->resolution_se),
			     info->couple_resolutions, 2, 3, 0, 2);
  gtk_widget_show (info->couple_resolutions);

  /*  hbox containing the Image type and fill type frames  */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (top_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  frame for Image Type  */
  frame = gtk_frame_new (_("Image Type"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /*  radio buttons and box  */
  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  group = NULL;
  list = g_list_first (image_new_get_image_base_type_names ());
  while (list)
    {
      GimpImageBaseTypeName *name_info;

      name_info = (GimpImageBaseTypeName*) list->data;

      button = gtk_radio_button_new_with_label (group, name_info->name);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, FALSE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) name_info->type);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_new_toggle_callback,
			  &values->type);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_new_image_size_callback, info);
      if (values->type == name_info->type)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_widget_show (button);

      info->type_w[name_info->type] = button;

      list = g_list_next (list);
    }

  /* frame for Fill Type */
  frame = gtk_frame_new (_("Fill Type"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  group = NULL;
  list = g_list_first (image_new_get_fill_type_names ());
  while (list)
    {
      GimpFillTypeName *name_info;

      name_info = (GimpFillTypeName*) list->data;

      button =
	gtk_radio_button_new_with_label (group, name_info->name);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) name_info->type);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_new_toggle_callback,
			  &values->fill_type);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_new_image_size_callback, info);
      if (values->fill_type == name_info->type)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_widget_show (button);

      info->fill_type_w[name_info->type] = button;

      list = g_list_next (list);
    }

  gimp_size_entry_grab_focus (GIMP_SIZE_ENTRY (info->size_se));

  gtk_widget_show (info->dlg);
}
