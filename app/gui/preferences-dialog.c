/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtklistitem.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "base/base-config.h"
#include "base/tile-cache.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcoreconfig.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell-render.h"

#include "tools/tool_manager.h"

#include "gui.h"
#include "resolution-calibrate-dialog.h"
#include "session.h"

#include "gimphelp.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


/*  gimprc will be parsed with a buffer size of 1024, 
 *  so don't set this too large
 */
#define MAX_COMMENT_LENGTH 512

typedef enum
{
  PREFS_OK,
  PREFS_CORRUPT,
  PREFS_RESTART
} PrefsState;


/*  preferences local functions  */
static PrefsState  prefs_check_settings          (Gimp          *gimp);
static void        prefs_ok_callback             (GtkWidget     *widget,
						  GtkWidget     *dlg);
static void        prefs_save_callback           (GtkWidget     *widget,
						  GtkWidget     *dlg);
static void        prefs_cancel_callback         (GtkWidget     *widget,
						  GtkWidget     *dlg);

static void  prefs_toggle_callback               (GtkWidget     *widget,
						  gpointer       data);
static void  prefs_preview_size_callback         (GtkWidget     *widget,
						  gpointer       data);
static void  prefs_nav_preview_size_callback     (GtkWidget     *widget,
						  gpointer       data);
static void  prefs_string_callback               (GtkWidget     *widget,
						  gpointer       data);
static void  prefs_text_callback                 (GtkTextBuffer *widget,
						  gpointer       data);
static void  prefs_filename_callback             (GtkWidget     *widget,
						  gpointer       data);
static void  prefs_path_callback                 (GtkWidget     *widget,
						  gpointer       data);
static void  prefs_clear_session_info_callback   (GtkWidget     *widget,
						  gpointer       data);
static void  prefs_default_size_callback         (GtkWidget     *widget,
						  gpointer       data);
static void  prefs_default_resolution_callback   (GtkWidget     *widget,
						  gpointer       data);
static void  prefs_res_source_callback           (GtkWidget     *widget,
						  gpointer       data);
static void  prefs_monitor_resolution_callback   (GtkWidget     *widget,
						  gpointer       data);
static void  prefs_resolution_calibrate_callback (GtkWidget     *widget,
						  gpointer       data);
static void  prefs_restart_notification          (void);


/*  static variables  */
static gboolean           old_perfectmouse;
static gint               old_transparency_type;
static gint               old_transparency_size;
static gint               old_levels_of_undo;
static gint               old_marching_speed;
static gboolean           old_resize_windows_on_zoom;
static gboolean           old_resize_windows_on_resize;
static gboolean           old_auto_save;
static gint               old_preview_size;
static gint               old_nav_preview_size;
static gboolean           old_no_cursor_updating;
static gboolean           old_show_tool_tips;
static gboolean           old_show_rulers;
static gboolean           old_show_statusbar;
static GimpInterpolationType  old_interpolation_type;
static gboolean           old_confirm_on_close;
static gboolean           old_save_session_info;
static gboolean           old_save_device_status;
static gboolean           old_always_restore_session;
static gint               old_default_width;
static gint               old_default_height;
static GimpUnit           old_default_units;
static gdouble            old_default_xresolution;
static gdouble            old_default_yresolution;
static GimpUnit           old_default_resolution_units;
static gint               old_default_type;
static gchar            * old_default_comment;
static gboolean           old_default_dot_for_dot;
static gboolean           old_stingy_memory_use;
static guint              old_tile_cache_size;
static gint               old_min_colors;
static gboolean           old_install_cmap;
static gboolean           old_cycled_marching_ants;
static gint               old_last_opened_size;
static gchar            * old_temp_path;
static gchar            * old_swap_path;
static gchar            * old_plug_in_path;
static gchar            * old_tool_plug_in_path;
static gchar            * old_module_path;
static gchar            * old_brush_path;
static gchar            * old_pattern_path;
static gchar            * old_palette_path;
static gchar            * old_gradient_path;
static gchar            * old_theme_path;
static gdouble            old_monitor_xres;
static gdouble            old_monitor_yres;
static gboolean           old_using_xserver_resolution;
static gint               old_num_processors;
static gchar            * old_image_title_format;
static gboolean           old_global_paint_options;
static guint              old_max_new_image_size;
static gboolean           old_write_thumbnails;
static gboolean           old_show_indicators;
static gboolean	          old_trust_dirty_flag;
static gboolean           old_use_help;
static gboolean           old_nav_window_per_display;
static gboolean           old_info_window_follows_mouse;
static gint               old_help_browser;
static gint               old_cursor_mode;
static gint               old_default_threshold;
static gboolean           old_disable_tearoff_menus;

/*  variables which can't be changed on the fly  */
static gboolean           edit_stingy_memory_use;
static gint               edit_min_colors;
static gboolean           edit_install_cmap;
static gboolean           edit_cycled_marching_ants;
static gint               edit_last_opened_size;
static gboolean           edit_show_indicators;
static gboolean           edit_nav_window_per_display;
static gboolean           edit_info_window_follows_mouse;
static gboolean           edit_disable_tearoff_menus;
static gchar            * edit_temp_path          = NULL;
static gchar            * edit_swap_path          = NULL;
static gchar            * edit_plug_in_path       = NULL;
static gchar            * edit_tool_plug_in_path  = NULL;
static gchar            * edit_module_path        = NULL;
static gchar            * edit_brush_path         = NULL;
static gchar            * edit_pattern_path       = NULL;
static gchar            * edit_palette_path       = NULL;
static gchar            * edit_gradient_path      = NULL;
static gchar            * edit_theme_path         = NULL;

/*  variables which will be changed _after_ closing the dialog  */
static guint              edit_tile_cache_size;

static GtkWidget        * prefs_dialog = NULL;


/* Some information regarding preferences, compiled by Raph Levien 11/3/97.
   updated by Michael Natterer 27/3/99

   The following preference items cannot be set on the fly (at least
   according to the existing pref code - it may be that changing them
   so they're set on the fly is not hard).

   stingy-memory-use
   min-colors
   install-cmap
   cycled-marching-ants
   last-opened-size
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
prefs_strset (gchar       **dst,
	      const gchar  *src)
{
  if (*dst)
    g_free (*dst);

  *dst = g_strdup (src);
}

/*  Duplicate the string, but treat NULL as the empty string.  */
static gchar *
prefs_strdup (gchar *src)
{
  return g_strdup (src == NULL ? "" : src);
}

/*  Compare two strings, but treat NULL as the empty string.  */
static int
prefs_strcmp (gchar *src1,
	      gchar *src2)
{
  return strcmp (src1 == NULL ? "" : src1,
		 src2 == NULL ? "" : src2);
}

static PrefsState
prefs_check_settings (Gimp *gimp)
{
  /*  First, check for invalid values...  */
  if (gimp->config->levels_of_undo < 0) 
    {
      g_message (_("Error: Levels of undo must be zero or greater."));
      gimp->config->levels_of_undo = old_levels_of_undo;
      return PREFS_CORRUPT;
    }
  if (gimprc.marching_speed < 50)
    {
      g_message (_("Error: Marching speed must be 50 or greater."));
      gimprc.marching_speed = old_marching_speed;
      return PREFS_CORRUPT;
    }
  if (gimp->config->default_width < 1)
    {
      g_message (_("Error: Default width must be one or greater."));
      gimp->config->default_width = old_default_width;
      return PREFS_CORRUPT;
    }
  if (gimp->config->default_height < 1)
    {
      g_message (_("Error: Default height must be one or greater."));
      gimp->config->default_height = old_default_height;
      return PREFS_CORRUPT;
    }
  if (gimp->config->default_units < GIMP_UNIT_INCH ||
      gimp->config->default_units >= gimp_unit_get_number_of_units ())
    {
      g_message (_("Error: Default unit must be within unit range."));
      gimp->config->default_units = old_default_units;
      return PREFS_CORRUPT;
    }
  if (gimp->config->default_xresolution < GIMP_MIN_RESOLUTION ||
      gimp->config->default_yresolution < GIMP_MIN_RESOLUTION)
    {
      g_message (_("Error: Default resolution must not be zero."));
      gimp->config->default_xresolution = old_default_xresolution;
      gimp->config->default_yresolution = old_default_yresolution;
      return PREFS_CORRUPT;
    }
  if (gimp->config->default_resolution_units < GIMP_UNIT_INCH ||
      gimp->config->default_resolution_units >= gimp_unit_get_number_of_units ())
    {
      g_message (_("Error: Default resolution unit must be within unit range."));
      gimp->config->default_resolution_units = old_default_resolution_units;
      return PREFS_CORRUPT;
    }
  if (gimprc.monitor_xres < GIMP_MIN_RESOLUTION ||
      gimprc.monitor_yres < GIMP_MIN_RESOLUTION)
    {
      g_message (_("Error: Monitor resolution must not be zero."));
      gimprc.monitor_xres = old_monitor_xres;
      gimprc.monitor_yres = old_monitor_yres;
      return PREFS_CORRUPT;
    }
  if (gimprc.image_title_format == NULL)
    {
      g_message (_("Error: Image title format must not be NULL."));
      gimprc.image_title_format = old_image_title_format;
      return PREFS_CORRUPT;
    }

  if (base_config->num_processors < 1 || base_config->num_processors > 30) 
    {
      g_message (_("Error: Number of processors must be between 1 and 30."));
      base_config->num_processors = old_num_processors;
      return PREFS_CORRUPT;
    }

  /*  ...then check if we need a restart notification  */
  if (edit_stingy_memory_use         != old_stingy_memory_use         ||
      edit_min_colors                != old_min_colors                ||
      edit_install_cmap              != old_install_cmap              ||
      edit_cycled_marching_ants      != old_cycled_marching_ants      ||
      edit_last_opened_size          != old_last_opened_size          ||
      edit_show_indicators           != old_show_indicators           ||
      edit_nav_window_per_display    != old_nav_window_per_display    ||
      edit_info_window_follows_mouse != old_info_window_follows_mouse ||
      edit_disable_tearoff_menus     != old_disable_tearoff_menus     ||

      prefs_strcmp (old_temp_path,         edit_temp_path)         ||
      prefs_strcmp (old_swap_path,         edit_swap_path)         ||
      prefs_strcmp (old_plug_in_path,      edit_plug_in_path)      ||
      prefs_strcmp (old_tool_plug_in_path, edit_tool_plug_in_path) ||
      prefs_strcmp (old_module_path,       edit_module_path)       ||
      prefs_strcmp (old_brush_path,        edit_brush_path)        ||
      prefs_strcmp (old_pattern_path,      edit_pattern_path)      ||
      prefs_strcmp (old_palette_path,      edit_palette_path)      ||
      prefs_strcmp (old_gradient_path,     edit_gradient_path)     ||
      prefs_strcmp (old_theme_path,        edit_theme_path))
    {
      return PREFS_RESTART;
    }

  return PREFS_OK;
}

