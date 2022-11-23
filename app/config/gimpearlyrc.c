/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaEarlyRc: pre-parsing of ligmarc suitable for use during early
 * initialization, when the ligma singleton is not constructed yet
 *
 * Copyright (C) 2017  Jehan <jehan@ligma.org>
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

#include <gio/gio.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "config-types.h"

#include "core/core-types.h"
#include "core/ligma-utils.h"

#include "ligmaearlyrc.h"

enum
{
  PROP_0,
  PROP_VERBOSE,
  PROP_SYSTEM_LIGMARC,
  PROP_USER_LIGMARC,
  PROP_LANGUAGE,
#ifdef G_OS_WIN32
  PROP_WIN32_POINTER_INPUT_API,
#endif
};


static void         ligma_early_rc_constructed       (GObject      *object);
static void         ligma_early_rc_finalize          (GObject      *object);
static void         ligma_early_rc_set_property      (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void         ligma_early_rc_get_property      (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);


/* Just use LigmaConfig interface's default implementation which will
 * fill the properties. */
G_DEFINE_TYPE_WITH_CODE (LigmaEarlyRc, ligma_early_rc, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG, NULL))

#define parent_class ligma_early_rc_parent_class


static void
ligma_early_rc_class_init (LigmaEarlyRcClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_early_rc_constructed;
  object_class->finalize     = ligma_early_rc_finalize;
  object_class->set_property = ligma_early_rc_set_property;
  object_class->get_property = ligma_early_rc_get_property;

  g_object_class_install_property (object_class, PROP_VERBOSE,
                                   g_param_spec_boolean ("verbose",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SYSTEM_LIGMARC,
                                   g_param_spec_object ("system-ligmarc",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_USER_LIGMARC,
                                   g_param_spec_object ("user-ligmarc",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  LIGMA_CONFIG_PROP_STRING (object_class, PROP_LANGUAGE,
                           "language", NULL, NULL, NULL,
                           LIGMA_PARAM_STATIC_STRINGS);

#ifdef G_OS_WIN32
  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_WIN32_POINTER_INPUT_API,
                         "win32-pointer-input-api", NULL, NULL,
                         LIGMA_TYPE_WIN32_POINTER_INPUT_API,
                         LIGMA_WIN32_POINTER_INPUT_API_WINDOWS_INK,
                         LIGMA_PARAM_STATIC_STRINGS |
                         LIGMA_CONFIG_PARAM_RESTART);
#endif
}

static void
ligma_early_rc_init (LigmaEarlyRc *rc)
{
}

static void
ligma_early_rc_constructed (GObject *object)
{
  LigmaEarlyRc *rc    = LIGMA_EARLY_RC (object);
  GError      *error = NULL;

  if (rc->verbose)
    g_print ("Parsing '%s' for configuration data required during early initialization.\n",
             ligma_file_get_utf8_name (rc->system_ligmarc));

  if (! ligma_config_deserialize_file (LIGMA_CONFIG (rc),
                                      rc->system_ligmarc, NULL, &error))
    {
      if (error->code != LIGMA_CONFIG_ERROR_OPEN_ENOENT)
        g_message ("%s", error->message);

      g_clear_error (&error);
    }

  if (rc->verbose)
    g_print ("Parsing '%s' for configuration data required during early initialization.\n",
             ligma_file_get_utf8_name (rc->user_ligmarc));

  if (! ligma_config_deserialize_file (LIGMA_CONFIG (rc),
                                      rc->user_ligmarc, NULL, &error))
    {
      if (error->code != LIGMA_CONFIG_ERROR_OPEN_ENOENT)
        g_message ("%s", error->message);

      g_clear_error (&error);
    }

  if (rc->verbose)
    {
      if (rc->language)
        g_print ("Language property found: %s.\n", rc->language);
      else
        g_print ("No language property found.\n");
    }
}

static void
ligma_early_rc_finalize (GObject *object)
{
  LigmaEarlyRc *rc = LIGMA_EARLY_RC (object);

  g_clear_object (&rc->system_ligmarc);
  g_clear_object (&rc->user_ligmarc);

  g_clear_pointer (&rc->language, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_early_rc_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  LigmaEarlyRc *rc = LIGMA_EARLY_RC (object);

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

    case PROP_LANGUAGE:
      if (rc->language)
        g_free (rc->language);
      rc->language = g_value_dup_string (value);
      break;

#ifdef G_OS_WIN32
    case PROP_WIN32_POINTER_INPUT_API:
      {
        LigmaWin32PointerInputAPI api = g_value_get_enum (value);
        gboolean have_wintab         = ligma_win32_have_wintab ();
        gboolean have_windows_ink    = ligma_win32_have_windows_ink ();
        gboolean api_is_wintab       = (api == LIGMA_WIN32_POINTER_INPUT_API_WINTAB);
        gboolean api_is_windows_ink  = (api == LIGMA_WIN32_POINTER_INPUT_API_WINDOWS_INK);

        if (api_is_wintab && !have_wintab && have_windows_ink)
          {
            rc->win32_pointer_input_api = LIGMA_WIN32_POINTER_INPUT_API_WINDOWS_INK;
          }
        else if (api_is_windows_ink && !have_windows_ink && have_wintab)
          {
            rc->win32_pointer_input_api = LIGMA_WIN32_POINTER_INPUT_API_WINTAB;
          }
        else
          {
            rc->win32_pointer_input_api = api;
          }
      }
      break;
#endif

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_early_rc_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  LigmaEarlyRc *rc = LIGMA_EARLY_RC (object);

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
    case PROP_LANGUAGE:
      g_value_set_string (value, rc->language);
      break;

#ifdef G_OS_WIN32
    case PROP_WIN32_POINTER_INPUT_API:
      g_value_set_enum (value, rc->win32_pointer_input_api);
      break;
#endif

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * ligma_early_rc_new:
 * @system_ligmarc: the name of the system-wide ligmarc file or %NULL to
 *                 use the standard location
 * @user_ligmarc:   the name of the user ligmarc file or %NULL to use the
 *                 standard location
 * @verbose:       enable console messages about loading the preferences
 *
 * Creates a new LigmaEarlyRc object which looks for some configuration
 * data required during early initialization steps
 *
 * Returns: the new #LigmaEarlyRc.
 */
LigmaEarlyRc *
ligma_early_rc_new (GFile    *system_ligmarc,
                   GFile    *user_ligmarc,
                   gboolean  verbose)
{
  LigmaEarlyRc *rc;

  g_return_val_if_fail (system_ligmarc == NULL || G_IS_FILE (system_ligmarc),
                        NULL);
  g_return_val_if_fail (user_ligmarc == NULL || G_IS_FILE (user_ligmarc),
                        NULL);

  rc = g_object_new (LIGMA_TYPE_EARLY_RC,
                     "verbose",       verbose,
                     "system-ligmarc", system_ligmarc,
                     "user-ligmarc",   user_ligmarc,
                     NULL);

  return rc;
}

/**
 * ligma_early_rc_get_language:
 * @rc: a #LigmaEarlyRc object.
 *
 * This function looks up the language set in `ligmarc`.
 *
 * Returns: a newly allocated string representing the language or
 *               %NULL if the key couldn't be found.
 **/
gchar *
ligma_early_rc_get_language (LigmaEarlyRc *rc)
{
  return rc->language ? g_strdup (rc->language) : NULL;
}

#ifdef G_OS_WIN32

/**
 * ligma_early_rc_get_win32_pointer_input_api:
 * @rc: a #LigmaEarlyRc object.
 *
 * This function looks up the win32-specific pointer input API
 * set in `ligmarc`.
 *
 * Returns: the selected win32-specific pointer input API
 **/
LigmaWin32PointerInputAPI
ligma_early_rc_get_win32_pointer_input_api (LigmaEarlyRc *rc)
{
  return rc->win32_pointer_input_api;
}

#endif
