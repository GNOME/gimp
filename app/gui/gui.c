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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"

#include "gui-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpenvirontable.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-render.h"

#include "tools/gimp-tools.h"

#include "widgets/gimpdevices.h"
#include "widgets/gimpdevicestatus.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdnd.h"
#include "widgets/gimphelp.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimpwidgets-utils.h"

#include "color-history.h"
#include "dialogs.h"
#include "dialogs-commands.h"
#include "gui.h"
#include "gui-vtable.h"
#include "menus.h"
#include "session.h"
#include "splash.h"
#include "themes.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void       gui_help_func                 (const gchar        *help_id,
                                                 gpointer            help_data);
static gboolean   gui_get_background_func       (GimpRGB            *color);
static gboolean   gui_get_foreground_func       (GimpRGB            *color);

static void       gui_initialize_after_callback (Gimp               *gimp,
                                                 GimpInitStatusFunc  callback);

static void       gui_restore_callback          (Gimp               *gimp,
                                                 GimpInitStatusFunc  callback);
static void       gui_restore_after_callback    (Gimp               *gimp,
                                                 GimpInitStatusFunc  callback);

static gboolean   gui_exit_callback             (Gimp               *gimp,
                                                 gboolean            force);
static gboolean   gui_exit_after_callback       (Gimp               *gimp,
                                                 gboolean            force);
static void       gui_really_quit_callback      (GtkWidget          *button,
                                                 gboolean            quit,
                                                 gpointer            data);

static void       gui_show_tooltips_notify      (GObject            *config,
                                                 GParamSpec         *param_spec,
                                                 Gimp               *gimp);
static void       gui_device_change_notify      (Gimp               *gimp);

static void       gui_display_changed           (GimpContext        *context,
                                                 GimpDisplay        *display,
                                                 Gimp               *gimp);
static void       gui_image_disconnect          (GimpImage          *gimage,
                                                 Gimp               *gimp);


/*  private variables  */

static Gimp            *the_gui_gimp                = NULL;

static GQuark           image_disconnect_handler_id = 0;

static GimpItemFactory *toolbox_item_factory        = NULL;
static GimpItemFactory *image_item_factory          = NULL;


/*  public functions  */

gboolean
gui_libs_init (gint    *argc,
	       gchar ***argv)
{
  g_return_val_if_fail (argc != NULL, FALSE);
  g_return_val_if_fail (argv != NULL, FALSE);

  if (! gtk_init_check (argc, argv))
    return FALSE;

  gimp_widgets_init (gui_help_func,
                     gui_get_foreground_func,
                     gui_get_background_func,
                     NULL);

  g_type_class_ref (GIMP_TYPE_COLOR_SELECT);

  return TRUE;
}

void
gui_abort (const gchar *abort_message)
{
  g_return_if_fail (abort_message != NULL);

  gimp_message_box (GIMP_STOCK_WILBER_EEK, NULL, abort_message,
                    (GtkCallback) gtk_main_quit, NULL);
  gtk_main ();
}

GimpInitStatusFunc
gui_init (Gimp     *gimp,
          gboolean  no_splash)
{
  GimpInitStatusFunc  status_callback = NULL;
  GdkScreen          *screen;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (the_gui_gimp == NULL, NULL);

  the_gui_gimp = gimp;

  gimp_dnd_init (gimp);

  themes_init (gimp);

  gdk_rgb_set_min_colors (CLAMP (gimp->config->min_colors, 27, 256));
  gdk_rgb_set_install (gimp->config->install_cmap);

  screen = gdk_screen_get_default ();
  gtk_widget_set_default_colormap (gdk_screen_get_rgb_colormap (screen));

  if (! no_splash)
    {
      splash_create ();
      status_callback = splash_update;
    }

  g_signal_connect_after (gimp, "initialize",
                          G_CALLBACK (gui_initialize_after_callback),
                          NULL);

  g_signal_connect (gimp, "restore",
                    G_CALLBACK (gui_restore_callback),
                    NULL);
  g_signal_connect_after (gimp, "restore",
                          G_CALLBACK (gui_restore_after_callback),
                          NULL);

  g_signal_connect (gimp, "exit",
                    G_CALLBACK (gui_exit_callback),
                    NULL);
  g_signal_connect_after (gimp, "exit",
                          G_CALLBACK (gui_exit_after_callback),
                          NULL);

  return status_callback;
}

void
gui_post_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (GIMP_GUI_CONFIG (gimp->config)->show_tips)
    gimp_dialog_factory_dialog_new (global_dialog_factory,
                                    gdk_screen_get_default (),
                                    "gimp-tips-dialog", -1);
}


