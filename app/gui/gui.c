/* GIMP - The GNU Image Manipulation Program
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

#if HAVE_DBUS_GLIB
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"

#include "gui-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "plug-in/gimpenvirontable.h"
#include "plug-in/gimppluginmanager.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-render.h"
#include "display/gimpstatusbar.h"

#include "tools/gimp-tools.h"

#include "widgets/gimpclipboard.h"
#include "widgets/gimpcolorselectorpalette.h"
#include "widgets/gimpcontrollers.h"
#include "widgets/gimpdbusservice.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimpdevicestatus.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdnd.h"
#include "widgets/gimprender.h"
#include "widgets/gimphelp.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "actions/actions.h"
#include "actions/dialogs-commands.h"

#include "menus/menus.h"

#include "dialogs/dialogs.h"

#include "color-history.h"
#include "gui.h"
#include "gui-vtable.h"
#include "session.h"
#include "splash.h"
#include "themes.h"
#ifdef GDK_WINDOWING_QUARTZ
#include "gtk-macmenu.h"
#endif /* GDK_WINDOWING_QUARTZ */

#include "gimp-intl.h"


/*  local function prototypes  */

static gchar    * gui_sanity_check              (void);
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

static void       gui_show_tooltips_notify      (GimpGuiConfig      *gui_config,
                                                 GParamSpec         *pspec,
                                                 Gimp               *gimp);
static void       gui_show_help_button_notify   (GimpGuiConfig      *gui_config,
                                                 GParamSpec         *pspec,
                                                 Gimp               *gimp);
static void       gui_tearoff_menus_notify      (GimpGuiConfig      *gui_config,
                                                 GParamSpec         *pspec,
                                                 GtkUIManager       *manager);
static void       gui_device_change_notify      (Gimp               *gimp);

static void       gui_global_buffer_changed     (Gimp               *gimp);

static void       gui_menu_show_tooltip         (GimpUIManager      *manager,
                                                 const gchar        *tooltip,
                                                 Gimp               *gimp);
static void       gui_menu_hide_tooltip         (GimpUIManager      *manager,
                                                 Gimp               *gimp);

static void       gui_display_changed           (GimpContext        *context,
                                                 GimpDisplay        *display,
                                                 Gimp               *gimp);
static void       gui_display_remove            (GimpContainer      *displays);

static void       gui_dbus_service_init         (Gimp               *gimp);
static void       gui_dbus_service_exit         (void);


/*  private variables  */

static Gimp            *the_gui_gimp     = NULL;
static GimpUIManager   *image_ui_manager = NULL;

#if HAVE_DBUS_GLIB
static DBusGConnection *dbus_connection  = NULL;
#endif


/*  public functions  */

void
gui_libs_init (GOptionContext *context)
{
  g_return_if_fail (context != NULL);

  g_option_context_add_group (context, gtk_get_option_group (TRUE));
}

void
gui_abort (const gchar *abort_message)
{
  GtkWidget *dialog;
  GtkWidget *box;

  g_return_if_fail (abort_message != NULL);

  dialog = gimp_dialog_new (_("GIMP Message"), "gimp-abort",
                            NULL, GTK_DIALOG_MODAL, NULL, NULL,
                            GTK_STOCK_OK, GTK_RESPONSE_OK,
                            NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  box = g_object_new (GIMP_TYPE_MESSAGE_BOX,
                      "stock-id",     GIMP_STOCK_WILBER_EEK,
                      "border-width", 12,
                      NULL);

  gimp_message_box_set_text (GIMP_MESSAGE_BOX (box), abort_message);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), box);
  gtk_widget_show (box);

  gimp_dialog_run (GIMP_DIALOG (dialog));

  exit (EXIT_FAILURE);
}

