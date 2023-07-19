/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Config file serialization and deserialization interface
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

#include <string.h>

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "gimpconfigtypes.h"

#include "gimpconfigwriter.h"
#include "gimpconfig-iface.h"
#include "gimpconfig-deserialize.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-params.h"
#include "gimpconfig-utils.h"
#include "gimpscanner.h"

#include "libgimp/libgimp-intl.h"


/*
 * GimpConfigIface:
 *
 * The [struct@Config] serialization and deserialization interface.
 */


/*  local function prototypes  */

static void         gimp_config_iface_default_init (GimpConfigInterface *iface);
static void         gimp_config_iface_base_init    (GimpConfigInterface *iface);

static gboolean     gimp_config_iface_serialize    (GimpConfig          *config,
                                                    GimpConfigWriter    *writer,
                                                    gpointer             data);
static gboolean     gimp_config_iface_deserialize  (GimpConfig          *config,
                                                    GScanner            *scanner,
                                                    gint                 nest_level,
                                                    gpointer             data);
static GimpConfig * gimp_config_iface_duplicate    (GimpConfig          *config);
static gboolean     gimp_config_iface_equal        (GimpConfig          *a,
                                                    GimpConfig          *b);
static void         gimp_config_iface_reset        (GimpConfig          *config);
static gboolean     gimp_config_iface_copy         (GimpConfig          *src,
                                                    GimpConfig          *dest,
                                                    GParamFlags          flags);


/*  private functions  */


GType
gimp_config_get_type (void)
{
  static GType config_iface_type = 0;

  if (! config_iface_type)
    {
      const GTypeInfo config_iface_info =
      {
        sizeof (GimpConfigInterface),
        (GBaseInitFunc)      gimp_config_iface_base_init,
        (GBaseFinalizeFunc)  NULL,
        (GClassInitFunc)     gimp_config_iface_default_init,
        (GClassFinalizeFunc) NULL,
      };

      config_iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                                  "GimpConfigInterface",
                                                  &config_iface_info,
                                                  0);

      g_type_interface_add_prerequisite (config_iface_type, G_TYPE_OBJECT);
    }

  return config_iface_type;
}

static void
gimp_config_iface_default_init (GimpConfigInterface *iface)
{
  iface->serialize   = gimp_config_iface_serialize;
  iface->deserialize = gimp_config_iface_deserialize;
  iface->duplicate   = gimp_config_iface_duplicate;
  iface->equal       = gimp_config_iface_equal;
  iface->reset       = gimp_config_iface_reset;
  iface->copy        = gimp_config_iface_copy;
}

static void
gimp_config_iface_base_init (GimpConfigInterface *iface)
{
  /*  always set these to NULL since we don't want to inherit them
   *  from parent classes
   */
  iface->serialize_property   = NULL;
  iface->deserialize_property = NULL;
}

static gboolean
gimp_config_iface_serialize (GimpConfig       *config,
                             GimpConfigWriter *writer,
                             gpointer          data)
{
  return gimp_config_serialize_properties (config, writer);
}

static gboolean
gimp_config_iface_deserialize (GimpConfig *config,
                               GScanner   *scanner,
                               gint        nest_level,
                               gpointer    data)
{
  return gimp_config_deserialize_properties (config, scanner, nest_level);
}

static GimpConfig *
gimp_config_iface_duplicate (GimpConfig *config)
{
  GObject       *object = G_OBJECT (config);
  GObjectClass  *klass  = G_OBJECT_GET_CLASS (object);
  GParamSpec   **property_specs;
  guint          n_property_specs;
  gint           n_construct_properties = 0;
  const gchar  **construct_names        = NULL;
  GValue        *construct_values       = NULL;
  guint          i;
  GObject       *dup;

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  construct_names  = g_new0 (const gchar *, n_property_specs);
  construct_values = g_new0 (GValue,        n_property_specs);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];

      if ((prop_spec->flags & G_PARAM_READABLE) &&
          (prop_spec->flags & G_PARAM_WRITABLE) &&
          (prop_spec->flags & G_PARAM_CONSTRUCT_ONLY))
        {
          construct_names[n_construct_properties] = prop_spec->name;

          g_value_init (&construct_values[n_construct_properties],
                        prop_spec->value_type);
          g_object_get_property (object, prop_spec->name,
                                 &construct_values[n_construct_properties]);

          n_construct_properties++;
        }
    }

  g_free (property_specs);

  dup = g_object_new_with_properties (G_TYPE_FROM_INSTANCE (object),
                                      n_construct_properties,
                                      (const gchar **) construct_names,
                                      (const GValue *) construct_values);

  for (i = 0; i < n_construct_properties; i++)
    g_value_unset (&construct_values[i]);

  g_free (construct_names);
  g_free (construct_values);

  gimp_config_copy (config, GIMP_CONFIG (dup), 0);

  return GIMP_CONFIG (dup);
}

