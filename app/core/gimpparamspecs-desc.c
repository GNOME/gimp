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

#include <string.h>

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpparamspecs.h"
#include "gimpparamspecs-desc.h"


static inline const gchar *
gimp_param_spec_get_blurb (GParamSpec *pspec)
{
  const gchar *blurb = g_param_spec_get_blurb (pspec);

  return blurb ? blurb : "";
}

static gchar *
gimp_param_spec_boolean_desc (GParamSpec *pspec)
{
  const gchar *blurb = gimp_param_spec_get_blurb (pspec);

  return g_strconcat (blurb, " (TRUE or FALSE)", NULL);
}

static gchar *
gimp_param_spec_int32_desc (GParamSpec *pspec)
{
  GParamSpecInt *ispec = G_PARAM_SPEC_INT (pspec);
  const gchar   *blurb = gimp_param_spec_get_blurb (pspec);

  if (ispec->minimum == G_MININT32 && ispec->maximum == G_MAXINT32)
    return g_strdup (blurb);

  if (ispec->minimum == G_MININT32)
    return g_strdup_printf ("%s (%s <= %d)", blurb,
                            g_param_spec_get_name (pspec),
                            ispec->maximum);

  if (ispec->maximum == G_MAXINT32)
    return g_strdup_printf ("%s (%s >= %d)", blurb,
                            g_param_spec_get_name (pspec),
                            ispec->minimum);

  return g_strdup_printf ("%s (%d <= %s <= %d)", blurb,
                          ispec->minimum,
                          g_param_spec_get_name (pspec),
                          ispec->maximum);
}

static gchar *
gimp_param_spec_double_desc (GParamSpec *pspec)
{
  GParamSpecDouble *dspec = G_PARAM_SPEC_DOUBLE (pspec);
  const gchar      *blurb = gimp_param_spec_get_blurb (pspec);

  if (dspec->minimum == - G_MAXDOUBLE && dspec->maximum == G_MAXDOUBLE)
    return g_strdup (blurb);

  if (dspec->minimum == - G_MAXDOUBLE)
    return g_strdup_printf ("%s (%s <= %g)", blurb,
                            g_param_spec_get_name (pspec),
                            dspec->maximum);

  if (dspec->maximum == G_MAXDOUBLE)
    return g_strdup_printf ("%s (%s >= %g)", blurb,
                            g_param_spec_get_name (pspec),
                            dspec->minimum);

  return g_strdup_printf ("%s (%g <= %s <= %g)", blurb,
                          dspec->minimum,
                          g_param_spec_get_name (pspec),
                          dspec->maximum);
}

static gchar *
gimp_param_spec_enum_desc (GParamSpec *pspec)
{
  const gchar    *blurb      = gimp_param_spec_get_blurb (pspec);
  GString        *str        = g_string_new (blurb);
  GEnumClass     *enum_class = g_type_class_peek (pspec->value_type);
  GEnumValue     *enum_value;
  GSList         *excluded;
  gint            i, n;

  if (GIMP_IS_PARAM_SPEC_ENUM (pspec))
    excluded = GIMP_PARAM_SPEC_ENUM (pspec)->excluded_values;
  else
    excluded = NULL;

  g_string_append (str, " { ");

  for (i = 0, n = 0, enum_value = enum_class->values;
       i < enum_class->n_values;
       i++, enum_value++)
    {
      GSList *list;
      gchar  *name;

      for (list = excluded; list; list = list->next)
        {
          gint value = GPOINTER_TO_INT (list->data);

          if (value == enum_value->value)
            break;
        }

      if (list)
        continue;

      if (n > 0)
        g_string_append (str, ", ");

      if (G_LIKELY (g_str_has_prefix (enum_value->value_name, "GIMP_")))
        name = gimp_canonicalize_identifier (enum_value->value_name + 5);
      else
        name = gimp_canonicalize_identifier (enum_value->value_name);

      g_string_append (str, name);
      g_free (name);

      g_string_append_printf (str, " (%d)", enum_value->value);

      n++;
    }

  g_string_append (str, " }");

  return g_string_free (str, FALSE);
}

/**
 * gimp_param_spec_get_desc:
 * @pspec: a #GParamSpec
 *
 * This function creates a description of the passed @pspec, which is
 * suitable for use in the PDB.  Actually, it currently only deals with
 * parameter types used in the PDB and should not be used for anything
 * else.
 *
 * Return value: A newly allocated string describing the parameter.
 */
gchar *
gimp_param_spec_get_desc (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);

  if (GIMP_IS_PARAM_SPEC_UNIT (pspec))
    {
    }
  else if (GIMP_IS_PARAM_SPEC_INT32 (pspec))
    {
      return gimp_param_spec_int32_desc (pspec);
    }
  else
    {
      switch (G_TYPE_FUNDAMENTAL (pspec->value_type))
        {
        case G_TYPE_BOOLEAN:
          return gimp_param_spec_boolean_desc (pspec);

        case G_TYPE_DOUBLE:
          return gimp_param_spec_double_desc (pspec);

        case G_TYPE_ENUM:
          return gimp_param_spec_enum_desc (pspec);
        }
    }

  return g_strdup (g_param_spec_get_blurb (pspec));
}
