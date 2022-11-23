/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligmaauxitem.h"


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


struct _LigmaAuxItemPrivate
{
  guint32  aux_item_id;
};


static void   ligma_aux_item_get_property (GObject      *object,
                                          guint         property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);
static void   ligma_aux_item_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (LigmaAuxItem, ligma_aux_item, G_TYPE_OBJECT)

static guint ligma_aux_item_signals[LAST_SIGNAL] = { 0 };


static void
ligma_aux_item_class_init (LigmaAuxItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  ligma_aux_item_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaAuxItemClass, removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->get_property = ligma_aux_item_get_property;
  object_class->set_property = ligma_aux_item_set_property;

  klass->removed             = NULL;

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_uint ("id", NULL, NULL,
                                                      0, G_MAXUINT32, 0,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      LIGMA_PARAM_READWRITE));
}

static void
ligma_aux_item_init (LigmaAuxItem *aux_item)
{
  aux_item->priv = ligma_aux_item_get_instance_private (aux_item);
}

static void
ligma_aux_item_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  LigmaAuxItem *aux_item = LIGMA_AUX_ITEM (object);

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
ligma_aux_item_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  LigmaAuxItem *aux_item = LIGMA_AUX_ITEM (object);

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
ligma_aux_item_get_id (LigmaAuxItem *aux_item)
{
  g_return_val_if_fail (LIGMA_IS_AUX_ITEM (aux_item), 0);

  return aux_item->priv->aux_item_id;
}

void
ligma_aux_item_removed (LigmaAuxItem *aux_item)
{
  g_return_if_fail (LIGMA_IS_AUX_ITEM (aux_item));

  g_signal_emit (aux_item, ligma_aux_item_signals[REMOVED], 0);
}
