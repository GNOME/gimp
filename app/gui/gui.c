/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"
#include "libligmawidgets/ligmawidgets-private.h"

#include "gui-types.h"
#include "ligmaapp.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmatoolinfo.h"

#include "plug-in/ligmaenvirontable.h"
#include "plug-in/ligmapluginmanager.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplay-foreach.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmastatusbar.h"

#include "tools/ligma-tools.h"
#include "tools/ligmatool.h"
#include "tools/tool_manager.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmaaction-history.h"
#include "widgets/ligmaclipboard.h"
#include "widgets/ligmacolorselectorpalette.h"
#include "widgets/ligmacontrollers.h"
#include "widgets/ligmadevices.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadnd.h"
#include "widgets/ligmarender.h"
#include "widgets/ligmahelp.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamenufactory.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmasessioninfo.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"
#include "widgets/ligmalanguagestore-parser.h"

#include "actions/actions.h"
#include "actions/windows-commands.h"

#include "menus/menus.h"

#include "dialogs/dialogs.h"

#include "ligmauiconfigurer.h"
#include "gui.h"
#include "gui-unique.h"
#include "gui-vtable.h"
#include "icon-themes.h"
#include "modifiers.h"
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

#include "ligma-intl.h"


/*  local function prototypes  */

static gchar    * gui_sanity_check              (void);
static void       gui_help_func                 (const gchar        *help_id,
                                                 gpointer            help_data);
static gboolean   gui_get_background_func       (LigmaRGB            *color);
static gboolean   gui_get_foreground_func       (LigmaRGB            *color);

static void       gui_initialize_after_callback (Ligma               *ligma,
                                                 LigmaInitStatusFunc  callback);

static void       gui_restore_callback          (Ligma               *ligma,
                                                 LigmaInitStatusFunc  callback);
static void       gui_restore_after_callback    (Ligma               *ligma,
                                                 LigmaInitStatusFunc  callback);

static gboolean   gui_exit_callback             (Ligma               *ligma,
                                                 gboolean            force);
static gboolean   gui_exit_after_callback       (Ligma               *ligma,
                                                 gboolean            force);

static void       gui_show_help_button_notify   (LigmaGuiConfig      *gui_config,
                                                 GParamSpec         *pspec,
                                                 Ligma               *ligma);
static void       gui_user_manual_notify        (LigmaGuiConfig      *gui_config,
                                                 GParamSpec         *pspec,
                                                 Ligma               *ligma);
static void       gui_single_window_mode_notify (LigmaGuiConfig      *gui_config,
                                                 GParamSpec         *pspec,
                                                 LigmaUIConfigurer   *ui_configurer);

static void       gui_clipboard_changed         (Ligma               *ligma);

static void       gui_menu_show_tooltip         (LigmaUIManager      *manager,
                                                 const gchar        *tooltip,
                                                 Ligma               *ligma);
static void       gui_menu_hide_tooltip         (LigmaUIManager      *manager,
                                                 Ligma               *ligma);

static void       gui_display_changed           (LigmaContext        *context,
                                                 LigmaDisplay        *display,
                                                 Ligma               *ligma);

static void       gui_compare_accelerator       (gpointer            data,
                                                 const gchar        *accel_path,
                                                 guint               accel_key,
                                                 GdkModifierType     accel_mods,
                                                 gboolean            changed);
static void       gui_check_unique_accelerator  (gpointer            data,
                                                 const gchar        *accel_path,
                                                 guint               accel_key,
                                                 GdkModifierType     accel_mods,
                                                 gboolean            changed);
static gboolean   gui_check_action_exists       (const gchar        *accel_path);


/*  private variables  */

static Ligma             *the_gui_ligma     = NULL;
static LigmaUIManager    *image_ui_manager = NULL;
static LigmaUIConfigurer *ui_configurer    = NULL;
static GdkMonitor       *initial_monitor  = NULL;


/*  public functions  */

void
gui_libs_init (GOptionContext *context)
{
  g_return_if_fail (context != NULL);

  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  /*  make the LigmaDisplay type known by name early, needed for the PDB */
  g_type_class_ref (LIGMA_TYPE_DISPLAY);
}

