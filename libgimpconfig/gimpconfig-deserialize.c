/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Object properties deserialization routines
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gimpconfigtypes.h"

#include "gimpconfigwriter.h"
#include "gimpconfig-iface.h"
#include "gimpconfig-deserialize.h"
#include "gimpconfig-params.h"
#include "gimpconfig-path.h"
#include "gimpscanner.h"

#include "libgimp/libgimp-intl.h"


/*
 *  All functions return G_TOKEN_RIGHT_PAREN on success,
 *  the GTokenType they would have expected but didn't get
 *  or G_TOKEN_NONE if they got the expected token but
 *  couldn't parse it.
 */

static GTokenType  gimp_config_deserialize_value       (GValue     *value,
                                                        GimpConfig *config,
                                                        GParamSpec *prop_spec,
                                                        GScanner   *scanner);
static GTokenType  gimp_config_deserialize_fundamental (GValue     *value,
                                                        GParamSpec *prop_spec,
                                                        GScanner   *scanner);
static GTokenType  gimp_config_deserialize_enum        (GValue     *value,
                                                        GParamSpec *prop_spec,
                                                        GScanner   *scanner);
static GTokenType  gimp_config_deserialize_memsize     (GValue     *value,
                                                        GParamSpec *prop_spec,
                                                        GScanner   *scanner);
static GTokenType  gimp_config_deserialize_path        (GValue     *value,
                                                        GParamSpec *prop_spec,
                                                        GScanner   *scanner);
static GTokenType  gimp_config_deserialize_rgb         (GValue     *value,
                                                        GParamSpec *prop_spec,
                                                        GScanner   *scanner);
static GTokenType  gimp_config_deserialize_matrix2     (GValue     *value,
                                                        GParamSpec *prop_spec,
                                                        GScanner   *scanner);
static GTokenType  gimp_config_deserialize_object      (GValue     *value,
                                                        GimpConfig *config,
                                                        GParamSpec *prop_spec,
                                                        GScanner   *scanner,
                                                        gint        nest_level);
static GTokenType  gimp_config_deserialize_value_array (GValue     *value,
                                                        GimpConfig *config,
                                                        GParamSpec *prop_spec,
                                                        GScanner   *scanner);
static GTokenType  gimp_config_deserialize_any         (GValue     *value,
                                                        GParamSpec *prop_spec,
                                                        GScanner   *scanner);
static GTokenType  gimp_config_skip_unknown_property   (GScanner   *scanner);

static inline gboolean  scanner_string_utf8_valid (GScanner    *scanner,
                                                   const gchar *token_name);

static inline gboolean
scanner_string_utf8_valid (GScanner    *scanner,
                           const gchar *token_name)
{
  if (g_utf8_validate (scanner->value.v_string, -1, NULL))
    return TRUE;

  g_scanner_error (scanner,
                   _("value for token %s is not a valid UTF-8 string"),
                   token_name);

  return FALSE;
}

/**
 * gimp_config_deserialize_properties:
 * @config: a #GimpConfig.
 * @scanner: a #GScanner.
 * @nest_level:
 *
 * This function uses the @scanner to configure the properties of @config.
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 *
 * Since: GIMP 2.4
 **/
gboolean
gimp_config_deserialize_properties (GimpConfig *config,
                                    GScanner   *scanner,
                                    gint        nest_level)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;
  guint          scope_id;
  guint          old_scope_id;
  GTokenType     token;
  GTokenType     next;

  g_return_val_if_fail (GIMP_IS_CONFIG (config), FALSE);

  klass = G_OBJECT_GET_CLASS (config);
  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (! property_specs)
    return TRUE;

  scope_id = g_type_qname (G_TYPE_FROM_INSTANCE (config));
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];

      if (prop_spec->flags & GIMP_CONFIG_PARAM_SERIALIZE)
        {
          g_scanner_scope_add_symbol (scanner, scope_id,
                                      prop_spec->name, prop_spec);
        }
    }

  g_free (property_specs);

  g_object_freeze_notify (G_OBJECT (config));

  token = G_TOKEN_LEFT_PAREN;

  while (TRUE)
    {
      next = g_scanner_peek_next_token (scanner);

      if (G_UNLIKELY (next != token &&
                      ! (token == G_TOKEN_SYMBOL &&
                         next  == G_TOKEN_IDENTIFIER)))
        {
          break;
        }

      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_IDENTIFIER:
          token = gimp_config_skip_unknown_property (scanner);
          break;

        case G_TOKEN_SYMBOL:
          token = gimp_config_deserialize_property (config,
                                                    scanner, nest_level);
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

  g_scanner_set_scope (scanner, old_scope_id);

  g_object_thaw_notify (G_OBJECT (config));

  if (token == G_TOKEN_NONE)
    return FALSE;

  return gimp_config_deserialize_return (scanner, token, nest_level);
}