static void
prefs_restart_notification_save_callback (GtkWidget *widget,
                                          gpointer   data)
{
  prefs_save_callback (widget, prefs_dialog);
  gtk_widget_destroy (GTK_WIDGET (data));
}

/*  The user pressed OK and not Save, but has changed some settings that
 *  only take effect after he restarts the GIMP. Allow him to save the
 *  settings.
 */
static void
prefs_restart_notification (void)
{
  GtkWidget *dlg;
  GtkWidget *hbox;
  GtkWidget *label;

  dlg = gimp_dialog_new (_("Save Preferences ?"), "gimp_message",
			 gimp_standard_help_func,
			 "dialogs/preferences/preferences.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, FALSE, FALSE,

			 GTK_STOCK_CLOSE, gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 GTK_STOCK_SAVE,
			 prefs_restart_notification_save_callback,
			 NULL, NULL, NULL, TRUE, FALSE,

			 NULL);

  g_signal_connect (G_OBJECT (dlg), "destroy",
		    G_CALLBACK (gtk_main_quit),
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
prefs_ok_callback (GtkWidget *widget,
		   GtkWidget *dlg)
{
  Gimp       *gimp;
  PrefsState  state;

  gimp = GIMP (g_object_get_data (G_OBJECT (dlg), "gimp"));

  state = prefs_check_settings (gimp);

  switch (state)
    {
    case PREFS_CORRUPT:
      return;
      break;

    case PREFS_RESTART:
      gtk_widget_set_sensitive (prefs_dialog, FALSE);
      prefs_restart_notification ();
      break;

    case PREFS_OK:
      if (gimprc.show_tool_tips)
	gimp_help_enable_tooltips ();
      else
	gimp_help_disable_tooltips ();

      if (edit_tile_cache_size != old_tile_cache_size)
	{
	  base_config->tile_cache_size = edit_tile_cache_size;
	  tile_cache_set_size (edit_tile_cache_size);
	}
      break;

    default:
      break;
    }

  if (prefs_dialog)
    {
      gtk_widget_destroy (prefs_dialog);
    }
}

static void
prefs_save_callback (GtkWidget *widget,
		     GtkWidget *dlg)
{
  Gimp       *gimp;
  PrefsState  state;

  GList      *update = NULL; /*  options that should be updated in .gimprc  */
  GList      *remove = NULL; /*  options that should be commented out       */

  gboolean    save_stingy_memory_use;
  gint        save_min_colors;
  gboolean    save_install_cmap;
  gboolean    save_cycled_marching_ants;
  gint        save_last_opened_size;
  gboolean    save_show_indicators;
  gboolean    save_nav_window_per_display;
  gboolean    save_info_window_follows_mouse;
  gchar      *save_temp_path;
  gchar      *save_swap_path;
  gchar      *save_plug_in_path;
  gchar      *save_tool_plug_in_path;
  gchar      *save_module_path;
  gchar      *save_brush_path;
  gchar      *save_pattern_path;
  gchar      *save_palette_path;
  gchar      *save_gradient_path;
  gchar      *save_theme_path;

  gimp = GIMP (g_object_get_data (G_OBJECT (dlg), "gimp"));

  state = prefs_check_settings (gimp);

  switch (state)
    {
    case PREFS_CORRUPT:
      return;
      break;

    case PREFS_RESTART:
      gtk_widget_set_sensitive (prefs_dialog, FALSE);
      g_message (_("You will need to restart GIMP for these "
		   "changes to take effect."));
      /* don't break */

    case PREFS_OK:
      if (gimprc.show_tool_tips)
	gimp_help_enable_tooltips ();
      else
	gimp_help_disable_tooltips ();

      if (edit_tile_cache_size != old_tile_cache_size)
	tile_cache_set_size (edit_tile_cache_size);
      break;

    default:
      break;
    }

  gtk_widget_destroy (prefs_dialog);

  /*  Save variables so that we can restore them later  */
  save_stingy_memory_use         = base_config->stingy_memory_use;
  save_min_colors                = gimprc.min_colors;
  save_install_cmap              = gimprc.install_cmap;
  save_cycled_marching_ants      = gimprc.cycled_marching_ants;
  save_last_opened_size          = gimprc.last_opened_size;
  save_show_indicators           = gimprc.show_indicators;
  save_nav_window_per_display    = gimprc.nav_window_per_display;
  save_info_window_follows_mouse = gimprc.info_window_follows_mouse;

  save_temp_path          = base_config->temp_path;
  save_swap_path          = base_config->swap_path;

  save_plug_in_path       = gimp->config->plug_in_path;
  save_tool_plug_in_path  = gimp->config->tool_plug_in_path;
  save_module_path        = gimp->config->module_path;
  save_brush_path         = gimp->config->brush_path;
  save_pattern_path       = gimp->config->pattern_path;
  save_palette_path       = gimp->config->palette_path;
  save_gradient_path      = gimp->config->gradient_path;

  save_theme_path     = gimprc.theme_path;

  if (gimp->config->levels_of_undo != old_levels_of_undo)
    {
      update = g_list_append (update, "undo-levels");
    }
  if (gimprc.marching_speed != old_marching_speed)
    {
      update = g_list_append (update, "marching-ants-speed");
    }
  if (gimprc.resize_windows_on_zoom != old_resize_windows_on_zoom)
    {
      update = g_list_append (update, "resize-windows-on-zoom");
      remove = g_list_append (remove, "dont-resize-windows-on-zoom");
    }
  if (gimprc.resize_windows_on_resize != old_resize_windows_on_resize)
    {
      update = g_list_append (update, "resize-windows-on-resize");
      remove = g_list_append (remove, "dont-resize-windows-on-resize");
    }
  if (gimprc.auto_save != old_auto_save)
    {
      update = g_list_append (update, "auto-save");
      remove = g_list_append (remove, "dont-auto-save");
    }
  if (gimprc.no_cursor_updating != old_no_cursor_updating)
    {
      update = g_list_append (update, "cursor-updating");
      remove = g_list_append (remove, "no-cursor-updating");
    }
  if (gimprc.show_tool_tips != old_show_tool_tips)
    {
      update = g_list_append (update, "show-tool-tips");
      remove = g_list_append (remove, "dont-show-tool-tips");
    }
  if (gimprc.show_rulers != old_show_rulers)
    {
      update = g_list_append (update, "show-rulers");
      remove = g_list_append (remove, "dont-show-rulers");
    }
  if (gimprc.show_statusbar != old_show_statusbar)
    {
      update = g_list_append (update, "show-statusbar");
      remove = g_list_append (remove, "dont-show-statusbar");
    }
  if (base_config->interpolation_type != old_interpolation_type)
    {
      update = g_list_append (update, "interpolation-type");
    }
  if (gimprc.confirm_on_close != old_confirm_on_close)
    {
      update = g_list_append (update, "confirm-on-close");
      remove = g_list_append (remove, "dont-confirm-on-close");
    }
  if (gimprc.save_session_info != old_save_session_info)
    {
      update = g_list_append (update, "save-session-info");
      remove = g_list_append (remove, "dont-save-session-info");
    }
  if (gimprc.save_device_status!= old_save_device_status)
    {
      update = g_list_append (update, "save-device-status");
      remove = g_list_append (remove, "dont-save-device-status");
    }
  if (gimprc.always_restore_session != old_always_restore_session)
    {
      update = g_list_append (update, "always-restore-session");
    }
  if (gimp->config->default_width != old_default_width ||
      gimp->config->default_height != old_default_height)
    {
      update = g_list_append (update, "default-image-size");
    }
  if (gimp->config->default_units != old_default_units)
    {
      update = g_list_append (update, "default-units");
    }
  if (ABS (gimp->config->default_xresolution - old_default_xresolution) > GIMP_MIN_RESOLUTION)
    {
      update = g_list_append (update, "default-xresolution");
    }
  if (ABS (gimp->config->default_yresolution - old_default_yresolution) > GIMP_MIN_RESOLUTION)
    {
      update = g_list_append (update, "default-yresolution");
    }
  if (gimp->config->default_resolution_units != old_default_resolution_units)
    {
      update = g_list_append (update, "default-resolution-units");
    }
  if (gimp->config->default_type != old_default_type)
    {
      update = g_list_append (update, "default-image-type");
    }
  if (prefs_strcmp (gimp->config->default_comment, old_default_comment))
    {
      update = g_list_append (update, "default-comment");
    }
  if (gimprc.default_dot_for_dot != old_default_dot_for_dot)
    {
      update = g_list_append (update, "default-dot-for-dot");
      remove = g_list_append (remove, "dont-default-dot-for-dot");
    }
  if (gimprc.preview_size != old_preview_size)
    {
      update = g_list_append (update, "preview-size");
    }
  if (gimprc.nav_preview_size != old_nav_preview_size)
    {
      update = g_list_append (update, "nav-preview-size");
    }
  if (gimprc.perfectmouse != old_perfectmouse)
    {
      update = g_list_append (update, "perfect-mouse");
    }
  if (gimprc.transparency_type != old_transparency_type)
    {
      update = g_list_append (update, "transparency-type");
    }
  if (gimprc.transparency_size != old_transparency_size)
    {
      update = g_list_append (update, "transparency-size");
    }
  if (gimprc.using_xserver_resolution != old_using_xserver_resolution ||
      ABS(gimprc.monitor_xres - old_monitor_xres) > GIMP_MIN_RESOLUTION)
    {
      update = g_list_append (update, "monitor-xresolution");
    }
  if (gimprc.using_xserver_resolution != old_using_xserver_resolution ||
      ABS(gimprc.monitor_yres - old_monitor_yres) > GIMP_MIN_RESOLUTION)
    {
      update = g_list_append (update, "monitor-yresolution");
    }
  if (gimprc.using_xserver_resolution)
    {
      /* special value of 0 for either x or y res in the gimprc file
       * means use the xserver's current resolution */
      gimprc.monitor_xres = 0.0;
      gimprc.monitor_yres = 0.0;
    }
  if (prefs_strcmp (gimprc.image_title_format, old_image_title_format))
    {
      update = g_list_append (update, "image-title-format");
    }
  if (gimprc.global_paint_options != old_global_paint_options)
    {
      update = g_list_append (update, "global-paint-options");
      remove = g_list_append (remove, "no-global-paint-options");
    }
  if (gimprc.max_new_image_size != old_max_new_image_size)
    {
      update = g_list_append (update, "max-new-image-size");
    }
  if (gimp->config->write_thumbnails != old_write_thumbnails)
    {
      update = g_list_append (update, "thumbnail-mode");
    }
  if (gimprc.trust_dirty_flag != old_trust_dirty_flag)
    {
      update = g_list_append (update, "trust-dirty-flag");
      remove = g_list_append (remove, "dont-trust-dirty-flag");
    }
  if (gimprc.use_help != old_use_help)
    {
      update = g_list_append (update, "use-help");
      remove = g_list_append (remove, "dont-use-help");
    }
  if (gimprc.help_browser != old_help_browser)
    {
      update = g_list_append (update, "help-browser");
    }
  if (gimprc.cursor_mode != old_cursor_mode)
    {
      update = g_list_append (update, "cursor-mode");
    }
  if (gimprc.default_threshold != old_default_threshold)
    {
      update = g_list_append (update, "default-threshold");
    }
  if (base_config->num_processors != old_num_processors)
    {
      update = g_list_append (update, "num-processors");
    }

  /*  values which can't be changed on the fly  */
  if (edit_stingy_memory_use != old_stingy_memory_use)
    {
      base_config->stingy_memory_use = edit_stingy_memory_use;
      update = g_list_append (update, "stingy-memory-use");
    }
  if (edit_min_colors != old_min_colors)
    {
      gimprc.min_colors = edit_min_colors;
      update = g_list_append (update, "min-colors");
    }
  if (edit_install_cmap != old_install_cmap)
    {
      gimprc.install_cmap = edit_install_cmap;
      update = g_list_append (update, "install-colormap");
    }
  if (edit_cycled_marching_ants != old_cycled_marching_ants)
    {
      gimprc.cycled_marching_ants = edit_cycled_marching_ants;
      update = g_list_append (update, "colormap-cycling");
    }
  if (edit_last_opened_size != old_last_opened_size)
    {
      gimprc.last_opened_size = edit_last_opened_size;
      update = g_list_append (update, "last-opened-size");
    }
  if (edit_show_indicators != old_show_indicators)
    {
      gimprc.show_indicators = edit_show_indicators;
      update = g_list_append (update, "show-indicators");
      remove = g_list_append (remove, "dont-show-indicators");
    }
  if (edit_nav_window_per_display != old_nav_window_per_display)
    {
      gimprc.nav_window_per_display = edit_nav_window_per_display;
      update = g_list_append (update, "nav-window-per-display");
      remove = g_list_append (remove, "nav-window-follows-auto");
    }
  if (edit_info_window_follows_mouse != old_info_window_follows_mouse)
    {
      gimprc.info_window_follows_mouse = edit_info_window_follows_mouse;
      update = g_list_append (update, "info-window-follows-mouse");
      remove = g_list_append (remove, "info-window-per-display");
    }
  if (edit_disable_tearoff_menus != old_disable_tearoff_menus)
    {
      gimprc.disable_tearoff_menus = edit_disable_tearoff_menus;
      update = g_list_append (update, "disable-tearoff-menus");
    }

  if (prefs_strcmp (old_temp_path, edit_temp_path))
    {
      base_config->temp_path = edit_temp_path;
      update = g_list_append (update, "temp-path");
    }
  if (prefs_strcmp (old_swap_path, edit_swap_path))
    {
      base_config->swap_path = edit_swap_path;
      update = g_list_append (update, "swap-path");
    }
  if (prefs_strcmp (old_plug_in_path, edit_plug_in_path))
    {
      gimp->config->plug_in_path = edit_plug_in_path;
      update = g_list_append (update, "plug-in-path");
    }
  if (prefs_strcmp (old_tool_plug_in_path, edit_tool_plug_in_path))
    {
      gimp->config->tool_plug_in_path = edit_tool_plug_in_path;
      update = g_list_append (update, "tool-plug-in-path");
    }
  if (prefs_strcmp (old_module_path, edit_module_path))
    {
      gimp->config->module_path = edit_module_path;
      update = g_list_append (update, "module-path");
    }
  if (prefs_strcmp (old_brush_path, edit_brush_path))
    {
      gimp->config->brush_path = edit_brush_path;
      update = g_list_append (update, "brush-path");
    }
  if (prefs_strcmp (old_pattern_path, edit_pattern_path))
    {
      gimp->config->pattern_path = edit_pattern_path;
      update = g_list_append (update, "pattern-path");
    }
  if (prefs_strcmp (old_palette_path, edit_palette_path))
    {
      gimp->config->palette_path = edit_palette_path;
      update = g_list_append (update, "palette-path");
    }
  if (prefs_strcmp (old_gradient_path, edit_gradient_path))
    {
      gimp->config->gradient_path = edit_gradient_path;
      update = g_list_append (update, "gradient-path");
    }
  if (prefs_strcmp (old_theme_path, edit_theme_path))
    {
      gimprc.theme_path = edit_theme_path;
      update = g_list_append (update, "theme-path");
    }

  /*  values which are changed on "OK" or "Save"  */
  if (edit_tile_cache_size != old_tile_cache_size)
    {
      base_config->tile_cache_size = edit_tile_cache_size;
      update = g_list_append (update, "tile-cache-size");
    }


  gimprc_save (&update, &remove);


  if (gimprc.using_xserver_resolution)
    gui_get_screen_resolution (&gimprc.monitor_xres,
                               &gimprc.monitor_yres);

  /*  restore variables which must not change  */
  base_config->stingy_memory_use = save_stingy_memory_use;
  gimprc.min_colors                     = save_min_colors;
  gimprc.install_cmap                   = save_install_cmap;
  gimprc.cycled_marching_ants           = save_cycled_marching_ants;
  gimprc.last_opened_size               = save_last_opened_size;
  gimprc.show_indicators                = save_show_indicators;
  gimprc.nav_window_per_display         = save_nav_window_per_display;
  gimprc.info_window_follows_mouse      = save_info_window_follows_mouse;

  base_config->temp_path     = save_temp_path;
  base_config->swap_path     = save_swap_path;

  gimp->config->plug_in_path       = save_plug_in_path;
  gimp->config->tool_plug_in_path  = save_tool_plug_in_path;
  gimp->config->module_path        = save_module_path;
  gimp->config->brush_path         = save_brush_path;
  gimp->config->pattern_path       = save_pattern_path;
  gimp->config->palette_path       = save_palette_path;
  gimp->config->gradient_path      = save_gradient_path;

  gimprc.theme_path = save_theme_path;

  /*  no need to restore values which are only changed on "OK" and "Save"  */

  g_list_free (update);
  g_list_free (remove);
}

static void
prefs_cancel_callback (GtkWidget *widget,
		       GtkWidget *dlg)
{
  Gimp *gimp;

  gimp = GIMP (g_object_get_data (G_OBJECT (dlg), "gimp"));

  gtk_widget_destroy (dlg);

  /*  restore ordinary gimprc variables  */
  base_config->interpolation_type        = old_interpolation_type;
  base_config->num_processors            = old_num_processors;

  gimp->config->default_type             = old_default_type;
  gimp->config->default_width            = old_default_width;
  gimp->config->default_height           = old_default_height;
  gimp->config->default_units            = old_default_units;
  gimp->config->default_xresolution      = old_default_xresolution;
  gimp->config->default_yresolution      = old_default_yresolution;
  gimp->config->default_resolution_units = old_default_resolution_units;
  gimp->config->levels_of_undo           = old_levels_of_undo;
  gimp->config->write_thumbnails         = old_write_thumbnails;

  gimprc.marching_speed                  = old_marching_speed;
  gimprc.resize_windows_on_zoom          = old_resize_windows_on_zoom;
  gimprc.resize_windows_on_resize        = old_resize_windows_on_resize;
  gimprc.auto_save                       = old_auto_save;
  gimprc.no_cursor_updating              = old_no_cursor_updating;
  gimprc.perfectmouse                    = old_perfectmouse;
  gimprc.show_tool_tips                  = old_show_tool_tips;
  gimprc.show_rulers                     = old_show_rulers;
  gimprc.show_statusbar                  = old_show_statusbar;
  gimprc.confirm_on_close                = old_confirm_on_close;
  gimprc.save_session_info               = old_save_session_info;
  gimprc.save_device_status              = old_save_device_status;
  gimprc.default_dot_for_dot             = old_default_dot_for_dot;
  gimprc.monitor_xres                    = old_monitor_xres;
  gimprc.monitor_yres                    = old_monitor_yres;
  gimprc.using_xserver_resolution        = old_using_xserver_resolution;
  gimprc.max_new_image_size              = old_max_new_image_size;
  gimprc.trust_dirty_flag                = old_trust_dirty_flag;
  gimprc.use_help                        = old_use_help;
  gimprc.help_browser                    = old_help_browser;
  gimprc.cursor_mode                     = old_cursor_mode;
  gimprc.default_threshold               = old_default_threshold;

  /*  restore variables which need some magic  */
  if (gimprc.preview_size != old_preview_size)
    {
#ifdef __GNUC__
#warning FIXME: update preview size
#endif
#if 0
      lc_dialog_rebuild (old_preview_size);
#endif
    }
  if (gimprc.nav_preview_size != old_nav_preview_size)
    {
      gimprc.nav_preview_size = old_nav_preview_size;
      gdisplays_nav_preview_resized ();
    }
  if ((gimprc.transparency_type != old_transparency_type) ||
      (gimprc.transparency_size != old_transparency_size))
    {
      gimprc.transparency_type = old_transparency_type;
      gimprc.transparency_size = old_transparency_size;

      render_setup (gimprc.transparency_type, gimprc.transparency_size);

      gimp_container_foreach (gimp->images,
			      (GFunc) gimp_image_invalidate_layer_previews,
			      NULL);

      gdisplays_expose_full ();
      gdisplays_flush ();
    }

  prefs_strset (&gimprc.image_title_format,     old_image_title_format);
  prefs_strset (&gimp->config->default_comment, old_default_comment);

  tool_manager_set_global_paint_options (gimp, old_global_paint_options);

  /*  restore values which need a restart  */
  edit_stingy_memory_use         = old_stingy_memory_use;
  edit_min_colors                = old_min_colors;
  edit_install_cmap              = old_install_cmap;
  edit_cycled_marching_ants      = old_cycled_marching_ants;
  edit_last_opened_size          = old_last_opened_size;
  edit_show_indicators           = old_show_indicators;
  edit_nav_window_per_display    = old_nav_window_per_display;
  edit_info_window_follows_mouse = old_info_window_follows_mouse;
  edit_disable_tearoff_menus     = old_disable_tearoff_menus;

  prefs_strset (&edit_temp_path,           old_temp_path);
  prefs_strset (&edit_swap_path,           old_swap_path);

  prefs_strset (&edit_plug_in_path,        old_plug_in_path);
  prefs_strset (&edit_tool_plug_in_path,   old_tool_plug_in_path);
  prefs_strset (&edit_module_path,         old_module_path);
  prefs_strset (&edit_brush_path,          old_brush_path);
  prefs_strset (&edit_pattern_path,        old_pattern_path);
  prefs_strset (&edit_palette_path,        old_palette_path);
  prefs_strset (&edit_gradient_path,       old_gradient_path);

  prefs_strset (&edit_theme_path,          old_theme_path);

  /*  no need to restore values which are only changed on "OK" and "Save"  */
}

static void
prefs_toggle_callback (GtkWidget *widget,
		       gpointer   data)
{
  GtkWidget *dialog;
  Gimp      *gimp;
  gint      *val;

  if (GTK_IS_MENU_ITEM (widget))
    {
      dialog =
        gtk_widget_get_toplevel (gtk_menu_get_attach_widget (GTK_MENU (widget->parent)));
    }
  else
    {
      dialog = gtk_widget_get_toplevel (widget);
    }

  gimp = GIMP (g_object_get_data (G_OBJECT (dialog), "gimp"));

  val = (gint *) data;

  /*  toggle buttos  */
  if (data == &gimprc.resize_windows_on_zoom   ||
      data == &gimprc.resize_windows_on_resize ||
      data == &gimprc.auto_save                ||
      data == &gimprc.no_cursor_updating       ||
      data == &gimprc.perfectmouse             ||
      data == &gimprc.show_tool_tips           ||
      data == &gimprc.show_rulers              ||
      data == &gimprc.show_statusbar           ||
      data == &gimprc.confirm_on_close         ||
      data == &gimprc.save_session_info        ||
      data == &gimprc.save_device_status       ||
      data == &gimprc.always_restore_session   ||
      data == &gimprc.default_dot_for_dot      ||
      data == &gimprc.use_help                 ||

      data == &edit_stingy_memory_use          ||
      data == &edit_install_cmap               ||
      data == &edit_cycled_marching_ants       ||
      data == &edit_show_indicators            ||
      data == &edit_nav_window_per_display     ||
      data == &edit_info_window_follows_mouse  ||
      data == &edit_disable_tearoff_menus)
    {
      *val = GTK_TOGGLE_BUTTON (widget)->active;
    }

  /*  radio buttons  */
  else if (data == &base_config->interpolation_type ||
	   data == &gimp->config->default_type      ||
	   data == &gimp->config->write_thumbnails  ||
           data == &gimprc.trust_dirty_flag         ||
	   data == &gimprc.help_browser             ||
	   data == &gimprc.cursor_mode)
    {
      *val = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
			      "gimp-item-data"));
    }

  /*  values which need some magic  */
  else if ((data == &gimprc.transparency_type) ||
	   (data == &gimprc.transparency_size))
    {
      *val = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
			      "gimp-item-data"));

      render_setup (gimprc.transparency_type, gimprc.transparency_size);
      gimp_container_foreach (gimp->images,
			      (GFunc) gimp_image_invalidate_layer_previews,
			      NULL);
      gdisplays_expose_full ();
      gdisplays_flush ();
    }
  else if (data == &gimprc.global_paint_options)
    {
      tool_manager_set_global_paint_options (gimp,
					     GTK_TOGGLE_BUTTON (widget)->active);
    }

  /*  no matching varible found  */
  else
    {
      /*  Are you a gimp-hacker who is getting this message?  You
       *  probably have to set your preferences value in one of the
       *  branches above...
       */
      g_warning ("Unknown prefs_toggle_callback() invoker - ignored.");
    }
}

