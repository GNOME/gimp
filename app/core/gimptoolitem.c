/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolitem.c
 * Copyright (C) 2020 Ell
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligmatoolitem.h"


enum
{
  VISIBLE_CHANGED,
  SHOWN_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_VISIBLE,
  PROP_SHOWN
};


struct _LigmaToolItemPrivate
{
  gboolean visible;
};


/*  local function prototypes  */

static void   ligma_tool_item_get_property  (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);
static void   ligma_tool_item_set_property  (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolItem, ligma_tool_item, LIGMA_TYPE_VIEWABLE)

#define parent_class ligma_tool_item_parent_class

static guint ligma_tool_item_signals[LAST_SIGNAL] = { 0 };


/*  private functions  */

static void
ligma_tool_item_class_init (LigmaToolItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  ligma_tool_item_signals[VISIBLE_CHANGED] =
    g_signal_new ("visible-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolItemClass, visible_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_tool_item_signals[SHOWN_CHANGED] =
    g_signal_new ("shown-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolItemClass, shown_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->get_property = ligma_tool_item_get_property;
  object_class->set_property = ligma_tool_item_set_property;

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_VISIBLE,
                            "visible", NULL, NULL,
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_SHOWN,
                                   g_param_spec_boolean ("shown", NULL, NULL,
                                                         TRUE,
                                                         LIGMA_PARAM_READABLE));
}

static void
ligma_tool_item_init (LigmaToolItem *tool_item)
{
  tool_item->priv = ligma_tool_item_get_instance_private (tool_item);
}

static void
ligma_tool_item_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaToolItem *tool_item = LIGMA_TOOL_ITEM (object);

  switch (property_id)
    {
    case PROP_VISIBLE:
      g_value_set_boolean (value, tool_item->priv->visible);
      break;
    case PROP_SHOWN:
      g_value_set_boolean (value, ligma_tool_item_get_shown (tool_item));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_tool_item_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaToolItem *tool_item = LIGMA_TOOL_ITEM (object);

  switch (property_id)
    {
    case PROP_VISIBLE:
      ligma_tool_item_set_visible (tool_item, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

void
ligma_tool_item_set_visible (LigmaToolItem *tool_item,
                            gboolean      visible)
{
  g_return_if_fail (LIGMA_IS_TOOL_ITEM (tool_item));

  if (visible != tool_item->priv->visible)
    {
      gboolean old_shown;

      g_object_freeze_notify (G_OBJECT (tool_item));

      old_shown = ligma_tool_item_get_shown (tool_item);

      tool_item->priv->visible = visible;

      g_signal_emit (tool_item, ligma_tool_item_signals[VISIBLE_CHANGED], 0);

      if (ligma_tool_item_get_shown (tool_item) != old_shown)
        ligma_tool_item_shown_changed (tool_item);

      g_object_notify (G_OBJECT (tool_item), "visible");

      g_object_thaw_notify (G_OBJECT (tool_item));
    }
}

gboolean
ligma_tool_item_get_visible (LigmaToolItem *tool_item)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_ITEM (tool_item), FALSE);

  return tool_item->priv->visible;
}

gboolean
ligma_tool_item_get_shown (LigmaToolItem *tool_item)
{
  LigmaToolItem *parent;

  g_return_val_if_fail (LIGMA_IS_TOOL_ITEM (tool_item), FALSE);

  parent = LIGMA_TOOL_ITEM (
    ligma_viewable_get_parent (LIGMA_VIEWABLE (tool_item)));

  return tool_item->priv->visible &&
         (! parent || ligma_tool_item_get_shown (parent));
}


/*  protected functions  */

void
ligma_tool_item_shown_changed (LigmaToolItem *tool_item)
{
  g_signal_emit (tool_item, ligma_tool_item_signals[SHOWN_CHANGED], 0);

  g_object_notify (G_OBJECT (tool_item), "shown");
}
