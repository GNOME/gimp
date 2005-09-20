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

#include <math.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpzoommodel.h"

#include "libgimp/libgimp-intl.h"

enum
{
  PROP_0,
  PROP_ZOOM_FACTOR,
  PROP_STEP_SIZE,
  PROP_MIN_FACTOR,
  PROP_MAX_FACTOR
};

typedef struct _GimpZoomModelPrivate GimpZoomModelPrivate;

struct _GimpZoomModelPrivate
{
  gdouble  zoom_factor;
  gdouble  step_size;
  gdouble  min_factor;
  gdouble  max_factor;
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

  object_class->set_property  = gimp_zoom_model_set_property;
  object_class->get_property  = gimp_zoom_model_get_property;

  g_object_class_install_property (object_class, PROP_ZOOM_FACTOR,
                                   g_param_spec_double ("zoom-factor",
                                                        _("Zoom factor"), NULL,
                                                        1.0 / 256.0, 256.0,
                                                        1.0,
                                                        G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_STEP_SIZE,
                                   g_param_spec_double ("step-size",
                                                        _("Step size"), NULL,
                                                        1.01, 10.0,
                                                        1.1,
                                                        G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_MIN_FACTOR,
                                   g_param_spec_double ("min-factor",
                                                        _("Min factor"), NULL,
                                                        1.0 / 256.0, 256.0,
                                                        1.0 / 256.0,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_MAX_FACTOR,
                                   g_param_spec_double ("max-factor",
                                                        _("Max factor"), NULL,
                                                        1.0 / 256.0, 256.0,
                                                        256.0,
                                                        G_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (GimpZoomModelPrivate));
}

static void
gimp_zoom_model_init (GimpZoomModel *zoom_model)
{
  GimpZoomModelPrivate *priv = GIMP_ZOOM_MODEL_GET_PRIVATE (zoom_model);

  priv->min_factor = 1.0 / 256.0;
  priv->max_factor = 256.0;
}

static void
gimp_zoom_model_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpZoomModelPrivate *priv  = GIMP_ZOOM_MODEL_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ZOOM_FACTOR:
      priv->zoom_factor = g_value_get_double (value);
      g_object_notify (object, "zoom-factor");
      break;

    case PROP_STEP_SIZE:
      priv->step_size = g_value_get_double (value);
      break;

    case PROP_MIN_FACTOR:
      priv->min_factor = MIN (g_value_get_double (value), priv->max_factor);
      break;

    case PROP_MAX_FACTOR:
      priv->max_factor = MAX (g_value_get_double (value), priv->min_factor);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  if (priv->zoom_factor > priv->max_factor ||
      priv->zoom_factor < priv->min_factor)
    {
      priv->zoom_factor = CLAMP (priv->zoom_factor,
                                 priv->min_factor,
                                 priv->max_factor);
      g_object_notify (object, "zoom-factor");
    }
}

static void
gimp_zoom_model_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpZoomModelPrivate *priv  = GIMP_ZOOM_MODEL_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ZOOM_FACTOR:
      g_value_set_double (value, priv->zoom_factor);
      break;

    case PROP_STEP_SIZE:
      g_value_set_double (value, priv->step_size);
      break;

    case PROP_MIN_FACTOR:
      g_value_set_double (value, priv->min_factor);
      break;

    case PROP_MAX_FACTOR:
      g_value_set_double (value, priv->max_factor);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_zoom_model_zoom_in (GimpZoomModel *zoom_model)
{
  GimpZoomModelPrivate *priv = GIMP_ZOOM_MODEL_GET_PRIVATE (zoom_model);

  g_return_if_fail (GIMP_IS_ZOOM_MODEL (zoom_model));

  if (priv->zoom_factor < priv->max_factor);
    {
      priv->zoom_factor *= priv->step_size;
      g_object_notify (G_OBJECT (zoom_model), "zoom-factor");
    }
}

static void
gimp_zoom_model_zoom_out (GimpZoomModel *zoom_model)
{
  GimpZoomModelPrivate *priv = GIMP_ZOOM_MODEL_GET_PRIVATE (zoom_model);

  g_return_if_fail (GIMP_IS_ZOOM_MODEL (zoom_model));

  if (priv->zoom_factor > priv->min_factor)
    {
      priv->zoom_factor /= priv->step_size;
      g_object_notify (G_OBJECT (zoom_model), "zoom-factor");
    }
}

static void
gimp_zoom_model_update_label (GimpZoomModel *zoom_model,
                              GParamSpec    *pspec,
                              GtkLabel      *label)
{
  gint                  numerator;
  gint                  denominator;
  gchar                *txt;
  GimpZoomModelPrivate *priv  = GIMP_ZOOM_MODEL_GET_PRIVATE (zoom_model);

  g_return_if_fail (GIMP_IS_ZOOM_MODEL (zoom_model));
  g_return_if_fail (GTK_IS_LABEL (label));

  gimp_zoom_model_get_fraction (priv->zoom_factor,
                                &numerator,
                                &denominator);
  txt = g_strdup_printf (_("zoom factor %d:%d"), numerator, denominator);
  gtk_label_set_text (label, txt);
  g_free (txt);
}

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
 * gimp_zoom_model_new:
 *
 * Creates a new #GimpZoomModel.
 *
 * Return value: a new #GimpZoomModel.
 *
 * Since GIMP 2.4
 **/
GimpZoomModel *
gimp_zoom_model_new (gdouble step_size)
{
  GimpZoomModel *zoom_model;

  zoom_model = g_object_new (GIMP_TYPE_ZOOM_MODEL,
                             "step-size", step_size,
                             "zoom-factor", 1.0,
                             NULL);
  return zoom_model;
}

/**
 * gimp_zoom_widget_new:
 *
 * Creates a new widget to interact with the #GimpZoomModel.
 * @zoom_model:
 * @widget:
 *
 * Return value: a new #GtkWidget.
 *
 * Since GIMP 2.4
 **/
GtkWidget *
gimp_zoom_widget_new (GimpZoomModel      *zoom_model,
                      GimpZoomWidgetType  widget)
{
  GtkWidget            *button;
  GtkWidget            *image;
  GtkWidget            *label;
  gint                  numerator;
  gint                  denominator;
  gchar                *txt;
  GimpZoomModelPrivate *priv = GIMP_ZOOM_MODEL_GET_PRIVATE (zoom_model);

  g_return_val_if_fail (GIMP_IS_ZOOM_MODEL (zoom_model), NULL);

  switch (widget)
    {
    case GIMP_ZOOM_IN_BUTTON:
      button = gtk_button_new ();
      image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_IN,
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);
      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK (gimp_zoom_model_zoom_in),
                                zoom_model);
      return button;

    case GIMP_ZOOM_OUT_BUTTON:
      button = gtk_button_new ();
      image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_OUT,
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);
      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK (gimp_zoom_model_zoom_out),
                                zoom_model);
      return button;