GimpInitStatusFunc
gui_init (Gimp     *gimp,
          gboolean  no_splash)
{
  GimpInitStatusFunc  status_callback = NULL;
  GdkScreen          *screen;
  gchar              *abort_message;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (the_gui_gimp == NULL, NULL);

  abort_message = gui_sanity_check ();
  if (abort_message)
    gui_abort (abort_message);

  gimp_widgets_init (gui_help_func,
                     gui_get_foreground_func,
                     gui_get_background_func,
                     NULL);

  g_type_class_ref (GIMP_TYPE_COLOR_SELECT);

  the_gui_gimp = gimp;

  gimp_dnd_init (gimp);

  themes_init (gimp);

  gdk_rgb_set_min_colors (CLAMP (gimp->config->min_colors, 27, 256));
  gdk_rgb_set_install (gimp->config->install_cmap);

  screen = gdk_screen_get_default ();
  gtk_widget_set_default_colormap (gdk_screen_get_rgb_colormap (screen));

  if (! no_splash)
    {
      splash_create (gimp->be_verbose);
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
                                    "gimp-tips-dialog", -1, TRUE);
}


/*  private functions  */

static gchar *
gui_sanity_check (void)
{
  const gchar *mismatch;

#define GTK_REQUIRED_MAJOR 2
#define GTK_REQUIRED_MINOR 10
#define GTK_REQUIRED_MICRO 13

  mismatch = gtk_check_version (GTK_REQUIRED_MAJOR,
                                GTK_REQUIRED_MINOR,
                                GTK_REQUIRED_MICRO);

  if (mismatch)
    {
      return g_strdup_printf
        ("%s\n\n"
         "GIMP requires GTK+ version %d.%d.%d or later.\n"
         "Installed GTK+ version is %d.%d.%d.\n\n"
         "Somehow you or your software packager managed\n"
         "to install GIMP with an older GTK+ version.\n\n"
         "Please upgrade to GTK+ version %d.%d.%d or later.",
         mismatch,
         GTK_REQUIRED_MAJOR, GTK_REQUIRED_MINOR, GTK_REQUIRED_MICRO,
         gtk_major_version, gtk_minor_version, gtk_micro_version,
         GTK_REQUIRED_MAJOR, GTK_REQUIRED_MINOR, GTK_REQUIRED_MICRO);
    }
#undef GTK_REQUIRED_MAJOR
#undef GTK_REQUIRED_MINOR
#undef GTK_REQUIRED_MICRO

  return NULL;
}


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
      gimp_environ_table_add (gimp->plug_in_manager->environ_table,
                              name, display, NULL);
      g_free (display);
    }

  gimp_tools_init (gimp);

  gimp_context_set_tool (gimp_get_user_context (gimp),
                         gimp_tool_info_get_standard (gimp));
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

  if (! gui_config->show_tooltips)
    gimp_help_disable_tooltips ();

  g_signal_connect (gui_config, "notify::show-tooltips",
                    G_CALLBACK (gui_show_tooltips_notify),
                    gimp);

  gimp_dialogs_show_help_button (gui_config->use_help &&
                                 gui_config->show_help_button);

  g_signal_connect (gui_config, "notify::use-help",
                    G_CALLBACK (gui_show_help_button_notify),
                    gimp);
  g_signal_connect (gui_config, "notify::show-help-button",
                    G_CALLBACK (gui_show_help_button_notify),
                    gimp);

  g_signal_connect (gimp_get_user_context (gimp), "display-changed",
                    G_CALLBACK (gui_display_changed),
                    gimp);
  g_signal_connect (gimp->displays, "remove",
                    G_CALLBACK (gui_display_remove),
                    NULL);

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

  actions_init (gimp);
  menus_init (gimp, global_action_factory);
  gimp_render_init (gimp);
  gimp_display_shell_render_init (gimp);

  dialogs_init (gimp, global_menu_factory);

  gimp_clipboard_init (gimp);
  gimp_clipboard_set_buffer (gimp, gimp->global_buffer);

  g_signal_connect (gimp, "buffer-changed",
                    G_CALLBACK (gui_global_buffer_changed),
                    NULL);

  gimp_devices_init (gimp, gui_device_change_notify);
  gimp_controllers_init (gimp);
  session_init (gimp);

  g_type_class_unref (g_type_class_ref (GIMP_TYPE_COLOR_SELECTOR_PALETTE));

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

  image_ui_manager = gimp_menu_factory_manager_new (global_menu_factory,
                                                    "<Image>",
                                                    gimp,
                                                    gui_config->tearoff_menus);
  gimp_ui_manager_update (image_ui_manager, NULL);

