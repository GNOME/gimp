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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "appenv.h"
#include "colormaps.h"
#include "commands.h"
#include "fileops.h"
#include "general.h"
#include "gimprc.h"
#include "interface.h"
#include "menus.h"
#include "paint_funcs.h"
#include "procedural_db.h"
#include "scale.h"
#include "tools.h"
#include "gdisplay.h"
#include "docindex.h"

#define MRU_MENU_ENTRY_SIZE sizeof ("/File/MRU00")
#define MRU_MENU_ACCEL_SIZE sizeof ("<control>0")

static void menus_init (void);

static GSList *last_opened_raw_filenames = NULL;

static GtkItemFactoryEntry toolbox_entries[] =
{
  { "/File/New", "<control>N", file_new_cmd_callback, 0 },
  { "/File/Open", "<control>O", file_open_cmd_callback, 0 },
  { "/File/About...", NULL, about_dialog_cmd_callback, 0 },
  { "/File/Preferences...", NULL, file_pref_cmd_callback, 0 },
  { "/File/Tip of the day", NULL, tips_dialog_cmd_callback, 0 },
  { "/File/---", NULL, NULL, 0, "<Separator>" },
  { "/File/Dialogs/Brushes...", "<control><shift>B", dialogs_brushes_cmd_callback, 0 },
  { "/File/Dialogs/Patterns...", "<control><shift>P", dialogs_patterns_cmd_callback, 0 },
  { "/File/Dialogs/Palette...", "<control>P", dialogs_palette_cmd_callback, 0 },
  { "/File/Dialogs/Gradient Editor...", "<control>G", dialogs_gradient_editor_cmd_callback, 0 },
  { "/File/Dialogs/Layers & Channels...", "<control>L", dialogs_lc_cmd_callback, 0 },
  { "/File/Dialogs/Tool Options...", "<control><shift>T", dialogs_tools_options_cmd_callback, 0 },
  { "/File/Dialogs/Input Devices...", NULL, dialogs_input_devices_cmd_callback, 0 },
  { "/File/Dialogs/Device Status...", NULL, dialogs_device_status_cmd_callback, 0 },
  { "/File/Dialogs/Document Index...", NULL, raise_idea_callback, 0 },
  { "/File/Dialogs/Error Console...", NULL, dialogs_error_console_cmd_callback, 0 },
  { "/File/---", NULL, NULL, 0, "<Separator>" },
};
static guint n_toolbox_entries = sizeof (toolbox_entries) / sizeof (toolbox_entries[0]);
static GtkItemFactory *toolbox_factory = NULL;

static GtkItemFactoryEntry file_menu_separator = { "/File/---", NULL, NULL, 0, "<Separator>" };
static GtkItemFactoryEntry toolbox_end = { "/File/Quit", "<control>Q", file_quit_cmd_callback, 0 };

