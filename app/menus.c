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

#include "config.h"
#include "libgimp/gimpintl.h"
#include "libgimp/gimpenv.h"

#define MRU_MENU_ENTRY_SIZE (strlen (_("/File/MRU00 ")) + 1)
#define MRU_MENU_ACCEL_SIZE sizeof ("<control>0")

static char* G_GNUC_UNUSED dummyMRU = N_("/File/MRU00 ");

static void menus_init (void);
static GtkItemFactoryEntry * translate_entries (const GtkItemFactoryEntry *, gint);
static void free_translated_entries(GtkItemFactoryEntry *, gint);

static GSList *last_opened_raw_filenames = NULL;

static const GtkItemFactoryEntry toolbox_entries[] =
{
  { N_("/_File"), "", NULL, 0, "<Branch>" },
  { N_("/File/_New"), "<control>N", file_new_cmd_callback, 0 },
  { N_("/File/_Open"), "<control>O", file_open_cmd_callback, 0 },
  { N_("/File/_About..."), NULL, about_dialog_cmd_callback, 0 },
  { N_("/File/_Preferences..."), NULL, file_pref_cmd_callback, 0 },
  { N_("/File/_Tip of the day"), NULL, tips_dialog_cmd_callback, 0 },
  { N_("/File/---"), NULL, NULL, 0, "<Separator>" },  
  { N_("/File/_Dialogs"), "", NULL, 0, "<Branch>" },
  { N_("/File/Dialogs/_Brushes..."), "<control><shift>B", dialogs_brushes_cmd_callback, 0 },
  { N_("/File/Dialogs/_Patterns..."), "<control><shift>P", dialogs_patterns_cmd_callback, 0 },
  { N_("/File/Dialogs/P_alette..."), "<control>P", dialogs_palette_cmd_callback, 0 },
  { N_("/File/Dialogs/_Gradient..."), "<control>G", dialogs_gradient_editor_cmd_callback, 0 },
  { N_("/File/Dialogs/_Layers & Channels..."), "<control>L", dialogs_lc_cmd_callback, 0 },
  { N_("/File/Dialogs/_Tool Options..."), "<control><shift>T", dialogs_tools_options_cmd_callback, 0 },
  { N_("/File/Dialogs/_Input Devices..."), NULL, dialogs_input_devices_cmd_callback, 0 },
  { N_("/File/Dialogs/_Device Status..."), NULL, dialogs_device_status_cmd_callback, 0 },
  { N_("/File/Dialogs/D_ocument Index..."), NULL, raise_idea_callback, 0 },
  { N_("/File/Dialogs/_Error Console..."), NULL, dialogs_error_console_cmd_callback, 0 },
  { N_("/File/Dialogs/_Module Browser..."), NULL, dialogs_module_browser_cmd_callback, 0 },
  { N_("/File/---"), NULL, NULL, 0, "<Separator>" }
};
static guint n_toolbox_entries = sizeof (toolbox_entries) / sizeof (toolbox_entries[0]);
static GtkItemFactory *toolbox_factory = NULL;

static const GtkItemFactoryEntry file_menu_separator = { N_("/File/---"), NULL, NULL, 0, "<Separator>" };
static const GtkItemFactoryEntry toolbox_end = { N_("/File/_Quit"), "<control>Q", file_quit_cmd_callback, 0 };

