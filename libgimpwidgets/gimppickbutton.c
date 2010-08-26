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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcairo-utils.h"
#include "gimphelpui.h"
#include "gimppickbutton.h"
#include "gimpstock.h"

#include "libgimp/libgimp-intl.h"


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


static void       gimp_pick_button_destroy       (GtkObject      *object);

static void       gimp_pick_button_clicked       (GtkButton      *button);

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


G_DEFINE_TYPE (GimpPickButton, gimp_pick_button, GTK_TYPE_BUTTON)

#define parent_class gimp_pick_button_parent_class

static guint pick_button_signals[LAST_SIGNAL] = { 0 };


static void
gimp_pick_button_class_init (GimpPickButtonClass* klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  /**
   * GimpPickButton::color-picked:
   * @gimppickbutton: the object which received the signal.
   * @arg1: pointer to a #GimpRGB structure that holds the picked color
   *
   * This signal is emitted when the user has picked a color.
   **/
  pick_button_signals[COLOR_PICKED] =
    g_signal_new ("color-picked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPickButtonClass, color_picked),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  object_class->destroy = gimp_pick_button_destroy;

  button_class->clicked = gimp_pick_button_clicked;

  klass->color_picked   = NULL;
}

static void
gimp_pick_button_init (GimpPickButton *button)
{
  GtkWidget *image;

  image = gtk_image_new_from_stock (GIMP_STOCK_COLOR_PICK_FROM_SCREEN,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (GTK_WIDGET (button),
                           _("Click the eyedropper, then click a color "
                             "anywhere on your screen to select that color."),
                           NULL);
}

static void
gimp_pick_button_destroy (GtkObject *object)
{
  GimpPickButton *button = GIMP_PICK_BUTTON (object);

  if (button->cursor)
    {
      gdk_cursor_unref (button->cursor);
      button->cursor = NULL;
    }

  if (button->grab_widget)
    {
      gtk_widget_destroy (button->grab_widget);
      button->grab_widget = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
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


/*  private functions  */


/*  cursor stuff will be removed again once the gimpcursor.[ch] utility
 *  stuff has been moved to libgimpwidgets  --mitch
 */
#define DROPPER_WIDTH   17
#define DROPPER_HEIGHT  17
#define DROPPER_X_HOT    2
#define DROPPER_Y_HOT   16

static const guchar dropper_bits[] =
{
  0xff, 0x8f, 0x01, 0xff, 0x77, 0x01, 0xff, 0xfb, 0x00, 0xff, 0xf8, 0x00,
  0x7f, 0xff, 0x00, 0xff, 0x7e, 0x01, 0xff, 0x9d, 0x01, 0xff, 0xd8, 0x01,
  0x7f, 0xd4, 0x01, 0x3f, 0xee, 0x01, 0x1f, 0xff, 0x01, 0x8f, 0xff, 0x01,
  0xc7, 0xff, 0x01, 0xe3, 0xff, 0x01, 0xf3, 0xff, 0x01, 0xfd, 0xff, 0x01,
  0xff, 0xff, 0x01
};

static const guchar dropper_mask[] =
{
  0x00, 0x70, 0x00, 0x00, 0xf8, 0x00, 0x00, 0xfc, 0x01, 0x00, 0xff, 0x01,
  0x80, 0xff, 0x01, 0x00, 0xff, 0x00, 0x00, 0x7f, 0x00, 0x80, 0x3f, 0x00,
  0xc0, 0x3f, 0x00, 0xe0, 0x13, 0x00, 0xf0, 0x01, 0x00, 0xf8, 0x00, 0x00,
  0x7c, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x0d, 0x00, 0x00,
  0x02, 0x00, 0x00
};

static GdkCursor *
make_cursor (void)
{
  GdkCursor      *cursor;
  const GdkColor  bg = { 0, 0xffff, 0xffff, 0xffff };
  const GdkColor  fg = { 0, 0x0000, 0x0000, 0x0000 };

  GdkPixmap *pixmap =
    gdk_bitmap_create_from_data (NULL,
                                 (gchar *) dropper_bits,
                                 DROPPER_WIDTH, DROPPER_HEIGHT);
  GdkPixmap *mask =
    gdk_bitmap_create_from_data (NULL,
                                 (gchar *) dropper_mask,
                                 DROPPER_WIDTH, DROPPER_HEIGHT);

  cursor = gdk_cursor_new_from_pixmap (pixmap, mask, &fg, &bg,
                                       DROPPER_X_HOT ,DROPPER_Y_HOT);

  g_object_unref (pixmap);
  g_object_unref (mask);

  return cursor;
}

static void
gimp_pick_button_clicked (GtkButton *gtk_button)
{
  GimpPickButton *button = GIMP_PICK_BUTTON (gtk_button);
  GtkWidget      *widget;
  guint32         timestamp;

  if (! button->cursor)
    button->cursor = make_cursor ();

  if (! button->grab_widget)
    {
      button->grab_widget = gtk_invisible_new ();

      gtk_widget_add_events (button->grab_widget,
                             GDK_BUTTON_RELEASE_MASK |
                             GDK_BUTTON_PRESS_MASK   |
                             GDK_POINTER_MOTION_MASK);

      gtk_widget_show (button->grab_widget);
    }

  widget = button->grab_widget;
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
                        button->cursor,
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
  if (event->keyval == GDK_Escape)
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
  GdkDisplay *display   = gtk_widget_get_display (button->grab_widget);
  guint32     timestamp = gtk_get_current_event_time ();

  gdk_display_keyboard_ungrab (display, timestamp);
  gdk_display_pointer_ungrab (display, timestamp);

  gtk_grab_remove (button->grab_widget);
}

static void
gimp_pick_button_pick (GdkScreen      *screen,
                       gint            x_root,
                       gint            y_root,
                       GimpPickButton *button)
{
  GdkWindow       *root_window = gdk_screen_get_root_window (screen);
  cairo_surface_t *image;
  cairo_t         *cr;
  guchar          *data;
  guchar           color[3];
  GimpRGB          rgb;

  image = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 1, 1);

  cr = cairo_create (image);

  gdk_cairo_set_source_pixmap (cr, root_window, -x_root, -y_root);
  cairo_paint (cr);

  cairo_destroy (cr);

  data = cairo_image_surface_get_data (image);
  GIMP_CAIRO_RGB24_GET_PIXEL (data, color[0], color[1], color[2]);

  cairo_surface_destroy (image);

  gimp_rgba_set_uchar (&rgb, color[0], color[1], color[2], 1.0);

  g_signal_emit (button, pick_button_signals[COLOR_PICKED], 0, &rgb);
}
