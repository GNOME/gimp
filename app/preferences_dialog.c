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
#include "devices.h"
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


typedef struct
{
  GtkWidget * shell;
  Resize *    resize;
  int         gimage_id;
} ImageResize;

/*  preferences local functions  */
static void file_prefs_ok_callback (GtkWidget *, GtkWidget *);
static void file_prefs_save_callback (GtkWidget *, GtkWidget *);
static void file_prefs_cancel_callback (GtkWidget *, GtkWidget *);
static gint file_prefs_delete_callback (GtkWidget *, GdkEvent *, GtkWidget *);
static void file_prefs_toggle_callback (GtkWidget *, gpointer);
static void file_prefs_text_callback (GtkWidget *, gpointer);
static void file_prefs_preview_size_callback (GtkWidget *, gpointer);


/*  static variables  */
static   int          last_type = RGB;

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
      g_message ("Error: Levels of undo must be zero or greater.");
      levels_of_undo = old_levels_of_undo;
      return;
    }
  if (marching_speed < 50)
    {
      g_message ("Error: Marching speed must be 50 or greater.");
      marching_speed = old_marching_speed;
      return;
    }
  if (default_width < 1)
    {
      g_message ("Error: Default width must be one or greater.");
      default_width = old_default_width;
      return;
    }
  if (default_height < 1)
    {
      g_message ("Error: Default height must be one or greater.");
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
  int restart_notification = FALSE;

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
      restart_notification = TRUE;
    }
  if (edit_tile_cache_size != tile_cache_size)
    {
      update = g_list_append (update, "tile-cache-size");
      tile_cache_size = edit_tile_cache_size;
      restart_notification = TRUE;
    }
  if (edit_install_cmap != old_install_cmap)
    {
      update = g_list_append (update, "install-colormap");
      install_cmap = edit_install_cmap;
      restart_notification = TRUE;
    }
  if (edit_cycled_marching_ants != cycled_marching_ants)
    {
      update = g_list_append (update, "colormap-cycling");
      cycled_marching_ants = edit_cycled_marching_ants;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (temp_path, edit_temp_path))
    {
      update = g_list_append (update, "temp-path");
      temp_path = edit_temp_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (swap_path, edit_swap_path))
    {
      update = g_list_append (update, "swap-path");
      swap_path = edit_swap_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (brush_path, edit_brush_path))
    {
      update = g_list_append (update, "brush-path");
      brush_path = edit_brush_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (pattern_path, edit_pattern_path))
    {
      update = g_list_append (update, "pattern-path");
      pattern_path = edit_pattern_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (palette_path, edit_palette_path))
    {
      update = g_list_append (update, "palette-path");
      palette_path = edit_palette_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (plug_in_path, edit_plug_in_path))
    {
      update = g_list_append (update, "plug-in-path");
      plug_in_path = edit_plug_in_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (gradient_path, edit_gradient_path))
    {
      update = g_list_append (update, "gradient-path");
      gradient_path = edit_gradient_path;
      restart_notification = TRUE;
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

  if (restart_notification)
    g_message ("You will need to restart GIMP for these changes to take effect.");
  
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
  else if (data==&edit_stingy_memory_use)
    edit_stingy_memory_use = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&edit_install_cmap)
    edit_install_cmap = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&edit_cycled_marching_ants)
    edit_cycled_marching_ants = GTK_TOGGLE_BUTTON (widget)->active;
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
			  &edit_stingy_memory_use);
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
                          &edit_tile_cache_size);
      gtk_widget_show (entry);
      
      button = gtk_check_button_new_with_label("Install colormap (8-bit only)");
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
				   install_cmap);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &edit_install_cmap);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      button = gtk_check_button_new_with_label("Colormap cycling (8-bit only)");
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   cycled_marching_ants);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &edit_cycled_marching_ants);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      if (g_visual->depth != 8)
	gtk_widget_set_sensitive (GTK_WIDGET(button), FALSE);
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
