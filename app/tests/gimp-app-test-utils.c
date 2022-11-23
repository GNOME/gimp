/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#include "display/display-types.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmaimagewindow.h"

#include "widgets/ligmauimanager.h"
#include "widgets/ligmadialogfactory.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmalayer-new.h"

#include "tests.h"

#include "ligma-app-test-utils.h"

#ifdef G_OS_WIN32
/* SendInput() requirement is Windows 2000 pro or over.
 * We may need to set WINVER to make sure the compiler does not try to
 * compile for on older version of win32, thus breaking the build.
 * See
 * http://msdn.microsoft.com/en-us/library/aa383745%28v=vs.85%29.aspx#setting_winver_or__win32_winnt
 */
#define WINVER 0x0500
#include <windows.h>
#endif /* G_OS_WIN32 */

#ifdef GDK_WINDOWING_QUARTZ
// only to get keycode definitions from HIToolbox/Events.h
#include <Carbon/Carbon.h>
#include <Cocoa/Cocoa.h>
#endif /* GDK_WINDOWING_QUARTZ */

void
ligma_test_utils_set_env_to_subdir (const gchar *root_env_var,
                                   const gchar *subdir,
                                   const gchar *target_env_var)
{
  const gchar *root_dir   = NULL;
  gchar       *target_dir = NULL;

  /* Get root dir */
  root_dir = g_getenv (root_env_var);
  if (! root_dir)
    g_printerr ("*\n"
                "*  The env var %s is not set, you are probably running\n"
                "*  in a debugger. Set it manually, e.g.:\n"
                "*\n"
                "*    set env %s=%s/source/ligma\n"
                "*\n",
                root_env_var,
                root_env_var, g_get_home_dir ());

  /* Construct path and setup target env var */
  target_dir = g_build_filename (root_dir, subdir, NULL);
  g_setenv (target_env_var, target_dir, TRUE);
  g_free (target_dir);
}

void
ligma_test_utils_set_env_to_subpath (const gchar *root_env_var1,
                                    const gchar *root_env_var2,
                                    const gchar *subdir,
                                    const gchar *target_env_var)
{
  const gchar *root_dir1   = NULL;
  const gchar *root_dir2   = NULL;
  gchar       *target_dir1 = NULL;
  gchar       *target_dir2 = NULL;
  gchar       *target_path = NULL;

  /* Get root dir */
  root_dir1 = g_getenv (root_env_var1);
  if (! root_dir1)
    g_printerr ("*\n"
                "*  The env var %s is not set, you are probably running\n"
                "*  in a debugger. Set it manually, e.g.:\n"
                "*\n"
                "*    set env %s=%s/source/ligma\n"
                "*\n",
                root_env_var1,
                root_env_var1, g_get_home_dir ());

  root_dir2 = g_getenv (root_env_var2);
  if (! root_dir2)
    g_printerr ("*\n"
                "*  The env var %s is not set, you are probably running\n"
                "*  in a debugger. Set it manually, e.g.:\n"
                "*\n"
                "*    set env %s=%s/source/ligma\n"
                "*\n",
                root_env_var2,
                root_env_var2, g_get_home_dir ());

  /* Construct path and setup target env var */
  target_dir1 = g_build_filename (root_dir1, subdir, NULL);
  target_dir2 = g_build_filename (root_dir2, subdir, NULL);

  target_path = g_strconcat (target_dir1, G_SEARCHPATH_SEPARATOR_S,
                             target_dir2, NULL);

  g_free (target_dir1);
  g_free (target_dir2);

  g_setenv (target_env_var, target_path, TRUE);
  g_free (target_path);
}


/**
 * ligma_test_utils_set_ligma3_directory:
 * @root_env_var: Either "LIGMA_TESTING_ABS_TOP_SRCDIR" or
 *                "LIGMA_TESTING_ABS_TOP_BUILDDIR"
 * @subdir:       Subdir, may be %NULL
 *
 * Sets LIGMA3_DIRECTORY to the source dir @root_env_var/@subdir. The
 * environment variables is set up by the test runner, see Makefile.am
 **/
void
ligma_test_utils_set_ligma3_directory (const gchar *root_env_var,
                                     const gchar *subdir)
{
  ligma_test_utils_set_env_to_subdir (root_env_var,
                                     subdir,
                                     "LIGMA3_DIRECTORY" /*target_env_var*/);
}

