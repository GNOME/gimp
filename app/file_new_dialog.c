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
#include "appenv.h"
#include "actionarea.h"
#include "file_new_dialog.h"
#include "gimage.h"
#include "gimpcontext.h"
#include "gimprc.h"
#include "global_edit.h"
#include "interface.h"
#include "lc_dialog.h"
#include "plug_in.h"
#include "tile_manager_pvt.h"
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

  GtkWidget *type_w[2];
  GtkWidget *fill_type_w[4];

  gint width;
  gint height;
  GUnit unit;

  gdouble xresolution;
  gdouble yresolution;
  GUnit res_unit;

  gdouble size;  /* in bytes */

  GimpImageBaseType type;
  GimpFillType fill_type;
} NewImageValues;

/*  new image local functions  */
static void file_new_create_image (NewImageValues *);
static void file_new_confirm_dialog (NewImageValues *);
static gchar * file_new_print_size (gdouble);

static void file_new_ok_callback (GtkWidget *, gpointer);
static void file_new_reset_callback (GtkWidget *, gpointer);
static gint file_new_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void file_new_cancel_callback (GtkWidget *, gpointer);
static void file_new_toggle_callback (GtkWidget *, gpointer);
static void file_new_resolution_callback (GtkWidget *, gpointer);
static void file_new_image_size_callback (GtkWidget *, gpointer);

/*  static variables  */
static gint     last_width       = 256;
static gint     last_height      = 256;
static GUnit    last_unit        = UNIT_INCH;

static gdouble  last_xresolution = 72.0;
static gdouble  last_yresolution = 72.0;
static GUnit    last_res_unit    = UNIT_INCH;

static GimpImageBaseType last_type = RGB;
static GimpFillType last_fill_type = BACKGROUND_FILL; 

static gboolean last_new_image   = FALSE;
static gboolean new_dialog_run   = FALSE;

extern TileManager *global_buf;

/*  functions  */
static void
file_new_create_image (NewImageValues *vals)
{
  GImage   *gimage;
  GDisplay *gdisplay;
  Layer    *layer;
  GimpImageType type;

  last_width = vals->width;
  last_height = vals->height;
  last_type = vals->type;
  last_fill_type = vals->fill_type;
  last_xresolution = vals->xresolution;
  last_yresolution = vals->yresolution;
  last_unit = vals->unit;
  last_res_unit = vals->res_unit;
  last_new_image = TRUE;

  switch (vals->fill_type)
    {
    case BACKGROUND_FILL:
    case FOREGROUND_FILL:
    case WHITE_FILL:
      type = (vals->type == RGB) ? RGB_GIMAGE : GRAY_GIMAGE;
      break;
    case TRANSPARENT_FILL:
      type = (vals->type == RGB) ? RGBA_GIMAGE : GRAYA_GIMAGE;
      break;
    default:
      type = RGB_IMAGE;
      break;
    }

  gimage = gimage_new (vals->width, vals->height, vals->type);

  gimp_image_set_resolution (gimage, vals->xresolution, vals->yresolution);
  gimp_image_set_unit (gimage, vals->unit);

  /*  Make the background (or first) layer  */
  layer = layer_new (gimage, gimage->width, gimage->height,
		     type, _("Background"), OPAQUE_OPACITY, NORMAL_MODE);

  if (layer)
    {
      /*  add the new layer to the gimage  */
      gimage_disable_undo (gimage);
      gimage_add_layer (gimage, layer, 0);
      gimage_enable_undo (gimage);
      
      drawable_fill (GIMP_DRAWABLE(layer), vals->fill_type);
      
      gimage_clean_all (gimage);
      
      gdisplay = gdisplay_new (gimage, 0x0101);

      gimp_context_set_display (gimp_context_get_user (), gdisplay);
    }

  g_free (vals);
}

