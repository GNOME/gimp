/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpzoommodel.c
 * Copyright (C) 2005  David Odin <dindinx@gimp.org>
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

#include "libgimpmath/gimpmath.h"

#include "gimpzoommodel.h"


#define ZOOM_MIN  (1.0 / 256.0)
#define ZOOM_MAX  (256.0)

enum
{
  PROP_0,
  PROP_VALUE,
  PROP_MINIMUM,
  PROP_MAXIMUM,
  PROP_FRACTION,
  PROP_PERCENTAGE
};

typedef struct _GimpZoomModelPrivate GimpZoomModelPrivate;

struct _GimpZoomModelPrivate
{
  gdouble  value;
  gdouble  minimum;
  gdouble  maximum;
};

#define GIMP_ZOOM_MODEL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIMP_TYPE_ZOOM_MODEL, GimpZoomModelPrivate))

static void  gimp_zoom_model_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);
static void  gimp_zoom_model_get_property (GObject      *object,
                                           guint         property_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);

G_DEFINE_TYPE (GimpZoomModel, gimp_zoom_model, G_TYPE_OBJECT);
#define parent_class gimp_zoom_model_parent_class

static void
gimp_zoom_model_class_init (GimpZoomModelClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_zoom_model_set_property;
  object_class->get_property = gimp_zoom_model_get_property;

  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_double ("value",
                                                        "Value", NULL,
                                                        ZOOM_MIN, ZOOM_MAX,
                                                        1.0,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_MINIMUM,
                                   g_param_spec_double ("minimum",
                                                        "Minimum", NULL,
                                                        ZOOM_MIN, ZOOM_MAX,
                                                        ZOOM_MIN,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_MAXIMUM,
                                   g_param_spec_double ("maximum",
                                                        "Maximum", NULL,
                                                        ZOOM_MIN, ZOOM_MAX,
                                                        ZOOM_MAX,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FRACTION,
                                   g_param_spec_string ("fraction",
                                                        "Fraction", NULL,
                                                        "1:1",
                                                        G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_PERCENTAGE,
                                   g_param_spec_string ("percentage",
                                                        "Percentage", NULL,
                                                        "100%",
                                                        G_PARAM_READABLE));

  g_type_class_add_private (object_class, sizeof (GimpZoomModelPrivate));
}

static void
gimp_zoom_model_init (GimpZoomModel *model)
{
  GimpZoomModelPrivate *priv = GIMP_ZOOM_MODEL_GET_PRIVATE (model);

  priv->value   = 1.0;
  priv->minimum = 1.0 / 256.0;
  priv->maximum = 256.0;
}

static void
gimp_zoom_model_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpZoomModelPrivate *priv  = GIMP_ZOOM_MODEL_GET_PRIVATE (object);

  g_object_freeze_notify (object);

  switch (property_id)
    {
    case PROP_VALUE:
      priv->value = g_value_get_double (value);

      g_object_notify (object, "fraction");
      g_object_notify (object, "percentage");
      break;

    case PROP_MINIMUM:
      priv->minimum = MIN (g_value_get_double (value), priv->maximum);
      break;

    case PROP_MAXIMUM:
      priv->maximum = MAX (g_value_get_double (value), priv->minimum);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  if (priv->value > priv->maximum || priv->value < priv->minimum)
    {
      priv->value = CLAMP (priv->value, priv->minimum, priv->maximum);

      g_object_notify (object, "value");
      g_object_notify (object, "fraction");
      g_object_notify (object, "percentage");
    }

  g_object_thaw_notify (object);
}

static void
gimp_zoom_model_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpZoomModelPrivate *priv  = GIMP_ZOOM_MODEL_GET_PRIVATE (object);
  gint                  numerator;
  gint                  denominator;
  gchar                *tmp;

  switch (property_id)
    {
    case PROP_VALUE:
      g_value_set_double (value, priv->value);
      break;

    case PROP_MINIMUM:
      g_value_set_double (value, priv->minimum);
      break;

    case PROP_MAXIMUM:
      g_value_set_double (value, priv->maximum);
      break;

    case PROP_FRACTION:
      gimp_zoom_model_get_fraction (priv->value, &numerator, &denominator);

      tmp = g_strdup_printf ("%d:%d", numerator, denominator);
      g_value_set_string (value, tmp);
      g_free (tmp);
      break;

    case PROP_PERCENTAGE:
      tmp = g_strdup_printf ("%d%%", (gint) (priv->value * 100.0));
      g_value_set_string (value, tmp);
      g_free (tmp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_zoom_model_zoom_in (GimpZoomModel *model)
{
  GimpZoomModelPrivate *priv = GIMP_ZOOM_MODEL_GET_PRIVATE (model);

  g_return_if_fail (GIMP_IS_ZOOM_MODEL (model));

  if (priv->value < priv->maximum);
    {
      gdouble scale = gimp_zoom_model_zoom_step (GIMP_ZOOM_IN, priv->value);

      priv->value = CLAMP (scale, priv->minimum, priv->maximum);

      g_object_notify (G_OBJECT (model), "value");
      g_object_notify (G_OBJECT (model), "fraction");
      g_object_notify (G_OBJECT (model), "percentage");
    }
}

static void
gimp_zoom_model_zoom_out (GimpZoomModel *model)
{
  GimpZoomModelPrivate *priv = GIMP_ZOOM_MODEL_GET_PRIVATE (model);

  g_return_if_fail (GIMP_IS_ZOOM_MODEL (model));

  if (priv->value > priv->minimum)
    {
      gdouble scale = gimp_zoom_model_zoom_step (GIMP_ZOOM_OUT, priv->value);

      priv->value = CLAMP (scale, priv->minimum, priv->maximum);

      g_object_notify (G_OBJECT (model), "value");
      g_object_notify (G_OBJECT (model), "fraction");
      g_object_notify (G_OBJECT (model), "percentage");
    }
}

/**
 * gimp_zoom_model_new:
 *
 * Creates a new #GimpZoomModel.
 *
 * Return value: a new #GimpZoomModel.
 *
 * Since GIMP 2.4
 **/
GimpZoomModel *
gimp_zoom_model_new (void)
{
  return g_object_new (GIMP_TYPE_ZOOM_MODEL, NULL);
}


/**
 * gimp_zoom_model_set_range:
 * @model: a #GimpZoomModel
 * @min: new lower limit for zoom factor
 * @max: new upper limit for zoom factor
 *
 * Sets the allowed range of the @model.
 *
 * Since GIMP 2.4
 **/
void
gimp_zoom_model_set_range (GimpZoomModel *model,
                           gdouble        min,
                           gdouble        max)
{
  g_return_if_fail (min < max);
  g_return_if_fail (min >= ZOOM_MIN);
  g_return_if_fail (max <= ZOOM_MAX);

  g_object_set (model,
		"minimum", min,
		"maximum", max,
		NULL);
}

/**
 * gimp_zoom_model_get_factor:
 * @model: a #GimpZoomModel
 *
 * Retrieve the current zoom factor of @model.
 *
 * Return value: the current scale factor
 *
 * Since GIMP 2.4
 **/
gdouble
gimp_zoom_model_get_factor (GimpZoomModel *model)
{
  GimpZoomModelPrivate *priv = GIMP_ZOOM_MODEL_GET_PRIVATE (model);

  g_return_val_if_fail (GIMP_IS_ZOOM_MODEL (model), 1.0);

  return priv->value;
}

/**
 * gimp_zoom_widget_new:
 * @model: a #GimpZoomModel
 * @type: the type of widget to create
 *
 * Creates a new widget to interact with the #GimpZoomModel.
 *
 * Return value: a new #GtkWidget.
 *
 * Since GIMP 2.4
 **/
GtkWidget *
gimp_zoom_widget_new (GimpZoomModel      *model,
                      GimpZoomWidgetType  type)
{
  GtkWidget *button;
  GtkWidget *image;

  g_return_val_if_fail (GIMP_IS_ZOOM_MODEL (model), NULL);

  switch (type)
    {
    case GIMP_ZOOM_IN_BUTTON:
      button = gtk_button_new ();
      image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_IN,
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);
      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK (gimp_zoom_model_zoom_in),
                                model);
      return button;

    case GIMP_ZOOM_OUT_BUTTON:
      button = gtk_button_new ();
      image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_OUT,
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);
      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK (gimp_zoom_model_zoom_out),
                                model);
      return button;
    }

  return NULL;
}

/**
 * gimp_zoom_model_get_fraction
 * @zoom_factor: a scale factor
 * @numerator: return location for numerator
 * @denominator: return location for denominator
 *
 * Utility function that expresses @zoom_factor as a fraction.
 *
 * Since GIMP 2.4
 **/
void
gimp_zoom_model_get_fraction (gdouble  zoom_factor,
                              gint    *numerator,
                              gint    *denominator)
{
  gint     p0, p1, p2;
  gint     q0, q1, q2;
  gdouble  remainder, next_cf;
  gboolean swapped = FALSE;

  g_return_if_fail (numerator != NULL && denominator != NULL);

  /* make sure that zooming behaves symmetrically */
  if (zoom_factor < 1.0)
    {
      zoom_factor = 1.0 / zoom_factor;
      swapped = TRUE;
    }

  /* calculate the continued fraction for the desired zoom factor */

  p0 = 1;
  q0 = 0;
  p1 = floor (zoom_factor);
  q1 = 1;

  remainder = zoom_factor - p1;

  while (fabs (remainder) >= 0.0001 &&
         fabs (((gdouble) p1 / q1) - zoom_factor) > 0.0001)
    {
      remainder = 1.0 / remainder;

      next_cf = floor (remainder);

      p2 = next_cf * p1 + p0;
      q2 = next_cf * q1 + q0;

      /* Numerator and Denominator are limited by 256 */
      /* also absurd ratios like 170:171 are excluded */
      if (p2 > 256 || q2 > 256 || (p2 > 1 && q2 > 1 && p2 * q2 > 200))
        break;

      /* remember the last two fractions */
      p0 = p1;
      p1 = p2;
      q0 = q1;
      q1 = q2;

      remainder = remainder - next_cf;
    }

  zoom_factor = (gdouble) p1 / q1;

  /* hard upper and lower bounds for zoom ratio */

  if (zoom_factor > 256.0)
    {
      p1 = 256;
      q1 = 1;
    }
  else if (zoom_factor < 1.0 / 256.0)
    {
      p1 = 1;
      q1 = 256;
    }

  if (swapped)
    {
      *numerator = q1;
      *denominator = p1;
    }
  else
    {
      *numerator = p1;
      *denominator = q1;
    }
}

/**
 * gimp_zoom_model_zoom_step:
 * @zoom_type:
 * @scale: ignored unless @zoom_type == %GIMP_ZOOM_TO
 *
 * Utility function to calculate a new scale factor.
 *
 * Return value: the new scale factor
 *
 * Since GIMP 2.4
 **/
gdouble
gimp_zoom_model_zoom_step (GimpZoomType zoom_type,
                           gdouble      scale)
{
  gint    i, n_presets;
  gdouble new_scale = 1.0;

  /* This table is constructed to have fractions, that approximate
   * sqrt(2)^k. This gives a smooth feeling regardless of the starting
   * zoom level.
   *
   * Zooming in/out always jumps to a zoom step from the list above.
   * However, we try to guarantee a certain size of the step, to
   * avoid silly jumps from 101% to 100%.
   * The factor 1.1 is chosen a bit arbitrary, but feels better
   * than the geometric median of the zoom steps (2^(1/4)).
   */

#define ZOOM_MIN_STEP 1.1

  gdouble presets[] = {
    1.0 / 256, 1.0 / 180, 1.0 / 128, 1.0 / 90,
    1.0 / 64,  1.0 / 45,  1.0 / 32,  1.0 / 23,
    1.0 / 16,  1.0 / 11,  1.0 / 8,   2.0 / 11,
    1.0 / 4,   1.0 / 3,   1.0 / 2,   2.0 / 3,
      1.0,
               3.0 / 2,      2.0,      3.0,
      4.0,    11.0 / 2,      8.0,     11.0,
      16.0,     23.0,       32.0,     45.0,
      64.0,     90.0,      128.0,    180.0,
      256.0,
  };

  n_presets = G_N_ELEMENTS (presets);

  switch (zoom_type)
    {
    case GIMP_ZOOM_IN:
      scale *= ZOOM_MIN_STEP;

      new_scale = presets[n_presets-1];

      for (i = n_presets - 1; i >= 0 && presets[i] > scale; i--)
        new_scale = presets[i];

      break;

    case GIMP_ZOOM_OUT:
      scale /= ZOOM_MIN_STEP;

      new_scale = presets[0];

      for (i = 0; i < n_presets && presets[i] < scale; i++)
        new_scale = presets[i];

      break;

    case GIMP_ZOOM_IN_MORE:
      scale = gimp_zoom_model_zoom_step (GIMP_ZOOM_IN, scale);
      scale = gimp_zoom_model_zoom_step (GIMP_ZOOM_IN, scale);
      scale = gimp_zoom_model_zoom_step (GIMP_ZOOM_IN, scale);
      new_scale = scale;
      break;

    case GIMP_ZOOM_OUT_MORE:
      scale = gimp_zoom_model_zoom_step (GIMP_ZOOM_IN, scale);
      scale = gimp_zoom_model_zoom_step (GIMP_ZOOM_IN, scale);
      scale = gimp_zoom_model_zoom_step (GIMP_ZOOM_IN, scale);
      new_scale = scale;
      break;

    case GIMP_ZOOM_IN_MAX:
      new_scale = ZOOM_MAX;
      break;

    case GIMP_ZOOM_OUT_MAX:
      new_scale = ZOOM_MIN;
      break;

    case GIMP_ZOOM_TO:
      new_scale = scale;
      break;
    }

  return CLAMP (new_scale, ZOOM_MIN, ZOOM_MAX);

#undef ZOOM_MIN_STEP
}
