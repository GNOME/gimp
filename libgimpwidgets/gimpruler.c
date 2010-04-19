/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimpruler.h"


#define DEFAULT_RULER_FONT_SCALE  PANGO_SCALE_SMALL
#define MINIMUM_INCR              5


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_UNIT,
  PROP_LOWER,
  PROP_UPPER,
  PROP_POSITION,
  PROP_MAX_SIZE
};


/* All distances below are in 1/72nd's of an inch. (According to
 * Adobe that's a point, but points are really 1/72.27 in.)
 */
typedef struct
{
  GtkOrientation  orientation;
  GimpUnit        unit;
  gdouble         lower;
  gdouble         upper;
  gdouble         position;
  gdouble         max_size;

  GdkPixmap      *backing_store;
  GdkGC          *non_gr_exp_gc;
  PangoLayout    *layout;
  gdouble         font_scale;

  gint            xsrc;
  gint            ysrc;
} GimpRulerPrivate;


static const struct
{
  const gdouble  ruler_scale[16];
  const gint     subdivide[5];
} ruler_metric =
{
  { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000 },
  { 1, 5, 10, 50, 100 }
};


static void          gimp_ruler_set_property  (GObject        *object,
                                               guint            prop_id,
                                               const GValue   *value,
                                               GParamSpec     *pspec);
static void          gimp_ruler_get_property  (GObject        *object,
                                               guint           prop_id,
                                               GValue         *value,
                                               GParamSpec     *pspec);

static void          gimp_ruler_realize       (GtkWidget      *widget);
static void          gimp_ruler_unrealize     (GtkWidget      *widget);
static void          gimp_ruler_size_allocate (GtkWidget      *widget,
                                               GtkAllocation  *allocation);
static void          gimp_ruler_size_request  (GtkWidget      *widget,
                                               GtkRequisition *requisition);
static void          gimp_ruler_style_set     (GtkWidget      *widget,
                                               GtkStyle       *prev_style);
static gboolean      gimp_ruler_motion_notify (GtkWidget      *widget,
                                               GdkEventMotion *event);
static gboolean      gimp_ruler_expose        (GtkWidget      *widget,
                                               GdkEventExpose *event);

static void          gimp_ruler_draw_ticks    (GimpRuler      *ruler);
static void          gimp_ruler_draw_pos      (GimpRuler      *ruler);
static void          gimp_ruler_make_pixmap   (GimpRuler      *ruler);

static PangoLayout * gimp_ruler_get_layout    (GtkWidget      *widget,
                                               const gchar    *text);


G_DEFINE_TYPE (GimpRuler, gimp_ruler, GTK_TYPE_WIDGET)

#define GIMP_RULER_GET_PRIVATE(ruler) \
  G_TYPE_INSTANCE_GET_PRIVATE (ruler, GIMP_TYPE_RULER, GimpRulerPrivate)


