/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Config file serialization and deserialization interface
 * Copyright (C) 2001-2002  Sven Neumann <sven@ligma.org>
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

#include "libligmabase/ligmabase.h"

#include "ligmaconfigtypes.h"

#include "ligmaconfigwriter.h"
#include "ligmaconfig-iface.h"
#include "ligmaconfig-deserialize.h"
#include "ligmaconfig-serialize.h"
#include "ligmaconfig-params.h"
#include "ligmaconfig-utils.h"
#include "ligmascanner.h"

#include "libligma/libligma-intl.h"


/*
 * LigmaConfigIface:
 *
 * The [struct@Config] serialization and deserialization interface.
 */


/*  local function prototypes  */

static void         ligma_config_iface_default_init (LigmaConfigInterface *iface);
static void         ligma_config_iface_base_init    (LigmaConfigInterface *iface);

static gboolean     ligma_config_iface_serialize    (LigmaConfig          *config,
                                                    LigmaConfigWriter    *writer,
                                                    gpointer             data);
static gboolean     ligma_config_iface_deserialize  (LigmaConfig          *config,
                                                    GScanner            *scanner,
                                                    gint                 nest_level,
                                                    gpointer             data);
static LigmaConfig * ligma_config_iface_duplicate    (LigmaConfig          *config);
static gboolean     ligma_config_iface_equal        (LigmaConfig          *a,
                                                    LigmaConfig          *b);
static void         ligma_config_iface_reset        (LigmaConfig          *config);
static gboolean     ligma_config_iface_copy         (LigmaConfig          *src,
                                                    LigmaConfig          *dest,
                                                    GParamFlags          flags);


/*  private functions  */


GType
ligma_config_get_type (void)
{
  static GType config_iface_type = 0;

  if (! config_iface_type)
    {
      const GTypeInfo config_iface_info =
      {
        sizeof (LigmaConfigInterface),
        (GBaseInitFunc)      ligma_config_iface_base_init,
        (GBaseFinalizeFunc)  NULL,
        (GClassInitFunc)     ligma_config_iface_default_init,
        (GClassFinalizeFunc) NULL,
      };

      config_iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                                  "LigmaConfigInterface",
                                                  &config_iface_info,
                                                  0);

      g_type_interface_add_prerequisite (config_iface_type, G_TYPE_OBJECT);
    }

  return config_iface_type;
}

static void
ligma_config_iface_default_init (LigmaConfigInterface *iface)
{
  iface->serialize   = ligma_config_iface_serialize;
  iface->deserialize = ligma_config_iface_deserialize;
  iface->duplicate   = ligma_config_iface_duplicate;
  iface->equal       = ligma_config_iface_equal;
  iface->reset       = ligma_config_iface_reset;
  iface->copy        = ligma_config_iface_copy;
}

static void
ligma_config_iface_base_init (LigmaConfigInterface *iface)
{
  /*  always set these to NULL since we don't want to inherit them
   *  from parent classes
   */
  iface->serialize_property   = NULL;
  iface->deserialize_property = NULL;
}

static gboolean
ligma_config_iface_serialize (LigmaConfig       *config,
                             LigmaConfigWriter *writer,
                             gpointer          data)
{
  return ligma_config_serialize_properties (config, writer);
}

static gboolean
ligma_config_iface_deserialize (LigmaConfig *config,
                               GScanner   *scanner,
                               gint        nest_level,
                               gpointer    data)
{
  return ligma_config_deserialize_properties (config, scanner, nest_level);
}

static LigmaConfig *
ligma_config_iface_duplicate (LigmaConfig *config)
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

  ligma_config_copy (config, LIGMA_CONFIG (dup), 0);

  return LIGMA_CONFIG (dup);
}

