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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>

static void parasite_print(Parasite *p)
{
  if (p == NULL)
  {
    printf("(pid %d)attempt to print a null parasite\n", getpid());
    return;
  }
  printf("(pid %d), parasite: %p\n", getpid(), p);
  if (p->name)
    printf("\tname: %s\n", p->name);
  else
    printf("\tname: NULL\n");
  printf("\tflags: %d\n", p->flags);
  printf("\tsize: %d\n", p->size);
  if (p->size > 0)
    printf("\tdata: %p\n", p->data);
}

Parasite *
parasite_new (const char *name, guint32 flags,
	      guint32 size, const void *data)
{
  Parasite *p;
  p = (Parasite *)g_malloc(sizeof(Parasite));
  if (name)
    p->name = g_strdup(name);
  else
  {
    p->name = NULL;
    g_free (p);
    return NULL;
  }
  p->flags = (flags & 0xFF);
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
  if (parasite == NULL)
    return;
  if (parasite->name)
    g_free(parasite->name);
  if (parasite->data)
    g_free(parasite->data);
  g_free(parasite);
}

int
parasite_is_type (const Parasite *parasite, const char *name)
{
  if (!parasite || !parasite->name)
    return FALSE;
  return (strcmp(parasite->name, name) == 0);
}

Parasite *
parasite_copy (const Parasite *parasite)
{
  if (parasite == NULL)
    return NULL;
  return parasite_new (parasite->name, parasite->flags,
		       parasite->size, parasite->data);
}

int
parasite_is_persistent(const Parasite *p)
{
  if (p == NULL)
    return FALSE;
  return (p->flags & PARASITE_PERSISTENT);
}

int
parasite_has_flag(const Parasite *p, gulong flag)
{
  if (p == NULL)
    return FALSE;
  return (p->flags & flag);
}

void *parasite_data(const Parasite *p)
{
  if (p)
    return p->data;
  return NULL;
}

long parasite_data_size(const Parasite *p)
{
  if (p)
    return p->size;
  return 0;
}
