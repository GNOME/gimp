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

#include <string.h>

#include "appenv.h"
#include "colormaps.h"
#include "context_manager.h"
#include "gdisplay_ops.h"
#include "gdisplay.h"
#include "gimprc.h"
#include "gimpui.h"
#include "image_render.h"
#include "lc_dialog.h"
#include "layer_select.h"
#include "session.h"
#include "tile_cache.h"

#include "libgimp/gimplimits.h"
#include "libgimp/gimpmath.h"

#include "libgimp/gimpintl.h"

/* gimprc will be parsed with a buffer size of 1024, 
   so don't set this too large */
#define MAX_COMMENT_LENGTH 512

typedef enum
{
  PREFS_OK,
  PREFS_CORRUPT,
  PREFS_RESTART
} PrefsState;


/*  preferences local functions  */
static PrefsState  file_prefs_check_settings        (void);
static void        file_prefs_ok_callback           (GtkWidget *, GtkWidget *);
static void        file_prefs_save_callback         (GtkWidget *, GtkWidget *);
static void        file_prefs_cancel_callback       (GtkWidget *, GtkWidget *);

static void  file_prefs_toggle_callback             (GtkWidget *, gpointer);
static void  file_prefs_preview_size_callback       (GtkWidget *, gpointer);
static void  file_prefs_nav_preview_size_callback   (GtkWidget *, gpointer);
static void  file_prefs_string_callback             (GtkWidget *, gpointer);
static void  file_prefs_text_callback               (GtkWidget *, gpointer);
static void  file_prefs_filename_callback           (GtkWidget *, gpointer);
static void  file_prefs_path_callback               (GtkWidget *, gpointer);
static void  file_prefs_clear_session_info_callback (GtkWidget *, gpointer);
static void  file_prefs_default_size_callback       (GtkWidget *, gpointer);
static void  file_prefs_default_resolution_callback (GtkWidget *, gpointer);
static void  file_prefs_res_source_callback         (GtkWidget *, gpointer);
static void  file_prefs_monitor_resolution_callback (GtkWidget *, gpointer);
static void  file_prefs_restart_notification        (void);


/*  static variables  */
static gint               old_perfectmouse;
static gint               old_transparency_type;
static gint               old_transparency_size;
static gint               old_levels_of_undo;
static gint               old_marching_speed;
static gint               old_allow_resize_windows;
static gint               old_auto_save;
static gint               old_preview_size;
static gint               old_nav_preview_size;
static gint               old_no_cursor_updating;
static gint               old_show_tool_tips;
static gint               old_show_rulers;
static gint               old_show_statusbar;
static InterpolationType  old_interpolation_type;
static gint               old_confirm_on_close;
static gint               old_save_session_info;
static gint               old_save_device_status;
static gint               old_always_restore_session;
static gint               old_default_width;
static gint               old_default_height;
static GimpUnit           old_default_units;
static gdouble            old_default_xresolution;
static gdouble            old_default_yresolution;
static GimpUnit           old_default_resolution_units;
static gint               old_default_type;
static gchar *            old_default_comment;
static gint               old_default_dot_for_dot;
static gint               old_stingy_memory_use;
static gint               old_tile_cache_size;
static gint               old_install_cmap;
static gint               old_cycled_marching_ants;
static gint               old_last_opened_size;
static gchar *            old_temp_path;
static gchar *            old_swap_path;
static gchar *            old_plug_in_path;
static gchar *            old_module_path;
static gchar *            old_brush_path;
static gchar *            old_brush_vbr_path;
static gchar *            old_pattern_path;
static gchar *            old_palette_path;
static gchar *            old_gradient_path;
static gdouble            old_monitor_xres;
static gdouble            old_monitor_yres;
static gint               old_using_xserver_resolution;
static gint               old_num_processors;
static gchar *            old_image_title_format;
static gint               old_global_paint_options;
static gint               old_max_new_image_size;
static gint               old_thumbnail_mode;
static gint 	          old_show_indicators;
static gint	          old_trust_dirty_flag;
static gint               old_use_help;
static gint               old_nav_window_per_display;
static gint               old_info_window_follows_mouse;
static gint               old_help_browser;
static gint               old_default_threshold;

/*  variables which can't be changed on the fly  */
static gint               edit_stingy_memory_use;
static gint               edit_install_cmap;
static gint               edit_cycled_marching_ants;
static gint               edit_last_opened_size;
static gint               edit_num_processors;
static gint               edit_show_indicators;
static gint               edit_nav_window_per_display;
static gint               edit_info_window_follows_mouse;
static gchar *            edit_temp_path      = NULL;
static gchar *            edit_swap_path      = NULL;
static gchar *            edit_brush_path     = NULL;
static gchar *            edit_brush_vbr_path = NULL;
static gchar *            edit_pattern_path   = NULL;
static gchar *            edit_palette_path   = NULL;
static gchar *            edit_gradient_path  = NULL;
static gchar *            edit_plug_in_path   = NULL;
static gchar *            edit_module_path    = NULL;

/*  variables which will be changed _after_ closing the dialog  */
static gint               edit_tile_cache_size;

static GtkWidget *        prefs_dlg = NULL;


/* Some information regarding preferences, compiled by Raph Levien 11/3/97.
   updated by Michael Natterer 27/3/99

   The following preference items cannot be set on the fly (at least
   according to the existing pref code - it may be that changing them
   so they're set on the fly is not hard).

   stingy-memory-use
   install-cmap
   cycled-marching-ants
   last-opened-size
   num-processors
   show-indicators
   nav-window-per-display
   info_window_follows_mouse
   temp-path
   swap-path
   brush-path
   pattern-path
   palette-path
   gradient-path
   plug-in-path
   module-path

   All of these now have variables of the form edit_temp_path, which
   are copied from the actual variables (e.g. temp_path) the first time
   the dialog box is started.

   Variables of the form old_temp_path represent the values at the
   time the dialog is opened - a cancel copies them back from old to
   the real variables or the edit variables, depending on whether they
   can be set on the fly.

   Here are the remaining issues as I see them:

   Still no settings for default-gradient, default-palette,
   gamma-correction.

   No widget for confirm-on-close although a lot of stuff is there.

   The semantics of "save" are a little funny - it only saves the
   settings that are different from the way they were when the dialog
   was opened. So you can set something, close the window, open it
   up again, click "save" and have nothing happen. To change this
   to more intuitive semantics, we should have a whole set of init_
   variables that are set the first time the dialog is opened (along
   with the edit_ variables that are currently set). Then, the save
   callback checks against the init_ variable rather than the old_.
*/

/*  Copy the string from source to destination, freeing the string stored
 *  in the destination if there is one there already.
 */
static void
file_prefs_strset (gchar **dst,
		   gchar  *src)
{
  if (*dst)
    g_free (*dst);

  *dst = g_strdup (src);
}

/*  Duplicate the string, but treat NULL as the empty string.  */
static gchar *
file_prefs_strdup (gchar *src)
{
  return g_strdup (src == NULL ? "" : src);
}

/*  Compare two strings, but treat NULL as the empty string.  */
static int
file_prefs_strcmp (gchar *src1,
		   gchar *src2)
{
  return strcmp (src1 == NULL ? "" : src1,
		 src2 == NULL ? "" : src2);
}