void
gui_abort (const gchar *abort_message)
{
  GtkWidget *dialog;
  GtkWidget *box;

  g_return_if_fail (abort_message != NULL);

  dialog = ligma_dialog_new (_("LIGMA Message"), "ligma-abort",
                            NULL, GTK_DIALOG_MODAL, NULL, NULL,

                            _("_OK"), GTK_RESPONSE_OK,

                            NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  box = g_object_new (LIGMA_TYPE_MESSAGE_BOX,
                      "icon-name",    LIGMA_ICON_WILBER_EEK,
                      "border-width", 12,
                      NULL);

  ligma_message_box_set_text (LIGMA_MESSAGE_BOX (box), "%s", abort_message);

  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  ligma_dialog_run (LIGMA_DIALOG (dialog));

  exit (EXIT_FAILURE);
}

/**
 * gui_init:
 * @ligma:
 * @no_splash:
 * @test_base_dir: a base prefix directory.
 *
 * @test_base_dir should be set to %NULL in all our codebase except for
 * unit testing calls.
 */
LigmaInitStatusFunc
gui_init (Ligma         *ligma,
          gboolean      no_splash,
          LigmaApp      *app,
          const gchar  *test_base_dir)
{
  LigmaInitStatusFunc  status_callback = NULL;
  gchar              *abort_message;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (the_gui_ligma == NULL, NULL);
  g_return_val_if_fail (LIGMA_IS_APP (app) || app == NULL, NULL);

  abort_message = gui_sanity_check ();
  if (abort_message)
    gui_abort (abort_message);

  the_gui_ligma = ligma;

  /* Normally this should have been taken care of during command line
   * parsing as a post-parse hook of gtk_get_option_group(), using the
   * system locales.
   * But user config may have overridden the language, therefore we must
   * check the widget directions again.
   */
  gtk_widget_set_default_direction (gtk_get_locale_direction ());

  gui_unique_init (ligma);
  ligma_language_store_parser_init ();

  /*  initialize icon themes before ligma_widgets_init() so we avoid
   *  setting the configured theme twice
   */
  icon_themes_init (ligma);

  ligma_widgets_init (gui_help_func,
                     gui_get_foreground_func,
                     gui_get_background_func,
                     NULL, test_base_dir);

  g_type_class_ref (LIGMA_TYPE_COLOR_SELECT);

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

  ligma_dnd_init (ligma);

  themes_init (ligma);

  initial_monitor = ligma_get_monitor_at_pointer ();

  if (! no_splash)
    {
      splash_create (ligma, ligma->be_verbose, initial_monitor, app);
      status_callback = splash_update;
    }

  g_signal_connect_after (ligma, "initialize",
                          G_CALLBACK (gui_initialize_after_callback),
                          NULL);

  g_signal_connect (ligma, "restore",
                    G_CALLBACK (gui_restore_callback),
                    NULL);
  g_signal_connect_after (ligma, "restore",
                          G_CALLBACK (gui_restore_after_callback),
                          NULL);

  g_signal_connect (ligma, "exit",
                    G_CALLBACK (gui_exit_callback),
                    NULL);
  g_signal_connect_after (ligma, "exit",
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

  dialog = ligma_dialog_new (_("Image Recovery"), "ligma-recovery",
                            NULL, GTK_DIALOG_MODAL, NULL, NULL,
                            _("_Discard"), GTK_RESPONSE_CANCEL,
                            _("_Recover"), GTK_RESPONSE_OK,
                            NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                   GTK_RESPONSE_OK);

  box = ligma_message_box_new (LIGMA_ICON_WILBER_EEK);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  ligma_message_box_set_primary_text (LIGMA_MESSAGE_BOX (box),
                                     _("Eeek! It looks like LIGMA recovered from a crash!"));

  ligma_message_box_set_text (LIGMA_MESSAGE_BOX (box),
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

  recover = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK);
  gtk_widget_destroy (dialog);

  return recover;
}

GdkMonitor *
gui_get_initial_monitor (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), 0);

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
         "LIGMA requires GTK version %d.%d.%d or later.\n"
         "Installed GTK version is %d.%d.%d.\n\n"
         "Somehow you or your software packager managed\n"
         "to install LIGMA with an older GTK version.\n\n"
         "Please upgrade to GTK version %d.%d.%d or later.",
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
  g_return_if_fail (LIGMA_IS_LIGMA (the_gui_ligma));

  ligma_help (the_gui_ligma, NULL, NULL, help_id);
}

