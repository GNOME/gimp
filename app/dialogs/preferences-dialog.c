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

#include "widgets/gimpdeviceinfo.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimpenummenu.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell-render.h"

#include "tools/tool_manager.h"

#include "gui.h"
#include "resolution-calibrate-dialog.h"
#include "session.h"

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
static void  prefs_input_dialog_able_callback    (GtkWidget     *widget,
                                                  GdkDevice     *device,
                                                  gpointer       data);
static void  prefs_input_dialog_save_callback    (GtkWidget     *widget,
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
static GimpPreviewSize    old_preview_size;
static GimpPreviewSize    old_nav_preview_size;
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
static gchar            * old_image_status_format;
static guint              old_max_new_image_size;
static GimpThumbnailSize  old_thumbnail_size;
static gboolean	          old_trust_dirty_flag;
static gboolean           old_use_help;
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
      g_warning ("Error: Levels of undo must be zero or greater.");
      gimp->config->levels_of_undo = old_levels_of_undo;
      return PREFS_CORRUPT;
    }
  if (gimprc.marching_speed < 50)
    {
      g_warning ("Error: Marching speed must be 50 or greater.");
      gimprc.marching_speed = old_marching_speed;
      return PREFS_CORRUPT;
    }
  if (gimp->config->default_width < 1)
    {
      g_warning ("Error: Default width must be one or greater.");
      gimp->config->default_width = old_default_width;
      return PREFS_CORRUPT;
    }
  if (gimp->config->default_height < 1)
    {
      g_warning ("Error: Default height must be one or greater.");
      gimp->config->default_height = old_default_height;
      return PREFS_CORRUPT;
    }
  if (gimp->config->default_units < GIMP_UNIT_INCH ||
      gimp->config->default_units >= gimp_unit_get_number_of_units ())
    {
      g_warning ("Error: Default unit must be within unit range.");
      gimp->config->default_units = old_default_units;
      return PREFS_CORRUPT;
    }
  if (gimp->config->default_xresolution < GIMP_MIN_RESOLUTION ||
      gimp->config->default_yresolution < GIMP_MIN_RESOLUTION)
    {
      g_warning ("Error: Default resolution must not be zero.");
      gimp->config->default_xresolution = old_default_xresolution;
      gimp->config->default_yresolution = old_default_yresolution;
      return PREFS_CORRUPT;
    }
  if (gimp->config->default_resolution_units < GIMP_UNIT_INCH ||
      gimp->config->default_resolution_units >= gimp_unit_get_number_of_units ())
    {
      g_warning ("Error: Default resolution unit must be within unit range.");
      gimp->config->default_resolution_units = old_default_resolution_units;
      return PREFS_CORRUPT;
    }
  if (gimprc.monitor_xres < GIMP_MIN_RESOLUTION ||
      gimprc.monitor_yres < GIMP_MIN_RESOLUTION)
    {
      g_warning ("Error: Monitor resolution must not be zero.");
      gimprc.monitor_xres = old_monitor_xres;
      gimprc.monitor_yres = old_monitor_yres;
      return PREFS_CORRUPT;
    }
  if (gimprc.image_title_format == NULL)
    {
      g_warning ("Error: Image title format must not be NULL.");
      gimprc.image_title_format = old_image_title_format;
      return PREFS_CORRUPT;
    }
  if (gimprc.image_status_format == NULL)
    {
      g_warning ("Error: Image status format must not be NULL.");
      gimprc.image_status_format = old_image_status_format;
      return PREFS_CORRUPT;
    }

  if (base_config->num_processors < 1 || base_config->num_processors > 30) 
    {
      g_warning ("Error: Number of processors must be between 1 and 30.");
      base_config->num_processors = old_num_processors;
      return PREFS_CORRUPT;
    }

  /*  ...then check if we need a restart notification  */
  if (edit_stingy_memory_use         != old_stingy_memory_use         ||
      edit_min_colors                != old_min_colors                ||
      edit_install_cmap              != old_install_cmap              ||
      edit_cycled_marching_ants      != old_cycled_marching_ants      ||
      edit_last_opened_size          != old_last_opened_size          ||
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
  if (gimp->config->interpolation_type != old_interpolation_type)
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
  if (prefs_strcmp (gimprc.image_status_format, old_image_status_format))
    {
      update = g_list_append (update, "image-status-format");
    }
  if (gimprc.max_new_image_size != old_max_new_image_size)
    {
      update = g_list_append (update, "max-new-image-size");
    }
  if (gimp->config->thumbnail_size != old_thumbnail_size)
    {
      update = g_list_append (update, "thumbnail-size");
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
  base_config->num_processors            = old_num_processors;

  gimp->config->interpolation_type       = old_interpolation_type;
  gimp->config->default_type             = old_default_type;
  gimp->config->default_width            = old_default_width;
  gimp->config->default_height           = old_default_height;
  gimp->config->default_units            = old_default_units;
  gimp->config->default_xresolution      = old_default_xresolution;
  gimp->config->default_yresolution      = old_default_yresolution;
  gimp->config->default_resolution_units = old_default_resolution_units;
  gimp->config->levels_of_undo           = old_levels_of_undo;
  gimp->config->thumbnail_size           = old_thumbnail_size;

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
      gimprc.preview_size = old_preview_size;
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
#if 0
      gdisplays_nav_preview_resized ();
#endif
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
      gimp_displays_flush (gimp);
    }

  prefs_strset (&gimprc.image_title_format,     old_image_title_format);
  prefs_strset (&gimprc.image_status_format,    old_image_status_format);
  prefs_strset (&gimp->config->default_comment, old_default_comment);

  /*  restore values which need a restart  */
  edit_stingy_memory_use         = old_stingy_memory_use;
  edit_min_colors                = old_min_colors;
  edit_install_cmap              = old_install_cmap;
  edit_cycled_marching_ants      = old_cycled_marching_ants;
  edit_last_opened_size          = old_last_opened_size;
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
      data == &edit_info_window_follows_mouse  ||
      data == &edit_disable_tearoff_menus)
    {
      *val = GTK_TOGGLE_BUTTON (widget)->active;
    }

  /*  radio buttons  */
  else if (data == &gimp->config->interpolation_type ||
	   data == &gimp->config->default_type       ||
           data == &gimp->config->thumbnail_size     ||
           data == &gimprc.trust_dirty_flag          ||
	   data == &gimprc.help_browser              ||
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
      gimp_displays_flush (gimp);
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
  gimp_menu_item_update (widget, data);

#ifdef __GNUC__
#warning FIXME: update preview size
#endif
#if 0
  lc_dialog_rebuild (gimprc.preview_size);
#endif
}

