/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimplabelcolor.c
 * Copyright (C) 2022 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "gimpwidgets.h"
#include "gimpwidgets-private.h"


/**
 * SECTION: gimplabelcolor
 * @title: GimpLabelColor
 * @short_description: Widget containing a color area and a label.
 *
 * This widget is a subclass of #GimpLabeled with a #GtkColor.
 **/

enum
{
  VALUE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_VALUE,
};

typedef struct _GimpLabelColorPrivate
{
  GtkWidget *area;
} GimpLabelColorPrivate;

static void        gimp_label_color_constructed       (GObject       *object);
static void        gimp_label_color_set_property      (GObject       *object,
                                                       guint          property_id,
                                                       const GValue  *value,
                                                       GParamSpec    *pspec);
static void        gimp_label_color_get_property      (GObject       *object,
                                                       guint          property_id,
                                                       GValue        *value,
                                                       GParamSpec    *pspec);

static GtkWidget * gimp_label_color_populate          (GimpLabeled   *color,
                                                       gint          *x,
                                                       gint          *y,
                                                       gint          *width,
                                                       gint          *height);

G_DEFINE_TYPE_WITH_PRIVATE (GimpLabelColor, gimp_label_color, GIMP_TYPE_LABELED)

#define parent_class gimp_label_color_parent_class

static guint gimp_label_color_signals[LAST_SIGNAL] = { 0 };

static void
gimp_label_color_class_init (GimpLabelColorClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GimpLabeledClass *labeled_class = GIMP_LABELED_CLASS (klass);
  GimpRGB           black;

  gimp_label_color_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLabelColorClass, value_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = gimp_label_color_constructed;
  object_class->set_property = gimp_label_color_set_property;
  object_class->get_property = gimp_label_color_get_property;

  labeled_class->populate    = gimp_label_color_populate;

  /**
   * GimpLabelColor:value:
   *
   * The currently set value.
   *
   * Since: 3.0
   **/
  gimp_rgba_set (&black, 0.0, 0.0, 0.0, 1.0);
  g_object_class_install_property (object_class, PROP_VALUE,
                                   gimp_param_spec_rgb ("value",
                                                        "Color",
                                                        "The displayed color",
                                                        TRUE, &black,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_label_color_init (GimpLabelColor *color)
{
  GimpLabelColorPrivate *priv = gimp_label_color_get_instance_private (color);
  GimpRGB                black;

  gimp_rgba_set (&black, 0.0, 0.0, 0.0, 1.0);
  priv->area = gimp_color_area_new (&black, GIMP_COLOR_AREA_SMALL_CHECKS,
                                    GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);

  /* Typically for a labelled color area, a small square next to your
   * label is probably what you want to display.
   */
  gtk_widget_set_size_request (priv->area, 20, 20);
}

static void
gimp_label_color_constructed (GObject *object)
{
  GimpLabelColor        *color = GIMP_LABEL_COLOR (object);
  GimpLabelColorPrivate *priv  = gimp_label_color_get_instance_private (color);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* This is important to make this object into a property widget. It
   * will allow config object to bind the "value" property of this
   * widget, and therefore be updated automatically.
   */
  g_object_bind_property (G_OBJECT (priv->area), "color",
                          G_OBJECT (color),      "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
gimp_label_color_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpLabelColor        *lcolor = GIMP_LABEL_COLOR (object);
  GimpLabelColorPrivate *priv  = gimp_label_color_get_instance_private (lcolor);

  switch (property_id)
    {
    case PROP_VALUE:
        {
          GimpRGB *new_color;
          GimpRGB  color;

          new_color = g_value_get_boxed (value);

          gimp_color_area_get_color (GIMP_COLOR_AREA (priv->area), &color);

          /* Avoid looping forever since we have bound this widget's
           * "value" property with the color button "value" property.
           */
          if (gimp_rgba_distance (&color, new_color) >= GIMP_RGBA_EPSILON)
            {
              gimp_color_area_set_color (GIMP_COLOR_AREA (priv->area), new_color);
              g_signal_emit (object, gimp_label_color_signals[VALUE_CHANGED], 0);
            }
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_label_color_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpLabelColor        *color = GIMP_LABEL_COLOR (object);
  GimpLabelColorPrivate *priv  = gimp_label_color_get_instance_private (color);

  switch (property_id)
    {
    case PROP_VALUE:
        {
          GimpRGB c;

          gimp_color_area_get_color (GIMP_COLOR_AREA( priv->area), &c);

          g_value_set_boxed (value, &c);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
gimp_label_color_populate (GimpLabeled *labeled,
                           gint        *x,
                           gint        *y,
                           gint        *width,
                           gint        *height)
{
  GimpLabelColor        *color = GIMP_LABEL_COLOR (labeled);
  GimpLabelColorPrivate *priv = gimp_label_color_get_instance_private (color);

  gtk_grid_attach (GTK_GRID (color), priv->area, 1, 0, 1, 1);
  /* Make sure the label and color won't be glued next to each other's. */
  gtk_grid_set_column_spacing (GTK_GRID (color),
                               4 * gtk_widget_get_scale_factor (GTK_WIDGET (color)));
  gtk_widget_show (priv->area);

  return priv->area;
}


/* Public Functions */


/**
 * gimp_label_color_new:
 * @label:  The text for the #GtkLabel.
 * @color:  The color displayed.
 *
 * Creates a #GimpLabelColor which contains a widget and displays a
 * color area. By default, the color area is of type
 * %GIMP_COLOR_AREA_SMALL_CHECKS, which means transparency of @color
 * will be shown. Moreover the color is draggable to other widgets
 * accepting color drops with buttons 1 and 2.
 *
 * If you wish to customize any of these default behaviours, get the
 * #GimpColorArea with gimp_label_color_get_color_area().
 *
 * Returns: (transfer full): The new #GimpLabelColor widget.
 **/
GtkWidget *
gimp_label_color_new (const gchar   *label,
                      const GimpRGB *color)
{
  GtkWidget *labeled;

  labeled = g_object_new (GIMP_TYPE_LABEL_COLOR,
                          "label",  label,
                          "value",  color,
                          NULL);

  return labeled;
}

/**
 * gimp_label_color_set_value:
 * @color: The #GtkLabelColor.
 * @value: A new value.
 *
 * This function sets the value in the #GtkColor inside @color.
 **/
void
gimp_label_color_set_value (GimpLabelColor *color,
                            const GimpRGB  *value)
{
  g_return_if_fail (GIMP_IS_LABEL_COLOR (color));

  g_object_set (color,
                "value", value,
                NULL);
}

/**
 * gimp_label_color_get_value:
 * @color: The #GtkLabelColor.
 * @value: (out): The color to assign to the color area.
 *
 * This function returns the value shown by @color.
 **/
void
gimp_label_color_get_value (GimpLabelColor *color,
                            GimpRGB        *value)
{
  GimpLabelColorPrivate *priv = gimp_label_color_get_instance_private (color);

  g_return_if_fail (GIMP_IS_LABEL_COLOR (color));

  gimp_color_area_get_color (GIMP_COLOR_AREA (priv->area), value);
}

/**
 * gimp_label_color_get_color_area:
 * @color: The #GimpLabelColor
 *
 * This function returns the #GimpColorArea packed in @color.
 *
 * Returns: (transfer none): The #GimpColorArea packed in @color.
 **/
GtkWidget *
gimp_label_color_get_color_area (GimpLabelColor *color)
{
  GimpLabelColorPrivate *priv = gimp_label_color_get_instance_private (color);

  g_return_val_if_fail (GIMP_IS_LABEL_COLOR (color), NULL);

  return priv->area;
}