static const GtkItemFactoryEntry image_entries[] =
{
  { N_("/_File"), "", NULL, 0, "<Branch>" },
  { N_("/File/_New"), "<control>N", file_new_cmd_callback, 1 },
  { N_("/File/_Open"), "<control>O", file_open_cmd_callback, 0 },
  { N_("/File/_Save"), "<control>S", file_save_cmd_callback, 0 },
  { N_("/File/Save _as"), NULL, file_save_as_cmd_callback, 0 },
  { N_("/File/_Revert"), NULL, file_revert_cmd_callback, 0 },
  { N_("/File/_Preferences..."), NULL, file_pref_cmd_callback, 0 },
  { N_("/File/---"), NULL, NULL, 0, "<Separator>" },
  
  { N_("/File/_Close"), "<control>W", file_close_cmd_callback, 0 },
  { N_("/File/_Quit"), "<control>Q", file_quit_cmd_callback, 0 },
  { N_("/File/---moved"), NULL, NULL, 0, "<Separator>" },
  
  { N_("/_Edit"), "", NULL, 0, "<Branch>" },
  { N_("/Edit/Cu_t"), "<control>X", edit_cut_cmd_callback, 0 },
  { N_("/Edit/_Copy"), "<control>C", edit_copy_cmd_callback, 0 },
  { N_("/Edit/_Paste"), "<control>V", edit_paste_cmd_callback, 0 },
  { N_("/Edit/Paste _Into"), NULL, edit_paste_into_cmd_callback, 0 },
  { N_("/Edit/C_lear"), "<control>K", edit_clear_cmd_callback, 0 },
  { N_("/Edit/_Fill"), "<control>period", edit_fill_cmd_callback, 0 },
  { N_("/Edit/_Stroke"), NULL, edit_stroke_cmd_callback, 0 },
  { N_("/Edit/_Undo"), "<control>Z", edit_undo_cmd_callback, 0 },
  { N_("/Edit/_Redo"), "<control>R", edit_redo_cmd_callback, 0 },
  { N_("/Edit/---"), NULL, NULL, 0, "<Separator>" },
  { N_("/Edit/Cut _Named"), "<control><shift>X", edit_named_cut_cmd_callback, 0 },
  { N_("/Edit/Cop_y Named"), "<control><shift>C", edit_named_copy_cmd_callback, 0 },
  { N_("/Edit/Past_e Named"), "<control><shift>V", edit_named_paste_cmd_callback, 0 },
  { N_("/Edit/---"), NULL, NULL, 0, "<Separator>" },
  
  { N_("/_Select"), "", NULL, 0, "<Branch>" },
  { N_("/Select/_Toggle"), "<control>T", select_toggle_cmd_callback, 0 },
  { N_("/Select/_Invert"), "<control>I", select_invert_cmd_callback, 0 },
  { N_("/Select/_All"), "<control>A", select_all_cmd_callback, 0 },
  { N_("/Select/_None"), "<control><shift>A", select_none_cmd_callback, 0 },
  { N_("/Select/_Float"), "<control><shift>L", select_float_cmd_callback, 0 },
  { N_("/Select/_Sharpen"), "<control><shift>H", select_sharpen_cmd_callback, 0 },
  { N_("/Select/_Border"), "<control><shift>B", select_border_cmd_callback, 0 },
  { N_("/Select/F_eather"), "<control><shift>F", select_feather_cmd_callback, 0 },
  { N_("/Select/_Grow"), NULL, select_grow_cmd_callback, 0 },
  { N_("/Select/S_hrink"), NULL, select_shrink_cmd_callback, 0 },
  { N_("/Select/Save To _Channel"), NULL, select_save_cmd_callback, 0 },

  { N_("/_View"), "", NULL, 0, "<Branch>" },
  { N_("/View/Zoom _In"), "equal", view_zoomin_cmd_callback, 0 },
  { N_("/View/Zoom _Out"), "minus", view_zoomout_cmd_callback, 0 },
  { N_("/View/Zoom/1_6:1"), NULL, view_zoom_16_1_callback, 0 },
  { N_("/View/Zoom/_8:1"), NULL, view_zoom_8_1_callback, 0 },
  { N_("/View/Zoom/_4:1"), NULL, view_zoom_4_1_callback, 0 },
  { N_("/View/Zoom/_2:1"), NULL, view_zoom_2_1_callback, 0 },
  { N_("/View/Zoom/_1:1"), "1", view_zoom_1_1_callback, 0 },
  { N_("/View/Zoom/1:2"), NULL, view_zoom_1_2_callback, 0 },
  { N_("/View/Zoom/1:4"), NULL, view_zoom_1_4_callback, 0 },
  { N_("/View/Zoom/1:8"), NULL, view_zoom_1_8_callback, 0 },
  { N_("/View/Zoom/1:16"), NULL, view_zoom_1_16_callback, 0 },
  { N_("/View/_Dot for dot"), NULL, view_dot_for_dot_callback, 0, "<ToggleItem>"},
  { N_("/View/_Window Info..."), "<control><shift>I", view_window_info_cmd_callback, 0 },
  { N_("/View/Toggle _Rulers"), "<control><shift>R", view_toggle_rulers_cmd_callback, 0, "<ToggleItem>" },
  { N_("/View/Toggle S_tatusbar"), "<control><shift>S", view_toggle_statusbar_cmd_callback, 0, "<ToggleItem>" },
  { N_("/View/Toggle _Guides"), "<control><shift>T", view_toggle_guides_cmd_callback, 0, "<ToggleItem>" },
  { N_("/View/_Snap To Guides"), NULL, view_snap_to_guides_cmd_callback, 0, "<ToggleItem>" },
  { N_("/View/---"), NULL, NULL, 0, "<Separator>" },
  { N_("/View/_New View"), NULL, view_new_view_cmd_callback, 0 },
  { N_("/View/S_hrink Wrap"), "<control>E", view_shrink_wrap_cmd_callback, 0 },
  
  { N_("/_Image"), "", NULL, 0, "<Branch>" },  
  { N_("/Image/_Colors"), "", NULL, 0, "<Branch>" },
  { N_("/Image/Colors/_Equalize"), NULL, image_equalize_cmd_callback, 0 },
  { N_("/Image/Colors/_Invert"), NULL, image_invert_cmd_callback, 0 },
  { N_("/Image/Colors/---"), NULL, NULL, 0, "<Separator>" },
  { N_("/Image/Colors/_Desaturate"), NULL, image_desaturate_cmd_callback, 0 },
  { N_("/Image/C_hannel Ops"), "", NULL, 0, "<Branch>" },
  { N_("/Image/Channel Ops/_Duplicate"), "<control>D", channel_ops_duplicate_cmd_callback, 0 },
  { N_("/Image/Channel Ops/_Offset"), "<control><shift>O", channel_ops_offset_cmd_callback, 0 },
  { N_("/Image/_Alpha"), "", NULL, 0, "<Branch>" },
  { N_("/Image/Alpha/_Add Alpha Channel"), NULL, layers_add_alpha_channel_cmd_callback, 0 },
  
  { N_("/Image/---"), NULL, NULL, 0, "<Separator>" },
  { N_("/Image/_RGB"), NULL, image_convert_rgb_cmd_callback, 0 },
  { N_("/Image/_Grayscale"), NULL, image_convert_grayscale_cmd_callback, 0 },
  { N_("/Image/_Indexed"), NULL, image_convert_indexed_cmd_callback, 0 },
  { N_("/Image/---"), NULL, NULL, 0, "<Separator>" },
  { N_("/Image/R_esize"), NULL, image_resize_cmd_callback, 0 },
  { N_("/Image/_Scale"), NULL, image_scale_cmd_callback, 0 },
  { N_("/Image/---"), NULL, NULL, 0, "<Separator>" },
  
  { N_("/_Layers"), "", NULL, 0, "<Branch>" },
  { N_("/Layers/_Layers & Channels..."), "<control>L", dialogs_lc_cmd_callback, 0 },
  { N_("/Layers/_Raise Layer"), "<control>F", layers_raise_cmd_callback, 0 },
  { N_("/Layers/L_ower Layer"), "<control>B", layers_lower_cmd_callback, 0 },
  { N_("/Layers/_Anchor Layer"), "<control>H", layers_anchor_cmd_callback, 0 },
  { N_("/Layers/_Merge Visible Layers"), "<control>M", layers_merge_cmd_callback, 0 },
  { N_("/Layers/_Flatten Image"), NULL, layers_flatten_cmd_callback, 0 },
  { N_("/Layers/Al_pha To Selection"), NULL, layers_alpha_select_cmd_callback, 0 },
  { N_("/Layers/Ma_sk To Selection"), NULL, layers_mask_select_cmd_callback, 0 },
  { N_("/Layers/A_dd Alpha Channel"), NULL, layers_add_alpha_channel_cmd_callback, 0 },
  
/* these are built on the fly */

/*
  { N_("/Tools/Ellipse Select"), "E", tools_select_cmd_callback, ELLIPSE_SELECT },
  { N_("/Tools/Free Select"), "F", tools_select_cmd_callback, FREE_SELECT },
  { N_("/Tools/Fuzzy Select"), "Z", tools_select_cmd_callback, FUZZY_SELECT },
  { N_("/Tools/Bezier Select"), "B", tools_select_cmd_callback, BEZIER_SELECT },
  { N_("/Tools/Intelligent Scissors"), "I", tools_select_cmd_callback, ISCISSORS },
  { N_("/Tools/Move"), "M", tools_select_cmd_callback, MOVE },
  { N_("/Tools/Magnify"), "<shift>M", tools_select_cmd_callback, MAGNIFY },
  { N_("/Tools/Crop"), "<shift>C", tools_select_cmd_callback, CROP },
  { N_("/Tools/Transform"), "<shift>T", tools_select_cmd_callback, ROTATE },
  { N_("/Tools/Flip"), "<shift>F", tools_select_cmd_callback, FLIP_HORZ },
  { N_("/Tools/Text"), "T", tools_select_cmd_callback, TEXT },
  { N_("/Tools/Color Picker"), "O", tools_select_cmd_callback, COLOR_PICKER },
  { N_("/Tools/Bucket Fill"), "<shift>B", tools_select_cmd_callback, BUCKET_FILL },
  { N_("/Tools/Blend"), "L", tools_select_cmd_callback, BLEND },
  { N_("/Tools/Paintbrush"), "P", tools_select_cmd_callback, PAINTBRUSH },
  { N_("/Tools/Pencil"), "<shift>P", tools_select_cmd_callback, PENCIL },
  { N_("/Tools/Eraser"), "<shift>E", tools_select_cmd_callback, ERASER },
  { N_("/Tools/Airbrush"), "A", tools_select_cmd_callback, AIRBRUSH },
  { N_("/Tools/Clone"), "C", tools_select_cmd_callback, CLONE },
  { N_("/Tools/Convolve"), "V", tools_select_cmd_callback, CONVOLVE },
  { N_("/Tools/Ink"), "K", tools_select_cmd_callback, INK },
  { N_("/Tools/Default Colors"), "D", tools_default_colors_cmd_callback, 0 },
  { N_("/Tools/Toolbox"), NULL, toolbox_raise_callback, 0 },
  { N_("/Tools/---"), NULL, NULL, 0, "<Separator>" },  
  { N_("/Tools/Default Colors"), "D", tools_default_colors_cmd_callback, 0 },
  { N_("/Tools/Swap Colors"), "X", tools_swap_colors_cmd_callback, 0 },
*/

  { N_("/_Tools"), "", NULL, 0, "<Branch>" },
  { N_("/Tools/_Toolbox"), NULL, toolbox_raise_callback, 0 },
  { N_("/Tools/_Default Colors"), "D", tools_default_colors_cmd_callback, 0 },
  { N_("/Tools/_Swap Colors"), "X", tools_swap_colors_cmd_callback, 0 },
  { N_("/Tools/---"), NULL, NULL, 0, "<Separator>" },  

  { N_("/Filte_rs"), "", NULL, 0, "<Branch>" },
  { N_("/Filters/_Repeat last"), "<alt>F", filters_repeat_cmd_callback, 0x0 },
  { N_("/Filters/Re-_show last"), "<alt><shift>F", filters_repeat_cmd_callback, 0x1 },
  { N_("/Filters/---"), NULL, NULL, 0, "<Separator>" },
  
  { N_("/Script-Fu/"), NULL, NULL, 0 },
  
  { N_("/_Dialogs"), "", NULL, 0, "<Branch>" },
  { N_("/Dialogs/_Brushes..."), "<control><shift>B", dialogs_brushes_cmd_callback, 0 },
  { N_("/Dialogs/_Patterns..."), "<control><shift>P", dialogs_patterns_cmd_callback, 0 },
  { N_("/Dialogs/P_alette..."), "<control>P", dialogs_palette_cmd_callback, 0 },
  { N_("/Dialogs/_Gradient..."), "<control>G", dialogs_gradient_editor_cmd_callback, 0 },
  { N_("/Dialogs/_Layers & Channels..."), "<control>L", dialogs_lc_cmd_callback, 0 },
  { N_("/Dialogs/_Indexed Palette..."), NULL, dialogs_indexed_palette_cmd_callback, 0 },
  { N_("/Dialogs/_Tool Options..."), NULL, dialogs_tools_options_cmd_callback, 0 },
  { N_("/Dialogs/I_nput Devices..."), NULL, dialogs_input_devices_cmd_callback, 0 },
  { N_("/Dialogs/_Device Status..."), NULL, dialogs_device_status_cmd_callback, 0 },
};
static guint n_image_entries = sizeof (image_entries) / sizeof (image_entries[0]);
static GtkItemFactory *image_factory = NULL;
  
