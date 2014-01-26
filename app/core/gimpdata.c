/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdata.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <errno.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include "core-types.h"

#include "gimp-utils.h"
#include "gimpdata.h"
#include "gimpmarshal.h"
#include "gimptag.h"
#include "gimptagged.h"

#include "gimp-intl.h"


enum
{
  DIRTY,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_FILENAME,
  PROP_WRITABLE,
  PROP_DELETABLE,
  PROP_MIME_TYPE
};


typedef struct _GimpDataPrivate GimpDataPrivate;

struct _GimpDataPrivate
{
  gchar  *filename;
  GQuark  mime_type;
  guint   writable  : 1;
  guint   deletable : 1;
  guint   dirty     : 1;
  guint   internal  : 1;
  gint    freeze_count;
  time_t  mtime;

  /* Identifies the GimpData object across sessions. Used when there
   * is not a filename associated with the object.
   */
  gchar  *identifier;

  GList  *tags;
};

#define GIMP_DATA_GET_PRIVATE(data) \
        G_TYPE_INSTANCE_GET_PRIVATE (data, GIMP_TYPE_DATA, GimpDataPrivate)


static void      gimp_data_class_init        (GimpDataClass       *klass);
static void      gimp_data_tagged_iface_init (GimpTaggedInterface *iface);

static void      gimp_data_init              (GimpData            *data,
                                              GimpDataClass       *data_class);

static void      gimp_data_constructed       (GObject             *object);
static void      gimp_data_finalize          (GObject             *object);
static void      gimp_data_set_property      (GObject             *object,
                                              guint                property_id,
                                              const GValue        *value,
                                              GParamSpec          *pspec);
static void      gimp_data_get_property      (GObject             *object,
                                              guint                property_id,
                                              GValue              *value,
                                              GParamSpec          *pspec);

static void      gimp_data_name_changed      (GimpObject          *object);
static gint64    gimp_data_get_memsize       (GimpObject          *object,
                                              gint64              *gui_size);

static void      gimp_data_real_dirty        (GimpData            *data);

static gboolean  gimp_data_add_tag           (GimpTagged          *tagged,
                                              GimpTag             *tag);
static gboolean  gimp_data_remove_tag        (GimpTagged          *tagged,
                                              GimpTag             *tag);
static GList *   gimp_data_get_tags          (GimpTagged          *tagged);
static gchar *   gimp_data_get_identifier    (GimpTagged          *tagged);
static gchar *   gimp_data_get_checksum      (GimpTagged          *tagged);


static guint data_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;


GType
gimp_data_get_type (void)
{
  static GType data_type = 0;

  if (! data_type)
    {
      const GTypeInfo data_info =
      {
        sizeof (GimpDataClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_data_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpData),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_data_init,
      };

      const GInterfaceInfo tagged_info =
      {
        (GInterfaceInitFunc) gimp_data_tagged_iface_init,
        NULL,           /* interface_finalize */
        NULL            /* interface_data     */
      };

      data_type = g_type_register_static (GIMP_TYPE_VIEWABLE,
                                          "GimpData",
                                          &data_info, 0);

      g_type_add_interface_static (data_type, GIMP_TYPE_TAGGED, &tagged_info);
    }

  return data_type;
}

