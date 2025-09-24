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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools/tools-types.h"

#include "tools/gimprectangleoptions.h"
#include "tools/tool_manager.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-callbacks.h"
#include "display/gimpdisplayshell-scale.h"
#include "display/gimpdisplayshell-tool-events.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimpimagewindow.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdocked.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimptoolbox.h"
#include "widgets/gimptooloptionseditor.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "gimpcoreapp.h"

#include "gimp-app-test-utils.h"
#include "tests.h"


#define GIMP_TEST_IMAGE_WIDTH            150
#define GIMP_TEST_IMAGE_HEIGHT           267

/* Put this in the code below when you want the test to pause so you
 * can do measurements of widgets on the screen for example
 */
#define GIMP_PAUSE (g_usleep (2 * 1000 * 1000))

#define ADD_TEST(function) \
  g_test_add ("/gimp-tools/" #function, \
              GimpTestFixture, \
              gimp, \
              gimp_tools_setup_image, \
              function, \
              gimp_tools_teardown_image);


typedef struct
{
  int avoid_sizeof_zero;
} GimpTestFixture;


static void               gimp_tools_setup_image                         (GimpTestFixture  *fixture,
                                                                          gconstpointer     data);
static void               gimp_tools_teardown_image                      (GimpTestFixture  *fixture,
                                                                          gconstpointer     data);
static void               gimp_tools_synthesize_image_click_drag_release (GimpDisplayShell *shell,
                                                                          gdouble           start_image_x,
                                                                          gdouble           start_image_y,
                                                                          gdouble           end_image_x,
                                                                          gdouble           end_image_y,
                                                                          gint              button,
                                                                          GdkModifierType   modifiers);
static GimpDisplay      * gimp_test_get_only_display                     (Gimp             *gimp);
static GimpImage        * gimp_test_get_only_image                       (Gimp             *gimp);
static GimpDisplayShell * gimp_test_get_only_display_shell               (Gimp             *gimp);


static void
gimp_tools_setup_image (GimpTestFixture *fixture,
                        gconstpointer    data)
{
  Gimp *gimp = GIMP (data);

  gimp_test_utils_create_image (gimp,
                                GIMP_TEST_IMAGE_WIDTH,
                                GIMP_TEST_IMAGE_HEIGHT);
  gimp_test_run_mainloop_until_idle ();
}

static void
gimp_tools_teardown_image (GimpTestFixture *fixture,
                           gconstpointer    data)
{
  Gimp *gimp = GIMP (data);

  g_object_unref (gimp_test_get_only_image (gimp));
  gimp_display_close (gimp_test_get_only_display (gimp));
  gimp_test_run_mainloop_until_idle ();
}

/**
 * gimp_tools_set_tool:
 * @gimp:
 * @tool_id:
 * @display:
 *
 * Makes sure the given tool is the active tool and that the passed
 * display is the focused tool display.
 **/
static void
gimp_tools_set_tool (Gimp        *gimp,
                     const gchar *tool_id,
                     GimpDisplay *display)
{
  /* Activate tool and setup active display for the new tool */
  gimp_context_set_tool (gimp_get_user_context (gimp),
                         gimp_get_tool_info (gimp, tool_id));
  tool_manager_focus_display_active (gimp, display);
}

/**
 * gimp_test_get_only_display:
 * @gimp:
 *
 * Asserts that there only is one image and display and then
 * returns the display.
 *
 * Returns: The #GimpDisplay.
 **/
static GimpDisplay *
gimp_test_get_only_display (Gimp *gimp)
{
  g_assert_true (g_list_length (gimp_get_image_iter (gimp)) == 1);
  g_assert_true (g_list_length (gimp_get_display_iter (gimp)) == 1);

  return GIMP_DISPLAY (gimp_get_display_iter (gimp)->data);
}

/**
 * gimp_test_get_only_display_shell:
 * @gimp:
 *
 * Asserts that there only is one image and display shell and then
 * returns the display shell.
 *
 * Returns: The #GimpDisplayShell.
 **/
static GimpDisplayShell *
gimp_test_get_only_display_shell (Gimp *gimp)
{
  return gimp_display_get_shell (gimp_test_get_only_display (gimp));
}

/**
 * gimp_test_get_only_image:
 * @gimp:
 *
 * Asserts that there is only one image and returns that.
 *
 * Returns: The #GimpImage.
 **/
static GimpImage *
gimp_test_get_only_image (Gimp *gimp)
{
  g_assert_true (g_list_length (gimp_get_image_iter (gimp)) == 1);
  g_assert_true (g_list_length (gimp_get_display_iter (gimp)) == 1);

  return GIMP_IMAGE (gimp_get_image_iter (gimp)->data);
}

