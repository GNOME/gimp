/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Object propoerties serialization routines
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

#include <stdio.h>
#include <string.h>

#include <glib-object.h>

#include "gimpconfig.h"
#include "gimpconfig-serialize.h"


void
gimp_config_serialize_properties (GimpConfig *config,
                                  FILE       *file)
{
  GimpConfigClass  *klass;
  GParamSpec      **property_specs;
  guint             n_property_specs;
  guint             i;
  GValue            value = { 0, };
  
  klass = GIMP_CONFIG_GET_CLASS (config);

  property_specs = g_object_class_list_properties (G_OBJECT_CLASS (klass),
                                                   &n_property_specs);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec;
      gchar      *str;

      prop_spec = property_specs[i];

      if (! (prop_spec->flags & G_PARAM_READWRITE &&
             g_type_is_a (prop_spec->owner_type, GIMP_TYPE_CONFIG)))
        continue;

      str = NULL;
      g_value_init (&value, prop_spec->value_type);

      g_object_get_property (G_OBJECT (config), prop_spec->name, &value);

      if (G_VALUE_HOLDS_STRING (&value))
        {
          const gchar *src;

          src = g_value_get_string (&value);
          
          if (!src)
            str = g_strdup ("NULL");
          else
            {
              gchar *s = g_strescape (src, NULL);
              
              str = g_strdup_printf ("\"%s\"", s);
              g_free (s);
            }
        }
      else if (g_value_type_transformable (G_VALUE_TYPE (&value), 
                                           G_TYPE_STRING))
        {
          GValue tmp_value = { 0, };
          
          g_value_init (&tmp_value, G_TYPE_STRING);
          g_value_transform (&value, &tmp_value);
          str = g_strescape (g_value_get_string (&tmp_value), NULL);
          g_value_unset (&tmp_value);
        }

      fprintf (file, "(%s %s)\n", prop_spec->name, str);

      g_free (str);
      g_value_unset (&value);
    }
}
