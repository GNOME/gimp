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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "appenv.h"
#include "colormaps.h"
#include "commands.h"
#include "fileops.h"
#include "gimprc.h"
#include "interface.h"
#include "menus.h"
#include "paint_funcs.h"
#include "procedural_db.h"
#include "scale.h"
#include "tools.h"


static void menus_init (void);
static void menus_foreach (gpointer key,
			   gpointer value,
			   gpointer user_data);
static gint menus_install_accel (GtkWidget *widget,
				 gchar     *signal_name,
				 gchar      key,
				 gchar      modifiers,
				 gchar     *path);
static void menus_remove_accel (GtkWidget *widget,
				gchar     *signal_name,
				gchar     *path);


static GtkMenuEntry menu_items[] =
{
  { "<Toolbox>/File/New", "<control>N", file_new_cmd_callback, NULL },
  { "<Toolbox>/File/Open", "<control>O", file_open_cmd_callback, NULL },
  { "<Toolbox>/File/About...", NULL, about_dialog_cmd_callback, NULL },
  { "<Toolbox>/File/Preferences...", NULL, file_pref_cmd_callback, NULL },
  { "<Toolbox>/File/Tip of the day", NULL, tips_dialog_cmd_callback, NULL },
  { "<Toolbox>/File/<separator>", NULL, NULL, NULL },
  { "<Toolbox>/File/Dialogs/Brushes...", "<control><shift>B", dialogs_brushes_cmd_callback, NULL },
  { "<Toolbox>/File/Dialogs/Patterns...", "<control><shift>P", dialogs_patterns_cmd_callback, NULL },
  { "<Toolbox>/File/Dialogs/Palette...", "<control>P", dialogs_palette_cmd_callback, NULL },
  { "<Toolbox>/File/Dialogs/Gradient Editor...", "<control>G", dialogs_gradient_editor_cmd_callback, NULL },
  { "<Toolbox>/File/Dialogs/Tool Options...", "<control><shift>T", dialogs_tools_options_cmd_callback, NULL },
  
  {"<Toolbox>/File/<separator>",NULL,NULL,NULL},



 { "<Toolbox>/File/Quit", "<control>Q", file_quit_cmd_callback, NULL },

  { "<Image>/File/New", "<control>N", file_new_cmd_callback, (gpointer) 1 },
  { "<Image>/File/Open", "<control>O", file_open_cmd_callback, NULL },
  { "<Image>/File/Save", "<control>S", file_save_cmd_callback, NULL },
  { "<Image>/File/Save as", NULL, file_save_as_cmd_callback, NULL },
  { "<Image>/File/Preferences...", NULL, file_pref_cmd_callback, NULL },
  { "<Image>/File/<separator>", NULL, NULL, NULL },





  { "<Image>/File/Close", "<control>W", file_close_cmd_callback, NULL },
  { "<Image>/File/Quit", "<control>Q", file_quit_cmd_callback, NULL },

  { "<Image>/Edit/Cut", "<control>X", edit_cut_cmd_callback, NULL },
  { "<Image>/Edit/Copy", "<control>C", edit_copy_cmd_callback, NULL },
  { "<Image>/Edit/Paste", "<control>V", edit_paste_cmd_callback, NULL },
  { "<Image>/Edit/Paste Into", NULL, edit_paste_into_cmd_callback, NULL },
  { "<Image>/Edit/Clear", "<control>K", edit_clear_cmd_callback, NULL },
  { "<Image>/Edit/Fill", "<control>.", edit_fill_cmd_callback, NULL },
  { "<Image>/Edit/Stroke", NULL, edit_stroke_cmd_callback, NULL },
  { "<Image>/Edit/Undo", "<control>Z", edit_undo_cmd_callback, NULL },
  { "<Image>/Edit/Redo", "<control>R", edit_redo_cmd_callback, NULL },
  { "<Image>/Edit/<separator>", NULL, NULL, NULL },
  { "<Image>/Edit/Cut Named", "<control><shift>X", edit_named_cut_cmd_callback, NULL },
  { "<Image>/Edit/Copy Named", "<control><shift>C", edit_named_copy_cmd_callback, NULL },
  { "<Image>/Edit/Paste Named", "<control><shift>V", edit_named_paste_cmd_callback, NULL },

  { "<Image>/Select/Toggle", "<control>T", select_toggle_cmd_callback, NULL },
  { "<Image>/Select/Invert", "<control>I", select_invert_cmd_callback, NULL },
  { "<Image>/Select/All", "<control>A", select_all_cmd_callback, NULL },
  { "<Image>/Select/None", "<control><shift>A", select_none_cmd_callback, NULL },
  { "<Image>/Select/Float", "<control><shift>L", select_float_cmd_callback, NULL },
  { "<Image>/Select/Sharpen", "<control><shift>H", select_sharpen_cmd_callback, NULL },
  { "<Image>/Select/Border", "<control><shift>B", select_border_cmd_callback, NULL },
  { "<Image>/Select/Feather", "<control><shift>F", select_feather_cmd_callback, NULL },
  { "<Image>/Select/Grow", NULL, select_grow_cmd_callback, NULL },
  { "<Image>/Select/Shrink", NULL, select_shrink_cmd_callback, NULL },
  { "<Image>/Select/Save To Channel", NULL, select_save_cmd_callback, NULL },
  { "<Image>/Select/By Color...", NULL, select_by_color_cmd_callback, NULL },

  { "<Image>/View/Zoom In", "=", view_zoomin_cmd_callback, NULL },
  { "<Image>/View/Zoom Out", "-", view_zoomout_cmd_callback, NULL },
  { "<Image>/View/Zoom/16:1", NULL, view_zoom_16_1_callback, NULL },
  { "<Image>/View/Zoom/8:1", NULL, view_zoom_8_1_callback, NULL },
  { "<Image>/View/Zoom/4:1", NULL, view_zoom_4_1_callback, NULL },
  { "<Image>/View/Zoom/2:1", NULL, view_zoom_2_1_callback, NULL },
  { "<Image>/View/Zoom/1:1", "1", view_zoom_1_1_callback, NULL },
  { "<Image>/View/Zoom/1:2", NULL, view_zoom_1_2_callback, NULL },
  { "<Image>/View/Zoom/1:4", NULL, view_zoom_1_4_callback, NULL },
  { "<Image>/View/Zoom/1:8", NULL, view_zoom_1_8_callback, NULL },
  { "<Image>/View/Zoom/1:16", NULL, view_zoom_1_16_callback, NULL },
  { "<Image>/View/Window Info...", "<control><shift>I", view_window_info_cmd_callback, NULL },
  { "<Image>/View/<check>Toggle Rulers", "<control><shift>R", view_toggle_rulers_cmd_callback, NULL },
  { "<Image>/View/<check>Toggle Guides", "<control><shift>T", view_toggle_guides_cmd_callback, NULL },
  { "<Image>/View/<check>Snap To Guides", NULL, view_snap_to_guides_cmd_callback, NULL },
  { "<Image>/View/<separator>", NULL, NULL, NULL },
  { "<Image>/View/New View", NULL, view_new_view_cmd_callback, NULL },
  { "<Image>/View/Shrink Wrap", "<control>E", view_shrink_wrap_cmd_callback, NULL },

  { "<Image>/Image/Map/Equalize", NULL, image_equalize_cmd_callback, NULL },
  { "<Image>/Image/Map/Invert", NULL, image_invert_cmd_callback, NULL },
  { "<Image>/Image/Map/Posterize", NULL, image_posterize_cmd_callback, NULL },
  { "<Image>/Image/Map/Threshold", NULL, image_threshold_cmd_callback, NULL },
  { "<Image>/Image/Adjust/Color Balance", NULL, image_color_balance_cmd_callback, NULL },
  { "<Image>/Image/Adjust/Brightness-Contrast", NULL, image_brightness_contrast_cmd_callback, NULL },
  { "<Image>/Image/Adjust/Hue-Saturation", NULL, image_hue_saturation_cmd_callback, NULL },
  { "<Image>/Image/Adjust/Curves", NULL, image_curves_cmd_callback, NULL },
  { "<Image>/Image/Adjust/Levels", NULL, image_levels_cmd_callback, NULL },
  { "<Image>/Image/Adjust/<separator>", NULL, NULL, NULL },
  { "<Image>/Image/Adjust/Desaturate", NULL, image_desaturate_cmd_callback, NULL },
  { "<Image>/Image/Channel Ops/Duplicate", "<control>D", channel_ops_duplicate_cmd_callback, NULL },
  { "<Image>/Image/Channel Ops/Offset", "<control><shift>O", channel_ops_offset_cmd_callback, NULL },
  { "<Image>/Image/<separator>", NULL, NULL, NULL },
  { "<Image>/Image/RGB", NULL, image_convert_rgb_cmd_callback, NULL },
  { "<Image>/Image/Grayscale", NULL, image_convert_grayscale_cmd_callback, NULL },
  { "<Image>/Image/Indexed", NULL, image_convert_indexed_cmd_callback, NULL },
  { "<Image>/Image/<separator>", NULL, NULL, NULL },
  { "<Image>/Image/Resize", NULL, image_resize_cmd_callback, NULL },
  { "<Image>/Image/Scale", NULL, image_scale_cmd_callback, NULL },
  { "<Image>/Image/<separator>", NULL, NULL, NULL },
  { "<Image>/Image/Histogram", NULL, image_histogram_cmd_callback, NULL },

  { "<Image>/Layers/Layers & Channels...", "<control>L", dialogs_lc_cmd_callback, NULL },
  { "<Image>/Layers/Raise Layer", "<control>F", layers_raise_cmd_callback, NULL },
  { "<Image>/Layers/Lower Layer", "<control>B", layers_lower_cmd_callback, NULL },
  { "<Image>/Layers/Anchor Layer", "<control>H", layers_anchor_cmd_callback, NULL },
  { "<Image>/Layers/Merge Visible Layers", "<control>M", layers_merge_cmd_callback, NULL },
  { "<Image>/Layers/Flatten Image", NULL, layers_flatten_cmd_callback, NULL },
  { "<Image>/Layers/Alpha To Selection", NULL, layers_alpha_select_cmd_callback, NULL },
  { "<Image>/Layers/Mask To Selection", NULL, layers_mask_select_cmd_callback, NULL },

  { "<Image>/Tools/Rect Select", "R", tools_select_cmd_callback, (gpointer) RECT_SELECT },
  { "<Image>/Tools/Ellipse Select", "E", tools_select_cmd_callback, (gpointer) ELLIPSE_SELECT },
  { "<Image>/Tools/Free Select", "F", tools_select_cmd_callback, (gpointer) FREE_SELECT },
  { "<Image>/Tools/Fuzzy Select", "Z", tools_select_cmd_callback, (gpointer) FUZZY_SELECT },
  { "<Image>/Tools/Bezier Select", "B", tools_select_cmd_callback, (gpointer) BEZIER_SELECT },
  { "<Image>/Tools/Intelligent Scissors", "I", tools_select_cmd_callback, (gpointer) ISCISSORS },
  { "<Image>/Tools/Move", "M", tools_select_cmd_callback, (gpointer) MOVE },
  { "<Image>/Tools/Magnify", "<shift>M", tools_select_cmd_callback, (gpointer) MAGNIFY },
  { "<Image>/Tools/Crop", "<shift>C", tools_select_cmd_callback, (gpointer) CROP },
  { "<Image>/Tools/Transform", "<shift>T", tools_select_cmd_callback, (gpointer) ROTATE },
  { "<Image>/Tools/Flip", "<shift>F", tools_select_cmd_callback, (gpointer) FLIP_HORZ },
  { "<Image>/Tools/Text", "T", tools_select_cmd_callback, (gpointer) TEXT },
  { "<Image>/Tools/Color Picker", "O", tools_select_cmd_callback, (gpointer) COLOR_PICKER },
  { "<Image>/Tools/Bucket Fill", "<shift>B", tools_select_cmd_callback, (gpointer) BUCKET_FILL },
  { "<Image>/Tools/Blend", "L", tools_select_cmd_callback, (gpointer) BLEND },
  { "<Image>/Tools/Paintbrush", "P", tools_select_cmd_callback, (gpointer) PAINTBRUSH },
  { "<Image>/Tools/Pencil", "<shift>P", tools_select_cmd_callback, (gpointer) PENCIL },
  { "<Image>/Tools/Eraser", "<shift>E", tools_select_cmd_callback, (gpointer) ERASER },
  { "<Image>/Tools/Airbrush", "A", tools_select_cmd_callback, (gpointer) AIRBRUSH },
  { "<Image>/Tools/Clone", "C", tools_select_cmd_callback, (gpointer) CLONE },
  { "<Image>/Tools/Convolve", "V", tools_select_cmd_callback, (gpointer) CONVOLVE },
  { "<Image>/Tools/<separator>", NULL, NULL, NULL },
  { "<Image>/Tools/Toolbox", NULL, toolbox_raise_callback, NULL },

  { "<Image>/Filters/", NULL, NULL, NULL },
  { "<Image>/Filters/Repeat last", "<alt>F", filters_repeat_cmd_callback, (gpointer) 0x0 },
  { "<Image>/Filters/Re-show last", "<alt><shift>F", filters_repeat_cmd_callback, (gpointer)
    0x1 },
  { "<Image>/Filters/<separator>", NULL, NULL, NULL },

  { "<Image>/Script-Fu/", NULL, NULL, NULL },

  { "<Image>/Dialogs/Brushes...", "<control><shift>B", dialogs_brushes_cmd_callback, NULL },
  { "<Image>/Dialogs/Patterns...", "<control><shift>P", dialogs_patterns_cmd_callback, NULL },
  { "<Image>/Dialogs/Palette...", "<control>P", dialogs_palette_cmd_callback, NULL },
  { "<Image>/Dialogs/Gradient Editor...", "<control>G", dialogs_gradient_editor_cmd_callback, NULL },
  { "<Image>/Dialogs/Layers & Channels...", "<control>L", dialogs_lc_cmd_callback, NULL },
  { "<Image>/Dialogs/Indexed Palette...", NULL, dialogs_indexed_palette_cmd_callback, NULL },
  { "<Image>/Dialogs/Tool Options...", NULL, dialogs_tools_options_cmd_callback, NULL },

  { "<Load>/Automatic", NULL, file_load_by_extension_callback, NULL },
  { "<Load>/<separator>", NULL, NULL, NULL },

  { "<Save>/By extension", NULL, file_save_by_extension_callback, NULL },
  { "<Save>/<separator>", NULL, NULL, NULL },
};
static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