static void
file_new_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  NewImageValues *vals;

  vals = data;

  /* get the image size in pixels */
  vals->width = (int)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (vals->size_se), 0) + 0.5);
  vals->height = (int)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (vals->size_se), 1) + 0.5);

  /* get the resolution in dpi */
  vals->xresolution =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (vals->resolution_se), 0);
  vals->yresolution =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (vals->resolution_se), 1);

  /* get the units */
  vals->unit =
    gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (vals->size_se));
  vals->res_unit =
    gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (vals->resolution_se));

  if (vals->size > max_new_image_size)
    {
      file_new_confirm_dialog (vals);
    }
  else
    {
      gtk_widget_destroy (vals->dlg);
      file_new_create_image (vals);
    }
}

static void
file_new_reset_callback (GtkWidget *widget,
			 gpointer   data)
{
  NewImageValues *vals;

  vals = data;

  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->resolution_se), vals);

  gimp_chain_button_set_active
    (GIMP_CHAIN_BUTTON (vals->couple_resolutions),
     ABS (default_xresolution - default_yresolution) < GIMP_MIN_RESOLUTION);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->resolution_se),
			      0, default_xresolution);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->resolution_se),
			      1, default_yresolution);
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (vals->resolution_se),
			    default_resolution_units);

  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->resolution_se), vals);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (vals->size_se),
				  0, default_xresolution, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (vals->size_se),
				  1, default_yresolution, TRUE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->size_se),
			      0, default_width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->size_se),
			      1, default_height);
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (vals->size_se),
			    default_units);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vals->type_w[default_type]),
				TRUE);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (vals->fill_type_w[BACKGROUND_FILL]), TRUE);
}

static gint
file_new_delete_callback (GtkWidget *widget,
			  GdkEvent  *event,
			  gpointer   data)
{
  file_new_cancel_callback (widget, data);
  return TRUE;
}

static void
file_new_cancel_callback (GtkWidget *widget,
			  gpointer   data)
{
  NewImageValues *vals;

  vals = data;

  gtk_widget_destroy (vals->dlg);
  g_free (vals);
}

/*  local callbacks of file_new_confirm_dialog()  */
static void
file_new_confirm_dialog_ok_callback (GtkWidget      *widget,
				     NewImageValues *vals)
{
  gtk_widget_destroy (vals->confirm_dlg);
  gtk_widget_destroy (vals->dlg);
  file_new_create_image (vals);
}

static void
file_new_confirm_dialog_cancel_callback (GtkWidget      *widget,
					 NewImageValues *vals)
{
  gtk_widget_destroy (vals->confirm_dlg);
  vals->confirm_dlg = NULL;
  gtk_widget_set_sensitive (vals->dlg, TRUE);
}

static gint
file_new_confirm_dialog_delete_callback (GtkWidget      *widget,
					 GdkEvent       *event,
					 NewImageValues *vals)
{
  file_new_confirm_dialog_cancel_callback (widget, vals);
  return TRUE;
}