static void
prefs_nav_preview_size_callback (GtkWidget *widget,
				 gpointer   data)
{
  gimp_menu_item_update (widget, data);

#if 0
  gdisplays_nav_preview_resized ();
#endif
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
  GtkWidget *dialog;
  GtkWidget *notebook;
  GtkWidget *image;

  dialog = gtk_widget_get_toplevel (GTK_WIDGET (data));

  notebook = g_object_get_data (G_OBJECT (dialog), "notebook");

  image = g_object_get_data (G_OBJECT (notebook), "image");

  resolution_calibrate_dialog (GTK_WIDGET (data),
                               gtk_image_get_pixbuf (GTK_IMAGE (image)),
                               NULL, NULL, NULL);
}

static void
prefs_input_dialog_able_callback (GtkWidget *widget,
                                  GdkDevice *device,
                                  gpointer   data)
{
  gimp_device_info_changed_by_device (device);
}

static void
prefs_input_dialog_save_callback (GtkWidget *widget,
                                  gpointer   data)
{
  gimp_devices_save (GIMP (data));
}

/*  create a new notebook page  */
static GtkWidget *
prefs_notebook_append_page (Gimp          *gimp,
                            GtkNotebook   *notebook,
			    gchar         *notebook_label,
                            const gchar   *notebook_icon,
			    GtkTreeStore  *tree,
			    gchar         *tree_label,
			    gchar         *help_data,
			    GtkTreeIter   *parent,
			    GtkTreeIter   *iter,
			    gint           page_index)
{
  GtkWidget   *event_box;
  GtkWidget   *vbox;
  GdkPixbuf   *pixbuf       = NULL;
  GdkPixbuf   *small_pixbuf = NULL;

  event_box = gtk_event_box_new ();
  gtk_notebook_append_page (notebook, event_box, NULL);
  gtk_widget_show (event_box);

  gimp_help_set_help_data (event_box, NULL, help_data);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (event_box), vbox);
  gtk_widget_show (vbox);

  if (notebook_icon)
    {
      gchar *filename;

      filename = g_build_filename (gui_themes_get_theme_dir (gimp),
                                   "images",
                                   "preferences",
                                   notebook_icon,
                                   NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
      else
        pixbuf = NULL;

      g_free (filename);

      if (pixbuf)
        {
          small_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                  18, 18,
                                                  GDK_INTERP_BILINEAR);
        }
    }

  gtk_tree_store_append (tree, iter, parent);
  gtk_tree_store_set (tree, iter,
                      0, small_pixbuf,
                      1, tree_label,
                      2, page_index,
                      3, notebook_label,
                      4, pixbuf,
                      -1);

  if (pixbuf)
    g_object_unref (pixbuf);

  if (small_pixbuf)
    g_object_unref (small_pixbuf);

  return vbox;
}

