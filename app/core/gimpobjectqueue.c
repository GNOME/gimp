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

#include <glib-object.h>

#include "core-types.h"

#include "ligmacontainer.h"
#include "ligmaobject.h"
#include "ligmaobjectqueue.h"
#include "ligmaprogress.h"


typedef struct
{
  LigmaObject *object;
  gint64      memsize;
} Item;


static void   ligma_object_queue_dispose      (GObject         *object);

static void   ligma_object_queue_push_swapped (gpointer         object,
                                              LigmaObjectQueue *queue);

static Item * ligma_object_queue_item_new     (LigmaObject      *object);
static void   ligma_object_queue_item_free    (Item            *item);


G_DEFINE_TYPE (LigmaObjectQueue, ligma_object_queue, LIGMA_TYPE_SUB_PROGRESS);

#define parent_class ligma_object_queue_parent_class


/*  private functions  */


static void
ligma_object_queue_class_init (LigmaObjectQueueClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ligma_object_queue_dispose;
}

static void
ligma_object_queue_init (LigmaObjectQueue *queue)
{
  g_queue_init (&queue->items);

  queue->processed_memsize = 0;
  queue->total_memsize     = 0;
}

static void
ligma_object_queue_dispose (GObject *object)
{
  ligma_object_queue_clear (LIGMA_OBJECT_QUEUE (object));

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_object_queue_push_swapped (gpointer         object,
                                LigmaObjectQueue *queue)
{
  ligma_object_queue_push (queue, object);
}

static Item *
ligma_object_queue_item_new (LigmaObject *object)
{
  Item *item = g_slice_new (Item);

  item->object  = object;
  item->memsize = ligma_object_get_memsize (object, NULL);

  return item;
}

static void
ligma_object_queue_item_free (Item *item)
{
  g_slice_free (Item, item);
}


/*  public functions  */


LigmaObjectQueue *
ligma_object_queue_new (LigmaProgress *progress)
{
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);

  return g_object_new (LIGMA_TYPE_OBJECT_QUEUE,
                       "progress", progress,
                       NULL);
}

void
ligma_object_queue_clear (LigmaObjectQueue *queue)
{
  Item *item;

  g_return_if_fail (LIGMA_IS_OBJECT_QUEUE (queue));

  while ((item = g_queue_pop_head (&queue->items)))
    ligma_object_queue_item_free (item);

  queue->processed_memsize = 0;
  queue->total_memsize     = 0;

  ligma_sub_progress_set_range (LIGMA_SUB_PROGRESS (queue), 0.0, 1.0);
}

void
ligma_object_queue_push (LigmaObjectQueue *queue,
                        gpointer         object)
{
  Item *item;

  g_return_if_fail (LIGMA_IS_OBJECT_QUEUE (queue));
  g_return_if_fail (LIGMA_IS_OBJECT (object));

  item = ligma_object_queue_item_new (LIGMA_OBJECT (object));

  g_queue_push_tail (&queue->items, item);

  queue->total_memsize += item->memsize;
}

void
ligma_object_queue_push_container (LigmaObjectQueue *queue,
                                  LigmaContainer   *container)
{
  g_return_if_fail (LIGMA_IS_OBJECT_QUEUE (queue));
  g_return_if_fail (LIGMA_IS_CONTAINER (container));

  ligma_container_foreach (container,
                          (GFunc) ligma_object_queue_push_swapped,
                          queue);
}

void
ligma_object_queue_push_list (LigmaObjectQueue *queue,
                             GList           *list)
{
  g_return_if_fail (LIGMA_IS_OBJECT_QUEUE (queue));

  g_list_foreach (list,
                  (GFunc) ligma_object_queue_push_swapped,
                  queue);
}

gpointer
ligma_object_queue_pop (LigmaObjectQueue *queue)
{
  Item       *item;
  LigmaObject *object;

  g_return_val_if_fail (LIGMA_IS_OBJECT_QUEUE (queue), NULL);

  item = g_queue_pop_head (&queue->items);

  if (! item)
    return NULL;

  object = item->object;

  ligma_sub_progress_set_range (LIGMA_SUB_PROGRESS (queue),
                               (gdouble) queue->processed_memsize  /
                               (gdouble) queue->total_memsize,
                               (gdouble) (queue->processed_memsize +
                                          item->memsize)              /
                               (gdouble) queue->total_memsize);
  queue->processed_memsize += item->memsize;

  ligma_object_queue_item_free (item);

  return object;
}
