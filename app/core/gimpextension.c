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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <appstream-glib.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp-utils.h"
#include "gimperror.h"
#include "gimpextension.h"
#include "gimpextension-error.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_PATH,
  PROP_WRITABLE,
  PROP_RUNNING
};

struct _GimpExtensionPrivate
{
  gchar    *path;

  AsApp    *app;
  gboolean  writable;
  gboolean  running;

  /* Extension metadata: directories. */
  GList    *brush_paths;
  GList    *dynamics_paths;
  GList    *mypaint_brush_paths;
  GList    *pattern_paths;
  GList    *gradient_paths;
  GList    *palette_paths;
  GList    *tool_preset_paths;
  GList    *splash_paths;
  GList    *theme_paths;

  /* Extension metadata: plug-in entry points. */
  GList    *plug_in_paths;
};

typedef struct
{
  GString  *text;
  gint      level;

  gboolean  numbered_list;
  gint      list_num;
  gboolean  unnumbered_list;

  const gchar *lang;
  GString     *original;
  gint         foreign_level;
} ParseState;


static void         gimp_extension_finalize        (GObject        *object);
static void         gimp_extension_set_property    (GObject        *object,
                                                    guint           property_id,
                                                    const GValue   *value,
                                                    GParamSpec     *pspec);
static void         gimp_extension_get_property    (GObject        *object,
                                                    guint           property_id,
                                                    GValue         *value,
                                                    GParamSpec     *pspec);

static void         gimp_extension_clean           (GimpExtension  *extension);
static gint         gimp_extension_file_cmp        (GFile          *a,
                                                    GFile          *b);
static GList      * gimp_extension_validate_paths  (GimpExtension  *extension,
                                                    const gchar    *paths,
                                                    gboolean        as_directories,
                                                    GError        **error);