/**
 * ligma_test_utils_setup_menus_path:
 *
 * Sets LIGMA_TESTING_MENUS_PATH to "$top_srcdir/menus:$top_builddir/menus".
 **/
void
ligma_test_utils_setup_menus_path (void)
{
  /* LIGMA_TESTING_ABS_TOP_SRCDIR is set by the automake test runner,
   * see Makefile.am
   */
  ligma_test_utils_set_env_to_subpath ("LIGMA_TESTING_ABS_TOP_SRCDIR",
                                      "LIGMA_TESTING_ABS_TOP_BUILDDIR",
                                      "menus",
                                      "LIGMA_TESTING_MENUS_PATH");
}

/**
 * ligma_test_utils_create_image:
 * @ligma:   A #Ligma instance.
 * @width:  Width of image (and layer)
 * @height: Height of image (and layer)
 *
 * Creates a new image of a given size with one layer of same size and
 * a display.
 *
 * Returns: The new #LigmaImage.
 **/
void
ligma_test_utils_create_image (Ligma *ligma,
                              gint  width,
                              gint  height)
{
  LigmaImage *image;
  LigmaLayer *layer;

  image = ligma_image_new (ligma, width, height,
                          LIGMA_RGB, LIGMA_PRECISION_U8_NON_LINEAR);

  layer = ligma_layer_new (image,
                          width,
                          height,
                          ligma_image_get_layer_format (image, TRUE),
                          "layer1",
                          1.0,
                          LIGMA_LAYER_MODE_NORMAL);

  ligma_image_add_layer (image,
                        layer,
                        NULL /*parent*/,
                        0 /*position*/,
                        FALSE /*push_undo*/);

  ligma_create_display (ligma,
                       image,
                       LIGMA_UNIT_PIXEL,
                       1.0 /*scale*/,
                       NULL);
}

/**
 * ligma_test_utils_synthesize_key_event:
 * @widget: Widget to target.
 * @keyval: Keyval, e.g. GDK_Return
 *
 * Simulates a keypress and release with gdk_test_simulate_key().
 **/
void
ligma_test_utils_synthesize_key_event (GtkWidget *widget,
                                      guint      keyval)
{
#if defined(GDK_WINDOWING_QUARTZ)

GdkKeymapKey *keys   = NULL;
gint          n_keys = 0;
gint          i;
CGEventRef    keyUp, keyDown;

if (gdk_keymap_get_entries_for_keyval (gdk_keymap_get_for_display (gdk_display_get_default ()), keyval, &keys, &n_keys))
  {
    /* XXX not in use yet */
    CGEventRef commandDown	=	CGEventCreateKeyboardEvent (NULL, (CGKeyCode)kVK_Command, true);
    CGEventRef commandUp	=	CGEventCreateKeyboardEvent (NULL, (CGKeyCode)kVK_Command, false);

    CGEventRef shiftDown	=	CGEventCreateKeyboardEvent (NULL, (CGKeyCode)kVK_Shift, true);
    CGEventRef shiftUp		=	CGEventCreateKeyboardEvent (NULL, (CGKeyCode)kVK_Shift, false);

    CGEventRef optionDown	=	CGEventCreateKeyboardEvent (NULL, (CGKeyCode)kVK_Option, true);
    CGEventRef optionUp		=	CGEventCreateKeyboardEvent (NULL, (CGKeyCode)kVK_Option, false);

    for (i = 0; i < n_keys; i++)
      {
        /* Option press. */
        if (keys[i].group)
          {
            CGEventPost (kCGHIDEventTap, optionDown);
          }
        /* Shift press. */
        if (keys[i].level)
          {
            CGEventPost(kCGHIDEventTap, shiftDown);
          }
        keyDown = CGEventCreateKeyboardEvent (NULL, (CGKeyCode)keys[i].keycode, true);
        keyUp = CGEventCreateKeyboardEvent (NULL, (CGKeyCode)keys[i].keycode, false);
        /* Key pressed. */
        CGEventPost (kCGHIDEventTap, keyDown);
        CFRelease (keyDown);
        usleep (100);
        /* key released */
        CGEventPost (kCGHIDEventTap, keyUp);
        CFRelease (keyUp);

        /* Shift release. */
        if (keys[i].level)
          {
            CGEventPost (kCGHIDEventTap, shiftDown);
          }

        /* Option release. */
        if (keys[i].group)
          {
            CGEventPost (kCGHIDEventTap, optionUp);
          }
        /* No need to loop for alternative keycodes. We want only one
         * key generated. */
        break;
      }
    CFRelease (commandDown);
    CFRelease (commandUp);
    CFRelease (shiftDown);
    CFRelease (shiftUp);
    CFRelease (optionDown);
    CFRelease (optionUp);
    g_free (keys);
  }
else
  {
    g_warning ("%s: no macOS key mapping found for keyval %d.", G_STRFUNC, keyval);
  }

#else /* ! GDK_WINDOWING_QUARTZ */
  gdk_test_simulate_key (gtk_widget_get_window (widget),
                         -1, -1, /*x, y*/
                         keyval,
                         0 /*modifiers*/,
                         GDK_KEY_PRESS);
  gdk_test_simulate_key (gtk_widget_get_window (widget),
                         -1, -1, /*x, y*/
                         keyval,
                         0 /*modifiers*/,
                         GDK_KEY_RELEASE);
#endif /* ! GDK_WINDOWING_QUARTZ */
}

