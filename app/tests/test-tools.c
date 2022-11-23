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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools/tools-types.h"

#include "tools/ligmarectangleoptions.h"
#include "tools/tool_manager.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-callbacks.h"
#include "display/ligmadisplayshell-scale.h"
#include "display/ligmadisplayshell-tool-events.h"
#include "display/ligmadisplayshell-transform.h"
#include "display/ligmaimagewindow.h"

#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmadockable.h"
#include "widgets/ligmadockbook.h"
#include "widgets/ligmadocked.h"
#include "widgets/ligmadockwindow.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmasessioninfo.h"
#include "widgets/ligmatoolbox.h"
#include "widgets/ligmatooloptionseditor.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmatooloptions.h"

#include "tests.h"

#include "ligma-app-test-utils.h"


#define LIGMA_TEST_IMAGE_WIDTH            150
#define LIGMA_TEST_IMAGE_HEIGHT           267

/* Put this in the code below when you want the test to pause so you
 * can do measurements of widgets on the screen for example
 */
#define LIGMA_PAUSE (g_usleep (2 * 1000 * 1000))

#define ADD_TEST(function) \
  g_test_add ("/ligma-tools/" #function, \
              LigmaTestFixture, \
              ligma, \
              ligma_tools_setup_image, \
              function, \
              ligma_tools_teardown_image);


typedef struct
{
  int avoid_sizeof_zero;
} LigmaTestFixture;


static void               ligma_tools_setup_image                         (LigmaTestFixture  *fixture,
                                                                          gconstpointer     data);
static void               ligma_tools_teardown_image                      (LigmaTestFixture  *fixture,
                                                                          gconstpointer     data);
static void               ligma_tools_synthesize_image_click_drag_release (LigmaDisplayShell *shell,
                                                                          gdouble           start_image_x,
                                                                          gdouble           start_image_y,
                                                                          gdouble           end_image_x,
                                                                          gdouble           end_image_y,
                                                                          gint              button,
                                                                          GdkModifierType   modifiers);
static LigmaDisplay      * ligma_test_get_only_display                     (Ligma             *ligma);
static LigmaImage        * ligma_test_get_only_image                       (Ligma             *ligma);
static LigmaDisplayShell * ligma_test_get_only_display_shell               (Ligma             *ligma);


static void
ligma_tools_setup_image (LigmaTestFixture *fixture,
                        gconstpointer    data)
{
  Ligma *ligma = LIGMA (data);

  ligma_test_utils_create_image (ligma,
                                LIGMA_TEST_IMAGE_WIDTH,
                                LIGMA_TEST_IMAGE_HEIGHT);
  ligma_test_run_mainloop_until_idle ();
}

static void
ligma_tools_teardown_image (LigmaTestFixture *fixture,
                           gconstpointer    data)
{
  Ligma *ligma = LIGMA (data);

  g_object_unref (ligma_test_get_only_image (ligma));
  ligma_display_close (ligma_test_get_only_display (ligma));
  ligma_test_run_mainloop_until_idle ();
}

/**
 * ligma_tools_set_tool:
 * @ligma:
 * @tool_id:
 * @display:
 *
 * Makes sure the given tool is the active tool and that the passed
 * display is the focused tool display.
 **/
static void
ligma_tools_set_tool (Ligma        *ligma,
                     const gchar *tool_id,
                     LigmaDisplay *display)
{
  /* Activate tool and setup active display for the new tool */
  ligma_context_set_tool (ligma_get_user_context (ligma),
                         ligma_get_tool_info (ligma, tool_id));
  tool_manager_focus_display_active (ligma, display);
}

/**
 * ligma_test_get_only_display:
 * @ligma:
 *
 * Asserts that there only is one image and display and then
 * returns the display.
 *
 * Returns: The #LigmaDisplay.
 **/
static LigmaDisplay *
ligma_test_get_only_display (Ligma *ligma)
{
  g_assert (g_list_length (ligma_get_image_iter (ligma)) == 1);
  g_assert (g_list_length (ligma_get_display_iter (ligma)) == 1);

  return LIGMA_DISPLAY (ligma_get_display_iter (ligma)->data);
}