static int initialize = TRUE;
static GtkMenuFactory *factory = NULL;
static GtkMenuFactory *subfactories[4];
static GHashTable *entry_ht = NULL;


void
menus_get_toolbox_menubar (GtkWidget           **menubar,
			   GtkAcceleratorTable **table)
{
  if (initialize)
    menus_init ();

  if (menubar)
    *menubar = subfactories[0]->widget;
  if (table)
    *table = subfactories[0]->table;
}

void
menus_get_image_menu (GtkWidget           **menu,
		      GtkAcceleratorTable **table)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = subfactories[1]->widget;
  if (table)
    *table = subfactories[1]->table;
}

void
menus_get_load_menu (GtkWidget           **menu,
		     GtkAcceleratorTable **table)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = subfactories[2]->widget;
  if (table)
    *table = subfactories[2]->table;
}

void
menus_get_save_menu (GtkWidget           **menu,
		     GtkAcceleratorTable **table)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = subfactories[3]->widget;
  if (table)
    *table = subfactories[3]->table;
}

void
menus_create (GtkMenuEntry *entries,
	      int           nmenu_entries)
{
  char *accelerator;
  int i;

  if (initialize)
    menus_init ();

  if (entry_ht)
    for (i = 0; i < nmenu_entries; i++)
      {
	accelerator = g_hash_table_lookup (entry_ht, entries[i].path);
	if (accelerator)
	  {
	    if (accelerator[0] == '\0')
	      entries[i].accelerator = NULL;
	    else
	      entries[i].accelerator = accelerator;
	  }
      }

  gtk_menu_factory_add_entries (factory, entries, nmenu_entries);

  for (i = 0; i < nmenu_entries; i++)
    if (entries[i].widget && GTK_BIN (entries[i].widget)->child)
      {
	gtk_signal_connect (GTK_OBJECT (entries[i].widget), "install_accelerator",
			    (GtkSignalFunc) menus_install_accel,
			    entries[i].path);
	gtk_signal_connect (GTK_OBJECT (entries[i].widget), "remove_accelerator",
			    (GtkSignalFunc) menus_remove_accel,
			    entries[i].path);
      }
}