static const GtkItemFactoryEntry load_entries[] =
{
  { N_("/Automatic"), NULL, file_load_by_extension_callback, 0 },
  { "/---", NULL, NULL, 0, "<Separator>" },
};
static guint n_load_entries = sizeof (load_entries) / sizeof (load_entries[0]);
static GtkItemFactory *load_factory = NULL;
  
static const GtkItemFactoryEntry save_entries[] =
{
  { N_("/By extension"), NULL, file_save_by_extension_callback, 0 },
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
  GtkItemFactory *ifactory;
  GtkWidget *menu_item;
  int i;
  int redo_image_menu = FALSE;

  if (initialize)
    menus_init ();

  gtk_item_factory_create_menu_entries (nmenu_entries, entries);

  for (i = 0; i < nmenu_entries; i++)
      if (!strncmp(entries[i].path, "<Image>", 7))
        redo_image_menu = TRUE;

  if (redo_image_menu)
    {
        ifactory = gtk_item_factory_from_path ("<Image>/File/Quit"); 
        menu_item = gtk_item_factory_get_widget (ifactory, "<Image>/File/---moved");
        if (menu_item && menu_item->parent)
            gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, -1);
        menu_item = gtk_item_factory_get_widget (ifactory, "<Image>/File/Close");
        if (menu_item && menu_item->parent)
            gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, -1);
        menu_item = gtk_item_factory_get_widget (ifactory, "<Image>/File/Quit");
        if (menu_item && menu_item->parent)
            gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, -1);

    }
}

