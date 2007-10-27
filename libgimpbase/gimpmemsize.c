/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#include <errno.h>

#include <glib-object.h>

#include "gimpmemsize.h"

#include "libgimp/libgimp-intl.h"


static void   memsize_to_string (const GValue *src_value,
                                 GValue       *dest_value);
static void   string_to_memsize (const GValue *src_value,
                                 GValue       *dest_value);


GType
gimp_memsize_get_type (void)
{
  static GType memsize_type = 0;

  if (! memsize_type)
    {
      const GTypeInfo type_info = { 0, };

      memsize_type = g_type_register_static (G_TYPE_UINT64, "GimpMemsize",
                                             &type_info, 0);

      g_value_register_transform_func (memsize_type, G_TYPE_STRING,
                                       memsize_to_string);
      g_value_register_transform_func (G_TYPE_STRING, memsize_type,
                                       string_to_memsize);
    }

  return memsize_type;
}

/**
 * gimp_memsize_serialize:
 * @memsize: memory size in bytes
 *
 * Creates a string representation of a given memory size. This string
 * can be parsed by gimp_memsize_deserialize() and can thus be used in
 * config files. It should not be displayed to the user. If you need a
 * nice human-readable string please use gimp_memsize_to_string().
 *
 * Return value: A newly allocated string representation of @memsize.
 *
 * Since: GIMP 2.2
 **/
gchar *
gimp_memsize_serialize (guint64 memsize)
{
  if (memsize > (1 << 30) && memsize % (1 << 30) == 0)
    return g_strdup_printf ("%" G_GUINT64_FORMAT "G", memsize >> 30);
  else if (memsize > (1 << 20) && memsize % (1 << 20) == 0)
    return g_strdup_printf ("%" G_GUINT64_FORMAT "M", memsize >> 20);
  else if (memsize > (1 << 10) && memsize % (1 << 10) == 0)
    return g_strdup_printf ("%" G_GUINT64_FORMAT "k", memsize >> 10);
  else
    return g_strdup_printf ("%" G_GUINT64_FORMAT, memsize);
}

/**
 * gimp_memsize_deserialize:
 * @string:  a string as returned by gimp_memsize_serialize()
 * @memsize: return location for memory size in bytes
 *
 * Parses a string representation of a memory size as returned by
 * gimp_memsize_serialize().
 *
 * Return value: %TRUE if the @string was successfully parsed and
 *               @memsize has been set, %FALSE otherwise.
 *
 * Since: GIMP 2.2
 **/
gboolean
gimp_memsize_deserialize (const gchar *string,
                          guint64     *memsize)
{
  gchar   *end;
  guint64  size;

  g_return_val_if_fail (string != NULL, FALSE);
  g_return_val_if_fail (memsize != NULL, FALSE);

  size = g_ascii_strtoull (string, &end, 0);

  if (size == G_MAXUINT64 && errno == ERANGE)
    return FALSE;

  if (end && *end)
    {
      guint shift;

      switch (g_ascii_tolower (*end))
        {
        case 'b':
          shift = 0;
          break;
        case 'k':
          shift = 10;
          break;
        case 'm':
          shift = 20;
          break;
        case 'g':
          shift = 30;
          break;
        default:
          return FALSE;
        }

      /* protect against overflow */
      if (shift)
        {
          guint64  limit = G_MAXUINT64 >> shift;

          if (size != (size & limit))
            return FALSE;

          size <<= shift;
        }
    }

  *memsize = size;

  return TRUE;
}


/**
 * gimp_memsize_to_string:
 * @memsize: A memory size in bytes.
 *
 * This function returns a human readable, translated representation
 * of the passed @memsize. Large values are displayed using a
 * reasonable memsize unit, e.g.: "345" becomes "345 Bytes", "4500"
 * becomes "4.4 KB" and so on.
 *
 * Return value: A newly allocated human-readable, translated string.
 **/