void
menus_set_sensitive (char *path,
		     int   sensitive)
{
  GtkMenuPath *menu_path;

  if (initialize)
    menus_init ();

  menu_path = gtk_menu_factory_find (factory, path);
  if (menu_path)
    gtk_widget_set_sensitive (menu_path->widget, sensitive);
  else
    g_warning ("Unable to set sensitivity for menu which doesn't exist: %s", path);
}

void
menus_set_state (char *path,
		 int   state)
{
  GtkMenuPath *menu_path;

  if (initialize)
    menus_init ();

  menu_path = gtk_menu_factory_find (factory, path);
  if (menu_path)
    {
      if (GTK_IS_CHECK_MENU_ITEM (menu_path->widget))
	gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (menu_path->widget), state);
    }
  else
    g_warning ("Unable to set state for menu which doesn't exist: %s", path);
}

void
menus_add_path (char *path,
		char *accelerator)
{
  if (!entry_ht)
    entry_ht = g_hash_table_new (g_string_hash, g_string_equal);

  g_hash_table_insert (entry_ht, path, accelerator);
}

void
menus_destroy (char *path)
{
  if (initialize)
    menus_init ();

  gtk_menu_factory_remove_paths (factory, &path, 1);
}

void
menus_quit ()
{
  FILE *fp;
  char filename[512];
  char *gimp_dir;

  if (!entry_ht)
    return;

  gimp_dir = gimp_directory ();
  if ('\000' != gimp_dir[0])
    {
      sprintf (filename, "%s/menurc", gimp_dir);

      fp = fopen (filename, "w");
      if (!fp)
	return;

      g_hash_table_foreach (entry_ht, menus_foreach, fp);

      fclose (fp);
    }
}


