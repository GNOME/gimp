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

#include "config.h"
#include "libgimp/gimpchainbutton.h"
#include "libgimp/gimpfileselection.h"
#include "libgimp/gimppatheditor.h"
#include "libgimp/gimpsizeentry.h"
#include "libgimp/gimplimits.h"
#include "libgimp/gimpintl.h"

/*  preferences local functions  */
static void file_prefs_ok_callback     (GtkWidget *, GtkWidget *);
static void file_prefs_save_callback   (GtkWidget *, GtkWidget *);
static void file_prefs_cancel_callback (GtkWidget *, GtkWidget *);

static void file_prefs_toggle_callback             (GtkWidget *, gpointer);
static void file_prefs_preview_size_callback       (GtkWidget *, gpointer);
static void file_prefs_nav_preview_size_callback   (GtkWidget *, gpointer);
static void file_prefs_mem_size_callback           (GtkWidget *, gpointer);
static void file_prefs_mem_size_unit_callback      (GtkWidget *, gpointer);
static void file_prefs_int_adjustment_callback     (GtkAdjustment *, gpointer);
static void file_prefs_string_callback             (GtkWidget *, gpointer);
static void file_prefs_filename_callback           (GtkWidget *, gpointer);
static void file_prefs_path_callback               (GtkWidget *, gpointer);
static void file_prefs_clear_session_info_callback (GtkWidget *, gpointer);
static void file_prefs_default_size_callback       (GtkWidget *, gpointer);
static void file_prefs_default_resolution_callback (GtkWidget *, gpointer);
static void file_prefs_res_source_callback         (GtkWidget *, gpointer);
static void file_prefs_monitor_resolution_callback (GtkWidget *, gpointer);

/*  static variables  */
static int        old_perfectmouse;
static int        old_transparency_type;
static int        old_transparency_size;
static int        old_levels_of_undo;
static int        old_marching_speed;
static int        old_allow_resize_windows;
static int        old_auto_save;
static int        old_preview_size;
static int        old_nav_preview_size;
static int        old_no_cursor_updating;
static int        old_show_tool_tips;
static int        old_show_rulers;
static int        old_show_statusbar;
static InterpolationType old_interpolation_type;
static int        old_confirm_on_close;
static int        old_save_session_info;
static int        old_save_device_status;
static int        old_always_restore_session;
static int        old_default_width;
static int        old_default_height;
static GUnit      old_default_units;
static double     old_default_xresolution;
static double     old_default_yresolution;
static GUnit      old_default_resolution_units;
static int        old_default_type;
static int        old_stingy_memory_use;
static int        old_tile_cache_size;
static int        old_install_cmap;
static int        old_cycled_marching_ants;
static int        old_last_opened_size;
static char *     old_temp_path;
static char *     old_swap_path;
static char *     old_plug_in_path;
static char *     old_module_path;
static char *     old_brush_path;
static char *     old_brush_vbr_path;
static char *     old_pattern_path;
static char *     old_palette_path;
static char *     old_gradient_path;
static double     old_monitor_xres;
static double     old_monitor_yres;
static int        old_using_xserver_resolution;
static int        old_num_processors;
static char *     old_image_title_format;
static int        old_global_paint_options;
static int        old_max_new_image_size;
static int        old_thumbnail_mode;
static int 	  old_show_indicators;
static int	  old_trust_dirty_flag;
static int        old_use_help;
static int        old_nav_window_per_display;
static int        old_info_window_follows_mouse;

/*  variables which can't be changed on the fly  */
static int        edit_stingy_memory_use;
static int        edit_tile_cache_size;
static int        edit_install_cmap;
static int        edit_cycled_marching_ants;
static int        edit_last_opened_size;
static int        edit_num_processors;
static char *     edit_temp_path = NULL;
static char *     edit_swap_path = NULL;
static char *     edit_plug_in_path = NULL;
static char *     edit_module_path = NULL;
static char *     edit_brush_path = NULL;
static char *     edit_brush_vbr_path = NULL;
static char *     edit_pattern_path = NULL;
static char *     edit_palette_path = NULL;
static char *     edit_gradient_path = NULL;
static int        edit_nav_window_per_display;
static int        edit_info_window_follows_mouse;

static GtkWidget *prefs_dlg = NULL;

/* Some information regarding preferences, compiled by Raph Levien 11/3/97.
   updated by Michael Natterer 27/3/99

   The following preference items cannot be set on the fly (at least
   according to the existing pref code - it may be that changing them
   so they're set on the fly is not hard).

   stingy-memory-use
   tile-cache-size
   install-cmap
   cycled-marching-ants
   last-opened-size
   num-processors
   temp-path
   swap-path
   plug-in-path
   module-path
   brush-path
   pattern-path
   palette-path
   gradient-path
   nav-window-per-display

   All of these now have variables of the form edit_temp_path, which
   are copied from the actual variables (e.g. temp_path) the first time
   the dialog box is started.

   Variables of the form old_temp_path represent the values at the
   time the dialog is opened - a cancel copies them back from old to
   the real variables or the edit variables, depending on whether they
   can be set on the fly.

   Here are the remaining issues as I see them:

   Still no settings for default-gradient, default-palette,
   gamma-correction, color-cube.

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

/* Copy the string from source to destination, freeing the string stored
   in the destination if there is one there already. */
