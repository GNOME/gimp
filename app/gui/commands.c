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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include "appenv.h"
#include "about_dialog.h"
#include "actionarea.h"
#include "app_procs.h"
#include "brightness_contrast.h"
#include "brushes.h"
#include "by_color_select.h"
#include "channels_dialog.h"
#include "colormaps.h"
#include "color_balance.h"
#include "commands.h"
#include "convert.h"
#include "curves.h"
#include "desaturate.h"
#include "channel_ops.h"
#include "drawable.h"
#include "equalize.h"
#include "fileops.h"
#include "floating_sel.h"
#include "gdisplay_ops.h"
#include "general.h"
#include "gimage_cmds.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "global_edit.h"
#include "gradient.h"
#include "histogram_tool.h"
#include "hue_saturation.h"
#include "image_render.h"
#include "indexed_palette.h"
#include "info_window.h"
#include "interface.h"
#include "invert.h"
#include "layers_dialog.h"
#include "layer_select.h"
#include "levels.h"
#include "palette.h"
#include "patterns.h"
#include "plug_in.h"
#include "posterize.h"
#include "resize.h"
#include "scale.h"
#include "threshold.h"
#include "tips_dialog.h"
#include "tools.h"
#include "undo.h"

/*  external functions  */
extern void layers_dialog_layer_merge_query (GImage *, int);

typedef struct {
  GtkWidget *dlg;
  GtkWidget *height_entry;
  GtkWidget *width_entry;
  int width;
  int height;
  int type;
  int fill_type;
} NewImageValues;

typedef struct
{
  GtkWidget * shell;
  Resize *    resize;
  int         gimage_id;
} ImageResize;

/*  new image local functions  */
static void file_new_ok_callback (GtkWidget *, gpointer);
static void file_new_cancel_callback (GtkWidget *, gpointer);
static gint file_new_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void file_new_toggle_callback (GtkWidget *, gpointer);

/*  static variables  */
static   int          last_width = 256;
static   int          last_height = 256;
static   int          last_type = RGB;
static   int          last_fill_type = BACKGROUND_FILL;

/*  preferences local functions  */
static void file_prefs_ok_callback (GtkWidget *, GtkWidget *);
static void file_prefs_save_callback (GtkWidget *, GtkWidget *);
static void file_prefs_cancel_callback (GtkWidget *, GtkWidget *);
static gint file_prefs_delete_callback (GtkWidget *, GdkEvent *, GtkWidget *);
static void file_prefs_toggle_callback (GtkWidget *, gpointer);
static void file_prefs_text_callback (GtkWidget *, gpointer);
static void file_prefs_preview_size_callback (GtkWidget *, gpointer);

/*  static variables  */
static   GtkWidget   *prefs_dlg = NULL;
static   int          old_transparency_type;
static   int          old_transparency_size;
static   int          old_levels_of_undo;
static   int          old_marching_speed;
static   int          old_allow_resize_windows;
static   int          old_auto_save;
static   int          old_preview_size;
static   int          old_no_cursor_updating;
static   int          old_show_tool_tips;
static   int          old_cubic_interpolation;
static   int          old_confirm_on_close;
static   int          old_default_width;
static   int          old_default_height;
static   int          old_default_type;
static   int          new_dialog_run;
static   int          old_stingy_memory_use;
static   int          old_tile_cache_size;
static   int          old_install_cmap;
static   int          old_cycled_marching_ants;
static   char *       old_temp_path;
static   char *       old_swap_path;
static   char *       old_brush_path;
static   char *       old_pattern_path;
static   char *       old_palette_path;
static   char *       old_plug_in_path;
static   char *       old_gradient_path;

static   char *       edit_temp_path = NULL;
static   char *       edit_swap_path = NULL;
static   char *       edit_brush_path = NULL;
static   char *       edit_pattern_path = NULL;
static   char *       edit_palette_path = NULL;
static   char *       edit_plug_in_path = NULL;
static   char *       edit_gradient_path = NULL;
static   int          edit_stingy_memory_use;
static   int          edit_tile_cache_size;
static   int          edit_install_cmap;
static   int          edit_cycled_marching_ants;


/*  local functions  */
static void   image_resize_callback (GtkWidget *, gpointer);
static void   image_scale_callback (GtkWidget *, gpointer);
static void   image_cancel_callback (GtkWidget *, gpointer);
static gint   image_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void   gimage_mask_feather_callback (GtkWidget *, gpointer, gpointer);
static void   gimage_mask_border_callback (GtkWidget *, gpointer, gpointer);
static void   gimage_mask_grow_callback (GtkWidget *, gpointer, gpointer);
static void   gimage_mask_shrink_callback (GtkWidget *, gpointer, gpointer);

/*  variables declared in gimage_mask.c--we need them to set up
 *  initial values for the various dialog boxes which query for values
 */
extern double gimage_mask_feather_radius;
extern int    gimage_mask_border_radius;
extern int    gimage_mask_grow_pixels;
extern int    gimage_mask_shrink_pixels;


static void
file_new_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  NewImageValues *vals;
  GImage *gimage;
  GDisplay *gdisplay;
  Layer *layer;
  int type;

  vals = data;

  vals->width = atoi (gtk_entry_get_text (GTK_ENTRY (vals->width_entry)));
  vals->height = atoi (gtk_entry_get_text (GTK_ENTRY (vals->height_entry)));

  gtk_widget_destroy (vals->dlg);

  last_width = vals->width;
  last_height = vals->height;
  last_type = vals->type;
  last_fill_type = vals->fill_type;

  switch (vals->fill_type)
    {
    case BACKGROUND_FILL:
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

  /*  Make the background (or first) layer  */
  layer = layer_new (gimage->ID, gimage->width, gimage->height,
		     type, "Background", OPAQUE_OPACITY, NORMAL);

  if (layer) {
    /*  add the new layer to the gimage  */
    gimage_disable_undo (gimage);
    gimage_add_layer (gimage, layer, 0);
    gimage_enable_undo (gimage);
    
    drawable_fill (GIMP_DRAWABLE(layer), vals->fill_type);

    gimage_clean_all (gimage);
    
    gdisplay = gdisplay_new (gimage, 0x0101);
  }

  g_free (vals);
}

static gint
file_new_delete_callback (GtkWidget *widget,
			  GdkEvent *event,
			  gpointer data)
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

static void
file_new_toggle_callback (GtkWidget *widget,
			  gpointer   data)
{
  int *val;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      val = data;
      *val = (long) gtk_object_get_user_data (GTK_OBJECT (widget));
    }
}

