/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ParamSpecs for config objects
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
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

#include "libgimpbase/gimpbase.h"

#include "gimpbaseparams.h"
#include "gimpbasetypes.h"


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

  if (!spec_type)
    {
      static const GTypeInfo type_info =
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


/*
 * GIMP_TYPE_PARAM_UNIT
 */

#define GIMP_PARAM_SPEC_UNIT(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_UNIT, GimpParamSpecUnit))

typedef struct _GimpParamSpecUnit GimpParamSpecUnit;

struct _GimpParamSpecUnit
{
  GParamSpecInt parent_instance;

  gboolean      allow_percent;
};

static void      gimp_param_unit_class_init     (GParamSpecClass *class);
static gboolean  gimp_param_unit_value_validate (GParamSpec      *pspec,
                                                 GValue          *value);

/**
 * gimp_param_unit_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a unit param object
 *
 * Since: GIMP 2.4
 **/
GType
gimp_param_unit_get_type (void)
{
  static GType spec_type = 0;

  if (!spec_type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_unit_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecUnit),
        0, NULL, NULL
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_INT,
                                          "GimpParamUnit",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
gimp_param_unit_class_init (GParamSpecClass *class)
{
  class->value_type     = GIMP_TYPE_UNIT;
  class->value_validate = gimp_param_unit_value_validate;
}

static gboolean
gimp_param_unit_value_validate (GParamSpec *pspec,
                                GValue     *value)
{
  GParamSpecInt     *ispec = G_PARAM_SPEC_INT (pspec);
  GimpParamSpecUnit *uspec = GIMP_PARAM_SPEC_UNIT (pspec);
  gint               oval  = value->data[0].v_int;

  if (uspec->allow_percent && value->data[0].v_int == GIMP_UNIT_PERCENT)
    {
      value->data[0].v_int = value->data[0].v_int;
    }
  else
    {
      value->data[0].v_int = CLAMP (value->data[0].v_int,
                                    ispec->minimum,
                                    gimp_unit_get_number_of_units () - 1);
    }

  return value->data[0].v_int != oval;
}

/**
 * gimp_param_spec_unit:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief desciption of param.
 * @allow_pixels:  Whether "pixels" is an allowed unit.
 * @allow_percent: Whether "perecent" is an allowed unit.
 * @default_value: Unit to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a units param.
 * See g_param_spec_internal() for more information.
 *
 * Returns: a newly allocated #GParamSpec instance
 *
 * Since: GIMP 2.4
 **/
GParamSpec *
gimp_param_spec_unit (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gboolean     allow_pixels,
                      gboolean     allow_percent,
                      GimpUnit     default_value,
                      GParamFlags  flags)
{
  GimpParamSpecUnit *pspec;
  GParamSpecInt     *ispec;

  pspec = g_param_spec_internal (GIMP_TYPE_PARAM_UNIT,
                                 name, nick, blurb, flags);

  ispec = G_PARAM_SPEC_INT (pspec);

  ispec->default_value = default_value;
  ispec->minimum       = allow_pixels ? GIMP_UNIT_PIXEL : GIMP_UNIT_INCH;
  ispec->maximum       = GIMP_UNIT_PERCENT - 1;

  pspec->allow_percent = allow_percent;

  return G_PARAM_SPEC (pspec);
}