static void
file_prefs_strset (gchar **dst,
		   gchar  *src)
{
  if (*dst)
    g_free (*dst);

  *dst = g_strdup (src);
}

/* Duplicate the string, but treat NULL as the empty string. */
static gchar *
file_prefs_strdup (gchar *src)
{
  return g_strdup (src == NULL ? "" : src);
}

/* Compare two strings, but treat NULL as the empty string. */
static int
file_prefs_strcmp (gchar *src1,
		   gchar *src2)
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
  if (default_units < UNIT_INCH ||
      default_units >= gimp_unit_get_number_of_units ())
    {
      g_message (_("Error: Default unit must be within unit range."));
      default_units = old_default_units;
      return;
    }
  if (default_xresolution < GIMP_MIN_RESOLUTION ||
      default_yresolution < GIMP_MIN_RESOLUTION)
    {
      g_message (_("Error: Default resolution must not be zero."));
      default_xresolution = old_default_xresolution;
      default_yresolution = old_default_yresolution;
      return;
    }
  if (default_resolution_units < UNIT_INCH ||
      default_resolution_units >= gimp_unit_get_number_of_units ())
    {
      g_message (_("Error: Default resolution unit must be within unit range."));
      default_resolution_units = old_default_resolution_units;
      return;
    }
  if (monitor_xres < GIMP_MIN_RESOLUTION ||
      monitor_yres < GIMP_MIN_RESOLUTION)
    {
      g_message (_("Error: Monitor resolution must not be zero."));
      monitor_xres = old_monitor_xres;
      monitor_yres = old_monitor_yres;
      return;
    }
  if (image_title_format == NULL)
    {
      g_message (_("Error: Image title format must not be NULL."));
      image_title_format = old_image_title_format;
      return;
    }

  gtk_widget_destroy (dlg);
  prefs_dlg = NULL;

  if (show_tool_tips)
    gimp_help_enable_tooltips ();
  else
    gimp_help_disable_tooltips ();

  /* This needs modification to notify the user of which simply cannot be
   * changed on the fly.  Currently it ignores these options if only OK is
   * pressed.
   */
}