static GtkItemFactoryEntry image_entries[] =
{
  { "/File/New", "<control>N", file_new_cmd_callback, 1 },
  { "/File/Open", "<control>O", file_open_cmd_callback, 0 },
  { "/File/Save", "<control>S", file_save_cmd_callback, 0 },
  { "/File/Save as", NULL, file_save_as_cmd_callback, 0 },
  { "/File/Preferences...", NULL, file_pref_cmd_callback, 0 },
  { "/File/---", NULL, NULL, 0, "<Separator>" },
  
  
  { "/File/Close", "<control>W", file_close_cmd_callback, 0 },
  { "/File/Quit", "<control>Q", file_quit_cmd_callback, 0 },
  { "/File/---", NULL, NULL, 0, "<Separator>" },
  
  { "/Edit/Cut", "<control>X", edit_cut_cmd_callback, 0 },
  { "/Edit/Copy", "<control>C", edit_copy_cmd_callback, 0 },
  { "/Edit/Paste", "<control>V", edit_paste_cmd_callback, 0 },
  { "/Edit/Paste Into", NULL, edit_paste_into_cmd_callback, 0 },
  { "/Edit/Clear", "<control>K", edit_clear_cmd_callback, 0 },
  { "/Edit/Fill", "<control>period", edit_fill_cmd_callback, 0 },
  { "/Edit/Stroke", NULL, edit_stroke_cmd_callback, 0 },
  { "/Edit/Undo", "<control>Z", edit_undo_cmd_callback, 0 },
  { "/Edit/Redo", "<control>R", edit_redo_cmd_callback, 0 },
  { "/Edit/---", NULL, NULL, 0, "<Separator>" },
  { "/Edit/Cut Named", "<control><shift>X", edit_named_cut_cmd_callback, 0 },
  { "/Edit/Copy Named", "<control><shift>C", edit_named_copy_cmd_callback, 0 },
  { "/Edit/Paste Named", "<control><shift>V", edit_named_paste_cmd_callback, 0 },
  { "/Edit/---", NULL, NULL, 0, "<Separator>" },
  
  { "/Select/Toggle", "<control>T", select_toggle_cmd_callback, 0 },
  { "/Select/Invert", "<control>I", select_invert_cmd_callback, 0 },
  { "/Select/All", "<control>A", select_all_cmd_callback, 0 },
  { "/Select/None", "<control><shift>A", select_none_cmd_callback, 0 },
  { "/Select/Float", "<control><shift>L", select_float_cmd_callback, 0 },
  { "/Select/Sharpen", "<control><shift>H", select_sharpen_cmd_callback, 0 },
  { "/Select/Border", "<control><shift>B", select_border_cmd_callback, 0 },
  { "/Select/Feather", "<control><shift>F", select_feather_cmd_callback, 0 },
  { "/Select/Grow", NULL, select_grow_cmd_callback, 0 },
  { "/Select/Shrink", NULL, select_shrink_cmd_callback, 0 },
  { "/Select/Save To Channel", NULL, select_save_cmd_callback, 0 },
  /*
  { "/Select/By Color...", NULL, tools_select_cmd_callback, BY_COLOR_SELECT }, 
  */
  { "/View/Zoom In", "equal", view_zoomin_cmd_callback, 0 },
  { "/View/Zoom Out", "minus", view_zoomout_cmd_callback, 0 },
  { "/View/Zoom/16:1", NULL, view_zoom_16_1_callback, 0 },
  { "/View/Zoom/8:1", NULL, view_zoom_8_1_callback, 0 },
  { "/View/Zoom/4:1", NULL, view_zoom_4_1_callback, 0 },
  { "/View/Zoom/2:1", NULL, view_zoom_2_1_callback, 0 },
  { "/View/Zoom/1:1", "1", view_zoom_1_1_callback, 0 },
  { "/View/Zoom/1:2", NULL, view_zoom_1_2_callback, 0 },
  { "/View/Zoom/1:4", NULL, view_zoom_1_4_callback, 0 },
  { "/View/Zoom/1:8", NULL, view_zoom_1_8_callback, 0 },
  { "/View/Zoom/1:16", NULL, view_zoom_1_16_callback, 0 },
  { "/View/Window Info...", "<control><shift>I", view_window_info_cmd_callback, 0 },
  { "/View/Toggle Rulers", "<control><shift>R", view_toggle_rulers_cmd_callback, 0, "<ToggleItem>" },
  { "/View/Toggle Statusbar", "<control><shift>S", view_toggle_statusbar_cmd_callback, 0, "<ToggleItem>" },
  { "/View/Toggle Guides", "<control><shift>T", view_toggle_guides_cmd_callback, 0, "<ToggleItem>" },
  { "/View/Snap To Guides", NULL, view_snap_to_guides_cmd_callback, 0, "<ToggleItem>" },
  { "/View/---", NULL, NULL, 0, "<Separator>" },
  { "/View/New View", NULL, view_new_view_cmd_callback, 0 },
  { "/View/Shrink Wrap", "<control>E", view_shrink_wrap_cmd_callback, 0 },
  
  { "/Image/Colors/Equalize", NULL, image_equalize_cmd_callback, 0 },
  { "/Image/Colors/Invert", NULL, image_invert_cmd_callback, 0 },
  /*
  { "/Image/Colors/Posterize", NULL, tools_select_cmd_callback, POSTERIZE },
  { "/Image/Colors/Threshold", NULL, tools_select_cmd_callback, THRESHOLD },
  { "/Image/Colors/---", NULL, NULL, 0, "<Separator>" },
  { "/Image/Colors/Color Balance", NULL, tools_select_cmd_callback, COLOR_BALANCE },
  { "/Image/Colors/Brightness-Contrast", NULL, tools_select_cmd_callback, BRIGHTNESS_CONTRAST },
  { "/Image/Colors/Hue-Saturation", NULL, tools_select_cmd_callback, 0 },
  { "/Image/Colors/Curves", NULL, tools_select_cmd_callback, CURVES },
  { "/Image/Colors/Levels", NULL, tools_select_cmd_callback, LEVELS },
  */
  { "/Image/Colors/---", NULL, NULL, 0, "<Separator>" },
  { "/Image/Colors/Desaturate", NULL, image_desaturate_cmd_callback, 0 },
  { "/Image/Channel Ops/Duplicate", "<control>D", channel_ops_duplicate_cmd_callback, 0 },
  { "/Image/Channel Ops/Offset", "<control><shift>O", channel_ops_offset_cmd_callback, 0 },
  { "/Image/Alpha/Add Alpha Channel", NULL, layers_add_alpha_channel_cmd_callback, 0 },
  
  { "/Image/---", NULL, NULL, 0, "<Separator>" },
  { "/Image/RGB", NULL, image_convert_rgb_cmd_callback, 0 },
  { "/Image/Grayscale", NULL, image_convert_grayscale_cmd_callback, 0 },
  { "/Image/Indexed", NULL, image_convert_indexed_cmd_callback, 0 },
  { "/Image/---", NULL, NULL, 0, "<Separator>" },
  { "/Image/Resize", NULL, image_resize_cmd_callback, 0 },
  { "/Image/Scale", NULL, image_scale_cmd_callback, 0 },
  { "/Image/---", NULL, NULL, 0, "<Separator>" },
  /*  { "/Image/Histogram", NULL, tools_select_cmd_callback, HISTOGRAM}, */
  { "/Image/---", NULL, NULL, 0, "<Separator>" },
  
  { "/Layers/Layers & Channels...", "<control>L", dialogs_lc_cmd_callback, 0 },
  { "/Layers/Raise Layer", "<control>F", layers_raise_cmd_callback, 0 },
  { "/Layers/Lower Layer", "<control>B", layers_lower_cmd_callback, 0 },
  { "/Layers/Anchor Layer", "<control>H", layers_anchor_cmd_callback, 0 },
  { "/Layers/Merge Visible Layers", "<control>M", layers_merge_cmd_callback, 0 },
  { "/Layers/Flatten Image", NULL, layers_flatten_cmd_callback, 0 },
  { "/Layers/Alpha To Selection", NULL, layers_alpha_select_cmd_callback, 0 },
  { "/Layers/Mask To Selection", NULL, layers_mask_select_cmd_callback, 0 },
  { "/Layers/Add Alpha Channel", NULL, layers_add_alpha_channel_cmd_callback, 0 },
  
  /*  { "/Tools/Rect Select", "R", tools_select_cmd_callback, RECT_SELECT },
  { "/Tools/Ellipse Select", "E", tools_select_cmd_callback, ELLIPSE_SELECT },
  { "/Tools/Free Select", "F", tools_select_cmd_callback, FREE_SELECT },
  { "/Tools/Fuzzy Select", "Z", tools_select_cmd_callback, FUZZY_SELECT },
  { "/Tools/Bezier Select", "B", tools_select_cmd_callback, BEZIER_SELECT },
  { "/Tools/Intelligent Scissors", "I", tools_select_cmd_callback, ISCISSORS },
  { "/Tools/Move", "M", tools_select_cmd_callback, MOVE },
  { "/Tools/Magnify", "<shift>M", tools_select_cmd_callback, MAGNIFY },
  { "/Tools/Crop", "<shift>C", tools_select_cmd_callback, CROP },
  { "/Tools/Transform", "<shift>T", tools_select_cmd_callback, ROTATE },
  { "/Tools/Flip", "<shift>F", tools_select_cmd_callback, FLIP_HORZ },
  { "/Tools/Text", "T", tools_select_cmd_callback, TEXT },
  { "/Tools/Color Picker", "O", tools_select_cmd_callback, COLOR_PICKER },
  { "/Tools/Bucket Fill", "<shift>B", tools_select_cmd_callback, BUCKET_FILL },
  { "/Tools/Blend", "L", tools_select_cmd_callback, BLEND },
  { "/Tools/Paintbrush", "P", tools_select_cmd_callback, PAINTBRUSH },
  { "/Tools/Pencil", "<shift>P", tools_select_cmd_callback, PENCIL },
  { "/Tools/Eraser", "<shift>E", tools_select_cmd_callback, ERASER },
  { "/Tools/Airbrush", "A", tools_select_cmd_callback, AIRBRUSH },
  { "/Tools/Clone", "C", tools_select_cmd_callback, CLONE },
  { "/Tools/Convolve", "V", tools_select_cmd_callback, CONVOLVE },
  { "/Tools/Ink", "K", tools_select_cmd_callback, INK },
  { "/Tools/Default Colors", "D", tools_default_colors_cmd_callback, 0 },
  { "/Tools/Swap Colors", "X", tools_swap_colors_cmd_callback, 0 }, */ 
  { "/Tools/Toolbox", NULL, toolbox_raise_callback, 0 },
  { "/Tools/---", NULL, NULL, 0, "<Separator>" },  
  { "/Tools/Default Colors", "D", tools_default_colors_cmd_callback, 0 },
  { "/Tools/Swap Colors", "X", tools_swap_colors_cmd_callback, 0 },
  { "/Filters/", NULL, NULL, 0 },
  { "/Filters/Repeat last", "<alt>F", filters_repeat_cmd_callback, 0x0 },
  { "/Filters/Re-show last", "<alt><shift>F", filters_repeat_cmd_callback, 0x1 },
  { "/Filters/---", NULL, NULL, 0, "<Separator>" },
  
  { "/Script-Fu/", NULL, NULL, 0 },
  
  { "/Dialogs/Brushes...", "<control><shift>B", dialogs_brushes_cmd_callback, 0 },
  { "/Dialogs/Patterns...", "<control><shift>P", dialogs_patterns_cmd_callback, 0 },
  { "/Dialogs/Palette...", "<control>P", dialogs_palette_cmd_callback, 0 },
  { "/Dialogs/Gradient Editor...", "<control>G", dialogs_gradient_editor_cmd_callback, 0 },
  { "/Dialogs/Layers & Channels...", "<control>L", dialogs_lc_cmd_callback, 0 },
  { "/Dialogs/Indexed Palette...", NULL, dialogs_indexed_palette_cmd_callback, 0 },
  { "/Dialogs/Tool Options...", NULL, dialogs_tools_options_cmd_callback, 0 },
  { "/Dialogs/Input Devices...", NULL, dialogs_input_devices_cmd_callback, 0 },
  { "/Dialogs/Device Status...", NULL, dialogs_device_status_cmd_callback, 0 },
};
static guint n_image_entries = sizeof (image_entries) / sizeof (image_entries[0]);
static GtkItemFactory *image_factory = NULL;
  