/*  private functions  */

static void
gui_help_func (const gchar *help_id,
               gpointer     help_data)
{
  g_return_if_fail (GIMP_IS_GIMP (the_gui_gimp));

  gimp_help (the_gui_gimp, NULL, help_id);
}

static gboolean
gui_get_foreground_func (GimpRGB *color)
{
  g_return_val_if_fail (color != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_GIMP (the_gui_gimp), FALSE);

  gimp_context_get_foreground (gimp_get_user_context (the_gui_gimp), color);

  return TRUE;
}

static gboolean
gui_get_background_func (GimpRGB *color)
{
  g_return_val_if_fail (color != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_GIMP (the_gui_gimp), FALSE);

  gimp_context_get_background (gimp_get_user_context (the_gui_gimp), color);

  return TRUE;
}

static void
gui_initialize_after_callback (Gimp               *gimp,
                               GimpInitStatusFunc  status_callback)
{
  const gchar *name = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->be_verbose)
    g_print ("INIT: gui_initialize_after_callback\n");

#if defined (GDK_WINDOWING_X11)
  name = "DISPLAY";
#elif defined (GDK_WINDOWING_DIRECTFB) || defined (GDK_WINDOWING_FB)
  name = "GDK_DISPLAY";
#endif

  /* TODO: Need to care about display migration with GTK+ 2.2 at some point */

  if (name)
    {
      gchar *display;

      display = gdk_get_display ();
      gimp_environ_table_add (gimp->environ_table, name, display, NULL);
      g_free (display);
    }

  gimp_tools_init (gimp);
}

static void
gui_restore_callback (Gimp               *gimp,
                      GimpInitStatusFunc  status_callback)
{
  GimpDisplayConfig *display_config = GIMP_DISPLAY_CONFIG (gimp->config);
  GimpGuiConfig     *gui_config     = GIMP_GUI_CONFIG (gimp->config);

  if (gimp->be_verbose)
    g_print ("INIT: gui_restore_callback\n");

  gui_vtable_init (gimp);

  image_disconnect_handler_id =
    gimp_container_add_handler (gimp->images, "disconnect",
				G_CALLBACK (gui_image_disconnect),
				gimp);

  if (! gui_config->show_tool_tips)
    gimp_help_disable_tooltips ();

  g_signal_connect (gui_config, "notify::show-tool-tips",
                    G_CALLBACK (gui_show_tooltips_notify),
                    gimp);

  g_signal_connect (gimp_get_user_context (gimp), "display_changed",
		    G_CALLBACK (gui_display_changed),
		    gimp);

  /* make sure the monitor resolution is valid */
  if (display_config->monitor_res_from_gdk               ||
      display_config->monitor_xres < GIMP_MIN_RESOLUTION ||
      display_config->monitor_yres < GIMP_MIN_RESOLUTION)
    {
      gdouble xres, yres;

      gimp_get_screen_resolution (NULL, &xres, &yres);

      g_object_set (gimp->config,
		    "monitor-xresolution",                      xres,
		    "monitor-yresolution",                      yres,
		    "monitor-resolution-from-windowing-system", TRUE,
		    NULL);
    }

  menus_init (gimp);
  render_init (gimp);

  dialogs_init (gimp);

  gimp_devices_init (gimp, gui_device_change_notify);
  session_init (gimp);

  (* status_callback) (NULL, _("Tool Options"), 1.0);
  gimp_tools_restore (gimp);
}

static void
gui_restore_after_callback (Gimp               *gimp,
                            GimpInitStatusFunc  status_callback)
{
  GimpGuiConfig *gui_config = GIMP_GUI_CONFIG (gimp->config);

  if (gimp->be_verbose)
    g_print ("INIT: gui_restore_after_callback\n");

  gimp->message_handler = GIMP_MESSAGE_BOX;

  if (gui_config->restore_accels)
    menus_restore (gimp);

  toolbox_item_factory = gimp_menu_factory_menu_new (global_menu_factory,
                                                     "<Toolbox>",
                                                     GTK_TYPE_MENU_BAR,
                                                     gimp,
                                                     TRUE);

  image_item_factory = gimp_menu_factory_menu_new (global_menu_factory,
                                                   "<Image>",
                                                   GTK_TYPE_MENU,
                                                   gimp,
                                                   TRUE);

  gimp_devices_restore (gimp);

  if (status_callback == splash_update)
    splash_destroy ();

  color_history_restore ();

  if (gui_config->restore_session)
    session_restore (gimp);

  dialogs_show_toolbox ();
}

