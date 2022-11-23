/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasgroup.c
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

#include "ligmacanvasgroup.h"
#include "ligmadisplayshell.h"


enum
{
  PROP_0,
  PROP_GROUP_STROKING,
  PROP_GROUP_FILLING
};


struct _LigmaCanvasGroupPrivate
{
  GQueue   *items;
  gboolean  group_stroking;
  gboolean  group_filling;
};


/*  local function prototypes  */

static void             ligma_canvas_group_finalize     (GObject         *object);
static void             ligma_canvas_group_set_property (GObject         *object,
                                                        guint            property_id,
                                                        const GValue    *value,
                                                        GParamSpec      *pspec);
static void             ligma_canvas_group_get_property (GObject         *object,
                                                        guint            property_id,
                                                        GValue          *value,
                                                        GParamSpec      *pspec);
static void             ligma_canvas_group_draw         (LigmaCanvasItem  *item,
                                                        cairo_t         *cr);
static cairo_region_t * ligma_canvas_group_get_extents  (LigmaCanvasItem  *item);
static gboolean         ligma_canvas_group_hit          (LigmaCanvasItem  *item,
                                                        gdouble          x,
                                                        gdouble          y);

static void             ligma_canvas_group_child_update (LigmaCanvasItem  *item,
                                                        cairo_region_t  *region,
                                                        LigmaCanvasGroup *group);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasGroup, ligma_canvas_group,
                            LIGMA_TYPE_CANVAS_ITEM)

#define parent_class ligma_canvas_group_parent_class