/**
 * gimp_config_deserialize_property:
 * @config: a #GimpConfig.
 * @scanner: a #GScanner.
 * @nest_level:
 *
 * This function deserializes a single property of @config. You
 * shouldn't need to call this function directly. If possible, use
 * gimp_config_deserialize_properties() instead.
 *
 * Return value: %G_TOKEN_RIGHT_PAREN on success, otherwise the
 * expected #GTokenType or %G_TOKEN_NONE if the expected token was
 * found but couldn't be parsed.
 *
 * Since: GIMP 2.4
 **/
GTokenType
gimp_config_deserialize_property (GimpConfig *config,
                                  GScanner   *scanner,
                                  gint        nest_level)
{
  GimpConfigInterface *config_iface = NULL;
  GimpConfigInterface *parent_iface = NULL;
  GParamSpec          *prop_spec;
  GTokenType           token = G_TOKEN_RIGHT_PAREN;
  GValue               value = { 0, };
  guint                old_scope_id;

  old_scope_id = g_scanner_set_scope (scanner, 0);

  prop_spec = G_PARAM_SPEC (scanner->value.v_symbol);

  g_value_init (&value, prop_spec->value_type);

  if (G_TYPE_IS_OBJECT (prop_spec->owner_type))
    {
      GTypeClass *owner_class = g_type_class_peek (prop_spec->owner_type);

      config_iface = g_type_interface_peek (owner_class, GIMP_TYPE_CONFIG);

      /*  We must call deserialize_property() *only* if the *exact* class
       *  which implements it is param_spec->owner_type's class.
       *
       *  Therefore, we ask param_spec->owner_type's immediate parent class
       *  for it's GimpConfigInterface and check if we get a different
       *  pointer.
       *
       *  (if the pointers are the same, param_spec->owner_type's
       *   GimpConfigInterface is inherited from one of it's parent classes
       *   and thus not able to handle param_spec->owner_type's properties).
       */
      if (config_iface)
        {
          GTypeClass *owner_parent_class;

          owner_parent_class = g_type_class_peek_parent (owner_class);

          parent_iface = g_type_interface_peek (owner_parent_class,
                                                GIMP_TYPE_CONFIG);
        }
    }

  if (config_iface                       &&
      config_iface != parent_iface       && /* see comment above */
      config_iface->deserialize_property &&
      config_iface->deserialize_property (config,
                                          prop_spec->param_id,
                                          &value,
                                          prop_spec,
                                          scanner,
                                          &token))
    {
      /* nop */
    }
  else
    {
      if (G_VALUE_HOLDS_OBJECT (&value))
        token = gimp_config_deserialize_object (&value,
                                                config, prop_spec,
                                                scanner, nest_level);
      else
        token = gimp_config_deserialize_value (&value,
                                               config, prop_spec, scanner);
    }

  if (token == G_TOKEN_RIGHT_PAREN &&
      g_scanner_peek_next_token (scanner) == token)
    {
      if (! (G_VALUE_HOLDS_OBJECT (&value) &&
             (prop_spec->flags & GIMP_CONFIG_PARAM_AGGREGATE)))
        g_object_set_property (G_OBJECT (config), prop_spec->name, &value);
    }
#ifdef CONFIG_DEBUG
  else
    {
      g_warning ("%s: couldn't deserialize property %s::%s of type %s",
                 G_STRFUNC,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 prop_spec->name,
                 g_type_name (prop_spec->value_type));
    }
#endif

  g_value_unset (&value);

  g_scanner_set_scope (scanner, old_scope_id);

  return token;
}

