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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcoreconfig.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"

#include "file-new-dialog.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"


typedef struct
{
  GtkWidget          *dialog;

  GtkWidget          *confirm_dialog;

  GtkWidget          *size_se;
  GtkWidget          *memsize_label;
  GtkWidget          *resolution_se;
  GtkWidget          *couple_resolutions;

  /* this should be a list */
  GtkWidget          *type_w[2];
  GtkWidget          *fill_type_w[4];

  GimpImageNewValues *values;
  gdouble             size;

  Gimp               *gimp;
} NewImageInfo;


/*  local function prototypes  */

static void   file_new_confirm_dialog      (NewImageInfo *info);

static void   file_new_ok_callback         (GtkWidget    *widget,
					    gpointer      data);
static void   file_new_reset_callback      (GtkWidget    *widget,
					    gpointer      data);
static void   file_new_cancel_callback     (GtkWidget    *widget,
					    gpointer      data);
static void   file_new_resolution_callback (GtkWidget    *widget,
					    gpointer      data);
static void   file_new_image_size_callback (GtkWidget    *widget,
					    gpointer      data);


/*  public functions  */

void
file_new_dialog_create (Gimp      *gimp,
                        GimpImage *gimage)
{
  NewImageInfo *info;
  GtkWidget    *top_vbox;
  GtkWidget    *hbox;
  GtkWidget    *vbox;
  GtkWidget    *abox;
  GtkWidget    *frame;
  GtkWidget    *table;
  GtkWidget    *separator;
  GtkWidget    *label;
  GtkWidget    *button;
  GtkObject    *adjustment;
  GtkWidget    *spinbutton;
  GtkWidget    *spinbutton2;
  GtkWidget    *radio_box;
  GSList       *group;
  GList        *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));

  info = g_new0 (NewImageInfo, 1);

  info->gimp           = gimp;

  info->values         = gimp_image_new_values_new (gimp, gimage);

  info->confirm_dialog = NULL;
  info->size           = 0.0;

  info->dialog = gimp_dialog_new (_("New Image"), "new_image",
				  gimp_standard_help_func,
				  "dialogs/file_new.html",
				  GTK_WIN_POS_MOUSE,
				  FALSE, FALSE, TRUE,

				  GIMP_STOCK_RESET, file_new_reset_callback,
				  info, NULL, NULL, FALSE, FALSE,

				  GTK_STOCK_CANCEL, file_new_cancel_callback,
				  info, NULL, NULL, FALSE, TRUE,

				  GTK_STOCK_OK, file_new_ok_callback,
				  info, NULL, NULL, TRUE, FALSE,

				  NULL);

  /*  vbox holding the rest of the dialog  */
  top_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (top_vbox), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info->dialog)->vbox),
		      top_vbox, TRUE, TRUE, 0);
  gtk_widget_show (top_vbox);

  /*  Image size frame  */
  frame = gtk_frame_new (_("Image Size"));
  gtk_box_pack_start (GTK_BOX (top_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
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
    gimp_size_entry_new (0, info->values->unit, "%a", FALSE, FALSE, TRUE, 75,
			 GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_col_spacing (GTK_TABLE (info->size_se), 1, 2);
  gtk_container_add (GTK_CONTAINER (abox), info->size_se);
  gtk_widget_show (info->size_se);
  gtk_widget_show (abox);

  /*  height in units  */
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_size_request (spinbutton, 75, -1);
  /*  add the "height in units" spinbutton to the sizeentry  */
  gtk_table_attach_defaults (GTK_TABLE (info->size_se), spinbutton,
			     0, 1, 2, 3);
  gtk_widget_show (spinbutton);

  /*  height in pixels  */
  hbox = gtk_hbox_new (FALSE, 2);
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton2 = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton2), TRUE);
  gtk_widget_set_size_request (spinbutton2, 75, -1);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton2, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton2);

  label = gtk_label_new (_("Pixels"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  info->memsize_label = gtk_label_new (NULL);
  gtk_box_pack_end (GTK_BOX (hbox), info->memsize_label, FALSE, FALSE, 0);
  gtk_widget_show (info->memsize_label);

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
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_size_request (spinbutton, 75, -1);
  /*  add the "width in units" spinbutton to the sizeentry  */
  gtk_table_attach_defaults (GTK_TABLE (info->size_se), spinbutton,
			     0, 1, 1, 2);
  gtk_widget_show (spinbutton);

  /*  width in pixels  */
  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 2, 0, 1);
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton2 = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton2), TRUE);
  gtk_widget_set_size_request (spinbutton2, 75, -1);
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
				  info->values->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se), 1,
				  info->values->yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (info->size_se), 0,
					 GIMP_MIN_IMAGE_SIZE,
					 GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (info->size_se), 1,
					 GIMP_MIN_IMAGE_SIZE,
					 GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->size_se), 0,
			      info->values->width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->size_se), 1,
			      info->values->height);

  g_signal_connect (G_OBJECT (info->size_se), "refval_changed",
		    G_CALLBACK (file_new_image_size_callback),
		    info);
  g_signal_connect (G_OBJECT (info->size_se), "value_changed",
		    G_CALLBACK (file_new_image_size_callback),
		    info);

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
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_size_request (spinbutton, 75, -1);

  info->resolution_se =
    gimp_size_entry_new (1, gimp->config->default_resolution_units,
			 _("pixels/%a"),
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

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->resolution_se), 0,
			      info->values->xresolution);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->resolution_se), 1,
			      info->values->yresolution);

  g_signal_connect (G_OBJECT (info->resolution_se), "value_changed",
		    G_CALLBACK (file_new_resolution_callback),
		    info);

  /*  the resolution chainbutton  */
  info->couple_resolutions = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active
    (GIMP_CHAIN_BUTTON (info->couple_resolutions),
     ABS (info->values->xresolution - info->values->yresolution)
     < GIMP_MIN_RESOLUTION);
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

  for (list = gimp_image_new_get_base_type_names (gimp);
       list;
       list = g_list_next (list))
    {
      GimpImageBaseTypeName *name_info;

      name_info = (GimpImageBaseTypeName*) list->data;

      button = gtk_radio_button_new_with_label (group, name_info->name);
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, FALSE, TRUE, 0);
      g_object_set_data (G_OBJECT (button), "gimp-item-data",
			 (gpointer) name_info->type);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "toggled",
			G_CALLBACK (gimp_radio_button_update),
			&info->values->type);
      g_signal_connect (G_OBJECT (button), "toggled",
			G_CALLBACK (file_new_image_size_callback),
			info);

      if (info->values->type == name_info->type)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      info->type_w[name_info->type] = button;
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

  for (list = gimp_image_new_get_fill_type_names (gimp);
       list;
       list = g_list_next (list))
    {
      GimpFillTypeName *name_info;

      name_info = (GimpFillTypeName*) list->data;

      button =
	gtk_radio_button_new_with_label (group, name_info->name);
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      g_object_set_data (G_OBJECT (button), "gimp-item-data",
			 (gpointer) name_info->type);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "toggled",
			G_CALLBACK (gimp_radio_button_update),
			&info->values->fill_type);
      g_signal_connect (G_OBJECT (button), "toggled",
			G_CALLBACK (file_new_image_size_callback),
			info);

      if (info->values->fill_type == name_info->type)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      info->fill_type_w[name_info->type] = button;
    }

  gimp_size_entry_grab_focus (GIMP_SIZE_ENTRY (info->size_se));

  gtk_widget_show (info->dialog);
}