static PrefsState
file_prefs_check_settings (void)
{
  /*  First, check for invalid values...  */
  if (levels_of_undo < 0) 
    {
      g_message (_("Error: Levels of undo must be zero or greater."));
      levels_of_undo = old_levels_of_undo;
      return PREFS_CORRUPT;
    }
  if (marching_speed < 50)
    {
      g_message (_("Error: Marching speed must be 50 or greater."));
      marching_speed = old_marching_speed;
      return PREFS_CORRUPT;
    }
  if (default_width < 1)
    {
      g_message (_("Error: Default width must be one or greater."));
      default_width = old_default_width;
      return PREFS_CORRUPT;
    }
  if (default_height < 1)
    {
      g_message (_("Error: Default height must be one or greater."));
      default_height = old_default_height;
      return PREFS_CORRUPT;
    }
  if (default_units < GIMP_UNIT_INCH ||
      default_units >= gimp_unit_get_number_of_units ())
    {
      g_message (_("Error: Default unit must be within unit range."));
      default_units = old_default_units;
      return PREFS_CORRUPT;
    }
  if (default_xresolution < GIMP_MIN_RESOLUTION ||
      default_yresolution < GIMP_MIN_RESOLUTION)
    {
      g_message (_("Error: Default resolution must not be zero."));
      default_xresolution = old_default_xresolution;
      default_yresolution = old_default_yresolution;
      return PREFS_CORRUPT;
    }
  if (default_resolution_units < GIMP_UNIT_INCH ||
      default_resolution_units >= gimp_unit_get_number_of_units ())
    {
      g_message (_("Error: Default resolution unit must be within unit range."));
      default_resolution_units = old_default_resolution_units;
      return PREFS_CORRUPT;
    }
  if (monitor_xres < GIMP_MIN_RESOLUTION ||
      monitor_yres < GIMP_MIN_RESOLUTION)
    {
      g_message (_("Error: Monitor resolution must not be zero."));
      monitor_xres = old_monitor_xres;
      monitor_yres = old_monitor_yres;
      return PREFS_CORRUPT;
    }
  if (image_title_format == NULL)
    {
      g_message (_("Error: Image title format must not be NULL."));
      image_title_format = old_image_title_format;
      return PREFS_CORRUPT;
    }

  if (edit_num_processors < 1 || edit_num_processors > 30) 
    {
      g_message (_("Error: Number of processors must be between 1 and 30."));
      edit_num_processors = old_num_processors;
      return PREFS_CORRUPT;
    }

  /*  ...then check if we need a restart notification  */
  if (edit_stingy_memory_use         != old_stingy_memory_use         ||
      edit_install_cmap              != old_install_cmap              ||
      edit_cycled_marching_ants      != old_cycled_marching_ants      ||
      edit_last_opened_size          != old_last_opened_size          ||
      edit_num_processors            != old_num_processors            ||
      edit_show_indicators           != old_show_indicators           ||
      edit_nav_window_per_display    != old_nav_window_per_display    ||
      edit_info_window_follows_mouse != old_info_window_follows_mouse ||

      file_prefs_strcmp (old_temp_path,      edit_temp_path)      ||
      file_prefs_strcmp (old_swap_path,      edit_swap_path)      ||
      file_prefs_strcmp (old_brush_path,     edit_brush_path)     ||
      file_prefs_strcmp (old_brush_vbr_path, edit_brush_vbr_path) ||
      file_prefs_strcmp (old_pattern_path,   edit_pattern_path)   ||
      file_prefs_strcmp (old_palette_path,   edit_palette_path)   ||
      file_prefs_strcmp (old_gradient_path,  edit_gradient_path)  ||
      file_prefs_strcmp (old_plug_in_path,   edit_plug_in_path)   ||
      file_prefs_strcmp (old_module_path,    edit_module_path))
    {
      return PREFS_RESTART;
    }

  return PREFS_OK;
}

static void
file_prefs_restart_notification_save_callback (GtkWidget *widget,
					       gpointer   data)
{
  file_prefs_save_callback (widget, prefs_dlg);
  gtk_widget_destroy (GTK_WIDGET (data));
}

/*  The user pressed OK and not Save, but has changed some settings that
 *  only take effect after he restarts the GIMP. Allow him to save the
 *  settings.
 */
static void
file_prefs_restart_notification (void)
{
  GtkWidget *dlg;
  GtkWidget *hbox;
  GtkWidget *label;

  dlg = gimp_dialog_new (_("Save Preferences ?"), "gimp_message",
			 gimp_standard_help_func,
			 "dialogs/preferences/preferences.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, FALSE, FALSE,

			 _("Save"), file_prefs_restart_notification_save_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Close"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, TRUE, FALSE, 4);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("At least one of the changes you made will only\n"
			   "take effect after you restart the GIMP.\n\n"
                           "You may choose 'Save' now to make your changes\n"
			   "permanent, so you can restart GIMP or hit 'Close'\n"
			   "and the critical parts of your changes will not\n"
			   "be applied."));
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 4);
  gtk_widget_show (label);

  gtk_widget_show (dlg);

  gtk_main ();
}

static void
file_prefs_ok_callback (GtkWidget *widget,
			GtkWidget *dlg)
{
  PrefsState state;

  state = file_prefs_check_settings ();
  switch (state)
    {
    case PREFS_CORRUPT:
      return;
      break;

    case PREFS_RESTART:
      gtk_widget_set_sensitive (prefs_dlg, FALSE);
      file_prefs_restart_notification ();
      break;

    case PREFS_OK:
      if (show_tool_tips)
	gimp_help_enable_tooltips ();
      else
	gimp_help_disable_tooltips ();

      if (edit_tile_cache_size != old_tile_cache_size)
	{
	  tile_cache_size = edit_tile_cache_size;
	  tile_cache_set_size (edit_tile_cache_size);
	}
      break;

    default:
      break;
    }

  if (prefs_dlg)
    {
      gtk_widget_destroy (prefs_dlg);
      prefs_dlg = NULL;
    }
}

