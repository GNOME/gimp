/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Object properties deserialization routines
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gimpconfigtypes.h"

#include "gimpconfigwriter.h"
#include "gimpconfig-array.h"
#include "gimpconfig-iface.h"
#include "gimpconfig-deserialize.h"
#include "gimpconfig-params.h"
#include "gimpconfig-path.h"
#include "gimpscanner.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpconfig-deserialize
 * @title: GimpConfig-deserialize
 * @short_description: Deserializing code for libgimpconfig.
 *
 * Deserializing code for libgimpconfig.
 **/


/*
 *  All functions return G_TOKEN_RIGHT_PAREN on success,
 *  the GTokenType they would have expected but didn't get
 *  or G_TOKEN_NONE if they got the expected token but
 *  couldn't parse it.
 */

static GTokenType  gimp_config_deserialize_value          (GValue     *value,
                                                           GimpConfig *config,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_deserialize_fundamental    (GValue     *value,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_deserialize_enum           (GValue     *value,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_deserialize_memsize        (GValue     *value,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_deserialize_path           (GValue     *value,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_deserialize_rgb            (GValue     *value,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_deserialize_matrix2        (GValue     *value,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_deserialize_object         (GValue     *value,
                                                           GimpConfig *config,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner,
                                                           gint        nest_level);
static GTokenType  gimp_config_deserialize_value_array    (GValue     *value,
                                                           GimpConfig *config,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GimpUnit  * gimp_config_get_unit_from_identifier   (const gchar *identifier);
static GTokenType  gimp_config_deserialize_unit           (GValue     *value,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_deserialize_file_value     (GValue     *value,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_deserialize_parasite_value (GValue     *value,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_deserialize_bytes          (GValue     *value,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_deserialize_color          (GValue     *value,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_deserialize_any            (GValue     *value,
                                                           GParamSpec *prop_spec,
                                                           GScanner   *scanner);
static GTokenType  gimp_config_skip_unknown_property      (GScanner   *scanner);

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
 * @nest_level: the nest level
 *
 * This function uses the @scanner to configure the properties of @config.
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 *
 * Since: 2.4
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
      GTokenType next = g_scanner_peek_next_token (scanner);

      if (next == G_TOKEN_EOF)
        break;

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
 * @nest_level: the nest level
 *
 * This function deserializes a single property of @config. You
 * shouldn't need to call this function directly. If possible, use
 * gimp_config_deserialize_properties() instead.
 *
 * Returns: %G_TOKEN_RIGHT_PAREN on success, otherwise the
 * expected #GTokenType or %G_TOKEN_NONE if the expected token was
 * found but couldn't be parsed.
 *
 * Since: 2.4
 **/
GTokenType
gimp_config_deserialize_property (GimpConfig *config,
                                  GScanner   *scanner,
                                  gint        nest_level)
{
  GimpConfigInterface *config_iface = NULL;
  GimpConfigInterface *parent_iface = NULL;
  GParamSpec          *prop_spec;
  GTokenType           token        = G_TOKEN_RIGHT_PAREN;
  GValue               value        = G_VALUE_INIT;
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
      if (G_VALUE_HOLDS_OBJECT (&value)            &&
          G_VALUE_TYPE (&value) != G_TYPE_FILE     &&
          G_VALUE_TYPE (&value) != GEGL_TYPE_COLOR &&
          G_VALUE_TYPE (&value) != GIMP_TYPE_UNIT)
        {
          token = gimp_config_deserialize_object (&value,
                                                  config, prop_spec,
                                                  scanner, nest_level);
        }
      else
        {
          token = gimp_config_deserialize_value (&value,
                                                 config, prop_spec, scanner);
        }
    }

  if (token == G_TOKEN_RIGHT_PAREN &&
      g_scanner_peek_next_token (scanner) == token)
    {
      if (! (prop_spec->flags & GIMP_PARAM_DONT_SERIALIZE) &&
          (GIMP_VALUE_HOLDS_COLOR (&value) ||
           ! (G_VALUE_HOLDS_OBJECT (&value)     &&
              (prop_spec->flags & GIMP_CONFIG_PARAM_AGGREGATE))))
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
  else if (prop_spec->value_type == GIMP_TYPE_VALUE_ARRAY)
    {
      return gimp_config_deserialize_value_array (value,
                                                  config, prop_spec, scanner);
    }
  else if (prop_spec->value_type == G_TYPE_STRV)
    {
      return gimp_config_deserialize_strv (value, scanner);
    }
  else if (prop_spec->value_type == GIMP_TYPE_UNIT)
    {
      return gimp_config_deserialize_unit (value, prop_spec, scanner);
    }
  else if (prop_spec->value_type == G_TYPE_FILE)
    {
      return gimp_config_deserialize_file_value (value, prop_spec, scanner);
    }
  else if (prop_spec->value_type == GIMP_TYPE_PARASITE)
    {
      return gimp_config_deserialize_parasite_value (value, prop_spec, scanner);
    }
  else if (prop_spec->value_type == G_TYPE_BYTES)
    {
      return gimp_config_deserialize_bytes (value, prop_spec, scanner);
    }
  else if (prop_spec->value_type == GEGL_TYPE_COLOR)
    {
      return gimp_config_deserialize_color (value, prop_spec, scanner);
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
  GTokenType next_token;
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
      g_assert_not_reached ();
      break;
    }

  next_token = g_scanner_peek_next_token (scanner);

  /* we parse integers into floats too, because g_ascii_dtostr()
   * serialized whole number without decimal point
   */
  if (next_token != token &&
      ! (token == G_TOKEN_FLOAT && next_token == G_TOKEN_INT))
    {
      return token;
    }

  g_scanner_get_next_token (scanner);

  switch (value_type)
    {
    case G_TYPE_STRING:
      if (scanner_string_utf8_valid (scanner, prop_spec->name))
        g_value_set_string (value, scanner->value.v_string);
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
                               - (gint) scanner->value.v_int64 :
                                 (gint) scanner->value.v_int64));
      break;
    case G_TYPE_UINT:
      g_value_set_uint (value, scanner->value.v_int64);
      break;

    case G_TYPE_LONG:
      g_value_set_long (value, (negate ?
                                - (glong) scanner->value.v_int64 :
                                  (glong) scanner->value.v_int64));
      break;
    case G_TYPE_ULONG:
      g_value_set_ulong (value, scanner->value.v_int64);
      break;

    case G_TYPE_INT64:
      g_value_set_int64 (value, (negate ?
                                 - (gint64) scanner->value.v_int64 :
                                   (gint64) scanner->value.v_int64));
      break;
    case G_TYPE_UINT64:
      g_value_set_uint64 (value, scanner->value.v_int64);
      break;

    case G_TYPE_FLOAT:
      if (next_token == G_TOKEN_FLOAT)
        g_value_set_float (value, negate ?
                           - scanner->value.v_float :
                             scanner->value.v_float);
      else
        g_value_set_float (value, negate ?
                           - (gfloat) scanner->value.v_int :
                             (gfloat) scanner->value.v_int);
      break;

    case G_TYPE_DOUBLE:
      if (next_token == G_TOKEN_FLOAT)
        g_value_set_double (value, negate ?
                            - scanner->value.v_float:
                              scanner->value.v_float);
      else
        g_value_set_double (value, negate ?
                            - (gdouble) scanner->value.v_int:
                              (gdouble) scanner->value.v_int);
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
      if (! enum_value)
        enum_value = g_enum_get_value_by_name (enum_class,
                                               scanner->value.v_identifier);
      if (! enum_value)
        {
          /*  if the value was not found, check if we have a compat
           *  enum to find the ideitifier
           */
          GQuark quark       = g_quark_from_static_string ("gimp-compat-enum");
          GType  compat_type = (GType) g_type_get_qdata (G_VALUE_TYPE (value),
                                                         quark);

          if (compat_type)
            {
              GEnumClass *compat_class = g_type_class_ref (compat_type);

              enum_value = g_enum_get_value_by_nick (compat_class,
                                                     scanner->value.v_identifier);
              if (! enum_value)
                enum_value = g_enum_get_value_by_name (compat_class,
                                                       scanner->value.v_identifier);

              /*  finally, if we found a compat value, make sure the
               *  same value exists in the original enum
               */
              if (enum_value)
                enum_value = g_enum_get_value (enum_class, enum_value->value);

              g_type_class_unref (compat_class);
           }
        }

      if (! enum_value)
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

      if (! enum_value)
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
  GeglColor *color = NULL;
  GimpRGB    rgb;

  if (! gimp_scanner_parse_color (scanner, &color))
    return G_TOKEN_NONE;

  gegl_color_get_pixel (color, babl_format ("R'G'B'A double"), &rgb);
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
  GType                type;

  g_object_get_property (G_OBJECT (config), prop_spec->name, value);

  prop_object = g_value_get_object (value);

  /*  if the object property is not GIMP_CONFIG_PARAM_AGGREGATE, read
   *  the type of the object.
   */
  if (! (prop_spec->flags & GIMP_CONFIG_PARAM_AGGREGATE))
    {
      gchar *type_name;

      if (! gimp_scanner_parse_string (scanner, &type_name))
        return G_TOKEN_STRING;

      if (! (type_name && *type_name))
        {
          g_scanner_error (scanner, "Type name is empty");
          g_free (type_name);
          return G_TOKEN_NONE;
        }

      type = g_type_from_name (type_name);
      g_free (type_name);

      if (! g_type_is_a (type, prop_spec->value_type))
        return G_TOKEN_STRING;
    }

  if (! prop_object)
    {
      /*  if the object property is not GIMP_CONFIG_PARAM_AGGREGATE,
       *  create the object.
       */
      if (! (prop_spec->flags & GIMP_CONFIG_PARAM_AGGREGATE))
        {
          prop_object = g_object_new (type, NULL);

          g_value_take_object (value, prop_object);
        }
      else
        {
          return G_TOKEN_RIGHT_PAREN;
        }
    }

  config_iface = GIMP_CONFIG_GET_IFACE (prop_object);

  if (! config_iface)
    return gimp_config_deserialize_any (value, prop_spec, scanner);

  if (config_iface->deserialize_create != NULL)
    {
      /* Some class may prefer to create themselves their objects, for instance
       * to maintain unicity of objects (in libgimp in particular, the various
       * GimpItem or GimpResource are managed by the lib. A single item or
       * resource must be represented for a single object across the whole
       * processus.
       */
      GimpConfig *created_object;

      created_object = config_iface->deserialize_create (G_TYPE_FROM_INSTANCE (prop_object),
                                                         scanner, nest_level + 1, NULL);

      if (created_object == NULL)
        return G_TOKEN_NONE;
      else
        g_value_take_object (value, created_object);
    }
  else if (! config_iface->deserialize (prop_object, scanner, nest_level + 1, NULL))
    {
      return G_TOKEN_NONE;
    }

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_deserialize_value_array (GValue     *value,
                                     GimpConfig *config,
                                     GParamSpec *prop_spec,
                                     GScanner   *scanner)
{
  GimpParamSpecValueArray *array_spec;
  GimpValueArray          *array;
  GValue                   array_value = G_VALUE_INIT;
  gint                     n_values;
  GTokenType               token;
  gint                     i;

  array_spec = GIMP_PARAM_SPEC_VALUE_ARRAY (prop_spec);

  if (! gimp_scanner_parse_int (scanner, &n_values))
    return G_TOKEN_INT;

  array = gimp_value_array_new (n_values);

  for (i = 0; i < n_values; i++)
    {
      g_value_init (&array_value, array_spec->element_spec->value_type);

      token = gimp_config_deserialize_value (&array_value,
                                             config,
                                             array_spec->element_spec,
                                             scanner);

      if (token == G_TOKEN_RIGHT_PAREN)
        gimp_value_array_append (array, &array_value);

      g_value_unset (&array_value);

      if (token != G_TOKEN_RIGHT_PAREN)
        {
          gimp_value_array_unref (array);
          return token;
        }
    }

  g_value_take_boxed (value, array);

  return G_TOKEN_RIGHT_PAREN;
}

static GimpUnit *
gimp_config_get_unit_from_identifier (const gchar *identifier)
{
  GimpUnit *unit;

  unit = gimp_unit_get_by_id (GIMP_UNIT_PIXEL);
  for (gint i = GIMP_UNIT_PIXEL; unit; i++)
    {
      if (g_strcmp0 (identifier, gimp_unit_get_name (unit)) == 0)
        break;

      unit = gimp_unit_get_by_id (i);
    }

  if (unit == NULL && g_strcmp0 (identifier, "percent") == 0)
    unit = gimp_unit_percent ();

  /* XXX This may return NULL, especially for user-defined units which
   * may have disappeared from one session to another. Should we return
   * some default unit then?
   */

  return unit;
}

/* This function is entirely sick, so is our method of serializing
 * units, which we write out as (unit foo bar) instead of
 * (unit "foo bar"). The assumption that caused this shit was that a
 * unit's "identifier" is really an identifier in the C-ish sense,
 * when in fact it's just a random user entered string.
 *
 * Here, we try to parse at least the default units shipped with gimp,
 * and we add code to parse (unit "foo bar") in order to be compatible
 * with future correct unit serializing.
 */
static GTokenType
gimp_config_deserialize_unit (GValue     *value,
                              GParamSpec *prop_spec,
                              GScanner   *scanner)
{
  gchar      *old_cset_skip_characters;
  gchar      *old_cset_identifier_first;
  gchar      *old_cset_identifier_nth;
  GString    *buffer;
  GimpUnit   *unit;
  GTokenType  token;

  /* parse the next token *before* reconfiguring the scanner, so it
   * skips whitespace first
   */
  token = g_scanner_peek_next_token (scanner);

  if (token == G_TOKEN_STRING)
    {
      g_scanner_get_next_token (scanner);
      unit = gimp_config_get_unit_from_identifier (scanner->value.v_string);
      g_value_set_object (value, unit);

      return G_TOKEN_RIGHT_PAREN;
    }

  old_cset_skip_characters  = scanner->config->cset_skip_characters;
  old_cset_identifier_first = scanner->config->cset_identifier_first;
  old_cset_identifier_nth   = scanner->config->cset_identifier_nth;

  scanner->config->cset_skip_characters  = "";
  scanner->config->cset_identifier_first = ( G_CSET_a_2_z G_CSET_A_2_Z "." );
  scanner->config->cset_identifier_nth   = ( G_CSET_a_2_z G_CSET_A_2_Z
                                             G_CSET_DIGITS "-_." );

  buffer = g_string_new ("");

  while (g_scanner_peek_next_token (scanner) != G_TOKEN_RIGHT_PAREN)
    {
      token = g_scanner_peek_next_token (scanner);

      if (token == G_TOKEN_IDENTIFIER)
        {
          g_scanner_get_next_token (scanner);
          g_string_append (buffer, scanner->value.v_identifier);
        }
      else if (token == G_TOKEN_CHAR)
        {
          g_scanner_get_next_token (scanner);
          g_string_append_c (buffer, scanner->value.v_char);
        }
      else if (token == ' ')
        {
          g_scanner_get_next_token (scanner);
          g_string_append_c (buffer, token);
        }
      else
        {
          token = G_TOKEN_IDENTIFIER;
          goto cleanup;
        }
    }

  unit = gimp_config_get_unit_from_identifier (buffer->str);
  g_value_set_object (value, unit);

  token = G_TOKEN_RIGHT_PAREN;

 cleanup:

  g_string_free (buffer, TRUE);

  scanner->config->cset_skip_characters  = old_cset_skip_characters;
  scanner->config->cset_identifier_first = old_cset_identifier_first;
  scanner->config->cset_identifier_nth   = old_cset_identifier_nth;

  return token;
}

static GTokenType
gimp_config_deserialize_file_value (GValue     *value,
                                    GParamSpec *prop_spec,
                                    GScanner   *scanner)
{
  GTokenType token;

  token = g_scanner_peek_next_token (scanner);

  if (token != G_TOKEN_IDENTIFIER &&
      token != G_TOKEN_STRING)
    {
      return G_TOKEN_STRING;
    }

  g_scanner_get_next_token (scanner);

  if (token == G_TOKEN_IDENTIFIER)
    {
      /* this is supposed to parse a literal "NULL" only, but so what... */
      g_value_set_object (value, NULL);
    }
  else
    {
      gchar *path = gimp_config_path_expand (scanner->value.v_string, TRUE,
                                             NULL);

      if (path)
        {
          GFile *file = g_file_new_for_path (path);

          g_value_take_object (value, file);
          g_free (path);
        }
      else
        {
          g_value_set_object (value, NULL);
        }
    }

  return G_TOKEN_RIGHT_PAREN;
}

/*
 * Note: this is different from gimp_config_deserialize_parasite()
 * which is a public API to deserialize random properties into a config
 * object from a parasite. Here we are deserializing the contents of a
 * parasite itself in @scanner.
 */
static GTokenType
gimp_config_deserialize_parasite_value (GValue     *value,
                                        GParamSpec *prop_spec,
                                        GScanner   *scanner)
{
  GimpParasite *parasite;
  gchar        *name;
  guint8       *data;
  gint          data_length;
  gint64        flags;

  if (! gimp_scanner_parse_string (scanner, &name))
    return G_TOKEN_STRING;

  if (! (name && *name))
    {
      g_scanner_error (scanner, "Parasite name is empty");
      g_free (name);
      return G_TOKEN_NONE;
    }

  if (! gimp_scanner_parse_int64 (scanner, &flags))
    return G_TOKEN_INT;

  if (! gimp_scanner_parse_int (scanner, &data_length))
    return G_TOKEN_INT;

  if (! gimp_scanner_parse_data (scanner, data_length, &data))
    return G_TOKEN_STRING;

  parasite = gimp_parasite_new (name, flags, data_length, data);
  g_free (data);

  g_value_take_boxed (value, parasite);

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_deserialize_bytes (GValue     *value,
                               GParamSpec *prop_spec,
                               GScanner   *scanner)
{
  GTokenType  token;
  GBytes     *bytes;
  guint8     *data;
  gint        data_length;

  token = g_scanner_peek_next_token (scanner);

  if (token == G_TOKEN_IDENTIFIER)
    {
      g_scanner_get_next_token (scanner);

      if (g_ascii_strcasecmp (scanner->value.v_identifier, "null") != 0)
        /* Do not fail the whole file parsing. Just output to stderr and assume
         * a NULL bytes property.
         */
        g_printerr ("%s: expected NULL identifier for bytes token '%s', got '%s'. "
                    "Assuming NULL instead.\n",
                    G_STRFUNC, prop_spec->name, scanner->value.v_identifier);

      g_value_set_boxed (value, NULL);
    }
  else if (token == G_TOKEN_INT)
    {
      if (! gimp_scanner_parse_int (scanner, &data_length))
        return G_TOKEN_INT;

      if (! gimp_scanner_parse_data (scanner, data_length, &data))
        return G_TOKEN_STRING;

      bytes = g_bytes_new_take (data, data_length);

      g_value_take_boxed (value, bytes);
    }
  else
    {
      return G_TOKEN_INT;
    }

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_deserialize_color (GValue     *value,
                               GParamSpec *prop_spec,
                               GScanner   *scanner)
{
  GeglColor *color = NULL;

  if (! gimp_scanner_parse_color (scanner, &color))
    return G_TOKEN_NONE;

  g_value_take_object (value, color);

  return G_TOKEN_RIGHT_PAREN;
}

static GTokenType
gimp_config_deserialize_any (GValue     *value,
                             GParamSpec *prop_spec,
                             GScanner   *scanner)
{
  GValue     src = G_VALUE_INIT;
  GTokenType token;

  if (!g_value_type_transformable (G_TYPE_STRING, prop_spec->value_type))
    {
      g_scanner_error (scanner,
                       "%s can not be transformed from a string",
                       g_type_name (prop_spec->value_type));
      return G_TOKEN_NONE;
    }

  token = g_scanner_peek_next_token (scanner);

  if (token != G_TOKEN_IDENTIFIER &&
      token != G_TOKEN_STRING)
    {
      return G_TOKEN_IDENTIFIER;
    }

  g_scanner_get_next_token (scanner);

  g_value_init (&src, G_TYPE_STRING);

  if (token == G_TOKEN_IDENTIFIER)
    g_value_set_static_string (&src, scanner->value.v_identifier);
  else
    g_value_set_static_string (&src, scanner->value.v_string);

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
