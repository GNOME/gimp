/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help Browser
 * Copyright (C) 1999-2005 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *
 * queue.c - a history queue
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib.h>

#include "queue.h"


struct _Queue
{
  GList *queue;
  GList *current;
};

typedef struct
{
  gchar   *uri;
  gchar   *title;
  gdouble  pos;
} Item;


static Item *
item_new (const gchar *uri)
{
  Item *item = g_slice_new0 (Item);

  item->uri = g_strdup (uri);

  return item;
}

static void
item_free (Item *item)
{
  g_free (item->uri);
  g_free (item->title);

  g_slice_free (Item, item);
}


Queue *
queue_new (void)
{
  return g_slice_new0 (Queue);
}

void
queue_free (Queue *h)
{
  g_return_if_fail (h != NULL);

  /* needs to free data in list as well! */
  if (h->queue)
    {
      g_list_foreach (h->queue, (GFunc) g_free, NULL);
      g_list_free (h->queue);
    }

  g_slice_free (Queue, h);
}

void
queue_move_prev (Queue *h,
                 gint   skip)
{
  if (!h || !h->queue || (h->current == g_list_first (h->queue)))
    return;

  h->current = g_list_previous (h->current);

  while (h->current && skip--)
    h->current = g_list_previous (h->current);
}

void
queue_move_next (Queue *h,
                 gint   skip)
{
  if (!h || !h->queue || (h->current == g_list_last (h->queue)))
    return;

  h->current = g_list_next (h->current);

  while (h->current && skip--)
    h->current = g_list_next (h->current);
}

const gchar *
queue_prev (Queue   *h,
            gint     skip,
            gdouble *pos)
{
  GList *p;
  Item  *item;

  if (!h || !h->queue || (h->current == g_list_first (h->queue)))
    return NULL;

  p = g_list_previous (h->current);

  while (p && skip--)
    p = g_list_previous (p);

  if (!p)
    return NULL;

  item = p->data;

  if (pos)
    *pos = item->pos;

  return (const gchar *) item->uri;
}

const gchar *
queue_next (Queue   *h,
            gint     skip,
            gdouble *pos)
{
  GList *p;
  Item  *item;

  if (!h || !h->queue || (h->current == g_list_last(h->queue)))
    return NULL;

  p = g_list_next (h->current);

  while (p && skip--)
    p = g_list_next (p);

  if (!p)
    return NULL;

  item = p->data;

  if (pos)
    *pos = item->pos;

  return (const gchar *) item->uri;
}

void
queue_add (Queue       *h,
	   const gchar *uri)
{
  GList *trash = NULL;

  g_return_if_fail (h != NULL);
  g_return_if_fail (uri != NULL);

  if (h->current)
    {
      trash = h->current->next;
      h->current->next = NULL;
    }

  h->queue   = g_list_append (h->queue, item_new (uri));
  h->current = g_list_last (h->queue);

  if (trash)
    {
      g_list_foreach (trash, (GFunc) item_free, NULL);
      g_list_free (trash);
    }
}

void
queue_set_title (Queue       *h,
                 const gchar *title)
{
  Item *item;

  g_return_if_fail (h != NULL);
  g_return_if_fail (title != NULL);

  if (! h->current || ! h->current->data)
    return;

  item = h->current->data;

  if (item->title)
    g_free (item->title);

  item->title = g_strdup (title);
}

void
queue_set_scroll_offset (Queue   *h,
                         gdouble  pos)
{
  Item *item;

  g_return_if_fail (h != NULL);

  if (! h->current || ! h->current->data)
    return;

  item = h->current->data;

  item->pos = pos;
}


gboolean
queue_has_next (Queue *h)
{
  if (!h || !h->queue || (h->current == g_list_last (h->queue)))
    return FALSE;

  return (g_list_next (h->current) != NULL);
}

gboolean
queue_has_prev (Queue *h)
{
  if (!h || !h->queue || (h->current == g_list_first (h->queue)))
    return FALSE;

  return (g_list_previous (h->current) != NULL);
}

GList *
queue_list_next (Queue *h)
{
  GList *result = NULL;

  if (queue_has_next (h))
    {
      GList *list;

      for (list = g_list_next (h->current);
           list;
           list = g_list_next (list))
        {
          Item *item = list->data;

          result = g_list_prepend (result,
                                   item->title ? item->title : item->uri);
        }
    }

  return g_list_reverse (result);
}

GList *
queue_list_prev (Queue *h)
{
  GList *result = NULL;

  if (queue_has_prev (h))
    {
      GList *list;

      for (list = g_list_previous (h->current);
           list;
           list = g_list_previous (list))
        {
          Item *item = list->data;

          result = g_list_prepend (result,
                                   item->title ? item->title : item->uri);
        }
    }

  return g_list_reverse (result);
}