static void
gimp_data_class_init (GimpDataClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  data_signals[DIRTY] =
    g_signal_new ("dirty",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDataClass, dirty),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->constructed       = gimp_data_constructed;
  object_class->finalize          = gimp_data_finalize;
  object_class->set_property      = gimp_data_set_property;
  object_class->get_property      = gimp_data_get_property;

  gimp_object_class->name_changed = gimp_data_name_changed;
  gimp_object_class->get_memsize  = gimp_data_get_memsize;

  klass->dirty                    = gimp_data_real_dirty;
  klass->save                     = NULL;
  klass->get_extension            = NULL;
  klass->duplicate                = NULL;

  g_object_class_install_property (object_class, PROP_FILENAME,
                                   g_param_spec_string ("filename", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_WRITABLE,
                                   g_param_spec_boolean ("writable", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_DELETABLE,
                                   g_param_spec_boolean ("deletable", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_MIME_TYPE,
                                   g_param_spec_string ("mime-type", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (GimpDataPrivate));
}

static void
gimp_data_tagged_iface_init (GimpTaggedInterface *iface)
{
  iface->add_tag        = gimp_data_add_tag;
  iface->remove_tag     = gimp_data_remove_tag;
  iface->get_tags       = gimp_data_get_tags;
  iface->get_identifier = gimp_data_get_identifier;
  iface->get_checksum   = gimp_data_get_checksum;
}

static void
gimp_data_init (GimpData      *data,
                GimpDataClass *data_class)
{
  GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (data);

  private->writable  = TRUE;
  private->deletable = TRUE;
  private->dirty     = TRUE;

  /*  look at the passed class pointer, not at GIMP_DATA_GET_CLASS(data)
   *  here, because the latter is always GimpDataClass itself
   */
  if (! data_class->save)
    private->writable = FALSE;

  /*  freeze the data object during construction  */
  gimp_data_freeze (data);
}

static void
gimp_data_constructed (GObject *object)
{
  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_data_thaw (GIMP_DATA (object));
}

static void
gimp_data_finalize (GObject *object)
{
  GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (object);

  if (private->filename)
    {
      g_free (private->filename);
      private->filename = NULL;
    }

  if (private->tags)
    {
      g_list_free_full (private->tags, (GDestroyNotify) g_object_unref);
      private->tags = NULL;
    }

  if (private->identifier)
    {
      g_free (private->identifier);
      private->identifier = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_data_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpData        *data    = GIMP_DATA (object);
  GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (data);

  switch (property_id)
    {
    case PROP_FILENAME:
      gimp_data_set_filename (data,
                              g_value_get_string (value),
                              private->writable,
                              private->deletable);
      break;

    case PROP_WRITABLE:
      private->writable = g_value_get_boolean (value);
      break;

    case PROP_DELETABLE:
      private->deletable = g_value_get_boolean (value);
      break;

    case PROP_MIME_TYPE:
      if (g_value_get_string (value))
        private->mime_type = g_quark_from_string (g_value_get_string (value));
      else
        private->mime_type = 0;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_data_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_FILENAME:
      g_value_set_string (value, private->filename);
      break;

    case PROP_WRITABLE:
      g_value_set_boolean (value, private->writable);
      break;

    case PROP_DELETABLE:
      g_value_set_boolean (value, private->deletable);
      break;

    case PROP_MIME_TYPE:
      g_value_set_string (value, g_quark_to_string (private->mime_type));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_data_name_changed (GimpObject *object)
{
  GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (object);

  private->dirty = TRUE;

  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);
}

static gint64
gimp_data_get_memsize (GimpObject *object,
                       gint64     *gui_size)
{
  GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (object);
  gint64           memsize = 0;

  memsize += gimp_string_get_memsize (private->filename);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_data_real_dirty (GimpData *data)
{
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (data));

  /* Emit the "name-changed" to signal general dirtiness, our name
   * changed implementation will also set the "dirty" flag to TRUE.
   */
  gimp_object_name_changed (GIMP_OBJECT (data));
}

static gboolean
gimp_data_add_tag (GimpTagged *tagged,
                   GimpTag    *tag)
{
  GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (tagged);
  GList           *list;

  for (list = private->tags; list; list = g_list_next (list))
    {
      GimpTag *this = GIMP_TAG (list->data);

      if (gimp_tag_equals (tag, this))
        return FALSE;
    }

  private->tags = g_list_prepend (private->tags, g_object_ref (tag));

  return TRUE;
}

static gboolean
gimp_data_remove_tag (GimpTagged *tagged,
                      GimpTag    *tag)
{
  GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (tagged);
  GList           *list;

  for (list = private->tags; list; list = g_list_next (list))
    {
      GimpTag *this = GIMP_TAG (list->data);

      if (gimp_tag_equals (tag, this))
        {
          private->tags = g_list_delete_link (private->tags, list);
          g_object_unref (this);
          return TRUE;
        }
    }

  return FALSE;
}

static GList *
gimp_data_get_tags (GimpTagged *tagged)
{
  GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (tagged);

  return private->tags;
}

static gchar *
gimp_data_get_identifier (GimpTagged *tagged)
{
  GimpDataPrivate *private    = GIMP_DATA_GET_PRIVATE (tagged);
  gchar           *identifier = NULL;

  if (private->filename)
    {
      const gchar *data_dir = gimp_data_directory ();
      const gchar *gimp_dir = gimp_directory ();
      gchar       *tmp;

      if (g_str_has_prefix (private->filename, data_dir))
        {
          tmp = g_strconcat ("${gimp_data_dir}",
                             private->filename + strlen (data_dir),
                             NULL);
          identifier = g_filename_to_utf8 (tmp, -1, NULL, NULL, NULL);
          g_free (tmp);
        }
      else if (g_str_has_prefix (private->filename, gimp_dir))
        {
          tmp = g_strconcat ("${gimp_dir}",
                             private->filename + strlen (gimp_dir),
                             NULL);
          identifier = g_filename_to_utf8 (tmp, -1, NULL, NULL, NULL);
          g_free (tmp);
        }
      else
        {
          identifier = g_filename_to_utf8 (private->filename, -1,
                                           NULL, NULL, NULL);
        }

      if (! identifier)
        {
          g_warning ("Failed to convert '%s' to utf8.\n", private->filename);
          identifier = g_strdup (private->filename);
        }
    }
  else if (private->internal)
    {
      identifier = g_strdup (private->identifier);
    }

  return identifier;
}

static gchar *
gimp_data_get_checksum (GimpTagged *tagged)
{
  return NULL;
}

/**
 * gimp_data_save:
 * @data:  object whose contents are to be saved.
 * @error: return location for errors or %NULL
 *
 * Save the object.  If the object is marked as "internal", nothing happens.
 * Otherwise, it is saved to disk, using the file name set by
 * gimp_data_set_filename().  If the save is successful, the
 * object is marked as not dirty.  If not, an error message is returned
 * using the @error argument.
 *
 * Returns: %TRUE if the object is internal or the save is successful.
 **/
gboolean
gimp_data_save (GimpData  *data,
                GError   **error)
{
  GimpDataPrivate *private;
  gboolean         success = FALSE;

  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  private = GIMP_DATA_GET_PRIVATE (data);

  g_return_val_if_fail (private->writable == TRUE, FALSE);

  if (private->internal)
    {
      private->dirty = FALSE;
      return TRUE;
    }

  g_return_val_if_fail (private->filename != NULL, FALSE);

  if (GIMP_DATA_GET_CLASS (data)->save)
    success = GIMP_DATA_GET_CLASS (data)->save (data, error);

  if (success)
    {
      GFile     *file = g_file_new_for_path (private->filename);
      GFileInfo *info = g_file_query_info (file, "time::*",
                                           G_FILE_QUERY_INFO_NONE,
                                           NULL, NULL);
      if (info)
        {
          private->mtime =
            g_file_info_get_attribute_uint64 (info,
                                              G_FILE_ATTRIBUTE_TIME_MODIFIED);
          g_object_unref (info);
        }

      g_object_unref (file);

      private->dirty = FALSE;
    }

  return success;
}

/**
 * gimp_data_dirty:
 * @data: a #GimpData object.
 *
 * Marks @data as dirty.  Unless the object is frozen, this causes
 * its preview to be invalidated, and emits a "dirty" signal.  If the
 * object is frozen, the function has no effect.
 **/
void
gimp_data_dirty (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_if_fail (GIMP_IS_DATA (data));

  private = GIMP_DATA_GET_PRIVATE (data);

  if (private->freeze_count == 0)
    g_signal_emit (data, data_signals[DIRTY], 0);
}

void
gimp_data_clean (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_if_fail (GIMP_IS_DATA (data));

  private = GIMP_DATA_GET_PRIVATE (data);

  private->dirty = FALSE;
}

gboolean
gimp_data_is_dirty (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);

  private = GIMP_DATA_GET_PRIVATE (data);

  return private->dirty;
}

/**
 * gimp_data_freeze:
 * @data: a #GimpData object.
 *
 * Increments the freeze count for the object.  A positive freeze count
 * prevents the object from being treated as dirty.  Any call to this
 * function must be followed eventually by a call to gimp_data_thaw().
 **/
void
gimp_data_freeze (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_if_fail (GIMP_IS_DATA (data));

  private = GIMP_DATA_GET_PRIVATE (data);

  private->freeze_count++;
}

/**
 * gimp_data_thaw:
 * @data: a #GimpData object.
 *
 * Decrements the freeze count for the object.  If the freeze count
 * drops to zero, the object is marked as dirty, and the "dirty"
 * signal is emitted.  It is an error to call this function without
 * having previously called gimp_data_freeze().
 **/
void
gimp_data_thaw (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_if_fail (GIMP_IS_DATA (data));

  private = GIMP_DATA_GET_PRIVATE (data);

  g_return_if_fail (private->freeze_count > 0);

  private->freeze_count--;

  if (private->freeze_count == 0)
    gimp_data_dirty (data);
}

gboolean
gimp_data_is_frozen (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);

  private = GIMP_DATA_GET_PRIVATE (data);

  return private->freeze_count > 0;
}

/**
 * gimp_data_delete_from_disk:
 * @data:  a #GimpData object.
 * @error: return location for errors or %NULL
 *
 * Deletes the object from disk.  If the object is marked as "internal",
 * nothing happens.  Otherwise, if the file exists whose name has been
 * set by gimp_data_set_filename(), it is deleted.  Obviously this is
 * a potentially dangerous function, which should be used with care.
 *
 * Returns: %TRUE if the object is internal to Gimp, or the deletion is
 *          successful.
 **/
gboolean
gimp_data_delete_from_disk (GimpData  *data,
                            GError   **error)
{
  GimpDataPrivate *private;

  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  private = GIMP_DATA_GET_PRIVATE (data);

  g_return_val_if_fail (private->filename != NULL, FALSE);
  g_return_val_if_fail (private->deletable == TRUE, FALSE);

  if (private->internal)
    return TRUE;

  if (g_unlink (private->filename) == -1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_DELETE,
                   _("Could not delete '%s': %s"),
                   gimp_filename_to_utf8 (private->filename),
                   g_strerror (errno));
      return FALSE;
    }

  return TRUE;
}

const gchar *
gimp_data_get_extension (GimpData *data)
{
  g_return_val_if_fail (GIMP_IS_DATA (data), NULL);

  if (GIMP_DATA_GET_CLASS (data)->get_extension)
    return GIMP_DATA_GET_CLASS (data)->get_extension (data);

  return NULL;
}

/**
 * gimp_data_set_filename:
 * @data:     A #GimpData object
 * @filename: File name to assign to @data.
 * @writable: %TRUE if we want to be able to write to this file.
 * @deletable: %TRUE if we want to be able to delete this file.
 *
 * This function assigns a file name to @data, and sets some flags
 * according to the properties of the file.  If @writable is %TRUE,
 * and the user has permission to write or overwrite the requested file
 * name, and a "save" method exists for @data's object type, then
 * @data is marked as writable.
 **/
void
gimp_data_set_filename (GimpData    *data,
                        const gchar *filename,
                        gboolean     writable,
                        gboolean     deletable)
{
  GimpDataPrivate *private;

  g_return_if_fail (GIMP_IS_DATA (data));
  g_return_if_fail (filename != NULL);
  g_return_if_fail (g_path_is_absolute (filename));

  private = GIMP_DATA_GET_PRIVATE (data);

  if (private->internal)
    return;

  if (private->filename)
    g_free (private->filename);

  private->filename  = g_strdup (filename);
  private->writable  = FALSE;
  private->deletable = FALSE;

  /*  if the data is supposed to be writable or deletable,
   *  still check if it really is
   */
  if (writable || deletable)
    {
      gchar *dirname = g_path_get_dirname (filename);

      if ((g_access (filename, F_OK) == 0 &&  /* check if the file exists    */
           g_access (filename, W_OK) == 0) || /* and is writable             */
          (g_access (filename, F_OK) != 0 &&  /* OR doesn't exist            */
           g_access (dirname,  W_OK) == 0))   /* and we can write to its dir */
        {
          private->writable  = writable  ? TRUE : FALSE;
          private->deletable = deletable ? TRUE : FALSE;
        }

      g_free (dirname);

      /*  if we can't save, we are not writable  */
      if (! GIMP_DATA_GET_CLASS (data)->save)
        private->writable = FALSE;
    }
}

/**
 * gimp_data_create_filename:
 * @data:     a #Gimpdata object.
 * @dest_dir: directory in which to create a file name.
 *
 * This function creates a unique file name to be used for saving
 * a representation of @data in the directory @dest_dir.  If the
 * user does not have write permission in @dest_dir, then @data
 * is marked as "not writable", so you should check on this before
 * assuming that @data can be saved.
 **/
void
gimp_data_create_filename (GimpData    *data,
                           const gchar *dest_dir)
{
  GimpDataPrivate *private;
  gchar           *safename;
  gchar           *filename;
  gchar           *fullpath;
  gint             i;
  gint             unum  = 1;
  GError          *error = NULL;

  g_return_if_fail (GIMP_IS_DATA (data));
  g_return_if_fail (dest_dir != NULL);
  g_return_if_fail (g_path_is_absolute (dest_dir));

  private = GIMP_DATA_GET_PRIVATE (data);

  if (private->internal)
    return;

  safename = g_filename_from_utf8 (gimp_object_get_name (data),
                                   -1, NULL, NULL, &error);
  if (! safename)
    {
      g_warning ("gimp_data_create_filename:\n"
                 "g_filename_from_utf8() failed for '%s': %s",
                 gimp_object_get_name (data), error->message);
      g_error_free (error);
      return;
    }

  g_strstrip (safename);

  if (safename[0] == '.')
    safename[0] = '-';

  for (i = 0; safename[i]; i++)
    if (strchr ("\\/*?\"`'<>{}|\n\t ;:$^&", safename[i]))
      safename[i] = '-';

  filename = g_strconcat (safename, gimp_data_get_extension (data), NULL);

  fullpath = g_build_filename (dest_dir, filename, NULL);

  g_free (filename);

  while (g_file_test (fullpath, G_FILE_TEST_EXISTS))
    {
      g_free (fullpath);

      filename = g_strdup_printf ("%s-%d%s",
                                  safename,
                                  unum++,
                                  gimp_data_get_extension (data));

      fullpath = g_build_filename (dest_dir, filename, NULL);

      g_free (filename);
    }

  g_free (safename);

  gimp_data_set_filename (data, fullpath, TRUE, TRUE);

  g_free (fullpath);
}

const gchar *
gimp_data_get_filename (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_val_if_fail (GIMP_IS_DATA (data), NULL);

  private = GIMP_DATA_GET_PRIVATE (data);

  return private->filename;
}

static const gchar *tag_blacklist[] = { "brushes",
                                        "dynamics",
                                        "patterns",
                                        "palettes",
                                        "gradients",
                                        "tool-presets" };

/**
 * gimp_data_set_folder_tags:
 * @data:          a #Gimpdata object.
 * @top_directory: the top directory of the currently processed data
 *                 hierarchy, or %NULL if that top directory is
 *                 currently processed itself
 *
 * Sets tags based on all folder names below top_directory. So if the
 * data's filename is /home/foo/.gimp/brushes/Flowers/Roses/rose.gbr,
 * it will add "Flowers" and "Roses" tags.
 *
 * if the top directory (as passed, or as derived from the data's
 * filename) does not end with one of the default data directory names
 * (brushes, patterns etc), its name will be added as tag too.
 **/
void
gimp_data_set_folder_tags (GimpData    *data,
                           const gchar *top_directory)
{
  GimpDataPrivate *private;
  gchar           *dirname;

  g_return_if_fail (GIMP_IS_DATA (data));

  private = GIMP_DATA_GET_PRIVATE (data);

  if (private->internal)
    return;

  g_return_if_fail (private->filename != NULL);

  dirname = g_path_get_dirname (private->filename);

  /*  if this data is in a subfolder, walk up the hierarchy and
   *  set each folder on the way as tag, except the top_directory
   */
  if (top_directory)
    {
      size_t top_directory_len = strlen (top_directory);

      g_return_if_fail (g_str_has_prefix (dirname, top_directory) &&
                        (dirname[top_directory_len] == '\0' ||
                         G_IS_DIR_SEPARATOR (dirname[top_directory_len])));

      do
        {
          gchar   *basename = g_path_get_basename (dirname);
          GimpTag *tag      = gimp_tag_new (basename);
          gchar   *tmp;

          gimp_tag_set_internal (tag, TRUE);
          gimp_tagged_add_tag (GIMP_TAGGED (data), tag);
          g_object_unref (tag);
          g_free (basename);

          tmp = g_path_get_dirname (dirname);
          g_free (dirname);
          dirname = tmp;
        }
      while (strcmp (dirname, top_directory));
    }

  if (dirname)
    {
      gchar *basename = g_path_get_basename (dirname);
      gint   i;

      for (i = 0; i <  G_N_ELEMENTS (tag_blacklist); i++)
        {
          if (! strcmp (basename, tag_blacklist[i]))
            break;
        }

      if (i == G_N_ELEMENTS (tag_blacklist))
        {
          GimpTag *tag = gimp_tag_new (basename);

          gimp_tag_set_internal (tag, TRUE);
          gimp_tagged_add_tag (GIMP_TAGGED (data), tag);
          g_object_unref (tag);
        }

      g_free (basename);
      g_free (dirname);
    }
}

const gchar *
gimp_data_get_mime_type (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_val_if_fail (GIMP_IS_DATA (data), NULL);

  private = GIMP_DATA_GET_PRIVATE (data);

  return g_quark_to_string (private->mime_type);
}

gboolean
gimp_data_is_writable (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);

  private = GIMP_DATA_GET_PRIVATE (data);

  return private->writable;
}

gboolean
gimp_data_is_deletable (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);

  private = GIMP_DATA_GET_PRIVATE (data);

  return private->deletable;
}

void
gimp_data_set_mtime (GimpData *data,
                     time_t    mtime)
{
  GimpDataPrivate *private;

  g_return_if_fail (GIMP_IS_DATA (data));

  private = GIMP_DATA_GET_PRIVATE (data);

  private->mtime = mtime;
}

time_t
gimp_data_get_mtime (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_val_if_fail (GIMP_IS_DATA (data), 0);

  private = GIMP_DATA_GET_PRIVATE (data);

  return private->mtime;
}

/**
 * gimp_data_duplicate:
 * @data: a #GimpData object
 *
 * Creates a copy of @data, if possible.  Only the object data is
 * copied:  the newly created object is not automatically given an
 * object name, file name, preview, etc.
 *
 * Returns: the newly created copy, or %NULL if @data cannot be copied.
 **/
GimpData *
gimp_data_duplicate (GimpData *data)
{
  g_return_val_if_fail (GIMP_IS_DATA (data), NULL);

  if (GIMP_DATA_GET_CLASS (data)->duplicate)
    {
      GimpData        *new     = GIMP_DATA_GET_CLASS (data)->duplicate (data);
      GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (new);

      g_object_set (new,
                    "name",      NULL,
                    "writable",  GIMP_DATA_GET_CLASS (new)->save != NULL,
                    "deletable", TRUE,
                    NULL);

      if (private->filename)
        {
          g_free (private->filename);
          private->filename = NULL;
        }

      return new;
    }

  return NULL;
}

/**
 * gimp_data_make_internal:
 * @data: a #GimpData object.
 *
 * Mark @data as "internal" to Gimp, which means that it will not be
 * saved to disk.  Note that if you do this, later calls to
 * gimp_data_save() and gimp_data_delete_from_disk() will
 * automatically return successfully without giving any warning.
 *
 * The identifier name shall be an untranslated globally unique string
 * that identifies the internal object across sessions.
 **/
void
gimp_data_make_internal (GimpData    *data,
                         const gchar *identifier)
{
  GimpDataPrivate *private;

  g_return_if_fail (GIMP_IS_DATA (data));

  private = GIMP_DATA_GET_PRIVATE (data);

  if (private->filename)
    {
      g_free (private->filename);
      private->filename = NULL;
    }

  private->identifier = g_strdup (identifier);
  private->writable   = FALSE;
  private->deletable  = FALSE;
  private->internal   = TRUE;
}

gboolean
gimp_data_is_internal (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);

  private = GIMP_DATA_GET_PRIVATE (data);

  return private->internal;
}