G_DEFINE_TYPE_WITH_PRIVATE (GimpExtension, gimp_extension, GIMP_TYPE_OBJECT)

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
  g_object_class_install_property (object_class, PROP_RUNNING,
                                   g_param_spec_boolean ("running",
                                                         NULL, NULL, FALSE,
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_extension_init (GimpExtension *extension)
{
  extension->p = gimp_extension_get_instance_private (extension);
}

static void
gimp_extension_finalize (GObject *object)
{
  GimpExtension *extension = GIMP_EXTENSION (object);

  gimp_extension_clean (extension);

  g_free (extension->p->path);
  if (extension->p->app)
    g_object_unref (extension->p->app);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
    case PROP_RUNNING:
      extension->p->running = g_value_get_boolean (value);
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
    case PROP_RUNNING:
      g_value_set_boolean (value, extension->p->running);
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

const gchar *
gimp_extension_get_name (GimpExtension *extension)
{
  g_return_val_if_fail (extension->p->app != NULL, NULL);

  return as_app_get_name (extension->p->app, g_getenv ("LANGUAGE")) ?
    as_app_get_name (extension->p->app, g_getenv ("LANGUAGE")) :
    as_app_get_name (extension->p->app, NULL);
}

const gchar *
gimp_extension_get_comment (GimpExtension *extension)
{
  g_return_val_if_fail (extension->p->app != NULL, NULL);

  return as_app_get_comment (extension->p->app, g_getenv ("LANGUAGE")) ?
    as_app_get_comment (extension->p->app, g_getenv ("LANGUAGE")) :
    as_app_get_comment (extension->p->app, NULL);
}

const gchar *
gimp_extension_get_description (GimpExtension *extension)
{
  g_return_val_if_fail (extension->p->app != NULL, NULL);

  return as_app_get_description (extension->p->app, g_getenv ("LANGUAGE")) ?
    as_app_get_description (extension->p->app, g_getenv ("LANGUAGE")) :
    as_app_get_description (extension->p->app, NULL);
}

GdkPixbuf *
gimp_extension_get_screenshot (GimpExtension  *extension,
                               gint            width,
                               gint            height,
                               const gchar   **caption)
{
  GdkPixbuf    *pixbuf = NULL;
  AsScreenshot *screenshot;

  g_return_val_if_fail (extension->p->app != NULL, NULL);

  screenshot = as_app_get_screenshot_default (extension->p->app);
  if (screenshot)
    {
      AsImage *image;

      image = as_screenshot_get_image_for_locale (screenshot, g_getenv ("LANGUAGE"), width, height);
      if (! image)
        image = as_screenshot_get_image_for_locale (screenshot, NULL, width, height);

      pixbuf = as_image_get_pixbuf (image);
      if (pixbuf)
        {
          g_object_ref (pixbuf);
        }
      else
        {
          GFile            *file;
          GFileInputStream *istream;
          GError           *error = NULL;

          file = g_file_new_for_uri (as_image_get_url (image));
          istream = g_file_read (file, NULL, &error);
          if (istream)
            {
              pixbuf = gdk_pixbuf_new_from_stream (G_INPUT_STREAM (istream), NULL, &error);
              g_object_unref (istream);
            }

          if (error)
            {
              g_printerr ("%s: %s\n", G_STRFUNC, error->message);
              g_error_free (error);
            }
          g_object_unref (file);
        }

      if (caption)
        {
          *caption = as_screenshot_get_caption (screenshot, g_getenv ("LANGUAGE"));
          if (*caption == NULL)
            *caption = as_screenshot_get_caption (screenshot, NULL);
        }
    }

  return pixbuf;
}

const gchar *
gimp_extension_get_path (GimpExtension *extension)
{
  g_return_val_if_fail (GIMP_IS_EXTENSION (extension), NULL);

  return extension->p->path;
}

gchar *
gimp_extension_get_markup_description (GimpExtension *extension)
{
  const gchar *description;

  g_return_val_if_fail (GIMP_IS_EXTENSION (extension), NULL);

  description = gimp_extension_get_description (extension);

  return gimp_appstream_to_pango_markup (description);
}

gboolean
gimp_extension_load (GimpExtension  *extension,
                     GError        **error)
{
  AsApp     *app;
  GPtrArray *extends;
  GPtrArray *requires;
  AsRelease *release;
  gchar     *appdata_name;
  gchar     *path;
  gboolean   success     = FALSE;
  gboolean   has_require = FALSE;

  g_clear_object (&extension->p->app);

  /* Search in subdirectory if a file with the same name as
   * directory and ending with ".metainfo.xml" exists.
   */
  appdata_name = g_strdup_printf ("%s.metainfo.xml",
                                  gimp_object_get_name (GIMP_OBJECT (extension)));
  path = g_build_filename (extension->p->path, appdata_name, NULL);
  g_free (appdata_name);

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
                              _("Extension AppData must advertise a version in a <release> tag."));
      success = FALSE;
    }

  requires = as_app_get_requires (app);
  if (success && requires)
    {
      gint i;

      /* An extension could set requirements, in particular a range of
       * supported version of GIMP, but also other extensions.
       */

      for (i = 0; i < requires->len; i++)
        {
          AsRequire *require = g_ptr_array_index (requires, i);

          if (as_require_get_kind (require) == AS_REQUIRE_KIND_ID &&
              g_strcmp0 (as_require_get_value (require), "org.gimp.GIMP") == 0)
            {
              has_require = TRUE;
              if (! as_require_version_compare (require, GIMP_VERSION, error))
                {
                  success = FALSE;
                  break;
                }
            }
          else if (error && *error == NULL)
            {
              /* Right now we only support requirement relative to GIMP
               * version.
               */
              *error = g_error_new (GIMP_EXTENSION_ERROR,
                                    GIMP_EXTENSION_FAILED,
                                    _("Unsupported <requires> \"%s\" (type %s)."),
                                    as_require_get_value (require),
                                    as_require_kind_to_string (as_require_get_kind (require)));
              success = FALSE;
              break;
            }
        }
    }
  if (! has_require)
    {
      success = FALSE;
      if (error && *error == NULL)
        {
          *error = g_error_new (GIMP_EXTENSION_ERROR,
                                GIMP_EXTENSION_FAILED,
                                _("<requires><id>org.gimp.GIMP</id></requires> for version comparison is mandatory."));
        }
    }

  if (success)
    extension->p->app = app;
  else
    g_object_unref (app);

  return success;
}

gboolean
gimp_extension_run (GimpExtension  *extension,
                    GError        **error)
{
  GHashTable *metadata;
  gchar      *value;

  g_return_val_if_fail (extension->p->app != NULL, FALSE);
  g_return_val_if_fail (error && *error == NULL, FALSE);

  gimp_extension_clean (extension);
  metadata = as_app_get_metadata (extension->p->app);

  value = g_hash_table_lookup (metadata, "GIMP::brush-path");
  extension->p->brush_paths = gimp_extension_validate_paths (extension,
                                                             value, TRUE,
                                                             error);

  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "GIMP::dynamics-path");
      extension->p->dynamics_paths = gimp_extension_validate_paths (extension,
                                                                    value, TRUE,
                                                                    error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "GIMP::mypaint-brush-path");
      extension->p->mypaint_brush_paths = gimp_extension_validate_paths (extension,
                                                                         value, TRUE,
                                                                         error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "GIMP::pattern-path");
      extension->p->pattern_paths = gimp_extension_validate_paths (extension,
                                                                   value, TRUE,
                                                                   error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "GIMP::gradient-path");
      extension->p->gradient_paths = gimp_extension_validate_paths (extension,
                                                                    value, TRUE,
                                                                    error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "GIMP::palette-path");
      extension->p->palette_paths = gimp_extension_validate_paths (extension,
                                                                   value, TRUE,
                                                                   error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "GIMP::tool-preset-path");
      extension->p->tool_preset_paths = gimp_extension_validate_paths (extension,
                                                                       value, TRUE,
                                                                       error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "GIMP::plug-in-path");
      extension->p->plug_in_paths = gimp_extension_validate_paths (extension,
                                                                   value, FALSE,
                                                                   error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "GIMP::splash-path");
      extension->p->splash_paths = gimp_extension_validate_paths (extension,
                                                                  value, TRUE,
                                                                  error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "GIMP::theme-path");
      extension->p->theme_paths = gimp_extension_validate_paths (extension,
                                                                 value, TRUE,
                                                                 error);
    }

  if (*error)
    gimp_extension_clean (extension);

  g_object_set (extension,
                "running", TRUE,
                NULL);

  return (*error == NULL);
}

