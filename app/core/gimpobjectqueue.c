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

#include <glib-object.h>

#include "core-types.h"

#include "gimpcontainer.h"
#include "gimpobject.h"
#include "gimpobjectqueue.h"
#include "gimpprogress.h"


typedef struct
{
  GimpObject *object;
  gint64      memsize;
} Item;


static void   gimp_object_queue_dispose      (GObject         *object);

static void   gimp_object_queue_push_swapped (gpointer         object,
                                              GimpObjectQueue *queue);

static Item * gimp_object_queue_item_new     (GimpObject      *object);
static void   gimp_object_queue_item_free    (Item            *item);


G_DEFINE_TYPE (GimpObjectQueue, gimp_object_queue, GIMP_TYPE_SUB_PROGRESS);

#define parent_class gimp_object_queue_parent_class


/*  private functions  */


static void
gimp_object_queue_class_init (GimpObjectQueueClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gimp_object_queue_dispose;
}

static void
gimp_object_queue_init (GimpObjectQueue *queue)
{
  g_queue_init (&queue->items);

  queue->processed_memsize = 0;
  queue->total_memsize     = 0;
}

static void
gimp_object_queue_dispose (GObject *object)
{
  gimp_object_queue_clear (GIMP_OBJECT_QUEUE (object));

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_object_queue_push_swapped (gpointer         object,
                                GimpObjectQueue *queue)
{
  gimp_object_queue_push (queue, object);
}

static Item *
gimp_object_queue_item_new (GimpObject *object)
{
  Item *item = g_slice_new (Item);

  item->object  = object;
  item->memsize = gimp_object_get_memsize (object, NULL);

  return item;
}

static void
gimp_object_queue_item_free (Item *item)
{
  g_slice_free (Item, item);
}


/*  public functions  */


GimpObjectQueue *
gimp_object_queue_new (GimpProgress *progress)
{
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  return g_object_new (GIMP_TYPE_OBJECT_QUEUE,
                       "progress", progress,
                       NULL);
}

void
gimp_object_queue_clear (GimpObjectQueue *queue)
{
  Item *item;

  g_return_if_fail (GIMP_IS_OBJECT_QUEUE (queue));

  while ((item = g_queue_pop_head (&queue->items)))
    gimp_object_queue_item_free (item);

  queue->processed_memsize = 0;
  queue->total_memsize     = 0;

  gimp_sub_progress_set_range (GIMP_SUB_PROGRESS (queue), 0.0, 1.0);
}

void
gimp_object_queue_push (GimpObjectQueue *queue,
                        gpointer         object)
{
  Item *item;

  g_return_if_fail (GIMP_IS_OBJECT_QUEUE (queue));
  g_return_if_fail (GIMP_IS_OBJECT (object));

  item = gimp_object_queue_item_new (GIMP_OBJECT (object));

  g_queue_push_tail (&queue->items, item);

  queue->total_memsize += item->memsize;
}

void
gimp_object_queue_push_container (GimpObjectQueue *queue,
                                  GimpContainer   *container)
{
  g_return_if_fail (GIMP_IS_OBJECT_QUEUE (queue));
  g_return_if_fail (GIMP_IS_CONTAINER (container));

  gimp_container_foreach (container,
                          (GFunc) gimp_object_queue_push_swapped,
                          queue);
}

void
gimp_object_queue_push_list (GimpObjectQueue *queue,
                             GList           *list)
{
  g_return_if_fail (GIMP_IS_OBJECT_QUEUE (queue));

  g_list_foreach (list,
                  (GFunc) gimp_object_queue_push_swapped,
                  queue);
}

gpointer
gimp_object_queue_pop (GimpObjectQueue *queue)
{
  Item       *item;
  GimpObject *object;

  g_return_val_if_fail (GIMP_IS_OBJECT_QUEUE (queue), NULL);

  item = g_queue_pop_head (&queue->items);

  if (! item)
    return NULL;

  object = item->object;

  gimp_sub_progress_set_range (GIMP_SUB_PROGRESS (queue),
                               (gdouble) queue->processed_memsize  /
                               (gdouble) queue->total_memsize,
                               (gdouble) (queue->processed_memsize +
                                          item->memsize)              /
                               (gdouble) queue->total_memsize);
  queue->processed_memsize += item->memsize;

  gimp_object_queue_item_free (item);

  return object;
}