static GTokenType
gimp_config_deserialize_value (GValue     *value,
                               GimpConfig *config,
                               GParamSpec *prop_spec,
                               GScanner   *scanner)
{
  if (G_TYPE_FUNDAMENTAL (prop_spec->value_type) == G_TYPE_ENUM)
    {
      return gimp_config_deserialize_enum (value, prop_spec, scanner);
    }
  else if (G_TYPE_IS_FUNDAMENTAL (prop_spec->value_type))
    {
      return gimp_config_deserialize_fundamental (value, prop_spec, scanner);
    }
  else if (prop_spec->value_type == GIMP_TYPE_MEMSIZE)
    {
      return gimp_config_deserialize_memsize (value, prop_spec, scanner);
    }
  else if (prop_spec->value_type == GIMP_TYPE_CONFIG_PATH)
    {
      return  gimp_config_deserialize_path (value, prop_spec, scanner);
    }
  else if (prop_spec->value_type == GIMP_TYPE_RGB)
    {
      return gimp_config_deserialize_rgb (value, prop_spec, scanner);
    }
  else if (prop_spec->value_type == GIMP_TYPE_MATRIX2)
    {
      return gimp_config_deserialize_matrix2 (value, prop_spec, scanner);
    }
  else if (prop_spec->value_type == G_TYPE_VALUE_ARRAY)
    {
      return gimp_config_deserialize_value_array (value,
                                                  config, prop_spec, scanner);
    }

  /*  This fallback will only work for value_types that
   *  can be transformed from a string value.
   */
  return gimp_config_deserialize_any (value, prop_spec, scanner);
}