static void
prefs_preview_size_callback (GtkWidget *widget,
			     gpointer   data)
{
#ifdef __GNUC__
#warning FIXME: update preview size
#endif
#if 0
  lc_dialog_rebuild ((long) g_object_get_data (G_OBJECT (widget),
                                               "gimp-item-data"));
#endif
}

static void
prefs_nav_preview_size_callback (GtkWidget *widget,
				 gpointer   data)
{
  gimprc.nav_preview_size =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "gimp-item-data"));

  gdisplays_nav_preview_resized ();
}

static void
prefs_string_callback (GtkWidget *widget,
		       gpointer   data)
{
  gchar **val;

  val = (gchar **) data;

  prefs_strset (val, gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void
prefs_text_callback (GtkTextBuffer *buffer,
		     gpointer       data)
{
  GtkTextIter   start_iter;
  GtkTextIter   end_iter;
  gchar       **val;
  gchar        *text;

  val = (gchar **) data;

  gtk_text_buffer_get_bounds (buffer, &start_iter, &end_iter);
  text = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);

  if (strlen (text) > MAX_COMMENT_LENGTH)
    {
      g_message (_("The default comment is limited to %d characters."), 
		 MAX_COMMENT_LENGTH);

      gtk_text_buffer_get_iter_at_offset (buffer, &start_iter,
					  MAX_COMMENT_LENGTH - 1);
      gtk_text_buffer_get_end_iter (buffer, &end_iter);

      /*  this calls us recursivaly, but in the else branch
       */
      gtk_text_buffer_delete (buffer, &start_iter, &end_iter);
    }
  else
    {
      prefs_strset (val, text);
    }

  g_free (text);
}

static void
prefs_filename_callback (GtkWidget *widget,
			 gpointer   data)
{
  gchar **val;
  gchar  *filename;

  val = (gchar **) data;

  filename = gimp_file_selection_get_filename (GIMP_FILE_SELECTION (widget));
  prefs_strset (val, filename);
  g_free (filename);
}

static void
prefs_path_callback (GtkWidget *widget,
		     gpointer   data)
{
  gchar **val;
  gchar  *path;

  val  = (gchar **) data;

  path = gimp_path_editor_get_path (GIMP_PATH_EDITOR (widget));
  prefs_strset (val, path);
  g_free (path);
}

static void
prefs_clear_session_info_callback (GtkWidget *widget,
				   gpointer   data)
{
#ifdef __GNUC__
#warning FIXME: g_list_free (session_info_updates);
#endif
#if 0
  g_list_free (session_info_updates);
  session_info_updates = NULL;
#endif
}

static void
prefs_default_size_callback (GtkWidget *widget,
			     gpointer   data)
{
  Gimp *gimp;

  gimp = GIMP (data);

  gimp->config->default_width =
    RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0));

  gimp->config->default_height =
    RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1));

  gimp->config->default_units =
    gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (widget));
}

