/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpLangRc: pre-parsing of gimprc returning the language.
 * Copyright (C) 2017  Jehan <jehan@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "config-types.h"

#include "gimplangrc.h"

enum
{
  PROP_0,
  PROP_VERBOSE,
  PROP_SYSTEM_GIMPRC,
  PROP_USER_GIMPRC,
  PROP_LANGUAGE
};


static void         gimp_lang_rc_constructed       (GObject      *object);
static void         gimp_lang_rc_finalize          (GObject      *object);
static void         gimp_lang_rc_set_property      (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);
static void         gimp_lang_rc_get_property      (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);


/* Just use GimpConfig interface's default implementation which will
 * fill the PROP_LANGUAGE property. */
G_DEFINE_TYPE_WITH_CODE (GimpLangRc, gimp_lang_rc, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

#define parent_class gimp_lang_rc_parent_class


static void
gimp_lang_rc_class_init (GimpLangRcClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_lang_rc_constructed;
  object_class->finalize     = gimp_lang_rc_finalize;
  object_class->set_property = gimp_lang_rc_set_property;
  object_class->get_property = gimp_lang_rc_get_property;

  g_object_class_install_property (object_class, PROP_VERBOSE,
                                   g_param_spec_boolean ("verbose",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SYSTEM_GIMPRC,
                                   g_param_spec_object ("system-gimprc",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_USER_GIMPRC,
                                   g_param_spec_object ("user-gimprc",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  GIMP_CONFIG_PROP_STRING (object_class, PROP_LANGUAGE,
                           "language", NULL, NULL, NULL,
                           GIMP_PARAM_STATIC_STRINGS);

}

static void
gimp_lang_rc_init (GimpLangRc *rc)
{
}

static void
gimp_lang_rc_constructed (GObject *object)
{
  GimpLangRc *rc = GIMP_LANG_RC (object);
  GError     *error = NULL;

  if (rc->verbose)
    g_print ("Parsing '%s' for configured language.\n",
             gimp_file_get_utf8_name (rc->system_gimprc));

  if (! gimp_config_deserialize_gfile (GIMP_CONFIG (rc),
                                       rc->system_gimprc, NULL, &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        g_message ("%s", error->message);

      g_clear_error (&error);
    }

  if (rc->verbose)
    g_print ("Parsing '%s' for configured language.\n",
             gimp_file_get_utf8_name (rc->user_gimprc));

  if (! gimp_config_deserialize_gfile (GIMP_CONFIG (rc),
                                       rc->user_gimprc, NULL, &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
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
gimp_lang_rc_finalize (GObject *object)
{
  GimpLangRc *rc = GIMP_LANG_RC (object);

  g_clear_object (&rc->system_gimprc);
  g_clear_object (&rc->user_gimprc);

  g_clear_pointer (&rc->language, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_lang_rc_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpLangRc *rc = GIMP_LANG_RC (object);

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

    case PROP_LANGUAGE:
      if (rc->language)
        g_free (rc->language);
      rc->language = g_value_dup_string (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_lang_rc_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpLangRc *rc = GIMP_LANG_RC (object);

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
    case PROP_LANGUAGE:
      g_value_set_string (value, rc->language);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_lang_rc_new:
 * @system_gimprc: the name of the system-wide gimprc file or %NULL to
 *                 use the standard location
 * @user_gimprc:   the name of the user gimprc file or %NULL to use the
 *                 standard location
 * @verbose:       enable console messages about loading the language
 *
 * Creates a new GimpLangRc object which only looks for the configure
 * language.
 *
 * Returns: the new #GimpLangRc.
 */
GimpLangRc *
gimp_lang_rc_new (GFile    *system_gimprc,
                  GFile    *user_gimprc,
                  gboolean  verbose)
{
  GimpLangRc *rc;

  g_return_val_if_fail (system_gimprc == NULL || G_IS_FILE (system_gimprc),
                        NULL);
  g_return_val_if_fail (user_gimprc == NULL || G_IS_FILE (user_gimprc),
                        NULL);

  rc = g_object_new (GIMP_TYPE_LANG_RC,
                     "verbose",       verbose,
                     "system-gimprc", system_gimprc,
                     "user-gimprc",   user_gimprc,
                     NULL);

  return rc;
}

/**
 * gimp_lang_rc_get_language:
 * @lang_rc:  a #GimpLangRc object.
 *
 * This function looks up the language set in `gimprc`.
 *
 * Return value: a newly allocated string representing the language or
 *               %NULL if the key couldn't be found.
 **/
gchar *
gimp_lang_rc_get_language (GimpLangRc *rc)
{
  return rc->language ? g_strdup (rc->language) : NULL;
}