/*  select a notebook page  */
static void
prefs_tree_select_callback (GtkTreeSelection *sel,
			    GtkNotebook      *notebook)
{
  GtkWidget    *label;
  GtkWidget    *image;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GValue        val = { 0, };

  if (! gtk_tree_selection_get_selected (sel, &model, &iter))
    return;

  label = g_object_get_data (G_OBJECT (notebook), "label");
  image = g_object_get_data (G_OBJECT (notebook), "image");

  gtk_tree_model_get_value (model, &iter, 3, &val);

  gtk_label_set_text (GTK_LABEL (label),
                      g_value_get_string (&val));

  g_value_unset (&val);

  gtk_tree_model_get_value (model, &iter, 4, &val);

  gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                             g_value_get_object (&val));

  g_value_unset (&val);

  gtk_tree_model_get_value (model, &iter, 2, &val);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook),
				 g_value_get_int (&val));

  g_value_unset (&val);
}

/*  create a frame with title and a vbox  */
static GtkWidget *
prefs_frame_new (gchar        *label,
		 GtkContainer *parent)
{
  GtkWidget *frame;
  GtkWidget *vbox2;

  frame = gtk_frame_new (label);

  if (GTK_IS_BOX (parent))
    gtk_box_pack_start (GTK_BOX (parent), frame, FALSE, FALSE, 0);
  else
    gtk_container_add (parent, frame);

  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  return vbox2;
}

static GtkWidget *
prefs_table_new (gint          rows,
                 GtkContainer *parent,
                 gboolean      left_align)
{
  GtkWidget *table;

  if (left_align)
    {
      GtkWidget *hbox;

      hbox = gtk_hbox_new (FALSE, 0);

      if (GTK_IS_BOX (parent))
        gtk_box_pack_start (GTK_BOX (parent), hbox, FALSE, FALSE, 0);
      else
        gtk_container_add (parent, hbox);

      gtk_widget_show (hbox);

      parent = GTK_CONTAINER (hbox);
    }

  table = gtk_table_new (rows, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);

  if (rows > 1)
    gtk_table_set_row_spacings (GTK_TABLE (table), 2);

  gtk_table_set_col_spacings (GTK_TABLE (table), 4);

  if (GTK_IS_BOX (parent))
    gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
  else
    gtk_container_add (parent, table);

  gtk_widget_show (table);

  return table;
}

