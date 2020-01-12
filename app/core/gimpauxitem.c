/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpauxitem.h"


enum
{
  REMOVED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ID
};


struct _GimpAuxItemPrivate
{
  guint32  aux_item_id;
};


static void   gimp_aux_item_get_property (GObject      *object,
                                          guint         property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);
static void   gimp_aux_item_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GimpAuxItem, gimp_aux_item, G_TYPE_OBJECT)

static guint gimp_aux_item_signals[LAST_SIGNAL] = { 0 };


static void
gimp_aux_item_class_init (GimpAuxItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  gimp_aux_item_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpAuxItemClass, removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->get_property = gimp_aux_item_get_property;
  object_class->set_property = gimp_aux_item_set_property;

  klass->removed             = NULL;

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_uint ("id", NULL, NULL,
                                                      0, G_MAXUINT32, 0,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      GIMP_PARAM_READWRITE));
}

static void
gimp_aux_item_init (GimpAuxItem *aux_item)
{
  aux_item->priv = gimp_aux_item_get_instance_private (aux_item);
}

static void
gimp_aux_item_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpAuxItem *aux_item = GIMP_AUX_ITEM (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_uint (value, aux_item->priv->aux_item_id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_aux_item_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpAuxItem *aux_item = GIMP_AUX_ITEM (object);

  switch (property_id)
    {
    case PROP_ID:
      aux_item->priv->aux_item_id = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

guint32
gimp_aux_item_get_id (GimpAuxItem *aux_item)
{
  g_return_val_if_fail (GIMP_IS_AUX_ITEM (aux_item), 0);

  return aux_item->priv->aux_item_id;
}

void
gimp_aux_item_removed (GimpAuxItem *aux_item)
{
  g_return_if_fail (GIMP_IS_AUX_ITEM (aux_item));

  g_signal_emit (aux_item, gimp_aux_item_signals[REMOVED], 0);
}
