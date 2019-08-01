/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpparasite.c
 * Copyright (C) 1998 Jay Cox <jaycox@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <process.h>                /* For _getpid() */
#endif

#include "gimpbasetypes.h"

#include "gimpparasite.h"


/**
 * SECTION: gimpparasite
 * @title: GimpParasite
 * @short_description: Arbitrary pieces of data which can be attached
 *                     to various GIMP objects.
 * @see_also: gimp_image_parasite_attach(),
 *            gimp_drawable_parasite_attach(), gimp_parasite_attach()
 *            and their related functions.
 *
 * Arbitrary pieces of data which can be attached to various GIMP objects.
 **/


/*
 * GIMP_TYPE_PARASITE
 */

GType
gimp_parasite_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpParasite",
                                         (GBoxedCopyFunc) gimp_parasite_copy,
                                         (GBoxedFreeFunc) gimp_parasite_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_PARASITE
 */

#define GIMP_PARAM_SPEC_PARASITE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_PARASITE, GimpParamSpecParasite))

typedef struct _GimpParamSpecParasite GimpParamSpecParasite;

struct _GimpParamSpecParasite
{
  GParamSpecBoxed parent_instance;
};

static void       gimp_param_parasite_class_init  (GParamSpecClass *class);
static void       gimp_param_parasite_init        (GParamSpec      *pspec);
static gboolean   gimp_param_parasite_validate    (GParamSpec      *pspec,
                                                   GValue          *value);
static gint       gimp_param_parasite_values_cmp  (GParamSpec      *pspec,
                                                   const GValue    *value1,
                                                   const GValue    *value2);

GType
gimp_param_parasite_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_parasite_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecParasite),
        0,
        (GInstanceInitFunc) gimp_param_parasite_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamParasite",
                                     &type_info, 0);
    }

  return type;
}

static void
gimp_param_parasite_class_init (GParamSpecClass *class)
{
  class->value_type     = GIMP_TYPE_PARASITE;
  class->value_validate = gimp_param_parasite_validate;
  class->values_cmp     = gimp_param_parasite_values_cmp;
}

static void
gimp_param_parasite_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_parasite_validate (GParamSpec *pspec,
                              GValue     *value)
{
  GimpParasite *parasite = value->data[0].v_pointer;

  if (! parasite)
    {
      return TRUE;
    }
  else if (parasite->name == NULL                          ||
           *parasite->name == '\0'                         ||
           ! g_utf8_validate (parasite->name, -1, NULL)    ||
           (parasite->size == 0 && parasite->data != NULL) ||
           (parasite->size >  0 && parasite->data == NULL))
    {
      g_value_set_boxed (value, NULL);
      return TRUE;
    }

  return FALSE;
}

static gint
gimp_param_parasite_values_cmp (GParamSpec   *pspec,
                                const GValue *value1,
                                const GValue *value2)
{
  GimpParasite *parasite1 = value1->data[0].v_pointer;
  GimpParasite *parasite2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! parasite1)
    return parasite2 != NULL ? -1 : 0;
  else if (! parasite2)
    return parasite1 != NULL;
  else
    return gimp_parasite_compare (parasite1, parasite2);
}

GParamSpec *
gimp_param_spec_parasite (const gchar *name,
                          const gchar *nick,
                          const gchar *blurb,
                          GParamFlags  flags)
{
  GimpParamSpecParasite *parasite_spec;

  parasite_spec = g_param_spec_internal (GIMP_TYPE_PARAM_PARASITE,
                                         name, nick, blurb, flags);

  return G_PARAM_SPEC (parasite_spec);
}


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
                   gconstpointer   data)
{
  GimpParasite *parasite;

  if (! (name && *name))
    return NULL;

  parasite = g_slice_new (GimpParasite);
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

  g_slice_free (GimpParasite, parasite);
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

gconstpointer
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