static void
prefs_default_resolution_callback (GtkWidget *widget,
				   gpointer   data)
{
  GtkWidget      *dialog;
  Gimp           *gimp;
  GtkWidget      *size_sizeentry;
  static gdouble  xres = 0.0;
  static gdouble  yres = 0.0;
  gdouble         new_xres;
  gdouble         new_yres;

  dialog = gtk_widget_get_toplevel (widget);

  gimp = GIMP (g_object_get_data (G_OBJECT (dialog), "gimp"));

  new_xres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_yres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  size_sizeentry = g_object_get_data (G_OBJECT (widget), "size_sizeentry");

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

  gimp->config->default_width =
    RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size_sizeentry), 0));
  gimp->config->default_height =
    RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size_sizeentry), 1));

  gimp->config->default_xresolution = xres;
  gimp->config->default_yresolution = yres;

  gimp->config->default_resolution_units = 
    gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (widget));
}

static void
prefs_res_source_callback (GtkWidget *widget,
			   gpointer   data)
{
  GtkWidget *monitor_resolution_sizeentry;

  gimp_toggle_button_sensitive_update (GTK_TOGGLE_BUTTON (widget));

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      gui_get_screen_resolution (&gimprc.monitor_xres,
                                 &gimprc.monitor_yres);
      gimprc.using_xserver_resolution = TRUE;
    }
  else
    {
      monitor_resolution_sizeentry =
	g_object_get_data (G_OBJECT (widget), "monitor_resolution_sizeentry");
      
      if (monitor_resolution_sizeentry)
	{
	  gimprc.monitor_xres = gimp_size_entry_get_refval
	    (GIMP_SIZE_ENTRY (monitor_resolution_sizeentry), 0);
	  gimprc.monitor_yres = gimp_size_entry_get_refval
	    (GIMP_SIZE_ENTRY (monitor_resolution_sizeentry), 1);
	}
      gimprc.using_xserver_resolution = FALSE;
    }
}

