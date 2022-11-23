/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmalabelcolor.c
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

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"

#include "ligmawidgets.h"
#include "ligmawidgets-private.h"


/**
 * SECTION: ligmalabelcolor
 * @title: LigmaLabelColor
 * @short_description: Widget containing a color widget and a label.
 *
 * This widget is a subclass of #LigmaLabeled with a #GtkColor.
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
  PROP_EDITABLE,
  N_PROPS
};
static GParamSpec *object_props[N_PROPS] = { NULL, };


typedef struct _LigmaLabelColorPrivate
{
  GtkWidget *area;
  gboolean   editable;
} LigmaLabelColorPrivate;

static void        ligma_label_color_constructed       (GObject       *object);
static void        ligma_label_color_set_property      (GObject       *object,
                                                       guint          property_id,
                                                       const GValue  *value,
                                                       GParamSpec    *pspec);
static void        ligma_label_color_get_property      (GObject       *object,
                                                       guint          property_id,
                                                       GValue        *value,
                                                       GParamSpec    *pspec);

static GtkWidget * ligma_label_color_populate          (LigmaLabeled   *color,
                                                       gint          *x,
                                                       gint          *y,
                                                       gint          *width,
                                                       gint          *height);

G_DEFINE_TYPE_WITH_PRIVATE (LigmaLabelColor, ligma_label_color, LIGMA_TYPE_LABELED)

#define parent_class ligma_label_color_parent_class

static guint ligma_label_color_signals[LAST_SIGNAL] = { 0 };

static void
ligma_label_color_class_init (LigmaLabelColorClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  LigmaLabeledClass *labeled_class = LIGMA_LABELED_CLASS (klass);
  LigmaRGB           black;

  ligma_label_color_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLabelColorClass, value_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = ligma_label_color_constructed;
  object_class->set_property = ligma_label_color_set_property;
  object_class->get_property = ligma_label_color_get_property;

  labeled_class->populate    = ligma_label_color_populate;

  /**
   * LigmaLabelColor:value:
   *
   * The currently set value.
   *
   * Since: 3.0
   **/
  ligma_rgba_set (&black, 0.0, 0.0, 0.0, 1.0);
  object_props[PROP_VALUE] = ligma_param_spec_rgb ("value",
                                                  "Color",
                                                  "The displayed color",
                                                  TRUE, &black,
                                                  LIGMA_PARAM_READWRITE    |
                                                  G_PARAM_CONSTRUCT);

  /**
   * LigmaLabelColor:editable:
   *
   * Whether the color can be edited.
   *
   * Since: 3.0
   **/
  object_props[PROP_EDITABLE] = g_param_spec_boolean ("editable",
                                                      "Whether the color can be edited",
                                                      "Whether the color can be edited",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, object_props);
}

static void
ligma_label_color_init (LigmaLabelColor *color)
{
  LigmaLabelColorPrivate *priv = ligma_label_color_get_instance_private (color);
  LigmaRGB                black;

  ligma_rgba_set (&black, 0.0, 0.0, 0.0, 1.0);
  priv->editable = FALSE;
  priv->area = ligma_color_area_new (&black, LIGMA_COLOR_AREA_SMALL_CHECKS,
                                    GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);

  /* Typically for a labelled color area, a small square next to your
   * label is probably what you want to display.
   */
  gtk_widget_set_size_request (priv->area, 20, 20);
}

