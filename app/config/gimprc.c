/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaRc, the object for LIGMAs user configuration file ligmarc.
 * Copyright (C) 2001-2002  Sven Neumann <sven@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "config-types.h"

#include "ligmaconfig-file.h"
#include "ligmarc.h"
#include "ligmarc-deserialize.h"
#include "ligmarc-serialize.h"
#include "ligmarc-unknown.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_VERBOSE,
  PROP_SYSTEM_LIGMARC,
  PROP_USER_LIGMARC
};


static void         ligma_rc_config_iface_init (LigmaConfigInterface *iface);

static void         ligma_rc_dispose           (GObject      *object);
static void         ligma_rc_finalize          (GObject      *object);
static void         ligma_rc_set_property      (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void         ligma_rc_get_property      (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);

static LigmaConfig * ligma_rc_duplicate         (LigmaConfig   *object);
static gboolean     ligma_rc_idle_save         (LigmaRc       *rc);
static void         ligma_rc_notify            (LigmaRc       *rc,
                                               GParamSpec   *param,
                                               gpointer      data);


G_DEFINE_TYPE_WITH_CODE (LigmaRc, ligma_rc, LIGMA_TYPE_PLUGIN_CONFIG,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_rc_config_iface_init))

#define parent_class ligma_rc_parent_class