void
menus_tools_create (ToolInfo *tool_info)
{
  GtkItemFactoryEntry entry;

  entry.path = gettext(tool_info->menu_path);
  entry.accelerator = tool_info->menu_accel;
  entry.callback = tools_select_cmd_callback;
  entry.callback_action = tool_info->tool_id;
  entry.item_type = NULL;
 
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
    g_message (_("Unable to set sensitivity for menu which doesn't exist:\n%s"), path);
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
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), state);
      else
	widget = NULL;
    }
  if (!ifactory || !widget)
    g_message (_("Unable to set state for menu which doesn't exist:\n%s\n"), path);
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

  filename = gimp_personal_rc_file ("menurc");
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
  filename = g_basename (raw_filename);

  if (!file_open(raw_filename, filename))
    g_message (_("Error opening file: %s\n"), raw_filename);
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
      g_string_sprintf (entry_filename, "%d. %s", i, g_basename (((GString *) filename_slist->data)->str));

      g_string_sprintf (path, _("/File/MRU%02d"), i);

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
      widget = gtk_item_factory_get_widget (toolbox_factory, gettext(file_menu_separator.path));
      gtk_widget_show (widget);
    }

  menus_last_opened_update_labels ();
}

void
menus_init_mru (void)
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

      g_snprintf (path, MRU_MENU_ENTRY_SIZE, _("/File/MRU%02d"), i + 1);
      if (accelerator != NULL)
	g_snprintf (accelerator, MRU_MENU_ACCEL_SIZE, "<control>%d", i + 1);
    }

  gtk_item_factory_create_items_ac (toolbox_factory, last_opened_size,
  				    last_opened_entries, NULL, 2);
  
  for (i=0; i < last_opened_size; i++)
    {
      widget = gtk_item_factory_get_widget (toolbox_factory,
      					    last_opened_entries[i].path);
      gtk_widget_hide (widget);
    }

  widget = gtk_item_factory_get_widget (toolbox_factory, gettext(file_menu_separator.path));
  gtk_widget_hide (widget);
  
  g_free (paths);
  g_free (accelerators);
  g_free (last_opened_entries);
}

