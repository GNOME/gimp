/* parasite.c
 * Copyright (C) 1998 Jay Cox <jaycox@earthlink.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "parasiteP.h"
#include "parasite.h"
#include <string.h>
#include <glib.h>

Parasite *
parasite_new (const char *creator, const char *type, guint32 flags,
	      guint32 size, const void *data)
{
  Parasite *p;
  p = (Parasite *)g_malloc(sizeof(Parasite));
  if (creator)
    memcpy(p->creator, creator, 4);
  else
    memset(p->creator, 0, 4);
  if (type)
    memcpy(p->type, type, 4);
  else
    memset(p->type, 0, 4);
  p->flags = flags;
  p->size = size;
  if (size)
    p->data = g_memdup(data, size);
  else
    p->data = NULL;
  return p;
}

void
parasite_free (Parasite *parasite)
{
  g_return_if_fail(parasite != NULL);
  if (parasite->data)
    g_free(parasite->data);
  g_free(parasite);
}

int
parasite_has_type (const Parasite *parasite, const char *creator, const char *type)
{
  if (!parasite)
    return FALSE;
  if (creator && parasite->creator && strncmp(creator, parasite->creator, 4) != 0)
    return FALSE;
  if (creator != 0 && parasite->creator == 0)
    return FALSE;
  if (type && parasite->type && strncmp(type, parasite->type, 4) != 0)
    return FALSE;
  if (type != 0 && parasite->type == 0)
    return FALSE;
  return TRUE;
}

Parasite *
parasite_copy (const Parasite *parasite)
{
  if (parasite == NULL)
    return NULL;
  return parasite_new (&parasite->creator[0], &parasite->type[0],
		       parasite->flags, parasite->size, parasite->data);
}

Parasite *
parasite_error()
{
  static Parasite *error_p = NULL;
  if (!error_p)
    error_p = parasite_new("eror", "eror", 0, 0, NULL);
  return error_p;
}

int
parasite_is_error(const Parasite *p)
{
  if (p == NULL)
    return TRUE;
  return parasite_has_type(p, "eror", "eror");
}

int
parasite_is_persistant(const Parasite *p)
{
  if (p == NULL)
    return FALSE;
  return (p->flags & PARASITE_PERSISTANT);
}

/* parasite list functions */

Parasite *
parasite_find_in_gslist (const GSList *list, const char *creator, const char *type)
{
  while (list)
  {
    if (parasite_has_type((Parasite *)(list->data), creator, type))
      return (Parasite *)(list->data);
    list = list->next;
  }
  return NULL;
}

GSList*
parasite_add_to_gslist (const Parasite *parasite, GSList *list)
{
  Parasite *p;
  if (parasite_is_error(parasite))
    return list;
  if ((p = parasite_find_in_gslist(list, parasite->creator, parasite->type)))
  {
    list = g_slist_remove(list, p);
    parasite_free(p);
  }
  return g_slist_prepend (list, parasite_copy(parasite));
}

GSList*
parasite_gslist_copy (const GSList *list)
{
  GSList *copy = NULL;
  while (list)
  {
    copy = g_slist_append (copy, parasite_copy((Parasite *)list->data));
    list = list->next;
  }
  return copy;
}

void
parasite_gslist_destroy (GSList *list)
{
  while (list)
  {
    parasite_free((Parasite *)list->data);
    list = g_slist_remove (list, list->data);
  }
}