static void
file_prefs_save_callback (GtkWidget *widget,
			  GtkWidget *dlg)
{
  GList *update = NULL; /* options that should be updated in .gimprc */
  GList *remove = NULL; /* options that should be commented out */

  int    save_stingy_memory_use;
  int    save_tile_cache_size;
  int    save_install_cmap;
  int    save_cycled_marching_ants;
  int    save_last_opened_size;
  int    save_num_processors;
  int    save_nav_window_per_display;
  int    save_info_window_follows_mouse;
  gchar *save_temp_path;
  gchar *save_swap_path;
  gchar *save_plug_in_path;
  gchar *save_module_path;
  gchar *save_brush_path;
  gchar *save_brush_vbr_path;
  gchar *save_pattern_path;
  gchar *save_palette_path;
  gchar *save_gradient_path;

  int    restart_notification = FALSE;

  file_prefs_ok_callback (widget, dlg);

  /* Save variables so that we can restore them later */
  save_stingy_memory_use = stingy_memory_use;
  save_tile_cache_size = tile_cache_size;
  save_install_cmap = install_cmap;
  save_cycled_marching_ants = cycled_marching_ants;
  save_last_opened_size = last_opened_size;
  save_num_processors = num_processors;
  save_temp_path = temp_path;
  save_swap_path = swap_path;
  save_plug_in_path = plug_in_path;
  save_module_path = module_path;
  save_brush_path = brush_path;
  save_brush_vbr_path = brush_vbr_path;
  save_pattern_path = pattern_path;
  save_palette_path = palette_path;
  save_gradient_path = gradient_path;
  save_nav_window_per_display = nav_window_per_display;
  save_info_window_follows_mouse = info_window_follows_mouse;

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
  if (interpolation_type != old_interpolation_type)
    update = g_list_append (update, "interpolation-type");
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
  if (show_indicators != old_show_indicators)
    {
      update = g_list_append (update, "show-indicators");
      remove = g_list_append (remove, "dont-show-indicators");
      restart_notification = TRUE;
    }
  if (always_restore_session != old_always_restore_session)
    update = g_list_append (update, "always-restore-session");
  if (default_width != old_default_width ||
      default_height != old_default_height)
    update = g_list_append (update, "default-image-size");
  if (default_units != old_default_units)
    update = g_list_append (update, "default-units");
  if (ABS(default_xresolution - old_default_xresolution) > GIMP_MIN_RESOLUTION)
    update = g_list_append (update, "default-xresolution");
  if (ABS(default_yresolution - old_default_yresolution) > GIMP_MIN_RESOLUTION)
    update = g_list_append (update, "default-yresolution");
  if (default_resolution_units != old_default_resolution_units)
    update = g_list_append (update, "default-resolution-units");
  if (default_type != old_default_type)
    update = g_list_append (update, "default-image-type");
  if (preview_size != old_preview_size)
    update = g_list_append (update, "preview-size");
  if (nav_preview_size != old_nav_preview_size)
    update = g_list_append (update, "nav-preview-size");
  if (perfectmouse != old_perfectmouse)
    update = g_list_append (update, "perfect-mouse");
  if (transparency_type != old_transparency_type)
    update = g_list_append (update, "transparency-type");
  if (transparency_size != old_transparency_size)
    update = g_list_append (update, "transparency-size");
  if (using_xserver_resolution != old_using_xserver_resolution ||
      ABS(monitor_xres - old_monitor_xres) > GIMP_MIN_RESOLUTION)
    update = g_list_append (update, "monitor-xresolution");
  if (using_xserver_resolution != old_using_xserver_resolution ||
      ABS(monitor_yres - old_monitor_yres) > GIMP_MIN_RESOLUTION)
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
  if (file_prefs_strcmp (plug_in_path, edit_plug_in_path))
    {
      update = g_list_append (update, "plug-in-path");
      plug_in_path = edit_plug_in_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (module_path, edit_module_path))
    {
      update = g_list_append (update, "module-path");
      module_path = edit_module_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (brush_path, edit_brush_path))
    {
      update = g_list_append (update, "brush-path");
      brush_path = edit_brush_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (brush_vbr_path, edit_brush_vbr_path))
    {
      update = g_list_append (update, "brush-vbr-path");
      brush_vbr_path = edit_brush_vbr_path;
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
  if (global_paint_options != old_global_paint_options)
    {
      update = g_list_append (update, "global-paint-options");
      remove = g_list_append (remove, "no-global-paint-options");
    }
  if (max_new_image_size != old_max_new_image_size)
    update = g_list_append (update, "max-new-image-size");
  if (thumbnail_mode != old_thumbnail_mode)
    update = g_list_append (update, "thumbnail-mode");
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
  if (edit_nav_window_per_display != old_nav_window_per_display)
    {
      update = g_list_append (update, "nav-window-per-display");
      remove = g_list_append (remove, "nav-window-follows-auto");
      nav_window_per_display = edit_nav_window_per_display;
      restart_notification = TRUE;
    }
  if (edit_info_window_follows_mouse != old_info_window_follows_mouse)
    {
      update = g_list_append (update, "info-window-follows-mouse");
      remove = g_list_append (remove, "info-window-per-display");
      info_window_follows_mouse = edit_info_window_follows_mouse;
      restart_notification = TRUE;
    }

  save_gimprc (&update, &remove);

  if (using_xserver_resolution)
    gdisplay_xserver_resolution (&monitor_xres, &monitor_yres);

  /* Restore variables which must not change */
  stingy_memory_use = save_stingy_memory_use;
  tile_cache_size = save_tile_cache_size;
  install_cmap = save_install_cmap;
  cycled_marching_ants = save_cycled_marching_ants;
  last_opened_size = save_last_opened_size;
  num_processors = save_num_processors;
  temp_path = save_temp_path;
  swap_path = save_swap_path;
  plug_in_path = save_plug_in_path;
  module_path = save_module_path;
  brush_path = save_brush_path;
  brush_vbr_path = save_brush_vbr_path;
  pattern_path = save_pattern_path;
  palette_path = save_palette_path;
  gradient_path = save_gradient_path;
  nav_window_per_display = save_nav_window_per_display;
  info_window_follows_mouse = save_info_window_follows_mouse;

  if (restart_notification)
    g_message (_("You will need to restart GIMP for these changes to take "
		 "effect."));

  g_list_free (update);
  g_list_free (remove);
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
  perfectmouse = old_perfectmouse;
  show_tool_tips = old_show_tool_tips;
  show_rulers = old_show_rulers;
  show_statusbar = old_show_statusbar;
  interpolation_type = old_interpolation_type;
  confirm_on_close = old_confirm_on_close;
  save_session_info = old_save_session_info;
  save_device_status = old_save_device_status;
  default_width = old_default_width;
  default_height = old_default_height;
  default_units = old_default_units;
  default_xresolution = old_default_xresolution;
  default_yresolution = old_default_yresolution;
  default_resolution_units = old_default_resolution_units;
  default_type = old_default_type;
  monitor_xres = old_monitor_xres;
  monitor_yres = old_monitor_yres;
  using_xserver_resolution = old_using_xserver_resolution;
  num_processors = old_num_processors;
  max_new_image_size = old_max_new_image_size;
  thumbnail_mode = old_thumbnail_mode;
  show_indicators = old_show_indicators;
  trust_dirty_flag = old_trust_dirty_flag;
  use_help = old_use_help;
  nav_window_per_display = old_nav_window_per_display;
  info_window_follows_mouse = old_info_window_follows_mouse;

  if (preview_size != old_preview_size)
    {
      lc_dialog_rebuild (old_preview_size);
      layer_select_update_preview_size ();
    }

  if (nav_preview_size != old_nav_preview_size)
    {
      nav_preview_size = old_nav_preview_size;
      gdisplays_nav_preview_resized();
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
  edit_nav_window_per_display = nav_window_per_display;
  edit_info_window_follows_mouse = info_window_follows_mouse;

  file_prefs_strset (&edit_temp_path, old_temp_path);
  file_prefs_strset (&edit_swap_path, old_swap_path);
  file_prefs_strset (&edit_plug_in_path, old_plug_in_path);
  file_prefs_strset (&edit_module_path, old_module_path);
  file_prefs_strset (&edit_brush_path, old_brush_path);
  file_prefs_strset (&edit_brush_vbr_path, old_brush_vbr_path);
  file_prefs_strset (&edit_pattern_path, old_pattern_path);
  file_prefs_strset (&edit_palette_path, old_palette_path);
  file_prefs_strset (&edit_gradient_path, old_gradient_path);

  file_prefs_strset (&image_title_format, old_image_title_format);

  context_manager_set_global_paint_options (old_global_paint_options);
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
  else if ((data == &transparency_type) ||
	   (data == &transparency_size))
    {
      val = data;
      *val = (long) gtk_object_get_user_data (GTK_OBJECT (widget));
      render_setup (transparency_type, transparency_size);
      gimage_foreach ((GFunc)layer_invalidate_previews, NULL);
      gimage_invalidate_previews ();
      gdisplays_expose_full ();
      gdisplays_flush ();
    }
  else if (data == &global_paint_options)
    context_manager_set_global_paint_options (GTK_TOGGLE_BUTTON (widget)->active);
  else if (data == &show_indicators)
    show_indicators = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &thumbnail_mode || data == &interpolation_type ||
           data == &trust_dirty_flag)
    {
      val = data;
      *val = (long) gtk_object_get_user_data (GTK_OBJECT (widget));
    }
  else if (data == &use_help)
    use_help = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &edit_nav_window_per_display)
    edit_nav_window_per_display = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &edit_info_window_follows_mouse)
    edit_info_window_follows_mouse = GTK_TOGGLE_BUTTON (widget)->active;
  else
    {
      /* Are you a gimp-hacker who is getting this message?  You
	 probably want to do the same as the &thumbnail_mode case
	 above is doing. */
      g_warning("Unknown file_prefs_toggle_callback() invoker - ignored.");
    }
}