void
menus_init_toolbox ()
{
  GtkItemFactoryEntry *translated_entries;

  toolbox_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<Toolbox>", NULL);
  translated_entries=translate_entries(toolbox_entries, n_toolbox_entries);
  gtk_item_factory_create_items_ac (toolbox_factory, n_toolbox_entries,
				    translated_entries, NULL, 2);
  free_translated_entries(translated_entries, n_toolbox_entries);
  menus_init_mru ();

  translated_entries=translate_entries(&file_menu_separator,1);
  gtk_item_factory_create_item (toolbox_factory, translated_entries, NULL, 2);
  free_translated_entries(translated_entries, 1);
   
  translated_entries=translate_entries(&toolbox_end,1);
  gtk_item_factory_create_item (toolbox_factory, translated_entries, NULL, 2);
  free_translated_entries(translated_entries, 1);
}

static GtkItemFactoryEntry *
translate_entries (const GtkItemFactoryEntry *entries, gint n)
{
  gint i;
  GtkItemFactoryEntry *ret;

  ret=g_malloc( sizeof(GtkItemFactoryEntry) * n );
  for (i=0; i<n; i++) {
    /* Translation. Note the explicit use of gettext(). */
    ret[i].path=g_strdup( gettext(entries[i].path) );
    /* accelerator and item_type are not duped, only referenced */
    ret[i].accelerator=entries[i].accelerator;
    ret[i].callback=entries[i].callback;
    ret[i].callback_action=entries[i].callback_action;
    ret[i].item_type=entries[i].item_type;
  }
  return ret;
}