void
file_new_cmd_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  GDisplay *gdisp;
  NewImageValues *vals;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *radio_box;
  GSList *group;
  char buffer[32];

  if(!new_dialog_run)
    {
      last_width = default_width;
      last_height = default_height;
      last_type = default_type;
      new_dialog_run = 1;  
    }

  /*  Before we try to determine the responsible gdisplay,
   *  make sure this wasn't called from the toolbox
   */
  if ((long) client_data)
    gdisp = gdisplay_active ();
  else
    gdisp = NULL;

  vals = g_malloc (sizeof (NewImageValues));
  vals->fill_type = last_fill_type;

  if (gdisp)
    {
      vals->width = gdisp->gimage->width;
      vals->height = gdisp->gimage->height;
      vals->type = gimage_base_type (gdisp->gimage);
    }
  else
    {
      vals->width = last_width;
      vals->height = last_height;
      vals->type = last_type;
    }

  if (vals->type == INDEXED)
    vals->type = RGB;    /* no indexed images */

  vals->dlg = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (vals->dlg), "new_image", "Gimp");
  gtk_window_set_title (GTK_WINDOW (vals->dlg), "New Image");
  gtk_window_position (GTK_WINDOW (vals->dlg), GTK_WIN_POS_MOUSE);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (vals->dlg), "delete_event",
		      GTK_SIGNAL_FUNC (file_new_delete_callback),
		      vals);

  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (vals->dlg)->action_area), 2);

  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) file_new_ok_callback,
                      vals);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) file_new_cancel_callback,
                      vals);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_show (button);


  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->vbox),
		      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  label = gtk_label_new ("Width:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new ("Height:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  vals->width_entry = gtk_entry_new ();
  gtk_widget_set_usize (vals->width_entry, 75, 0);
  sprintf (buffer, "%d", vals->width);
  gtk_entry_set_text (GTK_ENTRY (vals->width_entry), buffer);
  gtk_table_attach (GTK_TABLE (table), vals->width_entry, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (vals->width_entry);

  vals->height_entry = gtk_entry_new ();
  gtk_widget_set_usize (vals->height_entry, 75, 0);
  sprintf (buffer, "%d", vals->height);
  gtk_entry_set_text (GTK_ENTRY (vals->height_entry), buffer);
  gtk_table_attach (GTK_TABLE (table), vals->height_entry, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (vals->height_entry);


  frame = gtk_frame_new ("Image Type");
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  button = gtk_radio_button_new_with_label (NULL, "RGB");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) RGB);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->type);
  if (vals->type == RGB)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, "Grayscale");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) GRAY);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->type);
  if (vals->type == GRAY)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);


  frame = gtk_frame_new ("Fill Type");
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  button = gtk_radio_button_new_with_label (NULL, "Background");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) BACKGROUND_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == BACKGROUND_FILL)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, "White");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) WHITE_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == WHITE_FILL)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, "Transparent");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) TRANSPARENT_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == TRANSPARENT_FILL)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  gtk_widget_show (vals->dlg);
}

void
file_open_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  file_open_callback (widget, client_data);
}

void
file_save_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  file_save_callback (widget, client_data);
}

void
file_save_as_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  file_save_as_callback (widget, client_data);
}


/* Some information regarding preferences, compiled by Raph Levien 11/3/97.

   The following preference items cannot be set on the fly (at least
   according to the existing pref code - it may be that changing them
   so they're set on the fly is not hard).

   temp-path
   swap-path
   brush-path
   pattern-path
   plug-in-path
   palette-path
   gradient-path
   stingy-memory-use
   tile-cache-size
   install-cmap
   cycled-marching-ants

   All of these now have variables of the form edit_temp_path, which
   are copied from the actual variables (e.g. temp_path) the first time
   the dialog box is started.

   Variables of the form old_temp_path represent the values at the
   time the dialog is opened - a cancel copies them back from old to
   the real variables or the edit variables, depending on whether they
   can be set on the fly.

   Here are the remaining issues as I see them:

   Still no settings for default-brush, default-gradient,
   default-palette, default-pattern, gamma-correction, color-cube,
   show-rulers, ruler-units. No widget for confirm-on-close although
   a lot of stuff is there.

   No UI feedback for the fact that some settings won't take effect
   until the next Gimp restart.

   The semantics of "save" are a little funny - it only saves the
   settings that are different from the way they were when the dialog
   was opened. So you can set something, close the window, open it
   up again, click "save" and have nothing happen. To change this
   to more intuitive semantics, we should have a whole set of init_
   variables that are set the first time the dialog is opened (along
   with the edit_ variables that are currently set). Then, the save
   callback checks against the init_ variable rather than the old_.

   */

/* Copy the string from source to destination, freeing the string stored
   in the destination if there is one there already. */
static void
file_prefs_strset (char **dst, char *src)
{
  if (*dst != NULL)
    g_free (*dst);
  *dst = g_strdup (src);
}


/* Duplicate the string, but treat NULL as the empty string. */
static char *
file_prefs_strdup (char *src)
{
  return g_strdup (src == NULL ? "" : src);
}

/* Compare two strings, but treat NULL as the empty string. */
static int
file_prefs_strcmp (char *src1, char *src2)
{
  return strcmp (src1 == NULL ? "" : src1,
		   src2 == NULL ? "" : src2);
}

static void
file_prefs_ok_callback (GtkWidget *widget,
			GtkWidget *dlg)
{

  if (levels_of_undo < 0) 
    {
      message_box("Error: Levels of undo must be zero or greater.", 
		  NULL, NULL);
      levels_of_undo = old_levels_of_undo;
      return;
    }
  if (marching_speed < 50)
    {
      message_box("Error: Marching speed must be 50 or greater.",
		  NULL, NULL);
      marching_speed = old_marching_speed;
      return;
    }
  if (default_width < 1)
    {
      message_box("Error: Default width must be one or greater.",
		  NULL, NULL);
      default_width = old_default_width;
      return;
    }
  if (default_height < 1)
    {
      message_box("Error: Default height must be one or greater.",
		  NULL, NULL);
      default_height = old_default_height;
      return;
    }  
      
  gtk_widget_destroy (dlg);
  prefs_dlg = NULL;

  if (show_tool_tips)
    gtk_tooltips_enable (tool_tips);
  else
    gtk_tooltips_disable (tool_tips);
}

