#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "appenv.h"
#include "about_dialog.h"
#include "actionarea.h"
#include "app_procs.h"
#include "brightness_contrast.h"
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
#include "gimage_mask.h"
#include "gimprc.h"
#include "global_edit.h"
#include "gradient.h"
#include "histogram_tool.h"
#include "hue_saturation.h"
#include "image_render.h"
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
#include "scale.h"
#include "session.h"
#include "threshold.h"
#include "tips_dialog.h"
#include "tools.h"
#include "undo.h"
#include "libgimp/gimpsizeentry.h"

#include "config.h"
#include "libgimp/gimpintl.h"


/*  preferences local functions  */
static void file_prefs_ok_callback (GtkWidget *, GtkWidget *);
static void file_prefs_save_callback (GtkWidget *, GtkWidget *);
static void file_prefs_cancel_callback (GtkWidget *, GtkWidget *);
static gint file_prefs_delete_callback (GtkWidget *, GdkEvent *, GtkWidget *);
static void file_prefs_toggle_callback (GtkWidget *, gpointer);
/* static void file_prefs_text_callback (GtkWidget *, gpointer); */
static void file_prefs_spinbutton_callback (GtkWidget *, gpointer);
static void file_prefs_preview_size_callback (GtkWidget *, gpointer);
static void file_prefs_mem_size_unit_callback (GtkWidget *, gpointer);
static void file_prefs_clear_session_info_callback (GtkWidget *, gpointer);
static void file_prefs_resolution_callback (GtkWidget *, gpointer);

/*  static variables  */
static   int          last_type = RGB;

static   GtkWidget   *prefs_dlg = NULL;
static   int          old_perfectmouse;
static   int          old_transparency_type;
static   int          old_transparency_size;
static   int          old_levels_of_undo;
static   int          old_marching_speed;
static   int          old_allow_resize_windows;
static   int          old_auto_save;
static   int          old_preview_size;
static   int          old_no_cursor_updating;
static   int          old_show_tool_tips;
static   int          old_show_rulers;
static   int          old_show_statusbar;
static   int          old_cubic_interpolation;
static   int          old_confirm_on_close;
static   int          old_save_session_info;
static   int          old_save_device_status;
static   int          old_always_restore_session;
static   int          old_default_width;
static   int          old_default_height;
static   int          old_default_type;
static   int          old_stingy_memory_use;
static   int          old_tile_cache_size;
static   int          old_install_cmap;
static   int          old_cycled_marching_ants;
static   int          old_last_opened_size;
static   char *       old_temp_path;
static   char *       old_swap_path;
static   char *       old_brush_path;
static   char *       old_pattern_path;
static   char *       old_palette_path;
static   char *       old_plug_in_path;
static   char *       old_gradient_path;
static   float        old_monitor_xres;
static   float        old_monitor_yres;
static   int          old_using_xserver_resolution;
static   int          old_num_processors;
static   char *       old_image_title_format;

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
static   int          edit_last_opened_size;
static   int          edit_num_processors;

