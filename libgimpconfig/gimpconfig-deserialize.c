/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Object properties deserialization routines
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
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <glib-object.h>

#include "gimpconfig-deserialize.h"
#include "gimpconfig-types.h"


static void  gimp_config_deserialize_property (GObject    *object,
                                               GScanner   *scanner,
                                               GTokenType *token);


gboolean
gimp_config_deserialize_properties (GObject  *object,
                                    GScanner *scanner)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;
  guint          scope_id;
  guint          old_scope_id;
  GTokenType	 token;

  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);

  klass = G_OBJECT_GET_CLASS (object);
  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  g_return_val_if_fail (property_specs != NULL && n_property_specs > 0, FALSE);

  scope_id = g_quark_from_static_string ("gimp_config_deserialize_properties");
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];

      if (prop_spec->flags & G_PARAM_READWRITE)
        {
          g_scanner_scope_add_symbol (scanner, scope_id, 
                                      prop_spec->name, prop_spec);
        }
    }

  token = G_TOKEN_LEFT_PAREN;
  
  do
    {
      if (g_scanner_peek_next_token (scanner) != token)
        break;

      token = g_scanner_get_next_token (scanner);
      
      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;
          
        case G_TOKEN_SYMBOL:
          gimp_config_deserialize_property (object, scanner, &token);
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }
  while (token != G_TOKEN_EOF);

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             "parse error", TRUE);
    }

  if (property_specs)
    g_free (property_specs);

  g_scanner_set_scope (scanner, old_scope_id);

  return (token == G_TOKEN_EOF);
}

static void
gimp_config_deserialize_property (GObject    *object,
                                  GScanner   *scanner,
                                  GTokenType *token)
{
  GParamSpec *prop_spec;
  gchar      *orig_cset_first = NULL;
  gchar      *orig_cset_nth   = NULL;
  GValue      value           = { 0, };

  prop_spec = G_PARAM_SPEC (scanner->value.v_symbol);  

  if (G_TYPE_FUNDAMENTAL (prop_spec->value_type) == G_TYPE_ENUM)
    {
      *token = G_TOKEN_IDENTIFIER;
    }
  else if (G_TYPE_IS_FUNDAMENTAL (prop_spec->value_type))
    {
      switch (G_TYPE_FUNDAMENTAL (prop_spec->value_type))
        {
        case G_TYPE_STRING:
          *token = G_TOKEN_STRING;
          break;
          
        case G_TYPE_BOOLEAN:
           *token = G_TOKEN_IDENTIFIER;
          break;
        
        case G_TYPE_INT:
        case G_TYPE_UINT:
        case G_TYPE_LONG:
        case G_TYPE_ULONG:
          *token = G_TOKEN_INT;
          break;
          
        case G_TYPE_FLOAT:
        case G_TYPE_DOUBLE:
          *token = G_TOKEN_FLOAT;
          break;
          
        default:
          g_assert_not_reached ();
        }
    }
  else  /* not an enum and not a fundamental type */
    {
      if (g_value_type_transformable (G_TYPE_STRING, prop_spec->value_type))
        {
          *token = G_TOKEN_IDENTIFIER;
        }
      else
        {
          g_warning ("%s: %s can not be transformed from a string",
                     G_STRLOC, g_type_name (prop_spec->value_type));
          g_assert_not_reached ();
        }

      if (prop_spec->value_type == GIMP_TYPE_MEMSIZE)
        {
          orig_cset_first = scanner->config->cset_identifier_first;
          orig_cset_nth   = scanner->config->cset_identifier_nth;

          scanner->config->cset_identifier_first = G_CSET_DIGITS;
          scanner->config->cset_identifier_nth   = G_CSET_DIGITS "gGmMkKbB";
        }
    }

  if (g_scanner_peek_next_token (scanner) != *token)
    return;

  g_scanner_get_next_token (scanner);

  g_value_init (&value, prop_spec->value_type);

  if (G_TYPE_FUNDAMENTAL (prop_spec->value_type) == G_TYPE_ENUM)
    {
      GEnumClass *enum_class;
      GEnumValue *enum_value;
      
      enum_class = g_type_class_peek (G_VALUE_TYPE (&value));
      enum_value = g_enum_get_value_by_nick (G_ENUM_CLASS (enum_class), 
                                             scanner->value.v_identifier);
      if (!enum_value)
        enum_value = g_enum_get_value_by_name (G_ENUM_CLASS (enum_class), 
                                               scanner->value.v_identifier);
      
      if (enum_value)
        g_value_set_enum (&value, enum_value->value);
      else
        g_scanner_warn (scanner, 
                        "invalid value '%s' for enum property %s", 
                        scanner->value.v_identifier, prop_spec->name);
    }
  else if (G_TYPE_IS_FUNDAMENTAL (prop_spec->value_type))
    {
      switch (G_TYPE_FUNDAMENTAL (prop_spec->value_type))
        {
        case G_TYPE_STRING:
          g_value_set_string (&value, scanner->value.v_string);
          break;
          
        case G_TYPE_BOOLEAN:
          if (! g_ascii_strcasecmp (scanner->value.v_identifier, "yes") ||
              ! g_ascii_strcasecmp (scanner->value.v_identifier, "true"))
            g_value_set_boolean (&value, TRUE);
          else if (! g_ascii_strcasecmp (scanner->value.v_identifier, "no") ||
                   ! g_ascii_strcasecmp (scanner->value.v_identifier, "false"))
            g_value_set_boolean (&value, FALSE);
          else
            g_scanner_warn 
              (scanner, 
               "expected 'yes' or 'no' for boolean property %s, got '%s'", 
               prop_spec->name, scanner->value.v_identifier);
          break;
          
        case G_TYPE_INT:
          g_value_set_int (&value, scanner->value.v_int);
          break;
        case G_TYPE_UINT:
          g_value_set_uint (&value, scanner->value.v_int);
          break;
        case G_TYPE_LONG:
          g_value_set_int (&value, scanner->value.v_int);
          break;
        case G_TYPE_ULONG:
          g_value_set_uint (&value, scanner->value.v_int);
          break;      
        case G_TYPE_FLOAT:
          g_value_set_float (&value, scanner->value.v_float);
          break;
        case G_TYPE_DOUBLE:
          g_value_set_double (&value, scanner->value.v_float);
          break;

        default:
          g_assert_not_reached ();          
        }
    }
  else  /* not an enum and not a fundamental type */
    {
      GValue src = { 0, };
      
      g_value_init (&src, G_TYPE_STRING);
      g_value_set_string (&src, scanner->value.v_identifier);
      
      g_value_transform (&src, &value);
    }
    
  g_object_set_property (object, prop_spec->name, &value);
  g_value_unset (&value);
  
  if (orig_cset_first)
    scanner->config->cset_identifier_first = orig_cset_first;
  if (orig_cset_nth)
    scanner->config->cset_identifier_nth = orig_cset_nth;

  *token = G_TOKEN_RIGHT_PAREN;        
}
