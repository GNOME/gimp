/* gimpparasite.c
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

#include "gimpparasite.h"
#include <string.h>
#include <glib.h>
#include <stdio.h>

GParasite *
gparasite_new (const char *creator, const char *type, guint32 flags, guint32 size,
	      const void *data)
{
  GParasite *p;
  p = (GParasite *)g_malloc(sizeof(GParasite));
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
gparasite_free (GParasite *parasite)
{
  if (!parasite)
    return;
  if (parasite->data)
    g_free(parasite->data);
  g_free(parasite);
}

int
gparasite_has_type (const GParasite *parasite, const char *creator, const char *type)
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

GParasite *
gparasite_copy (const GParasite *parasite)
{
  GParasite *p;
  p = (GParasite *)g_malloc(sizeof(GParasite));
  if (parasite->creator)
    memcpy(p->creator, parasite->creator, 4);
  else
    memset(p->creator, 0, 4);
  if (parasite->type)
    memcpy(p->type, parasite->type, 4);
  else
    memset(p->type, 0, 4);
  p->size = parasite->size;
  if (parasite->size)
    p->data = g_memdup(parasite->data, parasite->size);
  else
    p->data = NULL;
  return p;
}

int
gparasite_is_error(const GParasite *p)
{
  if (p == NULL)
    return TRUE;
  return gparasite_has_type(p, "eror", "eror");
}

GParasite *
gparasite_error()
{
  static GParasite *error_p = NULL;
  if (!error_p)
    error_p = gparasite_new("eror", "eror", 0, 0, NULL);
  return error_p;
}
