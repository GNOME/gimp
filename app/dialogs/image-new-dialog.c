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

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"

#include "widgets/gimpenummenu.h"
#include "widgets/gimpviewabledialog.h"

#include "file-new-dialog.h"

#include "gimp-intl.h"


#define SB_WIDTH 10


typedef struct
{
  GtkWidget          *dialog;

  GtkWidget          *confirm_dialog;

  GtkWidget          *size_se;
  GtkWidget          *memsize_label;
  GtkWidget          *resolution_se;
  GtkWidget          *couple_resolutions;

  GtkWidget          *type_w;
  GtkWidget          *fill_type_w;

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
static void   file_new_image_type_callback (GtkWidget    *widget,
                                            gpointer      data);
static void   file_new_fill_type_callback  (GtkWidget    *widget,
                                            gpointer      data);
static void   file_new_image_size_callback (GtkWidget    *widget,
					    gpointer      data);


/*  public functions  */

void
file_new_dialog_create (Gimp      *gimp,
                        GimpImage *gimage)
{
  NewImageInfo *info;
  GtkWidget    *main_vbox;
  GtkWidget    *hbox;
  GtkWidget    *vbox;
  GtkWidget    *abox;
  GtkWidget    *frame;
  GtkWidget    *table;
  GtkWidget    *separator;
  GtkWidget    *label;
  GtkObject    *adjustment;
  GtkWidget    *spinbutton;
  GtkWidget    *spinbutton2;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));

  info = g_new0 (NewImageInfo, 1);

  info->gimp   = gimp;
  info->values = gimp_image_new_values_new (gimp, gimage);

  info->dialog =
    gimp_viewable_dialog_new (NULL,
                              _("New Image"), "new_image",
                              GTK_STOCK_NEW,
                              _("Create a New Image"),
                              gimp_standard_help_func,
                              "dialogs/file_new.html",

                              GIMP_STOCK_RESET, file_new_reset_callback,
                              info, NULL, NULL, FALSE, FALSE,

                              GTK_STOCK_CANCEL, file_new_cancel_callback,
                              info, NULL, NULL, FALSE, TRUE,

                              GTK_STOCK_OK, file_new_ok_callback,
                              info, NULL, NULL, TRUE, FALSE,

                              NULL);

  gtk_window_set_resizable (GTK_WINDOW (info->dialog), FALSE);

  /*  vbox holding the rest of the dialog  */
  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info->dialog)->vbox),
		      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /*  Image size frame  */
  frame = gtk_frame_new (_("Image Size"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (7, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
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
  gtk_widget_show (abox);

  info->size_se = gimp_size_entry_new (0, info->values->unit, "%a",
                                       FALSE, FALSE, TRUE, SB_WIDTH,
                                       GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_col_spacing (GTK_TABLE (info->size_se), 1, 4);
  gtk_table_set_row_spacing (GTK_TABLE (info->size_se), 1, 2);
  gtk_container_add (GTK_CONTAINER (abox), info->size_se);
  gtk_widget_show (info->size_se);

  /*  height in units  */
  spinbutton = gimp_spin_button_new (&adjustment,
                                     1, 1, 1, 1, 10, 0,
                                     1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);
  /*  add the "height in units" spinbutton to the sizeentry  */
  gtk_table_attach_defaults (GTK_TABLE (info->size_se), spinbutton,
			     0, 1, 2, 3);
  gtk_widget_show (spinbutton);

  /*  height in pixels  */
  hbox = gtk_hbox_new (FALSE, 4);

  spinbutton2 = gimp_spin_button_new (&adjustment,
                                      1, 1, 1, 1, 10, 0,
                                      1, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton2), SB_WIDTH);
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
  spinbutton = gimp_spin_button_new (&adjustment,
                                     1, 1, 1, 1, 10, 0,
                                     1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);
  /*  add the "width in units" spinbutton to the sizeentry  */
  gtk_table_attach_defaults (GTK_TABLE (info->size_se), spinbutton,
			     0, 1, 1, 2);
  gtk_widget_show (spinbutton);

  /*  width in pixels  */
  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 2, 0, 1);
  gtk_widget_show (abox);

  spinbutton2 = gimp_spin_button_new (&adjustment,
                                     1, 1, 1, 1, 10, 0,
                                     1, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton2), SB_WIDTH);
  /*  add the "width in pixels" spinbutton to the main table  */
  gtk_container_add (GTK_CONTAINER (abox), spinbutton2);
  gtk_widget_show (spinbutton2);

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

  g_signal_connect (info->size_se, "refval_changed",
		    G_CALLBACK (file_new_image_size_callback),
		    info);
  g_signal_connect (info->size_se, "value_changed",
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
  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 2, 5, 7);
  gtk_widget_show (abox);

  spinbutton = gimp_spin_button_new (&adjustment,
                                     1, 1, 1, 1, 10, 0,
                                     1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);

  info->resolution_se =
    gimp_size_entry_new (1, gimp->config->default_resolution_unit,
			 _("pixels/%a"),
		         FALSE, FALSE, FALSE, SB_WIDTH,
		         GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
  gtk_table_set_col_spacing (GTK_TABLE (info->resolution_se), 1, 2);
  gtk_table_set_col_spacing (GTK_TABLE (info->resolution_se), 2, 2);
  gtk_table_set_row_spacing (GTK_TABLE (info->resolution_se), 0, 2);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (info->resolution_se),
			     GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (info->resolution_se), spinbutton,
			     1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  gtk_container_add (GTK_CONTAINER (abox), info->resolution_se);  
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

  g_signal_connect (info->resolution_se, "value_changed",
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
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  frame for Image Type  */
  frame = gimp_enum_radio_frame_new_with_range (GIMP_TYPE_IMAGE_BASE_TYPE,
                                                GIMP_RGB, GIMP_GRAY,
                                                gtk_label_new (_("Image Type")),
                                                2,
                                                G_CALLBACK (file_new_image_type_callback),
                                                info,
                                          &info->type_w);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (info->type_w),
                               GINT_TO_POINTER (info->values->type));

  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* frame for Fill Type */
  frame = gimp_enum_radio_frame_new_with_range (GIMP_TYPE_FILL_TYPE,
                                                GIMP_FOREGROUND_FILL,
                                                GIMP_TRANSPARENT_FILL,
                                                gtk_label_new (_("Fill Type")),
                                                2,
                                                G_CALLBACK (file_new_fill_type_callback),
                                                info,
                                                &info->fill_type_w);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (info->fill_type_w),
                               GINT_TO_POINTER (info->values->fill_type));

  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

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

  if (info->size > GIMP_GUI_CONFIG (info->gimp->config)->max_new_image_size)
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
  NewImageInfo   *info;
  GimpCoreConfig *config;

  info = (NewImageInfo *) data;

  config = info->gimp->config;

  g_signal_handlers_block_by_func (info->resolution_se,
                                   file_new_resolution_callback,
                                   info);

  gimp_chain_button_set_active
    (GIMP_CHAIN_BUTTON (info->couple_resolutions),
     ABS (config->default_xresolution -
	  config->default_yresolution) < GIMP_MIN_RESOLUTION);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->resolution_se), 0,
			      config->default_xresolution);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->resolution_se), 1,
			      config->default_yresolution);
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (info->resolution_se),
			    config->default_resolution_unit);

  g_signal_handlers_unblock_by_func (info->resolution_se,
                                     file_new_resolution_callback,
                                     info);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se), 0,
				  config->default_xresolution, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se), 1,
				  config->default_yresolution, TRUE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->size_se), 0,
			      config->default_image_width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (info->size_se), 1,
			      config->default_image_height);
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (info->size_se),
			    config->default_unit);

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (info->type_w),
                               GINT_TO_POINTER (config->default_image_type));

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (info->fill_type_w),
                               GINT_TO_POINTER (GIMP_BACKGROUND_FILL));
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

  size     = gimp_image_new_get_memsize_string (info->size);
  max_size = gimp_image_new_get_memsize_string (GIMP_GUI_CONFIG (info->gimp->config)->max_new_image_size);

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
			    GIMP_STOCK_INFO,
			    text,
			    GTK_STOCK_OK, GTK_STOCK_CANCEL,
			    NULL, NULL,
			    file_new_confirm_dialog_callback,
			    info);

  g_free (text);
  g_free (max_size);
  g_free (size);

  gtk_window_set_transient_for (GTK_WINDOW (info->confirm_dialog),
				GTK_WINDOW (info->dialog));

  gtk_widget_set_sensitive (info->dialog, FALSE);

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
      g_signal_handlers_block_by_func (info->resolution_se,
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

      g_signal_handlers_unblock_by_func (info->resolution_se,
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
file_new_image_type_callback (GtkWidget *widget,
                              gpointer   data)
{
  NewImageInfo *info;

  info = (NewImageInfo*) data;

  gimp_radio_button_update (widget, &info->values->type);
  
  file_new_image_size_callback (widget, data);
}

static void
file_new_fill_type_callback (GtkWidget *widget,
                             gpointer   data)
{
  NewImageInfo *info;

  info = (NewImageInfo*) data;

  gimp_radio_button_update (widget, &info->values->fill_type);
  
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

  info->size = gimp_image_new_calculate_memsize (info->values);

  text = gimp_image_new_get_memsize_string (info->size);
  gtk_label_set_text (GTK_LABEL (info->memsize_label), text);
  g_free (text);
}
