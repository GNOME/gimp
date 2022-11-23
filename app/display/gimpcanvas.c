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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"

#include "config/ligmadisplayconfig.h"

#include "widgets/ligmawidgets-utils.h"

#include "ligmacanvas.h"

#include "ligma-intl.h"


#define MAX_BATCH_SIZE 32000


enum
{
  PROP_0,
  PROP_CONFIG
};


/*  local function prototypes  */

static void       ligma_canvas_set_property    (GObject         *object,
                                               guint            property_id,
                                               const GValue    *value,
                                               GParamSpec      *pspec);
static void       ligma_canvas_get_property    (GObject         *object,
                                               guint            property_id,
                                               GValue          *value,
                                               GParamSpec      *pspec);

static void       ligma_canvas_unrealize       (GtkWidget       *widget);
static void       ligma_canvas_style_updated   (GtkWidget       *widget);
static gboolean   ligma_canvas_focus_in_event  (GtkWidget       *widget,
                                               GdkEventFocus   *event);
static gboolean   ligma_canvas_focus_out_event (GtkWidget       *widget,
                                               GdkEventFocus   *event);
static gboolean   ligma_canvas_focus           (GtkWidget       *widget,
                                               GtkDirectionType direction);


G_DEFINE_TYPE (LigmaCanvas, ligma_canvas, LIGMA_TYPE_OVERLAY_BOX)

#define parent_class ligma_canvas_parent_class


static void
ligma_canvas_class_init (LigmaCanvasClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property    = ligma_canvas_set_property;
  object_class->get_property    = ligma_canvas_get_property;

  widget_class->unrealize       = ligma_canvas_unrealize;
  widget_class->style_updated   = ligma_canvas_style_updated;
  widget_class->focus_in_event  = ligma_canvas_focus_in_event;
  widget_class->focus_out_event = ligma_canvas_focus_out_event;
  widget_class->focus           = ligma_canvas_focus;

  g_object_class_install_property (object_class, PROP_CONFIG,
                                   g_param_spec_object ("config", NULL, NULL,
                                                        LIGMA_TYPE_DISPLAY_CONFIG,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_canvas_init (LigmaCanvas *canvas)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  gtk_widget_set_can_focus (widget, TRUE);
  gtk_widget_add_events (widget, LIGMA_CANVAS_EVENT_MASK);
}

static void
ligma_canvas_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  LigmaCanvas *canvas = LIGMA_CANVAS (object);

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
ligma_canvas_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  LigmaCanvas *canvas = LIGMA_CANVAS (object);

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
ligma_canvas_unrealize (GtkWidget *widget)
{
  LigmaCanvas *canvas = LIGMA_CANVAS (widget);

  g_clear_object (&canvas->layout);

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
ligma_canvas_style_updated (GtkWidget *widget)
{
  LigmaCanvas *canvas = LIGMA_CANVAS (widget);

  g_clear_object (&canvas->layout);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);
}

static gboolean
ligma_canvas_focus_in_event (GtkWidget     *widget,
                            GdkEventFocus *event)
{
  /*  don't allow the default impl to invalidate the whole widget,
   *  we don't draw a focus indicator anyway.
   */
  return FALSE;
}

static gboolean
ligma_canvas_focus_out_event (GtkWidget     *widget,
                             GdkEventFocus *event)
{
  /*  see focus-in-event
   */
  return FALSE;
}

static gboolean
ligma_canvas_focus (GtkWidget        *widget,
                   GtkDirectionType  direction)
{
  GtkWidget *focus = gtk_container_get_focus_child (GTK_CONTAINER (widget));

  /* override GtkContainer's focus() implementation which would always
   * give focus to the canvas because it is focussable. Instead, try
   * navigating in the focused overlay child first, and use
   * GtkContainer's default implementation only if that fails (which
   * happens when focus navigation leaves the overlay child).
   */

  if (focus && gtk_widget_child_focus (focus, direction))
    return TRUE;

  return GTK_WIDGET_CLASS (parent_class)->focus (widget, direction);
}


/*  public functions  */

/**
 * ligma_canvas_new:
 *
 * Creates a new #LigmaCanvas widget.
 *
 * The #LigmaCanvas widget is a #GtkDrawingArea abstraction. It manages
 * a set of graphic contexts for drawing on a LIGMA display. If you
 * draw using a #LigmaCanvasStyle, #LigmaCanvas makes sure that the
 * associated #GdkGC is created. All drawing on the canvas needs to
 * happen by means of the #LigmaCanvas drawing functions. Besides from
 * not needing a #GdkGC pointer, the #LigmaCanvas drawing functions
 * look and work like their #GdkDrawable counterparts. #LigmaCanvas
 * gracefully handles attempts to draw on the unrealized widget.
 *
 * Returns: a new #LigmaCanvas widget
 **/
GtkWidget *
ligma_canvas_new (LigmaDisplayConfig *config)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_CONFIG (config), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS,
                       "name",   "ligma-canvas",
                       "config", config,
                       NULL);
}

/**
 * ligma_canvas_get_layout:
 * @canvas:  a #LigmaCanvas widget
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
ligma_canvas_get_layout (LigmaCanvas  *canvas,
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
 * ligma_canvas_set_padding:
 * @canvas:   a #LigmaCanvas widget
 * @color:    a color in #LigmaRGB format
 *
 * Sets the background color of the canvas's window.  This
 * is the color the canvas is set to if it is cleared.
 **/
void
ligma_canvas_set_padding (LigmaCanvas            *canvas,
                         LigmaCanvasPaddingMode  padding_mode,
                         const LigmaRGB         *padding_color)
{
  g_return_if_fail (LIGMA_IS_CANVAS (canvas));
  g_return_if_fail (padding_color != NULL);

  canvas->padding_mode  = padding_mode;
  canvas->padding_color = *padding_color;

  gtk_widget_queue_draw (GTK_WIDGET (canvas));
}
