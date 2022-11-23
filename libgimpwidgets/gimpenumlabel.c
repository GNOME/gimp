/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaenumlabel.c
 * Copyright (C) 2005  Sven Neumann <sven@ligma.org>
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

#include "libligmabase/ligmabase.h"

#include "ligmawidgetstypes.h"

#include "ligmaenumlabel.h"


/**
 * SECTION: ligmaenumlabel
 * @title: LigmaEnumLabel
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


struct _LigmaEnumLabelPrivate
{
  GEnumClass *enum_class;
};

#define GET_PRIVATE(obj) (((LigmaEnumLabel *) (obj))->priv)


static void   ligma_enum_label_finalize     (GObject      *object);
static void   ligma_enum_label_get_property (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);
static void   ligma_enum_label_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaEnumLabel, ligma_enum_label, GTK_TYPE_LABEL)

#define parent_class ligma_enum_label_parent_class


static void
ligma_enum_label_class_init (LigmaEnumLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = ligma_enum_label_finalize;
  object_class->get_property = ligma_enum_label_get_property;
  object_class->set_property = ligma_enum_label_set_property;

  /**
   * LigmaEnumLabel:enum-type:
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
                                                       LIGMA_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  /**
   * LigmaEnumLabel:enum-value:
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
                                                     LIGMA_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));
}

static void
ligma_enum_label_init (LigmaEnumLabel *enum_label)
{
  enum_label->priv = ligma_enum_label_get_instance_private (enum_label);
}

static void
ligma_enum_label_finalize (GObject *object)
{
  LigmaEnumLabelPrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->enum_class, g_type_class_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_enum_label_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaEnumLabelPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ENUM_TYPE:
      if (private->enum_class)
        g_value_set_gtype (value, G_TYPE_FROM_CLASS (private->enum_class));
      else
        g_value_set_gtype (value, G_TYPE_NONE);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_enum_label_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaEnumLabel        *label   = LIGMA_ENUM_LABEL (object);
  LigmaEnumLabelPrivate *private = GET_PRIVATE (label);

  switch (property_id)
    {
    case PROP_ENUM_TYPE:
      private->enum_class = g_type_class_ref (g_value_get_gtype (value));
      break;

    case PROP_ENUM_VALUE:
      ligma_enum_label_set_value (label, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * ligma_enum_label_new:
 * @enum_type: the #GType of an enum
 * @value:     an enum value
 *
 * Returns: a new #LigmaEnumLabel.
 *
 * Since: 2.4
 **/
GtkWidget *
ligma_enum_label_new (GType enum_type,
                     gint  value)
{
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  return g_object_new (LIGMA_TYPE_ENUM_LABEL,
                       "enum-type",  enum_type,
                       "enum-value", value,
                       NULL);
}

/**
 * ligma_enum_label_set_value
 * @label: a #LigmaEnumLabel
 * @value: an enum value
 *
 * Since: 2.4
 **/
void
ligma_enum_label_set_value (LigmaEnumLabel *label,
                           gint           value)
{
  LigmaEnumLabelPrivate *private;
  const gchar          *nick;
  const gchar          *desc;

  g_return_if_fail (LIGMA_IS_ENUM_LABEL (label));

  private = GET_PRIVATE (label);

  if (! ligma_enum_get_value (G_TYPE_FROM_CLASS (private->enum_class), value,
                             NULL, &nick, &desc, NULL))
    {
      g_warning ("%s: %d is not valid for enum of type '%s'",
                 G_STRLOC, value,
                 g_type_name (G_TYPE_FROM_CLASS (private->enum_class)));
      return;
    }

  if (! desc)
    desc = nick;

  gtk_label_set_text (GTK_LABEL (label), desc);
}
