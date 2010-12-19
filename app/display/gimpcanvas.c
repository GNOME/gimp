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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvas.h"

#include "gimp-intl.h"


#define MAX_BATCH_SIZE 32000


enum
{
  PROP_0,
  PROP_CONFIG
};


/*  local function prototypes  */

static void       gimp_canvas_set_property    (GObject         *object,
                                               guint            property_id,
                                               const GValue    *value,
                                               GParamSpec      *pspec);
static void       gimp_canvas_get_property    (GObject         *object,
                                               guint            property_id,
                                               GValue          *value,
                                               GParamSpec      *pspec);

static void       gimp_canvas_unrealize       (GtkWidget       *widget);
static void       gimp_canvas_style_set       (GtkWidget       *widget,
                                               GtkStyle        *prev_style);
static gboolean   gimp_canvas_focus_in_event  (GtkWidget       *widget,
                                               GdkEventFocus   *event);
static gboolean   gimp_canvas_focus_out_event (GtkWidget       *widget,
                                               GdkEventFocus   *event);
static gboolean   gimp_canvas_focus           (GtkWidget       *widget,
                                               GtkDirectionType direction);


G_DEFINE_TYPE (GimpCanvas, gimp_canvas, GIMP_TYPE_OVERLAY_BOX)

#define parent_class gimp_canvas_parent_class


static void
gimp_canvas_class_init (GimpCanvasClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property    = gimp_canvas_set_property;
  object_class->get_property    = gimp_canvas_get_property;

  widget_class->unrealize       = gimp_canvas_unrealize;
  widget_class->style_set       = gimp_canvas_style_set;
  widget_class->focus_in_event  = gimp_canvas_focus_in_event;
  widget_class->focus_out_event = gimp_canvas_focus_out_event;
  widget_class->focus           = gimp_canvas_focus;

  g_object_class_install_property (object_class, PROP_CONFIG,
                                   g_param_spec_object ("config", NULL, NULL,
                                                        GIMP_TYPE_DISPLAY_CONFIG,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_canvas_init (GimpCanvas *canvas)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  gtk_widget_set_can_focus (widget, TRUE);
  gtk_widget_add_events (widget, GIMP_CANVAS_EVENT_MASK);
}

static void
gimp_canvas_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpCanvas *canvas = GIMP_CANVAS (object);

  switch (property_id)
    {
    case PROP_CONFIG:
      canvas->config = g_value_get_object (value); /* don't dup */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GimpCanvas *canvas = GIMP_CANVAS (object);

  switch (property_id)
    {
    case PROP_CONFIG:
      g_value_set_object (value, canvas->config);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_unrealize (GtkWidget *widget)
{
  GimpCanvas *canvas = GIMP_CANVAS (widget);

  if (canvas->layout)
    {
      g_object_unref (canvas->layout);
      canvas->layout = NULL;
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_canvas_style_set (GtkWidget *widget,
                       GtkStyle  *prev_style)
{
  GimpCanvas *canvas = GIMP_CANVAS (widget);

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  if (canvas->layout)
    {
      g_object_unref (canvas->layout);
      canvas->layout = NULL;
    }
}

static gboolean
gimp_canvas_focus_in_event (GtkWidget     *widget,
                            GdkEventFocus *event)
{
  /*  don't allow the default impl to invalidate the whole widget,
   *  we don't draw a focus indicator anyway.
   */
  return FALSE;
}

static gboolean
gimp_canvas_focus_out_event (GtkWidget     *widget,
                             GdkEventFocus *event)
{
  /*  see focus-in-event
   */
  return FALSE;
}

static gboolean
gimp_canvas_focus (GtkWidget        *widget,
                   GtkDirectionType  direction)
{
  GtkWidget *focus = gtk_container_get_focus_child (GTK_CONTAINER (widget));

  /* override GtkContainer's focus() implementation which would always
   * give focus to the canvas because it is focussable. Instead, try
   * navigating in the focussed overlay child first, and use
   * GtkContainer's default implementation only if that fails (which
   * happens when focus navigation leaves the overlay child).
   */

  if (focus && gtk_widget_child_focus (focus, direction))
    return TRUE;

  return GTK_WIDGET_CLASS (parent_class)->focus (widget, direction);
}


/*  public functions  */

/**
 * gimp_canvas_new:
 *
 * Creates a new #GimpCanvas widget.
 *
 * The #GimpCanvas widget is a #GtkDrawingArea abstraction. It manages
 * a set of graphic contexts for drawing on a GIMP display. If you
 * draw using a #GimpCanvasStyle, #GimpCanvas makes sure that the
 * associated #GdkGC is created. All drawing on the canvas needs to
 * happen by means of the #GimpCanvas drawing functions. Besides from
 * not needing a #GdkGC pointer, the #GimpCanvas drawing functions
 * look and work like their #GdkDrawable counterparts. #GimpCanvas
 * gracefully handles attempts to draw on the unrealized widget.
 *
 * Return value: a new #GimpCanvas widget
 **/
GtkWidget *
gimp_canvas_new (GimpDisplayConfig *config)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_CONFIG (config), NULL);

  return g_object_new (GIMP_TYPE_CANVAS,
                       "name",   "gimp-canvas",
                       "config", config,
                       NULL);
}

/**
 * gimp_canvas_get_layout:
 * @canvas:  a #GimpCanvas widget
 * @format:  a standard printf() format string.
 * @Varargs: the parameters to insert into the format string.
 *
 * Returns a layout which can be used for
 * pango_cairo_show_layout(). The layout belongs to the canvas and
 * should not be freed, not should a pointer to it be kept around
 * after drawing.
 *
 * Returns: a #PangoLayout owned by the canvas.
 **/
PangoLayout *
gimp_canvas_get_layout (GimpCanvas  *canvas,
                        const gchar *format,
                        ...)
{
  va_list  args;
  gchar   *text;

  if (! canvas->layout)
    canvas->layout = gtk_widget_create_pango_layout (GTK_WIDGET (canvas),
                                                     NULL);

  va_start (args, format);
  text = g_strdup_vprintf (format, args);
  va_end (args);

  pango_layout_set_text (canvas->layout, text, -1);
  g_free (text);

  return canvas->layout;
}

/**
 * gimp_canvas_set_bg_color:
 * @canvas:   a #GimpCanvas widget
 * @color:    a color in #GimpRGB format
 *
 * Sets the background color of the canvas's window.  This
 * is the color the canvas is set to if it is cleared.
 **/
void
gimp_canvas_set_bg_color (GimpCanvas *canvas,
                          GimpRGB    *color)
{
  GtkWidget *widget = GTK_WIDGET (canvas);
  GdkColor   gdk_color;

  if (! gtk_widget_get_realized (widget))
    return;

  gimp_rgb_get_gdk_color (color, &gdk_color);
  gdk_window_set_background (gtk_widget_get_window (widget), &gdk_color);

  gtk_widget_queue_draw (GTK_WIDGET (canvas));
}