static gboolean
ligma_config_iface_equal (LigmaConfig *a,
                         LigmaConfig *b)
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
            (prop_spec->flags & LIGMA_CONFIG_PARAM_DONT_COMPARE))
        {
          continue;
        }

      g_value_init (&a_value, prop_spec->value_type);
      g_value_init (&b_value, prop_spec->value_type);
      g_object_get_property (G_OBJECT (a), prop_spec->name, &a_value);
      g_object_get_property (G_OBJECT (b), prop_spec->name, &b_value);

      if (g_param_values_cmp (prop_spec, &a_value, &b_value))
        {
          if ((prop_spec->flags & LIGMA_CONFIG_PARAM_AGGREGATE) &&
              G_IS_PARAM_SPEC_OBJECT (prop_spec)        &&
              g_type_interface_peek (g_type_class_peek (prop_spec->value_type),
                                     LIGMA_TYPE_CONFIG))
            {
              if (! ligma_config_is_equal_to (g_value_get_object (&a_value),
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
ligma_config_iface_reset (LigmaConfig *config)
{
  ligma_config_reset_properties (G_OBJECT (config));
}

static gboolean
ligma_config_iface_copy (LigmaConfig  *src,
                        LigmaConfig  *dest,
                        GParamFlags  flags)
{
  return ligma_config_sync (G_OBJECT (src), G_OBJECT (dest), flags);
}


/*  public functions  */


/**
 * ligma_config_serialize_to_file:
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
ligma_config_serialize_to_file (LigmaConfig   *config,
                               GFile        *file,
                               const gchar  *header,
                               const gchar  *footer,
                               gpointer      data,
                               GError      **error)
{
  LigmaConfigWriter *writer;

  g_return_val_if_fail (LIGMA_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  writer = ligma_config_writer_new_from_file (file, TRUE, header, error);
  if (!writer)
    return FALSE;

  LIGMA_CONFIG_GET_IFACE (config)->serialize (config, writer, data);

  return ligma_config_writer_finish (writer, footer, error);
}

/**
 * ligma_config_serialize_to_stream:
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
ligma_config_serialize_to_stream (LigmaConfig     *config,
                                 GOutputStream  *output,
                                 const gchar    *header,
                                 const gchar    *footer,
                                 gpointer        data,
                                 GError        **error)
{
  LigmaConfigWriter *writer;

  g_return_val_if_fail (LIGMA_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  writer = ligma_config_writer_new_from_stream (output, header, error);
  if (!writer)
    return FALSE;

  LIGMA_CONFIG_GET_IFACE (config)->serialize (config, writer, data);

  return ligma_config_writer_finish (writer, footer, error);
}

/**
 * ligma_config_serialize_to_fd:
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
ligma_config_serialize_to_fd (LigmaConfig *config,
                             gint        fd,
                             gpointer    data)
{
  LigmaConfigWriter *writer;

  g_return_val_if_fail (LIGMA_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (fd > 0, FALSE);

  writer = ligma_config_writer_new_from_fd (fd);
  if (!writer)
    return FALSE;

  LIGMA_CONFIG_GET_IFACE (config)->serialize (config, writer, data);

  return ligma_config_writer_finish (writer, NULL, NULL);
}

/**
 * ligma_config_serialize_to_string:
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
ligma_config_serialize_to_string (LigmaConfig *config,
                                 gpointer    data)
{
  LigmaConfigWriter *writer;
  GString          *str;

  g_return_val_if_fail (LIGMA_IS_CONFIG (config), NULL);

  str = g_string_new (NULL);
  writer = ligma_config_writer_new_from_string (str);

  LIGMA_CONFIG_GET_IFACE (config)->serialize (config, writer, data);

  ligma_config_writer_finish (writer, NULL, NULL);

  return g_string_free (str, FALSE);
}

/**
 * ligma_config_serialize_to_parasite:
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
LigmaParasite *
ligma_config_serialize_to_parasite (LigmaConfig  *config,
                                   const gchar *parasite_name,
                                   guint        parasite_flags,
                                   gpointer     data)
{
  LigmaParasite *parasite;
  gchar        *str;

  g_return_val_if_fail (LIGMA_IS_CONFIG (config), NULL);
  g_return_val_if_fail (parasite_name != NULL, NULL);

  str = ligma_config_serialize_to_string (config, data);

  if (! str)
    return NULL;

  parasite = ligma_parasite_new (parasite_name,
                                parasite_flags,
                                0, NULL);

  parasite->size = strlen (str) + 1;
  parasite->data = str;

  return parasite;
}

/**
 * ligma_config_deserialize_file:
 * @config: an oObject that implements the #LigmaConfigInterface.
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
ligma_config_deserialize_file (LigmaConfig  *config,
                              GFile       *file,
                              gpointer     data,
                              GError     **error)
{
  GScanner *scanner;
  gboolean  success;

  g_return_val_if_fail (LIGMA_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  scanner = ligma_scanner_new_file (file, error);
  if (! scanner)
    return FALSE;

  g_object_freeze_notify (G_OBJECT (config));

  success = LIGMA_CONFIG_GET_IFACE (config)->deserialize (config,
                                                         scanner, 0, data);

  g_object_thaw_notify (G_OBJECT (config));

  ligma_scanner_unref (scanner);

  if (! success)
    /* If we get this assert, it means we have a bug in one of the
     * deserialize() implementations. Any failure case should report the
     * error condition with g_scanner_error() which will populate the
     * error object passed in ligma_scanner_new*().
     */
    g_assert (error == NULL || *error != NULL);

  return success;
}

/**
 * ligma_config_deserialize_stream:
 * @config: an object that implements the #LigmaConfigInterface.
 * @input: the input stream to read configuration from.
 * @data: user data passed to the deserialize implementation.
 * @error: return location for a possible error
 *
 * Reads configuration data from @input and configures @config
 * accordingly. Basically this function creates a properly configured
 * #GScanner for you and calls the deserialize function of the
 * @config's #LigmaConfigInterface.
 *
 * Returns: Whether deserialization succeeded.
 *
 * Since: 2.10
 **/
gboolean
ligma_config_deserialize_stream (LigmaConfig    *config,
                                GInputStream  *input,
                                gpointer       data,
                                GError       **error)
{
  GScanner *scanner;
  gboolean  success;

  g_return_val_if_fail (LIGMA_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  scanner = ligma_scanner_new_stream (input, error);
  if (! scanner)
    return FALSE;

  g_object_freeze_notify (G_OBJECT (config));

  success = LIGMA_CONFIG_GET_IFACE (config)->deserialize (config,
                                                         scanner, 0, data);

  g_object_thaw_notify (G_OBJECT (config));

  ligma_scanner_unref (scanner);

  if (! success)
    g_assert (error == NULL || *error != NULL);

  return success;
}

/**
 * ligma_config_deserialize_string:
 * @config:   a #GObject that implements the #LigmaConfigInterface.
 * @text: (array length=text_len): string to deserialize (in UTF-8 encoding)
 * @text_len: length of @text in bytes or -1
 * @data:     client data
 * @error:    return location for a possible error
 *
 * Configures @config from @text. Basically this function creates a
 * properly configured #GScanner for you and calls the deserialize
 * function of the @config's #LigmaConfigInterface.
 *
 * Returns: %TRUE if deserialization succeeded, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
ligma_config_deserialize_string (LigmaConfig   *config,
                                const gchar  *text,
                                gint          text_len,
                                gpointer      data,
                                GError      **error)
{
  GScanner *scanner;
  gboolean  success;

  g_return_val_if_fail (LIGMA_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (text != NULL || text_len == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  scanner = ligma_scanner_new_string (text, text_len, error);

  g_object_freeze_notify (G_OBJECT (config));

  success = LIGMA_CONFIG_GET_IFACE (config)->deserialize (config,
                                                         scanner, 0, data);

  g_object_thaw_notify (G_OBJECT (config));

  ligma_scanner_unref (scanner);

  if (! success)
    g_assert (error == NULL || *error != NULL);

  return success;
}

/**
 * ligma_config_deserialize_parasite:
 * @config:   a #GObject that implements the #LigmaConfigInterface.
 * @parasite: parasite containing a serialized config string
 * @data:     client data
 * @error:    return location for a possible error
 *
 * Configures @config from @parasite. Basically this function creates
 * a properly configured #GScanner for you and calls the deserialize
 * function of the @config's #LigmaConfigInterface.
 *
 * Returns: %TRUE if deserialization succeeded, %FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
ligma_config_deserialize_parasite (LigmaConfig          *config,
                                  const LigmaParasite  *parasite,
                                  gpointer             data,
                                  GError             **error)
{
  const gchar *parasite_data;
  guint32      parasite_size;

  g_return_val_if_fail (LIGMA_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  parasite_data = ligma_parasite_get_data (parasite, &parasite_size);
  if (! parasite_data)
    return TRUE;

  return ligma_config_deserialize_string (config, parasite_data, parasite_size,
                                         data, error);
}

/**
 * ligma_config_deserialize_return:
 * @scanner:        a #GScanner
 * @expected_token: the expected token
 * @nest_level:     the nest level
 *
 * Returns:
 *
 * Since: 2.4
 **/
gboolean
ligma_config_deserialize_return (GScanner     *scanner,
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
 * ligma_config_serialize:
 * @config: an object that implements the #LigmaConfigInterface.
 * @writer: the #LigmaConfigWriter to use.
 * @data: client data
 *
 * Serialize the #LigmaConfig object.
 *
 * Returns: Whether serialization succeeded.
 *
 * Since: 2.8
 **/
gboolean
ligma_config_serialize (LigmaConfig       *config,
                       LigmaConfigWriter *writer,
                       gpointer          data)
{
  g_return_val_if_fail (LIGMA_IS_CONFIG (config), FALSE);

  return LIGMA_CONFIG_GET_IFACE (config)->serialize (config, writer, data);
}

/**
 * ligma_config_deserialize:
 * @config: a #GObject that implements the #LigmaConfigInterface.
 * @scanner: the #GScanner to use.
 * @nest_level: the nest level.
 * @data: client data.
 *
 * Deserialize the #LigmaConfig object.
 *
 * Returns: Whether serialization succeeded.
 *
 * Since: 2.8
 **/
gboolean
ligma_config_deserialize (LigmaConfig *config,
                         GScanner   *scanner,
                         gint        nest_level,
                         gpointer    data)
{
  g_return_val_if_fail (LIGMA_IS_CONFIG (config), FALSE);

  return LIGMA_CONFIG_GET_IFACE (config)->deserialize (config,
                                                      scanner,
                                                      nest_level,
                                                      data);
}

/**
 * ligma_config_duplicate:
 * @config: a #GObject that implements the #LigmaConfigInterface.
 *
 * Creates a copy of the passed object by copying all object
 * properties. The default implementation of the #LigmaConfigInterface
 * only works for objects that are completely defined by their
 * properties.
 *
 * Returns: the duplicated #LigmaConfig object
 *
 * Since: 2.4
 **/
gpointer
ligma_config_duplicate (LigmaConfig *config)
{
  g_return_val_if_fail (LIGMA_IS_CONFIG (config), NULL);

  return LIGMA_CONFIG_GET_IFACE (config)->duplicate (config);
}

/**
 * ligma_config_is_equal_to:
 * @a: a #GObject that implements the #LigmaConfigInterface.
 * @b: another #GObject of the same type as @a.
 *
 * Compares the two objects. The default implementation of the
 * #LigmaConfigInterface compares the object properties and thus only
 * works for objects that are completely defined by their
 * properties.
 *
 * Returns: %TRUE if the two objects are equal.
 *
 * Since: 2.4
 **/
gboolean
ligma_config_is_equal_to (LigmaConfig *a,
                         LigmaConfig *b)
{
  g_return_val_if_fail (LIGMA_IS_CONFIG (a), FALSE);
  g_return_val_if_fail (LIGMA_IS_CONFIG (b), FALSE);
  g_return_val_if_fail (G_TYPE_FROM_INSTANCE (a) == G_TYPE_FROM_INSTANCE (b),
                        FALSE);

  return LIGMA_CONFIG_GET_IFACE (a)->equal (a, b);
}

/**
 * ligma_config_reset:
 * @config: a #GObject that implements the #LigmaConfigInterface.
 *
 * Resets the object to its default state. The default implementation of the
 * #LigmaConfigInterface only works for objects that are completely defined by
 * their properties.
 *
 * Since: 2.4
 **/
void
ligma_config_reset (LigmaConfig *config)
{
  g_return_if_fail (LIGMA_IS_CONFIG (config));

  g_object_freeze_notify (G_OBJECT (config));

  LIGMA_CONFIG_GET_IFACE (config)->reset (config);

  g_object_thaw_notify (G_OBJECT (config));
}

/**
 * ligma_config_copy:
 * @src: a #GObject that implements the #LigmaConfigInterface.
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
ligma_config_copy (LigmaConfig  *src,
                  LigmaConfig  *dest,
                  GParamFlags  flags)
{
  gboolean changed;

  g_return_val_if_fail (LIGMA_IS_CONFIG (src), FALSE);
  g_return_val_if_fail (LIGMA_IS_CONFIG (dest), FALSE);
  g_return_val_if_fail (G_TYPE_FROM_INSTANCE (src) == G_TYPE_FROM_INSTANCE (dest),
                        FALSE);

  g_object_freeze_notify (G_OBJECT (dest));

  changed = LIGMA_CONFIG_GET_IFACE (src)->copy (src, dest, flags);

  g_object_thaw_notify (G_OBJECT (dest));

  return changed;
}