static void
file_prefs_save_callback (GtkWidget *widget,
			  GtkWidget *dlg)
{
  GList *update = NULL; /*  options that should be updated in .gimprc  */
  GList *remove = NULL; /*  options that should be commented out       */

  PrefsState state;

  gint   save_stingy_memory_use;
  gint   save_install_cmap;
  gint   save_cycled_marching_ants;
  gint   save_last_opened_size;
  gint   save_num_processors;
  gint   save_show_indicators;
  gint   save_nav_window_per_display;
  gint   save_info_window_follows_mouse;
  gchar *save_temp_path;
  gchar *save_swap_path;
  gchar *save_plug_in_path;
  gchar *save_module_path;
  gchar *save_brush_path;
  gchar *save_brush_vbr_path;
  gchar *save_pattern_path;
  gchar *save_palette_path;
  gchar *save_gradient_path;

  state = file_prefs_check_settings ();
  switch (state)
    {
    case PREFS_CORRUPT:
      return;
      break;

    case PREFS_RESTART:
      gtk_widget_set_sensitive (prefs_dlg, FALSE);
      g_message (_("You will need to restart GIMP for these "
		   "changes to take effect."));
      /* don't break */

    case PREFS_OK:
      if (show_tool_tips)
	gimp_help_enable_tooltips ();
      else
	gimp_help_disable_tooltips ();

      if (edit_tile_cache_size != old_tile_cache_size)
	tile_cache_set_size (edit_tile_cache_size);
      break;

    default:
      break;
    }

  gtk_widget_destroy (prefs_dlg);
  prefs_dlg = NULL;

  /*  Save variables so that we can restore them later  */
  save_stingy_memory_use         = stingy_memory_use;
  save_install_cmap              = install_cmap;
  save_cycled_marching_ants      = cycled_marching_ants;
  save_last_opened_size          = last_opened_size;
  save_num_processors            = num_processors;
  save_show_indicators           = show_indicators;
  save_nav_window_per_display    = nav_window_per_display;
  save_info_window_follows_mouse = info_window_follows_mouse;

  save_temp_path      = temp_path;
  save_swap_path      = swap_path;
  save_brush_path     = brush_path;
  save_brush_vbr_path = brush_vbr_path;
  save_pattern_path   = pattern_path;
  save_palette_path   = palette_path;
  save_gradient_path  = gradient_path;
  save_plug_in_path   = plug_in_path;
  save_module_path    = module_path;

  if (levels_of_undo != old_levels_of_undo)
    {
      update = g_list_append (update, "undo-levels");
    }
  if (marching_speed != old_marching_speed)
    {
      update = g_list_append (update, "marching-ants-speed");
    }
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
  if (interpolation_type != old_interpolation_type)
    {
      update = g_list_append (update, "interpolation-type");
    }
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
    {
      update = g_list_append (update, "always-restore-session");
    }
  if (default_width != old_default_width ||
      default_height != old_default_height)
    {
      update = g_list_append (update, "default-image-size");
    }
  if (default_units != old_default_units)
    {
      update = g_list_append (update, "default-units");
    }
  if (ABS (default_xresolution - old_default_xresolution) > GIMP_MIN_RESOLUTION)
    {
      update = g_list_append (update, "default-xresolution");
    }
  if (ABS (default_yresolution - old_default_yresolution) > GIMP_MIN_RESOLUTION)
    {
      update = g_list_append (update, "default-yresolution");
    }
  if (default_resolution_units != old_default_resolution_units)
    {
      update = g_list_append (update, "default-resolution-units");
    }
  if (default_type != old_default_type)
    {
      update = g_list_append (update, "default-image-type");
    }
  if (file_prefs_strcmp (default_comment, old_default_comment))
    {
      update = g_list_append (update, "default-comment");
    }
  if (default_dot_for_dot != old_default_dot_for_dot)
    {
      update = g_list_append (update, "default-dot-for-dot");
      remove = g_list_append (remove, "dont-default-dot-for-dot");
    }
  if (preview_size != old_preview_size)
    {
      update = g_list_append (update, "preview-size");
    }
  if (nav_preview_size != old_nav_preview_size)
    {
      update = g_list_append (update, "nav-preview-size");
    }
  if (perfectmouse != old_perfectmouse)
    {
      update = g_list_append (update, "perfect-mouse");
    }
  if (transparency_type != old_transparency_type)
    {
      update = g_list_append (update, "transparency-type");
    }
  if (transparency_size != old_transparency_size)
    {
      update = g_list_append (update, "transparency-size");
    }
  if (using_xserver_resolution != old_using_xserver_resolution ||
      ABS(monitor_xres - old_monitor_xres) > GIMP_MIN_RESOLUTION)
    {
      update = g_list_append (update, "monitor-xresolution");
    }
  if (using_xserver_resolution != old_using_xserver_resolution ||
      ABS(monitor_yres - old_monitor_yres) > GIMP_MIN_RESOLUTION)
    {
      update = g_list_append (update, "monitor-yresolution");
    }
  if (using_xserver_resolution)
    {
      /* special value of 0 for either x or y res in the gimprc file
       * means use the xserver's current resolution */
      monitor_xres = 0.0;
      monitor_yres = 0.0;
    }
  if (file_prefs_strcmp (image_title_format, old_image_title_format))
    {
      update = g_list_append (update, "image-title-format");
    }
  if (global_paint_options != old_global_paint_options)
    {
      update = g_list_append (update, "global-paint-options");
      remove = g_list_append (remove, "no-global-paint-options");
    }
  if (max_new_image_size != old_max_new_image_size)
    {
      update = g_list_append (update, "max-new-image-size");
    }
  if (thumbnail_mode != old_thumbnail_mode)
    {
      update = g_list_append (update, "thumbnail-mode");
    }
  if (trust_dirty_flag != old_trust_dirty_flag)
    {
      update = g_list_append (update, "trust-dirty-flag");
      remove = g_list_append (update, "dont-trust-dirty-flag");
    }
  if (use_help != old_use_help)
    {
      update = g_list_append (update, "use-help");
      remove = g_list_append (remove, "dont-use-help");
    }
  if (help_browser != old_help_browser)
    {
      update = g_list_append (update, "help-browser");
    }
  if (default_threshold != old_default_threshold)
    {
      update = g_list_append (update, "default-threshold");
    }

  /*  values which can't be changed on the fly  */
  if (edit_stingy_memory_use != old_stingy_memory_use)
    {
      stingy_memory_use = edit_stingy_memory_use;
      update = g_list_append (update, "stingy-memory-use");
    }
  if (edit_install_cmap != old_install_cmap)
    {
      install_cmap = edit_install_cmap;
      update = g_list_append (update, "install-colormap");
    }
  if (edit_cycled_marching_ants != old_cycled_marching_ants)
    {
      cycled_marching_ants = edit_cycled_marching_ants;
      update = g_list_append (update, "colormap-cycling");
    }
  if (edit_last_opened_size != old_last_opened_size)
    {
      last_opened_size = edit_last_opened_size;
      update = g_list_append (update, "last-opened-size");
    }
  if (edit_num_processors != old_num_processors)
    {
      num_processors = edit_num_processors;
      update = g_list_append (update, "num-processors");
    }
  if (edit_show_indicators != old_show_indicators)
    {
      show_indicators = edit_show_indicators;
      update = g_list_append (update, "show-indicators");
      remove = g_list_append (remove, "dont-show-indicators");
    }
  if (edit_nav_window_per_display != old_nav_window_per_display)
    {
      nav_window_per_display = edit_nav_window_per_display;
      update = g_list_append (update, "nav-window-per-display");
      remove = g_list_append (remove, "nav-window-follows-auto");
    }
  if (edit_info_window_follows_mouse != old_info_window_follows_mouse)
    {
      info_window_follows_mouse = edit_info_window_follows_mouse;
      update = g_list_append (update, "info-window-follows-mouse");
      remove = g_list_append (remove, "info-window-per-display");
    }

  if (file_prefs_strcmp (old_temp_path, edit_temp_path))
    {
      temp_path = edit_temp_path;
      update = g_list_append (update, "temp-path");
    }
  if (file_prefs_strcmp (old_swap_path, edit_swap_path))
    {
      swap_path = edit_swap_path;
      update = g_list_append (update, "swap-path");
    }
  if (file_prefs_strcmp (old_brush_path, edit_brush_path))
    {
      brush_path = edit_brush_path;
      update = g_list_append (update, "brush-path");
    }
  if (file_prefs_strcmp (old_brush_vbr_path, edit_brush_vbr_path))
    {
      brush_vbr_path = edit_brush_vbr_path;
      update = g_list_append (update, "brush-vbr-path");
    }
  if (file_prefs_strcmp (old_pattern_path, edit_pattern_path))
    {
      pattern_path = edit_pattern_path;
      update = g_list_append (update, "pattern-path");
    }
  if (file_prefs_strcmp (old_palette_path, edit_palette_path))
    {
      palette_path = edit_palette_path;
      update = g_list_append (update, "palette-path");
    }
  if (file_prefs_strcmp (old_gradient_path, edit_gradient_path))
    {
      gradient_path = edit_gradient_path;
      update = g_list_append (update, "gradient-path");
    }
  if (file_prefs_strcmp (old_plug_in_path, edit_plug_in_path))
    {
      plug_in_path = edit_plug_in_path;
      update = g_list_append (update, "plug-in-path");
    }
  if (file_prefs_strcmp (old_module_path, edit_module_path))
    {
      module_path = edit_module_path;
      update = g_list_append (update, "module-path");
    }

  /*  values which are changed on "OK" or "Save"  */
  if (edit_tile_cache_size != old_tile_cache_size)
    {
      tile_cache_size = edit_tile_cache_size;
      update = g_list_append (update, "tile-cache-size");
    }


  save_gimprc (&update, &remove);


  if (using_xserver_resolution)
    gdisplay_xserver_resolution (&monitor_xres, &monitor_yres);

  /*  restore variables which must not change  */
  stingy_memory_use         = save_stingy_memory_use;
  install_cmap              = save_install_cmap;
  cycled_marching_ants      = save_cycled_marching_ants;
  last_opened_size          = save_last_opened_size;
  num_processors            = save_num_processors;
  show_indicators           = save_show_indicators;
  nav_window_per_display    = save_nav_window_per_display;
  info_window_follows_mouse = save_info_window_follows_mouse;

  temp_path      = save_temp_path;
  swap_path      = save_swap_path;
  brush_path     = save_brush_path;
  brush_vbr_path = save_brush_vbr_path;
  pattern_path   = save_pattern_path;
  palette_path   = save_palette_path;
  gradient_path  = save_gradient_path;
  plug_in_path   = save_plug_in_path;
  module_path    = save_module_path;

  /*  no need to restore values which are only changed on "OK" and "Save"  */

  g_list_free (update);
  g_list_free (remove);
}

