/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasproxygroup.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "gimpcanvas.h"
#include "gimpcanvasproxygroup.h"
#include "gimpdisplayshell.h"


enum
{
  PROP_0
};


typedef struct _GimpCanvasProxyGroupPrivate GimpCanvasProxyGroupPrivate;

struct _GimpCanvasProxyGroupPrivate
{
  GHashTable *proxy_hash;
};

#define GET_PRIVATE(proxy_group) \
        ((GimpCanvasProxyGroupPrivate *) gimp_canvas_proxy_group_get_instance_private ((GimpCanvasProxyGroup *) (proxy_group)))


/*  local function prototypes  */

static void        gimp_canvas_proxy_group_finalize     (GObject          *object);
static void        gimp_canvas_proxy_group_set_property (GObject          *object,
                                                         guint             property_id,
                                                         const GValue     *value,
                                                         GParamSpec       *pspec);
static void        gimp_canvas_proxy_group_get_property (GObject          *object,
                                                         guint             property_id,
                                                         GValue           *value,
                                                         GParamSpec       *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasProxyGroup, gimp_canvas_proxy_group,
                            GIMP_TYPE_CANVAS_GROUP)

#define parent_class gimp_canvas_proxy_group_parent_class


static void
gimp_canvas_proxy_group_class_init (GimpCanvasProxyGroupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_canvas_proxy_group_finalize;
  object_class->set_property = gimp_canvas_proxy_group_set_property;
  object_class->get_property = gimp_canvas_proxy_group_get_property;
}

static void
gimp_canvas_proxy_group_init (GimpCanvasProxyGroup *proxy_group)
{
  GimpCanvasProxyGroupPrivate *private = GET_PRIVATE (proxy_group);

  private->proxy_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void
gimp_canvas_proxy_group_finalize (GObject *object)
{
  GimpCanvasProxyGroupPrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->proxy_hash, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_canvas_proxy_group_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  /* GimpCanvasProxyGroupPrivate *private = GET_PRIVATE (object); */

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_proxy_group_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  /* GimpCanvasProxyGroupPrivate *private = GET_PRIVATE (object); */

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GimpCanvasItem *
gimp_canvas_proxy_group_new (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_PROXY_GROUP,
                       "shell", shell,
                       NULL);
}

void
gimp_canvas_proxy_group_add_item (GimpCanvasProxyGroup *group,
                                  gpointer              object,
                                  GimpCanvasItem       *proxy_item)
{
  GimpCanvasProxyGroupPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_GROUP (group));
  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (proxy_item));
  g_return_if_fail (GIMP_CANVAS_ITEM (group) != proxy_item);

  private = GET_PRIVATE (group);

  g_return_if_fail (g_hash_table_lookup (private->proxy_hash, object) ==
                    NULL);

  g_hash_table_insert (private->proxy_hash, object, proxy_item);

  gimp_canvas_group_add_item (GIMP_CANVAS_GROUP (group), proxy_item);
}

void
gimp_canvas_proxy_group_remove_item (GimpCanvasProxyGroup *group,
                                     gpointer              object)
{
  GimpCanvasProxyGroupPrivate *private;
  GimpCanvasItem              *proxy_item;

  g_return_if_fail (GIMP_IS_CANVAS_GROUP (group));
  g_return_if_fail (object != NULL);

  private = GET_PRIVATE (group);

  proxy_item = g_hash_table_lookup (private->proxy_hash, object);

  g_return_if_fail (proxy_item != NULL);

  g_hash_table_remove (private->proxy_hash, object);

  gimp_canvas_group_remove_item (GIMP_CANVAS_GROUP (group), proxy_item);
}

GimpCanvasItem *
gimp_canvas_proxy_group_get_item (GimpCanvasProxyGroup *group,
                                  gpointer              object)
{
  GimpCanvasProxyGroupPrivate *private;

  g_return_val_if_fail (GIMP_IS_CANVAS_GROUP (group), NULL);
  g_return_val_if_fail (object != NULL, NULL);

  private = GET_PRIVATE (group);

  return g_hash_table_lookup (private->proxy_hash, object);
}