static void
file_prefs_save_callback (GtkWidget *widget,
			  GtkWidget *dlg)
{
  GList *update = NULL; /* options that should be updated in .gimprc */
  GList *remove = NULL; /* options that should be commented out */
  int save_stingy_memory_use;
  int save_tile_cache_size;
  int save_install_cmap;
  int save_cycled_marching_ants;
  gchar *save_temp_path;
  gchar *save_swap_path;
  gchar *save_brush_path;
  gchar *save_pattern_path;
  gchar *save_palette_path;
  gchar *save_plug_in_path;
  gchar *save_gradient_path;

  file_prefs_ok_callback (widget, dlg);

  /* Save variables so that we can restore them later */
  save_stingy_memory_use = stingy_memory_use;
  save_tile_cache_size = tile_cache_size;
  save_install_cmap = install_cmap;
  save_cycled_marching_ants = cycled_marching_ants;
  save_temp_path = temp_path;
  save_swap_path = swap_path;
  save_brush_path = brush_path;
  save_pattern_path = pattern_path;
  save_palette_path = palette_path;
  save_plug_in_path = plug_in_path;
  save_gradient_path = gradient_path;

  if (levels_of_undo != old_levels_of_undo)
    update = g_list_append (update, "undo-levels");
  if (marching_speed != old_marching_speed)
    update = g_list_append (update, "marching-ants-speed");
  if (allow_resize_windows != old_allow_resize_windows)
    update = g_list_append (update, "allow-resize-windows");
  if (auto_save != old_auto_save)
    {
      update = g_list_append (update, "auto-save");
      remove = g_list_append (remove, "dont-auto-save");
    }
  if (no_cursor_updating != old_no_cursor_updating)
    {
      update = g_list_append (update, "cursor-updating");
      remove = g_list_append (remove, "no-cursor-updating");
    }
  if (show_tool_tips != old_show_tool_tips)
    {
      update = g_list_append (update, "show-tool-tips");
      remove = g_list_append (remove, "dont-show-tool-tips");
    }
  if (cubic_interpolation != old_cubic_interpolation)
    update = g_list_append (update, "cubic-interpolation");
  if (confirm_on_close != old_confirm_on_close)
    update = g_list_append (update, "confirm-on-close");
  if (default_width != old_default_width ||
      default_height != old_default_height)
    update = g_list_append (update, "default-image-size");
  if (default_type != old_default_type)
    update = g_list_append (update, "default-image-type");
  if (preview_size != old_preview_size)
    update = g_list_append (update, "preview-size");
  if (transparency_type != old_transparency_type)
    update = g_list_append (update, "transparency-type");
  if (transparency_size != old_transparency_size)
    update = g_list_append (update, "transparency-size");
  if (edit_stingy_memory_use != stingy_memory_use)
    {
      update = g_list_append (update, "stingy-memory-use");
      stingy_memory_use = edit_stingy_memory_use;
    }
  if (edit_tile_cache_size != tile_cache_size)
    {
      update = g_list_append (update, "tile-cache-size");
      tile_cache_size = edit_tile_cache_size;
    }
  if (edit_install_cmap != install_cmap)
    {
      update = g_list_append (update, "install-colormap");
      install_cmap = edit_install_cmap;
    }
  if (edit_cycled_marching_ants != cycled_marching_ants)
    {
      update = g_list_append (update, "colormap-cycling");
      cycled_marching_ants = edit_cycled_marching_ants;
    }
  if (file_prefs_strcmp (temp_path, edit_temp_path))
    {
      update = g_list_append (update, "temp-path");
      temp_path = edit_temp_path;
    }
  if (file_prefs_strcmp (swap_path, edit_swap_path))
    {
      update = g_list_append (update, "swap-path");
      swap_path = edit_swap_path;
    }
  if (file_prefs_strcmp (brush_path, edit_brush_path))
    {
      update = g_list_append (update, "brush-path");
      brush_path = edit_brush_path;
    }
  if (file_prefs_strcmp (pattern_path, edit_pattern_path))
    {
      update = g_list_append (update, "pattern-path");
      pattern_path = edit_pattern_path;
    }
  if (file_prefs_strcmp (palette_path, edit_palette_path))
    {
      update = g_list_append (update, "palette-path");
      palette_path = edit_palette_path;
    }
  if (file_prefs_strcmp (plug_in_path, edit_plug_in_path))
    {
      update = g_list_append (update, "plug-in-path");
      plug_in_path = edit_plug_in_path;
    }
  if (file_prefs_strcmp (gradient_path, edit_gradient_path))
    {
      update = g_list_append (update, "gradient-path");
      gradient_path = edit_gradient_path;
    }
  save_gimprc (&update, &remove);

  /* Restore variables which must not change */
  stingy_memory_use = save_stingy_memory_use;
  tile_cache_size = save_tile_cache_size;
  install_cmap = save_install_cmap;
  cycled_marching_ants = save_cycled_marching_ants;
  temp_path = save_temp_path;
  swap_path = save_swap_path;
  brush_path = save_brush_path;
  pattern_path = save_pattern_path;
  palette_path = save_palette_path;
  plug_in_path = save_plug_in_path;
  gradient_path = save_gradient_path;
  
  g_list_free (update);
  g_list_free (remove);
}

static int
file_prefs_delete_callback (GtkWidget *widget,
			    GdkEvent *event,
			    GtkWidget *dlg) 
{
  file_prefs_cancel_callback (widget, dlg);

  /* the widget is already destroyed here no need to try again */
  return TRUE;
}

static void
file_prefs_cancel_callback (GtkWidget *widget,
			    GtkWidget *dlg)
{
  gtk_widget_destroy (dlg);
  prefs_dlg = NULL;

  levels_of_undo = old_levels_of_undo;
  marching_speed = old_marching_speed;
  allow_resize_windows = old_allow_resize_windows;
  auto_save = old_auto_save;
  no_cursor_updating = old_no_cursor_updating;
  show_tool_tips = old_show_tool_tips;
  cubic_interpolation = old_cubic_interpolation;
  confirm_on_close = old_confirm_on_close;
  default_width = old_default_width;
  default_height = old_default_height;
  default_type = old_default_type;
  if (preview_size != old_preview_size)
    {
      lc_dialog_rebuild (old_preview_size);
      layer_select_update_preview_size ();
    }

  if ((transparency_type != old_transparency_type) ||
      (transparency_size != old_transparency_size))
    {
      transparency_type = old_transparency_type;
      transparency_size = old_transparency_size;

      render_setup (transparency_type, transparency_size);
      layer_invalidate_previews (-1);
      gimage_invalidate_previews ();
      gdisplays_expose_full ();
      gdisplays_flush ();
    }

  edit_stingy_memory_use = old_stingy_memory_use;
  edit_tile_cache_size = old_tile_cache_size;
  edit_install_cmap = old_install_cmap;
  edit_cycled_marching_ants = old_cycled_marching_ants;
  file_prefs_strset (&edit_temp_path, old_temp_path);
  file_prefs_strset (&edit_swap_path, old_swap_path);
  file_prefs_strset (&edit_brush_path, old_brush_path);
  file_prefs_strset (&edit_pattern_path, old_pattern_path);
  file_prefs_strset (&edit_palette_path, old_palette_path);
  file_prefs_strset (&edit_plug_in_path, old_plug_in_path);
  file_prefs_strset (&edit_gradient_path, old_gradient_path);
}