static GtkItemFactoryEntry load_entries[] =
{
  { "/Automatic", NULL, file_load_by_extension_callback, 0 },
  { "/---", NULL, NULL, 0, "<Separator>" },
};
static guint n_load_entries = sizeof (load_entries) / sizeof (load_entries[0]);
static GtkItemFactory *load_factory = NULL;
  
static GtkItemFactoryEntry save_entries[] =
{
  { "/By extension", NULL, file_save_by_extension_callback, 0 },
  { "/---", NULL, NULL, 0, "<Separator>" },
};
static guint n_save_entries = sizeof (save_entries) / sizeof (save_entries[0]);
static GtkItemFactory *save_factory = NULL;

static int initialize = TRUE;

extern int num_tools;

void
menus_get_toolbox_menubar (GtkWidget           **menubar,
			   GtkAccelGroup       **accel_group)
{
  if (initialize)
    menus_init ();
  
  if (menubar)
    *menubar = toolbox_factory->widget;
  if (accel_group)
    *accel_group = toolbox_factory->accel_group;
}

void
menus_get_image_menu (GtkWidget           **menu,
		      GtkAccelGroup	  **accel_group)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = image_factory->widget;
  if (accel_group)
    *accel_group = image_factory->accel_group;
}

void
menus_get_load_menu (GtkWidget           **menu,
		     GtkAccelGroup	 **accel_group)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = load_factory->widget;
  if (accel_group)
    *accel_group = load_factory->accel_group;
}

