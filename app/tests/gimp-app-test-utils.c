/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "display/display-types.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpimagewindow.h"

#include "widgets/gimpuimanager.h"
#include "widgets/gimpdialogfactory.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayer-new.h"

#include "tests.h"

#include "gimp-app-test-utils.h"

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

void
gimp_test_utils_set_env_to_subpath (const gchar *root_env_var,
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
                "*    set env %s=%s/source/gimp\n"
                "*\n",
                root_env_var,
                root_env_var, g_get_home_dir ());

  /* Construct path and setup target env var */
  target_dir = g_build_filename (root_dir, subdir, NULL);
  g_setenv (target_env_var, target_dir, TRUE);
  g_free (target_dir);
}


/**
 * gimp_test_utils_set_gimp2_directory:
 * @root_env_var: Either "GIMP_TESTING_ABS_TOP_SRCDIR" or
 *                "GIMP_TESTING_ABS_TOP_BUILDDIR"
 * @subdir:       Subdir, may be %NULL
 *
 * Sets GIMP2_DIRECTORY to the source dir @root_env_var/@subdir. The
 * environment variables is set up by the test runner, see Makefile.am
 **/
void
gimp_test_utils_set_gimp2_directory (const gchar *root_env_var,
                                     const gchar *subdir)
{
  gimp_test_utils_set_env_to_subpath (root_env_var,
                                      subdir,
                                      "GIMP2_DIRECTORY" /*target_env_var*/);
}

/**
 * gimp_test_utils_setup_menus_dir:
 *
 * Sets GIMP_TESTING_MENUS_DIR to "$top_srcdir/menus".
 **/
void
gimp_test_utils_setup_menus_dir (void)
{
  /* GIMP_TESTING_ABS_TOP_SRCDIR is set by the automake test runner,
   * see Makefile.am
   */
  gimp_test_utils_set_env_to_subpath ("GIMP_TESTING_ABS_TOP_SRCDIR" /*root_env_var*/,
                                      "menus" /*subdir*/,
                                      "GIMP_TESTING_MENUS_DIR" /*target_env_var*/);
}

/**
 * gimp_test_utils_create_image:
 * @gimp:   A #Gimp instance.
 * @width:  Width of image (and layer)
 * @height: Height of image (and layer)
 *
 * Creates a new image of a given size with one layer of same size and
 * a display.
 *
 * Returns: The new #GimpImage.
 **/
void
gimp_test_utils_create_image (Gimp *gimp,
                              gint  width,
                              gint  height)
{
  GimpImage *image;
  GimpLayer *layer;

  image = gimp_image_new (gimp, width, height,
                          GIMP_RGB, GIMP_PRECISION_U8_GAMMA);

  layer = gimp_layer_new (image,
                          width,
                          height,
                          gimp_image_get_layer_format (image, TRUE),
                          "layer1",
                          1.0,
                          GIMP_LAYER_MODE_NORMAL);

  gimp_image_add_layer (image,
                        layer,
                        NULL /*parent*/,
                        0 /*position*/,
                        FALSE /*push_undo*/);

  gimp_create_display (gimp,
                       image,
                       GIMP_UNIT_PIXEL,
                       1.0 /*scale*/,
                       NULL);
}

/**
 * gimp_test_utils_synthesize_key_event:
 * @widget: Widget to target.
 * @keyval: Keyval, e.g. GDK_Return
 *
 * Simulates a keypress and release with gdk_test_simulate_key().
 **/
