/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasgroup.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "gimpcanvasgroup.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-transform.h"


enum
{
  PROP_0
};


typedef struct _GimpCanvasGroupPrivate GimpCanvasGroupPrivate;

struct _GimpCanvasGroupPrivate
{
  GList *items;
};

#define GET_PRIVATE(group) \
        G_TYPE_INSTANCE_GET_PRIVATE (group, \
                                     GIMP_TYPE_CANVAS_GROUP, \
                                     GimpCanvasGroupPrivate)


/*  local function prototypes  */

static void        gimp_canvas_group_dispose      (GObject          *object);
static void        gimp_canvas_group_set_property (GObject          *object,
                                                   guint             property_id,
                                                   const GValue     *value,
                                                   GParamSpec       *pspec);
static void        gimp_canvas_group_get_property (GObject          *object,
                                                   guint             property_id,
                                                   GValue           *value,
                                                   GParamSpec       *pspec);
static void        gimp_canvas_group_draw         (GimpCanvasItem   *item,
                                                   GimpDisplayShell *shell,
                                                   cairo_t          *cr);
static GdkRegion * gimp_canvas_group_get_extents  (GimpCanvasItem   *item,
                                                   GimpDisplayShell *shell);


G_DEFINE_TYPE (GimpCanvasGroup, gimp_canvas_group, GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_group_parent_class


static void
gimp_canvas_group_class_init (GimpCanvasGroupClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->dispose      = gimp_canvas_group_dispose;
  object_class->set_property = gimp_canvas_group_set_property;
  object_class->get_property = gimp_canvas_group_get_property;

  item_class->draw           = gimp_canvas_group_draw;
  item_class->get_extents    = gimp_canvas_group_get_extents;

  g_type_class_add_private (klass, sizeof (GimpCanvasGroupPrivate));
}

static void
gimp_canvas_group_init (GimpCanvasGroup *group)
{
}

static void
gimp_canvas_group_dispose (GObject *object)
{
  GimpCanvasGroupPrivate *private = GET_PRIVATE (object);

  if (private->items)
    {
      g_list_foreach (private->items, (GFunc) g_object_unref, NULL);
      g_list_free (private->items);
      private->items = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_canvas_group_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  /* GimpCanvasGroupPrivate *private = GET_PRIVATE (object); */

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_group_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  /* GimpCanvasGroupPrivate *private = GET_PRIVATE (object); */

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_group_draw (GimpCanvasItem   *item,
                        GimpDisplayShell *shell,
                        cairo_t          *cr)
{
  GimpCanvasGroupPrivate *private = GET_PRIVATE (item);
  GList                  *list;

  for (list = private->items; list; list = g_list_next (list))
    {
      GimpCanvasItem *sub_item = list->data;

      gimp_canvas_item_draw (sub_item, shell, cr);
    }
}

static GdkRegion *
gimp_canvas_group_get_extents (GimpCanvasItem   *item,
                               GimpDisplayShell *shell)
{
  GimpCanvasGroupPrivate *private = GET_PRIVATE (item);
  GdkRegion              *region  = NULL;
  GList                  *list;

  for (list = private->items; list; list = g_list_next (list))
    {
      GimpCanvasItem *sub_item   = list->data;
      GdkRegion      *sub_region = gimp_canvas_item_get_extents (sub_item,
                                                                 shell);

      if (region)
        {
          gdk_region_union (region, sub_region);
          gdk_region_destroy (sub_region);
        }
      else
        {
          region = sub_region;
        }
    }

  return region;
}

GimpCanvasItem *
gimp_canvas_group_new (void)
{
  return g_object_new (GIMP_TYPE_CANVAS_GROUP, NULL);
}

void
gimp_canvas_group_add_item (GimpCanvasGroup *group,
                            GimpCanvasItem  *item)
{
  GimpCanvasGroupPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_GROUP (group));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));
  g_return_if_fail (GIMP_CANVAS_ITEM (group) != item);

  private = GET_PRIVATE (group);

  private->items = g_list_append (private->items, g_object_ref (item));
}