/*  private functions  */

static void
file_new_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  NewImageInfo       *info;
  GimpImageNewValues *values;

  info = (NewImageInfo*) data;
  values = info->values;

  /* get the image size in pixels */
  values->width = 
    RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (info->size_se), 0));
  values->height = 
    RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (info->size_se), 1));

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

  if (info->size > gimprc.max_new_image_size)
    {
      file_new_confirm_dialog (info);
    }
  else
    {
      gtk_widget_destroy (info->dialog);
      gimp_image_new_create_image (info->gimp, values);
      gimp_image_new_values_free (values);
      g_free (info);
    }
}

static void
file_new_reset_callback (GtkWidget *widget,
			 gpointer   data)
{
  NewImageInfo *info;

  info = (NewImageInfo *) data;

  g_signal_handlers_block_by_func (G_OBJECT (info->resolution_se),
                                   file_new_resolution_callback,
                                   info);

  gimp_chain_button_set_active
    (GIMP_CHAIN_BUTTON (info->couple_resolutions),
     ABS (info->gimp->config->default_xresolution -
	  info->gimp->config->default_yresolution) < GIMP_MIN_RESOLUTION);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->resolution_se), 0,
			      info->gimp->config->default_xresolution);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->resolution_se), 1,
			      info->gimp->config->default_yresolution);
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (info->resolution_se),
			    info->gimp->config->default_resolution_units);

  g_signal_handlers_unblock_by_func (G_OBJECT (info->resolution_se),
                                     file_new_resolution_callback,
                                     info);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se), 0,
				  info->gimp->config->default_xresolution, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se), 1,
				  info->gimp->config->default_yresolution, TRUE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->size_se), 0,
			      info->gimp->config->default_width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->size_se), 1,
			      info->gimp->config->default_height);
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (info->size_se),
			    info->gimp->config->default_units);

  gimp_radio_group_set_active
    (GTK_RADIO_BUTTON (info->type_w[0]),
     GINT_TO_POINTER (info->gimp->config->default_type));

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (info->fill_type_w[0]),
                               GINT_TO_POINTER (BACKGROUND_FILL));
}