void
menus_get_save_menu (GtkWidget           **menu,
		     GtkAccelGroup	 **accel_group)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = save_factory->widget;
  if (accel_group)
    *accel_group = save_factory->accel_group;
}

void
menus_create (GtkMenuEntry *entries,
	      int           nmenu_entries)
{
  if (initialize)
    menus_init ();

  gtk_item_factory_create_menu_entries (nmenu_entries, entries);
}

void
menus_tools_create (ToolInfo *tool_info)
{
  GtkItemFactoryEntry entry;
  
  /* entry.path = g_strconcat ("<Image>", tool_info->menu_path, NULL);*/
  /* entry.callback_data = tool_info; */
  entry.path = tool_info->menu_path;
  entry.accelerator = tool_info->menu_accel;
  entry.callback = tools_select_cmd_callback;
  entry.callback_action = tool_info->tool_id;
  entry.item_type = NULL;
 
  fflush(stderr);
  /*menus_create (&entry, 1);*/
  gtk_item_factory_create_item (image_factory,
				&entry,
				(gpointer)tool_info,
				2);

}

void
menus_set_sensitive (char *path,
		     int   sensitive)
{
  GtkItemFactory *ifactory;
  GtkWidget *widget = NULL;

  if (initialize)
    menus_init ();

  ifactory = gtk_item_factory_from_path (path);

  if (ifactory)
    {
      widget = gtk_item_factory_get_widget (ifactory, path);
      
      gtk_widget_set_sensitive (widget, sensitive);
    }
  if (!ifactory || !widget)
    printf ("Unable to set sensitivity for menu which doesn't exist:\n%s", path);
}

