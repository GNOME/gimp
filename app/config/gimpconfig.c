/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Config file serialization and deserialization interface
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include "gimpconfig.h"
#include "gimpconfig-serialize.h"
#include "gimpconfig-deserialize.h"
#include "gimpconfig-utils.h"

#include "libgimp/gimpintl.h"


/* 
 * The GimpConfig serialization and deserialization interface.
 */

static void  gimp_config_iface_init (GimpConfigInterface  *gimp_config_iface);

static void      gimp_config_iface_serialize    (GObject  *object,
                                                 gint      fd);
static gboolean  gimp_config_iface_deserialize  (GObject  *object,
                                                 GScanner *scanner);
static GObject  *gimp_config_iface_duplicate    (GObject  *object);
static gboolean  gimp_config_iface_equal        (GObject  *a,
                                                 GObject  *b);
static void      gimp_config_scanner_message    (GScanner *scanner,
                                                 gchar    *message,
                                                 gboolean  is_error);


GType
gimp_config_interface_get_type (void)
{
  static GType config_iface_type = 0;

  if (!config_iface_type)
    {
      static const GTypeInfo config_iface_info =
      {
        sizeof (GimpConfigInterface),
	(GBaseInitFunc)     gimp_config_iface_init,
	(GBaseFinalizeFunc) NULL,
      };

      config_iface_type = g_type_register_static (G_TYPE_INTERFACE, 
                                                  "GimpConfigInterface", 
                                                  &config_iface_info, 
                                                  0);

      g_type_interface_add_prerequisite (config_iface_type, 
                                         G_TYPE_OBJECT);
    }
  
  return config_iface_type;
}

static void
gimp_config_iface_init (GimpConfigInterface *gimp_config_iface)
{
  gimp_config_iface->serialize   = gimp_config_iface_serialize;
  gimp_config_iface->deserialize = gimp_config_iface_deserialize;
  gimp_config_iface->duplicate   = gimp_config_iface_duplicate;
  gimp_config_iface->equal       = gimp_config_iface_equal;
}

static void
gimp_config_iface_serialize (GObject *object,
                             gint     fd)
{
  gimp_config_serialize_properties (object, fd);
}

static gboolean
gimp_config_iface_deserialize (GObject  *object,
                               GScanner *scanner)
{
  return gimp_config_deserialize_properties (object, scanner, FALSE);
}

static GObject *
gimp_config_iface_duplicate (GObject *object)
{
  GObject *dup = g_object_new (G_TYPE_FROM_INSTANCE (object), NULL);

  gimp_config_copy_properties (object, dup);

  return dup;
}

static gboolean
gimp_config_iface_equal (GObject *a,
                         GObject *b)
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
      GValue       a_value = { 0, };
      GValue       b_value = { 0, };

      prop_spec = property_specs[i];

      if (! (prop_spec->flags & G_PARAM_READABLE))
        continue;

      g_value_init (&a_value, prop_spec->value_type);
      g_value_init (&b_value, prop_spec->value_type);
      g_object_get_property (a, prop_spec->name, &a_value);
      g_object_get_property (b, prop_spec->name, &b_value);

      equal = gimp_config_values_equal (&a_value, &b_value);

      g_value_unset (&a_value);
      g_value_unset (&b_value);
    }

  g_free (property_specs);

  return equal;
}

/**
 * gimp_config_serialize:
 * @object: a #GObject that implements the #GimpConfigInterface.
 * @filename: the name of the file to write the configuration to.
 * @error:
 * 
 * Serializes the object properties of @object to the file specified
 * by @filename. If a file with that name already exists, it is 
 * overwritten. Basically this function opens @filename for you and
 * calls the serialize function of the @object's #GimpConfigInterface.
 *
 * Return value: %TRUE if serialization succeeded, %FALSE otherwise.
 **/
gboolean
gimp_config_serialize (GObject      *object,
                       const gchar  *filename,
                       GError      **error)
{
  GimpConfigInterface *gimp_config_iface;
  gint                 fd;

  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  gimp_config_iface = GIMP_GET_CONFIG_INTERFACE (object);

  g_return_val_if_fail (gimp_config_iface != NULL, FALSE);

  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 
#ifndef G_OS_WIN32
             S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#else
             _S_IREAD | _S_IWRITE
#endif
             );

  if (fd == -1)
    {
      g_set_error (error, 
                   GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_FILE, 
                   _("Failed to open file: '%s': %s"),
                   filename, g_strerror (errno));
      return FALSE;
    }

  gimp_config_iface->serialize (object, fd);

  close (fd);

  return TRUE;
}