static void 
free_translated_entries(GtkItemFactoryEntry *entries, gint n)
{
  gint i;

  for (i=0; i<n; i++)
    g_free(entries[i].path);
  g_free(entries);
}


static void
menus_init ()
{
  int i;
  GtkItemFactoryEntry *translated_entries;

  if (initialize)
    {
      gchar *filename;

      initialize = FALSE;

      menus_init_toolbox ();

      image_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Image>", NULL);
      translated_entries=translate_entries(image_entries, n_image_entries);
      gtk_item_factory_create_items_ac (image_factory,
					n_image_entries,
					translated_entries,
					NULL, 2);
      free_translated_entries(translated_entries, n_image_entries);
      load_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Load>", NULL);
      translated_entries=translate_entries(load_entries, n_load_entries);
      gtk_item_factory_create_items_ac (load_factory,
					n_load_entries,
					translated_entries,
					NULL, 2);
      free_translated_entries(translated_entries, n_load_entries);
      save_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Save>", NULL);
      translated_entries=translate_entries(load_entries, n_save_entries);
      gtk_item_factory_create_items_ac (save_factory,
					n_save_entries,
					translated_entries,
					NULL, 2);
      free_translated_entries(translated_entries, n_save_entries);
      for (i = 0; i < num_tools; i++)
	{
	  /* FIXME this need to use access functions to check a flag */
	  if (tool_info[i].menu_path)
	    menus_tools_create (tool_info+i);
	}
      filename = gimp_personal_rc_file ("menurc");
      gtk_item_factory_parse_rc (filename);
      g_free (filename);
    }
}
