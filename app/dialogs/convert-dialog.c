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

#include "apptypes.h"

#include "tools/gimptool.h"
#include "tools/tool_manager.h"

#include "gdisplay.h"
#include "palette-select.h"

#include "context_manager.h"
#include "floating_sel.h"
#include "gimpdatafactory.h"
#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimage-convert.h"
#include "gimplist.h"
#include "gimplayer.h"
#include "gimppalette.h"

#include "libgimp/gimpintl.h"


typedef struct
{
  GtkWidget     *shell;
  GtkWidget     *custom_palette_button;
  GimpImage     *gimage;
  PaletteSelect *palette_select;
  gint           nodither_flag;
  gint           fsdither_flag;
  gint           fslowbleeddither_flag;
  gint           fixeddither_flag;
  gint           alphadither; /* flag */
  gint           remdups;     /* flag */
  gint           num_cols;
  gint           palette;
  gint           makepal_flag;
  gint           webpal_flag;
  gint           custompal_flag;
  gint           monopal_flag;
  gint           reusepal_flag;
} IndexedDialog;


static void indexed_ok_callback                     (GtkWidget *widget,
						     gpointer   data);
static void indexed_cancel_callback                 (GtkWidget *widget,
						     gpointer   data);

static void indexed_custom_palette_button_callback  (GtkWidget *widget,
						     gpointer   data);
static void indexed_palette_select_destroy_callback (GtkWidget *widget,
						     gpointer   data);

static GtkWidget * build_palette_button             (void);


static gboolean     UserHasWebPal    = FALSE;
static GimpPalette *theCustomPalette = NULL;

/* Defaults */
static gint     snum_cols              = 256;
static gboolean sfsdither_flag         = TRUE;
static gboolean sfslowbleeddither_flag = TRUE;
static gboolean snodither_flag         = FALSE;
static gboolean sfixeddither_flag      = FALSE;
static gboolean smakepal_flag          = TRUE;
static gboolean salphadither_flag      = FALSE;
static gboolean sremdups_flag          = TRUE;
static gboolean swebpal_flag           = FALSE;
static gboolean scustompal_flag        = FALSE;
static gboolean smonopal_flag          = FALSE;
static gboolean sreusepal_flag         = FALSE;


void
convert_to_rgb (GimpImage *gimage)
{
  gimp_image_convert (gimage, RGB, 0, 0, 0, 0, 0, NULL);
  gdisplays_flush ();
}