static void
file_prefs_cancel_callback (GtkWidget *widget,
			    GtkWidget *dlg)
{
  gtk_widget_destroy (dlg);
  prefs_dlg = NULL;

  /*  restore ordinary gimprc variables  */
  levels_of_undo           = old_levels_of_undo;
  marching_speed           = old_marching_speed;
  allow_resize_windows     = old_allow_resize_windows;
  auto_save                = old_auto_save;
  no_cursor_updating       = old_no_cursor_updating;
  perfectmouse             = old_perfectmouse;
  show_tool_tips           = old_show_tool_tips;
  show_rulers              = old_show_rulers;
  show_statusbar           = old_show_statusbar;
  interpolation_type       = old_interpolation_type;
  confirm_on_close         = old_confirm_on_close;
  save_session_info        = old_save_session_info;
  save_device_status       = old_save_device_status;
  default_width            = old_default_width;
  default_height           = old_default_height;
  default_units            = old_default_units;
  default_xresolution      = old_default_xresolution;
  default_yresolution      = old_default_yresolution;
  default_resolution_units = old_default_resolution_units;
  default_type             = old_default_type;
  default_dot_for_dot      = old_default_dot_for_dot;
  monitor_xres             = old_monitor_xres;
  monitor_yres             = old_monitor_yres;
  using_xserver_resolution = old_using_xserver_resolution;
  max_new_image_size       = old_max_new_image_size;
  thumbnail_mode           = old_thumbnail_mode;
  trust_dirty_flag         = old_trust_dirty_flag;
  use_help                 = old_use_help;
  help_browser             = old_help_browser;
  default_threshold        = old_default_threshold;

  /*  restore variables which need some magic  */
  if (preview_size != old_preview_size)
    {
      lc_dialog_rebuild (old_preview_size);
      layer_select_update_preview_size ();
    }
  if (nav_preview_size != old_nav_preview_size)
    {
      nav_preview_size = old_nav_preview_size;
      gdisplays_nav_preview_resized ();
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

  file_prefs_strset (&image_title_format, old_image_title_format);
  file_prefs_strset (&default_comment, old_default_comment);

  context_manager_set_global_paint_options (old_global_paint_options);

  /*  restore values which need a restart  */
  edit_stingy_memory_use         = old_stingy_memory_use;
  edit_install_cmap              = old_install_cmap;
  edit_cycled_marching_ants      = old_cycled_marching_ants;
  edit_last_opened_size          = old_last_opened_size;
  edit_num_processors            = old_num_processors;
  edit_show_indicators           = old_show_indicators;
  edit_nav_window_per_display    = old_nav_window_per_display;
  edit_info_window_follows_mouse = old_info_window_follows_mouse;

  file_prefs_strset (&edit_temp_path,      old_temp_path);
  file_prefs_strset (&edit_swap_path,      old_swap_path);
  file_prefs_strset (&edit_brush_path,     old_brush_path);
  file_prefs_strset (&edit_brush_vbr_path, old_brush_vbr_path);
  file_prefs_strset (&edit_pattern_path,   old_pattern_path);
  file_prefs_strset (&edit_palette_path,   old_palette_path);
  file_prefs_strset (&edit_gradient_path,  old_gradient_path);
  file_prefs_strset (&edit_plug_in_path,   old_plug_in_path);
  file_prefs_strset (&edit_module_path,    old_module_path);

  /*  no need to restore values which are only changed on "OK" and "Save"  */
}

static void
file_prefs_toggle_callback (GtkWidget *widget,
			    gpointer   data)
{
  gint *val;

  val = (gint *) data;

  /*  toggle buttos  */
  if (data == &allow_resize_windows   ||
      data == &auto_save              ||
      data == &no_cursor_updating     ||
      data == &perfectmouse           ||
      data == &show_tool_tips         ||
      data == &show_rulers            ||
      data == &show_statusbar         ||
      data == &confirm_on_close       ||
      data == &save_session_info      ||
      data == &save_device_status     ||
      data == &always_restore_session ||
      data == &default_dot_for_dot    ||
      data == &use_help               ||

      data == &edit_stingy_memory_use         ||
      data == &edit_install_cmap              ||
      data == &edit_cycled_marching_ants      ||
      data == &edit_show_indicators           ||
      data == &edit_nav_window_per_display    ||
      data == &edit_info_window_follows_mouse)
    {
      *val = GTK_TOGGLE_BUTTON (widget)->active;
    }

  /*  radio buttons  */
  else if (data == &thumbnail_mode     ||
	   data == &interpolation_type ||
           data == &trust_dirty_flag   ||
	   data == &help_browser       ||
	   data == &default_type)
    {
      *val = (gint) gtk_object_get_user_data (GTK_OBJECT (widget));
    }

  /*  values which need some magic  */
  else if ((data == &transparency_type) ||
	   (data == &transparency_size))
    {
      *val = (gint) gtk_object_get_user_data (GTK_OBJECT (widget));

      render_setup (transparency_type, transparency_size);
      gimage_foreach ((GFunc) layer_invalidate_previews, NULL);
      gimage_invalidate_previews ();
      gdisplays_expose_full ();
      gdisplays_flush ();
    }
  else if (data == &global_paint_options)
    {
      context_manager_set_global_paint_options
	(GTK_TOGGLE_BUTTON (widget)->active);
    }

  /*  no matching varible found  */
  else
    {
      /*  Are you a gimp-hacker who is getting this message?  You
       *  probably have to set your preferences value in one of the
       *  branches above...
       */
      g_warning ("Unknown file_prefs_toggle_callback() invoker - ignored.");
    }
}

static void
file_prefs_preview_size_callback (GtkWidget *widget,
                                  gpointer   data)
{
  lc_dialog_rebuild ((long) gtk_object_get_user_data (GTK_OBJECT (widget)));
  layer_select_update_preview_size ();
}

static void
file_prefs_nav_preview_size_callback (GtkWidget *widget,
				      gpointer   data)
{
  nav_preview_size = (gint) gtk_object_get_user_data (GTK_OBJECT (widget));;
  gdisplays_nav_preview_resized ();
}

static void
file_prefs_string_callback (GtkWidget *widget,
			    gpointer   data)
{
  gchar **val;

  val = data;
  file_prefs_strset (val, gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void
file_prefs_text_callback (GtkWidget *widget,
			  gpointer   data)
{
  gchar **val;
  gchar *text;
  
  val = data;

  text = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
  if (strlen (text) > MAX_COMMENT_LENGTH)
    {
      g_message (_("The default comment is limited to %d characters."), 
		 MAX_COMMENT_LENGTH);
      gtk_editable_delete_text (GTK_EDITABLE (widget), MAX_COMMENT_LENGTH, -1);
      g_free (text);
    }
  else
    file_prefs_strset (val, text);
}

static void
file_prefs_filename_callback (GtkWidget *widget,
			      gpointer   data)
{
  gchar **val;
  gchar *filename;

  val = data;
  filename = gimp_file_selection_get_filename (GIMP_FILE_SELECTION (widget));
  file_prefs_strset (val, filename);
  g_free (filename);
}

static void
file_prefs_path_callback (GtkWidget *widget,
			  gpointer   data)
{
  gchar **val;
  gchar  *path;

  val  = (gchar **) data;
  path = gimp_path_editor_get_path (GIMP_PATH_EDITOR (widget));
  file_prefs_strset (val, path);
  g_free (path);
}

static void
file_prefs_clear_session_info_callback (GtkWidget *widget,
					gpointer   data)
{
  g_list_free (session_info_updates);
  session_info_updates = NULL;
}

static void
file_prefs_default_size_callback (GtkWidget *widget,
				  gpointer   data)
{
  default_width =
    (gint) (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0) + 0.5);
  default_height =
    (gint) (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1) + 0.5);
  default_units = gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (widget));
}

static void
file_prefs_default_resolution_callback (GtkWidget *widget,
					gpointer   data)
{
  GtkWidget *size_sizeentry;
  static gdouble xres = 0.0;
  static gdouble yres = 0.0;
  gdouble new_xres;
  gdouble new_yres;

  new_xres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_yres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  size_sizeentry = gtk_object_get_data (GTK_OBJECT (widget), "size_sizeentry");

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (data)))
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

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (size_sizeentry),
				  0, xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (size_sizeentry),
				  1, yres, FALSE);

  default_width = (gint)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size_sizeentry), 0) + 0.5);
  default_height = (gint)
    (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size_sizeentry), 1) + 0.5);
  default_xresolution = xres;
  default_yresolution = yres;
  default_resolution_units = gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (widget));
}

static void
file_prefs_res_source_callback (GtkWidget *widget,
				gpointer   data)
{
  GtkWidget *resolution_xserver_label;
  GtkWidget *monitor_resolution_sizeentry;

  resolution_xserver_label =
    gtk_object_get_data (GTK_OBJECT (widget), "resolution_xserver_label");
  monitor_resolution_sizeentry =
    gtk_object_get_data (GTK_OBJECT (widget), "monitor_resolution_sizeentry");

  gtk_widget_set_sensitive (resolution_xserver_label,
			    GTK_TOGGLE_BUTTON (widget)->active);
  gtk_widget_set_sensitive (monitor_resolution_sizeentry,
			    ! GTK_TOGGLE_BUTTON (widget)->active);

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      gdisplay_xserver_resolution (&monitor_xres, &monitor_yres);
      using_xserver_resolution = TRUE;
    }
  else
    {
      if (monitor_resolution_sizeentry)
	{
	  monitor_xres = gimp_size_entry_get_refval
	    (GIMP_SIZE_ENTRY (monitor_resolution_sizeentry), 0);
	  monitor_yres = gimp_size_entry_get_refval
	    (GIMP_SIZE_ENTRY (monitor_resolution_sizeentry), 1);
	}
      using_xserver_resolution = FALSE;
    }
}

