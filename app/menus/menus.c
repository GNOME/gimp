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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#ifdef PLATFORM_OSX
#include <AppKit/AppKit.h>
#endif

#include "libgimpbase/gimpbase.h"

#include "menus-types.h"

#include "actions/actions.h"

#include "config/gimpconfig-file.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpactionfactory.h"
#include "widgets/gimpdashboard.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpuimanager.h"

#include "dockable-menu.h"
#include "image-menu.h"
#include "menus.h"
#include "plug-in-menus.h"
#include "shortcuts-rc.h"
#include "tool-options-menu.h"

#include "gimp-intl.h"


/*  local function prototypes  */


/*  private variables  */

static gboolean menurc_deleted = FALSE;

#ifdef PLATFORM_OSX
static Gimp    *unique_gimp    = NULL;
#endif


/*  public functions  */

void
menus_init (Gimp *gimp)
{
  GimpMenuFactory *global_menu_factory = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /* We need to make sure the property is installed before using it */
  g_type_class_ref (GTK_TYPE_MENU);

  global_menu_factory = menus_get_global_menu_factory (gimp);

  gimp_menu_factory_manager_register (global_menu_factory, "<Image>",
                                      "file",
                                      "context",
                                      "debug",
                                      "help",
                                      "edit",
                                      "select",
                                      "view",
                                      "image",
                                      "drawable",
                                      "layers",
                                      "channels",
                                      "paths",
                                      "tools",
                                      "dialogs",
                                      "windows",
                                      "plug-in",
                                      "filters",
                                      "quick-mask",
                                      NULL,
                                      "/image-menubar",
                                      "image-menu", image_menu_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<QuickMask>",
                                      "quick-mask",
                                      NULL,
                                      "/quick-mask-popup",
                                      "quick-mask-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<AppMenu>",
                                      "file",
                                      "dialogs",
                                      NULL,
                                      "/app-menu",
                                      "app-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Toolbox>",
                                      "file",
                                      "context",
                                      "help",
                                      "edit",
                                      "select",
                                      "view",
                                      "image",
                                      "drawable",
                                      "layers",
                                      "channels",
                                      "paths",
                                      "tools",
                                      "windows",
                                      "dialogs",
                                      "plug-in",
                                      "filters",
                                      "quick-mask",
                                      NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Dock>",
                                      "file",
                                      "context",
                                      "edit",
                                      "select",
                                      "view",
                                      "image",
                                      "drawable",
                                      "layers",
                                      "channels",
                                      "paths",
                                      "tools",
                                      "windows",
                                      "dialogs",
                                      "plug-in",
                                      "quick-mask",
                                      "dock",
                                      NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Layers>",
                                      "layers",
                                      "plug-in",
                                      "filters",
                                      NULL,
                                      "/layers-popup",
                                      "layers-menu", plug_in_menus_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Channels>",
                                      "channels",
                                      "plug-in",
                                      "filters",
                                      NULL,
                                      "/channels-popup",
                                      "channels-menu", plug_in_menus_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Paths>",
                                      "paths",
                                      "plug-in",
                                      NULL,
                                      "/paths-popup",
                                      "paths-menu", plug_in_menus_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<ToolPath>",
                                      "tool-path",
                                      NULL,
                                      "/tool-path-popup",
                                      "tool-path-menu",
                                      NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Colormap>",
                                      "colormap",
                                      "plug-in",
                                      NULL,
                                      "/colormap-popup",
                                      "colormap-menu", plug_in_menus_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Dockable>",
                                      "dockable",
                                      "dock",
                                      NULL,
                                      "/dockable-popup",
                                      "dockable-menu", dockable_menu_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Brushes>",
                                      "brushes",
                                      "plug-in",
                                      NULL,
                                      "/brushes-popup",
                                      "brushes-menu", plug_in_menus_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Dynamics>",
                                      "dynamics",
                                      "plug-in",
                                      NULL,
                                      "/dynamics-popup",
                                      "dynamics-menu", plug_in_menus_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<MyPaintBrushes>",
                                      "mypaint-brushes",
                                      "plug-in",
                                      NULL,
                                      "/mypaint-brushes-popup",
                                      "mypaint-brushes-menu", plug_in_menus_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Patterns>",
                                      "patterns",
                                      "plug-in",
                                      NULL,
                                      "/patterns-popup",
                                      "patterns-menu", plug_in_menus_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Gradients>",
                                      "gradients",
                                      "plug-in",
                                      NULL,
                                      "/gradients-popup",
                                      "gradients-menu", plug_in_menus_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Palettes>",
                                      "palettes",
                                      "plug-in",
                                      NULL,
                                      "/palettes-popup",
                                      "palettes-menu", plug_in_menus_setup,
                                      NULL);


  gimp_menu_factory_manager_register (global_menu_factory, "<ToolPresets>",
                                      "tool-presets",
                                      "plug-in",
                                      NULL,
                                      "/tool-presets-popup",
                                      "tool-presets-menu", plug_in_menus_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Fonts>",
                                      "fonts",
                                      "plug-in",
                                      NULL,
                                      "/fonts-popup",
                                      "fonts-menu", plug_in_menus_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Buffers>",
                                      "buffers",
                                      "plug-in",
                                      NULL,
                                      "/buffers-popup",
                                      "buffers-menu", plug_in_menus_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Documents>",
                                      "documents",
                                      NULL,
                                      "/documents-popup",
                                      "documents-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Templates>",
                                      "templates",
                                      NULL,
                                      "/templates-popup",
                                      "templates-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Images>",
                                      "images",
                                      NULL,
                                      "/images-popup",
                                      "images-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<BrushEditor>",
                                      "brush-editor",
                                      NULL,
                                      "/brush-editor-popup",
                                      "brush-editor-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<DynamicsEditor>",
                                      "dynamics-editor",
                                      NULL,
                                      "/dynamics-editor-popup",
                                      "dynamics-editor-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<GradientEditor>",
                                      "gradient-editor",
                                      NULL,
                                      "/gradient-editor-popup",
                                      "gradient-editor-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<PaletteEditor>",
                                      "palette-editor",
                                      NULL,
                                      "/palette-editor-popup",
                                      "palette-editor-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<ToolPresetEditor>",
                                      "tool-preset-editor",
                                      NULL,
                                      "/tool-preset-editor-popup",
                                      "tool-preset-editor-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Selection>",
                                      "select",
                                      "paths",
                                      NULL,
                                      "/selection-popup",
                                      "selection-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<NavigationEditor>",
                                      "view",
                                      NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Undo>",
                                      "edit",
                                      NULL,
                                      "/undo-popup",
                                      "undo-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<ErrorConsole>",
                                      "error-console",
                                      NULL,
                                      "/error-console-popup",
                                      "error-console-menu", NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<ToolOptions>",
                                      "tool-options",
                                      NULL,
                                      "/tool-options-popup",
                                      "tool-options-menu",
                                      tool_options_menu_setup,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<TextEditor>",
                                      "text-editor",
                                      NULL,
                                      "/text-editor-toolbar",
                                      "text-editor-toolbar",
                                      NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<TextTool>",
                                      "text-tool",
                                      NULL,
                                      "/text-tool-popup",
                                      "text-tool-menu",
                                      NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<CursorInfo>",
                                      "cursor-info",
                                      NULL,
                                      "/cursor-info-popup",
                                      "cursor-info-menu",
                                      NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<SamplePoints>",
                                      "sample-points",
                                      NULL,
                                      "/sample-points-popup",
                                      "sample-points-menu",
                                      NULL,
                                      NULL);

  gimp_menu_factory_manager_register (global_menu_factory, "<Dashboard>",
                                      "dashboard",
                                      NULL,
                                      "/dashboard-popup",
                                      "dashboard-menu", gimp_dashboard_menu_setup,
                                       NULL);
}