static void
gimp_test_synthesize_tool_button_event (GimpDisplayShell *shell,
                                       gint              x,
                                       gint              y,
                                       gint              button,
                                       gint              modifiers,
                                       GdkEventType      button_event_type)
{
  GdkEvent   *event   = gdk_event_new (button_event_type);
  GdkWindow  *window  = gtk_widget_get_window (GTK_WIDGET (shell->canvas));
  GdkDisplay *display = gdk_window_get_display (window);
  GdkSeat    *seat    = gdk_display_get_default_seat (display);

  g_assert_true (button_event_type == GDK_BUTTON_PRESS ||
                 button_event_type == GDK_BUTTON_RELEASE);

  gdk_event_set_device (event, gdk_seat_get_pointer (seat));

  event->button.window     = g_object_ref (window);
  event->button.send_event = TRUE;
  event->button.time       = gtk_get_current_event_time ();
  event->button.x          = x;
  event->button.y          = y;
  event->button.axes       = NULL;
  event->button.state      = 0;
  event->button.button     = button;
  event->button.x_root     = -1;
  event->button.y_root     = -1;

  gimp_display_shell_canvas_tool_events (shell->canvas,
                                         event,
                                         shell);
  gdk_event_free (event);
}

static void
gimp_test_synthesize_tool_motion_event (GimpDisplayShell *shell,
                                        gint              x,
                                        gint              y,
                                        gint              modifiers)
{
  GdkEvent   *event   = gdk_event_new (GDK_MOTION_NOTIFY);
  GdkWindow  *window  = gtk_widget_get_window (GTK_WIDGET (shell->canvas));
  GdkDisplay *display = gdk_window_get_display (window);
  GdkSeat    *seat    = gdk_display_get_default_seat (display);

  gdk_event_set_device (event, gdk_seat_get_pointer (seat));

  event->motion.window     = g_object_ref (window);
  event->motion.send_event = TRUE;
  event->motion.time       = gtk_get_current_event_time ();
  event->motion.x          = x;
  event->motion.y          = y;
  event->motion.axes       = NULL;
  event->motion.state      = GDK_BUTTON1_MASK | modifiers;
  event->motion.is_hint    = FALSE;
  event->motion.x_root     = -1;
  event->motion.y_root     = -1;

  gimp_display_shell_canvas_tool_events (shell->canvas,
                                         event,
                                         shell);
  gdk_event_free (event);
}

static void
gimp_test_synthesize_tool_crossing_event (GimpDisplayShell *shell,
                                          gint              x,
                                          gint              y,
                                          gint              modifiers,
                                          GdkEventType      crossing_event_type)
{
  GdkEvent   *event   = gdk_event_new (crossing_event_type);
  GdkWindow  *window  = gtk_widget_get_window (GTK_WIDGET (shell->canvas));
  GdkDisplay *display = gdk_window_get_display (window);
  GdkSeat    *seat    = gdk_display_get_default_seat (display);

  g_assert_true (crossing_event_type == GDK_ENTER_NOTIFY ||
                 crossing_event_type == GDK_LEAVE_NOTIFY);

  gdk_event_set_device (event, gdk_seat_get_pointer (seat));

  event->crossing.window     = g_object_ref (window);
  event->crossing.send_event = TRUE;
  event->crossing.subwindow  = NULL;
  event->crossing.time       = gtk_get_current_event_time ();
  event->crossing.x          = x;
  event->crossing.y          = y;
  event->crossing.x_root     = -1;
  event->crossing.y_root     = -1;
  event->crossing.mode       = GDK_CROSSING_NORMAL;
  event->crossing.detail     = GDK_NOTIFY_UNKNOWN;
  event->crossing.focus      = TRUE;
  event->crossing.state      = modifiers;

  gimp_display_shell_canvas_tool_events (shell->canvas,
                                         event,
                                         shell);
  gdk_event_free (event);
}

static void
gimp_tools_synthesize_image_click_drag_release (GimpDisplayShell *shell,
                                                gdouble           start_image_x,
                                                gdouble           start_image_y,
                                                gdouble           end_image_x,
                                                gdouble           end_image_y,
                                                gint              button /*1..3*/,
                                                GdkModifierType   modifiers)
{
  gdouble start_canvas_x  = -1.0;
  gdouble start_canvas_y  = -1.0;
  gdouble middle_canvas_x = -1.0;
  gdouble middle_canvas_y = -1.0;
  gdouble end_canvas_x    = -1.0;
  gdouble end_canvas_y    = -1.0;

  /* Transform coordinates */
  gimp_display_shell_transform_xy_f (shell,
                                     start_image_x,
                                     start_image_y,
                                     &start_canvas_x,
                                     &start_canvas_y);
  gimp_display_shell_transform_xy_f (shell,
                                     end_image_x,
                                     end_image_y,
                                     &end_canvas_x,
                                     &end_canvas_y);
  middle_canvas_x = (start_canvas_x + end_canvas_x) / 2;
  middle_canvas_y = (start_canvas_y + end_canvas_y) / 2;

  /* Enter notify */
  gimp_test_synthesize_tool_crossing_event (shell,
                                            (int)start_canvas_x,
                                            (int)start_canvas_y,
                                            modifiers,
                                            GDK_ENTER_NOTIFY);

  /* Button press */
  gimp_test_synthesize_tool_button_event (shell,
                                          (int)start_canvas_x,
                                          (int)start_canvas_y,
                                          button,
                                          modifiers,
                                          GDK_BUTTON_PRESS);

  /* Move events */
  gimp_test_synthesize_tool_motion_event (shell,
                                          (int)start_canvas_x,
                                          (int)start_canvas_y,
                                          modifiers);
  gimp_test_synthesize_tool_motion_event (shell,
                                          (int)middle_canvas_x,
                                          (int)middle_canvas_y,
                                          modifiers);
  gimp_test_synthesize_tool_motion_event (shell,
                                          (int)end_canvas_x,
                                          (int)end_canvas_y,
                                          modifiers);

  /* Button release */
  gimp_test_synthesize_tool_button_event (shell,
                                          (int)end_canvas_x,
                                          (int)end_canvas_y,
                                          button,
                                          modifiers,
                                          GDK_BUTTON_RELEASE);

  /* Leave notify */
  gimp_test_synthesize_tool_crossing_event (shell,
                                            (int)start_canvas_x,
                                            (int)start_canvas_y,
                                            modifiers,
                                            GDK_LEAVE_NOTIFY);

  /* Process them */
  gimp_test_run_mainloop_until_idle ();
}