static void
ligma_canvas_group_class_init (LigmaCanvasGroupClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = ligma_canvas_group_finalize;
  object_class->set_property = ligma_canvas_group_set_property;
  object_class->get_property = ligma_canvas_group_get_property;

  item_class->draw           = ligma_canvas_group_draw;
  item_class->get_extents    = ligma_canvas_group_get_extents;
  item_class->hit            = ligma_canvas_group_hit;

  g_object_class_install_property (object_class, PROP_GROUP_STROKING,
                                   g_param_spec_boolean ("group-stroking",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_GROUP_FILLING,
                                   g_param_spec_boolean ("group-filling",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_group_init (LigmaCanvasGroup *group)
{
  group->priv = ligma_canvas_group_get_instance_private (group);

  group->priv->items = g_queue_new ();
}

static void
ligma_canvas_group_finalize (GObject *object)
{
  LigmaCanvasGroup *group = LIGMA_CANVAS_GROUP (object);
  LigmaCanvasItem  *item;

  while ((item = g_queue_peek_head (group->priv->items)))
    ligma_canvas_group_remove_item (group, item);

  g_queue_free (group->priv->items);
  group->priv->items = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_canvas_group_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaCanvasGroup *group = LIGMA_CANVAS_GROUP (object);

  switch (property_id)
    {
    case PROP_GROUP_STROKING:
      group->priv->group_stroking = g_value_get_boolean (value);
      break;
    case PROP_GROUP_FILLING:
      group->priv->group_filling = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_group_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaCanvasGroup *group = LIGMA_CANVAS_GROUP (object);

  switch (property_id)
    {
    case PROP_GROUP_STROKING:
      g_value_set_boolean (value, group->priv->group_stroking);
      break;
    case PROP_GROUP_FILLING:
      g_value_set_boolean (value, group->priv->group_filling);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_group_draw (LigmaCanvasItem *item,
                        cairo_t        *cr)
{
  LigmaCanvasGroup *group = LIGMA_CANVAS_GROUP (item);
  GList           *list;

  for (list = group->priv->items->head; list; list = g_list_next (list))
    {
      LigmaCanvasItem *sub_item = list->data;

      ligma_canvas_item_draw (sub_item, cr);
    }

  if (group->priv->group_stroking)
    _ligma_canvas_item_stroke (item, cr);

  if (group->priv->group_filling)
    _ligma_canvas_item_fill (item, cr);
}

static cairo_region_t *
ligma_canvas_group_get_extents (LigmaCanvasItem *item)
{
  LigmaCanvasGroup *group  = LIGMA_CANVAS_GROUP (item);
  cairo_region_t  *region = NULL;
  GList           *list;

  for (list = group->priv->items->head; list; list = g_list_next (list))
    {
      LigmaCanvasItem *sub_item   = list->data;
      cairo_region_t *sub_region = ligma_canvas_item_get_extents (sub_item);

      if (! region)
        {
          region = sub_region;
        }
      else if (sub_region)
        {
          cairo_region_union (region, sub_region);
          cairo_region_destroy (sub_region);
        }
    }

  return region;
}

static gboolean
ligma_canvas_group_hit (LigmaCanvasItem *item,
                       gdouble         x,
                       gdouble         y)
{
  LigmaCanvasGroup *group = LIGMA_CANVAS_GROUP (item);
  GList           *list;

  for (list = group->priv->items->head; list; list = g_list_next (list))
    {
      if (ligma_canvas_item_hit (list->data, x, y))
        return TRUE;
    }

  return FALSE;
}

static void
ligma_canvas_group_child_update (LigmaCanvasItem  *item,
                                cairo_region_t  *region,
                                LigmaCanvasGroup *group)
{
  if (_ligma_canvas_item_needs_update (LIGMA_CANVAS_ITEM (group)))
    _ligma_canvas_item_update (LIGMA_CANVAS_ITEM (group), region);
}


/*  public functions  */

LigmaCanvasItem *
ligma_canvas_group_new (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_GROUP,
                       "shell", shell,
                       NULL);
}

void
ligma_canvas_group_add_item (LigmaCanvasGroup *group,
                            LigmaCanvasItem  *item)
{
  g_return_if_fail (LIGMA_IS_CANVAS_GROUP (group));
  g_return_if_fail (LIGMA_IS_CANVAS_ITEM (item));
  g_return_if_fail (LIGMA_CANVAS_ITEM (group) != item);

  if (group->priv->group_stroking)
    ligma_canvas_item_suspend_stroking (item);

  if (group->priv->group_filling)
    ligma_canvas_item_suspend_filling (item);

  g_queue_push_tail (group->priv->items, g_object_ref (item));

  if (_ligma_canvas_item_needs_update (LIGMA_CANVAS_ITEM (group)))
    {
      cairo_region_t *region = ligma_canvas_item_get_extents (item);

      if (region)
        {
          _ligma_canvas_item_update (LIGMA_CANVAS_ITEM (group), region);
          cairo_region_destroy (region);
        }
    }

  g_signal_connect (item, "update",
                    G_CALLBACK (ligma_canvas_group_child_update),
                    group);
}

void
ligma_canvas_group_remove_item (LigmaCanvasGroup *group,
                               LigmaCanvasItem  *item)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_CANVAS_GROUP (group));
  g_return_if_fail (LIGMA_IS_CANVAS_ITEM (item));

  list = g_queue_find (group->priv->items, item);

  g_return_if_fail (list != NULL);

  g_queue_delete_link (group->priv->items, list);

  if (group->priv->group_stroking)
    ligma_canvas_item_resume_stroking (item);

  if (group->priv->group_filling)
    ligma_canvas_item_resume_filling (item);

  if (_ligma_canvas_item_needs_update (LIGMA_CANVAS_ITEM (group)))
    {
      cairo_region_t *region = ligma_canvas_item_get_extents (item);

      if (region)
        {
          _ligma_canvas_item_update (LIGMA_CANVAS_ITEM (group), region);
          cairo_region_destroy (region);
        }
    }

  g_signal_handlers_disconnect_by_func (item,
                                        ligma_canvas_group_child_update,
                                        group);

  g_object_unref (item);
}

void
ligma_canvas_group_set_group_stroking (LigmaCanvasGroup *group,
                                      gboolean         group_stroking)
{
  g_return_if_fail (LIGMA_IS_CANVAS_GROUP (group));

  if (group->priv->group_stroking != group_stroking)
    {
      GList *list;

      ligma_canvas_item_begin_change (LIGMA_CANVAS_ITEM (group));

      g_object_set (group,
                    "group-stroking", group_stroking ? TRUE : FALSE,
                    NULL);

      for (list = group->priv->items->head; list; list = g_list_next (list))
        {
          if (group->priv->group_stroking)
            ligma_canvas_item_suspend_stroking (list->data);
          else
            ligma_canvas_item_resume_stroking (list->data);
        }

      ligma_canvas_item_end_change (LIGMA_CANVAS_ITEM (group));
    }
}

void
ligma_canvas_group_set_group_filling (LigmaCanvasGroup *group,
                                     gboolean         group_filling)
{
  g_return_if_fail (LIGMA_IS_CANVAS_GROUP (group));

  if (group->priv->group_filling != group_filling)
    {
      GList *list;

      ligma_canvas_item_begin_change (LIGMA_CANVAS_ITEM (group));

      g_object_set (group,
                    "group-filling", group_filling ? TRUE : FALSE,
                    NULL);

      for (list = group->priv->items->head; list; list = g_list_next (list))
        {
          if (group->priv->group_filling)
            ligma_canvas_item_suspend_filling (list->data);
          else
            ligma_canvas_item_resume_filling (list->data);
        }

      ligma_canvas_item_end_change (LIGMA_CANVAS_ITEM (group));
    }
}