static gboolean
gimp_config_iface_equal (GimpConfig *a,
                         GimpConfig *b)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;
  gboolean       equal = TRUE;

  klass = G_OBJECT_GET_CLASS (a);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  for (i = 0; equal && i < n_property_specs; i++)
    {
      GParamSpec  *prop_spec;
      GValue       a_value = G_VALUE_INIT;
      GValue       b_value = G_VALUE_INIT;

      prop_spec = property_specs[i];

      if (! (prop_spec->flags & G_PARAM_READABLE) ||
            (prop_spec->flags & GIMP_CONFIG_PARAM_DONT_COMPARE))
        {
          continue;
        }

      g_value_init (&a_value, prop_spec->value_type);
      g_value_init (&b_value, prop_spec->value_type);
      g_object_get_property (G_OBJECT (a), prop_spec->name, &a_value);
      g_object_get_property (G_OBJECT (b), prop_spec->name, &b_value);

      if (g_param_values_cmp (prop_spec, &a_value, &b_value))
        {
          if ((prop_spec->flags & GIMP_CONFIG_PARAM_AGGREGATE) &&
              G_IS_PARAM_SPEC_OBJECT (prop_spec)        &&
              g_type_interface_peek (g_type_class_peek (prop_spec->value_type),
                                     GIMP_TYPE_CONFIG))
            {
              if (! gimp_config_is_equal_to (g_value_get_object (&a_value),
                                             g_value_get_object (&b_value)))
                {
                  equal = FALSE;
                }
            }
          else
            {
              equal = FALSE;
            }
        }

      g_value_unset (&a_value);
      g_value_unset (&b_value);
    }

  g_free (property_specs);

  return equal;
}

static void
gimp_config_iface_reset (GimpConfig *config)
{
  gimp_config_reset_properties (G_OBJECT (config));
}

static gboolean
gimp_config_iface_copy (GimpConfig  *src,
                        GimpConfig  *dest,
                        GParamFlags  flags)
{
  return gimp_config_sync (G_OBJECT (src), G_OBJECT (dest), flags);
}


/*  public functions  */


/**
 * gimp_config_serialize_to_file:
 * @config: an object that implements [iface@ConfigInterface].
 * @file:   the file to write the configuration to.
 * @header: (nullable): optional file header (must be ASCII only)
 * @footer: (nullable): optional file footer (must be ASCII only)
 * @data: user data passed to the serialize implementation.
 * @error: return location for a possible error
 *
 * Serializes the object properties of @config to the file specified
 * by @file. If a file with that name already exists, it is
 * overwritten. Basically this function opens @file for you and calls
 * the serialize function of the @config's [iface@ConfigInterface].
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise.
 *
 * Since: 2.10
 **/