gchar *
gimp_memsize_to_string (guint64 memsize)
{
#if defined _MSC_VER && (_MSC_VER < 1300)
/* sorry, error C2520: conversion from unsigned __int64 to double not
 *                     implemented, use signed __int64
 */
#  define CAST_DOUBLE (gdouble)(gint64)
#else
#  define CAST_DOUBLE (gdouble)
#endif

  if (memsize < 1024)
    {
      gint bytes = (gint) memsize;

      return g_strdup_printf (dngettext (GETTEXT_PACKAGE "-libgimp",
                                         "%d Byte",
                                         "%d Bytes", bytes), bytes);
    }

  if (memsize < 1024 * 10)
    {
      return g_strdup_printf (_("%.2f KB"), CAST_DOUBLE memsize / 1024.0);
    }
  else if (memsize < 1024 * 100)
    {
      return g_strdup_printf (_("%.1f KB"), CAST_DOUBLE memsize / 1024.0);
    }
  else if (memsize < 1024 * 1024)
    {
      return g_strdup_printf (_("%d KB"), (gint) memsize / 1024);
    }

  memsize /= 1024;

  if (memsize < 1024 * 10)
    {
      return g_strdup_printf (_("%.2f MB"), CAST_DOUBLE memsize / 1024.0);
    }
  else if (memsize < 1024 * 100)
    {
      return g_strdup_printf (_("%.1f MB"), CAST_DOUBLE memsize / 1024.0);
    }
  else if (memsize < 1024 * 1024)
    {
      return g_strdup_printf (_("%d MB"), (gint) memsize / 1024);
    }

  memsize /= 1024;

  if (memsize < 1024 * 10)
    {
      return g_strdup_printf (_("%.2f GB"), CAST_DOUBLE memsize / 1024.0);
    }
  else if (memsize < 1024 * 100)
    {
      return g_strdup_printf (_("%.1f GB"), CAST_DOUBLE memsize / 1024.0);
    }
  else
    {
      return g_strdup_printf (_("%d GB"), (gint) memsize / 1024);
    }
#undef CAST_DOUBLE
}


static void
memsize_to_string (const GValue *src_value,
                   GValue       *dest_value)
{
  g_value_take_string (dest_value,
                       gimp_memsize_serialize (g_value_get_uint64 (src_value)));
}

static void
string_to_memsize (const GValue *src_value,
                   GValue       *dest_value)
{
  const gchar *str;
  guint64      memsize;

  str = g_value_get_string (src_value);

  if (str && gimp_memsize_deserialize (str, &memsize))
    {
      g_value_set_uint64 (dest_value, memsize);
    }
  else
    {
      g_warning ("Can't convert string to GimpMemsize.");
    }
}


/*
 * GIMP_TYPE_PARAM_MEMSIZE
 */

static void  gimp_param_memsize_class_init (GParamSpecClass *class);

/**
 * gimp_param_memsize_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a memsize object
 *
 * Since: GIMP 2.4
 **/
GType
gimp_param_memsize_get_type (void)
{
  static GType spec_type = 0;

  if (! spec_type)
    {
      const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_memsize_class_init,
        NULL, NULL,
        sizeof (GParamSpecUInt64),
        0, NULL, NULL
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_UINT64,
                                          "GimpParamMemsize",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
gimp_param_memsize_class_init (GParamSpecClass *class)
{
  class->value_type = GIMP_TYPE_MEMSIZE;
}

/**
 * gimp_param_spec_memsize:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief desciption of param.
 * @minimum:       Smallest allowed value of the parameter.
 * @maximum:       Largest allowed value of the parameter.
 * @default_value: Value to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a memory size value.
 * See g_param_spec_internal() for more information.
 *
 * Returns: a newly allocated #GParamSpec instance
 *
 * Since: GIMP 2.4
 **/
GParamSpec *
gimp_param_spec_memsize (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         guint64      minimum,
                         guint64      maximum,
                         guint64      default_value,
                         GParamFlags  flags)
{
  GParamSpecUInt64 *pspec;

  pspec = g_param_spec_internal (GIMP_TYPE_PARAM_MEMSIZE,
                                 name, nick, blurb, flags);

  pspec->minimum       = minimum;
  pspec->maximum       = maximum;
  pspec->default_value = default_value;

  return G_PARAM_SPEC (pspec);
}