static void
file_prefs_preview_size_callback (GtkWidget *widget,
                                  gpointer   data)
{
  lc_dialog_rebuild ((long) data);
  layer_select_update_preview_size ();
}

static void
file_prefs_nav_preview_size_callback (GtkWidget *widget,
				      gpointer   data)
{
  nav_preview_size = (gint) data;
  gdisplays_nav_preview_resized ();
}

static void
file_prefs_mem_size_callback (GtkWidget *widget,
			      gpointer   data)
{
  gint *mem_size;
  gint  divided_mem_size;
  gint  mem_size_unit;

  if (! (mem_size = gtk_object_get_data (GTK_OBJECT (widget), "mem_size")))
    return;

  divided_mem_size = (int) gtk_object_get_data (GTK_OBJECT (widget),
						"divided_mem_size");
  mem_size_unit = (int) gtk_object_get_data (GTK_OBJECT (widget),
					     "mem_size_unit");

  divided_mem_size = GTK_ADJUSTMENT (widget)->value;
  *mem_size = divided_mem_size * mem_size_unit;

  gtk_object_set_data (GTK_OBJECT (widget), "divided_mem_size",
		       (gpointer) divided_mem_size);
}

static void
file_prefs_mem_size_unit_callback (GtkWidget *widget,
				   gpointer   data)
{
  GtkObject *adjustment;
  gint  new_unit;
  gint *mem_size;
  gint  divided_mem_size;
  gint  mem_size_unit;

  adjustment = GTK_OBJECT (data);

  new_unit = (int) gtk_object_get_user_data (GTK_OBJECT (widget));

  if (! (mem_size = gtk_object_get_data (GTK_OBJECT (adjustment), "mem_size")))
    return;

  divided_mem_size = (int) gtk_object_get_data (GTK_OBJECT (adjustment),
						"divided_mem_size");
  mem_size_unit = (int) gtk_object_get_data (GTK_OBJECT (adjustment),
					     "mem_size_unit");

  if (new_unit != mem_size_unit)
    {
      divided_mem_size = *mem_size / new_unit;
      mem_size_unit = new_unit;

      gtk_signal_handler_block_by_data (GTK_OBJECT (adjustment), mem_size);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (adjustment), divided_mem_size);
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (adjustment), mem_size);
    }

  gtk_object_set_data (GTK_OBJECT (adjustment), "divided_mem_size",
		       (gpointer) divided_mem_size);
  gtk_object_set_data (GTK_OBJECT (adjustment), "mem_size_unit",
		       (gpointer) mem_size_unit);
}