void
menus_set_state (char *path,
		 int   state)
{
  GtkItemFactory *ifactory;
  GtkWidget *widget = NULL;

  if (initialize)
    menus_init ();

  ifactory = gtk_item_factory_from_path (path);

  if (ifactory)
    {
      widget = gtk_item_factory_get_widget (ifactory, path);

      if (widget && GTK_IS_CHECK_MENU_ITEM (widget))
	gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (widget), state);
      else
	widget = NULL;
    }
  if (!ifactory || !widget)
    printf ("Unable to set state for menu which doesn't exist:\n%s", path);
}

void
menus_destroy (char *path)
{
  if (initialize)
    menus_init ();

  gtk_item_factories_path_delete (NULL, path);
}

void
menus_quit ()
{
  gchar *filename;

  filename = g_strconcat (gimp_directory (), "/menurc", NULL);
  gtk_item_factory_dump_rc (filename, NULL, TRUE);
  g_free (filename);

  if (!initialize)
    {
      gtk_object_unref (GTK_OBJECT (toolbox_factory));
      gtk_object_unref (GTK_OBJECT (image_factory));
      gtk_object_unref (GTK_OBJECT (load_factory));
      gtk_object_unref (GTK_OBJECT (save_factory));
    }

}

void
menus_last_opened_cmd_callback (GtkWidget           *widget,
                                gpointer             callback_data,
                                guint                num)
{
  gchar *filename, *raw_filename;

  raw_filename = ((GString *) g_slist_nth_data (last_opened_raw_filenames, num))->str;
  filename = prune_filename (raw_filename);

  if (!file_open(raw_filename, filename))
    g_message ("Error opening file: %s\n", raw_filename);
}

void
menus_last_opened_update_labels ()
{
  GSList	*filename_slist;
  GString	*entry_filename, *path;
  GtkWidget	*widget;
  gint		i;
  guint         num_entries;

  entry_filename = g_string_new ("");
  path = g_string_new ("");

  filename_slist = last_opened_raw_filenames;
  num_entries = g_slist_length (last_opened_raw_filenames); 

  for (i = 1; i <= num_entries; i++)
    {
      g_string_sprintf (entry_filename, "%d. %s", i, prune_filename (((GString *) filename_slist->data)->str));

      g_string_sprintf (path, "/File/MRU%02d", i);

      widget = gtk_item_factory_get_widget (toolbox_factory, path->str);
      gtk_widget_show (widget);

      gtk_label_set (GTK_LABEL (GTK_BIN (widget)->child), entry_filename->str);
      
      filename_slist = filename_slist->next;
    }

  g_string_free (entry_filename, TRUE);
  g_string_free (path, TRUE);
}