/**
 * gimp_config_deserialize:
 * @object: a #GObject that implements the #GimpConfigInterface.
 * @filename: the name of the file to read configuration from.
 * @error: 
 * 
 * Opens the file specified by @filename, reads configuration data
 * from it and configures @object accordingly. Basically this function
 * creates a properly configured #GScanner for you and calls the
 * deserialize function of the @object's #GimpConfigInterface.
 * 
 * Return value: %TRUE if deserialization succeeded, %FALSE otherwise. 
 **/
gboolean
gimp_config_deserialize (GObject      *object,
                         const gchar  *filename,
                         GError      **error)
{
  GimpConfigInterface *gimp_config_iface;
  gint                 fd;
  GScanner            *scanner;
  gboolean             success;

  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  
  gimp_config_iface = GIMP_GET_CONFIG_INTERFACE (object);

  g_return_val_if_fail (gimp_config_iface != NULL, FALSE);

  fd = open (filename, O_RDONLY);

  if (fd == -1)
    {
      g_set_error (error,
                   GIMP_CONFIG_ERROR, 
                   (errno == ENOENT ?
                    GIMP_CONFIG_ERROR_FILE_ENOENT : GIMP_CONFIG_ERROR_FILE),
                   _("Failed to open file: '%s': %s"),
                   filename, g_strerror (errno));
      return FALSE;
    }

  scanner = g_scanner_new (NULL);

  scanner->user_data = error;
  scanner->msg_handler = gimp_config_scanner_message;

  scanner->config->cset_identifier_first = ( G_CSET_a_2_z );
  scanner->config->cset_identifier_nth   = ( G_CSET_a_2_z "-_" );

  g_scanner_input_file (scanner, fd);
  scanner->input_name = filename;

  success = gimp_config_iface->deserialize (object, scanner);

  g_scanner_destroy (scanner);
  close (fd);

  return success;
}

GQuark
gimp_config_error_quark (void)
{
  static GQuark q = 0;
  if (q == 0)
    q = g_quark_from_static_string ("gimp-config-error-quark");

  return q;
}

static void
gimp_config_scanner_message (GScanner *scanner,
                             gchar    *message,
                             gboolean  is_error)
{
  GError **error = scanner->user_data;

  /* we don't expect warnings */
  g_return_if_fail (is_error);

  g_set_error (error,
               GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
               _("Error while parsing '%s' in line %d:\n %s"), 
               scanner->input_name, scanner->line, message);
}

/**
 * gimp_config_duplicate:
 * @object: a #GObject that implements the #GimpConfigInterface.
 * 
 * Creates a copy of the passed object by copying all object
 * properties. The default implementation of the #GimpConfigInterface
 * only works for objects that are completely defined by their
 * properties.
 * 
 * Return value: the duplicated #GObject.
 **/
GObject *
gimp_config_duplicate (GObject *object)
{
  GimpConfigInterface *gimp_config_iface;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);

  gimp_config_iface = GIMP_GET_CONFIG_INTERFACE (object);

  g_return_val_if_fail (gimp_config_iface != NULL, FALSE);

  return gimp_config_iface->duplicate (object);
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
 * Return value: %TRUE if the two objects are equal.
 **/
gboolean
gimp_config_is_equal_to (GObject *a,
                         GObject *b)
{
  GimpConfigInterface *gimp_config_iface;

  g_return_val_if_fail (G_IS_OBJECT (a), FALSE);
  g_return_val_if_fail (G_IS_OBJECT (b), FALSE);
  g_return_val_if_fail (G_TYPE_FROM_INSTANCE (a) == G_TYPE_FROM_INSTANCE (b),
                        FALSE);

  gimp_config_iface = GIMP_GET_CONFIG_INTERFACE (a);

  g_return_val_if_fail (gimp_config_iface != NULL, FALSE);

  return gimp_config_iface->equal (a, b);
}


/* 
 * Code to store and lookup unknown tokens (string key/value pairs).
 */

#define GIMP_CONFIG_UNKNOWN_TOKENS "gimp-config-unknown-tokens"

typedef struct
{
  gchar *key;
  gchar *value;
} GimpConfigToken;