static void
file_prefs_toggle_callback (GtkWidget *widget,
			    gpointer   data)
{
  int *val;

  if (data==&allow_resize_windows)
    allow_resize_windows = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&auto_save)
    auto_save = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&no_cursor_updating)
    no_cursor_updating = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&show_tool_tips)
    show_tool_tips = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&cubic_interpolation)
    cubic_interpolation = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&confirm_on_close)
    confirm_on_close = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&old_stingy_memory_use)
    old_stingy_memory_use = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&old_install_cmap)
    old_install_cmap = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&old_cycled_marching_ants)
    old_cycled_marching_ants = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&default_type)
    {
      default_type = (long) gtk_object_get_user_data (GTK_OBJECT (widget));
    } 
  else if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      val = data;
      *val = (long) gtk_object_get_user_data (GTK_OBJECT (widget));
      render_setup (transparency_type, transparency_size);
      layer_invalidate_previews (-1);
      gimage_invalidate_previews ();
      gdisplays_expose_full ();
      gdisplays_flush ();
    }
}

static void
file_prefs_preview_size_callback (GtkWidget *widget,
                                  gpointer   data)
{
  lc_dialog_rebuild ((long)data);
  layer_select_update_preview_size ();
}

static void
file_prefs_text_callback (GtkWidget *widget,
			  gpointer   data)
{
  int *val;

  val = data;
  *val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void
file_prefs_string_callback (GtkWidget *widget,
			    gpointer   data)
{
  gchar **val;

  val = data;
  file_prefs_strset (val, gtk_entry_get_text (GTK_ENTRY (widget)));
}

void
file_pref_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *out_frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *abox;
  GtkWidget *label;
  GtkWidget *radio_box;
  GtkWidget *entry;
  GtkWidget *menu;
  GtkWidget *menuitem;
  GtkWidget *optionmenu;
  GtkWidget *notebook;
  GtkWidget *table;
  GSList *group;
  char buffer[32];
  char *transparencies[] =
  {
    "Light Checks",
    "Mid-Tone Checks",
    "Dark Checks",
    "White Only",
    "Gray Only",
    "Black Only",
  };
  char *checks[] =
  {
    "Small Checks",
    "Medium Checks",
    "Large Checks",
  };
  int transparency_vals[] =
  {
    LIGHT_CHECKS,
    GRAY_CHECKS,
    DARK_CHECKS,
    WHITE_ONLY,
    GRAY_ONLY,
    BLACK_ONLY,
  };
  int check_vals[] =
  {
    SMALL_CHECKS,
    MEDIUM_CHECKS,
    LARGE_CHECKS,
  };
  struct {
    char *label;
    char **mpath;
  } dirs[] =
    {
      {"Temp dir:", &edit_temp_path},
      {"Swap dir:", &edit_swap_path},
      {"Brushes dir:", &edit_brush_path},
      {"Gradients dir:", &edit_gradient_path},
      {"Patterns dir:", &edit_pattern_path},
      {"Palette dir:", &edit_palette_path},
      {"Plug-in dir:", &edit_plug_in_path}
    };
    struct {
      char *label;
      int size;
    } preview_sizes[] =
    {
      {"None",0},
      {"Small",32},
      {"Medium",64},
      {"Large",128}
    }; 
  int ntransparencies = sizeof (transparencies) / sizeof (transparencies[0]);
  int nchecks = sizeof (checks) / sizeof (checks[0]);
  int ndirs = sizeof(dirs) / sizeof (dirs[0]);
  int npreview_sizes = sizeof(preview_sizes) / sizeof (preview_sizes[0]); 
  int i;

  if (!prefs_dlg)
    {
      if (edit_temp_path == NULL)
	{
	  /* first time dialog is opened - copy config vals to edit
             variables. */
	  edit_temp_path = file_prefs_strdup (temp_path);	
	  edit_swap_path = file_prefs_strdup (swap_path);
	  edit_brush_path = file_prefs_strdup (brush_path);
	  edit_pattern_path = file_prefs_strdup (pattern_path);
	  edit_palette_path = file_prefs_strdup (palette_path);
	  edit_plug_in_path = file_prefs_strdup (plug_in_path);
	  edit_gradient_path = file_prefs_strdup (gradient_path);
	  edit_stingy_memory_use = stingy_memory_use;
	  edit_tile_cache_size = tile_cache_size;
	  edit_install_cmap = install_cmap;
	  edit_cycled_marching_ants = cycled_marching_ants;
	}
      old_transparency_type = transparency_type;
      old_transparency_size = transparency_size;
      old_levels_of_undo = levels_of_undo;
      old_marching_speed = marching_speed;
      old_allow_resize_windows = allow_resize_windows;
      old_auto_save = auto_save;
      old_preview_size = preview_size;
      old_no_cursor_updating = no_cursor_updating;
      old_show_tool_tips = show_tool_tips;
      old_cubic_interpolation = cubic_interpolation;
      old_confirm_on_close = confirm_on_close;
      old_default_width = default_width;
      old_default_height = default_height;
      old_default_type = default_type;
      old_stingy_memory_use = edit_stingy_memory_use;
      old_tile_cache_size = edit_tile_cache_size;
      old_install_cmap = edit_install_cmap;
      old_cycled_marching_ants = edit_cycled_marching_ants;
      file_prefs_strset (&old_temp_path, edit_temp_path);
      file_prefs_strset (&old_swap_path, edit_swap_path);
      file_prefs_strset (&old_brush_path, edit_brush_path);
      file_prefs_strset (&old_pattern_path, edit_pattern_path);
      file_prefs_strset (&old_palette_path, edit_palette_path);
      file_prefs_strset (&old_plug_in_path, edit_plug_in_path);
      file_prefs_strset (&old_gradient_path, edit_gradient_path);

      prefs_dlg = gtk_dialog_new ();
      gtk_window_set_wmclass (GTK_WINDOW (prefs_dlg), "preferences", "Gimp");
      gtk_window_set_title (GTK_WINDOW (prefs_dlg), "Preferences");

      /* handle the wm close signal */
      gtk_signal_connect (GTK_OBJECT (prefs_dlg), "delete_event",
			  GTK_SIGNAL_FUNC (file_prefs_delete_callback),
			  prefs_dlg);

      gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (prefs_dlg)->action_area), 2);
      
      /* Action area */
      button = gtk_button_new_with_label ("OK");
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) file_prefs_ok_callback,
			  prefs_dlg);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dlg)->action_area),
			  button, TRUE, TRUE, 0);
      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      button = gtk_button_new_with_label ("Save");
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) file_prefs_save_callback,
			  prefs_dlg);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dlg)->action_area),
			  button, TRUE, TRUE, 0);
      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      button = gtk_button_new_with_label ("Cancel");
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) file_prefs_cancel_callback,
			  prefs_dlg);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dlg)->action_area),
			  button, TRUE, TRUE, 0);
      gtk_widget_show (button);

      notebook = gtk_notebook_new ();
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dlg)->vbox),
			  notebook, TRUE, TRUE, 0);

      /* Display page */
      out_frame = gtk_frame_new ("Display settings");
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      frame = gtk_frame_new ("Image size"); 
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      abox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (abox), 1);
      gtk_container_add (GTK_CONTAINER (frame), abox);
      gtk_widget_show (abox);

      table = gtk_table_new (2, 2, FALSE);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_box_pack_start (GTK_BOX (abox), table, TRUE, TRUE, 0);
      gtk_widget_show (table);
      
      label = gtk_label_new ("Width:");
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new ("Height:");
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
      
      entry = gtk_entry_new ();
      gtk_widget_set_usize (entry, 25, 0);
      sprintf (buffer, "%d", default_width);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
                       GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_signal_connect (GTK_OBJECT (entry), "changed",
                          (GtkSignalFunc) file_prefs_text_callback,
                          &default_width);
      gtk_widget_show (entry);

      entry = gtk_entry_new ();
      gtk_widget_set_usize (entry, 25, 0);
      sprintf (buffer, "%d", default_height);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer); 
      gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_signal_connect (GTK_OBJECT (entry), "changed",
			  (GtkSignalFunc) file_prefs_text_callback,
                          &default_height);
      gtk_widget_show (entry);

      frame = gtk_frame_new ("Image type");
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      radio_box = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
      gtk_container_add (GTK_CONTAINER (frame), radio_box);
      gtk_widget_show (radio_box);

      button = gtk_radio_button_new_with_label (NULL, "RGB");
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) RGB);
      if (default_type == RGB)
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &default_type);
      gtk_widget_show (button);
      button = gtk_radio_button_new_with_label (group, "Grayscale");
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) GRAY);
      if (last_type == GRAY) 
	  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                         (GtkSignalFunc) file_prefs_toggle_callback,
			  &default_type);
      gtk_widget_show (button);
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);
      
      label = gtk_label_new ("Preview size:");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
      
      menu = gtk_menu_new ();
      for (i = 0; i < npreview_sizes; i++)
        {
          menuitem = gtk_menu_item_new_with_label (preview_sizes[i].label);
	  gtk_menu_append (GTK_MENU (menu), menuitem);
	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			      (GtkSignalFunc) file_prefs_preview_size_callback,
			      (gpointer)((long)preview_sizes[i].size));
	  gtk_widget_show (menuitem);
	}
      optionmenu = gtk_option_menu_new ();
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
      gtk_box_pack_start (GTK_BOX (hbox), optionmenu, TRUE, TRUE, 0);
      gtk_widget_show (optionmenu);
      for (i = 0; i < npreview_sizes; i++)
	if (preview_size==preview_sizes[i].size)
	  gtk_option_menu_set_history(GTK_OPTION_MENU (optionmenu),i);
	  
      button = gtk_check_button_new_with_label("Cubic interpolation");
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   cubic_interpolation);
      gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &cubic_interpolation);
      gtk_widget_show (button);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      frame = gtk_frame_new ("Transparency Type");
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      radio_box = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
      gtk_container_add (GTK_CONTAINER (frame), radio_box);
      gtk_widget_show (radio_box);

      group = NULL;
      for (i = 0; i < ntransparencies; i++)
        {
	  button = gtk_radio_button_new_with_label (group, transparencies[i]);
          group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
          gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
          gtk_object_set_user_data (GTK_OBJECT (button),
				    (gpointer) ((long) transparency_vals[i]));
          if (transparency_vals[i] == transparency_type)
            gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
          gtk_signal_connect (GTK_OBJECT (button), "toggled",
                              (GtkSignalFunc) file_prefs_toggle_callback,
                              &transparency_type);
          gtk_widget_show (button);
        }
      frame = gtk_frame_new ("Check Size");
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      radio_box = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
      gtk_container_add (GTK_CONTAINER (frame), radio_box);
      gtk_widget_show (radio_box);

      group = NULL;
      for (i = 0; i < nchecks; i++)
        {
          button = gtk_radio_button_new_with_label (group, checks[i]);
          group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
          gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
          gtk_object_set_user_data (GTK_OBJECT (button),
				    (gpointer) ((long) check_vals[i]));
          if (check_vals[i] == transparency_size)
            gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
          gtk_signal_connect (GTK_OBJECT (button), "toggled",
                              (GtkSignalFunc) file_prefs_toggle_callback,
                              &transparency_size);
          gtk_widget_show (button);
        }
      

      label = gtk_label_new ("Display");
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      /* Interface */
      out_frame = gtk_frame_new ("Interface settings");
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);
      
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Levels of undo:");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      entry = gtk_entry_new ();
      gtk_widget_set_usize (entry, 75, 0);
      sprintf (buffer, "%d", levels_of_undo);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (entry), "changed",
                          (GtkSignalFunc) file_prefs_text_callback,
                          &levels_of_undo);
      gtk_widget_show (entry);
      
      button = gtk_check_button_new_with_label("Resize window on zoom");
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   allow_resize_windows);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &allow_resize_windows);
      gtk_widget_show (button);
      
      /* Don't show the Auto-save button until we really 
	 have auto-saving in the gimp.
      
	 button = gtk_check_button_new_with_label("Auto save");
	 gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   auto_save);
	 gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	 gtk_signal_connect (GTK_OBJECT (button), "toggled",
	                           (GtkSignalFunc) file_prefs_toggle_callback,
                                   &auto_save);
         gtk_widget_show (button);
      */

      button = gtk_check_button_new_with_label("Disable cursor updating");
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   no_cursor_updating);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &no_cursor_updating);
      gtk_widget_show (button);

      button = gtk_check_button_new_with_label("Show tool tips");
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   show_tool_tips);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &show_tool_tips);
      gtk_widget_show (button);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Marching ants speed:");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      entry = gtk_entry_new ();
      gtk_widget_set_usize (entry, 75, 0);
      sprintf (buffer, "%d", marching_speed);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (entry), "changed",
                          (GtkSignalFunc) file_prefs_text_callback,
                          &marching_speed);
      gtk_widget_show (entry);      

      label = gtk_label_new ("Interface");
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      /* Environment */
      out_frame = gtk_frame_new ("Environment settings");
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_set_usize (out_frame, 320, 200);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);
      
      button = gtk_check_button_new_with_label("Conservative memory usage");
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   stingy_memory_use);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &old_stingy_memory_use);
      gtk_widget_show (button);
      
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Tile cache size (bytes):");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      entry = gtk_entry_new ();
      gtk_widget_set_usize (entry, 75, 0);
      sprintf (buffer, "%d", old_tile_cache_size);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (entry), "changed",
                          (GtkSignalFunc) file_prefs_text_callback,
                          &old_tile_cache_size);
      gtk_widget_show (entry);
      
      button = gtk_check_button_new_with_label("Install colormap (8-bit only)");
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
				   install_cmap);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &old_install_cmap);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      button = gtk_check_button_new_with_label("Colormap cycling (8-bit only)");
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   cycled_marching_ants);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &old_cycled_marching_ants);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
      
      label = gtk_label_new ("Environment");
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      /* Directories */
      out_frame = gtk_frame_new ("Directories settings");
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_set_usize (out_frame, 320, 200);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);

      table = gtk_table_new (ndirs+1, 2, FALSE);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
      gtk_widget_show (table);

      for (i = 0; i < ndirs; i++)
	{
	  label = gtk_label_new (dirs[i].label);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_table_attach (GTK_TABLE (table), label, 0, 1, i, i+1,
                            GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);
 
          entry = gtk_entry_new ();
          gtk_widget_set_usize (entry, 25, 0);
          gtk_entry_set_text (GTK_ENTRY (entry), *(dirs[i].mpath));
	  gtk_signal_connect (GTK_OBJECT (entry), "changed",
			      (GtkSignalFunc) file_prefs_string_callback,
			      dirs[i].mpath);
          gtk_table_attach (GTK_TABLE (table), entry, 1, 2, i, i+1,
                            GTK_EXPAND | GTK_FILL, 0, 0, 0);
          gtk_widget_show (entry); 
	}

      label = gtk_label_new ("Directories");
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      gtk_widget_show (notebook);

      gtk_widget_show (prefs_dlg);
    }
}