static gboolean
gui_get_foreground_func (LigmaRGB *color)
{
  g_return_val_if_fail (color != NULL, FALSE);
  g_return_val_if_fail (LIGMA_IS_LIGMA (the_gui_ligma), FALSE);

  ligma_context_get_foreground (ligma_get_user_context (the_gui_ligma), color);

  return TRUE;
}

static gboolean
gui_get_background_func (LigmaRGB *color)
{
  g_return_val_if_fail (color != NULL, FALSE);
  g_return_val_if_fail (LIGMA_IS_LIGMA (the_gui_ligma), FALSE);

  ligma_context_get_background (ligma_get_user_context (the_gui_ligma), color);

  return TRUE;
}

static void
gui_initialize_after_callback (Ligma               *ligma,
                               LigmaInitStatusFunc  status_callback)
{
  const gchar *name = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (ligma->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

#if defined (GDK_WINDOWING_X11)
  name = "DISPLAY";
#elif defined (GDK_WINDOWING_DIRECTFB) || defined (GDK_WINDOWING_FB)
  name = "GDK_DISPLAY";
#endif

  /* TODO: Need to care about display migration with GTK+ 2.2 at some point */

  if (name)
    {
      const gchar *display = gdk_display_get_name (gdk_display_get_default ());

      ligma_environ_table_add (ligma->plug_in_manager->environ_table,
                              name, display, NULL);
    }

  ligma_tools_init (ligma);

  ligma_context_set_tool (ligma_get_user_context (ligma),
                         ligma_tool_info_get_standard (ligma));
}

static void
gui_restore_callback (Ligma               *ligma,
                      LigmaInitStatusFunc  status_callback)
{
  LigmaDisplayConfig *display_config = LIGMA_DISPLAY_CONFIG (ligma->config);
  LigmaGuiConfig     *gui_config     = LIGMA_GUI_CONFIG (ligma->config);

  if (ligma->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  gui_vtable_init (ligma);

  ligma_dialogs_show_help_button (gui_config->use_help &&
                                 gui_config->show_help_button);

  g_signal_connect (gui_config, "notify::use-help",
                    G_CALLBACK (gui_show_help_button_notify),
                    ligma);
  g_signal_connect (gui_config, "notify::user-manual-online",
                    G_CALLBACK (gui_user_manual_notify),
                    ligma);
  g_signal_connect (gui_config, "notify::show-help-button",
                    G_CALLBACK (gui_show_help_button_notify),
                    ligma);

  g_signal_connect (ligma_get_user_context (ligma), "display-changed",
                    G_CALLBACK (gui_display_changed),
                    ligma);

  /* make sure the monitor resolution is valid */
  if (display_config->monitor_res_from_gdk               ||
      display_config->monitor_xres < LIGMA_MIN_RESOLUTION ||
      display_config->monitor_yres < LIGMA_MIN_RESOLUTION)
    {
      gdouble xres, yres;

      ligma_get_monitor_resolution (initial_monitor, &xres, &yres);

      g_object_set (ligma->config,
                    "monitor-xresolution",                      xres,
                    "monitor-yresolution",                      yres,
                    "monitor-resolution-from-windowing-system", TRUE,
                    NULL);
    }

  actions_init (ligma);
  menus_init (ligma, global_action_factory);
  ligma_render_init (ligma);

  dialogs_init (ligma, global_menu_factory);

  ligma_clipboard_init (ligma);
  if (ligma_get_clipboard_image (ligma))
    ligma_clipboard_set_image (ligma, ligma_get_clipboard_image (ligma));
  else
    ligma_clipboard_set_buffer (ligma, ligma_get_clipboard_buffer (ligma));

  g_signal_connect (ligma, "clipboard-changed",
                    G_CALLBACK (gui_clipboard_changed),
                    NULL);

  ligma_devices_init (ligma);
  ligma_controllers_init (ligma);
  modifiers_init (ligma);
  session_init (ligma);

  g_type_class_unref (g_type_class_ref (LIGMA_TYPE_COLOR_SELECTOR_PALETTE));

  status_callback (NULL, _("Tool Options"), 1.0);
  ligma_tools_restore (ligma);
}

#ifdef GDK_WINDOWING_QUARTZ
static void
gui_add_to_app_menu (LigmaUIManager     *ui_manager,
                     GtkosxApplication *osx_app,
                     const gchar       *action_path,
                     gint               index)
{
  GtkWidget *item;

  item = ligma_ui_manager_get_widget (ui_manager, action_path);

  if (GTK_IS_MENU_ITEM (item))
    gtkosx_application_insert_app_menu_item (osx_app, GTK_WIDGET (item), index);
}

static gboolean
gui_quartz_quit_callback (GtkosxApplication *osx_app,
                          LigmaUIManager     *ui_manager)
{
  ligma_ui_manager_activate_action (ui_manager, "file", "file-quit");

  return TRUE;
}
#endif

static void
gui_restore_after_callback (Ligma               *ligma,
                            LigmaInitStatusFunc  status_callback)
{
  LigmaGuiConfig *gui_config = LIGMA_GUI_CONFIG (ligma->config);
  LigmaDisplay   *display;

  if (ligma->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  ligma->message_handler = LIGMA_MESSAGE_BOX;

  /*  load the recent documents after ligma_real_restore() because we
   *  need the mime-types implemented by plug-ins
   */
  status_callback (NULL, _("Documents"), 0.9);
  ligma_recent_list_load (ligma);

  /*  enable this to always have icons everywhere  */
  if (g_getenv ("LIGMA_ICONS_LIKE_A_BOSS"))
    {
      GdkScreen *screen = gdk_screen_get_default ();

      g_object_set (G_OBJECT (gtk_settings_get_for_screen (screen)),
                    "gtk-button-images", TRUE,
                    "gtk-menu-images",   TRUE,
                    NULL);
    }

  if (gui_config->restore_accels)
    menus_restore (ligma);

  ui_configurer = g_object_new (LIGMA_TYPE_UI_CONFIGURER,
                                "ligma", ligma,
                                NULL);

  image_ui_manager = ligma_menu_factory_manager_new (global_menu_factory,
                                                    "<Image>",
                                                    ligma);
  ligma_ui_manager_update (image_ui_manager, ligma);

  /* Check that every accelerator is unique. */
  gtk_accel_map_foreach_unfiltered (NULL,
                                    gui_check_unique_accelerator);

  ligma_action_history_init (ligma);

  g_signal_connect_object (gui_config, "notify::single-window-mode",
                           G_CALLBACK (gui_single_window_mode_notify),
                           ui_configurer, 0);
  g_signal_connect (image_ui_manager, "show-tooltip",
                    G_CALLBACK (gui_menu_show_tooltip),
                    ligma);
  g_signal_connect (image_ui_manager, "hide-tooltip",
                    G_CALLBACK (gui_menu_hide_tooltip),
                    ligma);

  ligma_devices_restore (ligma);
  ligma_controllers_restore (ligma, image_ui_manager);
  modifiers_restore (ligma);

  if (status_callback == splash_update)
    splash_destroy ();

  if (ligma_get_show_gui (ligma))
    {
      LigmaDisplayShell *shell;
      GtkWidget        *toplevel;

      /*  create the empty display  */
      display = LIGMA_DISPLAY (ligma_create_display (ligma, NULL,
                                                   LIGMA_UNIT_PIXEL, 1.0,
                                                   G_OBJECT (initial_monitor)));

      shell = ligma_display_get_shell (display);

      if (gui_config->restore_session)
        session_restore (ligma, initial_monitor);

      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

#ifdef GDK_WINDOWING_QUARTZ
      {
        GtkosxApplication *osx_app;
        GtkWidget         *menu;
        GtkWidget         *item;

        [[NSUserDefaults standardUserDefaults] setObject:@"NO"
                                                  forKey:@"NSTreatUnknownArgumentsAsOpen"];

        osx_app = gtkosx_application_get ();

        menu = ligma_ui_manager_get_widget (image_ui_manager,
                                           "/image-menubar");
        /* menu should have window parent for accelerator support */
        gtk_widget_set_parent(menu, toplevel);

        if (GTK_IS_MENU_ITEM (menu))
          menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (menu));

        /* do not activate OSX menu if tests are running */
        if (! g_getenv ("LIGMA_TESTING_ABS_TOP_SRCDIR"))
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

        item = ligma_ui_manager_get_widget (image_ui_manager,
                                           "/image-menubar/File/file-quit");
        gtk_widget_hide (item);

        g_signal_connect (osx_app, "NSApplicationBlockTermination",
                          G_CALLBACK (gui_quartz_quit_callback),
                          image_ui_manager);

        gtkosx_application_ready (osx_app);
      }
#endif /* GDK_WINDOWING_QUARTZ */

      /*  move keyboard focus to the display  */
      gtk_window_present (GTK_WINDOW (toplevel));
    }

  /*  indicate that the application has finished loading  */
  gdk_notify_startup_complete ();

  /*  clear startup monitor variables  */
  initial_monitor = NULL;
}

static gboolean
gui_exit_callback (Ligma     *ligma,
                   gboolean  force)
{
  LigmaGuiConfig *gui_config = LIGMA_GUI_CONFIG (ligma->config);
  LigmaTool      *active_tool;

  if (ligma->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  if (! force && ligma_displays_dirty (ligma))
    {
      LigmaContext *context = ligma_get_user_context (ligma);
      LigmaDisplay *display = ligma_context_get_display (context);
      GdkMonitor  *monitor = ligma_get_monitor_at_pointer ();
      GtkWidget   *parent  = NULL;

      if (display)
        {
          LigmaDisplayShell *shell = ligma_display_get_shell (display);

          parent = GTK_WIDGET (ligma_display_shell_get_window (shell));
        }

      ligma_dialog_factory_dialog_raise (ligma_dialog_factory_get_singleton (),
                                        monitor, parent, "ligma-quit-dialog", -1);

      return TRUE; /* stop exit for now */
    }

  ligma->message_handler = LIGMA_CONSOLE;

  gui_unique_exit ();

  /* If any modifier is set when quitting (typically when exiting with
   * Ctrl-q for instance!), when serializing the tool options, it will
   * save any alternate value instead of the main one. Make sure that
   * any modifier is reset before saving options.
   */
  active_tool = tool_manager_get_active (ligma);
  if  (active_tool && active_tool->focus_display)
    ligma_tool_set_modifier_state  (active_tool, 0, active_tool->focus_display);

  if (gui_config->save_session_info)
    session_save (ligma, FALSE);

  if (gui_config->save_device_status)
    ligma_devices_save (ligma, FALSE);

  if (TRUE /* gui_config->save_controllers */)
    ligma_controllers_save (ligma);

  modifiers_save (ligma, FALSE);

  g_signal_handlers_disconnect_by_func (ligma_get_user_context (ligma),
                                        gui_display_changed,
                                        ligma);

  ligma_displays_delete (ligma);

  if (gui_config->save_accels)
    menus_save (ligma, FALSE);

  ligma_tools_save (ligma, gui_config->save_tool_options, FALSE);
  ligma_tools_exit (ligma);

  ligma_language_store_parser_clean ();

  return FALSE; /* continue exiting */
}

static gboolean
gui_exit_after_callback (Ligma     *ligma,
                         gboolean  force)
{
  if (ligma->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  g_signal_handlers_disconnect_by_func (ligma->config,
                                        gui_show_help_button_notify,
                                        ligma);
  g_signal_handlers_disconnect_by_func (ligma->config,
                                        gui_user_manual_notify,
                                        ligma);

  ligma_action_history_exit (ligma);

  g_object_unref (image_ui_manager);
  image_ui_manager = NULL;

  g_object_unref (ui_configurer);
  ui_configurer = NULL;

  /*  exit the clipboard before shutting down the GUI because it runs
   *  a whole lot of code paths. See bug #731389.
   */
  g_signal_handlers_disconnect_by_func (ligma,
                                        G_CALLBACK (gui_clipboard_changed),
                                        NULL);
  ligma_clipboard_exit (ligma);

  session_exit (ligma);
  menus_exit (ligma);
  actions_exit (ligma);
  ligma_render_exit (ligma);

  ligma_controllers_exit (ligma);
  modifiers_exit (ligma);
  ligma_devices_exit (ligma);
  dialogs_exit (ligma);
  themes_exit (ligma);

  g_type_class_unref (g_type_class_peek (LIGMA_TYPE_COLOR_SELECT));

  return FALSE; /* continue exiting */
}

static void
gui_show_help_button_notify (LigmaGuiConfig *gui_config,
                             GParamSpec    *param_spec,
                             Ligma          *ligma)
{
  ligma_dialogs_show_help_button (gui_config->use_help &&
                                 gui_config->show_help_button);
}

static void
gui_user_manual_notify (LigmaGuiConfig *gui_config,
                        GParamSpec    *param_spec,
                        Ligma          *ligma)
{
  ligma_help_user_manual_changed (ligma);
}

static void
gui_single_window_mode_notify (LigmaGuiConfig      *gui_config,
                               GParamSpec         *pspec,
                               LigmaUIConfigurer   *ui_configurer)
{
  ligma_ui_configurer_configure (ui_configurer,
                                gui_config->single_window_mode);
}

static void
gui_clipboard_changed (Ligma *ligma)
{
  if (ligma_get_clipboard_image (ligma))
    ligma_clipboard_set_image (ligma, ligma_get_clipboard_image (ligma));
  else
    ligma_clipboard_set_buffer (ligma, ligma_get_clipboard_buffer (ligma));
}

static void
gui_menu_show_tooltip (LigmaUIManager *manager,
                       const gchar   *tooltip,
                       Ligma          *ligma)
{
  LigmaContext *context = ligma_get_user_context (ligma);
  LigmaDisplay *display = ligma_context_get_display (context);

  if (display)
    {
      LigmaDisplayShell *shell     = ligma_display_get_shell (display);
      LigmaStatusbar    *statusbar = ligma_display_shell_get_statusbar (shell);

      ligma_statusbar_push (statusbar, "menu-tooltip",
                           NULL, "%s", tooltip);
    }
}

static void
gui_menu_hide_tooltip (LigmaUIManager *manager,
                       Ligma          *ligma)
{
  LigmaContext *context = ligma_get_user_context (ligma);
  LigmaDisplay *display = ligma_context_get_display (context);

  if (display)
    {
      LigmaDisplayShell *shell     = ligma_display_get_shell (display);
      LigmaStatusbar    *statusbar = ligma_display_shell_get_statusbar (shell);

      ligma_statusbar_pop (statusbar, "menu-tooltip");
    }
}

static void
gui_display_changed (LigmaContext *context,
                     LigmaDisplay *display,
                     Ligma        *ligma)
{
  if (! display)
    {
      LigmaImage *image = ligma_context_get_image (context);

      if (image)
        {
          GList *list;

          for (list = ligma_get_display_iter (ligma);
               list;
               list = g_list_next (list))
            {
              LigmaDisplay *display2 = list->data;

              if (ligma_display_get_image (display2) == image)
                {
                  ligma_context_set_display (context, display2);

                  /* stop the emission of the original signal
                   * (the emission of the recursive signal is finished)
                   */
                  g_signal_stop_emission_by_name (context, "display-changed");
                  return;
                }
            }

          ligma_context_set_image (context, NULL);
        }
    }

  ligma_ui_manager_update (image_ui_manager, display);
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
  LigmaUIManager *manager;
  gboolean       action_exists = FALSE;
  GList         *list;

  manager = ligma_ui_managers_from_name ("<Image>")->data;

  for (list = ligma_ui_manager_get_action_groups (manager);
       list;
       list = g_list_next (list))
    {
      LigmaActionGroup *group   = list->data;
      GList           *actions = NULL;
      GList           *list2;

      actions = ligma_action_group_list_actions (group);

      for (list2 = actions; list2; list2 = g_list_next (list2))
        {
          LigmaAction  *action = list2->data;
          const gchar *path   = ligma_action_get_accel_path (action);

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