static void
file_new_confirm_dialog (NewImageValues *vals)
{
  GtkWidget *label;
  gchar *size;
  gchar *max_size;
  gchar *text;

  static ActionAreaItem action_items[] =
  {
    { N_("OK"),
      (ActionCallback) file_new_confirm_dialog_ok_callback, NULL, NULL },
    { N_("Cancel"),
      (ActionCallback) file_new_confirm_dialog_cancel_callback, NULL, NULL }
  };

  gtk_widget_set_sensitive (vals->dlg, FALSE);

  vals->confirm_dlg = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (vals->confirm_dlg),
			  "confirm_size", "Gimp");
  gtk_window_set_title (GTK_WINDOW (vals->confirm_dlg), _("Confirm Image Size"));
  gtk_window_set_policy (GTK_WINDOW (vals->confirm_dlg), FALSE, FALSE, FALSE);
  gtk_window_position (GTK_WINDOW (vals->confirm_dlg), GTK_WIN_POS_MOUSE);

  /*  Handle the wm close signal  */
  gtk_signal_connect (GTK_OBJECT (vals->confirm_dlg), "delete_event",
		      (GtkSignalFunc) file_new_confirm_dialog_delete_callback,
		      vals);

  /*  The action area  */
  action_items[0].user_data = vals;
  action_items[1].user_data = vals;
  build_action_area (GTK_DIALOG (vals->confirm_dlg), action_items, 2, 0);

  text = g_strdup_printf (_("You are trying to create an image which\n"
			    "has an initial size of %s.\n\n"
			    "Choose OK to create this image anyway.\n"
			    "Choose Cancel if you didn't mean to\n"
			    "create such a large image.\n\n"
			    "To prevent this dialog from appearing,\n"
			    "increase the \"Maximum Image Size\"\n"
			    "setting (currently %s) in the\n"
			    "preferences dialog."),
			  size = file_new_print_size (vals->size),
			  max_size = file_new_print_size (max_new_image_size));
  label = gtk_label_new (text);
  gtk_misc_set_padding (GTK_MISC (label), 6, 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->confirm_dlg)->vbox), label,
		      TRUE, TRUE, 0);
  gtk_widget_show (label);

  g_free (text);
  g_free (max_size);
  g_free (size);

  gtk_widget_show (vals->confirm_dlg);
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
  NewImageValues *vals;

  static gdouble xres = 0.0;
  static gdouble yres = 0.0;
  gdouble new_xres;
  gdouble new_yres;

  vals = data;

  new_xres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_yres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if (gimp_chain_button_get_active
      (GIMP_CHAIN_BUTTON (vals->couple_resolutions)))
    {
      gtk_signal_handler_block_by_data
	(GTK_OBJECT (vals->resolution_se), vals);

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
	(GTK_OBJECT (vals->resolution_se), vals);
    }
  else
    {
      if (new_xres != xres)
	xres = new_xres;
      if (new_yres != yres)
	yres = new_yres;
    }

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (vals->size_se), 0,
				  xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (vals->size_se), 1,
				  yres, FALSE);

  file_new_image_size_callback (widget, data);
}

static gchar *
file_new_print_size (gdouble size)
{
  if (size < 4096)
    return g_strdup_printf (_("%d Bytes"), (gint) size);
  else if (size < 1024 * 10)
    return g_strdup_printf (_("%.2f KB"), size / 1024);
  else if (size < 1024 * 100)
    return g_strdup_printf (_("%.1f KB"), size / 1024);
  else if (size < 1024 * 1024)
    return g_strdup_printf (_("%d KB"), (gint) size / 1024);
  else if (size < 1024 * 1024 * 10)
    return g_strdup_printf (_("%.2f MB"), size / 1024 / 1024);
  else
    return g_strdup_printf (_("%.1f MB"), size / 1024 / 1024);
}

static void
file_new_image_size_callback (GtkWidget *widget,
			      gpointer   data)
{
  NewImageValues *vals;

  gdouble width, height, size;
  gchar *text;
  gchar *label;

  vals = data;

  width = (gdouble) (gint)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (vals->size_se), 0) + 0.5);
  height = (gdouble) (gint)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (vals->size_se), 1) + 0.5);

  size =
    width * height *
    ((vals->type == RGB ? 3 : 1) +	             /* bytes per pixel */
     (vals->fill_type == TRANSPARENT_FILL ? 1 : 0)); /* alpha channel */

  label = g_strdup_printf (_("Image Size: %s"),
			   text = file_new_print_size (size));
  gtk_frame_set_label (GTK_FRAME (vals->size_frame), label);

  g_free (label);
  g_free (text);

  vals->size = size;
}