void
file_close_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  gdisplay_close_window (gdisp, FALSE);
}

void
file_quit_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  app_exit (0);
}

void
edit_cut_cmd_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  global_edit_cut (gdisp);
}

void
edit_copy_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  global_edit_copy (gdisp);
}

void
edit_paste_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  global_edit_paste (gdisp, 0);
}

void
edit_paste_into_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  global_edit_paste (gdisp, 1);
}

void
edit_clear_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  edit_clear (gdisp->gimage, gimage_active_drawable (gdisp->gimage));

  gdisplays_flush ();
}

void
edit_fill_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  edit_fill (gdisp->gimage, gimage_active_drawable (gdisp->gimage));

  gdisplays_flush ();
}

void
edit_stroke_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_stroke (gdisp->gimage, gimage_active_drawable (gdisp->gimage));

  gdisplays_flush ();
}

void
edit_undo_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  undo_pop (gdisp->gimage);
}

void
edit_redo_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  undo_redo (gdisp->gimage);
}

void
edit_named_cut_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  named_edit_cut (gdisp);
}

void
edit_named_copy_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  named_edit_copy (gdisp);
}

void
edit_named_paste_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  named_edit_paste (gdisp);
}

void
select_toggle_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  selection_hide (gdisp->select, (void *) gdisp);
  gdisplays_flush ();
}

