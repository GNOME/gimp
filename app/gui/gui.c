/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"

#include "gui-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "plug-in/gimpenvirontable.h"
#include "plug-in/gimppluginmanager.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpstatusbar.h"

#include "tools/gimp-tools.h"

#include "widgets/gimpaction-history.h"
#include "widgets/gimpclipboard.h"
#include "widgets/gimpcolorselectorpalette.h"
#include "widgets/gimpcontrollers.h"
#include "widgets/gimpdevices.h"
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
#include "widgets/gimplanguagestore-parser.h"

#include "actions/actions.h"
#include "actions/windows-commands.h"

#include "menus/menus.h"

#include "dialogs/dialogs.h"

#include "gimpuiconfigurer.h"
#include "gui.h"
#include "gui-unique.h"
#include "gui-vtable.h"
#include "icon-themes.h"
#include "session.h"
#include "splash.h"
#include "themes.h"

#ifdef GDK_WINDOWING_QUARTZ
#import <AppKit/AppKit.h>
#include <gtkosxapplication.h>

/* Forward declare since we are building against old SDKs. */
#if !defined(MAC_OS_X_VERSION_10_12) || \
    MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_12

@interface NSWindow(ForwardDeclarations)
+ (void)setAllowsAutomaticWindowTabbing:(BOOL)allow;
@end

#endif

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
static void       gui_user_manual_notify        (GimpGuiConfig      *gui_config,
                                                 GParamSpec         *pspec,
                                                 Gimp               *gimp);
static void       gui_single_window_mode_notify (GimpGuiConfig      *gui_config,
                                                 GParamSpec         *pspec,
                                                 GimpUIConfigurer   *ui_configurer);
static void       gui_tearoff_menus_notify      (GimpGuiConfig      *gui_config,
                                                 GParamSpec         *pspec,
                                                 GtkUIManager       *manager);

static void       gui_clipboard_changed         (Gimp               *gimp);

static void       gui_menu_show_tooltip         (GimpUIManager      *manager,
                                                 const gchar        *tooltip,
                                                 Gimp               *gimp);
static void       gui_menu_hide_tooltip         (GimpUIManager      *manager,
                                                 Gimp               *gimp);

static void       gui_display_changed           (GimpContext        *context,
                                                 GimpDisplay        *display,
                                                 Gimp               *gimp);

static void       gui_compare_accelerator       (gpointer            data,
                                                 const gchar        *accel_path,
                                                 guint               accel_key,
                                                 GdkModifierType     accel_mods,
                                                 gboolean            changed);
static void      gui_check_unique_accelerator   (gpointer            data,
                                                 const gchar        *accel_path,
                                                 guint               accel_key,
                                                 GdkModifierType     accel_mods,
                                                 gboolean            changed);
static gboolean  gui_check_action_exists        (const gchar *accel_path);


/*  private variables  */

static Gimp             *the_gui_gimp     = NULL;
static GimpUIManager    *image_ui_manager = NULL;
static GimpUIConfigurer *ui_configurer    = NULL;
static GdkScreen        *initial_screen   = NULL;
static gint              initial_monitor  = -1;


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

                            _("_OK"), GTK_RESPONSE_OK,

                            NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  box = g_object_new (GIMP_TYPE_MESSAGE_BOX,
                      "icon-name",    GIMP_ICON_WILBER_EEK,
                      "border-width", 12,
                      NULL);

  gimp_message_box_set_text (GIMP_MESSAGE_BOX (box), "%s", abort_message);

  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  gimp_dialog_run (GIMP_DIALOG (dialog));

  exit (EXIT_FAILURE);
}