static void
gimp_ruler_class_init (GimpRulerClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property        = gimp_ruler_set_property;
  object_class->get_property        = gimp_ruler_get_property;

  widget_class->realize             = gimp_ruler_realize;
  widget_class->unrealize           = gimp_ruler_unrealize;
  widget_class->size_allocate       = gimp_ruler_size_allocate;
  widget_class->size_request        = gimp_ruler_size_request;
  widget_class->style_set           = gimp_ruler_style_set;
  widget_class->motion_notify_event = gimp_ruler_motion_notify;
  widget_class->expose_event        = gimp_ruler_expose;

  g_type_class_add_private (object_class, sizeof (GimpRulerPrivate));

  g_object_class_install_property (object_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation",
                                                      "Orientation",
                                                      "The orientation of the ruler",
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_HORIZONTAL,
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_LOWER,
                                   gimp_param_spec_unit ("unit",
                                                         "Unit",
                                                         "Unit of ruler",
                                                         TRUE, TRUE,
                                                         GIMP_UNIT_PIXEL,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_LOWER,
                                   g_param_spec_double ("lower",
                                                        "Lower",
                                                        "Lower limit of ruler",
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_UPPER,
                                   g_param_spec_double ("upper",
                                                        "Upper",
                                                        "Upper limit of ruler",
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_POSITION,
                                   g_param_spec_double ("position",
                                                        "Position",
                                                        "Position of mark on the ruler",
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_MAX_SIZE,
                                   g_param_spec_double ("max-size",
                                                        "Max Size",
                                                        "Maximum size of the ruler",
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_double ("font-scale",
                                                                NULL, NULL,
                                                                0.0,
                                                                G_MAXDOUBLE,
                                                                DEFAULT_RULER_FONT_SCALE,
                                                                GIMP_PARAM_READABLE));
}

static void
gimp_ruler_init (GimpRuler *ruler)
{
  GimpRulerPrivate *priv = GIMP_RULER_GET_PRIVATE (ruler);

  priv->orientation   = GTK_ORIENTATION_HORIZONTAL;
  priv->unit          = GIMP_PIXELS;
  priv->lower         = 0;
  priv->upper         = 0;
  priv->position      = 0;
  priv->max_size      = 0;
  priv->backing_store = NULL;
  priv->non_gr_exp_gc = NULL;
  priv->font_scale    = DEFAULT_RULER_FONT_SCALE;
}

static void
gimp_ruler_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpRuler        *ruler = GIMP_RULER (object);
  GimpRulerPrivate *priv  = GIMP_RULER_GET_PRIVATE (ruler);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      priv->orientation = g_value_get_enum (value);
      gtk_widget_queue_resize (GTK_WIDGET (ruler));
      break;

    case PROP_UNIT:
      gimp_ruler_set_unit (ruler, g_value_get_int (value));
      break;

    case PROP_LOWER:
      gimp_ruler_set_range (ruler,
                            g_value_get_double (value),
                            priv->upper,
                            priv->max_size);
      break;
    case PROP_UPPER:
      gimp_ruler_set_range (ruler,
                            priv->lower,
                            g_value_get_double (value),
                            priv->max_size);
      break;

    case PROP_POSITION:
      gimp_ruler_set_position (ruler, g_value_get_double (value));
      break;

    case PROP_MAX_SIZE:
      gimp_ruler_set_range (ruler,
                            priv->lower,
                            priv->upper,
                            g_value_get_double (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_ruler_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GimpRuler        *ruler = GIMP_RULER (object);
  GimpRulerPrivate *priv  = GIMP_RULER_GET_PRIVATE (ruler);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;

    case PROP_UNIT:
      g_value_set_int (value, priv->unit);
      break;

    case PROP_LOWER:
      g_value_set_double (value, priv->lower);
      break;

    case PROP_UPPER:
      g_value_set_double (value, priv->upper);
      break;

    case PROP_POSITION:
      g_value_set_double (value, priv->position);
      break;

    case PROP_MAX_SIZE:
      g_value_set_double (value, priv->max_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * gimp_ruler_new:
 * @orientation: the ruler's orientation.
 *
 * Creates a new ruler.
 *
 * Return value: a new #GimpRuler widget.
 *
 * Since: GIMP 2.8
 **/
GtkWidget *
gimp_ruler_new (GtkOrientation orientation)
{
  return g_object_new (GIMP_TYPE_RULER,
                       "orientation", orientation,
                       NULL);
}

/**
 * gimp_ruler_set_position:
 * @ruler: a #GimpRuler
 * @unit:  the #GimpUnit to set the ruler to
 *
 * This sets the unit of the ruler.
 *
 * Since: GIMP 2.8
 */
void
gimp_ruler_set_unit (GimpRuler *ruler,
                     GimpUnit   unit)
{
  GimpRulerPrivate *priv;

  g_return_if_fail (GIMP_IS_RULER (ruler));

  priv = GIMP_RULER_GET_PRIVATE (ruler);

  if (priv->unit != unit)
    {
      priv->unit = unit;
      g_object_notify (G_OBJECT (ruler), "unit");

      gtk_widget_queue_draw (GTK_WIDGET (ruler));
    }
}

/**
 * gimp_ruler_get_unit:
 * @ruler: a #GimpRuler
 *
 * Return value: the unit currently used in the @ruler widget.
 *
 * Since: GIMP 2.8
 **/
GimpUnit
gimp_ruler_get_unit (GimpRuler *ruler)
{
  g_return_val_if_fail (GIMP_IS_RULER (ruler), 0);

  return GIMP_RULER_GET_PRIVATE (ruler)->unit;
}

/**
 * gimp_ruler_set_position:
 * @ruler: a #GimpRuler
 * @position: the position to set the ruler to
 *
 * This sets the position of the ruler.
 *
 * Since: GIMP 2.8
 */
void
gimp_ruler_set_position (GimpRuler *ruler,
                         gdouble    position)
{
  GimpRulerPrivate *priv;

  g_return_if_fail (GIMP_IS_RULER (ruler));

  priv = GIMP_RULER_GET_PRIVATE (ruler);

  if (priv->position != position)
    {
      priv->position = position;
      g_object_notify (G_OBJECT (ruler), "position");

      gimp_ruler_draw_pos (ruler);
    }
}

/**
 * gimp_ruler_get_position:
 * @ruler: a #GimpRuler
 *
 * Return value: the current position of the @ruler widget.
 *
 * Since: GIMP 2.8
 **/
gdouble
gimp_ruler_get_position (GimpRuler *ruler)
{
  g_return_val_if_fail (GIMP_IS_RULER (ruler), 0.0);

  return GIMP_RULER_GET_PRIVATE (ruler)->position;
}

/**
 * gimp_ruler_set_range:
 * @ruler: a #GimpRuler
 * @lower: the lower limit of the ruler
 * @upper: the upper limit of the ruler
 * @max_size: the maximum size of the ruler used when calculating the space to
 * leave for the text
 *
 * This sets the range of the ruler.
 *
 * Since: GIMP 2.8
 */
void
gimp_ruler_set_range (GimpRuler *ruler,
                      gdouble    lower,
                      gdouble    upper,
                      gdouble    max_size)
{
  GimpRulerPrivate *priv;

  g_return_if_fail (GIMP_IS_RULER (ruler));

  priv = GIMP_RULER_GET_PRIVATE (ruler);

  g_object_freeze_notify (G_OBJECT (ruler));
  if (priv->lower != lower)
    {
      priv->lower = lower;
      g_object_notify (G_OBJECT (ruler), "lower");
    }
  if (priv->upper != upper)
    {
      priv->upper = upper;
      g_object_notify (G_OBJECT (ruler), "upper");
    }
  if (priv->max_size != max_size)
    {
      priv->max_size = max_size;
      g_object_notify (G_OBJECT (ruler), "max-size");
    }
  g_object_thaw_notify (G_OBJECT (ruler));

  gtk_widget_queue_draw (GTK_WIDGET (ruler));
}

/**
 * gimp_ruler_get_range:
 * @ruler: a #GimpRuler
 * @lower: location to store lower limit of the ruler, or %NULL
 * @upper: location to store upper limit of the ruler, or %NULL
 * @max_size: location to store the maximum size of the ruler used when
 *            calculating the space to leave for the text, or %NULL.
 *
 * Retrieves values indicating the range and current position of a #GimpRuler.
 * See gimp_ruler_set_range().
 *
 * Since: GIMP 2.8
 **/
void
gimp_ruler_get_range (GimpRuler *ruler,
                      gdouble   *lower,
                      gdouble   *upper,
                      gdouble   *max_size)
{
  GimpRulerPrivate *priv;

  g_return_if_fail (GIMP_IS_RULER (ruler));

  priv = GIMP_RULER_GET_PRIVATE (ruler);

  if (lower)
    *lower = priv->lower;
  if (upper)
    *upper = priv->upper;
  if (max_size)
    *max_size = priv->max_size;
}

static void
gimp_ruler_realize (GtkWidget *widget)
{
  GimpRuler     *ruler = GIMP_RULER (widget);
  GtkAllocation  allocation;
  GdkWindowAttr  attributes;
  gint           attributes_mask;

  gtk_widget_set_realized (widget, TRUE);

  gtk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x           = allocation.x;
  attributes.y           = allocation.y;
  attributes.width       = allocation.width;
  attributes.height      = allocation.height;
  attributes.wclass      = GDK_INPUT_OUTPUT;
  attributes.visual      = gtk_widget_get_visual (widget);
  attributes.colormap    = gtk_widget_get_colormap (widget);
  attributes.event_mask  = (gtk_widget_get_events (widget) |
                            GDK_EXPOSURE_MASK              |
                            GDK_POINTER_MOTION_MASK        |
                            GDK_POINTER_MOTION_HINT_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  gtk_widget_set_window (widget,
                         gdk_window_new (gtk_widget_get_parent_window (widget),
                                         &attributes, attributes_mask));
  gdk_window_set_user_data (gtk_widget_get_window (widget), ruler);

  gtk_widget_style_attach (widget);
  gtk_style_set_background (gtk_widget_get_style (widget),
                            gtk_widget_get_window (widget),
                            GTK_STATE_ACTIVE);

  gimp_ruler_make_pixmap (ruler);
}

static void
gimp_ruler_unrealize (GtkWidget *widget)
{
  GimpRuler        *ruler = GIMP_RULER (widget);
  GimpRulerPrivate *priv  = GIMP_RULER_GET_PRIVATE (ruler);

  if (priv->backing_store)
    {
      g_object_unref (priv->backing_store);
      priv->backing_store = NULL;
    }

  if (priv->non_gr_exp_gc)
    {
      g_object_unref (priv->non_gr_exp_gc);
      priv->non_gr_exp_gc = NULL;
    }

  if (priv->layout)
    {
      g_object_unref (priv->layout);
      priv->layout = NULL;
    }

  GTK_WIDGET_CLASS (gimp_ruler_parent_class)->unrealize (widget);
}

static void
gimp_ruler_size_allocate (GtkWidget     *widget,
                          GtkAllocation *allocation)
{
  GimpRuler *ruler = GIMP_RULER (widget);

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_realized (widget))
    {
      gdk_window_move_resize (gtk_widget_get_window (widget),
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);

      gimp_ruler_make_pixmap (ruler);
    }
}

static void
gimp_ruler_size_request (GtkWidget      *widget,
                         GtkRequisition *requisition)
{
  GimpRulerPrivate *priv  = GIMP_RULER_GET_PRIVATE (widget);
  GtkStyle         *style = gtk_widget_get_style (widget);
  PangoLayout      *layout;
  PangoRectangle    ink_rect;
  gint              size;

  layout = gimp_ruler_get_layout (widget, "0123456789");
  pango_layout_get_pixel_extents (layout, &ink_rect, NULL);

  size = 2 + ink_rect.height * 1.7;

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      requisition->width  = style->xthickness * 2 + 1;
      requisition->height = style->ythickness * 2 + size;
    }
  else
    {
      requisition->width  = style->xthickness * 2 + size;
      requisition->height = style->ythickness * 2 + 1;
    }
}

static void
gimp_ruler_style_set (GtkWidget *widget,
                      GtkStyle  *prev_style)
{
  GimpRulerPrivate *priv = GIMP_RULER_GET_PRIVATE (widget);

  GTK_WIDGET_CLASS (gimp_ruler_parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "font-scale", &priv->font_scale,
                        NULL);

  if (priv->layout)
    {
      g_object_unref (priv->layout);
      priv->layout = NULL;
    }
}

static gboolean
gimp_ruler_motion_notify (GtkWidget      *widget,
                          GdkEventMotion *event)
{
  GimpRuler        *ruler = GIMP_RULER (widget);
  GimpRulerPrivate *priv  = GIMP_RULER_GET_PRIVATE (ruler);
  GtkAllocation     allocation;
  gdouble           lower;
  gdouble           upper;

  gdk_event_request_motions (event);

  gtk_widget_get_allocation (widget, &allocation);
  gimp_ruler_get_range (ruler, &lower, &upper, NULL);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      gimp_ruler_set_position (ruler,
                               lower +
                               (upper - lower) * event->x /
                               allocation.width);
    }
  else
    {
      gimp_ruler_set_position (ruler,
                               lower +
                               (upper - lower) * event->y /
                               allocation.height);
    }

  return FALSE;
}

static gboolean
gimp_ruler_expose (GtkWidget      *widget,
                   GdkEventExpose *event)
{
  if (gtk_widget_is_drawable (widget))
    {
      GimpRuler        *ruler = GIMP_RULER (widget);
      GimpRulerPrivate *priv  = GIMP_RULER_GET_PRIVATE (ruler);
      GtkAllocation     allocation;

      gtk_widget_get_allocation (widget, &allocation);

      gimp_ruler_draw_ticks (ruler);

      gdk_draw_drawable (gtk_widget_get_window (widget),
                         priv->non_gr_exp_gc,
                         priv->backing_store,
                         0, 0, 0, 0,
                         allocation.width,
                         allocation.height);

      gimp_ruler_draw_pos (ruler);
    }

  return FALSE;
}

static void
gimp_ruler_draw_ticks (GimpRuler *ruler)
{
  GtkWidget        *widget = GTK_WIDGET (ruler);
  GtkStyle         *style  = gtk_widget_get_style (widget);
  GimpRulerPrivate *priv   = GIMP_RULER_GET_PRIVATE (ruler);
  GtkStateType      state  = gtk_widget_get_state (widget);
  GtkAllocation     allocation;
  cairo_t          *cr;
  gint              i;
  gint              width, height;
  gint              xthickness;
  gint              ythickness;
  gint              length, ideal_length;
  gdouble           lower, upper;  /* Upper and lower limits, in ruler units */
  gdouble           increment;     /* Number of pixels per unit */
  gint              scale;         /* Number of units per major unit */
  gdouble           start, end, cur;
  gchar             unit_str[32];
  gint              digit_height;
  gint              digit_offset;
  gint              text_size;
  gint              pos;
  gdouble           max_size;
  GimpUnit          unit;
  PangoLayout      *layout;
  PangoRectangle    logical_rect, ink_rect;

  if (! gtk_widget_is_drawable (widget))
    return;

  gtk_widget_get_allocation (widget, &allocation);

  xthickness = style->xthickness;
  ythickness = style->ythickness;

  layout = gimp_ruler_get_layout (widget, "0123456789");
  pango_layout_get_extents (layout, &ink_rect, &logical_rect);

  digit_height = PANGO_PIXELS (ink_rect.height) + 2;
  digit_offset = ink_rect.y;

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      width  = allocation.width;
      height = allocation.height - ythickness * 2;
    }
  else
    {
      width  = allocation.height;
      height = allocation.width - ythickness * 2;
    }

  gtk_paint_box (style, priv->backing_store,
                 GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                 NULL, widget,
                 priv->orientation == GTK_ORIENTATION_HORIZONTAL ?
                 "hruler" : "vruler",
                 0, 0,
                 allocation.width, allocation.height);

  cr = gdk_cairo_create (priv->backing_store);
  gdk_cairo_set_source_color (cr, &style->fg[state]);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      cairo_rectangle (cr,
                       xthickness,
                       height + ythickness,
                       allocation.width - 2 * xthickness,
                       1);
    }
  else
    {
      cairo_rectangle (cr,
                       height + xthickness,
                       ythickness,
                       1,
                       allocation.height - 2 * ythickness);
    }

  gimp_ruler_get_range (ruler, &lower, &upper, &max_size);

  if ((upper - lower) == 0)
    goto out;

  increment = (gdouble) width / (upper - lower);

  /* determine the scale
   *   use the maximum extents of the ruler to determine the largest
   *   possible number to be displayed.  Calculate the height in pixels
   *   of this displayed text. Use this height to find a scale which
   *   leaves sufficient room for drawing the ruler.
   *
   *   We calculate the text size as for the vruler instead of
   *   actually measuring the text width, so that the result for the
   *   scale looks consistent with an accompanying vruler.
   */
  scale = ceil (max_size);
  g_snprintf (unit_str, sizeof (unit_str), "%d", scale);
  text_size = strlen (unit_str) * digit_height + 1;

  for (scale = 0; scale < G_N_ELEMENTS (ruler_metric.ruler_scale); scale++)
    if (ruler_metric.ruler_scale[scale] * fabs (increment) > 2 * text_size)
      break;

  if (scale == G_N_ELEMENTS (ruler_metric.ruler_scale))
    scale = G_N_ELEMENTS (ruler_metric.ruler_scale) - 1;

  unit = gimp_ruler_get_unit (ruler);

  /* drawing starts here */
  length = 0;
  for (i = G_N_ELEMENTS (ruler_metric.subdivide) - 1; i >= 0; i--)
    {
      gdouble subd_incr;

      /* hack to get proper subdivisions at full pixels */
      if (unit == GIMP_UNIT_PIXEL && scale == 1 && i == 1)
        subd_incr = 1.0;
      else
        subd_incr = ((gdouble) ruler_metric.ruler_scale[scale] /
                     (gdouble) ruler_metric.subdivide[i]);

      if (subd_incr * fabs (increment) <= MINIMUM_INCR)
        continue;

      /* don't subdivide pixels */
      if (unit == GIMP_UNIT_PIXEL && subd_incr < 1.0)
        continue;

      /* Calculate the length of the tickmarks. Make sure that
       * this length increases for each set of ticks
       */
      ideal_length = height / (i + 1) - 1;
      if (ideal_length > ++length)
        length = ideal_length;

      if (lower < upper)
        {
          start = floor (lower / subd_incr) * subd_incr;
          end   = ceil  (upper / subd_incr) * subd_incr;
        }
      else
        {
          start = floor (upper / subd_incr) * subd_incr;
          end   = ceil  (lower / subd_incr) * subd_incr;
        }

      for (cur = start; cur <= end; cur += subd_incr)
        {
          pos = ROUND ((cur - lower) * increment);

          if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            {
              cairo_rectangle (cr,
                               pos, height + ythickness - length,
                               1,   length);
            }
          else
            {
              cairo_rectangle (cr,
                               height + xthickness - length, pos,
                               length,                       1);
            }

          /* draw label */
          if (i == 0)
            {
              g_snprintf (unit_str, sizeof (unit_str), "%d", (int) cur);

              if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
                {
                  pango_layout_set_text (layout, unit_str, -1);
                  pango_layout_get_extents (layout, &logical_rect, NULL);

                  gtk_paint_layout (style,
                                    priv->backing_store,
                                    state,
                                    FALSE,
                                    NULL,
                                    widget,
                                    "hruler",
                                    pos + 2,
                                    ythickness + PANGO_PIXELS (logical_rect.y - digit_offset),
                                    layout);
                }
              else
                {
                  gint j;

                  for (j = 0; j < (int) strlen (unit_str); j++)
                    {
                      pango_layout_set_text (layout, unit_str + j, 1);
                      pango_layout_get_extents (layout, NULL, &logical_rect);

                      gtk_paint_layout (style,
                                        priv->backing_store,
                                        state,
                                        FALSE,
                                        NULL,
                                        widget,
                                        "vruler",
                                        xthickness + 1,
                                        pos + digit_height * j + 2 + PANGO_PIXELS (logical_rect.y - digit_offset),
                                        layout);
                    }
                }
            }
        }
    }

  cairo_fill (cr);
out:
  cairo_destroy (cr);
}

static void
gimp_ruler_draw_pos (GimpRuler *ruler)
{
  GtkWidget        *widget = GTK_WIDGET (ruler);
  GtkStyle         *style  = gtk_widget_get_style (widget);
  GimpRulerPrivate *priv   = GIMP_RULER_GET_PRIVATE (ruler);
  GtkStateType      state  = gtk_widget_get_state (widget);
  GtkAllocation     allocation;
  gint              x, y;
  gint              width, height;
  gint              bs_width, bs_height;
  gint              xthickness;
  gint              ythickness;

  if (! gtk_widget_is_drawable (widget))
    return;

  gtk_widget_get_allocation (widget, &allocation);

  xthickness = style->xthickness;
  ythickness = style->ythickness;

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      width  = allocation.width;
      height = allocation.height - ythickness * 2;

      bs_width = height / 2 + 2;
      bs_width |= 1;  /* make sure it's odd */
      bs_height = bs_width / 2 + 1;
    }
  else
    {
      width  = allocation.width - xthickness * 2;
      height = allocation.height;

      bs_height = width / 2 + 2;
      bs_height |= 1;  /* make sure it's odd */
      bs_width = bs_height / 2 + 1;
    }

  if ((bs_width > 0) && (bs_height > 0))
    {
      cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
      gdouble  lower;
      gdouble  upper;
      gdouble  position;
      gdouble  increment;

      /*  If a backing store exists, restore the ruler  */
      if (priv->backing_store)
        gdk_draw_drawable (gtk_widget_get_window (widget),
                           style->black_gc,
                           priv->backing_store,
                           priv->xsrc, priv->ysrc,
                           priv->xsrc, priv->ysrc,
                           bs_width, bs_height);

      position = gimp_ruler_get_position (ruler);

      gimp_ruler_get_range (ruler, &lower, &upper, NULL);

      if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          increment = (gdouble) width / (upper - lower);

          x = ROUND ((position - lower) * increment) + (xthickness - bs_width) / 2 - 1;
          y = (height + bs_height) / 2 + ythickness;
        }
      else
        {
          increment = (gdouble) height / (upper - lower);

          x = (width + bs_width) / 2 + xthickness;
          y = ROUND ((position - lower) * increment) + (ythickness - bs_height) / 2 - 1;
        }

      gdk_cairo_set_source_color (cr, &style->fg[state]);

      cairo_move_to (cr, x, y);

      if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          cairo_line_to (cr, x + bs_width / 2.0, y + bs_height);
          cairo_line_to (cr, x + bs_width,       y);
        }
      else
        {
          cairo_line_to (cr, x + bs_width, y + bs_height / 2.0);
          cairo_line_to (cr, x,            y + bs_height);
        }

      cairo_fill (cr);

      cairo_destroy (cr);

      priv->xsrc = x;
      priv->ysrc = y;
    }
}

static void
gimp_ruler_make_pixmap (GimpRuler *ruler)
{
  GtkWidget        *widget = GTK_WIDGET (ruler);
  GimpRulerPrivate *priv   = GIMP_RULER_GET_PRIVATE (ruler);
  GtkAllocation     allocation;
  gint              width;
  gint              height;

  gtk_widget_get_allocation (widget, &allocation);

  if (priv->backing_store)
    {
      gdk_drawable_get_size (priv->backing_store, &width, &height);
      if ((width  == allocation.width) &&
          (height == allocation.height))
        return;

      g_object_unref (priv->backing_store);
    }

  priv->backing_store = gdk_pixmap_new (gtk_widget_get_window (widget),
                                        allocation.width,
                                        allocation.height,
                                        -1);

  if (!priv->non_gr_exp_gc)
    {
      priv->non_gr_exp_gc = gdk_gc_new (gtk_widget_get_window (widget));
      gdk_gc_set_exposures (priv->non_gr_exp_gc, FALSE);
    }
}


static PangoLayout *
gimp_ruler_create_layout (GtkWidget   *widget,
                          const gchar *text)
{
  GimpRulerPrivate *priv = GIMP_RULER_GET_PRIVATE (widget);
  PangoLayout      *layout;
  PangoAttrList    *attrs;
  PangoAttribute   *attr;

  layout = gtk_widget_create_pango_layout (widget, text);

  attrs = pango_attr_list_new ();

  attr = pango_attr_scale_new (priv->font_scale);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);

  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);

  return layout;
}

static PangoLayout *
gimp_ruler_get_layout (GtkWidget   *widget,
                       const gchar *text)
{
  GimpRulerPrivate *priv = GIMP_RULER_GET_PRIVATE (widget);

  if (priv->layout)
    {
      pango_layout_set_text (priv->layout, text, -1);
      return priv->layout;
    }

  priv->layout = gimp_ruler_create_layout (widget, text);

  return priv->layout;
}
