/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenumlabel.c
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpenumlabel.h"


/**
 * SECTION: gimpenumlabel
 * @title: GimpEnumLabel
 * @short_description: A #GtkLabel subclass that displays an enum value.
 *
 * A #GtkLabel subclass that displays an enum value.
 **/


enum
{
  PROP_0,
  PROP_ENUM_TYPE,
  PROP_ENUM_VALUE
};


static void   gimp_enum_label_finalize     (GObject      *object);
static void   gimp_enum_label_get_property (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);
static void   gimp_enum_label_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);


G_DEFINE_TYPE (GimpEnumLabel, gimp_enum_label, GTK_TYPE_LABEL)

#define parent_class gimp_enum_label_parent_class


static void
gimp_enum_label_class_init (GimpEnumLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_enum_label_finalize;
  object_class->get_property = gimp_enum_label_get_property;
  object_class->set_property = gimp_enum_label_set_property;

  /**
   * GimpEnumLabel:enum-type:
   *
   * The #GType of the enum.
   *
   * Since: 2.8
   **/
  g_object_class_install_property (object_class, PROP_ENUM_TYPE,
                                   g_param_spec_gtype ("enum-type",
                                                       "Enum Type",
                                                       "The type of the displayed enum",
                                                       G_TYPE_NONE,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  /**
   * GimpEnumLabel:enum-value:
   *
   * The value to display.
   *
   * Since: 2.8
   **/
  g_object_class_install_property (object_class, PROP_ENUM_VALUE,
                                   g_param_spec_int ("enum-value",
                                                     "Enum Value",
                                                     "The enum value to display",
                                                     G_MININT, G_MAXINT, 0,
                                                     GIMP_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));
}

static void
gimp_enum_label_init (GimpEnumLabel *enum_label)
{
}

static void
gimp_enum_label_finalize (GObject *object)
{
  GimpEnumLabel *enum_label = GIMP_ENUM_LABEL (object);

  g_clear_pointer (&enum_label->enum_class, g_type_class_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_enum_label_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpEnumLabel *label = GIMP_ENUM_LABEL (object);

  switch (property_id)
    {
    case PROP_ENUM_TYPE:
      if (label->enum_class)
        g_value_set_gtype (value, G_TYPE_FROM_CLASS (label->enum_class));
      else
        g_value_set_gtype (value, G_TYPE_NONE);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_enum_label_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpEnumLabel *label = GIMP_ENUM_LABEL (object);

  switch (property_id)
    {
    case PROP_ENUM_TYPE:
      label->enum_class = g_type_class_ref (g_value_get_gtype (value));
      break;

    case PROP_ENUM_VALUE:
      gimp_enum_label_set_value (label, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_enum_label_new:
 * @enum_type: the #GType of an enum
 * @value:     an enum value
 *
 * Return value: a new #GimpEnumLabel.
 *
 * Since: 2.4
 **/
GtkWidget *
gimp_enum_label_new (GType enum_type,
                     gint  value)
{
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  return g_object_new (GIMP_TYPE_ENUM_LABEL,
                       "enum-type",  enum_type,
                       "enum-value", value,
                       NULL);
}

/**
 * gimp_enum_label_set_value
 * @label: a #GimpEnumLabel
 * @value: an enum value
 *
 * Since: 2.4
 **/
void
gimp_enum_label_set_value (GimpEnumLabel *label,
                           gint           value)
{
  const gchar *nick;
  const gchar *desc;

  g_return_if_fail (GIMP_IS_ENUM_LABEL (label));

  if (! gimp_enum_get_value (G_TYPE_FROM_CLASS (label->enum_class), value,
                             NULL, &nick, &desc, NULL))
    {
      g_warning ("%s: %d is not valid for enum of type '%s'",
                 G_STRLOC, value,
                 g_type_name (G_TYPE_FROM_CLASS (label->enum_class)));
      return;
    }

  if (! desc)
    desc = nick;

  gtk_label_set_text (GTK_LABEL (label), desc);
}
