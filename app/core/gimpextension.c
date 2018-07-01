/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpextension.c
 * Copyright (C) 2018 Jehan <jehan@girinstud.io>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <appstream-glib.h>
#include <gegl.h>

#include "core-types.h"

#include "gimpextension.h"
#include "gimpextension-error.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_PATH,
  PROP_WRITABLE
};

struct _GimpExtensionPrivate
{
  gchar    *path;

  AsApp    *app;
  gboolean  writable;
};


static void         gimp_extension_finalize        (GObject      *object);
static void         gimp_extension_set_property    (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);
static void         gimp_extension_get_property    (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);


G_DEFINE_TYPE (GimpExtension, gimp_extension, GIMP_TYPE_OBJECT)

#define parent_class gimp_extension_parent_class


static void
gimp_extension_class_init (GimpExtensionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_extension_finalize;
  object_class->set_property = gimp_extension_set_property;
  object_class->get_property = gimp_extension_get_property;

  g_object_class_install_property (object_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        NULL, NULL, NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_WRITABLE,
                                   g_param_spec_boolean ("writable",
                                                         NULL, NULL, FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (GimpExtensionPrivate));
}

static void
gimp_extension_init (GimpExtension *extension)
{
  extension->p = G_TYPE_INSTANCE_GET_PRIVATE (extension,
                                              GIMP_TYPE_EXTENSION,
                                              GimpExtensionPrivate);
}

static void
gimp_extension_finalize (GObject *object)
{
  GimpExtension *extension = GIMP_EXTENSION (object);

  g_free (extension->p->path);
  if (extension->p->app)
    g_object_unref (extension->p->app);
}

static void
gimp_extension_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpExtension *extension = GIMP_EXTENSION (object);

  switch (property_id)
    {
    case PROP_PATH:
      g_free (extension->p->path);
      extension->p->path = g_value_dup_string (value);
      gimp_object_take_name (GIMP_OBJECT (object),
                             g_path_get_basename (extension->p->path));
      break;
    case PROP_WRITABLE:
      extension->p->writable = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_extension_get_property (GObject      *object,
                             guint         property_id,
                             GValue       *value,
                             GParamSpec   *pspec)
{
  GimpExtension *extension = GIMP_EXTENSION (object);

  switch (property_id)
    {
    case PROP_PATH:
      g_value_set_string (value, extension->p->path);
      break;
    case PROP_WRITABLE:
      g_value_set_boolean (value, extension->p->writable);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/*  public functions  */

GimpExtension *
gimp_extension_new (const gchar *dir,
                    gboolean     writable)
{
  g_return_val_if_fail (dir && g_file_test (dir, G_FILE_TEST_IS_DIR), NULL);

  return g_object_new (GIMP_TYPE_EXTENSION,
                       "path",     dir,
                       "writable", writable,
                       NULL);
}

gboolean
gimp_extension_load (GimpExtension  *extension,
                     GError        **error)
{
  AsApp     *app;
  GPtrArray *extends;
  AsRelease *release;
  gchar     *appdata_name;
  gchar     *path;
  gboolean   success = FALSE;

  g_clear_object (&extension->p->app);

  /* Search in subdirectory if a file with the same name as
   * directory and ending with ".metainfo.xml" exists.
   */
  appdata_name = g_strdup_printf ("%s.metainfo.xml",
                                  gimp_object_get_name (GIMP_OBJECT (extension)));
  path = g_build_filename (extension->p->path, appdata_name, NULL);

  app = as_app_new ();
  success = as_app_parse_file (app, path,
                               AS_APP_PARSE_FLAG_USE_HEURISTICS,
                               error);
  g_free (path);
  if (success && as_app_get_kind (app) != AS_APP_KIND_ADDON)
    {
      /* Properly setting the type will allow extensions to be
       * distributed appropriately through other means.
       */
      if (error && *error == NULL)
        *error = g_error_new (GIMP_EXTENSION_ERROR,
                              GIMP_EXTENSION_BAD_APPDATA,
                              _("Extension AppData must be of type \"addon\", found \"%s\" instead."),
                              as_app_kind_to_string (as_app_get_kind (app)));
      success = FALSE;
    }

  extends = as_app_get_extends (app);
  if (success &&
      ! g_ptr_array_find_with_equal_func (extends, "org.gimp.GIMP",
                                          g_str_equal, NULL))
    {
      /* Properly setting the <extends> will allow extensions to be
       * distributed appropriately through other means.
       */
      if (error && *error == NULL)
        *error = g_error_new (GIMP_EXTENSION_ERROR,
                              GIMP_EXTENSION_BAD_APPDATA,
                              _("Extension AppData must extend \"org.gimp.GIMP\"."));
      success = FALSE;
    }

  if (success &&
      g_strcmp0 (as_app_get_id (app),
                 gimp_object_get_name (extension)) != 0)
    {
      /* Extension IDs will be unique and we want therefore the
       * installation folder to sync in order to avoid path clashes.
       */
      if (error && *error == NULL)
        *error = g_error_new (GIMP_EXTENSION_ERROR,
                              GIMP_EXTENSION_FAILED,
                              _("Extension AppData id (\"%s\") and directory (\"%s\") must be the same."),
                              as_app_get_id (app), gimp_object_get_name (extension));
      success = FALSE;
    }

  release = as_app_get_release_default (app);
  if (success && (! release || ! as_release_get_version (release)))
    {
      /* We don't need the detail, just to know that the extension has a
       * release tag with a version. This is very important since it is
       * the only way we can manage updates.
       */
      if (error && *error == NULL)
        *error = g_error_new (GIMP_EXTENSION_ERROR,
                              GIMP_EXTENSION_NO_VERSION,
                              _("Extension AppData must advertize a version in a <release> tag."));
      success = FALSE;
    }

  if (success)
    extension->p->app = app;
  else
    g_object_unref (app);

  return success;
}