static void
file_prefs_monitor_resolution_callback (GtkWidget *widget,
					gpointer   data)
{
  static gdouble xres = 0.0;
  static gdouble yres = 0.0;
  gdouble new_xres;
  gdouble new_yres;

  new_xres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_yres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (data)))
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

/*  create a new notebook page  */
static GtkWidget *
file_prefs_notebook_append_page (GtkNotebook   *notebook,
				 gchar         *notebook_label,
				 GtkCTree      *ctree,
				 gchar         *tree_label,
				 gchar         *help_data,
				 GtkCTreeNode  *parent,
				 GtkCTreeNode **new_node,
				 gint           page_index)
{
  GtkWidget *event_box;
  GtkWidget *out_vbox;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *label;
  gchar     *titles[1];

  event_box = gtk_event_box_new ();
  gtk_widget_show (event_box);

  gimp_help_set_help_data (event_box, NULL, help_data);

  out_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (event_box), out_vbox);
  gtk_widget_show (out_vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_box_pack_start (GTK_BOX (out_vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = gtk_label_new (notebook_label);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label), 2, 1);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (out_vbox), vbox);
  gtk_widget_show (vbox);

  titles[0] = tree_label;
  *new_node = gtk_ctree_insert_node (ctree, parent, NULL,
				     titles, 0,
				     NULL, NULL, NULL, NULL,
				     FALSE, TRUE);
  gtk_ctree_node_set_row_data (ctree, *new_node, (gpointer) page_index);
  gtk_notebook_append_page (notebook, event_box, NULL);

  return vbox;
}

/*  select a notebook page  */
static void
file_pref_tree_select_callback (GtkWidget    *widget,
				GtkCTreeNode *node)
{
  GtkNotebook *notebook;
  gint         page;

  if (! GTK_CLIST (widget)->selection)
    return;

  notebook = (GtkNotebook*) gtk_object_get_user_data (GTK_OBJECT (widget));
  page = (gint) gtk_ctree_node_get_row_data (GTK_CTREE (widget), node);

  gtk_notebook_set_page (notebook, page);
}

/*  create a frame with title and a vbox  */
static GtkWidget *
file_prefs_frame_new (gchar  *label,
		      GtkBox *vbox)
{
  GtkWidget *frame;
  GtkWidget *vbox2;

  frame = gtk_frame_new (label);
  gtk_box_pack_start (vbox, frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  return vbox2;
}

static void
file_prefs_help_func (gchar *help_data)
{
  GtkWidget *notebook;
  GtkWidget *event_box;
  gint page_num;

  notebook = gtk_object_get_user_data (GTK_OBJECT (prefs_dlg));
  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
  event_box = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), page_num);

  help_data = gtk_object_get_data (GTK_OBJECT (event_box), "gimp_help_data");
  gimp_help (help_data);
}

/************************************************************************
 *  create the preferences dialog
 */
