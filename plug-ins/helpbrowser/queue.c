/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help Browser
 * Copyright (C) 1999 Sven Neumann <sven@gimp.org>
 *                    Michael Natterer <mitschel@cs.tu-berlin.de>
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

Queue *
queue_new (void)
{
  Queue *h;

  h = g_malloc (sizeof (Queue));
  
  h->queue = NULL;
  h->current = NULL;

  return (h);
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

  g_free (h);
}

void
queue_move_prev (Queue *h)
{
  if (!h || !h->queue || (h->current == g_list_first (h->queue)))
    return;

  h->current = g_list_previous (h->current);
}

void
queue_move_next (Queue *h)
{
  if (!h || !h->queue || (h->current == g_list_last (h->queue)))
    return;

  h->current = g_list_next (h->current);
}

const gchar *
queue_prev (Queue *h)
{
  GList *p;

  if (!h || !h->queue || (h->current == g_list_first (h->queue)))
    return NULL;

  p = g_list_previous (h->current);

  return (const gchar *) p->data;
}

const gchar *
queue_next (Queue *h)
{
  GList *p;

  if (!h || !h->queue || (h->current == g_list_last(h->queue)))
    return NULL;

  p = g_list_next (h->current);

  return (const gchar *) p->data;
}

void 
queue_add (Queue       *h,
	   const gchar *ref)
{
  GList *trash = NULL;

  g_return_if_fail (h != NULL);
  g_return_if_fail (ref != NULL);

  if (h->current)
    {
      trash = h->current->next;
      h->current->next = NULL;
    }

  h->queue   = g_list_append (h->queue, g_strdup (ref));
  h->current = g_list_last (h->queue);

  if (trash)
    {
      g_list_foreach (trash, (GFunc) g_free, NULL);
      g_list_free (trash);
    }	
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