static GtkWidget *
prefs_check_button_new (const gchar *label,
                        gboolean    *value_location,
                        GtkBox      *vbox)
{
  GtkWidget *button;

  button = gtk_check_button_new_with_label (label);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                *value_location);
  gtk_box_pack_start (vbox, button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (prefs_toggle_callback),
		    value_location);

  return button;
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
  GtkWidget         *tv;
  GtkTreeStore      *tree;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *cell;
  GtkTreeSelection  *sel;
  GtkTreeIter        top_iter;
  GtkTreeIter        child_iter;
  gint               page_index;

  GtkWidget         *frame;
  GtkWidget         *notebook;
  GtkWidget         *vbox;
  GtkWidget         *vbox2;
  GtkWidget         *hbox;
  GtkWidget         *abox;
  GtkWidget         *button;
  GtkWidget         *fileselection;
  GtkWidget         *patheditor;
  GtkWidget         *spinbutton;
  GtkWidget         *optionmenu;
  GtkWidget         *table;
  GtkWidget         *label;
  GtkWidget         *image;
  GtkObject         *adjustment;
  GtkWidget         *sizeentry;
  GtkWidget         *sizeentry2;
  GtkWidget         *separator;
  GtkWidget         *calibrate_button;
  GtkWidget         *scrolled_window;
  GtkWidget         *text_view;
  GtkTextBuffer     *text_buffer;
  PangoAttrList     *attrs;
  PangoAttribute    *attr;
  GSList            *group;

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
  old_num_processors           = base_config->num_processors;

  old_interpolation_type       = gimp->config->interpolation_type;
  old_default_type             = gimp->config->default_type;
  old_default_width            = gimp->config->default_width;
  old_default_height           = gimp->config->default_height;
  old_default_units            = gimp->config->default_units;
  old_default_xresolution      = gimp->config->default_xresolution;
  old_default_yresolution      = gimp->config->default_yresolution;
  old_default_resolution_units = gimp->config->default_resolution_units;
  old_levels_of_undo           = gimp->config->levels_of_undo;
  old_thumbnail_size           = gimp->config->thumbnail_size;

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
  old_max_new_image_size       = gimprc.max_new_image_size;
  old_trust_dirty_flag         = gimprc.trust_dirty_flag;
  old_use_help                 = gimprc.use_help;
  old_help_browser             = gimprc.help_browser;
  old_cursor_mode              = gimprc.cursor_mode;
  old_default_threshold        = gimprc.default_threshold;

  prefs_strset (&old_image_title_format,  gimprc.image_title_format);	
  prefs_strset (&old_image_status_format, gimprc.image_status_format);	
  prefs_strset (&old_default_comment,     gimp->config->default_comment);	

  /*  values which will need a restart  */
  old_stingy_memory_use         = edit_stingy_memory_use;
  old_min_colors                = edit_min_colors;
  old_install_cmap              = edit_install_cmap;
  old_cycled_marching_ants      = edit_cycled_marching_ants;
  old_last_opened_size          = edit_last_opened_size;
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

  tree = gtk_tree_store_new (5,
                             GDK_TYPE_PIXBUF, G_TYPE_STRING,
                             G_TYPE_INT, G_TYPE_STRING, GDK_TYPE_PIXBUF);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (tree));
  g_object_unref (G_OBJECT (tree));

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);

  column = gtk_tree_view_column_new ();

  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, cell, FALSE);
  gtk_tree_view_column_set_attributes (column, cell, "pixbuf", 0, NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell, "text", 1, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

  gtk_container_add (GTK_CONTAINER (frame), tv);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  attrs = pango_attr_list_new ();
  attr = pango_attr_scale_new (PANGO_SCALE_X_LARGE);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);
  gtk_label_set_attributes (GTK_LABEL (label), attrs);
  pango_attr_list_unref (attrs);

  image = gtk_image_new ();
  gtk_box_pack_end (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  /* The main preferences notebook */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (prefs_dialog), "notebook", notebook);

  g_object_set_data (G_OBJECT (notebook), "label", label);
  g_object_set_data (G_OBJECT (notebook), "image", image);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
  g_signal_connect (G_OBJECT (sel), "changed",
		    G_CALLBACK (prefs_tree_select_callback),
		    notebook);

  page_index = 0;

  /***************/
  /*  New Image  */
  /***************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("New Image"),
                                     "new-image.png",
				     GTK_TREE_STORE (tree),
				     _("New Image"),
				     "dialogs/preferences/new_file.html",
				     NULL,
				     &top_iter,
				     page_index++);

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

  table = prefs_table_new (2, GTK_CONTAINER (vbox), TRUE);

  optionmenu =
    gimp_enum_option_menu_new_with_range (GIMP_TYPE_IMAGE_BASE_TYPE,
                                          GIMP_RGB, GIMP_GRAY,
                                          G_CALLBACK (prefs_toggle_callback),
                                          &gimp->config->default_type);
  gimp_option_menu_set_history (GTK_OPTION_MENU (optionmenu),
                                GINT_TO_POINTER (gimp->config->default_type));

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Default Image Type:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  /*  The maximum size of a new image  */  
  adjustment = gtk_adjustment_new (gimprc.max_new_image_size, 
				   0, G_MAXULONG, 
				   1.0, 1.0, 0.0);
  hbox = gimp_memsize_entry_new (GTK_ADJUSTMENT (adjustment));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Maximum Image Size:"), 1.0, 0.5,
			     hbox, 1, TRUE);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_uint_adjustment_update),
		    &gimprc.max_new_image_size);


  /*********************************/
  /*  New Image / Default Comment  */
  /*********************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Default Comment"),
                                     "default-comment.png",
				     GTK_TREE_STORE (tree),
				     _("Default Comment"),
				     "dialogs/preferences/new_file.html#default_comment",
				     &top_iter,
				     &child_iter,
				     page_index++);

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


  /***************/
  /*  Interface  */
  /***************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Interface"),
                                     "interface.png",
				     GTK_TREE_STORE (tree),
				     _("Interface"),
				     "dialogs/preferences/interface.html",
				     NULL,
				     &top_iter,
				     page_index++);

  vbox2 = prefs_frame_new (_("General"), GTK_CONTAINER (vbox));

  table = prefs_table_new (4, GTK_CONTAINER (vbox2), FALSE);

  optionmenu = 
    gimp_enum_option_menu_new (GIMP_TYPE_PREVIEW_SIZE,
                               G_CALLBACK (prefs_preview_size_callback),
                               &gimprc.preview_size);
  gimp_option_menu_set_history (GTK_OPTION_MENU (optionmenu),
                                GINT_TO_POINTER (gimprc.preview_size));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Preview Size:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  optionmenu =
    gimp_option_menu_new2 (FALSE,
			   G_CALLBACK (prefs_nav_preview_size_callback),
			   &gimprc.nav_preview_size,
			   GINT_TO_POINTER (gimprc.nav_preview_size),

			   _("Small"),
                           GINT_TO_POINTER (GIMP_PREVIEW_SIZE_MEDIUM), NULL,

			   _("Medium"),
                           GINT_TO_POINTER (GIMP_PREVIEW_SIZE_EXTRA_LARGE), NULL,

			   _("Large"),
                           GINT_TO_POINTER (GIMP_PREVIEW_SIZE_HUGE), NULL,

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

  /* Dialog Bahaviour */
  vbox2 = prefs_frame_new (_("Dialog Behavior"), GTK_CONTAINER (vbox));

  prefs_check_button_new (_("Info Window Follows Mouse"),
                          &edit_info_window_follows_mouse, GTK_BOX (vbox2));

  /* Menus */
  vbox2 = prefs_frame_new (_("Menus"), GTK_CONTAINER (vbox));

  prefs_check_button_new (_("Disable Tearoff Menus"),
                          &edit_disable_tearoff_menus, GTK_BOX (vbox2));

  /* Window Positions */
  vbox2 = prefs_frame_new (_("Window Positions"), GTK_CONTAINER (vbox));

  prefs_check_button_new (_("Save Window Positions on Exit"),
                          &gimprc.save_session_info, GTK_BOX (vbox2));
  prefs_check_button_new (_("Restore Saved Window Positions on Start-up"),
                          &gimprc.always_restore_session, GTK_BOX (vbox2));

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


  /*****************************/
  /*  Interface / Help System  */
  /*****************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Help System"),
                                     "help-system.png",
				     GTK_TREE_STORE (tree),
				     _("Help System"),
				     "dialogs/preferences/interface.html#help_system",
				     &top_iter,
				     &child_iter,
				     page_index++);

  vbox2 = prefs_frame_new (_("General"), GTK_CONTAINER (vbox));

  prefs_check_button_new (_("Show Tool Tips"),
                          &gimprc.show_tool_tips, GTK_BOX (vbox2));
  prefs_check_button_new (_("Context Sensitive Help with \"F1\""),
                          &gimprc.use_help, GTK_BOX (vbox2));

  vbox2 = prefs_frame_new (_("Help Browser"), GTK_CONTAINER (vbox));

  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

  optionmenu = gimp_enum_option_menu_new (GIMP_TYPE_HELP_BROWSER_TYPE,
                                          G_CALLBACK (prefs_toggle_callback),
                                          &gimprc.help_browser);
  gimp_option_menu_set_history (GTK_OPTION_MENU (optionmenu),
                                GINT_TO_POINTER (gimprc.help_browser));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Help Browser to Use:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);


  /******************************/
  /*  Interface / Tool Options  */
  /******************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Tool Options"),
                                     "tool-options.png",
				     GTK_TREE_STORE (tree),
				     _("Tool Options"),
				     "dialogs/preferences/interface.html#tool_options",
				     &top_iter,
				     &child_iter,
				     page_index++);

  vbox2 = prefs_frame_new (_("Finding Contiguous Regions"), GTK_CONTAINER (vbox));

  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

  /*  Default threshold  */
  spinbutton = gimp_spin_button_new (&adjustment, gimprc.default_threshold,
				     0.0, 255.0, 1.0, 5.0, 0.0, 1.0, 0.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Default Threshold:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &gimprc.default_threshold);


  frame = gtk_frame_new (_("Scaling")); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = prefs_table_new (1, GTK_CONTAINER (frame), TRUE);

  optionmenu = gimp_enum_option_menu_new (GIMP_TYPE_INTERPOLATION_TYPE,
                                          G_CALLBACK (prefs_toggle_callback),
                                          &gimp->config->interpolation_type);
  gimp_option_menu_set_history (GTK_OPTION_MENU (optionmenu),
                                GINT_TO_POINTER (gimp->config->interpolation_type));

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Default Interpolation:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);


  /*******************************/
  /*  Interface / Input Devices  */
  /*******************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Input Devices"),
                                     "input-devices.png",
				     GTK_TREE_STORE (tree),
				     _("Input Devices"),
				     "dialogs/preferences/input-devices.html",
				     &top_iter,
				     &child_iter,
				     page_index++);

  vbox2 = prefs_frame_new (_("Input Device Settings"), GTK_CONTAINER (vbox));

  {
    GtkWidget *input_dialog;
    GtkWidget *input_vbox;
    GList     *input_children;

    input_dialog = gtk_input_dialog_new ();

    input_children = gtk_container_get_children (GTK_CONTAINER (GTK_DIALOG (input_dialog)->vbox));

    input_vbox = GTK_WIDGET (input_children->data);

    g_list_free (input_children);

    g_object_ref (G_OBJECT (input_vbox));

    gtk_container_remove (GTK_CONTAINER (GTK_DIALOG (input_dialog)->vbox),
                          input_vbox);
    gtk_box_pack_start (GTK_BOX (vbox2), input_vbox, TRUE, TRUE, 0);

    g_object_unref (G_OBJECT (input_vbox));

    g_object_weak_ref (G_OBJECT (input_vbox),
                       (GWeakNotify) gtk_widget_destroy,
                       input_dialog);

    g_signal_connect (G_OBJECT (input_dialog), "enable_device",
                      G_CALLBACK (prefs_input_dialog_able_callback),
                      gimp);
    g_signal_connect (G_OBJECT (input_dialog), "disable_device",
                      G_CALLBACK (prefs_input_dialog_able_callback),
                      gimp);
  }

  prefs_check_button_new (_("Save Input Device Settings on Exit"),
                          &gimprc.save_device_status, GTK_BOX (vbox));

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
      
  button = gtk_button_new_with_label (_("Save Input Device Settings Now"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (prefs_input_dialog_save_callback),
                    gimp);


  /*******************************/
  /*  Interface / Image Windows  */
  /*******************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Image Windows"),
                                     "image-windows.png",
				     GTK_TREE_STORE (tree),
				     _("Image Windows"),
				     "dialogs/preferences/interface.html#image_windows",
				     &top_iter,
				     &child_iter,
				     page_index++);

  vbox2 = prefs_frame_new (_("Appearance"), GTK_CONTAINER (vbox));

  prefs_check_button_new (_("Use \"Dot for Dot\" by default"),
                          &gimprc.default_dot_for_dot, GTK_BOX (vbox2));
  prefs_check_button_new (_("Resize Window on Zoom"),
                          &gimprc.resize_windows_on_zoom, GTK_BOX (vbox2));
  prefs_check_button_new (_("Resize Window on Image Size Change"),
                          &gimprc.resize_windows_on_resize, GTK_BOX (vbox2));
  prefs_check_button_new (_("Show Rulers"),
                          &gimprc.show_rulers, GTK_BOX (vbox2));
  prefs_check_button_new (_("Show Statusbar"),
                          &gimprc.show_statusbar, GTK_BOX (vbox2));

  table = prefs_table_new (3, GTK_CONTAINER (vbox2), FALSE);

  spinbutton = gimp_spin_button_new (&adjustment, gimprc.marching_speed,
				     50.0, 32000.0, 10.0, 100.0, 1.0, 1.0, 0.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Marching Ants Speed:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &gimprc.marching_speed);

  /* The title and status format strings */
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

    format_strings[0] = gimprc.image_status_format;

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

    gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                               _("Image Status Format:"), 1.0, 0.5,
                               combo, 1, FALSE);

    g_signal_connect (G_OBJECT (GTK_COMBO (combo)->entry), "changed",
                      G_CALLBACK (prefs_string_callback), 
                      &gimprc.image_status_format);
  }

  vbox2 = prefs_frame_new (_("Pointer Movement Feedback"), GTK_CONTAINER (vbox));

  prefs_check_button_new (_("Perfect-but-Slow Pointer Tracking"),
                          &gimprc.perfectmouse, GTK_BOX (vbox2));
  prefs_check_button_new (_("Disable Cursor Updating"),
                          &gimprc.no_cursor_updating, GTK_BOX (vbox2));

  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

  optionmenu = gimp_enum_option_menu_new (GIMP_TYPE_CURSOR_MODE,
                                          G_CALLBACK (prefs_toggle_callback),
                                          &gimprc.cursor_mode);
  gimp_option_menu_set_history (GTK_OPTION_MENU (optionmenu),
                                GINT_TO_POINTER (gimprc.cursor_mode));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Cursor Mode:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);


  /*************************/
  /*  Interface / Display  */
  /*************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Display"),
                                     "display.png",
				     GTK_TREE_STORE (tree),
				     _("Display"),
				     "dialogs/preferences/display.html",
				     &top_iter,
				     &child_iter,
				     page_index++);

  frame = gtk_frame_new (_("Transparency")); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = prefs_table_new (2, GTK_CONTAINER (frame), TRUE);

  optionmenu = gimp_enum_option_menu_new (GIMP_TYPE_CHECK_TYPE,
                                          G_CALLBACK (prefs_toggle_callback),
                                          &gimprc.transparency_type);
  gimp_option_menu_set_history (GTK_OPTION_MENU (optionmenu),
                                GINT_TO_POINTER (gimprc.transparency_type));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Transparency Type:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  optionmenu = gimp_enum_option_menu_new (GIMP_TYPE_CHECK_SIZE,
                                          G_CALLBACK (prefs_toggle_callback),
                                          &gimprc.transparency_size);
  gimp_option_menu_set_history (GTK_OPTION_MENU (optionmenu),
                                GINT_TO_POINTER (gimprc.transparency_size));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Check Size:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);

  vbox2 = prefs_frame_new (_("8-Bit Displays"), GTK_CONTAINER (vbox));

  if (gdk_rgb_get_visual ()->depth != 8)
    gtk_widget_set_sensitive (GTK_WIDGET (vbox2->parent), FALSE);

  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

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

  prefs_check_button_new (_("Install Colormap"),
                          &edit_install_cmap, GTK_BOX (vbox2));
  prefs_check_button_new (_("Colormap Cycling"),
                          &edit_cycled_marching_ants, GTK_BOX (vbox2));


  /*************************/
  /*  Interface / Monitor  */
  /*************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Monitor"),
                                     "monitor.png",
				     GTK_TREE_STORE (tree),
				     _("Monitor"),
				     "dialogs/preferences/monitor.html",
				     &top_iter,
				     &child_iter,
				     page_index++);

  vbox2 = prefs_frame_new (_("Get Monitor Resolution"), GTK_CONTAINER (vbox));

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
  button = gtk_radio_button_new_with_label (group, _("From Windowing System"));
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


  /*****************/
  /*  Environment  */
  /*****************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Environment"),
                                     "environment.png",
				     GTK_TREE_STORE (tree),
				     _("Environment"),
				     "dialogs/preferences/environment.html",
				     NULL,
				     &top_iter,
				     page_index++);

  vbox2 = prefs_frame_new (_("Resource Consumption"), GTK_CONTAINER (vbox));

  prefs_check_button_new (_("Conservative Memory Usage"),
                          &edit_stingy_memory_use, GTK_BOX (vbox2));

#ifdef ENABLE_MP
  table = prefs_table_new (3, GTK_CONTAINER (vbox2), FALSE);
#else
  table = prefs_table_new (2, GTK_CONTAINER (vbox2), FALSE);
#endif /* ENABLE_MP */

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
				   0, G_MAXULONG, 
				   1.0, 1.0, 0.0);
  hbox = gimp_memsize_entry_new (GTK_ADJUSTMENT (adjustment));
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

  vbox2 = prefs_frame_new (_("File Saving"), GTK_CONTAINER (vbox));

  table = prefs_table_new (2, GTK_CONTAINER (vbox2), TRUE);

