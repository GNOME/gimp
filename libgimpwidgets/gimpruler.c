/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpruler.h"


/* All distances below are in 1/72nd's of an inch. (According to
 * Adobe that's a point, but points are really 1/72.27 in.)
 */
typedef struct
{
  GdkPixmap       *backing_store;
  GdkGC           *non_gr_exp_gc;
  GimpRulerMetric *metric;

  /* The upper limit of the ruler (in points) */
  gdouble          lower;
  /* The lower limit of the ruler */
  gdouble          upper;
  /* The position of the mark on the ruler */
  gdouble          position;
  /* The maximum size of the ruler */
  gdouble          max_size;
} GimpRulerPrivate;

enum
{
  PROP_0,
  PROP_LOWER,
  PROP_UPPER,
  PROP_POSITION,
  PROP_MAX_SIZE,
  PROP_METRIC
};

static void gimp_ruler_realize       (GtkWidget      *widget);
static void gimp_ruler_unrealize     (GtkWidget      *widget);
static void gimp_ruler_size_allocate (GtkWidget      *widget,
                                      GtkAllocation  *allocation);
static gint gimp_ruler_expose        (GtkWidget      *widget,
                                      GdkEventExpose *event);
static void gimp_ruler_make_pixmap   (GimpRuler      *ruler);
static void gimp_ruler_set_property  (GObject        *object,
                                      guint            prop_id,
                                      const GValue   *value,
                                      GParamSpec     *pspec);
static void gimp_ruler_get_property  (GObject        *object,
                                      guint           prop_id,
                                      GValue         *value,
                                      GParamSpec     *pspec);

static const GimpRulerMetric const ruler_metrics[] =
{
  {
    "Pixel", "Pi", 1.0,
    { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000 },
    { 1, 5, 10, 50, 100 }
  },
  {
    "Inches", "In", 72.0,
    { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 },
    { 1, 2, 4, 8, 16 }
  },
  {
    "Centimeters", "Cm", 28.35,
    { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2550, 5000, 10000, 25000, 50000, 100000 },
    { 1, 5, 10, 50, 100 }
  }
};

G_DEFINE_TYPE (GimpRuler, gimp_ruler, GTK_TYPE_WIDGET)

#define GIMP_RULER_GET_PRIVATE(ruler) \
  G_TYPE_INSTANCE_GET_PRIVATE (ruler, GIMP_TYPE_RULER, GimpRulerPrivate)


static void
gimp_ruler_class_init (GimpRulerClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = gimp_ruler_set_property;
  object_class->get_property = gimp_ruler_get_property;

  widget_class->realize       = gimp_ruler_realize;
  widget_class->unrealize     = gimp_ruler_unrealize;
  widget_class->size_allocate = gimp_ruler_size_allocate;
  widget_class->expose_event  = gimp_ruler_expose;

  klass->draw_ticks = NULL;
  klass->draw_pos   = NULL;

  g_type_class_add_private (object_class, sizeof (GimpRulerPrivate));

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
  g_object_class_install_property (object_class,
                                   PROP_METRIC,
                                   g_param_spec_enum ("metric",
                                                      "Metric",
                                                      "The metric used for the ruler",
                                                      GTK_TYPE_METRIC_TYPE,
                                                      GTK_PIXELS,
                                                      GIMP_PARAM_READWRITE));
}