static void  gimp_config_destroy_unknown_tokens (GSList   *unknown_tokens);


/**
 * gimp_config_add_unknown_token:
 * @object: a #GObject.
 * @key: a nul-terminated string to identify the value.
 * @value: a nul-terminated string representing the value.
 * 
 * This function allows to add arbitrary key/value pairs to a GObject.
 * It's purpose is to attach additional data to a #GimpConfig object
 * that can be stored along with the object properties when
 * serializing the object to a configuration file. Please note however
 * that the default gimp_config_serialize() implementation does not
 * serialize unknown tokens.
 *
 * If you want to remove a key/value pair from the object, call this
 * function with a %NULL @value. 
 **/
void
gimp_config_add_unknown_token (GObject     *object,
                               const gchar *key,
                               const gchar *value)
{
  GimpConfigToken *token;
  GSList          *unknown_tokens;
  GSList          *last;
  GSList          *list;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  unknown_tokens = (GSList *) g_object_get_data (object, 
                                                 GIMP_CONFIG_UNKNOWN_TOKENS);

  for (last = NULL, list = unknown_tokens; 
       list; 
       last = list, list = g_slist_next (list))
    {
      token = (GimpConfigToken *) list->data;

      if (strcmp (token->key, key) == 0)
        {
          g_free (token->value);
          if (value)
            {
              token->value = g_strdup (value);
            }
          else
            {
              g_free (token->key);

              unknown_tokens = g_slist_remove (unknown_tokens, token);
              g_object_set_data_full (object, GIMP_CONFIG_UNKNOWN_TOKENS,
                                      unknown_tokens, 
                     (GDestroyNotify) gimp_config_destroy_unknown_tokens);
            }
          return;
        }
    }

  token = g_new (GimpConfigToken, 1);
  token->key   = g_strdup (key);
  token->value = g_strdup (value); 

  if (last)
    {
      g_slist_append (last, token);
    }
  else
    {
      unknown_tokens = g_slist_append (NULL, token);
 
      g_object_set_data_full (object, GIMP_CONFIG_UNKNOWN_TOKENS,
                              unknown_tokens, 
             (GDestroyNotify) gimp_config_destroy_unknown_tokens);
    }
}

/**
 * gimp_config_lookup_unknown_token:
 * @object: a #GObject.
 * @key: a nul-terminated string to identify the value.
 * 
 * This function retrieves data that was previously attached using
 * gimp_config_add_unknown_token(). You should not free or modify
 * the returned string.
 **/
const gchar *
gimp_config_lookup_unknown_token (GObject     *object,
                                  const gchar *key)
{
  GimpConfigToken *token;
  GSList          *unknown_tokens;
  GSList          *list;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (key != NULL, NULL);
  
  unknown_tokens = (GSList *) g_object_get_data (object, 
                                                 GIMP_CONFIG_UNKNOWN_TOKENS);
  
  for (list = unknown_tokens; list; list = g_slist_next (list))
    {
      token = (GimpConfigToken *) list->data;

      if (strcmp (token->key, key) == 0)
        return token->value;
    }

  return NULL;
}

/**
 * gimp_config_foreach_unknown_token:
 * @object: a #GObject.
 * @func: a function to call for each key/value pair.
 * @user_data: data to pass to @func.
 * 
 * Calls @func for each key/value stored with the @object using
 * gimp_config_add_unknown_token().
 **/
void
gimp_config_foreach_unknown_token (GObject               *object,
                                   GimpConfigForeachFunc  func,
                                   gpointer               user_data)
{
  GimpConfigToken *token;
  GSList          *unknown_tokens;
  GSList          *list;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (func != NULL);
  
  unknown_tokens = (GSList *) g_object_get_data (object, 
                                                 GIMP_CONFIG_UNKNOWN_TOKENS);

  for (list = unknown_tokens; list; list = g_slist_next (list))
    {
      token = (GimpConfigToken *) list->data;
      
      func (token->key, token->value, user_data);
    }  
}

static void
gimp_config_destroy_unknown_tokens (GSList *unknown_tokens)
{
  GimpConfigToken *token;
  GSList          *list;

  for (list = unknown_tokens; list; list = g_slist_next (list))
    {
      token = (GimpConfigToken *) list->data;
      
      g_free (token->key);
      g_free (token->value);
      g_free (token);
    }
  
  g_slist_free (unknown_tokens);
}
