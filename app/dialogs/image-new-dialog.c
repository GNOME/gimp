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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpconfig-utils.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimptemplate.h"

#include "widgets/gimpcontainermenuimpl.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewabledialog.h"

#include "file-new-dialog.h"

#include "gimp-intl.h"


#define SB_WIDTH 10


typedef struct
{
  GtkWidget    *dialog;
  GtkWidget    *confirm_dialog;

  GtkWidget    *option_menu;
  GtkWidget    *container_menu;

  GtkWidget    *size_se;
  GtkWidget    *memsize_label;
  GtkWidget    *resolution_se;
  GtkWidget    *couple_resolutions;

  GimpTemplate *template;
  gdouble       size;

  Gimp         *gimp;
} NewImageInfo;


/*  local function prototypes  */

static void   file_new_confirm_dialog  (NewImageInfo      *info);

static void   file_new_ok_callback     (GtkWidget         *widget,
                                        NewImageInfo      *info);
static void   file_new_cancel_callback (GtkWidget         *widget,
                                        NewImageInfo      *info);
static void   file_new_reset_callback  (GtkWidget         *widget,
                                        NewImageInfo      *info);
static void   file_new_template_notify (GimpTemplate      *template,
                                        GParamSpec        *param_spec,
                                        NewImageInfo      *info);
static void   file_new_template_select (GimpContainerMenu *menu,
                                        GimpViewable      *object,
                                        gpointer           insert_data,
                                        NewImageInfo      *info);


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
  g_return_if_fail (gimage == NULL || GIMP_IS_IMAGE (gimage));

  info = g_new0 (NewImageInfo, 1);

  info->gimp     = gimp;
  info->template = gimp_image_new_template_new (gimp, gimage);

  g_signal_connect (info->template, "notify",
                    G_CALLBACK (file_new_template_notify),
                    info);

  info->dialog =
    gimp_viewable_dialog_new (NULL,
                              _("New Image"), "new_image",
                              GIMP_STOCK_IMAGE,
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

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("From _Template:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  info->option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox), info->option_menu, TRUE, TRUE, 0);
  gtk_widget_show (info->option_menu);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), info->option_menu);

  info->container_menu = gimp_container_menu_new (gimp->templates, NULL, 16);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (info->option_menu),
			    info->container_menu);
  gtk_widget_show (info->container_menu);

  gimp_container_menu_select_item (GIMP_CONTAINER_MENU (info->container_menu),
                                   NULL);

  g_signal_connect (info->container_menu, "select_item",
                    G_CALLBACK (file_new_template_select),
                    info);

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

  info->size_se = gimp_size_entry_new (0, info->template->unit, "%a",
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
  gimp_prop_size_entry_connect (G_OBJECT (info->template),
                                "width", "height", "unit",
                                info->size_se, NULL,
                                info->template->xresolution,
                                info->template->yresolution);

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

  info->resolution_se =  gimp_size_entry_new (1, info->template->resolution_unit,
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

  /*  the resolution chainbutton  */
  info->couple_resolutions = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gtk_table_attach_defaults (GTK_TABLE (info->resolution_se),
			     info->couple_resolutions, 2, 3, 0, 2);
  gtk_widget_show (info->couple_resolutions);

  gimp_prop_size_entry_connect (G_OBJECT (info->template),
                                "xresolution", "yresolution",
                                "resolution-unit",
                                info->resolution_se,
                                info->couple_resolutions,
                                1.0, 1.0);

  /*  hbox containing the Image type and fill type frames  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  frame for Image Type  */
  frame = gimp_prop_enum_radio_frame_new (G_OBJECT (info->template),
                                          "image-type",
                                          _("Image Type"),
                                          GIMP_RGB, GIMP_GRAY);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* frame for Fill Type */
  frame = gimp_prop_enum_radio_frame_new (G_OBJECT (info->template),
                                          "fill-type",
                                          _("Fill Type"),
                                          -1, -1);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  gimp_size_entry_grab_focus (GIMP_SIZE_ENTRY (info->size_se));

  gtk_widget_show (info->dialog);
}


/*  private functions  */

static void
file_new_ok_callback (GtkWidget    *widget,
		      NewImageInfo *info)
{
  if (info->size > GIMP_GUI_CONFIG (info->gimp->config)->max_new_image_size)
    {
      file_new_confirm_dialog (info);
    }
  else
    {
      gtk_widget_destroy (info->dialog);
      gimp_image_new_create_image (info->gimp, info->template);
      g_object_unref (info->template);
      g_free (info);
    }
}

static void
file_new_cancel_callback (GtkWidget    *widget,
			  NewImageInfo *info)
{
  gtk_widget_destroy (info->dialog);
  g_object_unref (info->template);
  g_free (info);
}

static void
file_new_reset_callback (GtkWidget    *widget,
			 NewImageInfo *info)
{
  gint width;
  gint height;

  width  = info->gimp->config->default_image_width;
  height = info->gimp->config->default_image_height;

  gimp_template_set_from_config (info->template, info->gimp->config);

  g_object_set (info->template,
                "width",     width,
                "height",    height,
                "fill-type", GIMP_BACKGROUND_FILL,
                NULL);

  gimp_container_menu_select_item (GIMP_CONTAINER_MENU (info->container_menu),
                                   NULL);
}

/*  local callback of file_new_confirm_dialog()  */
static void
file_new_confirm_dialog_callback (GtkWidget *widget,
				  gboolean   create,
				  gpointer   data)
{
  NewImageInfo *info = (NewImageInfo*) data;

  info->confirm_dialog = NULL;

  if (create)
    {
      gtk_widget_destroy (info->dialog);
      gimp_image_new_create_image (info->gimp, info->template);
      g_object_unref (info->template);
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
file_new_template_notify (GimpTemplate *template,
                          GParamSpec   *param_spec,
                          NewImageInfo *info)
{
  gchar *text;

  if (! strcmp (param_spec->name, "xresolution"))
    {
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se), 0,
                                      template->xresolution, FALSE);
    }
  else if (! strcmp (param_spec->name, "yresolution"))
    {
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (info->size_se), 1,
                                      template->yresolution, FALSE);
    }

  info->size = gimp_template_calc_memsize (info->template);

  text = gimp_image_new_get_memsize_string (info->size);
  gtk_label_set_text (GTK_LABEL (info->memsize_label), text);
  g_free (text);
}

static void
file_new_template_select (GimpContainerMenu *menu,
                          GimpViewable      *object,
                          gpointer           insert_data,
                          NewImageInfo      *info)
{
  if (object)
    {
      gint width;
      gint height;

      width  = GIMP_TEMPLATE (object)->width;
      height = GIMP_TEMPLATE (object)->height;

      gimp_config_copy_properties (G_OBJECT (object),
                                   G_OBJECT (info->template));

      g_object_set (info->template,
                    "width",  width,
                    "height", height,
                    NULL);
    }
}
