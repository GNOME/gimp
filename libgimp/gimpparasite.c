/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpparasite.c
 * Copyright (C) 1998 Jay Cox <jaycox@earthlink.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "parasiteP.h"
#include "parasite.h"
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <glib.h>

#ifdef G_OS_WIN32
#include <process.h>		/* For _getpid() */
#endif

#ifdef DEBUG
static void 
gimp_parasite_print (GimpParasite *parasite)
{
  if (parasite == NULL)
    {
      g_print ("pid %d: attempt to print a null parasite\n", getpid ());
      return;
    }

  g_print ("pid %d: parasite: %p\n", getpid (), parasite);

  if (parasite->name)
    g_print ("\tname: %s\n", parasite->name);
  else
    g_print ("\tname: NULL\n");

  g_print ("\tflags: %d\n", parasite->flags);
  g_print ("\tsize: %d\n", parasite->size);
  if (parasite->size > 0)
    g_print ("\tdata: %p\n", parasite->data);
}
#endif

GimpParasite *
gimp_parasite_new (const gchar    *name, 
		   guint32         flags,
		   guint32         size, 
		   const gpointer  data)
{
  GimpParasite *parasite;

  if (!name)
    return NULL;

  parasite = g_new (GimpParasite, 1);
  parasite->name  = g_strdup (name);
  parasite->flags = (flags & 0xFF);
  parasite->size  = size;
  if (size)
    parasite->data = g_memdup (data, size);
  else
    parasite->data = NULL;

  return parasite;
}

void
gimp_parasite_free (GimpParasite *parasite)
{
  if (parasite == NULL)
    return;

  if (parasite->name)
    g_free (parasite->name);
  if (parasite->data)
    g_free (parasite->data);

  g_free (parasite);
}

gboolean
gimp_parasite_is_type (const GimpParasite *parasite, 
		       const gchar        *name)
{
  if (!parasite || !parasite->name)
    return FALSE;

  return (strcmp (parasite->name, name) == 0);
}

GimpParasite *
gimp_parasite_copy (const GimpParasite *parasite)
{
  if (parasite == NULL)
    return NULL;

  return gimp_parasite_new (parasite->name, parasite->flags,
			    parasite->size, parasite->data);
}

gboolean
gimp_parasite_compare (const GimpParasite *a, 
		       const GimpParasite *b)
{
  if (a && b &&
      a->name && b->name &&
      strcmp (a->name, b->name) == 0 &&
      a->flags == b->flags &&
      a->size == b->size)
    {
      if (a->data == NULL && b->data == NULL)  
	return TRUE;
      else if (a->data && b->data && memcmp (a->data, b->data, a->size) == 0)
	return TRUE;
    }

  return FALSE;
}

gulong
gimp_parasite_flags (const GimpParasite *parasite)
{
  if (parasite == NULL)
    return 0;

  return parasite->flags;
}

gboolean
gimp_parasite_is_persistent (const GimpParasite *parasite)
{
  if (parasite == NULL)
    return FALSE;

  return (parasite->flags & GIMP_PARASITE_PERSISTENT);
}

gboolean
gimp_parasite_is_undoable (const GimpParasite *parasite)
{
  if (parasite == NULL)
    return FALSE;

  return (parasite->flags & GIMP_PARASITE_UNDOABLE);
}

gboolean
gimp_parasite_has_flag (const GimpParasite *parasite, 
			gulong              flag)
{
  if (parasite == NULL)
    return FALSE;

  return (parasite->flags & flag);
}

const gchar *
gimp_parasite_name (const GimpParasite *parasite)
{
  if (parasite)
    return parasite->name;

  return NULL;
}

void *
gimp_parasite_data (const GimpParasite *parasite)
{
  if (parasite)
    return parasite->data;

  return NULL;
}

glong 
gimp_parasite_data_size (const GimpParasite *parasite)
{
  if (parasite)
    return parasite->size;

  return 0;
}