    case GIMP_ZOOM_LABEL:
      gimp_zoom_model_get_fraction (priv->zoom_factor,
                                    &numerator,
                                    &denominator);
      txt = g_strdup_printf (_("zoom factor %d:%d"), numerator, denominator);
      label = gtk_label_new (txt);
      g_free (txt);
      gtk_misc_set_padding (GTK_MISC (label), 6, 6);
      g_signal_connect (zoom_model, "notify::zoom-factor",
                        G_CALLBACK (gimp_zoom_model_update_label),
                        label);
      return label;
    }
  return NULL;
}

gdouble
gimp_zoom_model_get_factor (GimpZoomModel *zoom_model)
{
  GimpZoomModelPrivate *priv = GIMP_ZOOM_MODEL_GET_PRIVATE (zoom_model);

  g_return_val_if_fail (GIMP_IS_ZOOM_MODEL (zoom_model), 1.0);

  return priv->zoom_factor;
}

void
gimp_zoom_model_set_range (GimpZoomModel *zoom_model,
                           gdouble        min,
                           gdouble        max)
{
  GimpZoomModelPrivate *priv = GIMP_ZOOM_MODEL_GET_PRIVATE (zoom_model);

  g_return_if_fail (min < max);
  g_return_if_fail (min >= 1.0 / 256.0);
  g_return_if_fail (max <= 256.0);

  priv->min_factor = min;
  priv->max_factor = max;

  if (priv->zoom_factor > priv->max_factor ||
      priv->zoom_factor < priv->min_factor)
    {
      priv->zoom_factor = CLAMP (priv->zoom_factor,
                                 priv->min_factor,
                                 priv->max_factor);
      g_object_notify (G_OBJECT (zoom_model), "zoom-factor");
    }
}