void
file_pref_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GtkWidget    *ctree;
  gchar        *titles[1];
  GtkCTreeNode *top_insert;
  GtkCTreeNode *child_insert;
  gint          page_index;

  GtkWidget *frame;
  GtkWidget *notebook;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *abox;
  GtkWidget *button;
  GtkWidget *fileselection;
  GtkWidget *patheditor;
  GtkWidget *spinbutton;
  GtkWidget *combo;
  GtkWidget *comboitem;
  GtkWidget *optionmenu;
  GtkWidget *table;
  GtkWidget *label;
  GtkObject *adjustment;
  GtkWidget *sizeentry;
  GtkWidget *sizeentry2;
  GtkWidget *text;
  GSList    *group;

  gint i;
  gchar *pixels_per_unit;

  if (prefs_dlg)
    {
      gdk_window_raise (GTK_WIDGET (prefs_dlg)->window);
      return;
    }

  if (edit_temp_path == NULL)
    {
      /*  first time dialog is opened -
       *  copy config vals to edit variables.
       */
      edit_stingy_memory_use         = stingy_memory_use;
      edit_install_cmap              = install_cmap;
      edit_cycled_marching_ants      = cycled_marching_ants;
      edit_last_opened_size          = last_opened_size;
      edit_num_processors            = num_processors;
      edit_show_indicators           = show_indicators;
      edit_nav_window_per_display    = nav_window_per_display;
      edit_info_window_follows_mouse = info_window_follows_mouse;

      edit_temp_path      = file_prefs_strdup (temp_path);	
      edit_swap_path      = file_prefs_strdup (swap_path);
      edit_plug_in_path   = file_prefs_strdup (plug_in_path);
      edit_module_path    = file_prefs_strdup (module_path);
      edit_brush_path     = file_prefs_strdup (brush_path);
      edit_brush_vbr_path = file_prefs_strdup (brush_vbr_path);
      edit_pattern_path   = file_prefs_strdup (pattern_path);
      edit_palette_path   = file_prefs_strdup (palette_path);
      edit_gradient_path  = file_prefs_strdup (gradient_path);
    }

  /*  assign edit variables for values which get changed on "OK" and "Save"
   *  but not on the fly.
   */
  edit_tile_cache_size = tile_cache_size;

  /*  remember all old values  */
  old_perfectmouse             = perfectmouse;
  old_transparency_type        = transparency_type;
  old_transparency_size        = transparency_size;
  old_levels_of_undo           = levels_of_undo;
  old_marching_speed           = marching_speed;
  old_allow_resize_windows     = allow_resize_windows;
  old_auto_save                = auto_save;
  old_preview_size             = preview_size;
  old_nav_preview_size         = nav_preview_size;
  old_no_cursor_updating       = no_cursor_updating;
  old_show_tool_tips           = show_tool_tips;
  old_show_rulers              = show_rulers;
  old_show_statusbar           = show_statusbar;
  old_interpolation_type       = interpolation_type;
  old_confirm_on_close         = confirm_on_close;
  old_save_session_info        = save_session_info;
  old_save_device_status       = save_device_status;
  old_always_restore_session   = always_restore_session;
  old_default_width            = default_width;
  old_default_height           = default_height;
  old_default_units            = default_units;
  old_default_xresolution      = default_xresolution;
  old_default_yresolution      = default_yresolution;
  old_default_resolution_units = default_resolution_units;
  old_default_type             = default_type;
  old_default_dot_for_dot      = default_dot_for_dot;
  old_monitor_xres             = monitor_xres;
  old_monitor_yres             = monitor_yres;
  old_using_xserver_resolution = using_xserver_resolution;
  old_global_paint_options     = global_paint_options;
  old_max_new_image_size       = max_new_image_size;
  old_thumbnail_mode           = thumbnail_mode;
  old_trust_dirty_flag         = trust_dirty_flag;
  old_use_help                 = use_help;
  old_help_browser             = help_browser;
  old_default_threshold        = default_threshold;

  file_prefs_strset (&old_image_title_format, image_title_format);	
  file_prefs_strset (&old_default_comment, default_comment);	

  /*  values which will need a restart  */
  old_stingy_memory_use         = edit_stingy_memory_use;
  old_install_cmap              = edit_install_cmap;
  old_cycled_marching_ants      = edit_cycled_marching_ants;
  old_last_opened_size          = edit_last_opened_size;
  old_num_processors            = edit_num_processors;
  old_show_indicators           = edit_show_indicators;
  old_nav_window_per_display    = edit_nav_window_per_display;
  old_info_window_follows_mouse = edit_info_window_follows_mouse;

  file_prefs_strset (&old_temp_path,      edit_temp_path);
  file_prefs_strset (&old_swap_path,      edit_swap_path);
  file_prefs_strset (&old_plug_in_path,   edit_plug_in_path);
  file_prefs_strset (&old_module_path,    edit_module_path);
  file_prefs_strset (&old_brush_path,     edit_brush_path);
  file_prefs_strset (&old_brush_vbr_path, edit_brush_vbr_path);
  file_prefs_strset (&old_pattern_path,   edit_pattern_path);
  file_prefs_strset (&old_palette_path,   edit_palette_path);
  file_prefs_strset (&old_gradient_path,  edit_gradient_path);

  /*  values which will be changed on "OK" and "Save"  */
  old_tile_cache_size = edit_tile_cache_size;

  /* Create the dialog */
  prefs_dlg =
    gimp_dialog_new (_("Preferences"), "gimp_preferences",
		     file_prefs_help_func,
		     "dialogs/preferences/preferences.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     _("OK"), file_prefs_ok_callback,
		     NULL, NULL, NULL, FALSE, FALSE,
		     _("Save"), file_prefs_save_callback,
		     NULL, NULL, NULL, FALSE, FALSE,
		     _("Cancel"), file_prefs_cancel_callback,
		     NULL, NULL, NULL, TRUE, TRUE,

		     NULL);

  /* The main hbox */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (prefs_dlg)->vbox), hbox);
  gtk_widget_show (hbox);

  /* The categories tree */
  titles[0] = _("Categories");
  ctree = gtk_ctree_new_with_titles (1, 0, titles);
  gtk_ctree_set_indent (GTK_CTREE (ctree), 15);
  gtk_clist_column_titles_passive (GTK_CLIST (ctree));
  gtk_box_pack_start (GTK_BOX (hbox), ctree, FALSE, FALSE, 0);

  /* The main preferences notebook */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_container_add (GTK_CONTAINER (frame), notebook);

  gtk_object_set_user_data (GTK_OBJECT (prefs_dlg), notebook);
  gtk_object_set_user_data (GTK_OBJECT (ctree), notebook);

  gtk_signal_connect (GTK_OBJECT (ctree), "tree_select_row",
		      GTK_SIGNAL_FUNC (file_pref_tree_select_callback),
		      NULL);

  page_index = 0;

  /* New File page */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("New File"),
					  GTK_CTREE (ctree),
					  _("New File"),
					  "dialogs/preferences/new_file.html",
					  NULL,
					  &top_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  /* select this page in the tree */
  gtk_ctree_select (GTK_CTREE (ctree), top_insert);

  frame = gtk_frame_new (_("Default Image Size and Unit")); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  sizeentry =
    gimp_size_entry_new (2, default_units, "%p", FALSE, FALSE, TRUE, 75,
			 GIMP_SIZE_ENTRY_UPDATE_SIZE);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Width"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Height"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Pixels"), 1, 4, 0.0);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry),
				  0, default_xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry),
				  1, default_yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries
    (GIMP_SIZE_ENTRY (sizeentry), 0, GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries
    (GIMP_SIZE_ENTRY (sizeentry), 1, GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0, default_width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 1, default_height);

  gtk_signal_connect (GTK_OBJECT (sizeentry), "unit_changed",
		      GTK_SIGNAL_FUNC (file_prefs_default_size_callback),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (sizeentry), "value_changed",
		      GTK_SIGNAL_FUNC (file_prefs_default_size_callback),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (sizeentry), "refval_changed",
		      GTK_SIGNAL_FUNC (file_prefs_default_size_callback),
		      NULL);

  gtk_box_pack_start (GTK_BOX (hbox), sizeentry, FALSE, FALSE, 0);
  gtk_widget_show (sizeentry);

  frame = gtk_frame_new (_("Default Image Resolution and Resolution Unit"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  pixels_per_unit = g_strconcat (_("Pixels"), "/%s", NULL);

  sizeentry2 =
    gimp_size_entry_new (2, default_resolution_units, pixels_per_unit,
			 FALSE, FALSE, TRUE, 75,
			 GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);

  button = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
  if (ABS (default_xresolution - default_yresolution) < GIMP_MIN_RESOLUTION)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (button), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (sizeentry2), button, 1, 3, 3, 4);
  gtk_widget_show (button);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry2),
				_("Horizontal"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry2),
				_("Vertical"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry2),
				_("dpi"), 1, 4, 0.0);

  gtk_object_set_data (GTK_OBJECT (sizeentry2), "size_sizeentry", sizeentry);

  gimp_size_entry_set_refval_boundaries
    (GIMP_SIZE_ENTRY (sizeentry2), 0, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval_boundaries
    (GIMP_SIZE_ENTRY (sizeentry2), 1, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry2),
			      0, default_xresolution);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry2),
			      1, default_yresolution);

  gtk_signal_connect (GTK_OBJECT (sizeentry2), "unit_changed",
		      (GtkSignalFunc) file_prefs_default_resolution_callback,
		      button);
  gtk_signal_connect (GTK_OBJECT (sizeentry2), "value_changed",
		      (GtkSignalFunc) file_prefs_default_resolution_callback,
		      button);
  gtk_signal_connect (GTK_OBJECT (sizeentry2), "refval_changed",
		      (GtkSignalFunc) file_prefs_default_resolution_callback,
		      button);

  gtk_box_pack_start (GTK_BOX (hbox), sizeentry2, FALSE, FALSE, 0);
  gtk_widget_show (sizeentry2);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
      
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  optionmenu =
    gimp_option_menu_new2 (FALSE, file_prefs_toggle_callback,
			   &default_type, (gpointer) default_type,

			   _("RGB"),       (gpointer) RGB, NULL,
			   _("Grayscale"), (gpointer) GRAY, NULL,

			   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Default Image Type:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  /*  The maximum size of a new image  */  
  adjustment = gtk_adjustment_new (max_new_image_size, 
				   0, (4069.0 * 1024 * 1024), 1.0, 1.0, 0.0);
  hbox = gimp_mem_size_entry_new (GTK_ADJUSTMENT (adjustment));
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &max_new_image_size);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Maximum Image Size:"), 1.0, 0.5,
			     hbox, 1, TRUE);

  /* Default Comment page */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Default Comment"),
					  GTK_CTREE (ctree),
					  _("Default Comment"),
					  "dialogs/preferences/default_comment.html",
					  top_insert,
					  &child_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  frame = gtk_frame_new (_("Comment Used for New Images"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);
 
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  text = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (text), TRUE);
  gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, default_comment, -1);
  gtk_signal_connect (GTK_OBJECT (text), "changed",
		      GTK_SIGNAL_FUNC (file_prefs_text_callback),
		      &default_comment);
  gtk_container_add (GTK_CONTAINER (hbox), text);
  gtk_widget_show (text);

  /* Display page */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Display"),
					  GTK_CTREE (ctree),
					  _("Display"),
					  "dialogs/preferences/display.html",
					  NULL,
					  &top_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  frame = gtk_frame_new (_("Transparency")); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  optionmenu =
    gimp_option_menu_new2 (FALSE, file_prefs_toggle_callback,
			   &transparency_type, (gpointer) transparency_type,

			   _("Light Checks"),    (gpointer) LIGHT_CHECKS, NULL,
			   _("Mid-Tone Checks"), (gpointer) GRAY_CHECKS, NULL,
			   _("Dark Checks"),     (gpointer) DARK_CHECKS, NULL,
			   _("White Only"),      (gpointer) WHITE_ONLY, NULL,
			   _("Gray Only"),       (gpointer) GRAY_ONLY, NULL,
			   _("Black Only"),      (gpointer) BLACK_ONLY, NULL,

			   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Transparency Type:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  optionmenu =
    gimp_option_menu_new2 (FALSE, file_prefs_toggle_callback,
			   &transparency_size, (gpointer) transparency_size,

			   _("Small"),  (gpointer) SMALL_CHECKS, NULL,
			   _("Medium"), (gpointer) MEDIUM_CHECKS, NULL,
			   _("Large"),  (gpointer) LARGE_CHECKS, NULL,

			   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Check Size:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  vbox2 = file_prefs_frame_new (_("8-Bit Displays"), GTK_BOX (vbox));

  if (g_visual->depth != 8)
    gtk_widget_set_sensitive (GTK_WIDGET (vbox2->parent), FALSE);

  button = gtk_check_button_new_with_label(_("Install Colormap"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_install_cmap);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &edit_install_cmap);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_label(_("Colormap Cycling"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_cycled_marching_ants);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &edit_cycled_marching_ants);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);


  /* Interface */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Interface"),
					  GTK_CTREE (ctree),
					  _("Interface"),
					  "dialogs/preferences/interface.html",
					  NULL,
					  &top_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = file_prefs_frame_new (_("General"), GTK_BOX (vbox));

  table = gtk_table_new (4, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Don't show the Auto-save button until we really 
     have auto-saving in the gimp.

     button = gtk_check_button_new_with_label(_("Auto save"));
     gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                   auto_save);
     gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
     gtk_signal_connect (GTK_OBJECT (button), "toggled",
	                 GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
			 &auto_save);
     gtk_widget_show (button);
  */

  optionmenu =
    gimp_option_menu_new2 (FALSE, file_prefs_preview_size_callback,
			   &preview_size, (gpointer) preview_size,

			   _("None"),   (gpointer)   0, NULL,
			   _("Tiny"),   (gpointer)  24, NULL,
			   _("Small"),  (gpointer)  32, NULL,
			   _("Medium"), (gpointer)  48, NULL,
			   _("Large"),  (gpointer)  64, NULL,
			   _("Huge"),   (gpointer) 128, NULL,

			   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Preview Size:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  optionmenu =
    gimp_option_menu_new2 (FALSE, file_prefs_nav_preview_size_callback,
			   &nav_preview_size, (gpointer) nav_preview_size,

			   _("Small"),  (gpointer)  48, NULL,
			   _("Medium"), (gpointer)  80, NULL,
			   _("Large"),  (gpointer) 112, NULL,

			   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Nav Preview Size:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  spinbutton =
    gimp_spin_button_new (&adjustment, edit_last_opened_size,
			  0.0, 16.0, 1.0, 5.0, 0.0, 1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &edit_last_opened_size);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Recent Documents List Size:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  /* Indicators */
  vbox2 = file_prefs_frame_new (_("Toolbox"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label
    (_("Display Brush, Pattern and Gradient Indicators"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_show_indicators);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &edit_show_indicators);
  gtk_widget_show (button);

  vbox2 = file_prefs_frame_new (_("Dialog Behaviour"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label (_("Navigation Window per Display"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_nav_window_per_display);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &edit_nav_window_per_display);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_label (_("Info Window Follows Mouse"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_info_window_follows_mouse);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &edit_info_window_follows_mouse);
  gtk_widget_show (button);

  /* Interface / Help System */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Help System"),
					  GTK_CTREE (ctree),
					  _("Help System"),
					  "dialogs/preferences/interface.html#help_system",
					  top_insert,
					  &child_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = file_prefs_frame_new (_("General"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label (_("Show Tool Tips"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				show_tool_tips);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &show_tool_tips);
  gtk_widget_show (button);

  button =
    gtk_check_button_new_with_label (_("Context Sensitive Help with \"F1\""));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				use_help);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &use_help);
  gtk_widget_show (button);

  vbox2 = file_prefs_frame_new (_("Help Browser"), GTK_BOX (vbox));

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  optionmenu =
    gimp_option_menu_new2 (FALSE, file_prefs_toggle_callback,
			   &help_browser, (gpointer) help_browser,

			   _("Internal"), (gpointer) HELP_BROWSER_GIMP, NULL,
			   _("Netscape"), (gpointer) HELP_BROWSER_NETSCAPE, NULL,

			   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Help Browser to Use:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  /* Interface / Image Windows */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Image Windows"),
					  GTK_CTREE (ctree),
					  _("Image Windows"),
					  "dialogs/preferences/interface.html#image_windows",
					  top_insert,
					  &child_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = file_prefs_frame_new (_("Appearance"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label (_("Use \"Dot for Dot\" by default"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				default_dot_for_dot);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &default_dot_for_dot);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_label(_("Resize Window on Zoom"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				allow_resize_windows);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &allow_resize_windows);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_label(_("Show Rulers"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				show_rulers);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &show_rulers);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_label(_("Show Statusbar"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				show_statusbar);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &show_statusbar);
  gtk_widget_show (button);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  spinbutton =
    gimp_spin_button_new (&adjustment,
			  marching_speed, 50.0, 32000.0, 10.0, 100.0, 1.0,
			  1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &marching_speed);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Marching Ants Speed:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  /* The title format string */
  combo = gtk_combo_new ();
  gtk_combo_set_use_arrows (GTK_COMBO (combo), FALSE);
  gtk_combo_set_value_in_list (GTK_COMBO (combo), FALSE, FALSE);
  /* Set the currently used string as "Custom" */
  comboitem = gtk_list_item_new_with_label (_("Custom"));
  gtk_combo_set_item_string (GTK_COMBO (combo), GTK_ITEM (comboitem),
			     image_title_format);
  gtk_container_add (GTK_CONTAINER (GTK_COMBO (combo)->list), comboitem);
  gtk_widget_show (comboitem);
  /* set some commonly used format strings */
  comboitem = gtk_list_item_new_with_label (_("Standard"));
  gtk_combo_set_item_string (GTK_COMBO (combo), GTK_ITEM (comboitem),
			     "%f-%p.%i (%t)");
  gtk_container_add(GTK_CONTAINER (GTK_COMBO (combo)->list), comboitem);
  gtk_widget_show (comboitem);
  comboitem = gtk_list_item_new_with_label (_("Show zoom percentage"));
  gtk_combo_set_item_string (GTK_COMBO (combo), GTK_ITEM (comboitem),
			     "%f-%p.%i (%t) %z%%");
  gtk_container_add (GTK_CONTAINER (GTK_COMBO (combo)->list), comboitem);
  gtk_widget_show (comboitem);
  comboitem = gtk_list_item_new_with_label (_("Show zoom ratio"));
  gtk_combo_set_item_string (GTK_COMBO (combo), GTK_ITEM (comboitem),
			     "%f-%p.%i (%t) %d:%s");
  gtk_container_add (GTK_CONTAINER (GTK_COMBO (combo)->list), comboitem);
  gtk_widget_show (comboitem);
  comboitem = gtk_list_item_new_with_label (_("Show reversed zoom ratio"));
  gtk_combo_set_item_string (GTK_COMBO (combo), GTK_ITEM (comboitem),
			     "%f-%p.%i (%t) %s:%d");
  gtk_container_add (GTK_CONTAINER (GTK_COMBO (combo)->list), comboitem);
  gtk_widget_show (comboitem);

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry), "changed",
		      GTK_SIGNAL_FUNC (file_prefs_string_callback), 
		      &image_title_format);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Image Title Format:"), 1.0, 0.5,
			     combo, 1, FALSE);
  /* End of the title format string */

  vbox2 = file_prefs_frame_new (_("Pointer Movement Feedback"), GTK_BOX (vbox));

  button =
    gtk_check_button_new_with_label (_("Perfect-but-Slow Pointer Tracking"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				perfectmouse);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &perfectmouse);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_label (_("Disable Cursor Updating"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				no_cursor_updating);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &no_cursor_updating);
  gtk_widget_show (button);


  /* Interface / Tool Options */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Tool Options"),
					  GTK_CTREE (ctree),
					  _("Tool Options"),
					  "dialogs/preferences/interface.html#tool_options",
					  top_insert,
					  &child_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = file_prefs_frame_new (_("Paint Options"), GTK_BOX (vbox));

  button =
    gtk_check_button_new_with_label(_("Use Global Paint Options"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				global_paint_options);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &global_paint_options);
  gtk_widget_show (button);

  vbox2 = file_prefs_frame_new (_("Finding Contiguous Regions"),
				GTK_BOX (vbox));

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Default threshold  */
  spinbutton =
    gimp_spin_button_new (&adjustment,
			  default_threshold, 0.0, 255.0,
			  1.0, 5.0, 0.0, 1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &default_threshold);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Default Threshold:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  /* Expand the "Interface" branch */
  gtk_ctree_expand (GTK_CTREE (ctree), top_insert);


  /* Environment */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Environment"),
					  GTK_CTREE (ctree),
					  _("Environment"),
					  "dialogs/preferences/environment.html",
					  NULL,
					  &top_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = file_prefs_frame_new (_("Resource Consumption"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label(_("Conservative Memory Usage"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_stingy_memory_use);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &edit_stingy_memory_use);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

#ifdef ENABLE_MP
  table = gtk_table_new (3, 2, FALSE);
#else
  table = gtk_table_new (2, 2, FALSE);
#endif /* ENABLE_MP */
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Levels of Undo  */
  spinbutton =
    gimp_spin_button_new (&adjustment,
			  levels_of_undo, 0.0, 255.0, 1.0, 5.0, 0.0, 1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &levels_of_undo);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Levels of Undo:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  /*  The tile cache size  */
  adjustment = gtk_adjustment_new (edit_tile_cache_size, 
				   0, (4069.0 * 1024 * 1024), 1.0, 1.0, 0.0);
  hbox = gimp_mem_size_entry_new (GTK_ADJUSTMENT (adjustment));
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &edit_tile_cache_size);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Tile Cache Size:"), 1.0, 0.5,
			     hbox, 1, TRUE);

#ifdef ENABLE_MP
  spinbutton =
    gimp_spin_button_new (&adjustment, edit_num_processors,
			  1, 30, 1.0, 2.0, 0.0, 1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &edit_num_processors);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Number of Processors to Use:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
#endif /* ENABLE_MP */

  frame = gtk_frame_new (_("Scaling")); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  optionmenu =
    gimp_option_menu_new2 (FALSE, file_prefs_toggle_callback,
			   &interpolation_type, (gpointer) interpolation_type,

			   _("Nearest Neighbor (Fast)"),
			   (gpointer) NEAREST_NEIGHBOR_INTERPOLATION, NULL,
			   _("Linear"),
			   (gpointer) LINEAR_INTERPOLATION, NULL,
			   _("Cubic (Slow)"),
			   (gpointer) CUBIC_INTERPOLATION, NULL,

			   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Interpolation Type:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  vbox2 = file_prefs_frame_new (_("File Saving"), GTK_BOX (vbox));

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
      
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  optionmenu =
    gimp_option_menu_new2 (FALSE, file_prefs_toggle_callback,
			   &thumbnail_mode, (gpointer) thumbnail_mode,

			   _("Always"), (gpointer) 1, NULL,
			   _("Never"),  (gpointer) 0, NULL,

			   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Try to Write a Thumbnail File:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  optionmenu =
    gimp_option_menu_new2 (FALSE, file_prefs_toggle_callback,
			   &trust_dirty_flag, (gpointer) trust_dirty_flag,

			   _("Only when Modified"), (gpointer) 1, NULL,
			   _("Always"),             (gpointer) 0, NULL,

			   NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("\"File > Save\" Saves the Image:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);
                                     

  /* Session Management */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Session Management"),
					  GTK_CTREE (ctree),
					  _("Session"),
					  "dialogs/preferences/session.html",
					  NULL,
					  &top_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = file_prefs_frame_new (_("Window Positions"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label (_("Save Window Positions on Exit"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				save_session_info);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &save_session_info);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
      
  button = gtk_button_new_with_label (_("Clear Saved Window Positions Now"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (file_prefs_clear_session_info_callback),
		      NULL);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_label (_("Always Try to Restore Session"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				always_restore_session);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &always_restore_session);
  gtk_widget_show (button);

  vbox2 = file_prefs_frame_new (_("Devices"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label (_("Save Device Status on Exit"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				save_device_status);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &save_device_status);
  gtk_widget_show (button);

  /* Monitor */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Monitor"),
					  GTK_CTREE (ctree),
					  _("Monitor"),
					  "dialogs/preferences/monitor.html",
					  NULL,
					  &top_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = file_prefs_frame_new (_("Get Monitor Resolution"), GTK_BOX (vbox));

  {
    gdouble  xres, yres;
    gchar   *str;

    gdisplay_xserver_resolution (&xres, &yres);

    str = g_strdup_printf (_("(Currently %d x %d dpi)"),
			   ROUND (xres), ROUND (yres));
    label = gtk_label_new (str);
    g_free (str);
  }

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

  sizeentry =
    gimp_size_entry_new (2, GIMP_UNIT_INCH, pixels_per_unit,
			 FALSE, FALSE, TRUE, 75,
			 GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
  
  g_free (pixels_per_unit);
  pixels_per_unit = NULL;

  button = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
  if (ABS (monitor_xres - monitor_yres) < GIMP_MIN_RESOLUTION)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (button), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (sizeentry), button, 1, 3, 3, 4);
  gtk_widget_show (button);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Horizontal"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Vertical"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("dpi"), 1, 4, 0.0);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 0,
					 GIMP_MIN_RESOLUTION,
					 GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 1,
					 GIMP_MIN_RESOLUTION,
					 GIMP_MAX_RESOLUTION);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0, monitor_xres);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 1, monitor_yres);

  gtk_signal_connect (GTK_OBJECT (sizeentry), "value_changed",
		      GTK_SIGNAL_FUNC (file_prefs_monitor_resolution_callback),
		      button);
  gtk_signal_connect (GTK_OBJECT (sizeentry), "refval_changed",
		      GTK_SIGNAL_FUNC (file_prefs_monitor_resolution_callback),
		      button);

  gtk_container_add (GTK_CONTAINER (abox), sizeentry);
  gtk_widget_show (sizeentry);

  gtk_widget_set_sensitive (sizeentry, !using_xserver_resolution);

  group = NULL;
  button = gtk_radio_button_new_with_label (group, _("From X Server"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_res_source_callback),
		      NULL);
  gtk_object_set_data (GTK_OBJECT (button), "resolution_xserver_label",
		       label);
  gtk_object_set_data (GTK_OBJECT (button), "monitor_resolution_sizeentry",
		       sizeentry);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  button = gtk_radio_button_new_with_label (group, _("Manually:"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (vbox2), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  if (!using_xserver_resolution)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  /* Directories */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Directories"),
					  GTK_CTREE (ctree),
					  _("Directories"),
					  "dialogs/preferences/directories.html",
					  NULL,
					  &top_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  {
    static const struct
    {
      gchar  *label;
      gchar  *fs_label;
      gchar **mdir;
    }
    dirs[] =
    {
      { N_("Temp Dir:"), N_("Select Temp Dir"), &edit_temp_path },
      { N_("Swap Dir:"), N_("Select Swap Dir"), &edit_swap_path },
    };
    static gint ndirs = sizeof (dirs) / sizeof (dirs[0]);

    table = gtk_table_new (ndirs + 1, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 2);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
    gtk_widget_show (table);

    for (i = 0; i < ndirs; i++)
      {
	fileselection = gimp_file_selection_new (gettext(dirs[i].fs_label),
						 *(dirs[i].mdir),
						 TRUE, TRUE);
	gtk_signal_connect (GTK_OBJECT (fileselection), "filename_changed",
			    GTK_SIGNAL_FUNC (file_prefs_filename_callback),
			    dirs[i].mdir);
	gimp_table_attach_aligned (GTK_TABLE (table), 0, i,
				   gettext(dirs[i].label), 1.0, 0.5,
				   fileselection, 1, FALSE);
      }
  }

  /* Directories / <paths> */
  {
    static const struct
    {
      gchar  *tree_label;
      gchar  *label;
      gchar  *help_data;
      gchar  *fs_label;
      gchar **mpath;
    }
    paths[] =
    {
      { N_("Brushes"), N_("Brushes Directories"),
	"dialogs/preferences/directories.html#brushes",
	N_("Select Brushes Dir"),
	&edit_brush_path },
      { N_("Generated Brushes"), N_("Generated Brushes Directories"),
	"dialogs/preferences/directories.html#generated_brushes",
	N_("Select Generated Brushes Dir"),
	&edit_brush_vbr_path },
      { N_("Patterns"), N_("Patterns Directories"),
	"dialogs/preferences/directories.html#patterns",
	N_("Select Patterns Dir"),
	&edit_pattern_path },
      { N_("Palettes"), N_("Palettes Directories"),
	"dialogs/preferences/directories.html#palettes",
	N_("Select Palettes Dir"),
	&edit_palette_path },
      { N_("Gradients"), N_("Gradients Directories"),
	"dialogs/preferences/directories.html#gradients",
	N_("Select Gradients Dir"),
	&edit_gradient_path },
      { N_("Plug-Ins"), N_("Plug-Ins Directories"),
	"dialogs/preferences/directories.html#plug_ins",
	N_("Select Plug-Ins Dir"),
	&edit_plug_in_path },
      { N_("Modules"), N_("Modules Directories"),
	"dialogs/preferences/directories.html#modules",
	N_("Select Modules Dir"),
	&edit_module_path }
    };
    static gint npaths = sizeof (paths) / sizeof (paths[0]);
	
    for (i = 0; i < npaths; i++)
      {
	vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
						gettext (paths[i].label),
						GTK_CTREE (ctree),
						gettext (paths[i].tree_label),
						paths[i].help_data,
						top_insert,
						&child_insert,
						page_index);
	gtk_widget_show (vbox);
	page_index++;

	patheditor = gimp_path_editor_new (gettext(paths[i].fs_label),
					   *(paths[i].mpath));
	gtk_signal_connect (GTK_OBJECT (patheditor), "path_changed",
			    GTK_SIGNAL_FUNC (file_prefs_path_callback),
			    paths[i].mpath);
	gtk_container_add (GTK_CONTAINER (vbox), patheditor);
	gtk_widget_show (patheditor);
      }
  }

  /* Recalculate the width of the Category tree now */
  gtk_clist_set_column_width
    (GTK_CLIST (ctree), 0, 
     gtk_clist_optimal_column_width (GTK_CLIST (ctree), 0));

  gtk_widget_show (ctree);
  gtk_widget_show (notebook);

  gtk_widget_show (prefs_dlg);
}