static   GtkWidget   *tile_cache_size_spinbutton = NULL;
static   int          divided_tile_cache_size;
static   int          mem_size_unit;
static   GtkWidget   *resolution_xserver_label = NULL;
static   GtkWidget   *resolution_sizeentry = NULL;
static   GtkWidget   *resolution_force_equal = NULL;
static   GtkWidget   *num_processors_spinbutton = NULL;

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
   last-opened-size

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
  edit_tile_cache_size = mem_size_unit * divided_tile_cache_size;

  if (levels_of_undo < 0) 
    {
      g_message (_("Error: Levels of undo must be zero or greater."));
      levels_of_undo = old_levels_of_undo;
      return;
    }
  if (num_processors < 1 || num_processors > 30) 
    {
      g_message (_("Error: Number of processors must be between 1 and 30."));
      num_processors = old_num_processors;
      return;
    }
  if (marching_speed < 50)
    {
      g_message (_("Error: Marching speed must be 50 or greater."));
      marching_speed = old_marching_speed;
      return;
    }
  if (default_width < 1)
    {
      g_message (_("Error: Default width must be one or greater."));
      default_width = old_default_width;
      return;
    }
  if (default_height < 1)
    {
      g_message (_("Error: Default height must be one or greater."));
      default_height = old_default_height;
      return;
    }
  if (monitor_xres < 1e-5 || monitor_yres < 1e-5)
    {
      g_message (_("Error: monitor resolution must not be zero."));
      monitor_xres = old_monitor_xres;
      monitor_yres = old_monitor_yres;
      return;
    }
  if (image_title_format == NULL)
    {
      g_message (_("Error: image_title_format should never be NULL."));
      image_title_format = old_image_title_format;
      return;
    }
      
  gtk_widget_destroy (dlg);
  prefs_dlg = NULL;
  resolution_xserver_label = NULL;
  resolution_sizeentry = NULL;
  resolution_force_equal = NULL;

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
  int save_num_processors;
  int save_install_cmap;
  int save_cycled_marching_ants;
  int save_last_opened_size;
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
  save_last_opened_size = last_opened_size;
  save_temp_path = temp_path;
  save_swap_path = swap_path;
  save_brush_path = brush_path;
  save_pattern_path = pattern_path;
  save_palette_path = palette_path;
  save_plug_in_path = plug_in_path;
  save_gradient_path = gradient_path;
  save_num_processors = num_processors;

  if (levels_of_undo != old_levels_of_undo)
    update = g_list_append (update, "undo-levels");
  if (marching_speed != old_marching_speed)
    update = g_list_append (update, "marching-ants-speed");
  if (last_opened_size != old_last_opened_size)
    update = g_list_append (update, "last-opened-size");
  if (allow_resize_windows != old_allow_resize_windows)
    {
      update = g_list_append (update, "allow-resize-windows");
      remove = g_list_append (remove, "dont-allow-resize-windows");
    }
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
  if (show_rulers != old_show_rulers)
    {
      update = g_list_append (update, "show-rulers");
      remove = g_list_append (remove, "dont-show-rulers");
    }
  if (show_statusbar != old_show_statusbar)
    {
      update = g_list_append (update, "show-statusbar");
      remove = g_list_append (remove, "dont-show-statusbar");
    }
  if (cubic_interpolation != old_cubic_interpolation)
    update = g_list_append (update, "cubic-interpolation");
  if (confirm_on_close != old_confirm_on_close)
    {
      update = g_list_append (update, "confirm-on-close");
      remove = g_list_append (remove, "dont-confirm-on-close");
    }
  if (save_session_info != old_save_session_info)
    {
      update = g_list_append (update, "save-session-info");
      remove = g_list_append (remove, "dont-save-session-info");
    }
  if (save_device_status!= old_save_device_status)
    {
      update = g_list_append (update, "save-device-status");
      remove = g_list_append (remove, "dont-save-device-status");
    }
  if (always_restore_session != old_always_restore_session)
    update = g_list_append (update, "always-restore-session");
  if (default_width != old_default_width ||
      default_height != old_default_height)
    update = g_list_append (update, "default-image-size");
  if (default_type != old_default_type)
    update = g_list_append (update, "default-image-type");
  if (preview_size != old_preview_size)
    update = g_list_append (update, "preview-size");
  if (perfectmouse != old_perfectmouse)
    update = g_list_append (update, "perfect-mouse");
  if (transparency_type != old_transparency_type)
    update = g_list_append (update, "transparency-type");
  if (transparency_size != old_transparency_size)
    update = g_list_append (update, "transparency-size");
  if (using_xserver_resolution != old_using_xserver_resolution ||
      ABS(monitor_xres - old_monitor_xres) > 1e-5)
    update = g_list_append (update, "monitor-xresolution");
  if (using_xserver_resolution != old_using_xserver_resolution ||
      ABS(monitor_yres - old_monitor_yres) > 1e-5)
    update = g_list_append (update, "monitor-yresolution");
  if (edit_num_processors != num_processors)
    update = g_list_append (update, "num-processors");
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
  if (edit_last_opened_size != last_opened_size)
    {
      update = g_list_append (update, "last-opened-size");
      last_opened_size = edit_last_opened_size;
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
  if (using_xserver_resolution)
    {
      /* special value of 0 for either x or y res in the gimprc file
       * means use the xserver's current resolution */
      monitor_xres = 0.0;
      monitor_yres = 0.0;
    }
  if (file_prefs_strcmp (image_title_format, old_image_title_format))
    update = g_list_append (update, "image-title-format");

  save_gimprc (&update, &remove);

  if (using_xserver_resolution)
      gdisplay_xserver_resolution (&monitor_xres, &monitor_yres);

  /* Restore variables which must not change */
  stingy_memory_use = save_stingy_memory_use;
  tile_cache_size = save_tile_cache_size;
  install_cmap = save_install_cmap;
  cycled_marching_ants = save_cycled_marching_ants;
  last_opened_size = save_last_opened_size;
  temp_path = save_temp_path;
  swap_path = save_swap_path;
  brush_path = save_brush_path;
  pattern_path = save_pattern_path;
  palette_path = save_palette_path;
  plug_in_path = save_plug_in_path;
  gradient_path = save_gradient_path;

  if (restart_notification)
    g_message (_("You will need to restart GIMP for these changes to take effect."));
  
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
  resolution_xserver_label = NULL;
  resolution_sizeentry = NULL;
  resolution_force_equal = NULL;

  levels_of_undo = old_levels_of_undo;
  marching_speed = old_marching_speed;
  allow_resize_windows = old_allow_resize_windows;
  auto_save = old_auto_save;
  no_cursor_updating = old_no_cursor_updating;
  perfectmouse = old_perfectmouse;
  show_tool_tips = old_show_tool_tips;
  show_rulers = old_show_rulers;
  show_statusbar = old_show_statusbar;
  cubic_interpolation = old_cubic_interpolation;
  confirm_on_close = old_confirm_on_close;
  save_session_info = old_save_session_info;
  save_device_status = old_save_device_status;
  default_width = old_default_width;
  default_height = old_default_height;
  default_type = old_default_type;
  monitor_xres = old_monitor_xres;
  monitor_yres = old_monitor_yres;
  using_xserver_resolution = old_using_xserver_resolution;
  num_processors = old_num_processors;

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
      gimage_foreach ((GFunc)layer_invalidate_previews, NULL);
      gimage_invalidate_previews ();
      gdisplays_expose_full ();
      gdisplays_flush ();
    }

  edit_stingy_memory_use = old_stingy_memory_use;
  edit_tile_cache_size = old_tile_cache_size;
  edit_install_cmap = old_install_cmap;
  edit_cycled_marching_ants = old_cycled_marching_ants;
  edit_last_opened_size = old_last_opened_size;
  file_prefs_strset (&edit_temp_path, old_temp_path);
  file_prefs_strset (&edit_swap_path, old_swap_path);
  file_prefs_strset (&edit_brush_path, old_brush_path);
  file_prefs_strset (&edit_pattern_path, old_pattern_path);
  file_prefs_strset (&edit_palette_path, old_palette_path);
  file_prefs_strset (&edit_plug_in_path, old_plug_in_path);
  file_prefs_strset (&edit_gradient_path, old_gradient_path);

  file_prefs_strset (&image_title_format, old_image_title_format);
}