static gboolean
gui_exit_callback (Gimp     *gimp,
                   gboolean  force)
{
  GimpGuiConfig  *gui_config = GIMP_GUI_CONFIG (gimp->config);

  if (gimp->be_verbose)
    g_print ("EXIT: gui_exit_callback\n");

  if (! force && gimp_displays_dirty (gimp))
    {
      GtkWidget *dialog;

      gimp_item_factories_set_sensitive ("<Toolbox>", "/File/Quit", FALSE);
      gimp_item_factories_set_sensitive ("<Image>",   "/File/Quit", FALSE);

      dialog = gimp_query_boolean_box (_("Quit The GIMP?"),
                                       NULL,
                                       gimp_standard_help_func,
                                       GIMP_HELP_FILE_QUIT_CONFIRM,
                                       GIMP_STOCK_WILBER_EEK,
                                       _("Some files are unsaved.\n\n"
                                         "Really quit The GIMP?"),
                                       GTK_STOCK_QUIT, GTK_STOCK_CANCEL,
                                       NULL, NULL,
                                       gui_really_quit_callback,
                                       gimp);

      gtk_widget_show (dialog);

      return TRUE; /* stop exit for now */
    }

  gimp->message_handler = GIMP_CONSOLE;

  if (gui_config->save_session_info)
    session_save (gimp);

  color_history_save ();

  if (gui_config->save_accels)
    menus_save (gimp);

  if (gui_config->save_device_status)
    gimp_devices_save (gimp);

  gimp_displays_delete (gimp);

  gimp_tools_save (gimp);
  gimp_tools_exit (gimp);

  return FALSE; /* continue exiting */
}

static gboolean
gui_exit_after_callback (Gimp     *gimp,
                         gboolean  force)
{
  if (gimp->be_verbose)
    g_print ("EXIT: gui_exit_after_callback\n");

  g_signal_handlers_disconnect_by_func (gimp->config,
                                        gui_show_tooltips_notify,
                                        gimp);

  gimp_container_remove_handler (gimp->images, image_disconnect_handler_id);
  image_disconnect_handler_id = 0;

  g_object_unref (toolbox_item_factory);
  toolbox_item_factory = NULL;

  g_object_unref (image_item_factory);
  image_item_factory = NULL;

  menus_exit (gimp);
  render_exit (gimp);

  dialogs_exit (gimp);
  gimp_devices_exit (gimp);

  themes_exit (gimp);

  g_type_class_unref (g_type_class_peek (GIMP_TYPE_COLOR_SELECT));

  return FALSE; /* continue exiting */
}

static void
gui_really_quit_callback (GtkWidget *button,
			  gboolean   quit,
			  gpointer   data)
{
  Gimp *gimp = GIMP (data);

  if (quit)
    {
      gimp_exit (gimp, TRUE);
    }
  else
    {
      gimp_item_factories_set_sensitive ("<Toolbox>", "/File/Quit", TRUE);
      gimp_item_factories_set_sensitive ("<Image>",   "/File/Quit", TRUE);
    }
}

static void
gui_show_tooltips_notify (GObject    *config,
                          GParamSpec *param_spec,
                          Gimp       *gimp)
{
  gboolean show_tool_tips;

  g_object_get (config,
                "show-tool-tips", &show_tool_tips,
                NULL);

  if (show_tool_tips)
    gimp_help_enable_tooltips ();
  else
    gimp_help_disable_tooltips ();
}

static void
gui_device_change_notify (Gimp *gimp)
{
  GimpSessionInfo *session_info;

  session_info = gimp_dialog_factory_find_session_info (global_dock_factory,
                                                        "gimp-device-status");

  if (session_info && session_info->widget)
    {
      GtkWidget *device_status;

      device_status = GTK_BIN (session_info->widget)->child;

      gimp_device_status_update (GIMP_DEVICE_STATUS (device_status));
    }
}


#ifdef __GNUC__
#warning FIXME: this junk should mostly go to the display subsystem
#endif

static void
gui_display_changed (GimpContext *context,
		     GimpDisplay *display,
		     Gimp        *gimp)
{
  gimp_item_factory_update (gimp_item_factory_from_path ("<Image>"), display);
}

static void
gui_image_disconnect (GimpImage *gimage,
		      Gimp      *gimp)
{
  /*  check if this is the last image and if it had a display  */
  if (gimp_container_num_children (gimp->images) == 1 &&
      gimage->instance_count                      > 0)
    {
      dialogs_show_toolbox ();
    }
}