static void
prefs_monitor_resolution_callback (GtkWidget *widget,
				   gpointer   data)
{
  static gdouble xres = 0.0;
  static gdouble yres = 0.0;
  gdouble        new_xres;
  gdouble        new_yres;

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

  gimprc.monitor_xres = xres;
  gimprc.monitor_yres = yres;
}

static void
prefs_resolution_calibrate_callback (GtkWidget *widget,
				     gpointer   data)
{
  resolution_calibrate_dialog (GTK_WIDGET (data), NULL, NULL, NULL);
}

/*  create a new notebook page  */
static GtkWidget *
prefs_notebook_append_page (GtkNotebook   *notebook,
			    gchar         *notebook_label,
			    GtkTreeStore  *tree,
			    gchar         *tree_label,
			    gchar         *help_data,
			    GtkTreeIter   *parent,
			    GtkTreeIter   *iter,
			    gint           page_index)
{
  GtkWidget   *event_box;
  GtkWidget   *out_vbox;
  GtkWidget   *vbox;
  GtkWidget   *frame;
  GtkWidget   *label;

  event_box = gtk_event_box_new ();
  gtk_notebook_append_page (notebook, event_box, NULL);
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

  gtk_tree_store_append (tree, iter, parent);
  gtk_tree_store_set (tree, iter, 0, tree_label, 1, page_index, -1);

  return vbox;
}

/*  select a notebook page  */
static void
prefs_tree_select_callback (GtkTreeSelection *sel,
			    GtkNotebook      *notebook)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GValue        val = { 0, };

  if (! gtk_tree_selection_get_selected (sel, &model, &iter))
    return;

  gtk_tree_model_get_value (model, &iter, 1, &val);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook),
				 g_value_get_int (&val));

  g_value_unset (&val);
}

/*  create a frame with title and a vbox  */
static GtkWidget *
prefs_frame_new (gchar  *label,
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
prefs_help_func (const gchar *help_data)
{
  GtkWidget *notebook;
  GtkWidget *event_box;
  gint       page_num;

  notebook  = g_object_get_data (G_OBJECT (prefs_dialog), "notebook");
  page_num  = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
  event_box = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), page_num);

  help_data = g_object_get_data (G_OBJECT (event_box), "gimp_help_data");
  gimp_standard_help_func (help_data);
}

/************************************************************************
 *  create the preferences dialog
 */