/**
 * crop_tool_can_crop:
 * @fixture:
 * @data:
 *
 * Make sure it's possible to crop at all. Regression test for
 * "Bug 315255 - SIGSEGV, while doing a crop".
 **/
static void
crop_tool_can_crop (GimpTestFixture *fixture,
                    gconstpointer    data)
{
  Gimp             *gimp  = GIMP (data);
  GimpImage        *image = gimp_test_get_only_image (gimp);
  GimpDisplayShell *shell = gimp_test_get_only_display_shell (gimp);

  gint cropped_x = 10;
  gint cropped_y = 10;
  gint cropped_w = 20;
  gint cropped_h = 30;

  /* Fit display and pause and let it stabilize (two idlings seems to
   * always be enough)
   */
  gimp_ui_manager_activate_action (gimp_test_utils_get_ui_manager (gimp),
                                   "view",
                                   "view-shrink-wrap");
  gimp_test_run_mainloop_until_idle ();
  gimp_test_run_mainloop_until_idle ();

  /* Activate crop tool */
  gimp_tools_set_tool (gimp, "gimp-crop-tool", shell->display);

  /* Do the crop rect */
  gimp_tools_synthesize_image_click_drag_release (shell,
                                                  cropped_x,
                                                  cropped_y,
                                                  cropped_x + cropped_w,
                                                  cropped_y + cropped_h,
                                                  1 /*button*/,
                                                  0 /*modifiers*/);

  /* Crop */
  gimp_test_utils_synthesize_key_event (GTK_WIDGET (shell), GDK_KEY_Return);
  gimp_test_run_mainloop_until_idle ();

  /* Make sure the new image has the expected size */
  g_assert_cmpint (cropped_w, ==, gimp_image_get_width (image));
  g_assert_cmpint (cropped_h, ==, gimp_image_get_height (image));
}

/**
 * crop_tool_can_crop:
 * @fixture:
 * @data:
 *
 * Make sure it's possible to change width of crop rect in tool
 * options without there being a pending rectangle. Regression test
 * for "Bug 322396 - Crop dimension entering causes crash".
 **/
static void
crop_set_width_without_pending_rect (GimpTestFixture *fixture,
                                     gconstpointer    data)
{
  Gimp                 *gimp    = GIMP (data);
  GimpDisplay          *display = gimp_test_get_only_display (gimp);
  GimpToolInfo         *tool_info;
  GimpRectangleOptions *rectangle_options;
  GtkWidget            *size_entry;

  /* Activate crop tool */
  gimp_tools_set_tool (gimp, "gimp-crop-tool", display);

  /* Get tool options */
  tool_info         = gimp_get_tool_info (gimp, "gimp-crop-tool");
  rectangle_options = GIMP_RECTANGLE_OPTIONS (tool_info->tool_options);

  /* Find 'Width' or 'Height' GtkTextEntry in tool options */
  size_entry = gimp_rectangle_options_get_size_entry (rectangle_options);

  /* Set arbitrary non-0 value */
  gimp_size_entry_set_value (GIMP_SIZE_ENTRY (size_entry),
                             0 /*field*/,
                             42.0 /*lower*/);

  /* If we don't crash, everything s fine */
}

int main(int argc, char **argv)
{
  Gimp *gimp   = NULL;
  gint  result = -1;

  gimp_test_bail_if_no_display ();
  gtk_test_init (&argc, &argv, NULL);

  gimp_test_utils_setup_menus_path ();

  /* Start up GIMP */
  gimp = gimp_init_for_gui_testing (TRUE /*show_gui*/);
  gimp_test_run_mainloop_until_idle ();

  /* Add tests */
  ADD_TEST (crop_tool_can_crop);
  ADD_TEST (crop_set_width_without_pending_rect);

  /* Run the tests and return status */
  g_application_run (gimp->app, 0, NULL);
  result = gimp_core_app_get_exit_status (GIMP_CORE_APP (gimp->app));

  g_application_quit (G_APPLICATION (gimp->app));
  g_clear_object (&gimp->app);

  return result;
}