GimpInitStatusFunc
gui_init (Gimp     *gimp,
          gboolean  no_splash)
{
  GimpInitStatusFunc  status_callback = NULL;
  gchar              *abort_message;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (the_gui_gimp == NULL, NULL);

  abort_message = gui_sanity_check ();
  if (abort_message)
    gui_abort (abort_message);

  the_gui_gimp = gimp;

  /* TRANSLATORS: there is no need to translate this in GIMP. This uses
   * "gtk20" domain as a special trick to determine language direction,
   * but xgettext extracts it anyway mistakenly into GIMP po files.
   * Leave an empty string as translation. It does not matter.
   */
  if (g_strcmp0 (dgettext ("gtk20", "default:LTR"), "default:RTL") == 0)
    /* Normally this should have been taken care of during command line
     * parsing as a post-parse hook of gtk_get_option_group(), using the
     * system locales.
     * But user config may have overridden the language, therefore we must
     * check the widget directions again.
     */
    gtk_widget_set_default_direction (GTK_TEXT_DIR_RTL);
  else
    gtk_widget_set_default_direction (GTK_TEXT_DIR_LTR);

  gui_unique_init (gimp);
  gimp_language_store_parser_init ();

  /*  initialize icon themes before gimp_widgets_init() so we avoid
   *  setting the configured theme twice
   */
  icon_themes_init (gimp);

  gimp_widgets_init (gui_help_func,
                     gui_get_foreground_func,
                     gui_get_background_func,
                     NULL);

  g_type_class_ref (GIMP_TYPE_COLOR_SELECT);

  /*  disable automatic startup notification  */
  gtk_window_set_auto_startup_notification (FALSE);

#ifdef GDK_WINDOWING_QUARTZ
  /* Before the first window is created (typically the splash window),
   * we need to disable automatic tabbing behavior introduced on Sierra.
   * This is known to cause all kinds of weird issues (see for instance
   * Bugzilla #776294) and needs proper GTK+ support if we would want to
   * enable it.
   */
  if ([NSWindow respondsToSelector:@selector(setAllowsAutomaticWindowTabbing:)])
    [NSWindow setAllowsAutomaticWindowTabbing:NO];
#endif /* GDK_WINDOWING_QUARTZ */

  gimp_dnd_init (gimp);

  themes_init (gimp);

  initial_monitor = gimp_get_monitor_at_pointer (&initial_screen);

  if (! no_splash)
    {
      splash_create (gimp->be_verbose, initial_screen, initial_monitor);
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

/*
 * gui_recover:
 * @n_recoveries: number of recovered files.
 *
 * Query the user interactively if files were saved from a previous
 * crash, asking whether to try and recover or discard them.
 *
 * Returns: TRUE if answer is to try and recover, FALSE otherwise.
 */
gboolean
gui_recover (gint n_recoveries)
{
  GtkWidget *dialog;
  GtkWidget *box;
  gboolean   recover;

  dialog = gimp_dialog_new (_("Image Recovery"), "gimp-recovery",
                            NULL, GTK_DIALOG_MODAL, NULL, NULL,
                            _("_Discard"), GTK_RESPONSE_CANCEL,
                            _("_Recover"), GTK_RESPONSE_OK,
                            NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                   GTK_RESPONSE_OK);

  box = gimp_message_box_new (GIMP_ICON_WILBER_EEK);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_BOX (box),
                                     _("Eeek! It looks like GIMP recovered from a crash!"));

  gimp_message_box_set_text (GIMP_MESSAGE_BOX (box),
                             /* TRANSLATORS: even if English singular form does
                              * not use %d, you can use %d for translation in
                              * any singular/plural form of your language if
                              * suited. It will just work and be replaced by the
                              * number of images as expected.
                              */
                             ngettext ("An image was salvaged from the crash. "
                                       "Do you want to try and recover it?",
                                       "%d images were salvaged from the crash. "
                                       "Do you want to try and recover them?",
                                       n_recoveries), n_recoveries);

  recover = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);
  gtk_widget_destroy (dialog);

  return recover;
}

gint
gui_get_initial_monitor (Gimp       *gimp,
                         GdkScreen **screen)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), 0);
  g_return_val_if_fail (screen != NULL, 0);

  *screen = initial_screen;

  return initial_monitor;
}


/*  private functions  */