static void
ligma_label_color_constructed (GObject *object)
{
  LigmaLabelColor        *color = LIGMA_LABEL_COLOR (object);
  LigmaLabelColorPrivate *priv  = ligma_label_color_get_instance_private (color);

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
ligma_label_color_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaLabelColor        *lcolor = LIGMA_LABEL_COLOR (object);
  LigmaLabelColorPrivate *priv  = ligma_label_color_get_instance_private (lcolor);

  switch (property_id)
    {
    case PROP_VALUE:
        {
          LigmaRGB *new_color;
          LigmaRGB *color;

          new_color = g_value_get_boxed (value);

          g_object_get (priv->area,
                        "color", &color,
                        NULL);

          /* Avoid looping forever since we have bound this widget's
           * "value" property with the color button "value" property.
           */
          if (ligma_rgba_distance (color, new_color) >= LIGMA_RGBA_EPSILON)
            {
              g_object_set (priv->area, "color", new_color, NULL);
              g_signal_emit (object, ligma_label_color_signals[VALUE_CHANGED], 0);
            }
          g_boxed_free (LIGMA_TYPE_RGB, color);
        }
      break;
    case PROP_EDITABLE:
      if (priv->editable != g_value_get_boolean (value))
        {
          const gchar       *dialog_title;
          LigmaLabeled       *labeled;
          LigmaRGB           *color;
          LigmaColorAreaType  type;
          gboolean           attached;

          labeled = LIGMA_LABELED (lcolor);
          /* Reuse the label contents (without mnemonics) as dialog
           * title for the color selection. This is why the "editable"
           * property must not be a G_PARAM_CONSTRUCT.
           */
          dialog_title = gtk_label_get_text (GTK_LABEL (ligma_labeled_get_label (labeled)));

          attached = (gtk_widget_get_parent (priv->area) != NULL);
          g_object_get (priv->area,
                        "type",  &type,
                        "color", &color,
                        NULL);

          gtk_widget_destroy (priv->area);

          priv->editable = g_value_get_boolean (value);
          if (priv->editable)
            priv->area = ligma_color_button_new (dialog_title,
                                                20, 20, color, type);
          else
            priv->area = ligma_color_area_new (color, type,
                                              GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);

          g_boxed_free (LIGMA_TYPE_RGB, color);

          gtk_widget_set_size_request (priv->area, 20, 20);
          g_object_bind_property (G_OBJECT (priv->area), "color",
                                  G_OBJECT (lcolor),     "value",
                                  G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

          if (attached)
            {
              gtk_grid_attach (GTK_GRID (lcolor), priv->area, 1, 0, 1, 1);
              gtk_widget_show (priv->area);
              g_signal_emit_by_name (object, "mnemonic-widget-changed", priv->area);
            }
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_label_color_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaLabelColor        *color = LIGMA_LABEL_COLOR (object);
  LigmaLabelColorPrivate *priv  = ligma_label_color_get_instance_private (color);

  switch (property_id)
    {
    case PROP_VALUE:
      g_object_get_property (G_OBJECT (priv->area), "color", value);
      break;
    case PROP_EDITABLE:
      g_value_set_boolean (value, priv->editable);
      break;


    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
ligma_label_color_populate (LigmaLabeled *labeled,
                           gint        *x,
                           gint        *y,
                           gint        *width,
                           gint        *height)
{
  LigmaLabelColor        *color = LIGMA_LABEL_COLOR (labeled);
  LigmaLabelColorPrivate *priv = ligma_label_color_get_instance_private (color);

  gtk_grid_attach (GTK_GRID (color), priv->area, 1, 0, 1, 1);
  /* Make sure the label and color won't be glued next to each other's. */
  gtk_grid_set_column_spacing (GTK_GRID (color),
                               4 * gtk_widget_get_scale_factor (GTK_WIDGET (color)));
  gtk_widget_show (priv->area);

  return priv->area;
}


/* Public Functions */


/**
 * ligma_label_color_new:
 * @label:  The text for the #GtkLabel.
 * @color:  The color displayed.
 *
 * Creates a #LigmaLabelColor which contains a widget and displays a
 * color area. By default, the color area is of type
 * %LIGMA_COLOR_AREA_SMALL_CHECKS, which means transparency of @color
 * will be shown.
 *
 * Moreover in the non-editable case, the color is draggable to other
 * widgets accepting color drops with buttons 1 and 2.
 * In the editable case, the @label is reused as the color chooser's
 * dialog title.
 *
 * If you wish to customize any of these default behaviours, get the
 * #LigmaColorArea or #LigmaColorButton with ligma_label_color_get_color_widget().
 *
 * Returns: (transfer full): The new #LigmaLabelColor widget.
 **/
GtkWidget *
ligma_label_color_new (const gchar   *label,
                      const LigmaRGB *color,
                      gboolean       editable)
{
  GtkWidget *labeled;

  labeled = g_object_new (LIGMA_TYPE_LABEL_COLOR,
                          "label",    label,
                          "value",    color,
                          "editable", editable,
                          NULL);

  return labeled;
}

/**
 * ligma_label_color_set_value:
 * @color: The #GtkLabelColor.
 * @value: A new value.
 *
 * This function sets the value in the #GtkColor inside @color.
 **/
void
ligma_label_color_set_value (LigmaLabelColor *color,
                            const LigmaRGB  *value)
{
  g_return_if_fail (LIGMA_IS_LABEL_COLOR (color));

  g_object_set (color,
                "value", value,
                NULL);
}

/**
 * ligma_label_color_get_value:
 * @color: The #GtkLabelColor.
 * @value: (out callee-allocates): The color to assign to the color area.
 *
 * This function returns the value shown by @color.
 **/
void
ligma_label_color_get_value (LigmaLabelColor *color,
                            LigmaRGB        *value)
{
  LigmaLabelColorPrivate *priv = ligma_label_color_get_instance_private (color);

  g_return_if_fail (LIGMA_IS_LABEL_COLOR (color));

  g_object_get (priv->area,
                "color", &value,
                NULL);
}

/**
 * ligma_label_color_set_editable:
 * @color:    The #GtkLabelColor.
 * @editable: Whether the color should be editable.
 *
 * Changes the editability of the color.
 **/
void
ligma_label_color_set_editable (LigmaLabelColor *color,
                               gboolean        editable)
{
  g_return_if_fail (LIGMA_IS_LABEL_COLOR (color));

  g_object_set (color, "editable", editable, NULL);
}

/**
 * ligma_label_color_is_editable:
 * @color: The #GtkLabelColor.
 *
 * This function tells whether the color widget allows to edit the
 * color.
 * Returns: %TRUE if the color is editable.
 **/
gboolean
ligma_label_color_is_editable (LigmaLabelColor *color)
{
  LigmaLabelColorPrivate *priv = ligma_label_color_get_instance_private (color);

  g_return_val_if_fail (LIGMA_IS_LABEL_COLOR (color), FALSE);

  priv = ligma_label_color_get_instance_private (color);

  return LIGMA_IS_COLOR_SELECT (priv->area);
}

/**
 * ligma_label_color_get_color_widget:
 * @color: The #LigmaLabelColor
 *
 * This function returns the color widget packed in @color, which can be
 * either a #LigmaColorButton (if the @color is editable) or a
 * #LigmaColorArea otherwise.
 *
 * Returns: (transfer none): The color widget packed in @color.
 **/
GtkWidget *
ligma_label_color_get_color_widget (LigmaLabelColor *color)
{
  LigmaLabelColorPrivate *priv = ligma_label_color_get_instance_private (color);

  g_return_val_if_fail (LIGMA_IS_LABEL_COLOR (color), NULL);

  return priv->area;
}