#ifdef GDK_WINDOWING_QUARTZ
  {
    GtkWidget *menu;
    GtkWidget *item;

    menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (image_ui_manager),
				      "/dummy-menubar/image-popup");

    if (GTK_IS_MENU_ITEM (menu))
      menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (menu));

    gtk_macmenu_set_menubar (GTK_MENU_SHELL (menu));

    item = gtk_ui_manager_get_widget (GTK_UI_MANAGER (image_ui_manager),
                                      "/dummy-menubar/image-popup/File/file-quit");
    if (GTK_IS_MENU_ITEM (item))
      gtk_macmenu_set_quit_item (GTK_MENU_ITEM (item));

    item = gtk_ui_manager_get_widget (GTK_UI_MANAGER (image_ui_manager),
                                      "/dummy-menubar/image-popup/Help/dialogs-about");
    if (GTK_IS_MENU_ITEM (item))
      gtk_macmenu_set_about_item (GTK_MENU_ITEM (item), _("About GIMP"));

    item = gtk_ui_manager_get_widget (GTK_UI_MANAGER (image_ui_manager),
                                      "/dummy-menubar/image-popup/Edit/dialogs-preferences");
    if (GTK_IS_MENU_ITEM (item))
      gtk_macmenu_set_prefs_item (GTK_MENU_ITEM (item), _("Preferences"));
  }
#endif /* GDK_WINDOWING_QUARTZ */

  g_signal_connect_object (gui_config, "notify::tearoff-menus",
                           G_CALLBACK (gui_tearoff_menus_notify),
                           image_ui_manager, 0);
  g_signal_connect (image_ui_manager, "show-tooltip",
                    G_CALLBACK (gui_menu_show_tooltip),
                    gimp);
  g_signal_connect (image_ui_manager, "hide-tooltip",
                    G_CALLBACK (gui_menu_hide_tooltip),
                    gimp);

  gimp_devices_restore (gimp);
  gimp_controllers_restore (gimp, image_ui_manager);

  if (status_callback == splash_update)
    splash_destroy ();

  color_history_restore (gimp);

  if (gui_config->restore_session)
    session_restore (gimp);

  dialogs_show_toolbox ();

  gui_dbus_service_init (gimp);
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
      gimp_dialog_factory_dialog_raise (global_dialog_factory,
                                        gdk_screen_get_default (),
                                        "gimp-quit-dialog", -1);

      return TRUE; /* stop exit for now */
    }

  gimp->message_handler = GIMP_CONSOLE;

#if HAVE_DBUS_GLIB
  gui_dbus_service_exit ();
#endif

  if (gui_config->save_session_info)
    session_save (gimp, FALSE);

  color_history_save (gimp);

  if (gui_config->save_accels)
    menus_save (gimp, FALSE);

  if (gui_config->save_device_status)
    gimp_devices_save (gimp, FALSE);

  if (TRUE /* gui_config->save_controllers */)
    gimp_controllers_save (gimp);

  g_signal_handlers_disconnect_by_func (gimp->displays,
                                        gui_display_remove,
                                        NULL);
  g_signal_handlers_disconnect_by_func (gimp_get_user_context (gimp),
                                        gui_display_changed,
                                        gimp);

  gimp_displays_delete (gimp);

  gimp_tools_save (gimp, gui_config->save_tool_options, FALSE);
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
                                        gui_show_help_button_notify,
                                        gimp);
  g_signal_handlers_disconnect_by_func (gimp->config,
                                        gui_show_tooltips_notify,
                                        gimp);

  g_object_unref (image_ui_manager);
  image_ui_manager = NULL;

  session_exit (gimp);
  menus_exit (gimp);
  actions_exit (gimp);
  gimp_display_shell_render_exit (gimp);
  gimp_render_exit (gimp);

  dialogs_exit (gimp);
  gimp_controllers_exit (gimp);
  gimp_devices_exit (gimp);

  g_signal_handlers_disconnect_by_func (gimp,
                                        G_CALLBACK (gui_global_buffer_changed),
                                        NULL);
  gimp_clipboard_exit (gimp);

  themes_exit (gimp);

  g_type_class_unref (g_type_class_peek (GIMP_TYPE_COLOR_SELECT));

  return FALSE; /* continue exiting */
}