static gchar *
gui_sanity_check (void)
{
#define GTK_REQUIRED_MAJOR 3
#define GTK_REQUIRED_MINOR 22
#define GTK_REQUIRED_MICRO 29

  const gchar *mismatch = gtk_check_version (GTK_REQUIRED_MAJOR,
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

  gimp_help (the_gui_gimp, NULL, NULL, help_id);
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
    g_print ("INIT: %s\n", G_STRFUNC);

#if defined (GDK_WINDOWING_X11)
  name = "DISPLAY";
#elif defined (GDK_WINDOWING_DIRECTFB) || defined (GDK_WINDOWING_FB)
  name = "GDK_DISPLAY";
#endif

  /* TODO: Need to care about display migration with GTK+ 2.2 at some point */

  if (name)
    {
      gchar *display = gdk_get_display ();

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
    g_print ("INIT: %s\n", G_STRFUNC);

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
  g_signal_connect (gui_config, "notify::user-manual-online",
                    G_CALLBACK (gui_user_manual_notify),
                    gimp);
  g_signal_connect (gui_config, "notify::show-help-button",
                    G_CALLBACK (gui_show_help_button_notify),
                    gimp);

  g_signal_connect (gimp_get_user_context (gimp), "display-changed",
                    G_CALLBACK (gui_display_changed),
                    gimp);

  /* make sure the monitor resolution is valid */
  if (display_config->monitor_res_from_gdk               ||
      display_config->monitor_xres < GIMP_MIN_RESOLUTION ||
      display_config->monitor_yres < GIMP_MIN_RESOLUTION)
    {
      gdouble xres, yres;

      gimp_get_monitor_resolution (initial_screen,
                                   initial_monitor,
                                   &xres, &yres);

      g_object_set (gimp->config,
                    "monitor-xresolution",                      xres,
                    "monitor-yresolution",                      yres,
                    "monitor-resolution-from-windowing-system", TRUE,
                    NULL);
    }

  actions_init (gimp);
  menus_init (gimp, global_action_factory);
  gimp_render_init (gimp);

  dialogs_init (gimp, global_menu_factory);

  gimp_clipboard_init (gimp);
  if (gimp_get_clipboard_image (gimp))
    gimp_clipboard_set_image (gimp, gimp_get_clipboard_image (gimp));
  else
    gimp_clipboard_set_buffer (gimp, gimp_get_clipboard_buffer (gimp));

  g_signal_connect (gimp, "clipboard-changed",
                    G_CALLBACK (gui_clipboard_changed),
                    NULL);

  gimp_devices_init (gimp);
  gimp_controllers_init (gimp);
  session_init (gimp);

  g_type_class_unref (g_type_class_ref (GIMP_TYPE_COLOR_SELECTOR_PALETTE));

  status_callback (NULL, _("Tool Options"), 1.0);
  gimp_tools_restore (gimp);
}

#ifdef GDK_WINDOWING_QUARTZ
static void
gui_add_to_app_menu (GimpUIManager     *ui_manager,
                     GtkosxApplication *osx_app,
                     const gchar       *action_path,
                     gint               index)
{
  GtkWidget *item;

  item = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui_manager), action_path);

  if (GTK_IS_MENU_ITEM (item))
    gtkosx_application_insert_app_menu_item (osx_app, GTK_WIDGET (item), index);
}

static gboolean
gui_quartz_quit_callback (GtkosxApplication *osx_app,
                          GimpUIManager     *ui_manager)
{
  gimp_ui_manager_activate_action (ui_manager, "file", "file-quit");

  return TRUE;
}
#endif

static void
gui_restore_after_callback (Gimp               *gimp,
                            GimpInitStatusFunc  status_callback)
{
  GimpGuiConfig *gui_config = GIMP_GUI_CONFIG (gimp->config);
  GimpDisplay   *display;

  if (gimp->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  gimp->message_handler = GIMP_MESSAGE_BOX;

  /*  load the recent documents after gimp_real_restore() because we
   *  need the mime-types implemented by plug-ins
   */
  status_callback (NULL, _("Documents"), 0.9);
  gimp_recent_list_load (gimp);

  /*  enable this to always have icons everywhere  */
  if (g_getenv ("GIMP_ICONS_LIKE_A_BOSS"))
    {
      GdkScreen *screen = gdk_screen_get_default ();

      g_object_set (G_OBJECT (gtk_settings_get_for_screen (screen)),
                    "gtk-button-images", TRUE,
                    "gtk-menu-images",   TRUE,
                    NULL);
    }

  if (gui_config->restore_accels)
    menus_restore (gimp);

  ui_configurer = g_object_new (GIMP_TYPE_UI_CONFIGURER,
                                "gimp", gimp,
                                NULL);

  image_ui_manager = gimp_menu_factory_manager_new (global_menu_factory,
                                                    "<Image>",
                                                    gimp,
                                                    gui_config->tearoff_menus);
  gimp_ui_manager_update (image_ui_manager, gimp);

  /* Check that every accelerator is unique. */
  gtk_accel_map_foreach_unfiltered (NULL,
                                    gui_check_unique_accelerator);

  gimp_action_history_init (gimp);

#ifdef GDK_WINDOWING_QUARTZ
  {
    GtkosxApplication *osx_app;
    GtkWidget         *menu;
    GtkWidget         *item;

    [[NSUserDefaults standardUserDefaults] setObject:@"NO"
                                           forKey:@"NSTreatUnknownArgumentsAsOpen"];

    osx_app = gtkosx_application_get ();

    menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (image_ui_manager),
                                      "/image-menubar");
    if (GTK_IS_MENU_ITEM (menu))
      menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (menu));

    gtkosx_application_set_menu_bar (osx_app, GTK_MENU_SHELL (menu));
    gtkosx_application_set_use_quartz_accelerators (osx_app, FALSE);

    gui_add_to_app_menu (image_ui_manager, osx_app,
                         "/image-menubar/Help/dialogs-about", 0);
    gui_add_to_app_menu (image_ui_manager, osx_app,
                         "/image-menubar/Help/dialogs-search-action", 1);

#define PREFERENCES "/image-menubar/Edit/Preferences/"

    gui_add_to_app_menu (image_ui_manager, osx_app,
                         PREFERENCES "dialogs-preferences", 3);
    gui_add_to_app_menu (image_ui_manager, osx_app,
                         PREFERENCES "dialogs-input-devices", 4);
    gui_add_to_app_menu (image_ui_manager, osx_app,
                         PREFERENCES "dialogs-keyboard-shortcuts", 5);
    gui_add_to_app_menu (image_ui_manager, osx_app,
                         PREFERENCES "dialogs-module-dialog", 6);
    gui_add_to_app_menu (image_ui_manager, osx_app,
                         PREFERENCES "plug-in-unit-editor", 7);

#undef PREFERENCES

    item = gtk_separator_menu_item_new ();
    gtkosx_application_insert_app_menu_item (osx_app, item, 8);

    item = gtk_ui_manager_get_widget (GTK_UI_MANAGER (image_ui_manager),
                                      "/image-menubar/File/file-quit");
    gtk_widget_hide (item);

    g_signal_connect (osx_app, "NSApplicationBlockTermination",
                      G_CALLBACK (gui_quartz_quit_callback),
                      image_ui_manager);

    gtkosx_application_ready (osx_app);
  }