/**
 * ligma_test_utils_get_ui_manager:
 * @ligma: The #Ligma instance.
 *
 * Returns the "best" #LigmaUIManager to use when performing
 * actions. It gives the ui manager of the empty display if it exists,
 * otherwise it gives it the ui manager of the first display.
 *
 * Returns: The #LigmaUIManager.
 **/
LigmaUIManager *
ligma_test_utils_get_ui_manager (Ligma *ligma)
{
  LigmaDisplay       *display      = NULL;
  LigmaDisplayShell  *shell        = NULL;
  GtkWidget         *toplevel     = NULL;
  LigmaImageWindow   *image_window = NULL;
  LigmaUIManager     *ui_manager   = NULL;

  display = LIGMA_DISPLAY (ligma_get_empty_display (ligma));

  /* If there were not empty display, assume that there is at least
   * one image display and use that
   */
  if (! display)
    display = LIGMA_DISPLAY (ligma_get_display_iter (ligma)->data);

  shell            = ligma_display_get_shell (display);
  toplevel         = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  image_window     = LIGMA_IMAGE_WINDOW (toplevel);
  ui_manager       = ligma_image_window_get_ui_manager (image_window);

  return ui_manager;
}

/**
 * ligma_test_utils_create_image_from_dalog:
 * @ligma:
 *
 * Creates a new image using the "New image" dialog, and then returns
 * the #LigmaImage created.
 *
 * Returns: The created #LigmaImage.
 **/
LigmaImage *
ligma_test_utils_create_image_from_dialog (Ligma *ligma)
{
  LigmaImage     *image            = NULL;
  GtkWidget     *new_image_dialog = NULL;
  guint          n_initial_images = g_list_length (ligma_get_image_iter (ligma));
  guint          n_images         = -1;
  gint           tries_left       = 100;
  LigmaUIManager *ui_manager       = ligma_test_utils_get_ui_manager (ligma);

  /* Bring up the new image dialog */
  ligma_ui_manager_activate_action (ui_manager,
                                   "image",
                                   "image-new");
  ligma_test_run_mainloop_until_idle ();

  /* Get the GtkWindow of the dialog */
  new_image_dialog =
    ligma_dialog_factory_dialog_raise (ligma_dialog_factory_get_singleton (),
                                      gdk_display_get_monitor (gdk_display_get_default (), 0),
                                      NULL,
                                      "ligma-image-new-dialog",
                                      -1 /*view_size*/);

  /* Press the OK button. It will take a while for the image to be
   * created so loop for a while
   */
  gtk_dialog_response (GTK_DIALOG (new_image_dialog), GTK_RESPONSE_OK);
  do
    {
      g_usleep (20 * 1000);
      ligma_test_run_mainloop_until_idle ();
      n_images = g_list_length (ligma_get_image_iter (ligma));
    }
  while (tries_left-- &&
         n_images != n_initial_images + 1);

  /* Make sure there now is one image more than initially */
  g_assert_cmpint (n_images,
                   ==,
                   n_initial_images + 1);

  image = LIGMA_IMAGE (ligma_get_image_iter (ligma)->data);

  return image;
}

