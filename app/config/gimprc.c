/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpRc, the object for GIMPs user configuration file gimprc.
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "config-types.h"

#include "gimpconfig-file.h"
#include "gimprc.h"
#include "gimprc-deserialize.h"
#include "gimprc-serialize.h"
#include "gimprc-unknown.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_VERBOSE,
  PROP_SYSTEM_GIMPRC,
  PROP_USER_GIMPRC
};


static void         gimp_rc_config_iface_init (GimpConfigInterface *iface);

static void         gimp_rc_dispose           (GObject      *object);
static void         gimp_rc_finalize          (GObject      *object);
static void         gimp_rc_set_property      (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void         gimp_rc_get_property      (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);

static GimpConfig * gimp_rc_duplicate         (GimpConfig   *object);
static gboolean     gimp_rc_idle_save         (GimpRc       *rc);
static void         gimp_rc_notify            (GimpRc       *rc,
                                               GParamSpec   *param,
                                               gpointer      data);


G_DEFINE_TYPE_WITH_CODE (GimpRc, gimp_rc, GIMP_TYPE_PLUGIN_CONFIG,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_rc_config_iface_init))

#define parent_class gimp_rc_parent_class


static void
gimp_rc_class_init (GimpRcClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gimp_rc_dispose;
  object_class->finalize     = gimp_rc_finalize;
  object_class->set_property = gimp_rc_set_property;
  object_class->get_property = gimp_rc_get_property;

  g_object_class_install_property (object_class, PROP_VERBOSE,
                                   g_param_spec_boolean ("verbose",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SYSTEM_GIMPRC,
                                   g_param_spec_object ("system-gimprc",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USER_GIMPRC,
                                   g_param_spec_object ("user-gimprc",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_rc_init (GimpRc *rc)
{
  rc->autosave     = FALSE;
  rc->save_idle_id = 0;
}

static void
gimp_rc_dispose (GObject *object)
{
  GimpRc *rc = GIMP_RC (object);

  if (rc->save_idle_id)
    gimp_rc_idle_save (rc);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_rc_finalize (GObject *object)
{
  GimpRc *rc = GIMP_RC (object);

  g_clear_object (&rc->system_gimprc);
  g_clear_object (&rc->user_gimprc);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_rc_set_property (GObject      *object,
                      guint         property_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  GimpRc *rc = GIMP_RC (object);

  switch (property_id)
    {
    case PROP_VERBOSE:
      rc->verbose = g_value_get_boolean (value);
      break;

    case PROP_SYSTEM_GIMPRC:
      if (rc->system_gimprc)
        g_object_unref (rc->system_gimprc);

      if (g_value_get_object (value))
        rc->system_gimprc = g_value_dup_object (value);
      else
        rc->system_gimprc = gimp_sysconf_directory_file ("gimprc", NULL);
      break;

    case PROP_USER_GIMPRC:
      if (rc->user_gimprc)
        g_object_unref (rc->user_gimprc);

      if (g_value_get_object (value))
        rc->user_gimprc = g_value_dup_object (value);
      else
        rc->user_gimprc = gimp_directory_file ("gimprc", NULL);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_rc_get_property (GObject    *object,
                      guint       property_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  GimpRc *rc = GIMP_RC (object);

  switch (property_id)
    {
    case PROP_VERBOSE:
      g_value_set_boolean (value, rc->verbose);
      break;
    case PROP_SYSTEM_GIMPRC:
      g_value_set_object (value, rc->system_gimprc);
      break;
    case PROP_USER_GIMPRC:
      g_value_set_object (value, rc->user_gimprc);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_rc_config_iface_init (GimpConfigInterface *iface)
{
  iface->serialize   = gimp_rc_serialize;
  iface->deserialize = gimp_rc_deserialize;
  iface->duplicate   = gimp_rc_duplicate;
}

static void
gimp_rc_duplicate_unknown_token (const gchar *key,
                                 const gchar *value,
                                 gpointer     user_data)
{
  gimp_rc_add_unknown_token (GIMP_CONFIG (user_data), key, value);
}

static GimpConfig *
gimp_rc_duplicate (GimpConfig *config)
{
  GObject    *gimp;
  GimpConfig *dup;

  g_object_get (config, "gimp", &gimp, NULL);

  dup = g_object_new (GIMP_TYPE_RC,
                      "gimp", gimp,
                      NULL);

  if (gimp)
    g_object_unref (gimp);

  gimp_config_sync (G_OBJECT (config), G_OBJECT (dup), 0);

  gimp_rc_foreach_unknown_token (config,
                                 gimp_rc_duplicate_unknown_token, dup);

  return dup;
}

static gboolean
gimp_rc_idle_save (GimpRc *rc)
{
  gimp_rc_save (rc);

  rc->save_idle_id = 0;

  return FALSE;
}

static void
gimp_rc_notify (GimpRc     *rc,
                GParamSpec *param,
                gpointer    data)
{
  if (!rc->autosave)
    return;

  if (!rc->save_idle_id)
    rc->save_idle_id = g_idle_add ((GSourceFunc) gimp_rc_idle_save, rc);
}

/**
 * gimp_rc_new:
 * @gimp:          a #Gimp
 * @system_gimprc: the name of the system-wide gimprc file or %NULL to
 *                 use the standard location
 * @user_gimprc:   the name of the user gimprc file or %NULL to use the
 *                 standard location
 * @verbose:       enable console messages about loading and saving
 *
 * Creates a new GimpRc object and loads the system-wide and the user
 * configuration files.
 *
 * Returns: the new #GimpRc.
 */
GimpRc *
gimp_rc_new (GObject  *gimp,
             GFile    *system_gimprc,
             GFile    *user_gimprc,
             gboolean  verbose)
{
  GimpRc *rc;

  g_return_val_if_fail (G_IS_OBJECT (gimp), NULL);
  g_return_val_if_fail (system_gimprc == NULL || G_IS_FILE (system_gimprc),
                        NULL);
  g_return_val_if_fail (user_gimprc == NULL || G_IS_FILE (user_gimprc),
                        NULL);

  rc = g_object_new (GIMP_TYPE_RC,
                     "gimp",          gimp,
                     "verbose",       verbose,
                     "system-gimprc", system_gimprc,
                     "user-gimprc",   user_gimprc,
                     NULL);

  gimp_rc_load_system (rc);
  gimp_rc_load_user (rc);

  return rc;
}

void
gimp_rc_load_system (GimpRc *rc)
{
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_RC (rc));

  if (rc->verbose)
    g_print ("Parsing '%s'\n",
             gimp_file_get_utf8_name (rc->system_gimprc));

  if (! gimp_config_deserialize_file (GIMP_CONFIG (rc),
                                      rc->system_gimprc, NULL, &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        g_message ("%s", error->message);

      g_clear_error (&error);
    }
}

void
gimp_rc_load_user (GimpRc *rc)
{
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_RC (rc));

  if (rc->verbose)
    g_print ("Parsing '%s'\n",
             gimp_file_get_utf8_name (rc->user_gimprc));

  if (! gimp_config_deserialize_file (GIMP_CONFIG (rc),
                                      rc->user_gimprc, NULL, &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_message ("%s", error->message);

          gimp_config_file_backup_on_error (rc->user_gimprc, "gimprc", NULL);
        }

      g_clear_error (&error);
    }
}

void
gimp_rc_set_autosave (GimpRc   *rc,
                      gboolean  autosave)
{
  g_return_if_fail (GIMP_IS_RC (rc));

  autosave = autosave ? TRUE : FALSE;

  if (rc->autosave == autosave)
    return;

  if (autosave)
    g_signal_connect (rc, "notify",
                      G_CALLBACK (gimp_rc_notify),
                      NULL);
  else
    g_signal_handlers_disconnect_by_func (rc, gimp_rc_notify, NULL);

  rc->autosave = autosave;
}


/**
 * gimp_rc_query:
 * @rc:  a #GimpRc object.
 * @key: a string used as a key for the lookup.
 *
 * This function looks up @key in the object properties of @rc. If
 * there's a matching property, a string representation of its value
 * is returned. If no property is found, the list of unknown tokens
 * attached to the @rc object is searched.
 *
 * Returns: (nullable): a newly allocated string representing the value or %NULL
 *               if the key couldn't be found.
 **/
gchar *
gimp_rc_query (GimpRc      *rc,
               const gchar *key)
{
  GObjectClass  *klass;
  GObject       *rc_object;
  GParamSpec   **property_specs;
  GParamSpec    *prop_spec;
  guint          i, n_property_specs;
  gchar         *retval = NULL;

  g_return_val_if_fail (GIMP_IS_RC (rc), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  rc_object = G_OBJECT (rc);
  klass = G_OBJECT_GET_CLASS (rc);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (!property_specs)
    return NULL;

  for (i = 0, prop_spec = NULL; i < n_property_specs && !prop_spec; i++)
    {
      prop_spec = property_specs[i];

      if (! (prop_spec->flags & GIMP_CONFIG_PARAM_SERIALIZE) ||
          strcmp (prop_spec->name, key))
        {
          prop_spec = NULL;
        }
    }

  if (prop_spec)
    {
      GString *str   = g_string_new (NULL);
      GValue   value = G_VALUE_INIT;

      g_value_init (&value, prop_spec->value_type);
      g_object_get_property (rc_object, prop_spec->name, &value);

      if (gimp_config_serialize_value (&value, str, FALSE))
        retval = g_string_free (str, FALSE);
      else
        g_string_free (str, TRUE);

      g_value_unset (&value);
    }
  else
    {
      retval = g_strdup (gimp_rc_lookup_unknown_token (GIMP_CONFIG (rc), key));
    }

  g_free (property_specs);

  if (!retval)
    {
      const gchar * const path_tokens[] =
      {
        "gimp_dir",
        "gimp_data_dir",
        "gimp_plug_in_dir",
        "gimp_plugin_dir",
        "gimp_sysconf_dir"
      };

      for (i = 0; !retval && i < G_N_ELEMENTS (path_tokens); i++)
        if (strcmp (key, path_tokens[i]) == 0)
          retval = g_strdup_printf ("${%s}", path_tokens[i]);
    }

  if (retval)
    {
      gchar *tmp = gimp_config_path_expand (retval, FALSE, NULL);

      if (tmp)
        {
          g_free (retval);
          retval = tmp;
        }
    }

  return retval;
}

/**
 * gimp_rc_set_unknown_token:
 * @gimprc: a #GimpRc object.
 * @token:
 * @value:
 *
 * Calls gimp_rc_add_unknown_token() and triggers an idle-save if
 * autosave is enabled on @gimprc.
 **/
void
gimp_rc_set_unknown_token (GimpRc      *rc,
                           const gchar *token,
                           const gchar *value)
{
  g_return_if_fail (GIMP_IS_RC (rc));

  gimp_rc_add_unknown_token (GIMP_CONFIG (rc), token, value);

  if (rc->autosave)
    gimp_rc_notify (rc, NULL, NULL);
}

/**
 * gimp_rc_save:
 * @gimprc: a #GimpRc object.
 *
 * Saves any settings that differ from the system-wide defined
 * defaults to the users personal gimprc file.
 **/
void
gimp_rc_save (GimpRc *rc)
{
  GObject *gimp;
  GimpRc  *global;
  gchar   *header;
  GError  *error = NULL;

  const gchar *top =
    "GIMP gimprc\n"
    "\n"
    "This is your personal gimprc file.  Any variable defined in this file "
    "takes precedence over the value defined in the system-wide gimprc: ";
  const gchar *bottom =
    "\n"
    "Most values can be set within GIMP by changing some options in "
    "the Preferences dialog.";
  const gchar *footer =
    "end of gimprc";

  g_return_if_fail (GIMP_IS_RC (rc));

  g_object_get (rc, "gimp", &gimp, NULL);

  global = g_object_new (GIMP_TYPE_RC,
                         "gimp", gimp,
                         NULL);

  if (gimp)
    g_object_unref (gimp);

  gimp_config_deserialize_file (GIMP_CONFIG (global),
                                rc->system_gimprc, NULL, NULL);

  header = g_strconcat (top, gimp_file_get_utf8_name (rc->system_gimprc),
                        bottom, NULL);

  if (rc->verbose)
    g_print ("Writing '%s'\n",
             gimp_file_get_utf8_name (rc->user_gimprc));

  if (! gimp_config_serialize_to_file (GIMP_CONFIG (rc),
                                       rc->user_gimprc,
                                       header, footer, global,
                                       &error))
    {
      g_message ("%s", error->message);
      g_error_free (error);
    }

  g_free (header);
  g_object_unref (global);
}

/**
 * gimp_rc_migrate:
 * @rc: a #GimpRc object.
 *
 * Resets all GimpParamConfigPath properties of the passed rc object
 * to their default values, in order to prevent paths in a migrated
 * gimprc to refer to folders in the old GIMP's user directory.
 **/
void
gimp_rc_migrate (GimpRc *rc)
{
  GParamSpec **pspecs;
  guint        n_pspecs;
  gint         i;

  g_return_if_fail (GIMP_IS_RC (rc));

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (rc), &n_pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];

      if (GIMP_IS_PARAM_SPEC_CONFIG_PATH (pspec))
        {
          GValue value = G_VALUE_INIT;

          g_value_init (&value, pspec->value_type);

          g_param_value_set_default (pspec, &value);
          g_object_set_property (G_OBJECT (rc), pspec->name, &value);

          g_value_unset (&value);
        }
    }

  g_free (pspecs);
}