/**
 * gimp_data_compare:
 * @data1: a #GimpData object.
 * @data2: another #GimpData object.
 *
 * Compares two data objects for use in sorting. Objects marked as
 * "internal" come first, then user-writable objects, then system data
 * files. In these three groups, the objects are sorted alphabetically
 * by name, using gimp_object_name_collate().
 *
 * Return value: -1 if @data1 compares before @data2,
 *                0 if they compare equal,
 *                1 if @data1 compares after @data2.
 **/
gint
gimp_data_compare (GimpData *data1,
		   GimpData *data2)
{
  GimpDataPrivate *private1 = GIMP_DATA_GET_PRIVATE (data1);
  GimpDataPrivate *private2 = GIMP_DATA_GET_PRIVATE (data2);

  /*  move the internal objects (like the FG -> BG) gradient) to the top  */
  if (private1->internal != private2->internal)
    return private1->internal ? -1 : 1;

  /*  keep user-deletable objects above system resource files  */
  if (private1->deletable != private2->deletable)
    return private1->deletable ? -1 : 1;

  return gimp_object_name_collate ((GimpObject *) data1,
                                   (GimpObject *) data2);
}

/**
 * gimp_data_error_quark:
 *
 * This function is used to implement the GIMP_DATA_ERROR macro. It
 * shouldn't be called directly.
 *
 * Return value: the #GQuark to identify error in the GimpData error domain.
 **/
GQuark
gimp_data_error_quark (void)
{
  return g_quark_from_static_string ("gimp-data-error-quark");
}