GtkWidget *
preferences_dialog_create (Gimp *gimp)
{
  GtkWidget        *tv;
  GtkTreeStore     *tree;
  GtkTreeSelection *sel;
  GtkTreeIter       top_iter;
  GtkTreeIter       child_iter;
  gint              page_index;

  GtkWidget        *frame;
  GtkWidget        *notebook;
  GtkWidget        *vbox;
  GtkWidget        *vbox2;
  GtkWidget        *hbox;
  GtkWidget        *abox;
  GtkWidget        *button;
  GtkWidget        *fileselection;
  GtkWidget        *patheditor;
  GtkWidget        *spinbutton;
  GtkWidget        *optionmenu;
  GtkWidget        *table;
  GtkWidget        *label;
  GtkObject        *adjustment;
  GtkWidget        *sizeentry;
  GtkWidget        *sizeentry2;
  GtkWidget        *separator;
  GtkWidget        *calibrate_button;
  GtkWidget        *scrolled_window;
  GtkWidget        *text_view;
  GtkTextBuffer    *text_buffer;
  GSList           *group;

  gint   i;
  gchar *pixels_per_unit;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (prefs_dialog)
    return prefs_dialog;

  if (edit_temp_path == NULL)
    {
      /*  first time dialog is opened -
       *  copy config vals to edit variables.
       */
      edit_stingy_memory_use         = base_config->stingy_memory_use;
      edit_min_colors                = gimprc.min_colors;
      edit_install_cmap              = gimprc.install_cmap;
      edit_cycled_marching_ants      = gimprc.cycled_marching_ants;
      edit_last_opened_size          = gimprc.last_opened_size;
      edit_show_indicators           = gimprc.show_indicators;
      edit_nav_window_per_display    = gimprc.nav_window_per_display;
      edit_info_window_follows_mouse = gimprc.info_window_follows_mouse;
      edit_disable_tearoff_menus     = gimprc.disable_tearoff_menus;

      edit_temp_path      = prefs_strdup (base_config->temp_path);	
      edit_swap_path      = prefs_strdup (base_config->swap_path);

      edit_plug_in_path       = prefs_strdup (gimp->config->plug_in_path);
      edit_tool_plug_in_path  = prefs_strdup (gimp->config->tool_plug_in_path);
      edit_module_path        = prefs_strdup (gimp->config->module_path);
      edit_brush_path         = prefs_strdup (gimp->config->brush_path);
      edit_pattern_path       = prefs_strdup (gimp->config->pattern_path);
      edit_palette_path       = prefs_strdup (gimp->config->palette_path);
      edit_gradient_path      = prefs_strdup (gimp->config->gradient_path);

      edit_theme_path     = prefs_strdup (gimprc.theme_path);
    }

  /*  assign edit variables for values which get changed on "OK" and "Save"
   *  but not on the fly.
   */
  edit_tile_cache_size = base_config->tile_cache_size;

  /*  remember all old values  */
  old_interpolation_type       = base_config->interpolation_type;
  old_num_processors           = base_config->num_processors;

  old_default_type             = gimp->config->default_type;
  old_default_width            = gimp->config->default_width;
  old_default_height           = gimp->config->default_height;
  old_default_units            = gimp->config->default_units;
  old_default_xresolution      = gimp->config->default_xresolution;
  old_default_yresolution      = gimp->config->default_yresolution;
  old_default_resolution_units = gimp->config->default_resolution_units;
  old_levels_of_undo           = gimp->config->levels_of_undo;
  old_write_thumbnails         = gimp->config->write_thumbnails;

  old_perfectmouse             = gimprc.perfectmouse;
  old_transparency_type        = gimprc.transparency_type;
  old_transparency_size        = gimprc.transparency_size;
  old_marching_speed           = gimprc.marching_speed;
  old_resize_windows_on_zoom   = gimprc.resize_windows_on_zoom;
  old_resize_windows_on_resize = gimprc.resize_windows_on_resize;
  old_auto_save                = gimprc.auto_save;
  old_preview_size             = gimprc.preview_size;
  old_nav_preview_size         = gimprc.nav_preview_size;
  old_no_cursor_updating       = gimprc.no_cursor_updating;
  old_show_tool_tips           = gimprc.show_tool_tips;
  old_show_rulers              = gimprc.show_rulers;
  old_show_statusbar           = gimprc.show_statusbar;
  old_confirm_on_close         = gimprc.confirm_on_close;
  old_save_session_info        = gimprc.save_session_info;
  old_save_device_status       = gimprc.save_device_status;
  old_always_restore_session   = gimprc.always_restore_session;
  old_default_dot_for_dot      = gimprc.default_dot_for_dot;
  old_monitor_xres             = gimprc.monitor_xres;
  old_monitor_yres             = gimprc.monitor_yres;
  old_using_xserver_resolution = gimprc.using_xserver_resolution;
  old_global_paint_options     = gimprc.global_paint_options;
  old_max_new_image_size       = gimprc.max_new_image_size;
  old_trust_dirty_flag         = gimprc.trust_dirty_flag;
  old_use_help                 = gimprc.use_help;
  old_help_browser             = gimprc.help_browser;
  old_cursor_mode              = gimprc.cursor_mode;
  old_default_threshold        = gimprc.default_threshold;

  prefs_strset (&old_image_title_format, gimprc.image_title_format);	
  prefs_strset (&old_default_comment,    gimp->config->default_comment);	

  /*  values which will need a restart  */
  old_stingy_memory_use         = edit_stingy_memory_use;
  old_min_colors                = edit_min_colors;
  old_install_cmap              = edit_install_cmap;
  old_cycled_marching_ants      = edit_cycled_marching_ants;
  old_last_opened_size          = edit_last_opened_size;
  old_show_indicators           = edit_show_indicators;
  old_nav_window_per_display    = edit_nav_window_per_display;
  old_info_window_follows_mouse = edit_info_window_follows_mouse;
  old_disable_tearoff_menus     = edit_disable_tearoff_menus;

  prefs_strset (&old_temp_path,          edit_temp_path);
  prefs_strset (&old_swap_path,          edit_swap_path);
  prefs_strset (&old_plug_in_path,       edit_plug_in_path);
  prefs_strset (&old_tool_plug_in_path,  edit_tool_plug_in_path);
  prefs_strset (&old_module_path,        edit_module_path);
  prefs_strset (&old_brush_path,         edit_brush_path);
  prefs_strset (&old_pattern_path,       edit_pattern_path);
  prefs_strset (&old_palette_path,       edit_palette_path);
  prefs_strset (&old_gradient_path,      edit_gradient_path);
  prefs_strset (&old_theme_path,         edit_theme_path);

  /*  values which will be changed on "OK" and "Save"  */
  old_tile_cache_size = edit_tile_cache_size;

  /* Create the dialog */
  prefs_dialog = gimp_dialog_new (_("Preferences"), "gimp_preferences",
                                  prefs_help_func,
                                  "dialogs/preferences/preferences.html",
                                  GTK_WIN_POS_NONE,
                                  FALSE, TRUE, FALSE,

                                  GTK_STOCK_CANCEL, prefs_cancel_callback,
                                  NULL, NULL, NULL, FALSE, TRUE,

                                  GTK_STOCK_SAVE, prefs_save_callback,
                                  NULL, NULL, NULL, FALSE, FALSE,

                                  GTK_STOCK_OK, prefs_ok_callback,
                                  NULL, NULL, NULL, TRUE, FALSE,

                                  NULL);

  g_object_set_data (G_OBJECT (prefs_dialog), "gimp", gimp);

  g_object_add_weak_pointer (G_OBJECT (prefs_dialog),
                             (gpointer) &prefs_dialog);

  /* The main hbox */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (prefs_dialog)->vbox), hbox);
  gtk_widget_show (hbox);

  /* The categories tree */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  tree = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_INT);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (tree));
  g_object_unref (G_OBJECT (tree));

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv),
					       -1, _("Categories"),
					       gtk_cell_renderer_text_new (),
					       "text", 0, NULL);

  gtk_container_add (GTK_CONTAINER (frame), tv);

  /* The main preferences notebook */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_container_add (GTK_CONTAINER (frame), notebook);

  g_object_set_data (G_OBJECT (prefs_dialog), "notebook", notebook);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
  g_signal_connect (G_OBJECT (sel), "changed",
		    G_CALLBACK (prefs_tree_select_callback),
		    notebook);

  page_index = 0;

  /* New File page */
  vbox = prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
				     _("New File"),
				     GTK_TREE_STORE (tree),
				     _("New File"),
				     "dialogs/preferences/new_file.html",
				     NULL,
				     &top_iter,
				     page_index);
  gtk_widget_show (vbox);
  page_index++;

  /* select this page in the tree */
  gtk_tree_selection_select_iter (sel, &top_iter);

  frame = gtk_frame_new (_("Default Image Size and Unit")); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  sizeentry =
    gimp_size_entry_new (2, gimp->config->default_units, "%p",
			 FALSE, FALSE, TRUE, 75,
			 GIMP_SIZE_ENTRY_UPDATE_SIZE);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Width"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Height"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Pixels"), 1, 4, 0.0);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 0,
				  gimp->config->default_xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 1,
				  gimp->config->default_yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries
    (GIMP_SIZE_ENTRY (sizeentry), 0, GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries
    (GIMP_SIZE_ENTRY (sizeentry), 1, GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0,
			      gimp->config->default_width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 1,
			      gimp->config->default_height);

  g_signal_connect (G_OBJECT (sizeentry), "unit_changed",
		    G_CALLBACK (prefs_default_size_callback),
		    gimp);
  g_signal_connect (G_OBJECT (sizeentry), "value_changed",
		    G_CALLBACK (prefs_default_size_callback),
		    gimp);
  g_signal_connect (G_OBJECT (sizeentry), "refval_changed",
		    G_CALLBACK (prefs_default_size_callback),
		    gimp);

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

  sizeentry2 = gimp_size_entry_new (2, gimp->config->default_resolution_units,
				    pixels_per_unit,
				    FALSE, FALSE, TRUE, 75,
				    GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);

  button = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
  if (ABS (gimp->config->default_xresolution -
	   gimp->config->default_yresolution) < GIMP_MIN_RESOLUTION)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (button), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (sizeentry2), button, 1, 3, 3, 4);
  gtk_widget_show (button);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry2),
				_("Horizontal"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry2),
				_("Vertical"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry2),
				_("dpi"), 1, 4, 0.0);

  g_object_set_data (G_OBJECT (sizeentry2), "size_sizeentry", sizeentry);

  gimp_size_entry_set_refval_boundaries
    (GIMP_SIZE_ENTRY (sizeentry2), 0, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval_boundaries
    (GIMP_SIZE_ENTRY (sizeentry2), 1, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);
  
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry2), 0,
			      gimp->config->default_xresolution);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry2), 1,
			      gimp->config->default_yresolution);

  g_signal_connect (G_OBJECT (sizeentry2), "unit_changed",
		    G_CALLBACK (prefs_default_resolution_callback),
		    button);
  g_signal_connect (G_OBJECT (sizeentry2), "value_changed",
		    G_CALLBACK (prefs_default_resolution_callback),
		    button);
  g_signal_connect (G_OBJECT (sizeentry2), "refval_changed",
		    G_CALLBACK (prefs_default_resolution_callback),
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
    gimp_option_menu_new2 (FALSE,
			   G_CALLBACK (prefs_toggle_callback),
			   &gimp->config->default_type,
			   GINT_TO_POINTER (gimp->config->default_type),

			   _("RGB"),       GINT_TO_POINTER (GIMP_RGB),  NULL,
			   _("Grayscale"), GINT_TO_POINTER (GIMP_GRAY), NULL,

			   NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Default Image Type:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  /*  The maximum size of a new image  */  
  adjustment = gtk_adjustment_new (gimprc.max_new_image_size, 
				   0, (4069.0 * 1024 * 1024 - 1), 
				   1.0, 1.0, 0.0);
  hbox = gimp_mem_size_entry_new (GTK_ADJUSTMENT (adjustment));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Maximum Image Size:"), 1.0, 0.5,
			     hbox, 1, TRUE);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_uint_adjustment_update),
		    &gimprc.max_new_image_size);

  /* Default Comment page */
  vbox = prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
				     _("Default Comment"),
				     GTK_TREE_STORE (tree),
				     _("Default Comment"),
				     "dialogs/preferences/new_file.html#default_comment",
				     &top_iter,
				     &child_iter,
				     page_index);
  gtk_widget_show (vbox);
  page_index++;

  frame = gtk_frame_new (_("Comment Used for New Images"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 4);
  gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
  gtk_widget_show (scrolled_window);

  text_buffer = gtk_text_buffer_new (NULL);
  gtk_text_buffer_set_text (text_buffer, gimp->config->default_comment, -1);

  text_view = gtk_text_view_new_with_buffer (text_buffer);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  g_object_unref (G_OBJECT (text_buffer));

  g_signal_connect (G_OBJECT (text_buffer), "changed",
		    G_CALLBACK (prefs_text_callback),
		    &gimp->config->default_comment);

  /* Display page */
  vbox = prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
				     _("Display"),
				     GTK_TREE_STORE (tree),
				     _("Display"),
				     "dialogs/preferences/display.html",
				     NULL,
				     &top_iter,
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

  optionmenu = gimp_option_menu_new2
    (FALSE,
     G_CALLBACK (prefs_toggle_callback),
     &gimprc.transparency_type,
     GINT_TO_POINTER (gimprc.transparency_type),

     _("Light Checks"),    GINT_TO_POINTER (GIMP_LIGHT_CHECKS), NULL,
     _("Mid-Tone Checks"), GINT_TO_POINTER (GIMP_GRAY_CHECKS),  NULL,
     _("Dark Checks"),     GINT_TO_POINTER (GIMP_DARK_CHECKS),  NULL,
     _("White Only"),      GINT_TO_POINTER (GIMP_WHITE_ONLY),   NULL,
     _("Gray Only"),       GINT_TO_POINTER (GIMP_GRAY_ONLY),    NULL,
     _("Black Only"),      GINT_TO_POINTER (GIMP_BLACK_ONLY),   NULL,

     NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Transparency Type:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  optionmenu =
    gimp_option_menu_new2 (FALSE,
			   G_CALLBACK (prefs_toggle_callback),
			   &gimprc.transparency_size,
			   GINT_TO_POINTER (gimprc.transparency_size),

			   _("Small"),  
                           GINT_TO_POINTER (GIMP_SMALL_CHECKS),  NULL,
			   _("Medium"), 
                           GINT_TO_POINTER (GIMP_MEDIUM_CHECKS), NULL,
			   _("Large"),  
                           GINT_TO_POINTER (GIMP_LARGE_CHECKS),  NULL,

			   NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Check Size:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  vbox2 = prefs_frame_new (_("8-Bit Displays"), GTK_BOX (vbox));

  if (gdk_rgb_get_visual ()->depth != 8)
    gtk_widget_set_sensitive (GTK_WIDGET (vbox2->parent), FALSE);

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  spinbutton =
    gimp_spin_button_new (&adjustment, edit_min_colors,
			  27.0,
			  gtk_check_version (1, 2, 8) ? 216.0 : 256.0,
			  1.0, 8.0, 0.0, 1.0, 0.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Minimum Number of Colors:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &edit_min_colors);

  button = gtk_check_button_new_with_label(_("Install Colormap"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_install_cmap);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &edit_install_cmap);

  button = gtk_check_button_new_with_label(_("Colormap Cycling"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_cycled_marching_ants);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &edit_cycled_marching_ants);


  /* Interface */
  vbox = prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
				     _("Interface"),
				     GTK_TREE_STORE (tree),
				     _("Interface"),
				     "dialogs/preferences/interface.html",
				     NULL,
				     &top_iter,
				     page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = prefs_frame_new (_("General"), GTK_BOX (vbox));

  table = gtk_table_new (4, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

#if 0
  /*  Don't show the Auto-save button until we really 
   *  have auto-saving in the gimp.
   */
  button = gtk_check_button_new_with_label(_("Auto Save"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), auto_save);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (prefs_toggle_callback),
                    &auto_save);
#endif

  optionmenu =
    gimp_option_menu_new2 (FALSE,
			   G_CALLBACK (prefs_preview_size_callback),
			   &gimprc.preview_size,
			   GINT_TO_POINTER (gimprc.preview_size),

			   _("None"),
                           GINT_TO_POINTER (GIMP_PREVIEW_SIZE_NONE), NULL,
			   _("Tiny"),
                           GINT_TO_POINTER (GIMP_PREVIEW_SIZE_TINY), NULL,
			   _("Extra Small"),
                           GINT_TO_POINTER (GIMP_PREVIEW_SIZE_EXTRA_SMALL), NULL,
			   _("Small"),
                           GINT_TO_POINTER (GIMP_PREVIEW_SIZE_SMALL), NULL,
			   _("Medium"),
                           GINT_TO_POINTER (GIMP_PREVIEW_SIZE_MEDIUM), NULL,
			   _("Large"),
                           GINT_TO_POINTER (GIMP_PREVIEW_SIZE_LARGE), NULL,
			   _("Extra Large"),
                           GINT_TO_POINTER (GIMP_PREVIEW_SIZE_EXTRA_LARGE), NULL,
			   _("Huge"),
                           GINT_TO_POINTER (GIMP_PREVIEW_SIZE_HUGE), NULL,
			   _("Gigantic"),
                           GINT_TO_POINTER (GIMP_PREVIEW_SIZE_GIGANTIC), NULL,

			   NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Preview Size:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  optionmenu =
    gimp_option_menu_new2 (FALSE,
			   G_CALLBACK (prefs_nav_preview_size_callback),
			   &gimprc.nav_preview_size,
			   GINT_TO_POINTER (gimprc.nav_preview_size),

			   _("Small"),  GINT_TO_POINTER (48),  NULL,
			   _("Medium"), GINT_TO_POINTER (80),  NULL,
			   _("Large"),  GINT_TO_POINTER (112), NULL,

			   NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Nav Preview Size:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  spinbutton = gimp_spin_button_new (&adjustment, edit_last_opened_size,
				     0.0, 16.0, 1.0, 5.0, 0.0, 1.0, 0.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Recent Documents List Size:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &edit_last_opened_size);

  /* Indicators */
  vbox2 = prefs_frame_new (_("Toolbox"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label
    (_("Display Brush, Pattern and Gradient Indicators"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_show_indicators);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &edit_show_indicators);

  vbox2 = prefs_frame_new (_("Dialog Behaviour"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label (_("Navigation Window per Display"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_nav_window_per_display);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &edit_nav_window_per_display);

  button = gtk_check_button_new_with_label (_("Info Window Follows Mouse"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_info_window_follows_mouse);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &edit_info_window_follows_mouse);

  vbox2 = prefs_frame_new (_("Menus"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label (_("Disable Tearoff Menus"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_disable_tearoff_menus);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &edit_disable_tearoff_menus);

  /* Interface / Help System */
  vbox = prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
				     _("Help System"),
				     GTK_TREE_STORE (tree),
				     _("Help System"),
				     "dialogs/preferences/interface.html#help_system",
				     &top_iter,
				     &child_iter,
				     page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = prefs_frame_new (_("General"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label (_("Show Tool Tips"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.show_tool_tips);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.show_tool_tips);

  button =
    gtk_check_button_new_with_label (_("Context Sensitive Help with \"F1\""));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.use_help);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.use_help);

  vbox2 = prefs_frame_new (_("Help Browser"), GTK_BOX (vbox));

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  optionmenu = gimp_option_menu_new2
    (FALSE,
     G_CALLBACK (prefs_toggle_callback),
     &gimprc.help_browser,
     GINT_TO_POINTER (gimprc.help_browser),

     _("Internal"), GINT_TO_POINTER (HELP_BROWSER_GIMP), NULL,
     _("Netscape"), GINT_TO_POINTER (HELP_BROWSER_NETSCAPE), NULL,

     NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Help Browser to Use:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  /* Interface / Image Windows */
  vbox = prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
				     _("Image Windows"),
				     GTK_TREE_STORE (tree),
				     _("Image Windows"),
				     "dialogs/preferences/interface.html#image_windows",
				     &top_iter,
				     &child_iter,
				     page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = prefs_frame_new (_("Appearance"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label (_("Use \"Dot for Dot\" by default"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.default_dot_for_dot);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.default_dot_for_dot);

  button = gtk_check_button_new_with_label(_("Resize Window on Zoom"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.resize_windows_on_zoom);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.resize_windows_on_zoom);

  button = gtk_check_button_new_with_label(_("Resize Window on Image Size Change"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.resize_windows_on_resize);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.resize_windows_on_resize);

  button = gtk_check_button_new_with_label(_("Show Rulers"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.show_rulers);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.show_rulers);

  button = gtk_check_button_new_with_label(_("Show Statusbar"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.show_statusbar);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.show_statusbar);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adjustment, gimprc.marching_speed,
				     50.0, 32000.0, 10.0, 100.0, 1.0, 1.0, 0.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Marching Ants Speed:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &gimprc.marching_speed);

  /* The title format string */
  {
    GtkWidget *combo;
    GtkWidget *comboitem;

    const gchar *format_strings[] =
    {
      NULL,
      "%f-%p.%i (%t)",
      "%f-%p.%i (%t) %z%%",
      "%f-%p.%i (%t) %d:%s",
      "%f-%p.%i (%t) %s:%d",
      "%f-%p.%i (%t) %m"
    };

    const gchar *combo_strings[] =
    {
      N_("Custom"),
      N_("Standard"),
      N_("Show zoom percentage"),
      N_("Show zoom ratio"),
      N_("Show reversed zoom ratio"),
      N_("Show memory usage")
    };

    g_assert (G_N_ELEMENTS (format_strings) == G_N_ELEMENTS (combo_strings));

    format_strings[0] = gimprc.image_title_format;

    combo = gtk_combo_new ();
    gtk_combo_set_use_arrows (GTK_COMBO (combo), FALSE);
    gtk_combo_set_value_in_list (GTK_COMBO (combo), FALSE, FALSE);

    for (i = 0; i < G_N_ELEMENTS (combo_strings); i++)
      {
        comboitem = gtk_list_item_new_with_label (gettext (combo_strings[i]));
        gtk_combo_set_item_string (GTK_COMBO (combo), GTK_ITEM (comboitem),
                                   format_strings[i]);
        gtk_container_add (GTK_CONTAINER (GTK_COMBO (combo)->list), comboitem);
        gtk_widget_show (comboitem);
      }

    gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                               _("Image Title Format:"), 1.0, 0.5,
                               combo, 1, FALSE);

    g_signal_connect (G_OBJECT (GTK_COMBO (combo)->entry), "changed",
                      G_CALLBACK (prefs_string_callback), 
                      &gimprc.image_title_format);
  }

  vbox2 = prefs_frame_new (_("Pointer Movement Feedback"), GTK_BOX (vbox));

  button =
    gtk_check_button_new_with_label (_("Perfect-but-Slow Pointer Tracking"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.perfectmouse);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.perfectmouse);

  button = gtk_check_button_new_with_label (_("Disable Cursor Updating"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.no_cursor_updating);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.no_cursor_updating);

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  optionmenu =
    gimp_option_menu_new2 (FALSE,
			   G_CALLBACK (prefs_toggle_callback),
			   &gimprc.cursor_mode,
			   GINT_TO_POINTER (gimprc.cursor_mode),

			   _("Tool Icon"),
			   GINT_TO_POINTER (GIMP_CURSOR_MODE_TOOL_ICON),      NULL,
			   _("Tool Icon with Crosshair"),
			   GINT_TO_POINTER (GIMP_CURSOR_MODE_TOOL_CROSSHAIR), NULL,
			   _("Crosshair only"),
			   GINT_TO_POINTER (GIMP_CURSOR_MODE_CROSSHAIR),      NULL,

			   NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Cursor Mode:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);


  /* Interface / Tool Options */
  vbox = prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
				     _("Tool Options"),
				     GTK_TREE_STORE (tree),
				     _("Tool Options"),
				     "dialogs/preferences/interface.html#tool_options",
				     &top_iter,
				     &child_iter,
				     page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = prefs_frame_new (_("Paint Options"), GTK_BOX (vbox));

  button =
    gtk_check_button_new_with_label(_("Use Global Paint Options"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.global_paint_options);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.global_paint_options);

  vbox2 = prefs_frame_new (_("Finding Contiguous Regions"), GTK_BOX (vbox));

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Default threshold  */
  spinbutton = gimp_spin_button_new (&adjustment, gimprc.default_threshold,
				     0.0, 255.0, 1.0, 5.0, 0.0, 1.0, 0.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Default Threshold:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &gimprc.default_threshold);


  /* Environment */
  vbox = prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
				     _("Environment"),
				     GTK_TREE_STORE (tree),
				     _("Environment"),
				     "dialogs/preferences/environment.html",
				     NULL,
				     &top_iter,
				     page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = prefs_frame_new (_("Resource Consumption"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label(_("Conservative Memory Usage"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				edit_stingy_memory_use);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &edit_stingy_memory_use);

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
  spinbutton = gimp_spin_button_new (&adjustment,
				     gimp->config->levels_of_undo,
				     0.0, 255.0, 1.0, 5.0, 0.0, 1.0, 0.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Levels of Undo:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &gimp->config->levels_of_undo);

  /*  The tile cache size  */
  adjustment = gtk_adjustment_new (edit_tile_cache_size, 
				   0, (4069.0 * 1024 * 1024 - 1), 
				   1.0, 1.0, 0.0);
  hbox = gimp_mem_size_entry_new (GTK_ADJUSTMENT (adjustment));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Tile Cache Size:"), 1.0, 0.5,
			     hbox, 1, TRUE);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_uint_adjustment_update),
		    &edit_tile_cache_size);

#ifdef ENABLE_MP
  spinbutton =
    gimp_spin_button_new (&adjustment, base_config->num_processors,
			  1, 30, 1.0, 2.0, 0.0, 1.0, 0.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Number of Processors to Use:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &base_config->num_processors);
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
    gimp_option_menu_new2 (FALSE,
			   G_CALLBACK (prefs_toggle_callback),
			   &base_config->interpolation_type,
			   GINT_TO_POINTER (base_config->interpolation_type),

			   _("Nearest Neighbor (Fast)"),
			   GINT_TO_POINTER (GIMP_NEAREST_NEIGHBOR_INTERPOLATION), NULL,
			   _("Linear"),
			   GINT_TO_POINTER (GIMP_LINEAR_INTERPOLATION), NULL,
			   _("Cubic (Slow)"),
			   GINT_TO_POINTER (GIMP_CUBIC_INTERPOLATION), NULL,

			   NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Interpolation Type:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  vbox2 = prefs_frame_new (_("File Saving"), GTK_BOX (vbox));

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
    gimp_option_menu_new2 (FALSE,
			   G_CALLBACK (prefs_toggle_callback),
			   &gimp->config->write_thumbnails,
			   GINT_TO_POINTER (gimp->config->write_thumbnails),

			   _("Always"), GINT_TO_POINTER (TRUE),  NULL,
			   _("Never"),  GINT_TO_POINTER (FALSE), NULL,

			   NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Try to Write a Thumbnail File:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  optionmenu =
    gimp_option_menu_new2 (FALSE,
			   G_CALLBACK (prefs_toggle_callback),
			   &gimprc.trust_dirty_flag,
			   GINT_TO_POINTER (gimprc.trust_dirty_flag),

			   _("Only when Modified"), GINT_TO_POINTER (TRUE),  NULL,
			   _("Always"),             GINT_TO_POINTER (FALSE), NULL,

			   NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("\"File > Save\" Saves the Image:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);
                                     

  /* Session Management */
  vbox = prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
				     _("Session Management"),
				     GTK_TREE_STORE (tree),
				     _("Session"),
				     "dialogs/preferences/session.html",
				     NULL,
				     &top_iter,
				     page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = prefs_frame_new (_("Window Positions"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label (_("Save Window Positions on Exit"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.save_session_info);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.save_session_info);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
      
  button = gtk_button_new_with_label (_("Clear Saved Window Positions Now"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (prefs_clear_session_info_callback),
		    NULL);

  button = gtk_check_button_new_with_label (_("Always Try to Restore Session"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.always_restore_session);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.always_restore_session);

  vbox2 = prefs_frame_new (_("Devices"), GTK_BOX (vbox));

  button = gtk_check_button_new_with_label (_("Save Device Status on Exit"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gimprc.save_device_status);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    &gimprc.save_device_status);

  /* Monitor */
  vbox = prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
				     _("Monitor"),
				     GTK_TREE_STORE (tree),
				     _("Monitor"),
				     "dialogs/preferences/monitor.html",
				     NULL,
				     &top_iter,
				     page_index);
  gtk_widget_show (vbox);
  page_index++;

  vbox2 = prefs_frame_new (_("Get Monitor Resolution"), GTK_BOX (vbox));

  {
    gdouble  xres, yres;
    gchar   *str;

    gui_get_screen_resolution (&xres, &yres);

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
  if (ABS (gimprc.monitor_xres - gimprc.monitor_yres) < GIMP_MIN_RESOLUTION)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (button), TRUE);
  g_object_set_data (G_OBJECT (sizeentry), "chain_button", button); 
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

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0,
			      gimprc.monitor_xres);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 1,
			      gimprc.monitor_yres);

  g_signal_connect (G_OBJECT (sizeentry), "value_changed",
		    G_CALLBACK (prefs_monitor_resolution_callback),
		    button);
  g_signal_connect (G_OBJECT (sizeentry), "refval_changed",
		    G_CALLBACK (prefs_monitor_resolution_callback),
		    button);

  gtk_container_add (GTK_CONTAINER (abox), sizeentry);
  gtk_widget_show (sizeentry);
  gtk_widget_set_sensitive (sizeentry, ! gimprc.using_xserver_resolution);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  
  calibrate_button = gtk_button_new_with_label (_("Calibrate"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (calibrate_button)->child), 4, 0);
  gtk_box_pack_start (GTK_BOX (hbox), calibrate_button, FALSE, FALSE, 0);
  gtk_widget_show (calibrate_button);
  gtk_widget_set_sensitive (calibrate_button, ! gimprc.using_xserver_resolution);

  g_signal_connect (G_OBJECT (calibrate_button), "clicked",
		    G_CALLBACK (prefs_resolution_calibrate_callback),
		    sizeentry);

  group = NULL;
  button = gtk_radio_button_new_with_label (group, _("From windowing system"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_res_source_callback),
		    NULL);
  g_object_set_data (G_OBJECT (button), "monitor_resolution_sizeentry",
                     sizeentry);
  g_object_set_data (G_OBJECT (button), "set_sensitive",
                     label);
  g_object_set_data (G_OBJECT (button), "inverse_sensitive",
                     sizeentry);
  g_object_set_data (G_OBJECT (sizeentry), "inverse_sensitive",
                     calibrate_button);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, FALSE, 0);
  gtk_widget_show (separator);
  
  button = gtk_radio_button_new_with_label (group, _("Manually:"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  
  gtk_box_pack_start (GTK_BOX (vbox2), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  if (! gimprc.using_xserver_resolution)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  /* Directories */
  vbox = prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
				     _("Directories"),
				     GTK_TREE_STORE (tree),
				     _("Directories"),
				     "dialogs/preferences/directories.html",
				     NULL,
				     &top_iter,
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

    table = gtk_table_new (G_N_ELEMENTS (dirs) + 1, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 2);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
    gtk_widget_show (table);

    for (i = 0; i < G_N_ELEMENTS (dirs); i++)
      {
	fileselection = gimp_file_selection_new (gettext (dirs[i].fs_label),
						 *(dirs[i].mdir),
						 TRUE, TRUE);
	gimp_table_attach_aligned (GTK_TABLE (table), 0, i,
				   gettext (dirs[i].label), 1.0, 0.5,
				   fileselection, 1, FALSE);

	g_signal_connect (G_OBJECT (fileselection), "filename_changed",
			  G_CALLBACK (prefs_filename_callback),
			  dirs[i].mdir);
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
      { N_("Tool Plug-Ins"), N_("Tool Plug-Ins Directories"),
	"dialogs/preferences/directories.html#tool_plug_ins",
	N_("Select Tool Plug-Ins Dir"),
	&edit_tool_plug_in_path },
      { N_("Modules"), N_("Modules Directories"),
	"dialogs/preferences/directories.html#modules",
	N_("Select Modules Dir"),
	&edit_module_path },
      { N_("Themes"), N_("Themes Directories"),
	"dialogs/preferences/directories.html#themes",
	N_("Select Themes Dir"),
	&edit_theme_path }
    };

    for (i = 0; i < G_N_ELEMENTS (paths); i++)
      {
	vbox = prefs_notebook_append_page (GTK_NOTEBOOK (notebook),
					   gettext (paths[i].label),
				           GTK_TREE_STORE (tree),
					   gettext (paths[i].tree_label),
					   paths[i].help_data,
					   &top_iter,
					   &child_iter,
					   page_index);
	gtk_widget_show (vbox);
	page_index++;

	patheditor = gimp_path_editor_new (gettext (paths[i].fs_label),
					   *(paths[i].mpath));
	gtk_container_add (GTK_CONTAINER (vbox), patheditor);
	gtk_widget_show (patheditor);

	g_signal_connect (G_OBJECT (patheditor), "path_changed",
			  G_CALLBACK (prefs_path_callback),
			  paths[i].mpath);
      }
  }

  gtk_widget_show (tv);
  gtk_widget_show (notebook);

  gtk_tree_view_expand_all (GTK_TREE_VIEW (tv));

  return prefs_dialog;
}
