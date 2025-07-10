/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimplabelstringwidget.c
 * Copyright (C) 2023 Jehan
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

#include "gimplabelstringwidget.h"


/**
 * SECTION: gimplabelstringwidget
 * @title: GimpLabelStringWidget
 * @short_description: Widget containing a label and a widget with a
 *                     string "value" property.
 *
 * This widget is a subclass of #GimpLabeled.
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

struct _GimpLabelStringWidget
{
  GimpLabeled    parent_instance;

  GtkWidget     *widget;
  gchar         *value;
};

static void        gimp_label_string_widget_constructed       (GObject       *object);
static void        gimp_label_string_widget_finalize          (GObject       *object);
static void        gimp_label_string_widget_set_property      (GObject       *object,
                                                               guint          property_id,
                                                               const GValue  *value,
                                                               GParamSpec    *pspec);
static void        gimp_label_string_widget_get_property      (GObject       *object,
                                                               guint          property_id,
                                                               GValue        *value,
                                                               GParamSpec    *pspec);

static GtkWidget * gimp_label_string_widget_populate          (GimpLabeled   *widget,
                                                               gint          *x,
                                                               gint          *y,
                                                               gint          *width,
                                                               gint          *height);

G_DEFINE_TYPE (GimpLabelStringWidget, gimp_label_string_widget, GIMP_TYPE_LABELED)

#define parent_class gimp_label_string_widget_parent_class

static guint gimp_label_string_widget_signals[LAST_SIGNAL] = { 0 };

static void
gimp_label_string_widget_class_init (GimpLabelStringWidgetClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GimpLabeledClass *labeled_class = GIMP_LABELED_CLASS (klass);

  gimp_label_string_widget_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = gimp_label_string_widget_constructed;
  object_class->finalize     = gimp_label_string_widget_finalize;
  object_class->set_property = gimp_label_string_widget_set_property;
  object_class->get_property = gimp_label_string_widget_get_property;

  labeled_class->populate    = gimp_label_string_widget_populate;

  /**
   * GimpLabelStringWidget:value:
   *
   * The currently set value.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_string ("value", NULL,
                                                        "Current value",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  /**
   * GimpLabelStringWidget:widget:
   *
   * The widget holding a string property named "value".
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_WIDGET,
                                   g_param_spec_object ("widget", NULL,
                                                        "String widget",
                                                        GTK_TYPE_WIDGET,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_label_string_widget_init (GimpLabelStringWidget *widget)
{
}

static void
gimp_label_string_widget_constructed (GObject *object)
{
  GimpLabelStringWidget *widget = GIMP_LABEL_STRING_WIDGET (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_grid_set_column_spacing (GTK_GRID (widget), 6);
  gtk_grid_set_row_spacing (GTK_GRID (widget), 6);

  /* This is important to make this object into a property widget. It
   * will allow config object to bind the "value" property of this
   * widget, and therefore be updated automatically.
   */
  g_object_bind_property (G_OBJECT (widget->widget), "value",
                          object,                  "value",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);
}

static void
gimp_label_string_widget_finalize (GObject *object)
{
  GimpLabelStringWidget *widget = GIMP_LABEL_STRING_WIDGET (object);

  g_free (widget->value);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_label_string_widget_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpLabelStringWidget *widget = GIMP_LABEL_STRING_WIDGET (object);

  switch (property_id)
    {
    case PROP_VALUE:
      if (g_strcmp0 (widget->value, g_value_get_string (value)) != 0)
        {
          g_free (widget->value);
          widget->value = g_value_dup_string (value);
          g_signal_emit (object, gimp_label_string_widget_signals[VALUE_CHANGED], 0);
        }
      break;
    case PROP_WIDGET:
      widget->widget = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_label_string_widget_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpLabelStringWidget *widget = GIMP_LABEL_STRING_WIDGET (object);

  switch (property_id)
    {
    case PROP_VALUE:
      g_value_set_string (value, widget->value);
      break;
    case PROP_WIDGET:
      g_value_set_object (value, widget->widget);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
gimp_label_string_widget_populate (GimpLabeled *labeled,
                                   gint        *x,
                                   gint        *y,
                                   gint        *width,
                                   gint        *height)
{
  GimpLabelStringWidget *widget = GIMP_LABEL_STRING_WIDGET (labeled);

  gtk_grid_attach (GTK_GRID (widget), widget->widget, 1, 0, 1, 1);
  gtk_widget_show (widget->widget);

  return widget->widget;
}

/* Public Functions */

/**
 * gimp_label_string_widget_new:
 * @text:   The text for the #GtkLabel.
 * @widget: (transfer full): The #GtkWidget to use.
 *
 * Creates a new #GimpLabelStringWidget whose "value" property is bound to
 * that of @widget (which must therefore have such a string property).
 *
 * Returns: (transfer full): The new #GimpLabelStringWidget widget.
 **/
GtkWidget *
gimp_label_string_widget_new (const gchar *text,
                              GtkWidget   *widget)
{
  GtkWidget  *string_widget;
  GParamSpec *pspec;

  g_return_val_if_fail ((pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (widget),
                                                               "value")) &&
                        (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_STRING ||
                         G_PARAM_SPEC_TYPE (pspec) == GIMP_TYPE_PARAM_CHOICE),
                        NULL);

  string_widget = g_object_new (GIMP_TYPE_LABEL_STRING_WIDGET,
                                "label",  text,
                                "widget", widget,
                                NULL);

  return string_widget;
}

/**
 * gimp_label_string_widget_get_widget:
 * @widget: the #GimpLabelStringWidget.
 *
 * Returns: (transfer none): The new #GtkWidget packed next to the label.
 **/
GtkWidget *
gimp_label_string_widget_get_widget (GimpLabelStringWidget *widget)
{
  g_return_val_if_fail (GIMP_IS_LABEL_STRING_WIDGET (widget), NULL);

  return widget->widget;
}
