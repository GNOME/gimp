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

#include "gimpconfig-serialize.h"
#include "gimpconfig-types.h"


void
gimp_config_serialize_properties (GObject *object,
                                  FILE    *file)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;
  GValue         value = { 0, };
  
  klass = G_OBJECT_GET_CLASS (object);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec  *prop_spec;
      const gchar *cstr;
      gchar       *str;

      prop_spec = property_specs[i];

      if (! (prop_spec->flags & G_PARAM_READWRITE))
        continue;

      str  = NULL;
      cstr = NULL;
      g_value_init (&value, prop_spec->value_type);

      g_object_get_property (object, prop_spec->name, &value);

      if (G_VALUE_HOLDS_BOOLEAN (&value))
        {
          gboolean bool;

          bool = g_value_get_boolean (&value);
          cstr = bool ? "yes" : "no";
        }
      else if (G_VALUE_HOLDS_ENUM (&value))
        {
          GEnumClass *enum_class;
          GEnumValue *enum_value;

          enum_class = g_type_class_peek (prop_spec->value_type);
          enum_value = g_enum_get_value (G_ENUM_CLASS (enum_class),
                                         g_value_get_enum (&value));

          if (enum_value && enum_value->value_nick)
            cstr = enum_value->value_nick;
          else
            g_warning ("Couldn't get nick for enum_value of %s", 
                       G_ENUM_CLASS_TYPE_NAME (enum_class));
        }
      else if (G_VALUE_HOLDS_STRING (&value))
        {
          cstr = g_value_get_string (&value);
          
          if (cstr)
            {
              gchar *s = g_strescape (cstr, NULL);
              
              str = g_strdup_printf ("\"%s\"", s);
              g_free (s);
              cstr = NULL;
            }
        }
      else if (g_value_type_transformable (prop_spec->value_type, 
                                           G_TYPE_STRING))
        {
          GValue tmp_value = { 0, };
          
          g_value_init (&tmp_value, G_TYPE_STRING);
          g_value_transform (&value, &tmp_value);
          str = g_value_dup_string (&tmp_value);
          g_value_unset (&tmp_value);
        }
      else
        {
          g_warning ("couldn't serialize property %s::%s of type %s",
                     g_type_name (G_TYPE_FROM_INSTANCE (object)),
                     prop_spec->name, 
                     g_type_name (prop_spec->value_type));
        }

      if (cstr || str)
        {
          fprintf (file, "(%s %s)\n", prop_spec->name, cstr ? cstr : str);
          g_free (str);
        }

      g_value_unset (&value);
    }
}
