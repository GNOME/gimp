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

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpdatafactory.h"
#include "core/gimpimage.h"
#include "core/gimpimage-convert.h"
#include "core/gimplist.h"
#include "core/gimppalette.h"

#include "widgets/gimpenummenu.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "palette-select.h"

#include "gimp-intl.h"


typedef struct
{
  GtkWidget              *shell;
  GtkWidget              *custom_palette_button;
  GimpImage              *gimage;
  PaletteSelect          *palette_select;
  GimpConvertDitherType   dither_type;
  gboolean                alpha_dither;
  gboolean                remove_dups;
  gint                    num_colors;
  gint                    palette;
  GimpConvertPaletteType  palette_type;
} IndexedDialog;


static void indexed_response                        (GtkWidget     *widget,
                                                     gint           response_id,
						     IndexedDialog *dialog);
static void indexed_destroy_callback                (GtkWidget     *widget,
						     IndexedDialog *dialog);

static void indexed_custom_palette_button_callback  (GtkWidget     *widget,
						     IndexedDialog *dialog);
static void indexed_palette_select_destroy_callback (GtkWidget     *widget,
						     IndexedDialog *dialog);

static GtkWidget * build_palette_button             (Gimp      *gimp);


static gboolean     UserHasWebPal    = FALSE;
static GimpPalette *theCustomPalette = NULL;

/* Defaults */
static GimpConvertDitherType   saved_dither_type  = GIMP_FS_DITHER;
static gboolean                saved_alpha_dither = FALSE;
static gboolean                saved_remove_dups  = TRUE;
static gint                    saved_num_colors   = 256;
static GimpConvertPaletteType  saved_palette_type = GIMP_MAKE_PALETTE;


void
convert_to_rgb (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimp_image_convert (gimage, GIMP_RGB, 0, 0, 0, 0, 0, NULL);
  gimp_image_flush (gimage);
}

void
convert_to_grayscale (GimpImage* gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimp_image_convert (gimage, GIMP_GRAY, 0, 0, 0, 0, 0, NULL);
  gimp_image_flush (gimage);
}

