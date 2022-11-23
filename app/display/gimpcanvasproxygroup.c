/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasproxygroup.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "ligmacanvas.h"
#include "ligmacanvasproxygroup.h"
#include "ligmadisplayshell.h"


enum
{
  PROP_0
};


typedef struct _LigmaCanvasProxyGroupPrivate LigmaCanvasProxyGroupPrivate;

struct _LigmaCanvasProxyGroupPrivate
{
  GHashTable *proxy_hash;
};

#define GET_PRIVATE(proxy_group) \
        ((LigmaCanvasProxyGroupPrivate *) ligma_canvas_proxy_group_get_instance_private ((LigmaCanvasProxyGroup *) (proxy_group)))


/*  local function prototypes  */

static void        ligma_canvas_proxy_group_finalize     (GObject          *object);
static void        ligma_canvas_proxy_group_set_property (GObject          *object,
                                                         guint             property_id,
                                                         const GValue     *value,
                                                         GParamSpec       *pspec);
static void        ligma_canvas_proxy_group_get_property (GObject          *object,
                                                         guint             property_id,
                                                         GValue           *value,
                                                         GParamSpec       *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasProxyGroup, ligma_canvas_proxy_group,
                            LIGMA_TYPE_CANVAS_GROUP)

#define parent_class ligma_canvas_proxy_group_parent_class


static void
ligma_canvas_proxy_group_class_init (LigmaCanvasProxyGroupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = ligma_canvas_proxy_group_finalize;
  object_class->set_property = ligma_canvas_proxy_group_set_property;
  object_class->get_property = ligma_canvas_proxy_group_get_property;
}

static void
ligma_canvas_proxy_group_init (LigmaCanvasProxyGroup *proxy_group)
{
  LigmaCanvasProxyGroupPrivate *private = GET_PRIVATE (proxy_group);

  private->proxy_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void
ligma_canvas_proxy_group_finalize (GObject *object)
{
  LigmaCanvasProxyGroupPrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->proxy_hash, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_canvas_proxy_group_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  /* LigmaCanvasProxyGroupPrivate *private = GET_PRIVATE (object); */

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_proxy_group_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  /* LigmaCanvasProxyGroupPrivate *private = GET_PRIVATE (object); */

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

LigmaCanvasItem *
ligma_canvas_proxy_group_new (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_PROXY_GROUP,
                       "shell", shell,
                       NULL);
}

void
ligma_canvas_proxy_group_add_item (LigmaCanvasProxyGroup *group,
                                  gpointer              object,
                                  LigmaCanvasItem       *proxy_item)
{
  LigmaCanvasProxyGroupPrivate *private;

  g_return_if_fail (LIGMA_IS_CANVAS_GROUP (group));
  g_return_if_fail (object != NULL);
  g_return_if_fail (LIGMA_IS_CANVAS_ITEM (proxy_item));
  g_return_if_fail (LIGMA_CANVAS_ITEM (group) != proxy_item);

  private = GET_PRIVATE (group);

  g_return_if_fail (g_hash_table_lookup (private->proxy_hash, object) ==
                    NULL);

  g_hash_table_insert (private->proxy_hash, object, proxy_item);

  ligma_canvas_group_add_item (LIGMA_CANVAS_GROUP (group), proxy_item);
}

void
ligma_canvas_proxy_group_remove_item (LigmaCanvasProxyGroup *group,
                                     gpointer              object)
{
  LigmaCanvasProxyGroupPrivate *private;
  LigmaCanvasItem              *proxy_item;

  g_return_if_fail (LIGMA_IS_CANVAS_GROUP (group));
  g_return_if_fail (object != NULL);

  private = GET_PRIVATE (group);

  proxy_item = g_hash_table_lookup (private->proxy_hash, object);

  g_return_if_fail (proxy_item != NULL);

  g_hash_table_remove (private->proxy_hash, object);

  ligma_canvas_group_remove_item (LIGMA_CANVAS_GROUP (group), proxy_item);
}

LigmaCanvasItem *
ligma_canvas_proxy_group_get_item (LigmaCanvasProxyGroup *group,
                                  gpointer              object)
{
  LigmaCanvasProxyGroupPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CANVAS_GROUP (group), NULL);
  g_return_val_if_fail (object != NULL, NULL);

  private = GET_PRIVATE (group);

  return g_hash_table_lookup (private->proxy_hash, object);
}
