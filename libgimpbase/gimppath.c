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

#include <glib-object.h>

#include "gimpbasetypes.h"

#include "gimppath.h"

/*
 * GIMP_TYPE_PARAM_PATH
 */

#define GIMP_PARAM_SPEC_PATH(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_PATH, GimpParamSpecPath))

typedef struct _GimpParamSpecPath GimpParamSpecPath;

struct _GimpParamSpecPath
{
  GParamSpecString   parent_instance;

  GimpParamPathType  type;
};

static void  gimp_param_path_class_init (GParamSpecClass *class);

/**
 * gimp_param_path_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a directory path object
 *
 * Since: GIMP 2.4
 **/
GType
gimp_param_path_get_type (void)
{
  static GType spec_type = 0;

  if (!spec_type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_path_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecPath),
        0, NULL, NULL
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_STRING,
                                          "GimpParamPath",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
gimp_param_path_class_init (GParamSpecClass *class)
{
  class->value_type = GIMP_TYPE_PATH;
}

/**
 * gimp_param_spec_path:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief desciption of param.
 * @type:          a #GimpParamPathType value.
 * @default_value: Value to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a filename, dir name,
 * or list of file or dir names.
 * See g_param_spec_internal() for more information.
 *
 * Returns: a newly allocated #GParamSpec instance
 *
 * Since: GIMP 2.4
 **/
GParamSpec *
gimp_param_spec_path (const gchar        *name,
                      const gchar        *nick,
                      const gchar        *blurb,
		      GimpParamPathType   type,
                      gchar              *default_value,
                      GParamFlags         flags)
{
  GParamSpecString *pspec;

  pspec = g_param_spec_internal (GIMP_TYPE_PARAM_PATH,
                                 name, nick, blurb, flags);


  pspec->default_value = default_value;

  GIMP_PARAM_SPEC_PATH (pspec)->type = type;

  return G_PARAM_SPEC (pspec);
}

/**
 * gimp_param_spec_path_get_path_type:
 * @pspec:         A #GParamSpec for a path param
 *
 * Tells whether the path param encodes a filename,
 * dir name, or list of file or dir names.
 *
 * Returns: a #GimpParamPathType value
 *
 * Since: GIMP 2.4
 **/
GimpParamPathType
gimp_param_spec_path_type (GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_PATH (pspec), 0);

  return GIMP_PARAM_SPEC_PATH (pspec)->type;
}