static void
file_prefs_toggle_callback (GtkWidget *widget,
			    gpointer   data)
{
  int *val;

  if (data == &allow_resize_windows)
    allow_resize_windows = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &auto_save)
    auto_save = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &no_cursor_updating)
    no_cursor_updating = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &perfectmouse)
    perfectmouse = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &show_tool_tips)
    show_tool_tips = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &show_rulers)
    show_rulers = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &show_statusbar)
    show_statusbar = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &cubic_interpolation)
    cubic_interpolation = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &confirm_on_close)
    confirm_on_close = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &save_session_info)
    save_session_info = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &save_device_status)
    save_device_status = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &always_restore_session)
    always_restore_session = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &edit_stingy_memory_use)
    edit_stingy_memory_use = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &edit_install_cmap)
    edit_install_cmap = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &edit_cycled_marching_ants)
    edit_cycled_marching_ants = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &default_type)
    {
      default_type = (long) gtk_object_get_user_data (GTK_OBJECT (widget));
    } 
  else if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      val = data;
      *val = (long) gtk_object_get_user_data (GTK_OBJECT (widget));
      render_setup (transparency_type, transparency_size);
      gimage_foreach ((GFunc)layer_invalidate_previews, NULL);
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
file_prefs_mem_size_unit_callback (GtkWidget *widget,
				    gpointer   data)
{
  int new_unit;

  new_unit = (int)data;

  if (new_unit != mem_size_unit)
    {
      divided_tile_cache_size = divided_tile_cache_size * mem_size_unit / new_unit;
      mem_size_unit = new_unit;

      gtk_spin_button_set_value (GTK_SPIN_BUTTON (tile_cache_size_spinbutton), (float)divided_tile_cache_size);
    }
}

/*  commented out because it's not used 
static void
file_prefs_text_callback (GtkWidget *widget,
			  gpointer   data)
{
  int *val;
  
  val = data;
  *val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}
*/

static void
file_prefs_spinbutton_callback (GtkWidget *widget,
                                gpointer   data)
{
  int *val;

  val = data;
  *val = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
}

/*  commented out because it's not used 
static void
file_prefs_float_spinbutton_callback (GtkWidget *widget,
				      gpointer   data)
{
  float *val;

  val = data;
  *val = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
}
*/

static void
file_prefs_string_callback (GtkWidget *widget,
			    gpointer   data)
{
  gchar **val;

  val = data;
  file_prefs_strset (val, gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void
file_prefs_clear_session_info_callback (GtkWidget *widget,
					gpointer data)
{
  g_list_free (session_info_updates);
  session_info_updates = NULL;
}

static void
file_prefs_res_source_callback (GtkWidget *widget,
				gpointer data)
{
  if (resolution_xserver_label)
    gtk_widget_set_sensitive (resolution_xserver_label,
			      GTK_TOGGLE_BUTTON (widget)->active);

  if (resolution_sizeentry)
    gtk_widget_set_sensitive (resolution_sizeentry,
			      ! GTK_TOGGLE_BUTTON (widget)->active);

  if (resolution_force_equal)
    gtk_widget_set_sensitive (resolution_force_equal,
			      ! GTK_TOGGLE_BUTTON (widget)->active);

  if (GTK_TOGGLE_BUTTON (widget)->active)
  {
    gdisplay_xserver_resolution (&monitor_xres, &monitor_yres);
    using_xserver_resolution = TRUE;
  }
  else
  {
    if (resolution_sizeentry)
      {
	monitor_xres =
	  gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (resolution_sizeentry), 0);
	monitor_yres =
	  gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (resolution_sizeentry), 1);
      }
    using_xserver_resolution = FALSE;
  }
}