void
select_invert_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_invert (gdisp->gimage);
  gdisplays_flush ();
}

void
select_all_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_all (gdisp->gimage);
  gdisplays_flush ();
}

void
select_none_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_none (gdisp->gimage);
  gdisplays_flush ();
}

void
select_float_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_float (gdisp->gimage, gimage_active_drawable (gdisp->gimage), 0, 0);
  gdisplays_flush ();
}

void
select_sharpen_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_sharpen (gdisp->gimage);
  gdisplays_flush ();
}

void
select_border_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;
  char initial[16];

  gdisp = gdisplay_active ();

  sprintf (initial, "%d", gimage_mask_border_radius);
  query_string_box ("Border Selection", "Border selection by:", initial,
		    gimage_mask_border_callback, (gpointer) gdisp->gimage->ID);
}

void
select_feather_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;
  char initial[16];

  gdisp = gdisplay_active ();

  sprintf (initial, "%f", gimage_mask_feather_radius);
  query_string_box ("Feather Selection", "Feather selection by:", initial,
		    gimage_mask_feather_callback, (gpointer) gdisp->gimage->ID);
}

void
select_grow_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;
  char initial[16];

  gdisp = gdisplay_active ();

  sprintf (initial, "%d", gimage_mask_grow_pixels);
  query_string_box ("Grow Selection", "Grow selection by:", initial,
		    gimage_mask_grow_callback, (gpointer) gdisp->gimage->ID);
}

void
select_shrink_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;
  char initial[16];

  gdisp = gdisplay_active ();

  sprintf (initial, "%d", gimage_mask_shrink_pixels);
  query_string_box ("Shrink Selection", "Shrink selection by:", initial,
		    gimage_mask_shrink_callback, (gpointer) gdisp->gimage->ID);
}

void
select_by_color_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  gtk_widget_activate (tool_widgets[tool_info[(int) BY_COLOR_SELECT].toolbar_position]);
  by_color_select_initialize ((void *) gdisp->gimage);
}

void
select_save_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  gimage_mask_save (gdisp->gimage);
  gdisplays_flush ();
}

void
view_zoomin_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  change_scale (gdisp, ZOOMIN);
}

void
view_zoomout_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  change_scale (gdisp, ZOOMOUT);
}

static void
view_zoom_val (GtkWidget *widget,
	       gpointer   client_data,
	       int        val)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  change_scale (gdisp, val);
}

void
view_zoom_16_1_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  view_zoom_val (widget, client_data, 1601);
}

void
view_zoom_8_1_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 801);
}

void
view_zoom_4_1_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 401);
}

void
view_zoom_2_1_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 201);
}

void
view_zoom_1_1_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 101);
}

void
view_zoom_1_2_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 102);
}

void
view_zoom_1_4_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 104);
}

void
view_zoom_1_8_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 108);
}

void
view_zoom_1_16_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  view_zoom_val (widget, client_data, 116);
}

void
view_window_info_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  if (! gdisp->window_info_dialog)
    gdisp->window_info_dialog = info_window_create ((void *) gdisp);

  info_dialog_popup (gdisp->window_info_dialog);
}

