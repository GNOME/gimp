/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmalabelintwidget.c
 * Copyright (C) 2020 Jehan
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

#include "ligmalabelintwidget.h"


/**
 * SECTION: ligmalabelintwidget
 * @title: LigmaLabelIntWidget
 * @short_description: Widget containing a label and a widget with an
 *                     integer "value" property.
 *
 * This widget is a subclass of #LigmaLabeled.
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
  PROP_WIDGET,
};

typedef struct _LigmaLabelIntWidgetPrivate
{
  LigmaLabeled    parent_instance;

  GtkWidget     *widget;
  gint           value;
} LigmaLabelIntWidgetPrivate;

static void        ligma_label_int_widget_constructed       (GObject       *object);
static void        ligma_label_int_widget_set_property      (GObject       *object,
                                                            guint          property_id,
                                                            const GValue  *value,
                                                            GParamSpec    *pspec);
static void        ligma_label_int_widget_get_property      (GObject       *object,
                                                            guint          property_id,
                                                            GValue        *value,
                                                            GParamSpec    *pspec);

static GtkWidget * ligma_label_int_widget_populate          (LigmaLabeled   *widget,
                                                            gint          *x,
                                                            gint          *y,
                                                            gint          *width,
                                                            gint          *height);

G_DEFINE_TYPE_WITH_PRIVATE (LigmaLabelIntWidget, ligma_label_int_widget, LIGMA_TYPE_LABELED)

#define parent_class ligma_label_int_widget_parent_class

static guint ligma_label_int_widget_signals[LAST_SIGNAL] = { 0 };

static void
ligma_label_int_widget_class_init (LigmaLabelIntWidgetClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  LigmaLabeledClass *labeled_class = LIGMA_LABELED_CLASS (klass);

  ligma_label_int_widget_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLabelIntWidgetClass, value_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = ligma_label_int_widget_constructed;
  object_class->set_property = ligma_label_int_widget_set_property;
  object_class->get_property = ligma_label_int_widget_get_property;

  labeled_class->populate    = ligma_label_int_widget_populate;

  /**
   * LigmaLabelIntWidget:value:
   *
   * The currently set value.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_int ("value", NULL,
                                                     "Current value",
                                                     G_MININT, G_MAXINT, 0,
                                                     LIGMA_PARAM_READWRITE));

  /**
   * LigmaLabelIntWidget:widget:
   *
   * The widget holding an integer value.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_WIDGET,
                                   g_param_spec_object ("widget", NULL,
                                                        "Integer widget",
                                                        GTK_TYPE_WIDGET,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_label_int_widget_init (LigmaLabelIntWidget *widget)
{
}

static void
ligma_label_int_widget_constructed (GObject *object)
{
  LigmaLabelIntWidget        *widget = LIGMA_LABEL_INT_WIDGET (object);
  LigmaLabelIntWidgetPrivate *priv   = ligma_label_int_widget_get_instance_private (widget);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_grid_set_column_spacing (GTK_GRID (widget), 6);
  gtk_grid_set_row_spacing (GTK_GRID (widget), 6);

  /* This is important to make this object into a property widget. It
   * will allow config object to bind the "value" property of this
   * widget, and therefore be updated automatically.
   */
  g_object_bind_property (G_OBJECT (priv->widget), "value",
                          object,                  "value",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);
}

static void
ligma_label_int_widget_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  LigmaLabelIntWidget        *widget = LIGMA_LABEL_INT_WIDGET (object);
  LigmaLabelIntWidgetPrivate *priv   = ligma_label_int_widget_get_instance_private (widget);

  switch (property_id)
    {
    case PROP_VALUE:
      priv->value = g_value_get_int (value);
      g_signal_emit (object, ligma_label_int_widget_signals[VALUE_CHANGED], 0);
      break;
    case PROP_WIDGET:
      priv->widget = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_label_int_widget_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  LigmaLabelIntWidget        *widget = LIGMA_LABEL_INT_WIDGET (object);
  LigmaLabelIntWidgetPrivate *priv   = ligma_label_int_widget_get_instance_private (widget);

  switch (property_id)
    {
    case PROP_VALUE:
      g_value_set_int (value, priv->value);
      break;
    case PROP_WIDGET:
      g_value_set_object (value, priv->widget);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
ligma_label_int_widget_populate (LigmaLabeled *labeled,
                                gint        *x,
                                gint        *y,
                                gint        *width,
                                gint        *height)
{
  LigmaLabelIntWidget        *widget = LIGMA_LABEL_INT_WIDGET (labeled);
  LigmaLabelIntWidgetPrivate *priv   = ligma_label_int_widget_get_instance_private (widget);

  gtk_grid_attach (GTK_GRID (widget), priv->widget, 1, 0, 1, 1);
  gtk_widget_show (priv->widget);

  return priv->widget;
}

/* Public Functions */

/**
 * ligma_label_int_widget_new:
 * @text:   The text for the #GtkLabel.
 * @widget: (transfer full): The #GtkWidget to use.
 *
 * Creates a new #LigmaLabelIntWidget whose "value" property is bound to
 * that of @widget (which must therefore have such an integer property).
 *
 * Returns: (transfer full): The new #LigmaLabelIntWidget widget.
 **/
GtkWidget *
ligma_label_int_widget_new (const gchar *text,
                           GtkWidget   *widget)
{
  GtkWidget  *int_widget;
  GParamSpec *pspec;

  g_return_val_if_fail ((pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (widget),
                                                               "value")) &&
                        G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_INT,
                        NULL);

  int_widget = g_object_new (LIGMA_TYPE_LABEL_INT_WIDGET,
                             "label",  text,
                             "widget", widget,
                             NULL);

  return int_widget;
}

/**
 * ligma_label_int_widget_get_widget:
 * @widget: the #LigmaLabelIntWidget.
 *
 * Returns: (transfer none): The new #GtkWidget packed next to the label.
 **/
GtkWidget *
ligma_label_int_widget_get_widget (LigmaLabelIntWidget *widget)
{
  LigmaLabelIntWidgetPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_LABEL_INT_WIDGET (widget), NULL);

  priv = ligma_label_int_widget_get_instance_private (widget);

  return priv->widget;
}
