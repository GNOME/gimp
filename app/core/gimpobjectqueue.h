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

#ifndef __LIGMA_OBJECT_QUEUE_H__
#define __LIGMA_OBJECT_QUEUE_H__


#include "ligmasubprogress.h"


#define LIGMA_TYPE_OBJECT_QUEUE            (ligma_object_queue_get_type ())
#define LIGMA_OBJECT_QUEUE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OBJECT_QUEUE, LigmaObjectQueue))
#define LIGMA_OBJECT_QUEUE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_OBJECT_QUEUE, LigmaObjectQueueClass))
#define LIGMA_IS_OBJECT_QUEUE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OBJECT_QUEUE))
#define LIGMA_IS_OBJECT_QUEUE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_OBJECT_QUEUE))
#define LIGMA_OBJECT_QUEUE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_OBJECT_QUEUE, LigmaObjectQueueClass))


typedef struct _LigmaObjectQueueClass LigmaObjectQueueClass;

struct _LigmaObjectQueue
{
  LigmaSubProgress  parent_instance;

  GQueue           items;
  gint64           processed_memsize;
  gint64           total_memsize;
};

struct _LigmaObjectQueueClass
{
  LigmaSubProgressClass  parent_class;
};


GType             ligma_object_queue_get_type       (void) G_GNUC_CONST;

LigmaObjectQueue * ligma_object_queue_new            (LigmaProgress    *progress);

void              ligma_object_queue_clear          (LigmaObjectQueue *queue);

void              ligma_object_queue_push           (LigmaObjectQueue *queue,
                                                    gpointer         object);
void              ligma_object_queue_push_container (LigmaObjectQueue *queue,
                                                    LigmaContainer   *container);
void              ligma_object_queue_push_list      (LigmaObjectQueue *queue,
                                                    GList           *list);

gpointer          ligma_object_queue_pop            (LigmaObjectQueue *queue);


#endif /* __LIGMA_OBJECT_QUEUE_H__ */