void
view_toggle_rulers_cmd_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  /* This routine use promiscuous knowledge of gtk internals
   *  in order to hide and show the rulers "smoothly". This
   *  is kludgy and a hack and may break if gtk is changed
   *  internally.
   */

  if (!GTK_CHECK_MENU_ITEM (widget)->active)
    {
      if (GTK_WIDGET_VISIBLE (gdisp->origin))
	{
	  gtk_widget_unmap (gdisp->origin);
	  gtk_widget_unmap (gdisp->hrule);
	  gtk_widget_unmap (gdisp->vrule);

	  GTK_WIDGET_UNSET_FLAGS (gdisp->origin, GTK_VISIBLE);
	  GTK_WIDGET_UNSET_FLAGS (gdisp->hrule, GTK_VISIBLE);
	  GTK_WIDGET_UNSET_FLAGS (gdisp->vrule, GTK_VISIBLE);

	  gtk_widget_queue_resize (GTK_WIDGET (gdisp->origin->parent));
	}
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (gdisp->origin))
	{
	  gtk_widget_map (gdisp->origin);
	  gtk_widget_map (gdisp->hrule);
	  gtk_widget_map (gdisp->vrule);

	  GTK_WIDGET_SET_FLAGS (gdisp->origin, GTK_VISIBLE);
	  GTK_WIDGET_SET_FLAGS (gdisp->hrule, GTK_VISIBLE);
	  GTK_WIDGET_SET_FLAGS (gdisp->vrule, GTK_VISIBLE);

	  gtk_widget_queue_resize (GTK_WIDGET (gdisp->origin->parent));
	}
    }
}

void
view_toggle_guides_cmd_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay * gdisp;
  int old_val;

  gdisp = gdisplay_active ();

  old_val = gdisp->draw_guides;
  gdisp->draw_guides = GTK_CHECK_MENU_ITEM (widget)->active;

  if ((old_val != gdisp->draw_guides) && gdisp->gimage->guides)
    {
      gdisplay_expose_full (gdisp);
      gdisplays_flush ();
    }
}

void
view_snap_to_guides_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gdisp->snap_to_guides = GTK_CHECK_MENU_ITEM (widget)->active;
}

void
view_new_view_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  gdisplay_new_view (gdisp);
}

void
view_shrink_wrap_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  if (gdisp)
    shrink_wrap_display (gdisp);
}

void
image_equalize_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  image_equalize ((void *) gdisp->gimage);
  gdisplays_flush ();
}

void
image_invert_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  image_invert ((void *) gdisp->gimage);
  gdisplays_flush ();
}

void
image_posterize_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  gtk_widget_activate (tool_widgets[tool_info[(int) POSTERIZE].toolbar_position]);
  posterize_initialize ((void *) gdisp);
}

void
image_threshold_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  gtk_widget_activate (tool_widgets[tool_info[(int) THRESHOLD].toolbar_position]);
  threshold_initialize ((void *) gdisp);
}

void
image_color_balance_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  gtk_widget_activate (tool_widgets[tool_info[(int) COLOR_BALANCE].toolbar_position]);
  color_balance_initialize ((void *) gdisp);
}

void
image_brightness_contrast_cmd_callback (GtkWidget *widget,
					gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  gtk_widget_activate (tool_widgets[tool_info[(int) BRIGHTNESS_CONTRAST].toolbar_position]);
  brightness_contrast_initialize ((void *) gdisp);
}

void
image_hue_saturation_cmd_callback (GtkWidget *widget,
				   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  gtk_widget_activate (tool_widgets[tool_info[(int) HUE_SATURATION].toolbar_position]);
  hue_saturation_initialize ((void *) gdisp);
}

void
image_curves_cmd_callback (GtkWidget *widget,
			   gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  gtk_widget_activate (tool_widgets[tool_info[(int) CURVES].toolbar_position]);
  curves_initialize ((void *) gdisp);
}

void
image_levels_cmd_callback (GtkWidget *widget,
			   gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  gtk_widget_activate (tool_widgets[tool_info[(int) LEVELS].toolbar_position]);
  levels_initialize ((void *) gdisp);
}

void
image_desaturate_cmd_callback (GtkWidget *widget,
			       gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  image_desaturate ((void *) gdisp->gimage);
  gdisplays_flush ();
}

void
channel_ops_duplicate_cmd_callback (GtkWidget *widget,
				    gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  channel_ops_duplicate ((void *) gdisp->gimage);
}

void
channel_ops_offset_cmd_callback (GtkWidget *widget,
				 gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  channel_ops_offset ((void *) gdisp->gimage);
}

void
image_convert_rgb_cmd_callback (GtkWidget *widget,
				gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  convert_to_rgb ((void *) gdisp->gimage);
}

void
image_convert_grayscale_cmd_callback (GtkWidget *widget,
				      gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  convert_to_grayscale ((void *) gdisp->gimage);
}

void
image_convert_indexed_cmd_callback (GtkWidget *widget,
				    gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  convert_to_indexed ((void *) gdisp->gimage);
}

void
image_resize_cmd_callback (GtkWidget *widget,
			   gpointer client_data)
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", image_resize_callback, NULL, NULL },
    { "Cancel", image_cancel_callback, NULL, NULL }
  };
  GDisplay * gdisp;
  GtkWidget *vbox;
  ImageResize *image_resize;

  gdisp = gdisplay_active ();

  /*  the ImageResize structure  */
  image_resize = (ImageResize *) g_malloc (sizeof (ImageResize));
  image_resize->gimage_id = gdisp->gimage->ID;
  image_resize->resize = resize_widget_new (ResizeWidget, gdisp->gimage->width, gdisp->gimage->height);

  /*  the dialog  */
  image_resize->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (image_resize->shell), "image_resize", "Gimp");
  gtk_window_set_title (GTK_WINDOW (image_resize->shell), "Image Resize");
  gtk_window_set_policy (GTK_WINDOW (image_resize->shell), FALSE, FALSE, TRUE);
  gtk_window_position (GTK_WINDOW (image_resize->shell), GTK_WIN_POS_MOUSE);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (image_resize->shell), "delete_event",
		      GTK_SIGNAL_FUNC (image_delete_callback),
		      image_resize);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (image_resize->shell)->vbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), image_resize->resize->resize_widget, FALSE, FALSE, 0);

  action_items[0].user_data = image_resize;
  action_items[1].user_data = image_resize;
  build_action_area (GTK_DIALOG (image_resize->shell), action_items, 2, 0);

  gtk_widget_show (image_resize->resize->resize_widget);
  gtk_widget_show (vbox);
  gtk_widget_show (image_resize->shell);
}

void
image_scale_cmd_callback (GtkWidget *widget,
			  gpointer client_data)
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", image_scale_callback, NULL, NULL },
    { "Cancel", image_cancel_callback, NULL, NULL }
  };
  GDisplay * gdisp;
  GtkWidget *vbox;
  ImageResize *image_scale;

  gdisp = gdisplay_active ();

  /*  the ImageResize structure  */
  image_scale = (ImageResize *) g_malloc (sizeof (ImageResize));
  image_scale->gimage_id = gdisp->gimage->ID;
  image_scale->resize = resize_widget_new (ScaleWidget, gdisp->gimage->width, gdisp->gimage->height);

  /*  the dialog  */
  image_scale->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (image_scale->shell), "image_scale", "Gimp");
  gtk_window_set_title (GTK_WINDOW (image_scale->shell), "Image Scale");
  gtk_window_set_policy (GTK_WINDOW (image_scale->shell), FALSE, FALSE, TRUE);
  gtk_window_position (GTK_WINDOW (image_scale->shell), GTK_WIN_POS_MOUSE);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (image_scale->shell), "delete_event",
		      GTK_SIGNAL_FUNC (image_delete_callback),
		      image_scale);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (image_scale->shell)->vbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), image_scale->resize->resize_widget, FALSE, FALSE, 0);

  action_items[0].user_data = image_scale;
  action_items[1].user_data = image_scale;
  build_action_area (GTK_DIALOG (image_scale->shell), action_items, 2, 0);

  gtk_widget_show (image_scale->resize->resize_widget);
  gtk_widget_show (vbox);
  gtk_widget_show (image_scale->shell);
}

