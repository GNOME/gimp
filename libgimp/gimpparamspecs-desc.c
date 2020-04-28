/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpparamspecs-desc.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "gimp.h"
#include "gimpparamspecs-desc.h"


static gchar *
gimp_param_spec_boolean_desc (GParamSpec *pspec)
{
  GParamSpecBoolean *bspec = G_PARAM_SPEC_BOOLEAN (pspec);

  return g_strdup_printf ("(TRUE or FALSE, default %s)",
                          bspec->default_value ? "TRUE" : "FALSE");
}

static gchar *
gimp_param_spec_int_desc (GParamSpec *pspec)
{
  GParamSpecInt *ispec = G_PARAM_SPEC_INT (pspec);

  if (ispec->minimum == G_MININT32 && ispec->maximum == G_MAXINT32)
    return g_strdup_printf ("(default %d)",
                            ispec->default_value);

  if (ispec->minimum == G_MININT32)
    return g_strdup_printf ("(%s <= %d, default %d)",
                            g_param_spec_get_name (pspec),
                            ispec->maximum,
                            ispec->default_value);

  if (ispec->maximum == G_MAXINT32)
    return g_strdup_printf ("(%s >= %d, default %d)",
                            g_param_spec_get_name (pspec),
                            ispec->minimum,
                            ispec->default_value);

  return g_strdup_printf ("(%d <= %s <= %d, default %d)",
                          ispec->minimum,
                          g_param_spec_get_name (pspec),
                          ispec->maximum,
                          ispec->default_value);
}

static gchar *
gimp_param_spec_double_desc (GParamSpec *pspec)
{
  GParamSpecDouble *dspec = G_PARAM_SPEC_DOUBLE (pspec);

  if (dspec->minimum == - G_MAXDOUBLE && dspec->maximum == G_MAXDOUBLE)
    return g_strdup_printf ("(default %g)",
                            dspec->default_value);

  if (dspec->minimum == - G_MAXDOUBLE)
    return g_strdup_printf ("(%s <= %g, default %g)",
                            g_param_spec_get_name (pspec),
                            dspec->maximum,
                            dspec->default_value);

  if (dspec->maximum == G_MAXDOUBLE)
    return g_strdup_printf ("(%s >= %g, default %g)",
                            g_param_spec_get_name (pspec),
                            dspec->minimum,
                            dspec->default_value);

  return g_strdup_printf ("(%g <= %s <= %g, default %g)",
                          dspec->minimum,
                          g_param_spec_get_name (pspec),
                          dspec->maximum,
                          dspec->default_value);
}

static gchar *
gimp_param_spec_enum_desc (GParamSpec *pspec)
{
  GParamSpecEnum *espec      = G_PARAM_SPEC_ENUM (pspec);
  GEnumClass     *enum_class = g_type_class_peek (pspec->value_type);
  GEnumValue     *enum_value;
  GSList         *excluded;
  GString        *str          = g_string_new (NULL);
  gchar          *default_name = NULL;
  gint            i, n;

#if 0
  if (GIMP_IS_PARAM_SPEC_ENUM (pspec))
    excluded = GIMP_PARAM_SPEC_ENUM (pspec)->excluded_values;
  else
#endif
    excluded = NULL;

  g_string_append (str, "{ ");

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

      if (enum_value->value == espec->default_value)
        default_name = g_strdup (name);

      g_string_append (str, name);
      g_free (name);

      g_string_append_printf (str, " (%d)", enum_value->value);

      n++;
    }

  g_string_append (str, " }");

  if (default_name)
    {
      g_string_append_printf (str, ", default %s (%d)",
                              default_name, espec->default_value);
      g_free (default_name);
    }

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
 * Returns: A newly allocated string describing the parameter.
 *
 * Since: 3.0
 */
gchar *
gimp_param_spec_get_desc (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);

  if (GIMP_IS_PARAM_SPEC_UNIT (pspec))
    {
    }
  else if (G_IS_PARAM_SPEC_INT (pspec))
    {
      return gimp_param_spec_int_desc (pspec);
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

  return NULL;
}