static void
file_prefs_resolution_callback (GtkWidget *widget,
				gpointer data)
{
  static float xres = 0.0;
  static float yres = 0.0;
  float new_xres;
  float new_yres;

  new_xres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_yres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data)))
    {
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
    }
  else
    {
      if (new_xres != xres)
	xres = new_xres;
      if (new_yres != yres)
	yres = new_yres;
    }

  monitor_xres = xres;
  monitor_yres = yres;
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
  GtkWidget *spinbutton;
  GtkWidget *combo;
  GtkWidget *comboitem;
  GtkWidget *menu;
  GtkWidget *menuitem;
  GtkWidget *optionmenu;
  GtkWidget *notebook;
  GtkWidget *table;
  GtkAdjustment *adj;
  GSList *group;
  static const char *transparencies[] =
  {
    N_("Light Checks"),
    N_("Mid-Tone Checks"),
    N_("Dark Checks"),
    N_("White Only"),
    N_("Gray Only"),
    N_("Black Only"),
  };
  static const char *checks[] =
  {
    N_("Small Checks"),
    N_("Medium Checks"),
    N_("Large Checks"),
  };
  static const int transparency_vals[] =
  {
    LIGHT_CHECKS,
    GRAY_CHECKS,
    DARK_CHECKS,
    WHITE_ONLY,
    GRAY_ONLY,
    BLACK_ONLY,
  };
  static const int check_vals[] =
  {
    SMALL_CHECKS,
    MEDIUM_CHECKS,
    LARGE_CHECKS,
  };
  static const struct {
    char *label;
    int unit;
  } mem_size_units[] =
    {
      {N_("Bytes"), 1},
      {N_("KiloBytes"), 1024},
      {N_("MegaBytes"), (1024*1024)}
    };
  static const struct {
    char *label;
    char **mpath;
  } dirs[] =
    {
      {N_("Temp dir:"), &edit_temp_path},
      {N_("Swap dir:"), &edit_swap_path},
      {N_("Brushes dir:"), &edit_brush_path},
      {N_("Gradients dir:"), &edit_gradient_path},
      {N_("Patterns dir:"), &edit_pattern_path},
      {N_("Palette dir:"), &edit_palette_path},
      {N_("Plug-in dir:"), &edit_plug_in_path}
    };
  static const struct {
    char *label;
    int size;
  } preview_sizes[] =
    {
      {N_("None"),0},
      {N_("Small"),32},
      {N_("Medium"),64},
      {N_("Large"),128}
    }; 
  int ntransparencies = sizeof (transparencies) / sizeof (transparencies[0]);
  int nchecks = sizeof (checks) / sizeof (checks[0]);
  int ndirs = sizeof(dirs) / sizeof (dirs[0]);
  int npreview_sizes = sizeof(preview_sizes) / sizeof (preview_sizes[0]); 
  int nmem_size_units = sizeof(mem_size_units) / sizeof (mem_size_units[0]);
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
	  edit_last_opened_size = last_opened_size;
	}
      old_perfectmouse = perfectmouse;
      old_transparency_type = transparency_type;
      old_transparency_size = transparency_size;
      old_levels_of_undo = levels_of_undo;
      old_marching_speed = marching_speed;
      old_allow_resize_windows = allow_resize_windows;
      old_auto_save = auto_save;
      old_preview_size = preview_size;
      old_no_cursor_updating = no_cursor_updating;
      old_show_tool_tips = show_tool_tips;
      old_show_rulers = show_rulers;
      old_show_statusbar = show_statusbar;
      old_cubic_interpolation = cubic_interpolation;
      old_confirm_on_close = confirm_on_close;
      old_save_session_info = save_session_info;
      old_save_device_status = save_device_status;
      old_always_restore_session = always_restore_session;
      old_default_width = default_width;
      old_default_height = default_height;
      old_default_type = default_type;
      old_stingy_memory_use = edit_stingy_memory_use;
      old_tile_cache_size = edit_tile_cache_size;
      old_install_cmap = edit_install_cmap;
      old_cycled_marching_ants = edit_cycled_marching_ants;
      old_last_opened_size = edit_last_opened_size;
      old_monitor_xres = monitor_xres;
      old_monitor_yres = monitor_yres;
      old_using_xserver_resolution = using_xserver_resolution;
      old_num_processors = num_processors;
      old_image_title_format = file_prefs_strdup (image_title_format);	

      file_prefs_strset (&old_temp_path, edit_temp_path);
      file_prefs_strset (&old_swap_path, edit_swap_path);
      file_prefs_strset (&old_brush_path, edit_brush_path);
      file_prefs_strset (&old_pattern_path, edit_pattern_path);
      file_prefs_strset (&old_palette_path, edit_palette_path);
      file_prefs_strset (&old_plug_in_path, edit_plug_in_path);
      file_prefs_strset (&old_gradient_path, edit_gradient_path);

      for (i = 0; i < nmem_size_units; i++)
	{ 
	  if (edit_tile_cache_size % mem_size_units[i].unit == 0)
	    mem_size_unit = mem_size_units[i].unit;
	}
      divided_tile_cache_size = edit_tile_cache_size / mem_size_unit;

      prefs_dlg = gtk_dialog_new ();
      gtk_window_set_wmclass (GTK_WINDOW (prefs_dlg), "preferences", "Gimp");
      gtk_window_set_title (GTK_WINDOW (prefs_dlg), _("Preferences"));

      /* handle the wm close signal */
      gtk_signal_connect (GTK_OBJECT (prefs_dlg), "delete_event",
			  GTK_SIGNAL_FUNC (file_prefs_delete_callback),
			  prefs_dlg);

      gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (prefs_dlg)->action_area), 2);
      
      /* Action area */
      button = gtk_button_new_with_label (_("OK"));
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) file_prefs_ok_callback,
			  prefs_dlg);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dlg)->action_area),
			  button, TRUE, TRUE, 0);
      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Save"));
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) file_prefs_save_callback,
			  prefs_dlg);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dlg)->action_area),
			  button, TRUE, TRUE, 0);
      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Cancel"));
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
      out_frame = gtk_frame_new (_("Display settings"));
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      frame = gtk_frame_new (_("Default image size")); 
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
      
      label = gtk_label_new (_("Width: "));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Height: "));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
      
      adj = (GtkAdjustment *) gtk_adjustment_new (default_width, 1.0,
                                                  32767.0, 1.0, 50.0, 0.0);
      spinbutton = gtk_spin_button_new (adj, 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(spinbutton), GTK_SHADOW_NONE);      
      gtk_widget_set_usize (spinbutton, 50, 0);
      gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, 0, 1,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_signal_connect (GTK_OBJECT (spinbutton), "changed",
                          (GtkSignalFunc) file_prefs_spinbutton_callback,
                          &default_width);
      gtk_widget_show (spinbutton);

      adj = (GtkAdjustment *) gtk_adjustment_new (default_height, 1.0,
                                                  32767.0, 1.0, 50.0, 0.0);
      spinbutton = gtk_spin_button_new (adj, 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(spinbutton), GTK_SHADOW_NONE);      
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spinbutton), TRUE);
      gtk_widget_set_usize (spinbutton, 50, 0);
      gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, 1, 2,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_signal_connect (GTK_OBJECT (spinbutton), "changed",
			  (GtkSignalFunc) file_prefs_spinbutton_callback,
                          &default_height);
      gtk_widget_show (spinbutton);

      frame = gtk_frame_new (_("Default image type"));
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      radio_box = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
      gtk_container_add (GTK_CONTAINER (frame), radio_box);
      gtk_widget_show (radio_box);

      button = gtk_radio_button_new_with_label (NULL, _("RGB"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) RGB);
      if (default_type == RGB)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &default_type);
      gtk_widget_show (button);
      button = gtk_radio_button_new_with_label (group, _("Grayscale"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) GRAY);
      if (last_type == GRAY) 
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                         (GtkSignalFunc) file_prefs_toggle_callback,
			  &default_type);
      gtk_widget_show (button);
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);
      
      label = gtk_label_new (_("Preview size: "));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
      
      menu = gtk_menu_new ();
      for (i = 0; i < npreview_sizes; i++)
        {
          menuitem = gtk_menu_item_new_with_label (gettext(preview_sizes[i].label));
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
	  
      button = gtk_check_button_new_with_label(_("Cubic interpolation"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    cubic_interpolation);
      gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &cubic_interpolation);
      gtk_widget_show (button);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      frame = gtk_frame_new (_("Transparency Type"));
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      radio_box = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
      gtk_container_add (GTK_CONTAINER (frame), radio_box);
      gtk_widget_show (radio_box);

      group = NULL;
      for (i = 0; i < ntransparencies; i++)
        {
	  button = gtk_radio_button_new_with_label (group, gettext(transparencies[i]));
          group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
          gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
          gtk_object_set_user_data (GTK_OBJECT (button),
				    (gpointer) ((long) transparency_vals[i]));
          if (transparency_vals[i] == transparency_type)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
          gtk_signal_connect (GTK_OBJECT (button), "toggled",
                              (GtkSignalFunc) file_prefs_toggle_callback,
                              &transparency_type);
          gtk_widget_show (button);
        }
      frame = gtk_frame_new (_("Check Size"));
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      radio_box = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
      gtk_container_add (GTK_CONTAINER (frame), radio_box);
      gtk_widget_show (radio_box);

      group = NULL;
      for (i = 0; i < nchecks; i++)
        {
          button = gtk_radio_button_new_with_label (group, gettext(checks[i]));
          group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
          gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
          gtk_object_set_user_data (GTK_OBJECT (button),
				    (gpointer) ((long) check_vals[i]));
          if (check_vals[i] == transparency_size)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
          gtk_signal_connect (GTK_OBJECT (button), "toggled",
                              (GtkSignalFunc) file_prefs_toggle_callback,
                              &transparency_size);
          gtk_widget_show (button);
        }
      
      label = gtk_label_new (_("Display"));
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      /* Interface */
      out_frame = gtk_frame_new (_("Interface settings"));
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);

      button = gtk_check_button_new_with_label(_("Resize window on zoom"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    allow_resize_windows);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &allow_resize_windows);
      gtk_widget_show (button);
      
      button = gtk_check_button_new_with_label(_("Perfect-but-slow pointer tracking"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    perfectmouse);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &perfectmouse);
      gtk_widget_show (button);
      
      /* Don't show the Auto-save button until we really 
	 have auto-saving in the gimp.
      
	 button = gtk_check_button_new_with_label(_("Auto save"));
	 gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                       auto_save);
	 gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	 gtk_signal_connect (GTK_OBJECT (button), "toggled",
	                           (GtkSignalFunc) file_prefs_toggle_callback,
                                   &auto_save);
         gtk_widget_show (button);
      */

      button = gtk_check_button_new_with_label(_("Disable cursor updating"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                   no_cursor_updating);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &no_cursor_updating);
      gtk_widget_show (button);

      button = gtk_check_button_new_with_label(_("Show tool tips"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    show_tool_tips);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &show_tool_tips);
      gtk_widget_show (button);

      button = gtk_check_button_new_with_label(_("Show rulers"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    show_rulers);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &show_rulers);
      gtk_widget_show (button);

      button = gtk_check_button_new_with_label(_("Show statusbar"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    show_statusbar);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &show_statusbar);
      gtk_widget_show (button);

      /* The title format string */
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);
      
      label = gtk_label_new (_("Image title format:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      combo = gtk_combo_new();
      gtk_combo_set_use_arrows(GTK_COMBO(combo), FALSE);
      gtk_combo_set_value_in_list(GTK_COMBO(combo), FALSE, FALSE);
      /* Set the currently used string as "Custom" */
      comboitem = gtk_list_item_new_with_label(_("Custom"));
      gtk_combo_set_item_string(GTK_COMBO(combo), GTK_ITEM(comboitem),
				image_title_format);
      gtk_container_add(GTK_CONTAINER(GTK_COMBO(combo)->list), comboitem);
      gtk_widget_show(comboitem);      
      /* set some commonly used format strings */
      comboitem = gtk_list_item_new_with_label(_("Standard"));
      gtk_combo_set_item_string(GTK_COMBO(combo), GTK_ITEM(comboitem),
				"%f-%p.%i (%t)");
      gtk_container_add(GTK_CONTAINER(GTK_COMBO(combo)->list), comboitem);
      gtk_widget_show(comboitem);
      comboitem = gtk_list_item_new_with_label(_("Show zoom percentage"));
      gtk_combo_set_item_string(GTK_COMBO(combo), GTK_ITEM(comboitem),
				"%f-%p.%i (%t) %z%%");
      gtk_container_add(GTK_CONTAINER(GTK_COMBO(combo)->list), comboitem);
      gtk_widget_show(comboitem);
      comboitem = gtk_list_item_new_with_label(_("Show zoom ratio"));
      gtk_combo_set_item_string(GTK_COMBO(combo), GTK_ITEM(comboitem),
				"%f-%p.%i (%t) %d:%s");
      gtk_container_add(GTK_CONTAINER(GTK_COMBO(combo)->list), comboitem);
      gtk_widget_show(comboitem);
      comboitem = gtk_list_item_new_with_label(_("Show reversed zoom ratio"));
      gtk_combo_set_item_string(GTK_COMBO(combo), GTK_ITEM(comboitem),
				"%f-%p.%i (%t) %s:%d");
      gtk_container_add(GTK_CONTAINER(GTK_COMBO(combo)->list), comboitem);
      gtk_widget_show(comboitem);

      gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
      gtk_widget_show(combo);

      gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed",
			 GTK_SIGNAL_FUNC(file_prefs_string_callback), 
			 &image_title_format);
      /* End of the title format string */

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new (_("Levels of undo:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      adj = (GtkAdjustment *) gtk_adjustment_new (levels_of_undo, 0.0,
                                                  255.0, 1.0, 5.0, 0.0);
      spinbutton = gtk_spin_button_new (adj, 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(spinbutton), GTK_SHADOW_NONE);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spinbutton), TRUE);
      gtk_widget_set_usize (spinbutton, 75, 0);
      gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (spinbutton), "changed",
                          (GtkSignalFunc) file_prefs_spinbutton_callback,
                          &levels_of_undo);
      gtk_widget_show (spinbutton);
      
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new (_("Marching ants speed:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      adj = (GtkAdjustment *) gtk_adjustment_new (marching_speed, 0.0,
                                                  32000.0, 50.0, 100.0, 0.0);
      spinbutton = gtk_spin_button_new (adj, 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(spinbutton), GTK_SHADOW_NONE);      
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spinbutton), TRUE);
      gtk_widget_set_usize (spinbutton, 75, 0);
      gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (spinbutton), "changed",
                          (GtkSignalFunc) file_prefs_spinbutton_callback,
                          &marching_speed);
      gtk_widget_show (spinbutton);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new (_("Recent Documents list size:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      adj = (GtkAdjustment *) gtk_adjustment_new (last_opened_size, 0.0,
                                                  256.0, 1.0, 5.0, 0.0);
      spinbutton = gtk_spin_button_new (adj, 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(spinbutton), GTK_SHADOW_NONE);      
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spinbutton), TRUE);
      gtk_widget_set_usize (spinbutton, 75, 0);
      gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (spinbutton), "changed",
                          (GtkSignalFunc) file_prefs_spinbutton_callback,
                          &edit_last_opened_size);
      gtk_widget_show (spinbutton);

      label = gtk_label_new (_("Interface"));
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      /* Environment */
      out_frame = gtk_frame_new (_("Environment settings"));
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_set_usize (out_frame, 320, 200);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);
      
      button = gtk_check_button_new_with_label(_("Conservative memory usage"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    stingy_memory_use);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &edit_stingy_memory_use);
      gtk_widget_show (button);
      
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new (_("Tile cache size:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      adj = (GtkAdjustment *) gtk_adjustment_new (divided_tile_cache_size, 0.0,
                                                  (4069.0 * 1024 * 1024), 1.0,
                                                  16.0, 0.0);
      tile_cache_size_spinbutton = gtk_spin_button_new (adj, 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(tile_cache_size_spinbutton), 
				       GTK_SHADOW_NONE);      
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tile_cache_size_spinbutton), TRUE);
      gtk_widget_set_usize (tile_cache_size_spinbutton, 75, 0);
      gtk_box_pack_start (GTK_BOX (hbox), tile_cache_size_spinbutton, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (tile_cache_size_spinbutton), "changed",
                          (GtkSignalFunc) file_prefs_spinbutton_callback,
                          &divided_tile_cache_size);
      gtk_widget_show (tile_cache_size_spinbutton);
      
      menu = gtk_menu_new ();
      for (i = 0; i < nmem_size_units; i++)
        {
          menuitem = gtk_menu_item_new_with_label (gettext(mem_size_units[i].label));
	  gtk_menu_append (GTK_MENU (menu), menuitem);
	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			      (GtkSignalFunc) file_prefs_mem_size_unit_callback,
			      (gpointer) mem_size_units[i].unit);
	  gtk_widget_show (menuitem);
	}
      optionmenu = gtk_option_menu_new ();
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
      gtk_box_pack_start (GTK_BOX (hbox), optionmenu, FALSE, FALSE, 0);
      gtk_widget_show (optionmenu);
      for (i = 0; i < nmem_size_units; i++)
	if (mem_size_unit == mem_size_units[i].unit)
	  gtk_option_menu_set_history(GTK_OPTION_MENU (optionmenu),i);
      
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new (_("Number of processors to use:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      adj = (GtkAdjustment *) gtk_adjustment_new (num_processors, 1,
                                                  30, 1.0,
                                                  2.0, 0.0);
      num_processors_spinbutton = gtk_spin_button_new (adj, 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(num_processors_spinbutton),
				       GTK_SHADOW_NONE);      
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(num_processors_spinbutton),
				  TRUE);
      gtk_widget_set_usize (num_processors_spinbutton, 75, 0);
      gtk_box_pack_start (GTK_BOX (hbox), num_processors_spinbutton,
			  FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (num_processors_spinbutton), "changed",
                          (GtkSignalFunc) file_prefs_spinbutton_callback,
                          &num_processors);
      gtk_widget_show (num_processors_spinbutton);


      button = gtk_check_button_new_with_label(_("Install colormap (8-bit only)"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    install_cmap);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &edit_install_cmap);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      if (g_visual->depth != 8)
	gtk_widget_set_sensitive (GTK_WIDGET(button), FALSE);
      gtk_widget_show (button);

      button = gtk_check_button_new_with_label(_("Colormap cycling (8-bit only)"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    cycled_marching_ants);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &edit_cycled_marching_ants);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      if (g_visual->depth != 8)
	gtk_widget_set_sensitive (GTK_WIDGET(button), FALSE);
      gtk_widget_show (button);
      
      label = gtk_label_new (_("Environment"));
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      /* Session Management */
      out_frame = gtk_frame_new (_("Session managment"));
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_set_usize (out_frame, 320, 200);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);

      button = gtk_check_button_new_with_label (_("Save window positions on exit"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    save_session_info);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &save_session_info);
      gtk_widget_show (button);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (hbox), 4);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);
      
      button = gtk_button_new_with_label (_("Clear saved window positions"));
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          (GtkSignalFunc) file_prefs_clear_session_info_callback,
                          NULL);
      gtk_widget_show (button);

      button = gtk_check_button_new_with_label (_("Always try to restore session"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    always_restore_session);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &always_restore_session);
      gtk_widget_show (button);

      button = gtk_check_button_new_with_label (_("Save device status on exit"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    save_device_status);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &save_device_status);
      gtk_widget_show (button);
      
      label = gtk_label_new (_("Session"));
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      gtk_widget_show (notebook);

      /* Directories */
      out_frame = gtk_frame_new (_("Directories settings"));
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
	  label = gtk_label_new (gettext(dirs[i].label));
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

      label = gtk_label_new (_("Directories"));
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      /* Monitor */
      out_frame = gtk_frame_new (_("Monitor information"));
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_set_usize (out_frame, 320, 200);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);

      label = gtk_label_new (_("Get monitor resolution"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      group = NULL;
      button = gtk_radio_button_new_with_label (group, _("from X server"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  GTK_SIGNAL_FUNC (file_prefs_res_source_callback),
			  NULL);
      gtk_widget_show (button);

      {
	  float xres, yres;
	  char buf[80];

	  gdisplay_xserver_resolution (&xres, &yres);

	  g_snprintf (buf, sizeof(buf), _("(currently %d x %d dpi)"),
		      (int)(xres + 0.5), (int)(yres + 0.5));
	  resolution_xserver_label = gtk_label_new (buf);
	  gtk_box_pack_start (GTK_BOX (vbox), resolution_xserver_label,
			      FALSE, FALSE, 0);
	  gtk_widget_show (resolution_xserver_label);
      }

      button = gtk_radio_button_new_with_label (group, _("manually:"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
      if (!using_xserver_resolution)
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      abox = gtk_alignment_new (0.5, 0.5, 0.0, 1.0);
      gtk_box_pack_start (GTK_BOX (vbox), abox, FALSE, FALSE, 0);
      gtk_widget_show (abox);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_add (GTK_CONTAINER (abox), vbox);
      gtk_widget_show (vbox);

      resolution_force_equal = gtk_check_button_new_with_label (_("Force equal horizontal and vertical resolutions"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (resolution_force_equal),
				    TRUE);

      resolution_sizeentry = gimp_size_entry_new (2, UNIT_INCH, "%s",
						  FALSE, TRUE, 75,
						  GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (resolution_sizeentry),
					     0, 1, 32767);
      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (resolution_sizeentry),
					     1, 1, 32767);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (resolution_sizeentry), 0,
				  monitor_xres);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (resolution_sizeentry), 1,
				  monitor_yres);
      gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (resolution_sizeentry),
				    _("Horizontal"), 0, 1, 0.0);
      gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (resolution_sizeentry),
				    _("Vertical"), 0, 2, 0.0);
      gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (resolution_sizeentry),
				    _("dpi"), 1, 3, 0.0);
      gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (resolution_sizeentry),
				    _("pixels per "), 2, 3, 0.0);
      gtk_signal_connect (GTK_OBJECT (resolution_sizeentry), "value_changed",
			  (GtkSignalFunc)file_prefs_resolution_callback,
			  resolution_force_equal);
      gtk_signal_connect (GTK_OBJECT (resolution_sizeentry), "refval_changed",
			  (GtkSignalFunc)file_prefs_resolution_callback,
			  resolution_force_equal);
      gtk_box_pack_start (GTK_BOX (vbox), resolution_sizeentry, TRUE, TRUE, 0);
      gtk_widget_show (resolution_sizeentry);
      
      gtk_box_pack_start (GTK_BOX (vbox), resolution_force_equal, TRUE, TRUE, 0);
      gtk_widget_show (resolution_force_equal);
      
      gtk_widget_set_sensitive (resolution_sizeentry, !using_xserver_resolution);
      gtk_widget_set_sensitive (resolution_force_equal,
				!using_xserver_resolution);
      
      label = gtk_label_new (_("Monitor"));
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);
      
      gtk_widget_show (notebook);
      
      gtk_widget_show (prefs_dlg);
    }
}