static void
gimp_ruler_init (GimpRuler *ruler)
{
  GimpRulerPrivate *priv = GIMP_RULER_GET_PRIVATE (ruler);

  priv->backing_store = NULL;
  priv->lower         = 0;
  priv->upper         = 0;
  priv->position      = 0;
  priv->max_size      = 0;

  gimp_ruler_set_metric (ruler, GTK_PIXELS);
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
    case PROP_LOWER:
      gimp_ruler_set_range (ruler, g_value_get_double (value), priv->upper,
                            priv->position, priv->max_size);
      break;
    case PROP_UPPER:
      gimp_ruler_set_range (ruler, priv->lower, g_value_get_double (value),
                            priv->position, priv->max_size);
      break;
    case PROP_POSITION:
      gimp_ruler_set_range (ruler, priv->lower, priv->upper,
                            g_value_get_double (value), priv->max_size);
      break;
    case PROP_MAX_SIZE:
      gimp_ruler_set_range (ruler, priv->lower, priv->upper,
                            priv->position,  g_value_get_double (value));
      break;
    case PROP_METRIC:
      gimp_ruler_set_metric (ruler, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_ruler_get_property (GObject      *object,
                         guint         prop_id,
                         GValue       *value,
                         GParamSpec   *pspec)
{
  GimpRuler        *ruler = GIMP_RULER (object);
  GimpRulerPrivate *priv  = GIMP_RULER_GET_PRIVATE (ruler);

  switch (prop_id)
    {
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
    case PROP_METRIC:
      g_value_set_enum (value, gimp_ruler_get_metric (ruler));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
gimp_ruler_set_metric (GimpRuler     *ruler,
                       GtkMetricType  metric)
{
  GimpRulerPrivate *priv;

  g_return_if_fail (GIMP_IS_RULER (ruler));

  priv = GIMP_RULER_GET_PRIVATE (ruler);

  priv->metric = (GimpRulerMetric *) &ruler_metrics[metric];

  if (GTK_WIDGET_DRAWABLE (ruler))
    gtk_widget_queue_draw (GTK_WIDGET (ruler));

  g_object_notify (G_OBJECT (ruler), "metric");
}

/**
 * gimp_ruler_get_metric:
 * @ruler: a #GimpRuler
 *
 * Gets the units used for a #GimpRuler. See gimp_ruler_set_metric().
 *
 * Return value: the units currently used for @ruler
 *
 * Since: GIMP 2.8
 **/
GtkMetricType
gimp_ruler_get_metric (GimpRuler *ruler)
{
  GimpRulerPrivate *priv;
  gint              i;

  g_return_val_if_fail (GIMP_IS_RULER (ruler), 0);

  priv = GIMP_RULER_GET_PRIVATE (ruler);

  for (i = 0; i < G_N_ELEMENTS (ruler_metrics); i++)
    if (priv->metric == &ruler_metrics[i])
      return i;

  g_assert_not_reached ();

  return 0;
}

/**
 * gimp_ruler_set_range:
 * @ruler: the gtkruler
 * @lower: the lower limit of the ruler
 * @upper: the upper limit of the ruler
 * @position: the mark on the ruler
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
                      gdouble    position,
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
  if (priv->position != position)
    {
      priv->position = position;
      g_object_notify (G_OBJECT (ruler), "position");
    }
  if (priv->max_size != max_size)
    {
      priv->max_size = max_size;
      g_object_notify (G_OBJECT (ruler), "max-size");
    }
  g_object_thaw_notify (G_OBJECT (ruler));

  if (GTK_WIDGET_DRAWABLE (ruler))
    gtk_widget_queue_draw (GTK_WIDGET (ruler));
}

/**
 * gimp_ruler_get_range:
 * @ruler: a #GimpRuler
 * @lower: location to store lower limit of the ruler, or %NULL
 * @upper: location to store upper limit of the ruler, or %NULL
 * @position: location to store the current position of the mark on the ruler, or %NULL
 * @max_size: location to store the maximum size of the ruler used when calculating
 *            the space to leave for the text, or %NULL.
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
                      gdouble   *position,
                      gdouble   *max_size)
{
  GimpRulerPrivate *priv;

  g_return_if_fail (GIMP_IS_RULER (ruler));

  priv = GIMP_RULER_GET_PRIVATE (ruler);

  if (lower)
    *lower = priv->lower;
  if (upper)
    *upper = priv->upper;
  if (position)
    *position = priv->position;
  if (max_size)
    *max_size = priv->max_size;
}

void
gimp_ruler_draw_ticks (GimpRuler *ruler)
{
  g_return_if_fail (GIMP_IS_RULER (ruler));

  if (GIMP_RULER_GET_CLASS (ruler)->draw_ticks)
    GIMP_RULER_GET_CLASS (ruler)->draw_ticks (ruler);
}

void
gimp_ruler_draw_pos (GimpRuler *ruler)
{
  g_return_if_fail (GIMP_IS_RULER (ruler));

  if (GIMP_RULER_GET_CLASS (ruler)->draw_pos)
     GIMP_RULER_GET_CLASS (ruler)->draw_pos (ruler);
}


GdkDrawable *
_gimp_ruler_get_backing_store (GimpRuler *ruler)
{
  return GIMP_RULER_GET_PRIVATE (ruler)->backing_store;
}

const GimpRulerMetric *
_gimp_ruler_get_metric (GimpRuler *ruler)
{
  return GIMP_RULER_GET_PRIVATE (ruler)->metric;
}

PangoLayout *
_gimp_ruler_create_pango_layout (GtkWidget   *widget,
                                 const gchar *text)
{
  PangoLayout    *layout;
  PangoAttrList  *attrs;
  PangoAttribute *attr;

  layout = gtk_widget_create_pango_layout (widget, text);

  attrs = pango_attr_list_new ();

  attr = pango_attr_scale_new (PANGO_SCALE_X_SMALL);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);

  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);

  return layout;
}

static void
gimp_ruler_realize (GtkWidget *widget)
{
  GimpRuler     *ruler = GIMP_RULER (widget);
  GdkWindowAttr  attributes;
  gint           attributes_mask;

  GTK_WIDGET_SET_FLAGS (ruler, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x           = widget->allocation.x;
  attributes.y           = widget->allocation.y;
  attributes.width       = widget->allocation.width;
  attributes.height      = widget->allocation.height;
  attributes.wclass      = GDK_INPUT_OUTPUT;
  attributes.visual      = gtk_widget_get_visual (widget);
  attributes.colormap    = gtk_widget_get_colormap (widget);
  attributes.event_mask  = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
                            GDK_POINTER_MOTION_MASK |
                            GDK_POINTER_MOTION_HINT_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                   &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, ruler);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);

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

  if (GTK_WIDGET_CLASS (gimp_ruler_parent_class)->unrealize)
    (* GTK_WIDGET_CLASS (gimp_ruler_parent_class)->unrealize) (widget);
}

static void
gimp_ruler_size_allocate (GtkWidget     *widget,
                          GtkAllocation *allocation)
{
  GimpRuler *ruler = GIMP_RULER (widget);

  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);

      gimp_ruler_make_pixmap (ruler);
    }
}

static gint
gimp_ruler_expose (GtkWidget      *widget,
                   GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GimpRuler        *ruler = GIMP_RULER (widget);
      GimpRulerPrivate *priv  = GIMP_RULER_GET_PRIVATE (ruler);

      gimp_ruler_draw_ticks (ruler);

      gdk_draw_drawable (widget->window,
                         priv->non_gr_exp_gc,
                         priv->backing_store,
                         0, 0, 0, 0,
                         widget->allocation.width,
                         widget->allocation.height);

      gimp_ruler_draw_pos (ruler);
    }

  return FALSE;
}

static void
gimp_ruler_make_pixmap (GimpRuler *ruler)
{
  GimpRulerPrivate *priv   = GIMP_RULER_GET_PRIVATE (ruler);
  GtkWidget        *widget = GTK_WIDGET (ruler);
  gint              width;
  gint              height;


  if (priv->backing_store)
    {
      gdk_drawable_get_size (priv->backing_store, &width, &height);
      if ((width == widget->allocation.width) &&
          (height == widget->allocation.height))
        return;

      g_object_unref (priv->backing_store);
    }

  priv->backing_store = gdk_pixmap_new (widget->window,
                                        widget->allocation.width,
                                        widget->allocation.height,
                                        -1);

  if (!priv->non_gr_exp_gc)
    {
      priv->non_gr_exp_gc = gdk_gc_new (widget->window);
      gdk_gc_set_exposures (priv->non_gr_exp_gc, FALSE);
    }
}
