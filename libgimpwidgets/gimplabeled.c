/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimplabeled.c
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

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "gimpwidgets.h"


/**
 * SECTION: gimplabeled
 * @title: GimpLabeled
 * @short_description: Widget containing a label as mnemonic for another
 *                     widget.
 *
 * This widget is a #GtkGrid showing a #GtkLabel used as mnemonic on
 * another widget.
 **/

enum
{
  MNEMONIC_WIDGET_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_LABEL,
};

typedef struct _GimpLabeledPrivate
{
  GtkWidget *label;
} GimpLabeledPrivate;


static void       gimp_labeled_constructed            (GObject       *object);
static void       gimp_labeled_set_property           (GObject       *object,
                                                       guint          property_id,
                                                       const GValue  *value,
                                                       GParamSpec    *pspec);
static void       gimp_labeled_get_property           (GObject       *object,
                                                       guint          property_id,
                                                       GValue        *value,
                                                       GParamSpec    *pspec);

static void gimp_labeled_real_mnemonic_widget_changed (GimpLabeled   *labeled,
                                                       GtkWidget     *widget);


G_DEFINE_TYPE_WITH_PRIVATE (GimpLabeled, gimp_labeled, GTK_TYPE_GRID)

#define parent_class gimp_labeled_parent_class

static guint signals[LAST_SIGNAL] = { 0 };

static void
gimp_labeled_class_init (GimpLabeledClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_labeled_constructed;
  object_class->set_property = gimp_labeled_set_property;
  object_class->get_property = gimp_labeled_get_property;

  signals[MNEMONIC_WIDGET_CHANGED] =
    g_signal_new ("mnemonic-widget-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLabeledClass, mnemonic_widget_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GTK_TYPE_WIDGET);

  klass->mnemonic_widget_changed = gimp_labeled_real_mnemonic_widget_changed;

  /**
   * GimpLabeled:label:
   *
   * Label text with pango markup and mnemonic.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        "Label text",
                                                        "The text of the label part of this widget",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_labeled_init (GimpLabeled *labeled)
{
}

static void
gimp_labeled_constructed (GObject *object)
{
  GimpLabeledClass   *klass;
  GimpLabeled        *labeled = GIMP_LABELED (object);
  GimpLabeledPrivate *priv    = gimp_labeled_get_instance_private (labeled);
  GtkWidget          *mnemonic_widget;
  gint                x       = 0;
  gint                y       = 0;
  gint                width   = 1;
  gint                height  = 1;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  priv->label = gtk_label_new_with_mnemonic (NULL);
  gtk_label_set_xalign (GTK_LABEL (priv->label), 0.0);

  klass = GIMP_LABELED_GET_CLASS (labeled);
  g_return_if_fail (klass->populate);
  mnemonic_widget = klass->populate (labeled, &x, &y, &width, &height);

  g_signal_emit (object, signals[MNEMONIC_WIDGET_CHANGED], 0,
                 mnemonic_widget);

  gtk_grid_attach (GTK_GRID (labeled), priv->label, x, y, width, height);
  gtk_widget_show (priv->label);
}

static void
gimp_labeled_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpLabeled        *entry = GIMP_LABELED (object);
  GimpLabeledPrivate *priv  = gimp_labeled_get_instance_private (entry);

  switch (property_id)
    {
    case PROP_LABEL:
        {
          /* This should not happen since the property is **not** set with
           * G_PARAM_CONSTRUCT, hence the label should exist when the
           * property is first set.
           */
          g_return_if_fail (priv->label);

          gtk_label_set_markup_with_mnemonic (GTK_LABEL (priv->label),
                                              g_value_get_string (value));
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_labeled_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpLabeled        *entry = GIMP_LABELED (object);
  GimpLabeledPrivate *priv  = gimp_labeled_get_instance_private (entry);

  switch (property_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, gtk_label_get_label (GTK_LABEL (priv->label)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_labeled_real_mnemonic_widget_changed (GimpLabeled *labeled,
                                           GtkWidget   *widget)
{
  GimpLabeledPrivate *priv  = gimp_labeled_get_instance_private (labeled);

  gtk_label_set_mnemonic_widget (GTK_LABEL (priv->label), widget);
}

/* Public functions */

/**
 * gimp_labeled_get_label:
 * @labeled: The #GimpLabeled.
 *
 * This function returns the #GtkLabel packed in @labeled. This can be
 * useful if you need to customize some aspects of the widget.
 *
 * Returns: (transfer none) (type GtkLabel): The #GtkLabel contained in @labeled.
 **/
GtkWidget *
gimp_labeled_get_label (GimpLabeled *labeled)
{
  GimpLabeledPrivate *priv = gimp_labeled_get_instance_private (labeled);

  g_return_val_if_fail (GIMP_IS_LABELED (labeled), NULL);

  return priv->label;
}

/**
 * gimp_labeled_get_text:
 * @labeled: the #GimpLabeled.
 *
 * This function will return exactly what you entered with
 * gimp_labeled_set_text() or through the "label" property because this
 * class expects labels to have mnemonics (and allows Pango formatting).
 * To obtain instead the text as displayed with mnemonics and markup
 * removed, call:
 * |[<!-- language="C" -->
 * gtk_label_get_text (GTK_LABEL (gimp_labeled_get_label (@labeled)));
 * ]|
 *
 * Returns: the label text as entered, which includes pango markup and
 *          mnemonics similarly to gtk_label_get_label().
 */
const gchar *
gimp_labeled_get_text (GimpLabeled *labeled)
{
  GimpLabeledPrivate *priv = gimp_labeled_get_instance_private (labeled);

  g_return_val_if_fail (GIMP_IS_LABELED (labeled), NULL);

  return gtk_label_get_label (GTK_LABEL (priv->label));
}

/**
 * gimp_labeled_set_text:
 * @labeled: the #GimpLabeled.
 * @text: label text with Pango markup and mnemonic.
 *
 * This is the equivalent of running
 * gtk_label_set_markup_with_mnemonic() on the #GtkLabel as a
 * #GimpLabeled expects a mnemonic. Pango markup are also allowed.
 */
void
gimp_labeled_set_text (GimpLabeled *labeled,
                       const gchar *text)
{
  GimpLabeledPrivate *priv = gimp_labeled_get_instance_private (labeled);

  g_return_if_fail (GIMP_IS_LABELED (labeled));

  gtk_label_set_markup_with_mnemonic (GTK_LABEL (priv->label), text);
}
