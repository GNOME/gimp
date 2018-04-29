/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppickbutton.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on gtk+/gtk/gtkcolorsel.c
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcairo-utils.h"
#include "gimphelpui.h"
#include "gimpicons.h"
#include "gimppickbutton.h"
#include "gimppickbutton-default.h"
#include "gimppickbutton-private.h"
#include "gimpwidgetsutils.h"

#include "cursors/gimp-color-picker-cursors.c"

#include "libgimp/libgimp-intl.h"


static gboolean   gimp_pick_button_mouse_press   (GtkWidget      *invisible,
                                                  GdkEventButton *event,
                                                  GimpPickButton *button);
static gboolean   gimp_pick_button_key_press     (GtkWidget      *invisible,
                                                  GdkEventKey    *event,
                                                  GimpPickButton *button);
static gboolean   gimp_pick_button_mouse_motion  (GtkWidget      *invisible,
                                                  GdkEventMotion *event,
                                                  GimpPickButton *button);
static gboolean   gimp_pick_button_mouse_release (GtkWidget      *invisible,
                                                  GdkEventButton *event,
                                                  GimpPickButton *button);
static void       gimp_pick_button_shutdown      (GimpPickButton *button);
static void       gimp_pick_button_pick          (GdkScreen      *screen,
                                                  gint            x_root,
                                                  gint            y_root,
                                                  GimpPickButton *button);


static GdkCursor *
make_cursor (GdkDisplay *display)
{
  GdkPixbuf *pixbuf;
  GError    *error = NULL;

  pixbuf = gdk_pixbuf_new_from_resource ("/org/gimp/color-picker-cursors/cursor-color-picker.png",
                                         &error);

  if (pixbuf)
    {
      GdkCursor *cursor = gdk_cursor_new_from_pixbuf (display, pixbuf, 1, 30);

      g_object_unref (pixbuf);

      return cursor;
    }
  else
    {
      g_critical ("Failed to create cursor image: %s", error->message);
      g_clear_error (&error);
    }

  return NULL;
}