void
menus_last_opened_add (gchar *filename)
{
  GString	*raw_filename;
  GtkWidget	*widget;
  guint         num_entries;

  num_entries = g_slist_length (last_opened_raw_filenames);

  if (num_entries == last_opened_size)
    {
      g_slist_remove_link (last_opened_raw_filenames, 
			   g_slist_last (last_opened_raw_filenames));
    }

  raw_filename = g_string_new (filename);
  last_opened_raw_filenames = g_slist_prepend (last_opened_raw_filenames, raw_filename);

  if (num_entries == 0)
    {
      widget = gtk_item_factory_get_widget (toolbox_factory, file_menu_separator.path);
      gtk_widget_show (widget);
    }

  menus_last_opened_update_labels ();
}

void
menus_init_mru ()
{
  gchar			*paths, *accelerators;
  gint			i;
  GtkItemFactoryEntry	*last_opened_entries;
  GtkWidget		*widget;
  
  last_opened_entries = g_new (GtkItemFactoryEntry, last_opened_size);

  paths = g_new (gchar, last_opened_size * MRU_MENU_ENTRY_SIZE);
  accelerators = g_new (gchar, 9 * MRU_MENU_ACCEL_SIZE);

  for (i = 0; i < last_opened_size; i++)
    {
      gchar *path, *accelerator;
      
      path = &paths[i * MRU_MENU_ENTRY_SIZE];
      if (i < 9)
        accelerator = &accelerators[i * MRU_MENU_ACCEL_SIZE];
      else
        accelerator = NULL;
    
      last_opened_entries[i].path = path;
      last_opened_entries[i].accelerator = accelerator;
      last_opened_entries[i].callback = (GtkItemFactoryCallback) menus_last_opened_cmd_callback;
      last_opened_entries[i].callback_action = i;
      last_opened_entries[i].item_type = NULL;

      sprintf (path, "/File/MRU%02d", i + 1);
      sprintf (accelerator, "<control>%d", i + 1);
    }

  gtk_item_factory_create_items_ac (toolbox_factory, last_opened_size,
  				    last_opened_entries, NULL, 2);
  gtk_item_factory_create_item (toolbox_factory, &file_menu_separator, NULL, 2);
  gtk_item_factory_create_item (toolbox_factory, &toolbox_end, NULL, 2);

  for (i=0; i < last_opened_size; i++)
    {
      widget = gtk_item_factory_get_widget (toolbox_factory,
      					    last_opened_entries[i].path);
      gtk_widget_hide (widget);
    }

  widget = gtk_item_factory_get_widget (toolbox_factory, file_menu_separator.path);
  gtk_widget_hide (widget);
  
  g_free (paths);
  g_free (accelerators);
  g_free (last_opened_entries);
}

/*  This is separate from menus_init() in case the last_opened_size changes,
    or for any other reason we might want to regen just the toolbox menu     */
void
menus_init_toolbox ()
{
  toolbox_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<Toolbox>", NULL);
  gtk_item_factory_create_items_ac (toolbox_factory, n_toolbox_entries,
				    toolbox_entries, NULL, 2);
  menus_init_mru ();
}

static void
menus_init ()
{
  int i;

  if (initialize)
    {
      gchar *filename;

      initialize = FALSE;

      menus_init_toolbox ();

      image_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Image>", NULL);
      gtk_item_factory_create_items_ac (image_factory,
					n_image_entries,
					image_entries,
					NULL, 2);
      load_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Load>", NULL);
      gtk_item_factory_create_items_ac (load_factory,
					n_load_entries,
					load_entries,
					NULL, 2);
      save_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Save>", NULL);
      gtk_item_factory_create_items_ac (save_factory,
					n_save_entries,
					save_entries,
					NULL, 2);
      for (i = 0; i < num_tools; i++)
	{
	  /* FIXME this need to use access functions to check a flag */
	  if (tool_info[i].menu_path)
	    menus_tools_create (tool_info+i);
	}
      filename = g_strconcat (gimp_directory (), "/menurc", NULL);
      gtk_item_factory_parse_rc (filename);
      g_free (filename);
    }
}