gboolean
gimp_config_serialize_to_file (GimpConfig   *config,
                               GFile        *file,
                               const gchar  *header,
                               const gchar  *footer,
                               gpointer      data,
                               GError      **error)
{
  GimpConfigWriter *writer;

  g_return_val_if_fail (GIMP_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  writer = gimp_config_writer_new_from_file (file, TRUE, header, error);
  if (!writer)
    return FALSE;

  GIMP_CONFIG_GET_IFACE (config)->serialize (config, writer, data);

  return gimp_config_writer_finish (writer, footer, error);
}

/**
 * gimp_config_serialize_to_stream:
 * @config: an object that implements [iface@ConfigInterface].
 * @output: the #GOutputStream to write the configuration to.
 * @header: (nullable): optional file header (must be ASCII only)
 * @footer: (nullable): optional file footer (must be ASCII only)
 * @data: user data passed to the serialize implementation.
 * @error: return location for a possible error
 *
 * Serializes the object properties of @config to the stream specified
 * by @output.
 *
 * Returns: Whether serialization succeeded.
 *
 * Since: 2.10
 **/
gboolean
gimp_config_serialize_to_stream (GimpConfig     *config,
                                 GOutputStream  *output,
                                 const gchar    *header,
                                 const gchar    *footer,
                                 gpointer        data,
                                 GError        **error)
{
  GimpConfigWriter *writer;

  g_return_val_if_fail (GIMP_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  writer = gimp_config_writer_new_from_stream (output, header, error);
  if (!writer)
    return FALSE;

  GIMP_CONFIG_GET_IFACE (config)->serialize (config, writer, data);

  return gimp_config_writer_finish (writer, footer, error);
}

/**
 * gimp_config_serialize_to_fd:
 * @config: an object that implements [iface@ConfigInterface].
 * @fd: a file descriptor, opened for writing
 * @data: user data passed to the serialize implementation.
 *
 * Serializes the object properties of @config to the given file
 * descriptor.
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
gimp_config_serialize_to_fd (GimpConfig *config,
                             gint        fd,
                             gpointer    data)
{
  GimpConfigWriter *writer;

  g_return_val_if_fail (GIMP_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (fd > 0, FALSE);

  writer = gimp_config_writer_new_from_fd (fd);
  if (!writer)
    return FALSE;

  GIMP_CONFIG_GET_IFACE (config)->serialize (config, writer, data);

  return gimp_config_writer_finish (writer, NULL, NULL);
}

/**
 * gimp_config_serialize_to_string:
 * @config: an object that implements the [iface@ConfigInterface].
 * @data: user data passed to the serialize implementation.
 *
 * Serializes the object properties of @config to a string.
 *
 * Returns: a newly allocated NUL-terminated string.
 *
 * Since: 2.4
 **/
gchar *
gimp_config_serialize_to_string (GimpConfig *config,
                                 gpointer    data)
{
  GimpConfigWriter *writer;
  GString          *str;

  g_return_val_if_fail (GIMP_IS_CONFIG (config), NULL);

  str = g_string_new (NULL);
  writer = gimp_config_writer_new_from_string (str);

  GIMP_CONFIG_GET_IFACE (config)->serialize (config, writer, data);

  gimp_config_writer_finish (writer, NULL, NULL);

  return g_string_free (str, FALSE);
}

/**
 * gimp_config_serialize_to_parasite:
 * @config:         an object that implements the [iface@ConfigInterface].
 * @parasite_name:  the new parasite's name
 * @parasite_flags: the new parasite's flags
 * @data:           user data passed to the serialize implementation.
 *
 * Serializes the object properties of @config to a [struct@Parasite].
 *
 * Returns: (transfer full): the newly allocated parasite.
 *
 * Since: 3.0
 **/
GimpParasite *
gimp_config_serialize_to_parasite (GimpConfig  *config,
                                   const gchar *parasite_name,
                                   guint        parasite_flags,
                                   gpointer     data)
{
  GimpParasite *parasite;
  gchar        *str;

  g_return_val_if_fail (GIMP_IS_CONFIG (config), NULL);
  g_return_val_if_fail (parasite_name != NULL, NULL);

  str = gimp_config_serialize_to_string (config, data);

  if (! str)
    return NULL;

  parasite = gimp_parasite_new (parasite_name,
                                parasite_flags,
                                0, NULL);

  parasite->size = strlen (str) + 1;
  parasite->data = str;

  return parasite;
}

/**
 * gimp_config_deserialize_file:
 * @config: an object that implements the #GimpConfigInterface.
 * @file: the file to read configuration from.
 * @data: user data passed to the deserialize implementation.
 * @error: return location for a possible error
 *
 * Opens the file specified by @file, reads configuration data from it
 * and configures @config accordingly. Basically this function creates
 * a properly configured [struct@GLib.Scanner] for you and calls the deserialize
 * function of the @config's [iface@ConfigInterface].
 *
 * Returns: Whether deserialization succeeded.
 *
 * Since: 2.10
 **/
gboolean
gimp_config_deserialize_file (GimpConfig  *config,
                              GFile       *file,
                              gpointer     data,
                              GError     **error)
{
  GScanner *scanner;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  scanner = gimp_scanner_new_file (file, error);
  if (! scanner)
    return FALSE;

  g_object_freeze_notify (G_OBJECT (config));

  success = GIMP_CONFIG_GET_IFACE (config)->deserialize (config,
                                                         scanner, 0, data);

  g_object_thaw_notify (G_OBJECT (config));

  gimp_scanner_unref (scanner);

  if (! success)
    /* If we get this assert, it means we have a bug in one of the
     * deserialize() implementations. Any failure case should report the
     * error condition with g_scanner_error() which will populate the
     * error object passed in gimp_scanner_new*().
     */
    g_assert (error == NULL || *error != NULL);

  return success;
}

/**
 * gimp_config_deserialize_stream:
 * @config: an object that implements the #GimpConfigInterface.
 * @input: the input stream to read configuration from.
 * @data: user data passed to the deserialize implementation.
 * @error: return location for a possible error
 *
 * Reads configuration data from @input and configures @config
 * accordingly. Basically this function creates a properly configured
 * #GScanner for you and calls the deserialize function of the
 * @config's #GimpConfigInterface.
 *
 * Returns: Whether deserialization succeeded.
 *
 * Since: 2.10
 **/
gboolean
gimp_config_deserialize_stream (GimpConfig    *config,
                                GInputStream  *input,
                                gpointer       data,
                                GError       **error)
{
  GScanner *scanner;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  scanner = gimp_scanner_new_stream (input, error);
  if (! scanner)
    return FALSE;

  g_object_freeze_notify (G_OBJECT (config));

  success = GIMP_CONFIG_GET_IFACE (config)->deserialize (config,
                                                         scanner, 0, data);

  g_object_thaw_notify (G_OBJECT (config));

  gimp_scanner_unref (scanner);

  if (! success)
    g_assert (error == NULL || *error != NULL);

  return success;
}

/**
 * gimp_config_deserialize_string:
 * @config:   a #GObject that implements the #GimpConfigInterface.
 * @text: (array length=text_len): string to deserialize (in UTF-8 encoding)
 * @text_len: length of @text in bytes or -1
 * @data:     client data
 * @error:    return location for a possible error
 *
 * Configures @config from @text. Basically this function creates a
 * properly configured #GScanner for you and calls the deserialize
 * function of the @config's #GimpConfigInterface.
 *
 * Returns: %TRUE if deserialization succeeded, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
gimp_config_deserialize_string (GimpConfig   *config,
                                const gchar  *text,
                                gint          text_len,
                                gpointer      data,
                                GError      **error)
{
  GScanner *scanner;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (text != NULL || text_len == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  scanner = gimp_scanner_new_string (text, text_len, error);

  g_object_freeze_notify (G_OBJECT (config));

  success = GIMP_CONFIG_GET_IFACE (config)->deserialize (config,
                                                         scanner, 0, data);

  g_object_thaw_notify (G_OBJECT (config));

  gimp_scanner_unref (scanner);

  if (! success)
    g_assert (error == NULL || *error != NULL);

  return success;
}

/**
 * gimp_config_deserialize_parasite:
 * @config:   a #GObject that implements the #GimpConfigInterface.
 * @parasite: parasite containing a serialized config string
 * @data:     client data
 * @error:    return location for a possible error
 *
 * Configures @config from @parasite. Basically this function creates
 * a properly configured #GScanner for you and calls the deserialize
 * function of the @config's #GimpConfigInterface.
 *
 * Returns: %TRUE if deserialization succeeded, %FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
gimp_config_deserialize_parasite (GimpConfig          *config,
                                  const GimpParasite  *parasite,
                                  gpointer             data,
                                  GError             **error)
{
  const gchar *parasite_data;
  guint32      parasite_size;

  g_return_val_if_fail (GIMP_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  parasite_data = gimp_parasite_get_data (parasite, &parasite_size);
  if (! parasite_data)
    return TRUE;

  return gimp_config_deserialize_string (config, parasite_data, parasite_size,
                                         data, error);
}

/**
 * gimp_config_deserialize_return:
 * @scanner:        a #GScanner
 * @expected_token: the expected token
 * @nest_level:     the nest level
 *
 * Returns:
 *
 * Since: 2.4
 **/
gboolean
gimp_config_deserialize_return (GScanner     *scanner,
                                GTokenType    expected_token,
                                gint          nest_level)
{
  GTokenType next_token;

  g_return_val_if_fail (scanner != NULL, FALSE);

  next_token = g_scanner_peek_next_token (scanner);

  if (expected_token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, expected_token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
      return FALSE;
    }
  else
    {
      if (nest_level > 0 && next_token == G_TOKEN_RIGHT_PAREN)
        {
          return TRUE;
        }
      else if (next_token != G_TOKEN_EOF)
        {
          g_scanner_get_next_token (scanner);
          g_scanner_unexp_token (scanner, expected_token, NULL, NULL, NULL,
                                 _("fatal parse error"), TRUE);
          return FALSE;
        }
    }

  return TRUE;
}


/**
 * gimp_config_serialize:
 * @config: an object that implements the #GimpConfigInterface.
 * @writer: the #GimpConfigWriter to use.
 * @data: client data
 *
 * Serialize the #GimpConfig object.
 *
 * Returns: Whether serialization succeeded.
 *
 * Since: 2.8
 **/
gboolean
gimp_config_serialize (GimpConfig       *config,
                       GimpConfigWriter *writer,
                       gpointer          data)
{
  g_return_val_if_fail (GIMP_IS_CONFIG (config), FALSE);

  return GIMP_CONFIG_GET_IFACE (config)->serialize (config, writer, data);
}

/**
 * gimp_config_deserialize:
 * @config: a #GObject that implements the #GimpConfigInterface.
 * @scanner: the #GScanner to use.
 * @nest_level: the nest level.
 * @data: client data.
 *
 * Deserialize the #GimpConfig object.
 *
 * Returns: Whether serialization succeeded.
 *
 * Since: 2.8
 **/
gboolean
gimp_config_deserialize (GimpConfig *config,
                         GScanner   *scanner,
                         gint        nest_level,
                         gpointer    data)
{
  g_return_val_if_fail (GIMP_IS_CONFIG (config), FALSE);

  return GIMP_CONFIG_GET_IFACE (config)->deserialize (config,
                                                      scanner,
                                                      nest_level,
                                                      data);
}

/**
 * gimp_config_duplicate:
 * @config: a #GObject that implements the #GimpConfigInterface.
 *
 * Creates a copy of the passed object by copying all object
 * properties. The default implementation of the #GimpConfigInterface
 * only works for objects that are completely defined by their
 * properties.
 *
 * Returns: the duplicated #GimpConfig object
 *
 * Since: 2.4
 **/
gpointer
gimp_config_duplicate (GimpConfig *config)
{
  g_return_val_if_fail (GIMP_IS_CONFIG (config), NULL);

  return GIMP_CONFIG_GET_IFACE (config)->duplicate (config);
}

/**
 * gimp_config_is_equal_to:
 * @a: a #GObject that implements the #GimpConfigInterface.
 * @b: another #GObject of the same type as @a.
 *
 * Compares the two objects. The default implementation of the
 * #GimpConfigInterface compares the object properties and thus only
 * works for objects that are completely defined by their
 * properties.
 *
 * Returns: %TRUE if the two objects are equal.
 *
 * Since: 2.4
 **/
gboolean
gimp_config_is_equal_to (GimpConfig *a,
                         GimpConfig *b)
{
  g_return_val_if_fail (GIMP_IS_CONFIG (a), FALSE);
  g_return_val_if_fail (GIMP_IS_CONFIG (b), FALSE);
  g_return_val_if_fail (G_TYPE_FROM_INSTANCE (a) == G_TYPE_FROM_INSTANCE (b),
                        FALSE);

  return GIMP_CONFIG_GET_IFACE (a)->equal (a, b);
}

/**
 * gimp_config_reset:
 * @config: a #GObject that implements the #GimpConfigInterface.
 *
 * Resets the object to its default state. The default implementation of the
 * #GimpConfigInterface only works for objects that are completely defined by
 * their properties.
 *
 * Since: 2.4
 **/
void
gimp_config_reset (GimpConfig *config)
{
  g_return_if_fail (GIMP_IS_CONFIG (config));

  g_object_freeze_notify (G_OBJECT (config));

  GIMP_CONFIG_GET_IFACE (config)->reset (config);

  g_object_thaw_notify (G_OBJECT (config));
}

/**
 * gimp_config_copy:
 * @src: a #GObject that implements the #GimpConfigInterface.
 * @dest: another #GObject of the same type as @a.
 * @flags: a mask of GParamFlags
 *
 * Compares all read- and write-able properties from @src and @dest
 * that have all @flags set. Differing values are then copied from
 * @src to @dest. If @flags is 0, all differing read/write properties.
 *
 * Properties marked as "construct-only" are not touched.
 *
 * Returns: %TRUE if @dest was modified, %FALSE otherwise
 *
 * Since: 2.6
 **/
gboolean
gimp_config_copy (GimpConfig  *src,
                  GimpConfig  *dest,
                  GParamFlags  flags)
{
  gboolean changed;

  g_return_val_if_fail (GIMP_IS_CONFIG (src), FALSE);
  g_return_val_if_fail (GIMP_IS_CONFIG (dest), FALSE);
  g_return_val_if_fail (G_TYPE_FROM_INSTANCE (src) == G_TYPE_FROM_INSTANCE (dest),
                        FALSE);

  g_object_freeze_notify (G_OBJECT (dest));

  changed = GIMP_CONFIG_GET_IFACE (src)->copy (src, dest, flags);

  g_object_thaw_notify (G_OBJECT (dest));

  return changed;
}