static void
file_prefs_int_adjustment_callback (GtkAdjustment *adj,
				    gpointer       data)
{
  gint *val;

  val = data;
  *val = (int) adj->value;
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
  gchar *path;

  val = data;
  path =  gimp_path_editor_get_path (GIMP_PATH_EDITOR (widget));
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
    (int) (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0) + 0.5);
  default_height =
    (int) (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1) + 0.5);
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
				 GtkCTreeNode  *parent,
				 GtkCTreeNode **new_node,
				 gint           page_index)
{
  GtkWidget *out_vbox;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *label;
  gchar     *titles[1];

  out_vbox = gtk_vbox_new (FALSE, 0);
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
  gtk_notebook_append_page (notebook, out_vbox, NULL);

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
  GSList    *group;

  gint i;
  gint divided_mem_size;
  gint mem_size_unit;

  if (prefs_dlg)
    {
      gdk_window_raise (GTK_WIDGET (prefs_dlg)->window);
      return;
    }

  if (edit_temp_path == NULL)
    {
      /* first time dialog is opened - copy config vals to edit
	 variables. */
      edit_stingy_memory_use = stingy_memory_use;
      edit_tile_cache_size = tile_cache_size;
      edit_install_cmap = install_cmap;
      edit_cycled_marching_ants = cycled_marching_ants;
      edit_last_opened_size = last_opened_size;
      edit_temp_path = file_prefs_strdup (temp_path);	
      edit_swap_path = file_prefs_strdup (swap_path);
      edit_brush_path = file_prefs_strdup (brush_path);
      edit_brush_vbr_path = file_prefs_strdup (brush_vbr_path);
      edit_pattern_path = file_prefs_strdup (pattern_path);
      edit_palette_path = file_prefs_strdup (palette_path);
      edit_plug_in_path = file_prefs_strdup (plug_in_path);
      edit_module_path = file_prefs_strdup (module_path);
      edit_gradient_path = file_prefs_strdup (gradient_path);
      edit_nav_window_per_display = nav_window_per_display;
      edit_info_window_follows_mouse = info_window_follows_mouse;
    }
  old_perfectmouse = perfectmouse;
  old_transparency_type = transparency_type;
  old_transparency_size = transparency_size;
  old_levels_of_undo = levels_of_undo;
  old_marching_speed = marching_speed;
  old_allow_resize_windows = allow_resize_windows;
  old_auto_save = auto_save;
  old_preview_size = preview_size;
  old_nav_preview_size = nav_preview_size;
  old_no_cursor_updating = no_cursor_updating;
  old_show_tool_tips = show_tool_tips;
  old_show_rulers = show_rulers;
  old_show_statusbar = show_statusbar;
  old_interpolation_type = interpolation_type;
  old_confirm_on_close = confirm_on_close;
  old_save_session_info = save_session_info;
  old_save_device_status = save_device_status;
  old_always_restore_session = always_restore_session;
  old_default_width = default_width;
  old_default_height = default_height;
  old_default_units = default_units;
  old_default_xresolution = default_xresolution;
  old_default_yresolution = default_yresolution;
  old_default_resolution_units = default_resolution_units;
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
  old_global_paint_options = global_paint_options;
  old_max_new_image_size = max_new_image_size;
  old_thumbnail_mode = thumbnail_mode;
  old_show_indicators = show_indicators;
  old_trust_dirty_flag = trust_dirty_flag;
  old_use_help = use_help;
  old_nav_window_per_display = nav_window_per_display;
  old_info_window_follows_mouse = info_window_follows_mouse;

  file_prefs_strset (&old_image_title_format, image_title_format);	
  file_prefs_strset (&old_temp_path, edit_temp_path);
  file_prefs_strset (&old_swap_path, edit_swap_path);
  file_prefs_strset (&old_plug_in_path, edit_plug_in_path);
  file_prefs_strset (&old_module_path, edit_module_path);
  file_prefs_strset (&old_brush_path, edit_brush_path);
  file_prefs_strset (&old_brush_vbr_path, edit_brush_vbr_path);
  file_prefs_strset (&old_pattern_path, edit_pattern_path);
  file_prefs_strset (&old_palette_path, edit_palette_path);
  file_prefs_strset (&old_gradient_path, edit_gradient_path);

  /* Create the dialog */
  prefs_dlg =
    gimp_dialog_new (_("Preferences"), "gimp_preferences",
		     gimp_standard_help_func,
		     "dialogs/preferences/preferences.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     _("OK"), file_prefs_ok_callback,
		     NULL, NULL, FALSE, FALSE,
		     _("Save"), file_prefs_save_callback,
		     NULL, NULL, FALSE, FALSE,
		     _("Cancel"), file_prefs_cancel_callback,
		     NULL, NULL, TRUE, TRUE,

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

  gtk_object_set_user_data (GTK_OBJECT (ctree), notebook);
  gtk_signal_connect (GTK_OBJECT (ctree), "tree_select_row",
		      GTK_SIGNAL_FUNC (file_pref_tree_select_callback),
		      NULL);

  page_index = 0;

  /* New File page */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("New File Settings"),
					  GTK_CTREE (ctree),
					  _("New File"),
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

  sizeentry2 =
    gimp_size_entry_new (2, default_resolution_units, "Pixels/%s",
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
      
  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  optionmenu =
    gimp_option_menu_new (file_prefs_toggle_callback,
			  (gpointer) default_type,
			  _("RGB"),       &default_type, (gpointer) RGB,
			  _("Grayscale"), &default_type, (gpointer) GRAY,
			  NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0,
			     _("Default Image Type:"), 1.0, 0.5,
			     optionmenu, TRUE);

  /* Display page */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Display Settings"),
					  GTK_CTREE (ctree),
					  _("Display"),
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
    gimp_option_menu_new (file_prefs_toggle_callback,
			  (gpointer) transparency_type,
			  _("Light Checks"),
			  &transparency_type, (gpointer) LIGHT_CHECKS,
			  _("Mid-Tone Checks"),
			  &transparency_type, (gpointer) GRAY_CHECKS,
			  _("Dark Checks"),
			  &transparency_type, (gpointer) DARK_CHECKS,
			  _("White Only"),
			  &transparency_type, (gpointer) WHITE_ONLY,
			  _("Gray Only"),
			  &transparency_type, (gpointer) GRAY_ONLY,
			  _("Black Only"),
			  &transparency_type, (gpointer) BLACK_ONLY,
			  NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0,
			     _("Transparency Type:"), 1.0, 0.5,
			     optionmenu, TRUE);

  optionmenu =
    gimp_option_menu_new (file_prefs_toggle_callback,
			  (gpointer) transparency_size,
			  _("Small"),
			  &transparency_size, (gpointer) SMALL_CHECKS,
			  _("Medium"),
			  &transparency_size, (gpointer) MEDIUM_CHECKS,
			  _("Large"), 
			  &transparency_size, (gpointer) LARGE_CHECKS,
			  NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 1,
			     _("Check Size:"), 1.0, 0.5, optionmenu, TRUE);

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
    gimp_option_menu_new (file_prefs_toggle_callback,
			  (gpointer) interpolation_type,
			  _("Nearest Neighbor (Fast)"), &interpolation_type,
			  (gpointer) NEAREST_NEIGHBOR_INTERPOLATION,
			  _("Linear"), &interpolation_type,
			  (gpointer) LINEAR_INTERPOLATION,
			  _("Cubic (Slow)"), &interpolation_type,
			  (gpointer) CUBIC_INTERPOLATION,
			  NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0,
			     _("Interpolation Type:"), 1.0, 0.5,
			     optionmenu, TRUE);

  /* Interface */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Interface Settings"),
					  GTK_CTREE (ctree),
					  _("Interface"),
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
    gimp_option_menu_new (file_prefs_preview_size_callback,
			  (gpointer) preview_size,
			  _("None"),   (gpointer)   0, (gpointer)   0,
			  _("Tiny"),   (gpointer)  24, (gpointer)  24,
			  _("Small"),  (gpointer)  32, (gpointer)  32,
			  _("Medium"), (gpointer)  48, (gpointer)  48,
			  _("Large"),  (gpointer)  64, (gpointer)  64,
			  _("Huge"),   (gpointer) 128, (gpointer) 128,
			  NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0,
			     _("Preview Size:"), 1.0, 0.5, optionmenu, TRUE);

  optionmenu =
    gimp_option_menu_new (file_prefs_nav_preview_size_callback,
			  (gpointer) nav_preview_size,
			  _("Small"),  (gpointer)  48, (gpointer)  48,
			  _("Medium"), (gpointer)  80, (gpointer)  80,
			  _("Large"),  (gpointer) 112, (gpointer) 112,
			  NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 1,
			     _("Nav Preview Size:"), 1.0, 0.5, optionmenu, TRUE);

  spinbutton =
    gimp_spin_button_new (&adjustment,
			  levels_of_undo, 0.0, 255.0, 1.0, 5.0, 0.0, 1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) file_prefs_int_adjustment_callback,
		      &levels_of_undo);
  gimp_table_attach_aligned (GTK_TABLE (table), 2,
			     _("Levels of Undo:"), 1.0, 0.5, spinbutton, TRUE);

  spinbutton =
    gimp_spin_button_new (&adjustment, edit_last_opened_size,
			  0.0, 16.0, 1.0, 5.0, 0.0, 1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (file_prefs_int_adjustment_callback),
		      &edit_last_opened_size);
  gimp_table_attach_aligned (GTK_TABLE (table), 3,
			     _("Recent Documents List Size:"), 1.0, 0.5,
			     spinbutton, TRUE);

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
					  _("Help System Settings"),
					  GTK_CTREE (ctree),
					  _("Help System"),
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

  /* Interface / Image Windows */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Image Windows Settings"),
					  GTK_CTREE (ctree),
					  _("Image Windows"),
					  top_insert,
					  &child_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = file_prefs_frame_new (_("Appearance"), GTK_BOX (vbox));

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
		      GTK_SIGNAL_FUNC (file_prefs_int_adjustment_callback),
		      &marching_speed);
  gimp_table_attach_aligned (GTK_TABLE (table), 0,
			     _("Marching Ants Speed:"), 1.0, 0.5,
			     spinbutton, TRUE);

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

  gimp_table_attach_aligned (GTK_TABLE (table), 1,
			     _("Image Title Format:"), 1.0, 0.5, combo, FALSE);
  /* End of the title format string */

  vbox2 = file_prefs_frame_new (_("Pointer Movement Feedback"), GTK_BOX (vbox));

  button =
    gtk_check_button_new_with_label(_("Perfect-but-slow Pointer Tracking"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				perfectmouse);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &perfectmouse);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_label(_("Disable Cursor Updating"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				no_cursor_updating);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &no_cursor_updating);
  gtk_widget_show (button);

  /* Interface / Tool Options */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Tool Options Settings"),
					  GTK_CTREE (ctree),
					  _("Tool Options"),
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

  /* Indicators */
  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
  gtk_container_add (GTK_CONTAINER (vbox), vbox2);
  gtk_widget_show (vbox2);

  button = gtk_check_button_new_with_label
    (_("Display brush and pattern indicators on Toolbar"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				show_indicators);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &show_indicators);
  gtk_widget_show (button);

  /* Expand the "Interface" branch */
  gtk_ctree_expand (GTK_CTREE (ctree), top_insert);

  /* Environment */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Environment Settings"),
					  GTK_CTREE (ctree),
					  _("Environment"),
					  NULL,
					  &top_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = file_prefs_frame_new (_("Resource Consumption"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label(_("Conservative Memory Usage"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), stingy_memory_use);
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

  /*  The tile cache size  */
  mem_size_unit = 1;
  for (i = 0; i < 3; i++)
    {
      if (edit_tile_cache_size % (mem_size_unit * 1024) != 0)
	break;
      mem_size_unit *= 1024;
    }
  divided_mem_size = edit_tile_cache_size / mem_size_unit;

  hbox = gtk_hbox_new (FALSE, 2);
  spinbutton =
    gimp_spin_button_new (&adjustment, divided_mem_size,
			  0.0, (4069.0 * 1024 * 1024), 1.0, 16.0, 0.0,
			  1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (file_prefs_mem_size_callback),
		      &edit_tile_cache_size);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  /* for the mem_size_unit callback  */
  gtk_object_set_data (GTK_OBJECT (adjustment), "mem_size",
		       &edit_tile_cache_size);
  gtk_object_set_data (GTK_OBJECT (adjustment), "divided_mem_size",
		       (gpointer) divided_mem_size);
  gtk_object_set_data (GTK_OBJECT (adjustment), "mem_size_unit",
		       (gpointer) mem_size_unit);

  optionmenu =
    gimp_option_menu_new (file_prefs_mem_size_unit_callback,
			  (gpointer) mem_size_unit,
			  _("Bytes"),     adjustment, (gpointer) 1,
			  _("KiloBytes"), adjustment, (gpointer) 1024,
			  _("MegaBytes"), adjustment, (gpointer) (1024*1024),
			  NULL);
  gtk_box_pack_start (GTK_BOX (hbox), optionmenu, FALSE, FALSE, 0);
  gtk_widget_show (optionmenu);
  gimp_table_attach_aligned (GTK_TABLE (table), 0,
			     _("Tile Cache Size:"), 1.0, 0.5, hbox, TRUE);

  /*  The maximum size of a new image  */
  mem_size_unit = 1;
  for (i = 0; i < 3; i++)
    {
      if (max_new_image_size % (mem_size_unit * 1024) != 0)
	break;
      mem_size_unit *= 1024;
    }
  divided_mem_size = max_new_image_size / mem_size_unit;

  hbox = gtk_hbox_new (FALSE, 2);
  spinbutton =
    gimp_spin_button_new (&adjustment, divided_mem_size,
			  0.0, (4069.0 * 1024 * 1024), 1.0, 16.0, 0.0,
			  1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (file_prefs_mem_size_callback),
		      &max_new_image_size);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  /* for the mem_size_unit callback  */
  gtk_object_set_data (GTK_OBJECT (adjustment), "mem_size",
		       &max_new_image_size);
  gtk_object_set_data (GTK_OBJECT (adjustment), "divided_mem_size",
		       (gpointer) divided_mem_size);
  gtk_object_set_data (GTK_OBJECT (adjustment), "mem_size_unit",
		       (gpointer) mem_size_unit);

  optionmenu =
    gimp_option_menu_new (file_prefs_mem_size_unit_callback,
			  (gpointer) mem_size_unit,
			  _("Bytes"),     adjustment, (gpointer) 1,
			  _("KiloBytes"), adjustment, (gpointer) 1024,
			  _("MegaBytes"), adjustment, (gpointer) (1024*1024),
			  NULL);
  gtk_box_pack_start (GTK_BOX (hbox), optionmenu, FALSE, FALSE, 0);
  gtk_widget_show (optionmenu);
  gimp_table_attach_aligned (GTK_TABLE (table), 1,
			     _("Maximum Image Size:"), 1.0, 0.5, hbox, TRUE);

#ifdef ENABLE_MP
  spinbutton =
    gimp_spin_button_new (&adjustment,
			  num_processors, 1, 30, 1.0, 2.0, 0.0, 1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (file_prefs_int_adjustment_callback),
		      &num_processors);
  gimp_table_attach_aligned (GTK_TABLE (table), 2,
			     _("Number of Processors to Use:"), 1.0, 0.5,
			     spinbutton, TRUE);
#endif /* ENABLE_MP */

  vbox2 = file_prefs_frame_new (_("8-Bit Displays"), GTK_BOX (vbox));

  if (g_visual->depth != 8)
    gtk_widget_set_sensitive (GTK_WIDGET (vbox2->parent), FALSE);

  button = gtk_check_button_new_with_label(_("Install Colormap"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				install_cmap);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &edit_install_cmap);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_label(_("Colormap Cycling"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				cycled_marching_ants);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (file_prefs_toggle_callback),
		      &edit_cycled_marching_ants);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

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
    gimp_option_menu_new (file_prefs_toggle_callback,
			  (gpointer) thumbnail_mode,
			  _("Always"), &thumbnail_mode, (gpointer) 1,
			  _("Never"),  &thumbnail_mode, (gpointer) 0,
			  NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0,
			     _("Try to Write a Thumbnail File:"), 1.0, 0.5,
			     optionmenu, TRUE);

  optionmenu = gimp_option_menu_new (file_prefs_toggle_callback,
                                     (gpointer) trust_dirty_flag,
                                     _("Only when modified"), &trust_dirty_flag, (gpointer) 1,
                                     _("Always"), &trust_dirty_flag, (gpointer) 0,
                                     NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 1,
                             _("'File > Save' saves the image:"), 1.0, 0.5,
                               optionmenu, TRUE);
                                     

  /* Session Management */
  vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					  _("Session Management"),
					  GTK_CTREE (ctree),
					  _("Session"),
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
      
  button = gtk_button_new_with_label (_("Clear Saved Window Positions"));
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
					  _("Monitor Information"),
					  GTK_CTREE (ctree),
					  _("Monitor"),
					  NULL,
					  &top_insert,
					  page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = file_prefs_frame_new (_("Get Monitor Resolution"), GTK_BOX (vbox));

  {
    gdouble xres, yres;
    gchar   buf[80];

    gdisplay_xserver_resolution (&xres, &yres);

    g_snprintf (buf, sizeof (buf), _("(Currently %d x %d dpi)"),
		(int) (xres + 0.5), (int) (yres + 0.5));
    label = gtk_label_new (buf);
  }

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

  sizeentry =
    gimp_size_entry_new (2, UNIT_INCH, "Pixels/%s", FALSE, FALSE, TRUE, 75,
			 GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);

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
					  _("Directories Settings"),
					  GTK_CTREE (ctree),
					  _("Directories"),
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
    static int ndirs = sizeof (dirs) / sizeof (dirs[0]);

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
	gimp_table_attach_aligned (GTK_TABLE (table), i,
				   gettext(dirs[i].label), 1.0, 0.5,
				   fileselection, FALSE);
      }
  }

  /* Directories / <paths> */
  {
    static const struct
    {
      gchar  *tree_label;
      gchar  *label;
      gchar  *fs_label;
      gchar **mpath;
    }
    paths[] =
    {
      { N_("Brushes"),   N_("Brushes Directories"),   N_("Select Brushes Dir"),
	&edit_brush_path },
      { N_("Generated Brushes"),   N_("Generated Brushes Directories"),   N_("Select Generated Brushes Dir"),
	&edit_brush_vbr_path },
      { N_("Patterns"),  N_("Patterns Directories"),  N_("Select Patterns Dir"),
	&edit_pattern_path },
      { N_("Palettes"),  N_("Palettes Directories"),  N_("Select Palettes Dir"),
	&edit_palette_path },
      { N_("Gradients"), N_("Gradients Directories"), N_("Select Gradients Dir"),
	&edit_gradient_path },
      { N_("Plug-Ins"),  N_("Plug-Ins Directories"),  N_("Select Plug-Ins Dir"),
	&edit_plug_in_path },
      { N_("Modules"),   N_("Modules Directories"),   N_("Select Modules Dir"),
	&edit_module_path }
    };
    static int npaths = sizeof (paths) / sizeof (paths[0]);
	
    for (i = 0; i < npaths; i++)
      {
	vbox = file_prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
						gettext (paths[i].label),
						GTK_CTREE (ctree),
						gettext (paths[i].tree_label),
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