static void
ligma_rc_class_init (LigmaRcClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = ligma_rc_dispose;
  object_class->finalize     = ligma_rc_finalize;
  object_class->set_property = ligma_rc_set_property;
  object_class->get_property = ligma_rc_get_property;

  g_object_class_install_property (object_class, PROP_VERBOSE,
                                   g_param_spec_boolean ("verbose",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SYSTEM_LIGMARC,
                                   g_param_spec_object ("system-ligmarc",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USER_LIGMARC,
                                   g_param_spec_object ("user-ligmarc",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_rc_init (LigmaRc *rc)
{
  rc->autosave     = FALSE;
  rc->save_idle_id = 0;
}

static void
ligma_rc_dispose (GObject *object)
{
  LigmaRc *rc = LIGMA_RC (object);

  if (rc->save_idle_id)
    ligma_rc_idle_save (rc);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_rc_finalize (GObject *object)
{
  LigmaRc *rc = LIGMA_RC (object);

  g_clear_object (&rc->system_ligmarc);
  g_clear_object (&rc->user_ligmarc);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_rc_set_property (GObject      *object,
                      guint         property_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  LigmaRc *rc = LIGMA_RC (object);

  switch (property_id)
    {
    case PROP_VERBOSE:
      rc->verbose = g_value_get_boolean (value);
      break;

    case PROP_SYSTEM_LIGMARC:
      if (rc->system_ligmarc)
        g_object_unref (rc->system_ligmarc);

      if (g_value_get_object (value))
        rc->system_ligmarc = g_value_dup_object (value);
      else
        rc->system_ligmarc = ligma_sysconf_directory_file ("ligmarc", NULL);
      break;

    case PROP_USER_LIGMARC:
      if (rc->user_ligmarc)
        g_object_unref (rc->user_ligmarc);

      if (g_value_get_object (value))
        rc->user_ligmarc = g_value_dup_object (value);
      else
        rc->user_ligmarc = ligma_directory_file ("ligmarc", NULL);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_rc_get_property (GObject    *object,
                      guint       property_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  LigmaRc *rc = LIGMA_RC (object);

  switch (property_id)
    {
    case PROP_VERBOSE:
      g_value_set_boolean (value, rc->verbose);
      break;
    case PROP_SYSTEM_LIGMARC:
      g_value_set_object (value, rc->system_ligmarc);
      break;
    case PROP_USER_LIGMARC:
      g_value_set_object (value, rc->user_ligmarc);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_rc_config_iface_init (LigmaConfigInterface *iface)
{
  iface->serialize   = ligma_rc_serialize;
  iface->deserialize = ligma_rc_deserialize;
  iface->duplicate   = ligma_rc_duplicate;
}

static void
ligma_rc_duplicate_unknown_token (const gchar *key,
                                 const gchar *value,
                                 gpointer     user_data)
{
  ligma_rc_add_unknown_token (LIGMA_CONFIG (user_data), key, value);
}

static LigmaConfig *
ligma_rc_duplicate (LigmaConfig *config)
{
  GObject    *ligma;
  LigmaConfig *dup;

  g_object_get (config, "ligma", &ligma, NULL);

  dup = g_object_new (LIGMA_TYPE_RC,
                      "ligma", ligma,
                      NULL);

  if (ligma)
    g_object_unref (ligma);

  ligma_config_sync (G_OBJECT (config), G_OBJECT (dup), 0);

  ligma_rc_foreach_unknown_token (config,
                                 ligma_rc_duplicate_unknown_token, dup);

  return dup;
}

static gboolean
ligma_rc_idle_save (LigmaRc *rc)
{
  ligma_rc_save (rc);

  rc->save_idle_id = 0;

  return FALSE;
}

static void
ligma_rc_notify (LigmaRc     *rc,
                GParamSpec *param,
                gpointer    data)
{
  if (!rc->autosave)
    return;

  if (!rc->save_idle_id)
    rc->save_idle_id = g_idle_add ((GSourceFunc) ligma_rc_idle_save, rc);
}

/**
 * ligma_rc_new:
 * @ligma:          a #Ligma
 * @system_ligmarc: the name of the system-wide ligmarc file or %NULL to
 *                 use the standard location
 * @user_ligmarc:   the name of the user ligmarc file or %NULL to use the
 *                 standard location
 * @verbose:       enable console messages about loading and saving
 *
 * Creates a new LigmaRc object and loads the system-wide and the user
 * configuration files.
 *
 * Returns: the new #LigmaRc.
 */
LigmaRc *
ligma_rc_new (GObject  *ligma,
             GFile    *system_ligmarc,
             GFile    *user_ligmarc,
             gboolean  verbose)
{
  LigmaRc *rc;

  g_return_val_if_fail (G_IS_OBJECT (ligma), NULL);
  g_return_val_if_fail (system_ligmarc == NULL || G_IS_FILE (system_ligmarc),
                        NULL);
  g_return_val_if_fail (user_ligmarc == NULL || G_IS_FILE (user_ligmarc),
                        NULL);

  rc = g_object_new (LIGMA_TYPE_RC,
                     "ligma",          ligma,
                     "verbose",       verbose,
                     "system-ligmarc", system_ligmarc,
                     "user-ligmarc",   user_ligmarc,
                     NULL);

  ligma_rc_load_system (rc);
  ligma_rc_load_user (rc);

  return rc;
}

void
ligma_rc_load_system (LigmaRc *rc)
{
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_RC (rc));

  if (rc->verbose)
    g_print ("Parsing '%s'\n",
             ligma_file_get_utf8_name (rc->system_ligmarc));

  if (! ligma_config_deserialize_file (LIGMA_CONFIG (rc),
                                      rc->system_ligmarc, NULL, &error))
    {
      if (error->code != LIGMA_CONFIG_ERROR_OPEN_ENOENT)
        g_message ("%s", error->message);

      g_clear_error (&error);
    }
}

void
ligma_rc_load_user (LigmaRc *rc)
{
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_RC (rc));

  if (rc->verbose)
    g_print ("Parsing '%s'\n",
             ligma_file_get_utf8_name (rc->user_ligmarc));

  if (! ligma_config_deserialize_file (LIGMA_CONFIG (rc),
                                      rc->user_ligmarc, NULL, &error))
    {
      if (error->code != LIGMA_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_message ("%s", error->message);

          ligma_config_file_backup_on_error (rc->user_ligmarc, "ligmarc", NULL);
        }

      g_clear_error (&error);
    }
}

void
ligma_rc_set_autosave (LigmaRc   *rc,
                      gboolean  autosave)
{
  g_return_if_fail (LIGMA_IS_RC (rc));

  autosave = autosave ? TRUE : FALSE;

  if (rc->autosave == autosave)
    return;

  if (autosave)
    g_signal_connect (rc, "notify",
                      G_CALLBACK (ligma_rc_notify),
                      NULL);
  else
    g_signal_handlers_disconnect_by_func (rc, ligma_rc_notify, NULL);

  rc->autosave = autosave;
}


/**
 * ligma_rc_query:
 * @rc:  a #LigmaRc object.
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
ligma_rc_query (LigmaRc      *rc,
               const gchar *key)
{
  GObjectClass  *klass;
  GObject       *rc_object;
  GParamSpec   **property_specs;
  GParamSpec    *prop_spec;
  guint          i, n_property_specs;
  gchar         *retval = NULL;

  g_return_val_if_fail (LIGMA_IS_RC (rc), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  rc_object = G_OBJECT (rc);
  klass = G_OBJECT_GET_CLASS (rc);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (!property_specs)
    return NULL;

  for (i = 0, prop_spec = NULL; i < n_property_specs && !prop_spec; i++)
    {
      prop_spec = property_specs[i];

      if (! (prop_spec->flags & LIGMA_CONFIG_PARAM_SERIALIZE) ||
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

      if (ligma_config_serialize_value (&value, str, FALSE))
        retval = g_string_free (str, FALSE);
      else
        g_string_free (str, TRUE);

      g_value_unset (&value);
    }
  else
    {
      retval = g_strdup (ligma_rc_lookup_unknown_token (LIGMA_CONFIG (rc), key));
    }

  g_free (property_specs);

  if (!retval)
    {
      const gchar * const path_tokens[] =
      {
        "ligma_dir",
        "ligma_data_dir",
        "ligma_plug_in_dir",
        "ligma_plugin_dir",
        "ligma_sysconf_dir"
      };

      for (i = 0; !retval && i < G_N_ELEMENTS (path_tokens); i++)
        if (strcmp (key, path_tokens[i]) == 0)
          retval = g_strdup_printf ("${%s}", path_tokens[i]);
    }

  if (retval)
    {
      gchar *tmp = ligma_config_path_expand (retval, FALSE, NULL);

      if (tmp)
        {
          g_free (retval);
          retval = tmp;
        }
    }

  return retval;
}

/**
 * ligma_rc_set_unknown_token:
 * @ligmarc: a #LigmaRc object.
 * @token:
 * @value:
 *
 * Calls ligma_rc_add_unknown_token() and triggers an idle-save if
 * autosave is enabled on @ligmarc.
 **/
void
ligma_rc_set_unknown_token (LigmaRc      *rc,
                           const gchar *token,
                           const gchar *value)
{
  g_return_if_fail (LIGMA_IS_RC (rc));

  ligma_rc_add_unknown_token (LIGMA_CONFIG (rc), token, value);

  if (rc->autosave)
    ligma_rc_notify (rc, NULL, NULL);
}

/**
 * ligma_rc_save:
 * @ligmarc: a #LigmaRc object.
 *
 * Saves any settings that differ from the system-wide defined
 * defaults to the users personal ligmarc file.
 **/
void
ligma_rc_save (LigmaRc *rc)
{
  GObject *ligma;
  LigmaRc  *global;
  gchar   *header;
  GError  *error = NULL;

  const gchar *top =
    "LIGMA ligmarc\n"
    "\n"
    "This is your personal ligmarc file.  Any variable defined in this file "
    "takes precedence over the value defined in the system-wide ligmarc: ";
  const gchar *bottom =
    "\n"
    "Most values can be set within LIGMA by changing some options in "
    "the Preferences dialog.";
  const gchar *footer =
    "end of ligmarc";

  g_return_if_fail (LIGMA_IS_RC (rc));

  g_object_get (rc, "ligma", &ligma, NULL);

  global = g_object_new (LIGMA_TYPE_RC,
                         "ligma", ligma,
                         NULL);

  if (ligma)
    g_object_unref (ligma);

  ligma_config_deserialize_file (LIGMA_CONFIG (global),
                                rc->system_ligmarc, NULL, NULL);

  header = g_strconcat (top, ligma_file_get_utf8_name (rc->system_ligmarc),
                        bottom, NULL);

  if (rc->verbose)
    g_print ("Writing '%s'\n",
             ligma_file_get_utf8_name (rc->user_ligmarc));

  if (! ligma_config_serialize_to_file (LIGMA_CONFIG (rc),
                                       rc->user_ligmarc,
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
 * ligma_rc_migrate:
 * @rc: a #LigmaRc object.
 *
 * Resets all LigmaParamConfigPath properties of the passed rc object
 * to their default values, in order to prevent paths in a migrated
 * ligmarc to refer to folders in the old LIGMA's user directory.
 **/
void
ligma_rc_migrate (LigmaRc *rc)
{
  GParamSpec **pspecs;
  guint        n_pspecs;
  gint         i;

  g_return_if_fail (LIGMA_IS_RC (rc));

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (rc), &n_pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];

      if (LIGMA_IS_PARAM_SPEC_CONFIG_PATH (pspec))
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