void
gimp_test_utils_synthesize_key_event (GtkWidget *widget,
                                      guint      keyval)
{
#if defined G_OS_WIN32 && ! GTK_CHECK_VERSION (2, 24, 25)
  /* gdk_test_simulate_key() has no implementation for win32 until
   * GTK+ 2.24.25.
   * TODO: remove the below hack when our GTK+ requirement is over 2.24.25. */
  GdkKeymapKey *keys   = NULL;
  gint          n_keys = 0;
  INPUT         ip;
  gint          i;

  ip.type = INPUT_KEYBOARD;
  ip.ki.wScan = 0;
  ip.ki.time = 0;
  ip.ki.dwExtraInfo = 0;
  if (gdk_keymap_get_entries_for_keyval (gdk_keymap_get_default (), keyval, &keys, &n_keys))
    {
      for (i = 0; i < n_keys; i++)
        {
          ip.ki.dwFlags = 0;
          /* AltGr press. */
          if (keys[i].group)
            {
              /* According to some virtualbox code I found, AltGr is
               * simulated on win32 with LCtrl+RAlt */
              ip.ki.wVk = VK_CONTROL;
              SendInput(1, &ip, sizeof(INPUT));
              ip.ki.wVk = VK_MENU;
              SendInput(1, &ip, sizeof(INPUT));
            }
          /* Shift press. */
          if (keys[i].level)
            {
              ip.ki.wVk = VK_SHIFT;
              SendInput(1, &ip, sizeof(INPUT));
            }
          /* Key pressed. */
          ip.ki.wVk = keys[i].keycode;
          SendInput(1, &ip, sizeof(INPUT));

          ip.ki.dwFlags = KEYEVENTF_KEYUP;
          /* Key released. */
          SendInput(1, &ip, sizeof(INPUT));
          /* Shift release. */
          if (keys[i].level)
            {
              ip.ki.wVk = VK_SHIFT;
              SendInput(1, &ip, sizeof(INPUT));
            }
          /* AltrGr release. */
          if (keys[i].group)
            {
              ip.ki.wVk = VK_MENU;
              SendInput(1, &ip, sizeof(INPUT));
              ip.ki.wVk = VK_CONTROL;
              SendInput(1, &ip, sizeof(INPUT));
            }
          /* No need to loop for alternative keycodes. We want only one
           * key generated. */
          break;
        }
      g_free (keys);
    }
  else
    {
      g_warning ("%s: no win32 key mapping found for keyval %d.", G_STRFUNC, keyval);
    }
#else /* G_OS_WIN32  && ! GTK_CHECK_VERSION (2, 24, 25) */
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
#endif /* G_OS_WIN32  && ! GTK_CHECK_VERSION (2, 24, 25) */
}

/**
 * gimp_test_utils_get_ui_manager:
 * @gimp: The #Gimp instance.
 *
 * Returns the "best" #GimpUIManager to use when performing
 * actions. It gives the ui manager of the empty display if it exists,
 * otherwise it gives it the ui manager of the first display.
 *
 * Returns: The #GimpUIManager.
 **/
GimpUIManager *
gimp_test_utils_get_ui_manager (Gimp *gimp)
{
  GimpDisplay       *display      = NULL;
  GimpDisplayShell  *shell        = NULL;
  GtkWidget         *toplevel     = NULL;
  GimpImageWindow   *image_window = NULL;
  GimpUIManager     *ui_manager   = NULL;

  display = GIMP_DISPLAY (gimp_get_empty_display (gimp));

  /* If there were not empty display, assume that there is at least
   * one image display and use that
   */
  if (! display)
    display = GIMP_DISPLAY (gimp_get_display_iter (gimp)->data);

  shell            = gimp_display_get_shell (display);
  toplevel         = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  image_window     = GIMP_IMAGE_WINDOW (toplevel);
  ui_manager       = gimp_image_window_get_ui_manager (image_window);

  return ui_manager;
}

/**
 * gimp_test_utils_create_image_from_dalog:
 * @gimp:
 *
 * Creates a new image using the "New image" dialog, and then returns
 * the #GimpImage created.
 *
 * Returns: The created #GimpImage.
 **/
GimpImage *
gimp_test_utils_create_image_from_dialog (Gimp *gimp)
{
  GimpImage     *image            = NULL;
  GtkWidget     *new_image_dialog = NULL;
  guint          n_initial_images = g_list_length (gimp_get_image_iter (gimp));
  guint          n_images         = -1;
  gint           tries_left       = 100;
  GimpUIManager *ui_manager       = gimp_test_utils_get_ui_manager (gimp);

  /* Bring up the new image dialog */
  gimp_ui_manager_activate_action (ui_manager,
                                   "image",
                                   "image-new");
  gimp_test_run_mainloop_until_idle ();

  /* Get the GtkWindow of the dialog */
  new_image_dialog =
    gimp_dialog_factory_dialog_raise (gimp_dialog_factory_get_singleton (),
                                      gdk_display_get_monitor (gdk_display_get_default (), 0),
                                      NULL,
                                      "gimp-image-new-dialog",
                                      -1 /*view_size*/);

  /* Press the focused widget, it should be the Ok button. It will
   * take a while for the image to be created so loop for a while
   */
  gtk_widget_activate (gtk_window_get_focus (GTK_WINDOW (new_image_dialog)));
  do
    {
      g_usleep (20 * 1000);
      gimp_test_run_mainloop_until_idle ();
      n_images = g_list_length (gimp_get_image_iter (gimp));
    }
  while (tries_left-- &&
         n_images != n_initial_images + 1);

  /* Make sure there now is one image more than initially */
  g_assert_cmpint (n_images,
                   ==,
                   n_initial_images + 1);

  image = GIMP_IMAGE (gimp_get_image_iter (gimp)->data);

  return image;
}