void
image_histogram_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  gtk_widget_activate (tool_widgets[tool_info[(int) HISTOGRAM].toolbar_position]);
  histogram_tool_initialize ((void *) gdisp);
}

void
layers_raise_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_raise_layer (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_lower_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_lower_layer (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}
  int value;

void
layers_anchor_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  floating_sel_anchor (gimage_get_active_layer (gdisp->gimage));
  gdisplays_flush ();
}

void
layers_merge_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  layers_dialog_layer_merge_query (gdisp->gimage, TRUE);
}

void
layers_flatten_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_flatten (gdisp->gimage);
  gdisplays_flush ();
}

void
layers_alpha_select_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_layer_alpha (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_mask_select_cmd_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_layer_mask (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
tools_default_colors_cmd_callback (GtkWidget *widget,
				   gpointer   client_data)
{
  palette_set_default_colors ();
}

void
tools_swap_colors_cmd_callback (GtkWidget *widget,
				gpointer   client_data)
{
  palette_swap_colors ();
}

void
tools_select_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  /*  Activate the approriate widget  */
  gtk_widget_activate (tool_widgets[tool_info[(long) client_data].toolbar_position]);
}

void
filters_repeat_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  plug_in_repeat ((long) client_data);
}

void
dialogs_brushes_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  create_brush_dialog ();
}

void
dialogs_patterns_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  create_pattern_dialog ();
}

void
dialogs_palette_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  palette_create ();
}

void
dialogs_gradient_editor_cmd_callback(GtkWidget *widget,
				     gpointer   client_data)
{
  grad_create_gradient_editor ();
} /* dialogs_gradient_editor_cmd_callback */

void
dialogs_lc_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  lc_dialog_create (gdisp->gimage->ID);
}

void
dialogs_indexed_palette_cmd_callback (GtkWidget *widget,
				      gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  indexed_palette_create (gdisp->gimage->ID);
}

void
dialogs_tools_options_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  tools_options_dialog_show ();
}

void
about_dialog_cmd_callback (GtkWidget *widget,
                           gpointer   client_data)
{
  about_dialog_create (FALSE);
}

void
tips_dialog_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  tips_dialog_create ();
}


/****************************************************/
/**           LOCAL FUNCTIONS                      **/
/****************************************************/


/*********************/
/*  Local functions  */
/*********************/

static void
image_resize_callback (GtkWidget *w,
		       gpointer   client_data)
{
  ImageResize *image_resize;
  GImage *gimage;

  image_resize = (ImageResize *) client_data;
  if ((gimage = gimage_get_ID (image_resize->gimage_id)) != NULL)
    {
      if (image_resize->resize->width > 0 &&
	  image_resize->resize->height > 0) 
	{
	  gimage_resize (gimage,
			 image_resize->resize->width,
			 image_resize->resize->height,
			 image_resize->resize->off_x,
			 image_resize->resize->off_y);
	  gdisplays_flush ();
	}
      else 
	{
	  message_box("Resize Error: Both width and height must be greater than zero.", NULL, NULL);
	  return;
	}
    }

  gtk_widget_destroy (image_resize->shell);
  resize_widget_free (image_resize->resize);
  g_free (image_resize);
}

static void
image_scale_callback (GtkWidget *w,
		      gpointer   client_data)
{
  ImageResize *image_scale;
  GImage *gimage;

  image_scale = (ImageResize *) client_data;
  if ((gimage = gimage_get_ID (image_scale->gimage_id)) != NULL)
    {
      if (image_scale->resize->width > 0 &&
	  image_scale->resize->height > 0) 
	{
	  gimage_scale (gimage,
			image_scale->resize->width,
			image_scale->resize->height);
	  gdisplays_flush ();
	}
      else 
	{
	  message_box("Scale Error: Both width and height must be greater than zero.", NULL, NULL);
	  return;
	}
    }

  gtk_widget_destroy (image_scale->shell);
  resize_widget_free (image_scale->resize);
  g_free (image_scale);
}

static gint
image_delete_callback (GtkWidget *w,
		       GdkEvent *e,
		       gpointer client_data)
{
  image_cancel_callback (w, client_data);

  return TRUE;
}


static void
image_cancel_callback (GtkWidget *w,
		       gpointer   client_data)
{
  ImageResize *image_resize;

  image_resize = (ImageResize *) client_data;

  gtk_widget_destroy (image_resize->shell);
  resize_widget_free (image_resize->resize);
  g_free (image_resize);
}

static void
gimage_mask_feather_callback (GtkWidget *w,
			      gpointer   client_data,
			      gpointer   call_data)
{
  GImage *gimage;
  double feather_radius;

  if (!(gimage = gimage_get_ID ((int)client_data)))
    return;

  feather_radius = atof (call_data);

  gimage_mask_feather (gimage, feather_radius);
  gdisplays_flush ();
}


static void
gimage_mask_border_callback (GtkWidget *w,
			     gpointer   client_data,
			     gpointer   call_data)
{
  GImage *gimage;
  int border_radius;

  if (!(gimage = gimage_get_ID ((int)client_data)))
    return;

  border_radius = atoi (call_data);

  gimage_mask_border (gimage, border_radius);
  gdisplays_flush ();
}


static void
gimage_mask_grow_callback (GtkWidget *w,
			   gpointer   client_data,
			   gpointer   call_data)
{
  GImage *gimage;
  int grow_pixels;

  if (!(gimage = gimage_get_ID ((int)client_data)))
    return;

  grow_pixels = atoi (call_data);

  gimage_mask_grow (gimage, grow_pixels);
  gdisplays_flush ();
}


static void
gimage_mask_shrink_callback (GtkWidget *w,
			     gpointer   client_data,
			     gpointer   call_data)
{
  GImage *gimage;
  int shrink_pixels;

  if (!(gimage = gimage_get_ID ((int)client_data)))
    return;

  shrink_pixels = atoi (call_data);

  gimage_mask_shrink (gimage, shrink_pixels);
  gdisplays_flush ();
}