static GTokenType
gimp_config_deserialize_fundamental (GValue     *value,
                                     GParamSpec *prop_spec,
                                     GScanner   *scanner)
{
  GTokenType token;
  GType      value_type;
  gboolean   negate = FALSE;

  value_type = G_TYPE_FUNDAMENTAL (prop_spec->value_type);

  switch (value_type)
    {
    case G_TYPE_STRING:
      token = G_TOKEN_STRING;
      break;

    case G_TYPE_BOOLEAN:
      token = G_TOKEN_IDENTIFIER;
      break;

    case G_TYPE_INT:
    case G_TYPE_LONG:
    case G_TYPE_INT64:
      if (g_scanner_peek_next_token (scanner) == '-')
        {
          negate = TRUE;
          g_scanner_get_next_token (scanner);
        }
      /*  fallthrough  */
    case G_TYPE_UINT:
    case G_TYPE_ULONG:
    case G_TYPE_UINT64:
      token = G_TOKEN_INT;
      break;

    case G_TYPE_FLOAT:
    case G_TYPE_DOUBLE:
      if (g_scanner_peek_next_token (scanner) == '-')
        {
          negate = TRUE;
          g_scanner_get_next_token (scanner);
        }
      token = G_TOKEN_FLOAT;
      break;

    default:
      token = G_TOKEN_NONE;
      g_assert_not_reached ();
      break;
    }

  if (g_scanner_peek_next_token (scanner) != token)
    {
      return token;
    }

  g_scanner_get_next_token (scanner);

  switch (value_type)
    {
    case G_TYPE_STRING:
      if (scanner_string_utf8_valid (scanner, prop_spec->name))
        g_value_set_static_string (value, scanner->value.v_string);
      else
        return G_TOKEN_NONE;
      break;

    case G_TYPE_BOOLEAN:
      if (! g_ascii_strcasecmp (scanner->value.v_identifier, "yes") ||
          ! g_ascii_strcasecmp (scanner->value.v_identifier, "true"))
        g_value_set_boolean (value, TRUE);
      else if (! g_ascii_strcasecmp (scanner->value.v_identifier, "no") ||
               ! g_ascii_strcasecmp (scanner->value.v_identifier, "false"))
        g_value_set_boolean (value, FALSE);
      else
        {
          g_scanner_error
            (scanner,
             /* please don't translate 'yes' and 'no' */
             _("expected 'yes' or 'no' for boolean token %s, got '%s'"),
             prop_spec->name, scanner->value.v_identifier);
          return G_TOKEN_NONE;
        }
      break;

    case G_TYPE_INT:
      g_value_set_int (value, (negate ?
                               - scanner->value.v_int64 :
                               scanner->value.v_int64));
      break;
    case G_TYPE_UINT:
      g_value_set_uint (value, scanner->value.v_int64);
      break;

    case G_TYPE_LONG:
      g_value_set_long (value, (negate ?
                                - scanner->value.v_int64 :
                                scanner->value.v_int64));
      break;
    case G_TYPE_ULONG:
      g_value_set_ulong (value, scanner->value.v_int64);
      break;

    case G_TYPE_INT64:
      g_value_set_int64 (value, (negate ?
                                 - scanner->value.v_int64 :
                                 scanner->value.v_int64));
      break;
    case G_TYPE_UINT64:
      g_value_set_uint64 (value, scanner->value.v_int64);
      break;

    case G_TYPE_FLOAT:
      g_value_set_float (value, negate ?
                         - scanner->value.v_float : scanner->value.v_float);
      break;
    case G_TYPE_DOUBLE:
      g_value_set_double (value, negate ?
                          - scanner->value.v_float: scanner->value.v_float);
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_deserialize_enum (GValue     *value,
                              GParamSpec *prop_spec,
                              GScanner   *scanner)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_peek (G_VALUE_TYPE (value));

  switch (g_scanner_peek_next_token (scanner))
    {
    case G_TOKEN_IDENTIFIER:
      g_scanner_get_next_token (scanner);

      enum_value = g_enum_get_value_by_nick (enum_class,
                                             scanner->value.v_identifier);
      if (!enum_value)
        enum_value = g_enum_get_value_by_name (enum_class,
                                               scanner->value.v_identifier);

      if (!enum_value)
        {
          g_scanner_error (scanner,
                           _("invalid value '%s' for token %s"),
                           scanner->value.v_identifier, prop_spec->name);
          return G_TOKEN_NONE;
        }
      break;

    case G_TOKEN_INT:
      g_scanner_get_next_token (scanner);

      enum_value = g_enum_get_value (enum_class,
                                     (gint) scanner->value.v_int64);

      if (!enum_value)
        {
          g_scanner_error (scanner,
                           _("invalid value '%ld' for token %s"),
                           (glong) scanner->value.v_int64, prop_spec->name);
          return G_TOKEN_NONE;
        }
      break;

    default:
      return G_TOKEN_IDENTIFIER;
    }

  g_value_set_enum (value, enum_value->value);

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_deserialize_memsize (GValue     *value,
                                 GParamSpec *prop_spec,
                                 GScanner   *scanner)
{
  gchar   *orig_cset_first = scanner->config->cset_identifier_first;
  gchar   *orig_cset_nth   = scanner->config->cset_identifier_nth;
  guint64  memsize;

  scanner->config->cset_identifier_first = G_CSET_DIGITS;
  scanner->config->cset_identifier_nth   = G_CSET_DIGITS "gGmMkKbB";

  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return G_TOKEN_IDENTIFIER;

  g_scanner_get_next_token (scanner);

  scanner->config->cset_identifier_first = orig_cset_first;
  scanner->config->cset_identifier_nth   = orig_cset_nth;

  if (! gimp_memsize_deserialize (scanner->value.v_identifier, &memsize))
    return G_TOKEN_NONE;

  g_value_set_uint64 (value, memsize);

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_deserialize_path (GValue     *value,
                              GParamSpec *prop_spec,
                              GScanner   *scanner)
{
  GError *error = NULL;

  if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
    return G_TOKEN_STRING;

  g_scanner_get_next_token (scanner);

  if (!scanner_string_utf8_valid (scanner, prop_spec->name))
    return G_TOKEN_NONE;

  if (scanner->value.v_string)
    {
      /*  Check if the string can be expanded
       *  and converted to the filesystem encoding.
       */
      gchar *expand = gimp_config_path_expand (scanner->value.v_string,
                                               TRUE, &error);

      if (!expand)
        {
          g_scanner_error (scanner,
                           _("while parsing token '%s': %s"),
                           prop_spec->name, error->message);
          g_error_free (error);

          return G_TOKEN_NONE;
        }

      g_free (expand);

      g_value_set_static_string (value, scanner->value.v_string);
    }

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_deserialize_rgb (GValue     *value,
                             GParamSpec *prop_spec,
                             GScanner   *scanner)
{
  GimpRGB rgb;

  if (! gimp_scanner_parse_color (scanner, &rgb))
    return G_TOKEN_NONE;

  g_value_set_boxed (value, &rgb);

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_deserialize_matrix2 (GValue     *value,
                                 GParamSpec *prop_spec,
                                 GScanner   *scanner)
{
  GimpMatrix2 matrix;

  if (! gimp_scanner_parse_matrix2 (scanner, &matrix))
    return G_TOKEN_NONE;

  g_value_set_boxed (value, &matrix);

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_deserialize_object (GValue     *value,
                                GimpConfig *config,
                                GParamSpec *prop_spec,
                                GScanner   *scanner,
                                gint        nest_level)
{
  GimpConfigInterface *config_iface;
  GimpConfig          *prop_object;

  g_object_get_property (G_OBJECT (config), prop_spec->name, value);

  prop_object = g_value_get_object (value);

  if (! prop_object)
    {
      /*  if the object property is not GIMP_CONFIG_PARAM_AGGREGATE, read
       *  the type of the object and create it
       */
      if (! (prop_spec->flags & GIMP_CONFIG_PARAM_AGGREGATE))
        {
          gchar *type_name;
          GType  type;

          if (! gimp_scanner_parse_string (scanner, &type_name))
            return G_TOKEN_STRING;

          type = g_type_from_name (type_name);
          g_free (type_name);

          if (! g_type_is_a (type, prop_spec->value_type))
            return G_TOKEN_STRING;

          prop_object = g_object_new (type, NULL);

          g_value_take_object (value, prop_object);
        }
      else
        {
          return G_TOKEN_RIGHT_PAREN;
        }
    }

  config_iface = GIMP_CONFIG_GET_INTERFACE (prop_object);

  if (! config_iface)
    return gimp_config_deserialize_any (value, prop_spec, scanner);

  if (! config_iface->deserialize (prop_object, scanner, nest_level + 1, NULL))
    return G_TOKEN_NONE;

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_deserialize_value_array (GValue     *value,
                                     GimpConfig *config,
                                     GParamSpec *prop_spec,
                                     GScanner   *scanner)
{
  GParamSpecValueArray *array_spec;
  GValueArray          *array;
  GValue                array_value = { 0, };
  gint                  n_values;
  GTokenType            token;
  gint                  i;

  array_spec = G_PARAM_SPEC_VALUE_ARRAY (prop_spec);

  if (! gimp_scanner_parse_int (scanner, &n_values))
    return G_TOKEN_INT;

  array = g_value_array_new (n_values);

  for (i = 0; i < n_values; i++)
    {
      g_value_init (&array_value, array_spec->element_spec->value_type);

      token = gimp_config_deserialize_value (&array_value,
                                             config,
                                             array_spec->element_spec,
                                             scanner);

      if (token == G_TOKEN_RIGHT_PAREN)
        g_value_array_append (array, &array_value);

      g_value_unset (&array_value);

      if (token != G_TOKEN_RIGHT_PAREN)
        return token;
    }

  g_value_take_boxed (value, array);

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_deserialize_any (GValue     *value,
                             GParamSpec *prop_spec,
                             GScanner   *scanner)
{
  GValue src = { 0, };

  if (!g_value_type_transformable (G_TYPE_STRING, prop_spec->value_type))
    {
      g_warning ("%s: %s can not be transformed from a string",
                 G_STRFUNC, g_type_name (prop_spec->value_type));
      return G_TOKEN_NONE;
    }

  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return G_TOKEN_IDENTIFIER;

  g_scanner_get_next_token (scanner);

  g_value_init (&src, G_TYPE_STRING);
  g_value_set_static_string (&src, scanner->value.v_identifier);
  g_value_transform (&src, value);
  g_value_unset (&src);

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_skip_unknown_property (GScanner *scanner)
{
  gint open_paren = 0;

  while (TRUE)
    {
      GTokenType token = g_scanner_peek_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          open_paren++;
          g_scanner_get_next_token (scanner);
          break;

        case G_TOKEN_RIGHT_PAREN:
          if (open_paren == 0)
            return token;

          open_paren--;
          g_scanner_get_next_token (scanner);
          break;

        case G_TOKEN_EOF:
          return token;

        default:
          g_scanner_get_next_token (scanner);
          break;
        }
    }
}