static void
gui_show_tooltips_notify (GimpGuiConfig *gui_config,
                          GParamSpec    *param_spec,
                          Gimp          *gimp)
{
  if (gui_config->show_tooltips)
    gimp_help_enable_tooltips ();
  else
    gimp_help_disable_tooltips ();
}

static void
gui_show_help_button_notify (GimpGuiConfig *gui_config,
                             GParamSpec    *param_spec,
                             Gimp          *gimp)
{
  gimp_dialogs_show_help_button (gui_config->use_help &&
                                 gui_config->show_help_button);
}

static void
gui_tearoff_menus_notify (GimpGuiConfig *gui_config,
                          GParamSpec    *pspec,
                          GtkUIManager  *manager)
{
  gtk_ui_manager_set_add_tearoffs (manager, gui_config->tearoff_menus);
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

static void
gui_global_buffer_changed (Gimp *gimp)
{
  gimp_clipboard_set_buffer (gimp, gimp->global_buffer);
}

static void
gui_menu_show_tooltip (GimpUIManager *manager,
                       const gchar   *tooltip,
                       Gimp          *gimp)
{
  GimpContext *context = gimp_get_user_context (gimp);
  GimpDisplay *display = gimp_context_get_display (context);

  if (display)
    {
      GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (display->shell);

      gimp_statusbar_push (GIMP_STATUSBAR (shell->statusbar), "menu-tooltip",
                           "%s", tooltip);
    }
}

static void
gui_menu_hide_tooltip (GimpUIManager *manager,
                       Gimp          *gimp)
{
  GimpContext *context = gimp_get_user_context (gimp);
  GimpDisplay *display = gimp_context_get_display (context);

  if (display)
    {
      GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (display->shell);

      gimp_statusbar_pop (GIMP_STATUSBAR (shell->statusbar), "menu-tooltip");
    }
}

static void
gui_display_changed (GimpContext *context,
                     GimpDisplay *display,
                     Gimp        *gimp)
{
  if (! display)
    {
      GimpImage *image = gimp_context_get_image (context);

      if (image)
        {
          GList *list;

          for (list = GIMP_LIST (gimp->displays)->list;
               list;
               list = g_list_next (list))
            {
              GimpDisplay *display2 = list->data;

              if (display2->image == image)
                {
                  gimp_context_set_display (context, display2);

                  /* stop the emission of the original signal
                   * (the emission of the recursive signal is finished)
                   */
                  g_signal_stop_emission_by_name (context, "display-changed");
                  return;
                }
            }

          gimp_context_set_image (context, NULL);
        }
    }

  gimp_ui_manager_update (image_ui_manager, display);
}

static void
gui_display_remove (GimpContainer *displays)
{
  /* show the toolbox when the last image window is closed */

  if (gimp_container_is_empty (displays))
    dialogs_show_toolbox ();
}

static void
gui_dbus_service_init (Gimp *gimp)
{
#if HAVE_DBUS_GLIB
  GError  *error = NULL;

  g_return_if_fail (dbus_connection == NULL);

  dbus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (dbus_connection)
    {
      GObject *service = gimp_dbus_service_new (gimp);

      dbus_bus_request_name (dbus_g_connection_get_connection (dbus_connection),
                             GIMP_DBUS_SERVICE_NAME, 0, NULL);

      dbus_g_connection_register_g_object (dbus_connection,
                                           GIMP_DBUS_SERVICE_PATH, service);
    }
  else
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
    }
#endif
}

static void
gui_dbus_service_exit (void)
{
#if HAVE_DBUS_GLIB
  if (dbus_connection)
    {
      dbus_g_connection_unref (dbus_connection);
      dbus_connection = NULL;
    }
#endif
}