void
menus_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (menus_get_global_menu_factory (gimp) != NULL);

  g_object_unref (menus_get_global_menu_factory (gimp));
}

void
menus_restore (Gimp *gimp)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  file = gimp_directory_file ("shortcutsrc", NULL);

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  if (g_file_query_exists (file, NULL) &&
      ! shortcuts_rc_parse (GTK_APPLICATION (gimp->app), file, &error))
    g_printerr ("Failed reading '%s': %s\n",
                g_file_peek_path (file), error->message);

  g_object_unref (file);
}

void
menus_save (Gimp     *gimp,
            gboolean  always_save)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (menurc_deleted && ! always_save)
    return;

  file = gimp_directory_file ("shortcutsrc", NULL);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  if (! shortcuts_rc_write (GTK_APPLICATION (gimp->app), file, &error))
    g_printerr ("Failed writing to '%s': %s\n",
                g_file_peek_path (file), error->message);

  g_object_unref (file);
  g_clear_error (&error);

  menurc_deleted = FALSE;
}

gboolean
menus_clear (Gimp    *gimp,
             GError **error)
{
  GFile    *file;
  gboolean  success  = TRUE;
  GError   *my_error = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = gimp_directory_file ("shortcutsrc", NULL);

  if (! g_file_delete (file, NULL, &my_error) &&
      my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      g_set_error (error, my_error->domain, my_error->code,
                   _("Deleting \"%s\" failed: %s"),
                   gimp_file_get_utf8_name (file), my_error->message);
      success = FALSE;
    }
  else
    {
      menurc_deleted = TRUE;
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}

void
menus_remove (Gimp *gimp)
{
  gchar **actions;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  actions = g_action_group_list_actions (G_ACTION_GROUP (gimp->app));
  for (gint i = 0; actions[i] != NULL; i++)
    {
      GimpAction *action;

      action = (GimpAction *) g_action_map_lookup_action (G_ACTION_MAP (gimp->app),
                                                          actions[i]);
      gimp_action_set_accels (action, (const gchar *[]) { NULL });
    }
  g_strfreev (actions);
}

GimpMenuFactory *
menus_get_global_menu_factory (Gimp *gimp)
{
  static GimpMenuFactory *global_menu_factory = NULL;
  static gboolean         created             = FALSE;

  if (global_menu_factory == NULL && ! created)
    {
      g_set_weak_pointer (&global_menu_factory,
                          gimp_menu_factory_new (gimp, global_action_factory));

      created = TRUE;
    }

  return global_menu_factory;
}

GimpUIManager *
menus_get_image_manager_singleton (Gimp *gimp)
{
  static GimpUIManager *image_ui_manager = NULL;

  if (image_ui_manager == NULL)
    {
      image_ui_manager = gimp_menu_factory_get_manager (menus_get_global_menu_factory (gimp),
                                                        "<Image>", gimp);
      image_ui_manager->store_action_paths = TRUE;
    }

  return image_ui_manager;
}

#ifdef PLATFORM_OSX
@interface GimpappMenuHandler : NSObject
@end

@implementation GimpappMenuHandler
+ (void) gimpShowAbout:(id) sender
{
  GimpUIManager *ui_manager = menus_get_image_manager_singleton (unique_gimp);

  gimp_ui_manager_activate_action (ui_manager, "dialogs", "dialogs-about");
}

+ (void) gimpShowWelcomeDialog:(id) sender
{
  GimpUIManager *ui_manager = menus_get_image_manager_singleton (unique_gimp);

  gimp_ui_manager_activate_action (ui_manager, "dialogs", "dialogs-welcome");
}

+ (void) gimpShowPreferences:(id) sender
{
  GimpUIManager *ui_manager = menus_get_image_manager_singleton (unique_gimp);

  gimp_ui_manager_activate_action (ui_manager, "dialogs", "dialogs-preferences");
}

+ (void) gimpShowInputDevices:(id) sender
{
  GimpUIManager *ui_manager = menus_get_image_manager_singleton (unique_gimp);

  gimp_ui_manager_activate_action (ui_manager, "dialogs", "dialogs-input-devices");
}

+ (void) gimpShowKeyboardShortcuts:(id) sender
{
  GimpUIManager *ui_manager = menus_get_image_manager_singleton (unique_gimp);

  gimp_ui_manager_activate_action (ui_manager, "dialogs", "dialogs-keyboard-shortcuts");
}

+ (void) gimpQuit:(id) sender
{
  GimpUIManager *ui_manager = menus_get_image_manager_singleton (unique_gimp);

  gimp_ui_manager_activate_action (ui_manager, "file", "file-quit");
}
@end

void
menus_quartz_app_menu (Gimp *gimp)
{
  NSMenu     *main_menu;
  NSMenuItem *app_menu_item;
  NSMenu     *app_menu;
  NSInteger   last_index;
  NSMenuItem *item;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  unique_gimp = gimp;

  main_menu     = [NSApp mainMenu];
  app_menu_item = [main_menu itemAtIndex:0];
  app_menu      = [app_menu_item submenu];

  /* On macOS, some standard menu items (e.g. "Hide", "Hide Others", "Show All", "Quit")
   * are automatically provided by the system rather than created by our application.
   * For the items we need to customize, we override their default behavior with our own
   * implementations. In addition, we extend the menu with extra entries specific to
   * our application’s functionality. */

  [app_menu setTitle:@"GIMP"];

  /* About */
  item = [app_menu itemAtIndex:0];
  [item setTarget:[GimpappMenuHandler class]];
  [item setAction:@selector (gimpShowAbout:)];

  /* Welcome Dialog */
  item = [[NSMenuItem alloc] initWithTitle:@"Welcome Dialog"
                                    action:@selector (gimpShowWelcomeDialog:)
                             keyEquivalent:@""];
  [item setTarget:[GimpappMenuHandler class]];
  [app_menu insertItem:item atIndex:1];

  /* Settings */
  item = [app_menu itemAtIndex:3];
  [item setTarget:[GimpappMenuHandler class]];
  [item setAction:@selector (gimpShowPreferences:)];

  /* Input Devices */
  item = [[NSMenuItem alloc] initWithTitle:@"Input Devices"
                                    action:@selector (gimpShowInputDevices:)
                             keyEquivalent:@""];
  [item setTarget:[GimpappMenuHandler class]];
  [app_menu insertItem:item atIndex:4];

  /* Keyboard Shortcuts */
  item = [[NSMenuItem alloc] initWithTitle:@"Keyboard Shortcuts"
                                    action:@selector (gimpShowKeyboardShortcuts:)
                             keyEquivalent:@""];
  [item setTarget:[GimpappMenuHandler class]];
  [app_menu insertItem:item atIndex:5];

  /* Quit */
  last_index = [app_menu numberOfItems] - 1;
  item       = [app_menu itemAtIndex:last_index];
  [item setTarget:[GimpappMenuHandler class]];
  [item setAction:@selector (gimpQuit:)];
}
#endif