#if 0
  /*  Don't show the Auto-save button until we really 
   *  have auto-saving in the gimp.
   */
  prefs_check_button (_("Auto Save"),
                      &auto_save,
                      vbox2);
#endif

  optionmenu =
    gimp_option_menu_new2 (FALSE,
			   G_CALLBACK (prefs_toggle_callback),
			   &gimprc.trust_dirty_flag,
			   GINT_TO_POINTER (gimprc.trust_dirty_flag),

			   _("Only when Modified"), GINT_TO_POINTER (TRUE),  NULL,
			   _("Always"),             GINT_TO_POINTER (FALSE), NULL,

			   NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("\"File -> Save\" Saves the Image:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);
                                     
  optionmenu = 
    gimp_enum_option_menu_new (GIMP_TYPE_THUMBNAIL_SIZE,
                               G_CALLBACK (prefs_toggle_callback),
                               &gimp->config->thumbnail_size);
  gimp_option_menu_set_history (GTK_OPTION_MENU (optionmenu),
                                GINT_TO_POINTER (gimp->config->thumbnail_size));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Size of Thumbnails Files:"), 1.0, 0.5,
			     optionmenu, 1, TRUE);


  /*************/
  /*  Folders  */
  /*************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Folders"),
                                     "folders.png",
				     GTK_TREE_STORE (tree),
				     _("Folders"),
				     "dialogs/preferences/folders.html",
				     NULL,
				     &top_iter,
				     page_index++);

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

    table = prefs_table_new (G_N_ELEMENTS (dirs) + 1,
                             GTK_CONTAINER (vbox), FALSE);

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

  /* Folders / <paths> */
  {
    static const struct
    {
      gchar  *tree_label;
      gchar  *label;
      gchar  *icon;
      gchar  *help_data;
      gchar  *fs_label;
      gchar **mpath;
    }
    paths[] =
    {
      { N_("Brushes"), N_("Brush Folders"), "folders-brushes.png",
	"dialogs/preferences/folders.html#brushes",
	N_("Select Brush Folders"),
	&edit_brush_path },
      { N_("Patterns"), N_("Pattern Folders"), "folders-patterns.png",
	"dialogs/preferences/folders.html#patterns",
	N_("Select Pattern Folders"),
	&edit_pattern_path },
      { N_("Palettes"), N_("Palette Folders"), "folders-palettes.png",
	"dialogs/preferences/folders.html#palettes",
	N_("Select Palette Folders"),
	&edit_palette_path },
      { N_("Gradients"), N_("Gradient Folders"), "folders-gradients.png",
	"dialogs/preferences/folders.html#gradients",
	N_("Select Gradient Folders"),
	&edit_gradient_path },
      { N_("Plug-Ins"), N_("Plug-In Folders"), "folders-plug-ins.png",
	"dialogs/preferences/folders.html#plug_ins",
	N_("Select Plug-In Folders"),
	&edit_plug_in_path },
      { N_("Tool Plug-Ins"), N_("Tool Plug-In Folders"), "folders-tool-plug-ins.png",
	"dialogs/preferences/folders.html#tool_plug_ins",
	N_("Select Tool Plug-In Folders"),
	&edit_tool_plug_in_path },
      { N_("Modules"), N_("Module Folders"), "folders-modules.png",
	"dialogs/preferences/folders.html#modules",
	N_("Select Module Folders"),
	&edit_module_path },
      { N_("Themes"), N_("Theme Folders"), "folders-themes.png",
	"dialogs/preferences/folders.html#themes",
	N_("Select Theme Folders"),
	&edit_theme_path }
    };

    for (i = 0; i < G_N_ELEMENTS (paths); i++)
      {
	vbox = prefs_notebook_append_page (gimp,
                                           GTK_NOTEBOOK (notebook),
					   gettext (paths[i].label),
                                           paths[i].icon,
				           GTK_TREE_STORE (tree),
					   gettext (paths[i].tree_label),
					   paths[i].help_data,
					   &top_iter,
					   &child_iter,
					   page_index++);

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