void
convert_to_indexed (GimpImage *gimage)
{
  IndexedDialog *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkObject     *adjustment;
  GtkWidget     *spinbutton;
  GtkWidget     *frame;
  GtkWidget     *toggle;
  GSList        *group = NULL;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  dialog = g_new0 (IndexedDialog, 1);

  dialog->gimage = gimage;

  dialog->custom_palette_button = build_palette_button (gimage->gimp);
  dialog->dither_type           = saved_dither_type;
  dialog->alpha_dither          = saved_alpha_dither;
  dialog->remove_dups           = saved_remove_dups;
  dialog->num_colors            = saved_num_colors;
  dialog->palette_type          = saved_palette_type;

  dialog->shell =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (gimage),
                              _("Indexed Color Conversion"),
                              "gimp-image-convert-indexed",
                              GIMP_STOCK_CONVERT_INDEXED,
                              _("Convert Image to Indexed Colors"),
                              gimp_standard_help_func,
                              GIMP_HELP_IMAGE_CONVERT_INDEXED,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  g_signal_connect (dialog->shell, "response",
                    G_CALLBACK (indexed_response),
                    dialog);

  g_signal_connect (dialog->shell, "destroy",
                    G_CALLBACK (indexed_destroy_callback),
                    dialog);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog->shell)->vbox),
		     main_vbox);
  gtk_widget_show (main_vbox);

  frame = gtk_frame_new (_("General Palette Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  'generate palette'  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  toggle = gtk_radio_button_new_with_label (NULL,
                                            _("Generate Optimum Palette:"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                     GINT_TO_POINTER (GIMP_MAKE_PALETTE));
  g_signal_connect (toggle, "toggled",
		    G_CALLBACK (gimp_radio_button_update),
		    &dialog->palette_type);

  if (dialog->num_colors == 256 && gimp_image_has_alpha (gimage))
    {
      dialog->num_colors = 255;
    }

  spinbutton = gimp_spin_button_new (&adjustment, dialog->num_colors,
				     2, 256, 1, 8, 0, 1, 0);
  gtk_box_pack_end (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adjustment, "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &dialog->num_colors);

  label = gtk_label_new (_("Max. Number of Colors:"));
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_set_sensitive (GTK_WIDGET (spinbutton), dialog->num_colors);
  gtk_widget_set_sensitive (GTK_WIDGET (label), dialog->num_colors);
  g_object_set_data (G_OBJECT (toggle), "set_sensitive", spinbutton);
  g_object_set_data (G_OBJECT (spinbutton), "set_sensitive", label);

  gtk_widget_show (hbox);

  if (! UserHasWebPal)
    {
      /*  'web palette'
       * Only presented as an option to the user if they do not
       * already have the 'Web' GIMP palette installed on their
       * system.
       */
      hbox = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      toggle = gtk_radio_button_new_with_label (group,
                                                _("Use WWW-Optimized Palette"));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
      gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                         GINT_TO_POINTER (GIMP_WEB_PALETTE));
      g_signal_connect (toggle, "toggled",
			G_CALLBACK (gimp_radio_button_update),
			&dialog->palette_type);
    }

  /*  'mono palette'  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  toggle = gtk_radio_button_new_with_label (group,
                                            _("Use Black and White (1-Bit) Palette"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                     GINT_TO_POINTER (GIMP_MONO_PALETTE));
  g_signal_connect (toggle, "toggled",
		    G_CALLBACK (gimp_radio_button_update),
		    &dialog->palette_type);

  /* 'custom' palette from dialog */
  if (dialog->custom_palette_button)
    {
      GtkWidget *remove_toggle;

      remove_toggle = gtk_check_button_new_with_label (_("Remove Unused Colors from Final Palette"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (remove_toggle),
                                    dialog->remove_dups);
      g_signal_connect (remove_toggle, "toggled",
			G_CALLBACK (gimp_toggle_button_update),
			&dialog->remove_dups);

      /* 'custom' palette from dialog */
      hbox = gtk_hbox_new (FALSE, 4);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      toggle = gtk_radio_button_new_with_label (group,
                                                _("Use Custom Palette:"));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
      gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                         GINT_TO_POINTER (GIMP_CUSTOM_PALETTE));
      g_signal_connect (toggle, "toggled",
			G_CALLBACK (gimp_radio_button_update),
			&dialog->palette_type);
      g_object_set_data (G_OBJECT (toggle), "set_sensitive", remove_toggle);

      g_signal_connect (dialog->custom_palette_button, "clicked",
			G_CALLBACK (indexed_custom_palette_button_callback),
			dialog);
      gtk_box_pack_end (GTK_BOX (hbox),
                        dialog->custom_palette_button, TRUE, TRUE, 0);
      gtk_widget_show (dialog->custom_palette_button);

      gtk_widget_set_sensitive (remove_toggle,
				dialog->palette_type == GIMP_CUSTOM_PALETTE);
      gtk_widget_set_sensitive (dialog->custom_palette_button,
                                dialog->palette_type == GIMP_CUSTOM_PALETTE);
      g_object_set_data (G_OBJECT (toggle), "set_sensitive", remove_toggle);
      g_object_set_data (G_OBJECT (remove_toggle), "set_sensitive",
			 dialog->custom_palette_button);

      /*  add the remove-duplicates toggle  */
      hbox = gtk_hbox_new (FALSE, 4);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      gtk_box_pack_start (GTK_BOX (hbox), remove_toggle, FALSE, FALSE, 20);
      gtk_widget_show (remove_toggle);
    }

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (toggle),
                               GINT_TO_POINTER (dialog->palette_type));

  /*  the dither type  */
  frame = gtk_frame_new (_("Dithering Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gimp_enum_radio_box_new (GIMP_TYPE_CONVERT_DITHER_TYPE,
                                  G_CALLBACK (gimp_radio_button_update),
                                  &dialog->dither_type,
                                  &toggle);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (toggle),
                               GINT_TO_POINTER (dialog->dither_type));
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show(vbox);

  /*  the alpha-dither toggle  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  toggle =
    gtk_check_button_new_with_label (_("Enable Dithering of Transparency"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				dialog->alpha_dither);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);
  g_signal_connect (toggle, "toggled",
		    G_CALLBACK (gimp_toggle_button_update),
		    &dialog->alpha_dither);

  /* if the image isn't non-alpha/layered, set the default number of
     colours to one less than max, to leave room for a transparent index
     for transparent/animated GIFs */
  if (gimp_image_has_alpha (gimage))
    {
      frame = gtk_frame_new (_("[ Warning ]"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      label = gtk_label_new
	(_("You are attempting to convert an image with an alpha channel "
           "to indexed colors.\n"
           "Do not generate a palette of more than 255 colors if you "
           "intend to create a transparent or animated GIF file."));
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_container_add (GTK_CONTAINER (vbox), label);
      gtk_widget_show (label);
    }

  gtk_widget_show (dialog->shell);
}


static GtkWidget *
build_palette_button (Gimp *gimp)
{
  GList       *list;
  GimpPalette *palette;
  GimpPalette *theWebPalette = NULL;
  gint         i;
  gint         default_palette;

  UserHasWebPal = FALSE;

  list = GIMP_LIST (gimp->palette_factory->container)->list;

  if (! list)
    {
      return NULL;
    }

  for (i = 0, default_palette = -1;
       list;
       i++, list = g_list_next (list))
    {
      palette = (GimpPalette *) list->data;

      /* Preferentially, the initial default is 'Web' if available */
      if (theWebPalette == NULL &&
	  g_ascii_strcasecmp (GIMP_OBJECT (palette)->name, "Web") == 0)
	{
	  theWebPalette = palette;
	  UserHasWebPal = TRUE;
	}

      /* We can't dither to > 256 colors */
      if (palette->n_colors <= 256)
	{
	  if (theCustomPalette == palette)
	    {
	      default_palette = i;
	    }
	}
    }

  /* default to first one with <= 256 colors
   * (only used if 'web' palette not available)
   */
   if (default_palette == -1)
     {
       if (theWebPalette)
	 {
	   theCustomPalette = theWebPalette;
	   default_palette = 1;  /*  dummy value  */
	 }
       else
	 {
	   for (i = 0, list = GIMP_LIST (gimp->palette_factory->container)->list;
		list && default_palette == -1;
		i++, list = g_list_next (list))
	     {
	       palette = (GimpPalette *) list->data;

	       if (palette->n_colors <= 256)
		 {
		   theCustomPalette = palette;
		   default_palette = i;
		 }
	     }
	 }
     }

   if (default_palette == -1)
     return NULL;
   else
     return gtk_button_new_with_label (GIMP_OBJECT (theCustomPalette)->name);
}


static void
indexed_response (GtkWidget     *widget,
                  gint           response_id,
                  IndexedDialog *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      /*  Convert the image to indexed color  */
      gimp_image_convert (dialog->gimage,
                          GIMP_INDEXED,
                          dialog->num_colors,
                          dialog->dither_type,
                          dialog->alpha_dither,
                          dialog->remove_dups,
                          dialog->palette_type,
                          theCustomPalette);
      gimp_image_flush (dialog->gimage);

      /* Save defaults for next time */
      saved_dither_type  = dialog->dither_type;
      saved_alpha_dither = dialog->alpha_dither;
      saved_remove_dups  = dialog->remove_dups;
      saved_num_colors   = dialog->num_colors;
      saved_palette_type = dialog->palette_type;
    }

  gtk_widget_destroy (dialog->shell);
}

static void
indexed_destroy_callback (GtkWidget     *widget,
                          IndexedDialog *dialog)
{
  if (dialog->palette_select)
    gtk_widget_destroy (dialog->palette_select->shell);

  g_free (dialog);
}

static void
indexed_palette_select_destroy_callback (GtkWidget     *widget,
                                         IndexedDialog *dialog)
{
  dialog->palette_select = NULL;
}

static void
indexed_palette_select_palette (GimpContext   *context,
				GimpPalette   *palette,
                                IndexedDialog *dialog)
{
  if (palette && palette->n_colors <= 256)
    {
      theCustomPalette = palette;

      gtk_label_set_text (GTK_LABEL (GTK_BIN (dialog->custom_palette_button)->child),
                          GIMP_OBJECT (theCustomPalette)->name);
    }
}

static void
indexed_custom_palette_button_callback (GtkWidget     *widget,
                                        IndexedDialog *dialog)
{
  if (dialog->palette_select == NULL)
    {
      dialog->palette_select =
	palette_select_new (dialog->gimage->gimp,
                            _("Select Custom Palette"),
			    GIMP_OBJECT (theCustomPalette)->name,
                            NULL);

      g_signal_connect (dialog->palette_select->shell, "destroy",
			G_CALLBACK (indexed_palette_select_destroy_callback),
			dialog);
      g_signal_connect (dialog->palette_select->context,
			"palette_changed",
			G_CALLBACK (indexed_palette_select_palette),
			dialog);
    }
  else
    {
      gtk_window_present (GTK_WINDOW (dialog->palette_select->shell));
    }
}
