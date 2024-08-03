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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
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
#include "gimppickbutton-private.h"
#include "gimpwidgetsutils.h"

#include "libgimp/libgimp-intl.h"

#if defined (GDK_WINDOWING_QUARTZ)
#include "gimppickbutton-quartz.h"
#elif defined (GDK_WINDOWING_WIN32)
#include "gimppickbutton-win32.h"
#else
#include "gimppickbutton-kwin.h"
#include "gimppickbutton-xdg.h"
#endif

/**
 * SECTION: gimppickbutton
 * @title: GimpPickButton
 * @short_description: Widget to pick a color from screen.
 *
 * #GimpPickButton is a specialized button. When clicked, it changes
 * the cursor to a color-picker pipette and allows the user to pick a
 * color from any point on the screen.
 **/


enum
{
  COLOR_PICKED,
  LAST_SIGNAL
};

#define GET_PRIVATE(obj) ((GimpPickButtonPrivate *) gimp_pick_button_get_instance_private ((GimpPickButton *) (obj)))

static void       gimp_pick_button_dispose          (GObject        *object);

static void       gimp_pick_button_clicked          (GtkButton      *button);

static gboolean   gimp_pick_button_mouse_press      (GtkWidget      *invisible,
                                                     GdkEventButton *event,
                                                     GimpPickButton *button);
static gboolean   gimp_pick_button_key_press        (GtkWidget      *invisible,
                                                     GdkEventKey    *event,
                                                     GimpPickButton *button);
static gboolean   gimp_pick_button_mouse_motion     (GtkWidget      *invisible,
                                                     GdkEventMotion *event,
                                                     GimpPickButton *button);
static gboolean   gimp_pick_button_mouse_release    (GtkWidget      *invisible,
                                                     GdkEventButton *event,
                                                     GimpPickButton *button);
static void       gimp_pick_button_shutdown         (GimpPickButton *button);
static void       gimp_pick_button_pick             (GimpPickButton *button,
                                                     GdkEvent       *event);



G_DEFINE_TYPE_WITH_PRIVATE (GimpPickButton, gimp_pick_button, GTK_TYPE_BUTTON)

#define parent_class gimp_pick_button_parent_class

static guint pick_button_signals[LAST_SIGNAL] = { 0 };