#endif /* GDK_WINDOWING_QUARTZ */

  g_signal_connect_object (gui_config, "notify::single-window-mode",
                           G_CALLBACK (gui_single_window_mode_notify),
                           ui_configurer, 0);
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

  if (gimp_get_show_gui (gimp))
    {
      GimpDisplayShell *shell;
      GtkWidget        *toplevel;

      /*  create the empty display  */
      display = GIMP_DISPLAY (gimp_create_display (gimp, NULL,
                                                   GIMP_UNIT_PIXEL, 1.0,
                                                   G_OBJECT (initial_screen),
                                                   initial_monitor));

      shell = gimp_display_get_shell (display);

      if (gui_config->restore_session)
        session_restore (gimp,
                         initial_screen,
                         initial_monitor);

      /*  move keyboard focus to the display  */
      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));
      gtk_window_present (GTK_WINDOW (toplevel));
    }

  /*  indicate that the application has finished loading  */
  gdk_notify_startup_complete ();

  /*  clear startup monitor variables  */
  initial_screen  = NULL;
  initial_monitor = -1;
}

static gboolean
gui_exit_callback (Gimp     *gimp,
                   gboolean  force)
{
  GimpGuiConfig  *gui_config = GIMP_GUI_CONFIG (gimp->config);

  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  if (! force && gimp_displays_dirty (gimp))
    {
      GdkScreen *screen;
      gint       monitor;

      monitor = gimp_get_monitor_at_pointer (&screen);

      gimp_dialog_factory_dialog_raise (gimp_dialog_factory_get_singleton (),
                                        screen, monitor,
                                        "gimp-quit-dialog", -1);

      return TRUE; /* stop exit for now */
    }

  gimp->message_handler = GIMP_CONSOLE;

  gui_unique_exit ();

  if (gui_config->save_session_info)
    session_save (gimp, FALSE);

  if (gui_config->save_device_status)
    gimp_devices_save (gimp, FALSE);

  if (TRUE /* gui_config->save_controllers */)
    gimp_controllers_save (gimp);

  g_signal_handlers_disconnect_by_func (gimp_get_user_context (gimp),
                                        gui_display_changed,
                                        gimp);

  gimp_displays_delete (gimp);

  if (gui_config->save_accels)
    menus_save (gimp, FALSE);

  gimp_tools_save (gimp, gui_config->save_tool_options, FALSE);
  gimp_tools_exit (gimp);

  gimp_language_store_parser_clean ();

  return FALSE; /* continue exiting */
}