void
convert_to_grayscale (GimpImage* gimage)
{
  gimp_image_convert (gimage, GRAY, 0, 0, 0, 0, 0, NULL);
  gdisplays_flush ();
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
  GtkWidget     *custom_frame = NULL;
  GtkWidget     *toggle;
  GSList        *group = NULL;

  dialog = g_new0 (IndexedDialog, 1);

  dialog->gimage = gimage;

  dialog->custom_palette_button = NULL;
  dialog->palette_select        = NULL;

  dialog->num_cols              = snum_cols;
  dialog->nodither_flag         = snodither_flag;
  dialog->fsdither_flag         = sfsdither_flag;
  dialog->fslowbleeddither_flag = sfslowbleeddither_flag;
  dialog->fixeddither_flag      = sfixeddither_flag;
  dialog->alphadither           = salphadither_flag;
  dialog->remdups               = sremdups_flag;
  dialog->makepal_flag          = smakepal_flag;
  dialog->webpal_flag           = swebpal_flag;
  dialog->custompal_flag        = scustompal_flag;
  dialog->monopal_flag          = smonopal_flag;
  dialog->reusepal_flag         = sreusepal_flag;

  dialog->shell =
    gimp_dialog_new (_("Indexed Color Conversion"), "indexed_color_conversion",
		     gimp_standard_help_func,
		     "dialogs/convert_to_indexed.html",
		     GTK_WIN_POS_NONE,
		     FALSE, FALSE, TRUE,

		     _("OK"), indexed_ok_callback,
		     dialog, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), indexed_cancel_callback,
		     dialog, NULL, NULL, FALSE, TRUE,

		     NULL);

  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
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

  toggle =
    gtk_radio_button_new_with_label (NULL, _("Generate Optimal Palette:"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &(dialog->makepal_flag));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				dialog->makepal_flag);
  gtk_widget_show (toggle);

  if (dialog->num_cols == 256)
    {
      if ((! gimp_image_is_empty (gimage)) &&
	  GIMP_IMAGE_TYPE_HAS_ALPHA (gimage->base_type))
	{
	  dialog->num_cols = 255;
	}      
    }

  spinbutton = gimp_spin_button_new (&adjustment, dialog->num_cols,
				     2, 256, 1, 5, 0, 1, 0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &(dialog->num_cols));
  gtk_box_pack_end (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  label = gtk_label_new (_("# of Colors:"));
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_set_sensitive (GTK_WIDGET (spinbutton), dialog->num_cols);
  gtk_widget_set_sensitive (GTK_WIDGET (label), dialog->num_cols);
  gtk_object_set_data (GTK_OBJECT (toggle), "set_sensitive", spinbutton);
  gtk_object_set_data (GTK_OBJECT (spinbutton), "set_sensitive", label);

  gtk_widget_show (hbox);

  /* 'custom' palette from dialog */
  dialog->custom_palette_button = build_palette_button ();
  if (dialog->custom_palette_button)
    {
      /* create the custom_frame here, it'll be added later */
      custom_frame = gtk_frame_new (_("Custom Palette Options"));
      gtk_container_set_border_width (GTK_CONTAINER (custom_frame), 2);
      
      /*  The remove-duplicates toggle  */
      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_add (GTK_CONTAINER (custom_frame), hbox);
      toggle = gtk_check_button_new_with_label (_("Remove Unused Colors from Final Palette"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), dialog->remdups);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &(dialog->remdups));
      gtk_widget_show (toggle);
      gtk_widget_show (hbox);      

      /* 'custom' palette from dialog */
      hbox = gtk_hbox_new (FALSE, 4);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      toggle = gtk_radio_button_new_with_label (group, _("Use Custom Palette:"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
      gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &(dialog->custompal_flag));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				    dialog->custompal_flag);
      gtk_object_set_data (GTK_OBJECT (toggle), "set_sensitive", custom_frame);
      gtk_widget_show (toggle);
      
      gtk_signal_connect (GTK_OBJECT (dialog->custom_palette_button), "clicked",
			  GTK_SIGNAL_FUNC (indexed_custom_palette_button_callback), 
			  dialog);
      gtk_box_pack_end (GTK_BOX (hbox), dialog->custom_palette_button, TRUE, TRUE, 0);
      gtk_widget_show (dialog->custom_palette_button);
      gtk_widget_show (hbox);

      gtk_widget_set_sensitive (GTK_WIDGET (custom_frame),
				dialog->custompal_flag);
      gtk_widget_set_sensitive (GTK_WIDGET (dialog->custom_palette_button),
				dialog->custompal_flag);
      gtk_object_set_data (GTK_OBJECT (toggle), "set_sensitive", custom_frame);
      gtk_object_set_data (GTK_OBJECT (custom_frame), "set_sensitive",
			   dialog->custom_palette_button);
    }

  if (! UserHasWebPal)
    {
      /*  'web palette'
       * Only presented as an option to the user if they do not
       * already have the 'Web' GIMP palette installed on their
       * system.
       */
      hbox = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      toggle =
	gtk_radio_button_new_with_label (group, _("Use WWW-Optimized Palette"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
      gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &(dialog->webpal_flag));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				    dialog->webpal_flag);
      gtk_widget_show (toggle);
      gtk_widget_show (hbox);
    }

  /*  'mono palette'  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  toggle =
    gtk_radio_button_new_with_label (group, _("Use Black/White (1-Bit) Palette"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &(dialog->monopal_flag));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				dialog->monopal_flag);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  frame = gtk_frame_new (_("Dither Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show(vbox);

  /*  The dither radio buttons  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      
  toggle = gtk_radio_button_new_with_label (NULL, _("No Color Dithering"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &(dialog->nodither_flag));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				dialog->nodither_flag);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  toggle = gtk_radio_button_new_with_label (group, _("Positioned Color Dithering"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &(dialog->fixeddither_flag));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				dialog->fixeddither_flag);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  toggle = gtk_radio_button_new_with_label (group, _("Floyd-Steinberg Color Dithering (Reduced Color Bleeding)"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &(dialog->fslowbleeddither_flag));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				dialog->fslowbleeddither_flag);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  toggle = gtk_radio_button_new_with_label (group, _("Floyd-Steinberg Color Dithering (Normal)"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &(dialog->fsdither_flag));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				dialog->fsdither_flag);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  /*  The alpha-dither toggle  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  toggle = gtk_check_button_new_with_label (_("Enable Dithering of Transparency"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				dialog->alphadither);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &(dialog->alphadither));
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);
  
  /* now add the custom_frame */
  if (custom_frame)
    {
      gtk_box_pack_start (GTK_BOX (main_vbox), custom_frame, FALSE, FALSE, 0);
      gtk_widget_show (custom_frame);
    }
  
  /* if the image isn't non-alpha/layered, set the default number of
     colours to one less than max, to leave room for a transparent index
     for transparent/animated GIFs */
  if (! gimp_image_is_empty (gimage) &&
      GIMP_IMAGE_TYPE_HAS_ALPHA (gimage->base_type))
    {
      frame = gtk_frame_new (_("[ Warning ]"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      label = gtk_label_new
	(_("You are attempting to convert an image with alpha/layers "
	   "from RGB/GRAY to INDEXED.\nYou should not generate a "
	   "palette of more than 255 colors if you intend to create "
	   "a transparent or animated GIF file from this image."));
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_container_add (GTK_CONTAINER (vbox), label);
      gtk_widget_show (label);

    }

  gtk_widget_show (dialog->shell);
}


static GtkWidget *
build_palette_button (void)
{
  GList       *list;
  GimpPalette *palette;
  GimpPalette *theWebPalette = NULL;
  gint         i;
  gint         default_palette;

  UserHasWebPal = FALSE;

  list = GIMP_LIST (global_palette_factory->container)->list;

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
	  g_strcasecmp (GIMP_OBJECT (palette)->name, "Web") == 0)
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
	   for (i = 0, list = GIMP_LIST (global_palette_factory->container)->list;
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
indexed_ok_callback (GtkWidget *widget,
		     gpointer   data)
{
  IndexedDialog      *dialog;
  ConvertPaletteType  palette_type;
  ConvertDitherType   dither_type;

  dialog = (IndexedDialog *) data;

  if (dialog->webpal_flag)
    palette_type = WEB_PALETTE;
  else if (dialog->custompal_flag)
    palette_type = CUSTOM_PALETTE;
  else if (dialog->monopal_flag)
    palette_type = MONO_PALETTE;
  else if (dialog->makepal_flag)
    palette_type = MAKE_PALETTE;
  else
    palette_type = REUSE_PALETTE;

  if (dialog->nodither_flag)
    dither_type = NO_DITHER;
  else if (dialog->fsdither_flag)
    dither_type = FS_DITHER;
  else if (dialog->fslowbleeddither_flag)
    dither_type = FSLOWBLEED_DITHER;
  else
    dither_type = FIXED_DITHER;

  /* Close the dialogs when open because they're useless for indexed
   *  images and could crash the GIMP when used nevertheless
   */
  if (active_tool)
    tool_manager_control_active (HALT, active_tool->gdisp);
  
  /*  Convert the image to indexed color  */
  gimp_image_convert (dialog->gimage, INDEXED, dialog->num_cols,
		      dither_type, dialog->alphadither,
		      dialog->remdups, palette_type, theCustomPalette);
  gdisplays_flush ();

  /* Save defaults for next time */
  snum_cols              = dialog->num_cols;
  snodither_flag         = dialog->nodither_flag;
  sfsdither_flag         = dialog->fsdither_flag;
  sfslowbleeddither_flag = dialog->fslowbleeddither_flag;
  sfixeddither_flag      = dialog->fixeddither_flag;
  salphadither_flag      = dialog->alphadither;
  sremdups_flag          = dialog->remdups;
  smakepal_flag          = dialog->makepal_flag;
  swebpal_flag           = dialog->webpal_flag;
  scustompal_flag        = dialog->custompal_flag;
  smonopal_flag          = dialog->monopal_flag;
  sreusepal_flag         = dialog->reusepal_flag;

  if (dialog->palette_select)
    gtk_widget_destroy (dialog->palette_select->shell);  

  gtk_widget_destroy (dialog->shell);

  g_free (dialog);
}

static void
indexed_cancel_callback (GtkWidget *widget,
			 gpointer   data)
{
  IndexedDialog *dialog;

  dialog = (IndexedDialog *) data;

  if (dialog->palette_select)
    gtk_widget_destroy (dialog->palette_select->shell);

  gtk_widget_destroy (dialog->shell);

  g_free (dialog);
}

static void
indexed_palette_select_destroy_callback (GtkWidget *widget,
					 gpointer   data)
{
  IndexedDialog *dialog = (IndexedDialog *)data;

  if (dialog)
    dialog->palette_select = NULL;
}

static gint
indexed_palette_select_palette (GimpContext *context,
				GimpPalette *palette,
				gpointer     data)
{
  IndexedDialog *dialog;

  dialog = (IndexedDialog *) data;

  if (palette)
    {
      if (palette->n_colors <= 256)
	{
	  theCustomPalette = palette;

	  gtk_label_set_text (GTK_LABEL (GTK_BIN (dialog->custom_palette_button)->child),
			      GIMP_OBJECT (theCustomPalette)->name);
	}
    }

  return FALSE;
}

static void
indexed_custom_palette_button_callback (GtkWidget *widget,
					gpointer   data)
{
  IndexedDialog *dialog = (IndexedDialog *)data;

  if (dialog->palette_select == NULL)
    {
      dialog->palette_select =
	palette_select_new (_("Select Custom Palette"), 
			    GIMP_OBJECT (theCustomPalette)->name);

      gtk_signal_connect (GTK_OBJECT (dialog->palette_select->shell), "destroy", 
			  GTK_SIGNAL_FUNC (indexed_palette_select_destroy_callback), 
			  dialog);
      gtk_signal_connect (GTK_OBJECT (dialog->palette_select->context),
			  "palette_changed",
			  GTK_SIGNAL_FUNC (indexed_palette_select_palette),
			  dialog);
    } 
  else
    {
      gdk_window_raise (dialog->palette_select->shell->window);
    }
}