static void
gimp_pick_button_class_init (GimpPickButtonClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  /**
   * GimpPickButton::color-picked:
   * @gimppickbutton: the object which received the signal.
   * @color: pointer to a #GeglColor structure that holds the picked color
   *
   * This signal is emitted when the user has picked a color.
   **/
  pick_button_signals[COLOR_PICKED] =
    g_signal_new ("color-picked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPickButtonClass, color_picked),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_COLOR);

  object_class->dispose = gimp_pick_button_dispose;

  button_class->clicked = gimp_pick_button_clicked;

  klass->color_picked   = NULL;
}

static void
gimp_pick_button_init (GimpPickButton *button)
{
  GtkWidget *image;

  image = gtk_image_new_from_icon_name (GIMP_ICON_COLOR_PICK_FROM_SCREEN,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (GTK_WIDGET (button),
                           _("Click the eyedropper, then click a color "
                             "anywhere on your screen to select that color."),
                           NULL);
}

static void
gimp_pick_button_dispose (GObject *object)
{
  GimpPickButton        *button = GIMP_PICK_BUTTON (object);
  GimpPickButtonPrivate *priv   = GET_PRIVATE (button);

  if (priv->cursor)
    {
      g_object_unref (priv->cursor);
      priv->cursor = NULL;
    }

  if (priv->grab_widget)
    {
      gtk_widget_destroy (priv->grab_widget);
      priv->grab_widget = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_pick_button_clicked (GtkButton *button)
{
#if defined (GDK_WINDOWING_QUARTZ)
  _gimp_pick_button_quartz_pick (GIMP_PICK_BUTTON (button));
#elif defined (GDK_WINDOWING_WIN32)
  _gimp_pick_button_win32_pick (GIMP_PICK_BUTTON (button));
#else
#ifdef GDK_WINDOWING_X11
  /* It's a bit weird as we use the default pick code both in first and
   * last cases. It's because when running GIMP on X11 in particular,
   * the portals don't return color space information. So the returned
   * color is in the display space, not in the current image space and
   * we have no way to convert the data back (well if running on X11, we
   * could still get a profile from the display, but if there are
   * several displays, we can't know for sure where the color was picked
   * from.).
   * See: https://github.com/flatpak/xdg-desktop-portal/issues/862
   */
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    _gimp_pick_button_default_pick (GIMP_PICK_BUTTON (button));
  else
#endif
  if (_gimp_pick_button_xdg_available ())
    _gimp_pick_button_xdg_pick (GIMP_PICK_BUTTON (button));
  else if (_gimp_pick_button_kwin_available ())
    _gimp_pick_button_kwin_pick (GIMP_PICK_BUTTON (button));
  else
    _gimp_pick_button_default_pick (GIMP_PICK_BUTTON (button));
#endif
}

/*  default implementation functions */

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

      gimp_pick_button_pick (button, (GdkEvent *) event);

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
  gimp_pick_button_pick (button, (GdkEvent *) event);

  return TRUE;
}

static gboolean
gimp_pick_button_mouse_release (GtkWidget      *invisible,
                                GdkEventButton *event,
                                GimpPickButton *button)
{
  if (event->button != 1)
    return FALSE;

  gimp_pick_button_pick (button, (GdkEvent *) event);

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
  GimpPickButtonPrivate *priv    = GET_PRIVATE (button);
  GdkDisplay            *display = gtk_widget_get_display (priv->grab_widget);

  gtk_grab_remove (priv->grab_widget);

  gdk_seat_ungrab (gdk_display_get_default_seat (display));
}

static void
gimp_pick_button_pick (GimpPickButton *button,
                       GdkEvent       *event)
{
  GdkScreen        *screen = gdk_event_get_screen (event);
  GimpColorProfile *monitor_profile;
  GdkMonitor       *monitor;
  GeglColor        *rgb    = gegl_color_new ("black");
  const Babl       *space  = NULL;
  gint              x_root;
  gint              y_root;
  gdouble           x_win;
  gdouble           y_win;

  gdk_window_get_origin (gdk_event_get_window (event), &x_root, &y_root);
  gdk_event_get_coords (event, &x_win, &y_win);
  x_root += x_win;
  y_root += y_win;

  monitor = gdk_display_get_monitor_at_point (gdk_screen_get_display (screen),
                                              x_root, y_root);
  monitor_profile = gimp_monitor_get_color_profile (monitor);

  if (monitor_profile)
    space = gimp_color_profile_get_space (monitor_profile,
                                          GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                          NULL);

#ifdef G_OS_WIN32

  {
    HDC      hdc;
    RECT     rect;
    COLORREF win32_color;
    guchar   temp_rgb[3];

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

    temp_rgb[0] = GetRValue (win32_color);
    temp_rgb[1] = GetGValue (win32_color);
    temp_rgb[2] = GetBValue (win32_color);

    gegl_color_set_pixel (rgb, babl_format_with_space ("R'G'B' u8", space),
                          temp_rgb);
  }

#else

  {
    GdkWindow       *window;
    gint             x_window;
    gint             y_window;
    cairo_surface_t *image;
    cairo_t         *cr;
    guchar          *data;
    guchar           color[3];

    /* we try to pick from the local window under the cursor, and fall
     * back to picking from the root window if this fails (i.e., if
     * the cursor is not under a local window).  on wayland, picking
     * from the root window is not supported, so this at least allows
     * us to pick from local windows.  see bug #780375.
     */
    window = gdk_device_get_window_at_position (gdk_event_get_device (event),
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

    gegl_color_set_pixel (rgb, babl_format_with_space ("R'G'B' u8", space),
                          color);
  }

#endif

  g_signal_emit_by_name (button, "color-picked", rgb);
  g_object_unref (rgb);
}

/* entry point to this file, called from gimppickbutton.c */
void
_gimp_pick_button_default_pick (GimpPickButton *button)
{
  GimpPickButtonPrivate *priv = GET_PRIVATE (button);
  GdkDisplay            *display;
  GtkWidget             *widget;

  if (! priv->cursor)
    priv->cursor =
      make_cursor (gtk_widget_get_display (GTK_WIDGET (button)));

  if (! priv->grab_widget)
    {
      priv->grab_widget = gtk_invisible_new ();

      gtk_widget_add_events (priv->grab_widget,
                             GDK_BUTTON_PRESS_MASK   |
                             GDK_BUTTON_RELEASE_MASK |
                             GDK_BUTTON1_MOTION_MASK);

      gtk_widget_show (priv->grab_widget);
    }

  widget = priv->grab_widget;

  display = gtk_widget_get_display (widget);

  if (gdk_seat_grab (gdk_display_get_default_seat (display),
                     gtk_widget_get_window (widget),
                     GDK_SEAT_CAPABILITY_ALL,
                     FALSE,
                     priv->cursor,
                     NULL,
                     NULL, NULL) != GDK_GRAB_SUCCESS)
    {
      g_warning ("Failed to grab seat to do eyedropper");
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

/*  public functions  */

/**
 * gimp_pick_button_new:
 *
 * Creates a new #GimpPickButton widget.
 *
 * Returns: A new #GimpPickButton widget.
 **/
GtkWidget *
gimp_pick_button_new (void)
{
  return g_object_new (GIMP_TYPE_PICK_BUTTON, NULL);
}