static gboolean
gui_exit_after_callback (Gimp     *gimp,
                         gboolean  force)
{
  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  g_signal_handlers_disconnect_by_func (gimp->config,
                                        gui_show_help_button_notify,
                                        gimp);
  g_signal_handlers_disconnect_by_func (gimp->config,
                                        gui_user_manual_notify,
                                        gimp);
  g_signal_handlers_disconnect_by_func (gimp->config,
                                        gui_show_tooltips_notify,
                                        gimp);

  gimp_action_history_exit (gimp);

  g_object_unref (image_ui_manager);
  image_ui_manager = NULL;

  g_object_unref (ui_configurer);
  ui_configurer = NULL;

  /*  exit the clipboard before shutting down the GUI because it runs
   *  a whole lot of code paths. See bug #731389.
   */
  g_signal_handlers_disconnect_by_func (gimp,
                                        G_CALLBACK (gui_clipboard_changed),
                                        NULL);
  gimp_clipboard_exit (gimp);

  session_exit (gimp);
  menus_exit (gimp);
  actions_exit (gimp);
  gimp_render_exit (gimp);

  gimp_controllers_exit (gimp);
  gimp_devices_exit (gimp);
  dialogs_exit (gimp);
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
gui_user_manual_notify (GimpGuiConfig *gui_config,
                        GParamSpec    *param_spec,
                        Gimp          *gimp)
{
  gimp_help_user_manual_changed (gimp);
}

static void
gui_single_window_mode_notify (GimpGuiConfig      *gui_config,
                               GParamSpec         *pspec,
                               GimpUIConfigurer   *ui_configurer)
{
  gimp_ui_configurer_configure (ui_configurer,
                                gui_config->single_window_mode);
}
static void
gui_tearoff_menus_notify (GimpGuiConfig *gui_config,
                          GParamSpec    *pspec,
                          GtkUIManager  *manager)
{
  gtk_ui_manager_set_add_tearoffs (manager, gui_config->tearoff_menus);
}

static void
gui_clipboard_changed (Gimp *gimp)
{
  if (gimp_get_clipboard_image (gimp))
    gimp_clipboard_set_image (gimp, gimp_get_clipboard_image (gimp));
  else
    gimp_clipboard_set_buffer (gimp, gimp_get_clipboard_buffer (gimp));
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
      GimpDisplayShell *shell     = gimp_display_get_shell (display);
      GimpStatusbar    *statusbar = gimp_display_shell_get_statusbar (shell);

      gimp_statusbar_push (statusbar, "menu-tooltip",
                           NULL, "%s", tooltip);
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
      GimpDisplayShell *shell     = gimp_display_get_shell (display);
      GimpStatusbar    *statusbar = gimp_display_shell_get_statusbar (shell);

      gimp_statusbar_pop (statusbar, "menu-tooltip");
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

          for (list = gimp_get_display_iter (gimp);
               list;
               list = g_list_next (list))
            {
              GimpDisplay *display2 = list->data;

              if (gimp_display_get_image (display2) == image)
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

typedef struct
{
  const gchar     *path;
  guint            key;
  GdkModifierType  mods;
}
accelData;

static void
gui_compare_accelerator (gpointer         data,
                         const gchar     *accel_path,
                         guint            accel_key,
                         GdkModifierType  accel_mods,
                         gboolean         changed)
{
  accelData *accel = data;

  if (accel->key == accel_key && accel->mods == accel_mods &&
      g_strcmp0 (accel->path, accel_path))
    {
      g_printerr ("Actions \"%s\" and \"%s\" use the same accelerator.\n"
                  "  Disabling the accelerator on \"%s\".\n",
                  accel->path, accel_path, accel_path);
      gtk_accel_map_change_entry (accel_path, 0, 0, FALSE);
    }
}

static void
gui_check_unique_accelerator (gpointer         data,
                              const gchar     *accel_path,
                              guint            accel_key,
                              GdkModifierType  accel_mods,
                              gboolean         changed)
{
  if (gtk_accelerator_valid (accel_key, accel_mods) &&
      gui_check_action_exists (accel_path))
    {
      accelData accel;

      accel.path = accel_path;
      accel.key  = accel_key;
      accel.mods = accel_mods;

      gtk_accel_map_foreach_unfiltered (&accel,
                                        gui_compare_accelerator);
    }
}

static gboolean
gui_check_action_exists (const gchar *accel_path)
{
  GimpUIManager *manager;
  gboolean       action_exists = FALSE;
  GList         *list;

  manager = gimp_ui_managers_from_name ("<Image>")->data;
  for (list = gtk_ui_manager_get_action_groups (GTK_UI_MANAGER (manager));
       list;
       list = g_list_next (list))
    {
      GtkActionGroup *group   = list->data;
      GList          *actions = NULL;
      GList          *list2;

      actions = gtk_action_group_list_actions (GTK_ACTION_GROUP (group));
      for (list2 = actions; list2; list2 = g_list_next (list2))
        {
          const gchar *path;
          GtkAction   *action = list2->data;

          path = gtk_action_get_accel_path (action);
          if (g_strcmp0 (path, accel_path) == 0)
            {
              action_exists = TRUE;
              break;
            }
        }
      g_list_free (actions);
      if (action_exists)
        break;
    }
  return action_exists;
}