void
file_new_cmd_callback (GtkWidget *widget,
		       gpointer   callback_data,
		       guint      callback_action)
{
  GDisplay       *gdisp;
  NewImageValues *vals;

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
  gint    i;

  static ActionAreaItem action_items[] =
  {
    { N_("OK"), file_new_ok_callback, NULL, NULL },
    { N_("Reset"), file_new_reset_callback, NULL, NULL },
    { N_("Cancel"), file_new_cancel_callback, NULL, NULL }
  };

  static gchar *type_names[] =
  {
    N_("RGB"),
    N_("Grayscale")
  };
  static gint ntypes = sizeof (type_names) / sizeof (type_names[0]);

  static gchar *fill_type_names[] =
  {
    N_("Foreground"),
    N_("Background"),
    N_("White"),
    N_("Transparent")
  };
  static gint nfill_types =
    sizeof (fill_type_names) / sizeof (fill_type_names[0]);

  if(!new_dialog_run)
    {
      /*  all from gimprc  */
      last_width = default_width;
      last_height = default_height;
      last_unit = default_units;

      last_xresolution = default_xresolution;
      last_yresolution = default_yresolution;
      last_res_unit = default_resolution_units;

      last_type = default_type;
 
      new_dialog_run = TRUE;  
    }

  /*  Before we try to determine the responsible gdisplay,
   *  make sure this wasn't called from the toolbox
   */
  if (callback_action)
    gdisp = gdisplay_active ();
  else
    gdisp = NULL;

  vals = g_malloc (sizeof (NewImageValues));
  vals->confirm_dlg = NULL;
  vals->size = 0.0;
  vals->res_unit = last_res_unit;
  vals->fill_type = last_fill_type;

  if (gdisp)
    {
      vals->width = gdisp->gimage->width;
      vals->height = gdisp->gimage->height;
      vals->unit = gdisp->gimage->unit;

      vals->xresolution = gdisp->gimage->xresolution;
      vals->yresolution = gdisp->gimage->yresolution;

      vals->type = gimage_base_type (gdisp->gimage);
    }
  else
    {
      vals->width = last_width;
      vals->height = last_height;
      vals->unit = last_unit;

      vals->xresolution = last_xresolution;
      vals->yresolution = last_yresolution;

      vals->type = last_type;
    }

  if (vals->type == INDEXED)
    vals->type = RGB;    /* no indexed images */

  /*  If a cut buffer exists, default to using its size for the new image
   *  also check to see if a new_image has been opened
   */

  if(global_buf && !last_new_image)
    {
      vals->width = global_buf->width;
      vals->height = global_buf->height;
    }

  vals->dlg = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (vals->dlg), "new_image", "Gimp");
  gtk_window_set_title (GTK_WINDOW (vals->dlg), _("New Image"));
  gtk_window_set_position (GTK_WINDOW (vals->dlg), GTK_WIN_POS_MOUSE);
  gtk_window_set_policy(GTK_WINDOW (vals->dlg), FALSE, FALSE, TRUE);

  /*  Handle the wm close signal  */
  gtk_signal_connect (GTK_OBJECT (vals->dlg), "delete_event",
		      GTK_SIGNAL_FUNC (file_new_delete_callback),
		      vals);

  /*  The action area  */
  action_items[0].user_data = vals;
  action_items[1].user_data = vals;
  action_items[2].user_data = vals;
  build_action_area (GTK_DIALOG (vals->dlg), action_items, 3, 2);

  /*  vbox holding the rest of the dialog  */
  top_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (top_vbox), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->vbox),
		      top_vbox, TRUE, TRUE, 0);
  gtk_widget_show (top_vbox);

  /*  Image size frame  */
  vals->size_frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (top_vbox), vals->size_frame, FALSE, FALSE, 0);
  gtk_widget_show (vals->size_frame);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (vals->size_frame), vbox);
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
  vals->size_se =
    gimp_size_entry_new (0, vals->unit, "%a", FALSE, FALSE, TRUE, 75,
			 GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_col_spacing (GTK_TABLE (vals->size_se), 1, 2);
  gtk_container_add (GTK_CONTAINER (abox), vals->size_se);
  gtk_widget_show (vals->size_se);
  gtk_widget_show (abox);

  /*  height in units  */
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
                                   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  /*  add the "height in units" spinbutton to the sizeentry  */
  gtk_table_attach_defaults (GTK_TABLE (vals->size_se), spinbutton,
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
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (vals->size_se),
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
  gtk_table_attach_defaults (GTK_TABLE (vals->size_se), spinbutton,
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
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (vals->size_se),
                             GTK_SPIN_BUTTON (spinbutton),
			     GTK_SPIN_BUTTON (spinbutton2));

  /*  initialize the sizeentry  */
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (vals->size_se), 0,
				  vals->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (vals->size_se), 1,
				  vals->yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (vals->size_se), 0,
					 GIMP_MIN_IMAGE_SIZE,
					 GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (vals->size_se), 1,
					 GIMP_MIN_IMAGE_SIZE,
					 GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->size_se), 0, vals->width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->size_se), 1, vals->height);

  gtk_signal_connect (GTK_OBJECT (vals->size_se), "refval_changed",
		      (GtkSignalFunc) file_new_image_size_callback, vals);
  gtk_signal_connect (GTK_OBJECT (vals->size_se), "value_changed",
		      (GtkSignalFunc) file_new_image_size_callback, vals);

  /*  initialize the size label  */
  file_new_image_size_callback (vals->size_se, vals);

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

  vals->resolution_se =
  gimp_size_entry_new (1, default_resolution_units, _("pixels/%a"),
		       FALSE, FALSE, FALSE, 75,
		       GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
  gtk_table_set_col_spacing (GTK_TABLE (vals->resolution_se), 1, 2);
  gtk_table_set_col_spacing (GTK_TABLE (vals->resolution_se), 2, 2);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (vals->resolution_se),
			     GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (vals->resolution_se), spinbutton,
			     1, 2, 0, 1);
  gtk_widget_show (spinbutton);
  gtk_table_attach (GTK_TABLE (table), vals->resolution_se, 1, 2, 5, 7,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (vals->resolution_se);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (vals->resolution_se),
					 0, GIMP_MIN_RESOLUTION,
					 GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (vals->resolution_se),
					 1, GIMP_MIN_RESOLUTION,
					 GIMP_MAX_RESOLUTION);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->resolution_se),
			      0, vals->xresolution);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->resolution_se),
			      1, vals->yresolution);

  gtk_signal_connect (GTK_OBJECT (vals->resolution_se), "value_changed",
		      (GtkSignalFunc) file_new_resolution_callback, vals);

  /*  the resolution chainbutton  */
  vals->couple_resolutions = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active
    (GIMP_CHAIN_BUTTON (vals->couple_resolutions),
     ABS (vals->xresolution - vals->yresolution) < GIMP_MIN_RESOLUTION);
  gtk_table_attach_defaults (GTK_TABLE (vals->resolution_se),
			     vals->couple_resolutions, 2, 3, 0, 2);
  gtk_widget_show (vals->couple_resolutions);

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
  for (i = 0; i < ntypes; i++)
    {
      button = gtk_radio_button_new_with_label (group, gettext (type_names[i]));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, FALSE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) i);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_new_toggle_callback,
			  &vals->type);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_new_image_size_callback, vals);
      if (vals->type == i)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_widget_show (button);

      vals->type_w[i] = button;
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
  for (i = 0; i < nfill_types; i++)
    {
      button =
	gtk_radio_button_new_with_label (group, gettext (fill_type_names[i]));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) i);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_new_toggle_callback,
			  &vals->fill_type);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_new_image_size_callback, vals);
      if (vals->fill_type == i)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_widget_show (button);

      vals->fill_type_w[i] = button;
    }

  gimp_size_entry_grab_focus (GIMP_SIZE_ENTRY (vals->size_se));

  gtk_widget_show (vals->dlg);
}

void
file_new_reset_current_cut_buffer ()
{
  /* this function just changes the status of last_image_new
     so i can if theres been a cut/copy since the last file new */
  last_new_image = FALSE;
}