/**
 * ligma_test_get_only_display_shell:
 * @ligma:
 *
 * Asserts that there only is one image and display shell and then
 * returns the display shell.
 *
 * Returns: The #LigmaDisplayShell.
 **/
static LigmaDisplayShell *
ligma_test_get_only_display_shell (Ligma *ligma)
{
  return ligma_display_get_shell (ligma_test_get_only_display (ligma));
}

/**
 * ligma_test_get_only_image:
 * @ligma:
 *
 * Asserts that there is only one image and returns that.
 *
 * Returns: The #LigmaImage.
 **/
static LigmaImage *
ligma_test_get_only_image (Ligma *ligma)
{
  g_assert (g_list_length (ligma_get_image_iter (ligma)) == 1);
  g_assert (g_list_length (ligma_get_display_iter (ligma)) == 1);

  return LIGMA_IMAGE (ligma_get_image_iter (ligma)->data);
}

static void
ligma_test_synthesize_tool_button_event (LigmaDisplayShell *shell,
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

  g_assert (button_event_type == GDK_BUTTON_PRESS ||
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

  ligma_display_shell_canvas_tool_events (shell->canvas,
                                         event,
                                         shell);
  gdk_event_free (event);
}

static void
ligma_test_synthesize_tool_motion_event (LigmaDisplayShell *shell,
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

  ligma_display_shell_canvas_tool_events (shell->canvas,
                                         event,
                                         shell);
  gdk_event_free (event);
}

static void
ligma_test_synthesize_tool_crossing_event (LigmaDisplayShell *shell,
                                          gint              x,
                                          gint              y,
                                          gint              modifiers,
                                          GdkEventType      crossing_event_type)
{
  GdkEvent   *event   = gdk_event_new (crossing_event_type);
  GdkWindow  *window  = gtk_widget_get_window (GTK_WIDGET (shell->canvas));
  GdkDisplay *display = gdk_window_get_display (window);
  GdkSeat    *seat    = gdk_display_get_default_seat (display);

  g_assert (crossing_event_type == GDK_ENTER_NOTIFY ||
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

  ligma_display_shell_canvas_tool_events (shell->canvas,
                                         event,
                                         shell);
  gdk_event_free (event);
}

static void
ligma_tools_synthesize_image_click_drag_release (LigmaDisplayShell *shell,
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
  ligma_display_shell_transform_xy_f (shell,
                                     start_image_x,
                                     start_image_y,
                                     &start_canvas_x,
                                     &start_canvas_y);
  ligma_display_shell_transform_xy_f (shell,
                                     end_image_x,
                                     end_image_y,
                                     &end_canvas_x,
                                     &end_canvas_y);
  middle_canvas_x = (start_canvas_x + end_canvas_x) / 2;
  middle_canvas_y = (start_canvas_y + end_canvas_y) / 2;

  /* Enter notify */
  ligma_test_synthesize_tool_crossing_event (shell,
                                            (int)start_canvas_x,
                                            (int)start_canvas_y,
                                            modifiers,
                                            GDK_ENTER_NOTIFY);

  /* Button press */
  ligma_test_synthesize_tool_button_event (shell,
                                          (int)start_canvas_x,
                                          (int)start_canvas_y,
                                          button,
                                          modifiers,
                                          GDK_BUTTON_PRESS);

  /* Move events */
  ligma_test_synthesize_tool_motion_event (shell,
                                          (int)start_canvas_x,
                                          (int)start_canvas_y,
                                          modifiers);
  ligma_test_synthesize_tool_motion_event (shell,
                                          (int)middle_canvas_x,
                                          (int)middle_canvas_y,
                                          modifiers);
  ligma_test_synthesize_tool_motion_event (shell,
                                          (int)end_canvas_x,
                                          (int)end_canvas_y,
                                          modifiers);

  /* Button release */
  ligma_test_synthesize_tool_button_event (shell,
                                          (int)end_canvas_x,
                                          (int)end_canvas_y,
                                          button,
                                          modifiers,
                                          GDK_BUTTON_RELEASE);

  /* Leave notify */
  ligma_test_synthesize_tool_crossing_event (shell,
                                            (int)start_canvas_x,
                                            (int)start_canvas_y,
                                            modifiers,
                                            GDK_LEAVE_NOTIFY);

  /* Process them */
  ligma_test_run_mainloop_until_idle ();
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
crop_tool_can_crop (LigmaTestFixture *fixture,
                    gconstpointer    data)
{
  Ligma             *ligma  = LIGMA (data);
  LigmaImage        *image = ligma_test_get_only_image (ligma);
  LigmaDisplayShell *shell = ligma_test_get_only_display_shell (ligma);

  gint cropped_x = 10;
  gint cropped_y = 10;
  gint cropped_w = 20;
  gint cropped_h = 30;

  /* Fit display and pause and let it stabalize (two idlings seems to
   * always be enough)
   */
  ligma_ui_manager_activate_action (ligma_test_utils_get_ui_manager (ligma),
                                   "view",
                                   "view-shrink-wrap");
  ligma_test_run_mainloop_until_idle ();
  ligma_test_run_mainloop_until_idle ();

  /* Activate crop tool */
  ligma_tools_set_tool (ligma, "ligma-crop-tool", shell->display);

  /* Do the crop rect */
  ligma_tools_synthesize_image_click_drag_release (shell,
                                                  cropped_x,
                                                  cropped_y,
                                                  cropped_x + cropped_w,
                                                  cropped_y + cropped_h,
                                                  1 /*button*/,
                                                  0 /*modifiers*/);

  /* Crop */
  ligma_test_utils_synthesize_key_event (GTK_WIDGET (shell), GDK_KEY_Return);
  ligma_test_run_mainloop_until_idle ();

  /* Make sure the new image has the expected size */
  g_assert_cmpint (cropped_w, ==, ligma_image_get_width (image));
  g_assert_cmpint (cropped_h, ==, ligma_image_get_height (image));
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
crop_set_width_without_pending_rect (LigmaTestFixture *fixture,
                                     gconstpointer    data)
{
  Ligma                 *ligma    = LIGMA (data);
  LigmaDisplay          *display = ligma_test_get_only_display (ligma);
  LigmaToolInfo         *tool_info;
  LigmaRectangleOptions *rectangle_options;
  GtkWidget            *tool_options_gui;
  GtkWidget            *size_entry;

  /* Activate crop tool */
  ligma_tools_set_tool (ligma, "ligma-crop-tool", display);

  /* Get tool options */
  tool_info         = ligma_get_tool_info (ligma, "ligma-crop-tool");
  tool_options_gui  = ligma_tools_get_tool_options_gui (tool_info->tool_options);
  rectangle_options = LIGMA_RECTANGLE_OPTIONS (tool_info->tool_options);

  /* Find 'Width' or 'Height' GtkTextEntry in tool options */
  size_entry = ligma_rectangle_options_get_size_entry (rectangle_options);

  /* Set arbitrary non-0 value */
  ligma_size_entry_set_value (LIGMA_SIZE_ENTRY (size_entry),
                             0 /*field*/,
                             42.0 /*lower*/);

  /* If we don't crash, everything s fine */
}

int main(int argc, char **argv)
{
  Ligma *ligma   = NULL;
  gint  result = -1;

  ligma_test_bail_if_no_display ();
  gtk_test_init (&argc, &argv, NULL);

  ligma_test_utils_set_ligma3_directory ("LIGMA_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/ligmadir");
  ligma_test_utils_setup_menus_path ();

  /* Start up LIGMA */
  ligma = ligma_init_for_gui_testing (TRUE /*show_gui*/);
  ligma_test_run_mainloop_until_idle ();

  /* Add tests */
  ADD_TEST (crop_tool_can_crop);
  ADD_TEST (crop_set_width_without_pending_rect);

  /* Run the tests and return status */
  result = g_test_run ();

  /* Don't write files to the source dir */
  ligma_test_utils_set_ligma3_directory ("LIGMA_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/ligmadir-output");

  /* Exit properly so we don't break script-fu plug-in wire */
  ligma_exit (ligma, TRUE);

  return result;
}