void
gimp_extension_stop (GimpExtension  *extension)
{
  gimp_extension_clean (extension);
  g_object_set (extension,
                "running", FALSE,
                NULL);
}

GList *
gimp_extension_get_brush_paths (GimpExtension  *extension)
{
  return extension->p->brush_paths;
}

GList *
gimp_extension_get_dynamics_paths (GimpExtension *extension)
{
  return extension->p->dynamics_paths;
}

GList *
gimp_extension_get_mypaint_brush_paths (GimpExtension *extension)
{
  return extension->p->mypaint_brush_paths;
}

GList *
gimp_extension_get_pattern_paths (GimpExtension *extension)
{
  return extension->p->pattern_paths;
}

GList *
gimp_extension_get_gradient_paths (GimpExtension *extension)
{
  return extension->p->gradient_paths;
}

GList *
gimp_extension_get_palette_paths (GimpExtension *extension)
{
  return extension->p->palette_paths;
}

GList *
gimp_extension_get_tool_preset_paths (GimpExtension *extension)
{
  return extension->p->tool_preset_paths;
}

GList *
gimp_extension_get_splash_paths (GimpExtension *extension)
{
  return extension->p->splash_paths;
}

GList *
gimp_extension_get_theme_paths (GimpExtension *extension)
{
  return extension->p->theme_paths;
}

GList *
gimp_extension_get_plug_in_paths (GimpExtension *extension)
{
  return extension->p->plug_in_paths;
}

/**
 * @extension1: a #GimpExtension.
 * @extension2: another #GimpExtension.
 *
 * Compare 2 extensions by their ID.
 *
 * Returns: 0 if the 2 extensions have the same ID (even though they may
 * represent different versions of the same extension).
 */
gint
gimp_extension_cmp (GimpExtension *extension1,
                    GimpExtension *extension2)
{
  g_return_val_if_fail (GIMP_IS_EXTENSION (extension1), -1);
  g_return_val_if_fail (GIMP_IS_EXTENSION (extension2), -1);

  return g_strcmp0 (gimp_object_get_name (extension1),
                    gimp_object_get_name (extension2));
}

/**
 * @extension: a #GimpExtension.
 * @id:        an extension ID (reverse-DNS scheme)
 *
 * Compare the extension ID with @id.
 *
 * Returns: 0 if @extension have @id as appstream ID.
 */
gint
gimp_extension_id_cmp (GimpExtension *extension,
                       const gchar   *id)
{
  return g_strcmp0 (gimp_object_get_name (extension), id);
}

static void
gimp_extension_clean (GimpExtension  *extension)
{
  g_list_free_full (extension->p->brush_paths, g_object_unref);
  extension->p->brush_paths = NULL;
  g_list_free_full (extension->p->dynamics_paths, g_object_unref);
  extension->p->dynamics_paths = NULL;
  g_list_free_full (extension->p->mypaint_brush_paths, g_object_unref);
  extension->p->brush_paths = NULL;
  g_list_free_full (extension->p->pattern_paths, g_object_unref);
  extension->p->pattern_paths = NULL;
  g_list_free_full (extension->p->gradient_paths, g_object_unref);
  extension->p->gradient_paths = NULL;
  g_list_free_full (extension->p->palette_paths, g_object_unref);
  extension->p->palette_paths = NULL;
  g_list_free_full (extension->p->tool_preset_paths, g_object_unref);
  extension->p->tool_preset_paths = NULL;
  g_list_free_full (extension->p->plug_in_paths, g_object_unref);
  extension->p->plug_in_paths = NULL;
  g_list_free_full (extension->p->splash_paths, g_object_unref);
  extension->p->splash_paths = NULL;
  g_list_free_full (extension->p->theme_paths, g_object_unref);
  extension->p->theme_paths = NULL;
}

