/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ParamSpecs for config objects
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbasetypes.h"
#include "libgimpbase/gimpunit.h"

#include "gimpconfig-params.h"
#include "gimpconfig-types.h"


static void  gimp_param_memsize_class_init (GParamSpecClass *class);

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
        sizeof (GParamSpecUInt),
        0, NULL, NULL
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_UINT, 
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

GParamSpec *
gimp_param_spec_memsize (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         guint        minimum,
                         guint        maximum,
                         guint        default_value,
                         GParamFlags  flags)
{
  GParamSpecUInt *pspec;

  pspec = g_param_spec_internal (GIMP_TYPE_PARAM_MEMSIZE,
                                 name, nick, blurb, flags);
  
  pspec->minimum = minimum;
  pspec->maximum = maximum;
  pspec->default_value = default_value;
  
  return G_PARAM_SPEC (pspec);
}


static void  gimp_param_path_class_init (GParamSpecClass *class);

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
        sizeof (GParamSpecString),
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

GParamSpec *
gimp_param_spec_path (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gchar       *default_value,
                      GParamFlags  flags)
{
  GParamSpecString *pspec;

  pspec = g_param_spec_internal (GIMP_TYPE_PARAM_PATH,
                                 name, nick, blurb, flags);
  
  pspec->default_value = default_value;
  
  return G_PARAM_SPEC (pspec);
}


static void      gimp_param_unit_class_init     (GParamSpecClass *class);
static gboolean  gimp_param_unit_value_validate (GParamSpec      *pspec,
                                                 GValue          *value);

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
        sizeof (GParamSpecInt),
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
  GParamSpecInt *ispec = G_PARAM_SPEC_INT (pspec);
  gint oval = value->data[0].v_int;
  
  value->data[0].v_int = CLAMP (value->data[0].v_int, 
                                ispec->minimum, 
                                gimp_unit_get_number_of_units () - 1);
  
  return value->data[0].v_int != oval;
}

GParamSpec *
gimp_param_spec_unit (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      GimpUnit     default_value,
                      GParamFlags  flags)
{
  GParamSpecInt *pspec;

  pspec = g_param_spec_internal (GIMP_TYPE_PARAM_UNIT,
                                 name, nick, blurb, flags);

  pspec->default_value = default_value;
  pspec->minimum       = GIMP_UNIT_INCH;
  pspec->maximum       = G_MAXINT;

  return G_PARAM_SPEC (pspec);
}