static gboolean
gimp_pick_button_mouse_press (GtkWidget      *invisible,
                              GdkEventButton *event,
                              GimpPickButton *button)
{
  if (event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
      g_signal_connect (invisible, "motion-notify-event",
                        G_CALLBACK (gimp_pick_button_mouse_motion),
                        button);
      g_signal_connect (invisible, "button-release-event",
                        G_CALLBACK (gimp_pick_button_mouse_release),
                        button);

      g_signal_handlers_disconnect_by_func (invisible,
                                            gimp_pick_button_mouse_press,
                                            button);
      g_signal_handlers_disconnect_by_func (invisible,
                                            gimp_pick_button_key_press,
                                            button);

      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_pick_button_key_press (GtkWidget      *invisible,
                            GdkEventKey    *event,
                            GimpPickButton *button)
{
  if (event->keyval == GDK_KEY_Escape)
    {
      gimp_pick_button_shutdown (button);

      g_signal_handlers_disconnect_by_func (invisible,
                                            gimp_pick_button_mouse_press,
                                            button);
      g_signal_handlers_disconnect_by_func (invisible,
                                            gimp_pick_button_key_press,
                                            button);

      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_pick_button_mouse_motion (GtkWidget      *invisible,
                               GdkEventMotion *event,
                               GimpPickButton *button)
{
  gint x_root;
  gint y_root;

  gdk_window_get_origin (event->window, &x_root, &y_root);
  x_root += event->x;
  y_root += event->y;

  gimp_pick_button_pick (gdk_event_get_screen ((GdkEvent *) event),
                         x_root, y_root, button);

  return TRUE;
}

static gboolean
gimp_pick_button_mouse_release (GtkWidget      *invisible,
                                GdkEventButton *event,
                                GimpPickButton *button)
{
  gint x_root;
  gint y_root;

  if (event->button != 1)
    return FALSE;

  gdk_window_get_origin (event->window, &x_root, &y_root);
  x_root += event->x;
  y_root += event->y;

  gimp_pick_button_pick (gdk_event_get_screen ((GdkEvent *) event),
                         x_root, y_root, button);

  gimp_pick_button_shutdown (button);

  g_signal_handlers_disconnect_by_func (invisible,
                                        gimp_pick_button_mouse_motion,
                                        button);
  g_signal_handlers_disconnect_by_func (invisible,
                                        gimp_pick_button_mouse_release,
                                        button);

  return TRUE;
}

static void
gimp_pick_button_shutdown (GimpPickButton *button)
{
  GdkDisplay *display   = gtk_widget_get_display (button->priv->grab_widget);
  guint32     timestamp = gtk_get_current_event_time ();

  gdk_display_keyboard_ungrab (display, timestamp);
  gdk_display_pointer_ungrab (display, timestamp);

  gtk_grab_remove (button->priv->grab_widget);
}

static void
gimp_pick_button_pick (GdkScreen      *screen,
                       gint            x_root,
                       gint            y_root,
                       GimpPickButton *button)
{
  GimpColorProfile *monitor_profile;
  GdkMonitor       *monitor;
  GimpRGB           rgb;

#ifdef G_OS_WIN32

  HDC               hdc;
  RECT              rect;
  COLORREF          win32_color;

  /* For MS Windows, use native GDI functions to get the pixel, as
   * cairo does not handle the case where you have multiple monitors
   * with a monitor on the left or above the primary monitor.  That
   * scenario create a cairo primary surface with negative extent,
   * which is not handled properly (bug 740634).
   */

  hdc = GetDC (HWND_DESKTOP);
  GetClipBox (hdc, &rect);
  win32_color = GetPixel (hdc, x_root + rect.left, y_root + rect.top);
  ReleaseDC (HWND_DESKTOP, hdc);

  gimp_rgba_set_uchar (&rgb,
                       GetRValue (win32_color),
                       GetGValue (win32_color),
                       GetBValue (win32_color),
                       255);

#else

  GdkWindow        *window;
  gint              x_window;
  gint              y_window;
  cairo_surface_t  *image;
  cairo_t          *cr;
  guchar           *data;
  guchar            color[3];

  /* we try to pick from the local window under the cursor, and fall back to
   * picking from the root window if this fails (i.e., if the cursor is not
   * under a local window).  on wayland, picking from the root window is not
   * supported, so this at least allows us to pick from local windows.  see
   * bug #780375.
   */
  window = gdk_display_get_window_at_pointer (gdk_screen_get_display (screen),
                                              &x_window, &y_window);

  if (! window)
    {
      window   = gdk_screen_get_root_window (screen);
      x_window = x_root;
      y_window = y_root;
    }

  image = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 1, 1);

  cr = cairo_create (image);

  gdk_cairo_set_source_window (cr, window, -x_window, -y_window);
  cairo_paint (cr);

  cairo_destroy (cr);

  data = cairo_image_surface_get_data (image);
  GIMP_CAIRO_RGB24_GET_PIXEL (data, color[0], color[1], color[2]);

  cairo_surface_destroy (image);

  gimp_rgba_set_uchar (&rgb, color[0], color[1], color[2], 255);

#endif

  monitor = gdk_display_get_monitor_at_point (gdk_screen_get_display (screen),
                                              x_root, y_root);
  monitor_profile = gimp_monitor_get_color_profile (monitor);

  if (monitor_profile)
    {
      GimpColorProfile        *srgb_profile;
      GimpColorTransform      *transform;
      const Babl              *format;
      GimpColorTransformFlags  flags = 0;

      format = babl_format ("R'G'B'A double");

      flags |= GIMP_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE;
      flags |= GIMP_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION;

      srgb_profile = gimp_color_profile_new_rgb_srgb ();
      transform = gimp_color_transform_new (monitor_profile, format,
                                            srgb_profile,    format,
                                            GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                            flags);
      g_object_unref (srgb_profile);

      if (transform)
        {
          gimp_color_transform_process_pixels (transform,
                                               format, &rgb,
                                               format, &rgb,
                                               1);
          gimp_rgb_clamp (&rgb);

          g_object_unref (transform);
        }
    }

  g_signal_emit_by_name (button, "color-picked", &rgb);
}

/* entry point to this file, called from gimppickbutton.c */
void
_gimp_pick_button_default_pick (GimpPickButton *button)
{
  GtkWidget *widget;
  guint32    timestamp;

  if (! button->priv->cursor)
    button->priv->cursor =
      make_cursor (gtk_widget_get_display (GTK_WIDGET (button)));

  if (! button->priv->grab_widget)
    {
      button->priv->grab_widget = gtk_invisible_new ();

      gtk_widget_add_events (button->priv->grab_widget,
                             GDK_BUTTON_RELEASE_MASK |
                             GDK_BUTTON_PRESS_MASK   |
                             GDK_POINTER_MOTION_MASK);

      gtk_widget_show (button->priv->grab_widget);
    }

  widget = button->priv->grab_widget;
  timestamp = gtk_get_current_event_time ();

  if (gdk_keyboard_grab (gtk_widget_get_window (widget), FALSE,
                         timestamp) != GDK_GRAB_SUCCESS)
    {
      g_warning ("Failed to grab keyboard to do eyedropper");
      return;
    }

  if (gdk_pointer_grab (gtk_widget_get_window (widget), FALSE,
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_BUTTON_PRESS_MASK   |
                        GDK_POINTER_MOTION_MASK,
                        NULL,
                        button->priv->cursor,
                        timestamp) != GDK_GRAB_SUCCESS)
    {
      gdk_display_keyboard_ungrab (gtk_widget_get_display (widget), timestamp);
      g_warning ("Failed to grab pointer to do eyedropper");
      return;
    }

  gtk_grab_add (widget);

  g_signal_connect (widget, "button-press-event",
                    G_CALLBACK (gimp_pick_button_mouse_press),
                    button);
  g_signal_connect (widget, "key-press-event",
                    G_CALLBACK (gimp_pick_button_key_press),
                    button);
}