/**
 * gimp_extension_file_cmp:
 * @a:
 * @b:
 *
 * A small g_file_equal() wrapper using GCompareFunc signature.
 */
static gint
gimp_extension_file_cmp (GFile *a,
                         GFile *b)
{
  return g_file_equal (a, b) ? 0 : 1;
}

/**
 * gimp_extension_validate_paths:
 * @extension: the #GimpExtension
 * @path:      A list of directories separated by ':'.
 * @error:
 *
 * Very similar to gimp_path_parse() except that we don't use
 * G_SEARCHPATH_SEPARATOR as path separator, because it must not be
 * os-dependent.
 * Also we only allow relative path which are children of the main
 * extension directory (we do not allow extensions to list external
 * folders).
 *
 * Returns: A #GList of #GFile as listed in @path.
 **/
static GList *
gimp_extension_validate_paths (GimpExtension  *extension,
                               const gchar    *paths,
                               gboolean        as_directories,
                               GError        **error)
{
  gchar **patharray;
  GList *list      = NULL;
  gint   i;

  g_return_val_if_fail (error && *error == NULL, FALSE);

  if (!paths || ! (*paths))
    return NULL;

  patharray = g_strsplit (paths, ":", 0);

  for (i = 0; patharray[i]; i++)
    {
      /* Note: appstream-glib is supposed to return everything as UTF-8,
       * so we should not have to bother about this. */
      gchar    *path;
      GFile    *file;
      GFile    *ext_dir;
      GFile    *parent;
      GFile    *child;
      gboolean  is_subpath = FALSE;
      gint      max_depth  = 10;

      if (g_path_is_absolute (patharray[i]))
        {
          *error = g_error_new (GIMP_EXTENSION_ERROR,
                                GIMP_EXTENSION_BAD_PATH,
                                _("'%s' is not a relative path."),
                                patharray[i]);
          break;
        }
      path = g_build_filename (extension->p->path, patharray[i], NULL);
      file = g_file_new_for_path (path);
      g_free (path);

      ext_dir = g_file_new_for_path (extension->p->path);

      /* Even with relative paths, it is easy to trick the system
       * and leak out of the extension. So check actual kinship.
       */
      child = g_object_ref (file);
      while (max_depth > 0 && (parent = g_file_get_parent (child)))
        {
          if (g_file_equal (parent, ext_dir))
            {
              is_subpath = TRUE;
              g_object_unref (parent);
              break;
            }
          g_object_unref (child);
          child = parent;
          /* Avoid unfinite looping. */
          max_depth--;
        }
      g_object_unref (child);
      g_object_unref (ext_dir);

      if (! is_subpath)
        {
          *error = g_error_new (GIMP_EXTENSION_ERROR,
                                GIMP_EXTENSION_BAD_PATH,
                                _("'%s' is not a child of the extension."),
                                patharray[i]);
          g_object_unref (file);
          break;
        }

      if (as_directories)
        {
          if (g_file_query_file_type (file,
                                      G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                      NULL) != G_FILE_TYPE_DIRECTORY)
            {
              *error = g_error_new (GIMP_EXTENSION_ERROR,
                                    GIMP_EXTENSION_BAD_PATH,
                                    _("'%s' is not a directory."),
                                    patharray[i]);
              g_object_unref (file);
              break;
            }
        }
      else
        {
          if (g_file_query_file_type (file,
                                      G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                      NULL) != G_FILE_TYPE_REGULAR)
            {
              *error = g_error_new (GIMP_EXTENSION_ERROR,
                                    GIMP_EXTENSION_BAD_PATH,
                                    _("'%s' is not a valid file."),
                                    patharray[i]);
              g_object_unref (file);
              break;
            }
        }

      g_return_val_if_fail (path != NULL, NULL);
      if (g_list_find_custom (list, file, (GCompareFunc) gimp_extension_file_cmp))
        {
          /* Silently ignore duplicate paths. */
          g_object_unref (file);
          continue;
        }

      list = g_list_prepend (list, file);
    }

  g_strfreev (patharray);
  list = g_list_reverse (list);

  return list;
}