static void
menus_init ()
{
  char filename[512];

  if (initialize)
    {
      initialize = FALSE;

      factory = gtk_menu_factory_new (GTK_MENU_FACTORY_MENU_BAR);

      subfactories[0] = gtk_menu_factory_new (GTK_MENU_FACTORY_MENU_BAR);
      gtk_menu_factory_add_subfactory (factory, subfactories[0], "<Toolbox>");

      subfactories[1] = gtk_menu_factory_new (GTK_MENU_FACTORY_MENU);
      gtk_menu_factory_add_subfactory (factory, subfactories[1], "<Image>");

      subfactories[2] = gtk_menu_factory_new (GTK_MENU_FACTORY_MENU);
      gtk_menu_factory_add_subfactory (factory, subfactories[2], "<Load>");

      subfactories[3] = gtk_menu_factory_new (GTK_MENU_FACTORY_MENU);
      gtk_menu_factory_add_subfactory (factory, subfactories[3], "<Save>");

      sprintf (filename, "%s/menurc", gimp_directory ());
      parse_gimprc_file (filename);

      menus_create (menu_items, nmenu_items);
    }
}

static void
menus_foreach (gpointer key,
	       gpointer value,
	       gpointer user_data)
{
  char accel[64];
  int i, j;
 
  for (i = j = 0; ((char*) value)[i] != '\0'; i++, j++)
    {
      if (((char *) value)[i] == '"' || ((char *) value)[i] == '\\')
        accel[j++] = '\\';
      accel[j] = ((char *) value)[i];
    }
 
  accel[j] = '\0';
 
  fprintf ((FILE*) user_data, "(menu-path \"%s\" \"%s\")\n", (char*) key, accel);
}

static gint
menus_install_accel (GtkWidget *widget,
		     gchar     *signal_name,
		     gchar      key,
		     gchar      modifiers,
		     gchar     *path)
{
  char accel[64];
  char *t1, t2[2];

  accel[0] = '\0';
  if (modifiers & GDK_CONTROL_MASK)
    strcat (accel, "<control>");
  if (modifiers & GDK_SHIFT_MASK)
    strcat (accel, "<shift>");
  if (modifiers & GDK_MOD1_MASK)
    strcat (accel, "<alt>");

  t2[0] = key;
  t2[1] = '\0';
  strcat (accel, t2);

  if (entry_ht)
    {
      t1 = g_hash_table_lookup (entry_ht, path);
      g_free (t1);
    }
  else
    entry_ht = g_hash_table_new (g_string_hash, g_string_equal);

  g_hash_table_insert (entry_ht, path, g_strdup (accel));

  return TRUE;
}

static void
menus_remove_accel (GtkWidget *widget,
		    gchar     *signal_name,
		    gchar     *path)
{
  char *t;

  if (entry_ht)
    {
      t = g_hash_table_lookup (entry_ht, path);
      g_free (t);

      g_hash_table_insert (entry_ht, path, g_strdup (""));
    }
}