static void
file_new_cancel_callback (GtkWidget *widget,
			  gpointer   data)
{
  NewImageInfo *info;

  info = (NewImageInfo*) data;

  gtk_widget_destroy (info->dialog);
  gimp_image_new_values_free (info->values);
  g_free (info);
}

/*  local callback of file_new_confirm_dialog()  */
static void
file_new_confirm_dialog_callback (GtkWidget *widget,
				  gboolean   create,
				  gpointer   data)
{
  NewImageInfo *info;

  info = (NewImageInfo*) data;

  info->confirm_dialog = NULL;

  if (create)
    {
      gtk_widget_destroy (info->dialog);
      gimp_image_new_create_image (info->gimp, info->values);
      gimp_image_new_values_free (info->values);
      g_free (info);
    }
  else
    {
      gtk_widget_set_sensitive (info->dialog, TRUE);
    }
}

static void
file_new_confirm_dialog (NewImageInfo *info)
{
  gchar *size;
  gchar *max_size;
  gchar *text;

  gtk_widget_set_sensitive (info->dialog, FALSE);

  size     = gimp_image_new_get_size_string (info->size);
  max_size = gimp_image_new_get_size_string (gimprc.max_new_image_size);

  /* xgettext:no-c-format */
	    
  text = g_strdup_printf (_("You are trying to create an image with\n"
			    "an initial size of %s.\n\n"
			    "Choose OK to create this image anyway.\n"
			    "Choose Cancel if you did not intend to\n"
			    "create such a large image.\n\n"
			    "To prevent this dialog from appearing,\n"
			    "increase the \"Maximum Image Size\"\n"
			    "setting (currently %s) in the\n"
			    "Preferences dialog."),
                          size, max_size);

  info->confirm_dialog =
    gimp_query_boolean_box (_("Confirm Image Size"),
			    gimp_standard_help_func,
			    "dialogs/file_new.html#confirm_size",
			    GTK_STOCK_DIALOG_INFO,
			    text,
			    GTK_STOCK_OK, GTK_STOCK_CANCEL,
			    NULL, NULL,
			    file_new_confirm_dialog_callback,
			    info);

  g_free (text);
  g_free (max_size);
  g_free (size);

  gtk_widget_show (info->confirm_dialog);
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

  info = (NewImageInfo *) data;

  new_xres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_yres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if (gimp_chain_button_get_active
      (GIMP_CHAIN_BUTTON (info->couple_resolutions)))
    {
      g_signal_handlers_block_by_func (G_OBJECT (info->resolution_se),
                                       file_new_resolution_callback,
                                       info);

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

      g_signal_handlers_unblock_by_func (G_OBJECT (info->resolution_se),
                                         file_new_resolution_callback,
                                         info);
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
  gchar        *text;

  info = (NewImageInfo*) data;

  info->values->width = 
    RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (info->size_se), 0));
  info->values->height =
    RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (info->size_se), 1));

  info->size = gimp_image_new_calculate_size (info->values);

  text = gimp_image_new_get_size_string (info->size);
  gtk_label_set_text (GTK_LABEL (info->memsize_label), text);
  g_free (text);
}
