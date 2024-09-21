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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp-memsize.h"
#include "gimpdata.h"
#include "gimpidtable.h"
#include "gimpimage.h"
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
  PROP_ID,
  PROP_FILE,
  PROP_IMAGE,
  PROP_WRITABLE,
  PROP_DELETABLE,
  PROP_MIME_TYPE
};


struct _GimpDataPrivate
{
  gint       ID;
  GFile     *file;
  GimpImage *image;

  GQuark  mime_type;
  guint   writable  : 1;
  guint   deletable : 1;
  guint   dirty     : 1;
  guint   internal  : 1;
  gint    freeze_count;
  gint64  mtime;

  /* Identifies the collection this GimpData belongs to.
   * Used when there is not a filename associated with the object.
   */
  gchar  *collection;

  GList  *tags;
};

#define GIMP_DATA_GET_PRIVATE(obj) (((GimpData *) (obj))->priv)


static void       gimp_data_tagged_iface_init (GimpTaggedInterface *iface);

static void       gimp_data_constructed       (GObject             *object);
static void       gimp_data_finalize          (GObject             *object);
static void       gimp_data_set_property      (GObject             *object,
                                               guint                property_id,
                                               const GValue        *value,
                                               GParamSpec          *pspec);
static void       gimp_data_get_property      (GObject             *object,
                                               guint                property_id,
                                               GValue              *value,
                                               GParamSpec          *pspec);

static void       gimp_data_name_changed      (GimpObject          *object);
static gint64     gimp_data_get_memsize       (GimpObject          *object,
                                               gint64              *gui_size);

static gboolean   gimp_data_is_name_editable  (GimpViewable        *viewable);

static void       gimp_data_real_dirty        (GimpData            *data);
static GimpData * gimp_data_real_duplicate    (GimpData            *data);
static gint       gimp_data_real_compare      (GimpData            *data1,
                                               GimpData            *data2);

static gboolean   gimp_data_add_tag           (GimpTagged          *tagged,
                                               GimpTag             *tag);
static gboolean   gimp_data_remove_tag        (GimpTagged          *tagged,
                                               GimpTag             *tag);
static GList    * gimp_data_get_tags          (GimpTagged          *tagged);
static gchar    * gimp_data_get_identifier    (GimpTagged          *tagged);
static gchar    * gimp_data_get_checksum      (GimpTagged          *tagged);

static gchar    * gimp_data_get_collection    (GimpData            *data);


G_DEFINE_TYPE_WITH_CODE (GimpData, gimp_data, GIMP_TYPE_RESOURCE,
                         G_ADD_PRIVATE (GimpData)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_TAGGED,
                                                gimp_data_tagged_iface_init))

#define parent_class gimp_data_parent_class

static guint data_signals[LAST_SIGNAL] = { 0 };

static GimpIdTable *data_id_table = NULL;


static void
gimp_data_class_init (GimpDataClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  data_signals[DIRTY] =
    g_signal_new ("dirty",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDataClass, dirty),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed        = gimp_data_constructed;
  object_class->finalize           = gimp_data_finalize;
  object_class->set_property       = gimp_data_set_property;
  object_class->get_property       = gimp_data_get_property;

  gimp_object_class->name_changed  = gimp_data_name_changed;
  gimp_object_class->get_memsize   = gimp_data_get_memsize;

  viewable_class->name_editable    = TRUE;
  viewable_class->is_name_editable = gimp_data_is_name_editable;

  klass->dirty                     = gimp_data_real_dirty;
  klass->save                      = NULL;
  klass->get_extension             = NULL;
  klass->copy                      = NULL;
  klass->duplicate                 = gimp_data_real_duplicate;
  klass->compare                   = gimp_data_real_compare;

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_int ("id", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_FILE,
                                   g_param_spec_object ("file", NULL, NULL,
                                                        G_TYPE_FILE,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        G_PARAM_EXPLICIT_NOTIFY |
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

  data_id_table = gimp_id_table_new ();
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
gimp_data_init (GimpData *data)
{
  GimpDataPrivate *private = gimp_data_get_instance_private (data);

  data->priv = private;

  private->ID        = gimp_id_table_insert (data_id_table, data);
  private->writable  = TRUE;
  private->deletable = TRUE;
  private->dirty     = TRUE;

  /*  freeze the data object during construction  */
  gimp_data_freeze (data);
}

static void
gimp_data_constructed (GObject *object)
{
  GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  if (! GIMP_DATA_GET_CLASS (object)->save)
    private->writable = FALSE;

  gimp_data_thaw (GIMP_DATA (object));
}

static void
gimp_data_finalize (GObject *object)
{
  GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (object);

  gimp_id_table_remove (data_id_table, private->ID);

  g_clear_object (&private->file);
  g_clear_weak_pointer (&private->image);

  if (private->tags)
    {
      g_list_free_full (private->tags, (GDestroyNotify) g_object_unref);
      private->tags = NULL;
    }

  g_clear_pointer (&private->collection, g_free);

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
    case PROP_FILE:
      gimp_data_set_file (data,
                          g_value_get_object (value),
                          private->writable,
                          private->deletable);
      break;

    case PROP_IMAGE:
      gimp_data_set_image (data,
                           g_value_get_object (value),
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
    case PROP_ID:
      g_value_set_int (value, private->ID);
      break;

    case PROP_FILE:
      g_value_set_object (value, private->file);
      break;

    case PROP_IMAGE:
      g_value_set_object (value, private->image);
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

  memsize += gimp_g_object_get_memsize (G_OBJECT (private->file));

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_data_is_name_editable (GimpViewable *viewable)
{
  return gimp_data_is_writable (GIMP_DATA (viewable)) &&
         ! gimp_data_is_internal (GIMP_DATA (viewable));
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

static GimpData *
gimp_data_real_duplicate (GimpData *data)
{
  if (GIMP_DATA_GET_CLASS (data)->copy)
    {
      GimpData *new = g_object_new (G_OBJECT_TYPE (data), NULL);

      gimp_data_copy (new, data);

      return new;
    }

  return NULL;
}

static gint
gimp_data_real_compare (GimpData *data1,
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

  if (g_strcmp0 (private1->collection, private2->collection) != 0)
    return g_strcmp0 (private1->collection, private2->collection);

  return gimp_object_name_collate ((GimpObject *) data1,
                                   (GimpObject *) data2);
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
  GimpData        *data       = GIMP_DATA (tagged);
  GimpDataPrivate *private    = GIMP_DATA_GET_PRIVATE (data);
  gchar           *identifier = NULL;
  gchar           *collection = NULL;

  g_return_val_if_fail (private->internal || private->file != NULL || private->image != NULL, NULL);

  collection = gimp_data_get_collection (data);
  /* The identifier is guaranteed to be unique because we use 2 directory
   * separators between the collection and the data name. Since the collection
   * is either a controlled internal name or built from g_file_get_path(), which
   * is guaranteed to be a canonical path, we know it won't contain 2
   * separators. Therefore it should be impossible to construct a file path able
   * to create duplicate identifiers.
   * The last point is obviously that it should not be possible to have
   * duplicate data names in a single collection. So every identifier should be
   * unique.
   */
  identifier = g_strdup_printf ("%s:%s%s%s%s",
                                private->internal ? "internal" : "external",
                                collection, G_DIR_SEPARATOR_S, G_DIR_SEPARATOR_S,
                                gimp_object_get_name (GIMP_OBJECT (data)));
  g_free (collection);

  return identifier;
}

static gchar *
gimp_data_get_checksum (GimpTagged *tagged)
{
  return NULL;
}

/*
 * A data collection name is either generated from the file path, or set when
 * marking a data as internal.
 * Several data objects may belong to a same collection. A very common example
 * of this in fonts are collections of fonts (e.g. TrueType Collection .TTC
 * files).
 */
static gchar *
gimp_data_get_collection (GimpData *data)
{
  GimpDataPrivate *private    = GIMP_DATA_GET_PRIVATE (data);
  gchar           *collection = NULL;

  g_return_val_if_fail (private->internal || private->file != NULL || private->image != NULL, NULL);

  if (private->file)
    {
      const gchar *data_dir = gimp_data_directory ();
      const gchar *gimp_dir = gimp_directory ();
      gchar       *path     = g_file_get_path (private->file);
      gchar       *tmp;

      if (g_str_has_prefix (path, data_dir))
        {
          tmp = g_strconcat ("${gimp_data_dir}",
                             path + strlen (data_dir),
                             NULL);
          collection = g_filename_to_utf8 (tmp, -1, NULL, NULL, NULL);
          g_free (tmp);
        }
      else if (g_str_has_prefix (path, gimp_dir))
        {
          tmp = g_strconcat ("${gimp_dir}",
                             path + strlen (gimp_dir),
                             NULL);
          collection = g_filename_to_utf8 (tmp, -1, NULL, NULL, NULL);
          g_free (tmp);
        }
      else if (g_str_has_prefix (path, MYPAINT_BRUSHES_DIR))
        {
          tmp = g_strconcat ("${mypaint_brushes_dir}",
                             path + strlen (MYPAINT_BRUSHES_DIR),
                             NULL);
          collection = g_filename_to_utf8 (tmp, -1, NULL, NULL, NULL);
          g_free (tmp);
        }
      else
        {
          collection = g_filename_to_utf8 (path, -1,
                                           NULL, NULL, NULL);
        }

      if (! collection)
        {
          g_printerr ("%s: failed to convert '%s' to utf8.\n",
                      G_STRFUNC, path);
          collection = g_strdup (path);
        }

      g_free (path);
    }
  else if (private->image)
    {
      collection = g_strdup_printf ("[image-id-%d]", gimp_image_get_id (private->image));
    }
  else if (private->internal)
    {
      collection = g_strdup (private->collection);
    }

  return collection;
}


/*  public functions  */

gint
gimp_data_get_id (GimpData *data)
{
  g_return_val_if_fail (GIMP_IS_DATA (data), -1);

  return GIMP_DATA_GET_PRIVATE (data)->ID;
}

GimpData *
gimp_data_get_by_id (gint data_id)
{
  return (GimpData *) gimp_id_table_lookup (data_id_table, data_id);
}

/**
 * gimp_data_save:
 * @data:  object whose contents are to be saved.
 * @error: return location for errors or %NULL
 *
 * Save the object.  If the object is marked as "internal", nothing
 * happens.  Otherwise, it is saved to disk, using the file name set
 * by gimp_data_set_file().  If the save is successful, the object is
 * marked as not dirty.  If not, an error message is returned using
 * the @error argument.
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

  if (private->internal || private->image != NULL)
    {
      private->dirty = FALSE;
      return TRUE;
    }

  g_return_val_if_fail (G_IS_FILE (private->file), FALSE);

  if (GIMP_DATA_GET_CLASS (data)->save)
    {
      GOutputStream *output;

      output = G_OUTPUT_STREAM (g_file_replace (private->file,
                                                NULL, FALSE, G_FILE_CREATE_NONE,
                                                NULL, error));

      if (output)
        {
          success = GIMP_DATA_GET_CLASS (data)->save (data, output, error);

          if (success)
            {
              if (! g_output_stream_close (output, NULL, error))
                {
                  g_prefix_error (error,
                                  _("Error saving '%s': "),
                                  gimp_file_get_utf8_name (private->file));
                  success = FALSE;
                }
            }
          else
            {
              GCancellable *cancellable = g_cancellable_new ();

              g_cancellable_cancel (cancellable);
              if (error && *error)
                {
                  g_prefix_error (error,
                                  _("Error saving '%s': "),
                                  gimp_file_get_utf8_name (private->file));
                }
              else
                {
                  g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_WRITE,
                               _("Error saving '%s'"),
                               gimp_file_get_utf8_name (private->file));
                }
              g_output_stream_close (output, cancellable, NULL);
              g_object_unref (cancellable);
            }

          g_object_unref (output);
        }
    }

  if (success)
    {
      GFileInfo *info = g_file_query_info (private->file,
                                           G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                           G_FILE_QUERY_INFO_NONE,
                                           NULL, NULL);
      if (info)
        {
          private->mtime =
            g_file_info_get_attribute_uint64 (info,
                                              G_FILE_ATTRIBUTE_TIME_MODIFIED);
          g_object_unref (info);
        }

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
 * set by gimp_data_set_file(), it is deleted.  Obviously this is
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

  g_return_val_if_fail (private->file      != NULL, FALSE);
  g_return_val_if_fail (private->deletable == TRUE, FALSE);

  if (private->internal)
    return TRUE;

  return g_file_delete (private->file, NULL, error);
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
 * gimp_data_set_file:
 * @data:     A #GimpData object
 * @file:     File to assign to @data.
 * @writable: %TRUE if we want to be able to write to this file.
 * @deletable: %TRUE if we want to be able to delete this file.
 *
 * This function assigns a file to @data, and sets some flags
 * according to the properties of the file.  If @writable is %TRUE,
 * and the user has permission to write or overwrite the requested
 * file name, and a "save" method exists for @data's object type, then
 * @data is marked as writable.
 **/
void
gimp_data_set_file (GimpData *data,
                    GFile    *file,
                    gboolean  writable,
                    gboolean  deletable)
{
  GimpDataPrivate *private;
  gchar           *path;

  g_return_if_fail (GIMP_IS_DATA (data));
  g_return_if_fail (G_IS_FILE (file));

  path = g_file_get_path (file);

  g_return_if_fail (path != NULL);
  g_return_if_fail (g_path_is_absolute (path));

  g_free (path);

  private = GIMP_DATA_GET_PRIVATE (data);

  if (private->internal)
    return;

  g_return_if_fail (private->image == NULL);

  g_set_object (&private->file, file);

  private->writable  = FALSE;
  private->deletable = FALSE;

  /*  if the data is supposed to be writable or deletable,
   *  still check if it really is
   */
  if (writable || deletable)
    {
      GFileInfo *info;

      if (g_file_query_exists (private->file, NULL)) /* check if it exists */
        {
          info = g_file_query_info (private->file,
                                    G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
                                    G_FILE_QUERY_INFO_NONE,
                                    NULL, NULL);

          /* and we can write it */
          if (info)
            {
              if (g_file_info_get_attribute_boolean (info,
                                                     G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE))
                {
                  private->writable  = writable  ? TRUE : FALSE;
                  private->deletable = deletable ? TRUE : FALSE;
                }

              g_object_unref (info);
            }
        }
      else /* OR it doesn't exist */
        {
          GFile *parent = g_file_get_parent (private->file);

          info = g_file_query_info (parent,
                                    G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
                                    G_FILE_QUERY_INFO_NONE,
                                    NULL, NULL);

          /* and we can write to its parent directory */
          if (info)
            {
              if (g_file_info_get_attribute_boolean (info,
                                                     G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE))
                {
                  private->writable  = writable  ? TRUE : FALSE;
                  private->deletable = deletable ? TRUE : FALSE;
                }

              g_object_unref (info);
            }

          g_object_unref (parent);
        }

      /*  if we can't save, we are not writable  */
      if (! GIMP_DATA_GET_CLASS (data)->save)
        private->writable = FALSE;
    }
}

GFile *
gimp_data_get_file (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_val_if_fail (GIMP_IS_DATA (data), NULL);

  private = GIMP_DATA_GET_PRIVATE (data);

  return private->file;
}

/**
 * gimp_data_set_image:
 * @data:     A #GimpData object
 * @image:    Image to assign to @data.
 * @writable: %TRUE if we want to be able to write to this file.
 * @deletable: %TRUE if we want to be able to delete this file.
 *
 * This function assigns an image to @data. This can only be done if no file has
 * been assigned (a non-internal data can be attached either to a file or to an
 * image).
 **/
void
gimp_data_set_image (GimpData  *data,
                     GimpImage *image,
                     gboolean   writable,
                     gboolean   deletable)
{
  GimpDataPrivate *private;

  g_return_if_fail (GIMP_IS_DATA (data));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_DATA_GET_PRIVATE (data);

  if (private->internal)
    return;

  g_return_if_fail (private->file == NULL);

  g_set_weak_pointer (&private->image, image);

  private->writable  = writable  ? TRUE : FALSE;
  private->deletable = deletable ? TRUE : FALSE;

  g_object_notify (G_OBJECT (data), "image");
}

GimpImage *
gimp_data_get_image (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_val_if_fail (GIMP_IS_DATA (data), NULL);

  private = GIMP_DATA_GET_PRIVATE (data);

  return private->image;
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
gimp_data_create_filename (GimpData *data,
                           GFile    *dest_dir)
{
  GimpDataPrivate *private;
  gchar           *safename;
  gchar           *basename;
  GFile           *file;
  gint             i;
  gint             unum  = 1;
  GError          *error = NULL;

  g_return_if_fail (GIMP_IS_DATA (data));
  g_return_if_fail (G_IS_FILE (dest_dir));

  private = GIMP_DATA_GET_PRIVATE (data);

  if (private->internal)
    return;

  safename = g_strstrip (g_strdup (gimp_object_get_name (data)));

  if (safename[0] == '.')
    safename[0] = '-';

  for (i = 0; safename[i]; i++)
    if (strchr ("\\/*?\"`'<>{}|\n\t ;:$^&", safename[i]))
      safename[i] = '-';

  basename = g_strconcat (safename, gimp_data_get_extension (data), NULL);

  file = g_file_get_child_for_display_name (dest_dir, basename, &error);
  g_free (basename);

  if (! file)
    {
      g_warning ("gimp_data_create_filename:\n"
                 "g_file_get_child_for_display_name() failed for '%s': %s",
                 gimp_object_get_name (data), error->message);
      g_clear_error (&error);
      g_free (safename);
      return;
    }

  while (g_file_query_exists (file, NULL))
    {
      g_object_unref (file);

      basename = g_strdup_printf ("%s-%d%s",
                                  safename,
                                  unum++,
                                  gimp_data_get_extension (data));

      file = g_file_get_child_for_display_name (dest_dir, basename, NULL);
      g_free (basename);
    }

  g_free (safename);

  gimp_data_set_file (data, file, TRUE, TRUE);

  g_object_unref (file);
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
 *                 hierarchy.
 *
 * Sets tags based on all folder names below top_directory. So if the
 * data's filename is e.g.
 * /home/foo/.config/GIMP/X.Y/brushes/Flowers/Roses/rose.gbr, it will
 * add "Flowers" and "Roses" tags.
 *
 * if the top directory (as passed, or as derived from the data's
 * filename) does not end with one of the default data directory names
 * (brushes, patterns etc), its name will be added as tag too.
 **/
void
gimp_data_set_folder_tags (GimpData *data,
                           GFile    *top_directory)
{
  GimpDataPrivate *private;
  gchar           *tmp;
  gchar           *dirname;
  gchar           *top_path;

  g_return_if_fail (GIMP_IS_DATA (data));
  g_return_if_fail (G_IS_FILE (top_directory));

  private = GIMP_DATA_GET_PRIVATE (data);

  if (private->internal)
    return;

  g_return_if_fail (private->file != NULL);

  tmp = g_file_get_path (private->file);
  dirname = g_path_get_dirname (tmp);
  g_free (tmp);

  top_path = g_file_get_path (top_directory);

  g_return_if_fail (g_str_has_prefix (dirname, top_path));

  /*  walk up the hierarchy and set each folder on the way as tag,
   *  except the top_directory
   */
  while (strcmp (dirname, top_path))
    {
      gchar   *basename = g_path_get_basename (dirname);
      GimpTag *tag      = gimp_tag_new (basename);

      gimp_tag_set_internal (tag, TRUE);
      gimp_tagged_add_tag (GIMP_TAGGED (data), tag);
      g_object_unref (tag);
      g_free (basename);

      tmp = g_path_get_dirname (dirname);
      g_free (dirname);
      dirname = tmp;
    }

  g_free (top_path);

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
                     gint64    mtime)
{
  GimpDataPrivate *private;

  g_return_if_fail (GIMP_IS_DATA (data));

  private = GIMP_DATA_GET_PRIVATE (data);

  private->mtime = mtime;
}

gint64
gimp_data_get_mtime (GimpData *data)
{
  GimpDataPrivate *private;

  g_return_val_if_fail (GIMP_IS_DATA (data), 0);

  private = GIMP_DATA_GET_PRIVATE (data);

  return private->mtime;
}

gboolean
gimp_data_is_copyable (GimpData *data)
{
  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);

  return GIMP_DATA_GET_CLASS (data)->copy != NULL;
}

/**
 * gimp_data_copy:
 * @data:     a #GimpData object
 * @src_data: the #GimpData object to copy from
 *
 * Copies @src_data to @data.  Only the object data is  copied:  the
 * name, file name, preview, etc. are not copied.
 **/
void
gimp_data_copy (GimpData *data,
                GimpData *src_data)
{
  g_return_if_fail (GIMP_IS_DATA (data));
  g_return_if_fail (GIMP_IS_DATA (src_data));
  g_return_if_fail (GIMP_DATA_GET_CLASS (data)->copy != NULL);
  g_return_if_fail (GIMP_DATA_GET_CLASS (data)->copy ==
                    GIMP_DATA_GET_CLASS (src_data)->copy);

  if (data != src_data)
    GIMP_DATA_GET_CLASS (data)->copy (data, src_data);
}

gboolean
gimp_data_is_duplicatable (GimpData *data)
{
  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);

  if (GIMP_DATA_GET_CLASS (data)->duplicate == gimp_data_real_duplicate)
    return gimp_data_is_copyable (data);
  else
    return GIMP_DATA_GET_CLASS (data)->duplicate != NULL;
}

/**
 * gimp_data_duplicate:
 * @data: a #GimpData object
 *
 * Creates a copy of @data, if possible.  Only the object data is
 * copied:  the newly created object is not automatically given an
 * object name, file name, preview, etc.
 *
 * Returns: (nullable) (transfer full): the newly created copy, or %NULL if
 *          @data cannot be copied.
 **/
GimpData *
gimp_data_duplicate (GimpData *data)
{
  g_return_val_if_fail (GIMP_IS_DATA (data), NULL);

  if (gimp_data_is_duplicatable (data))
    {
      GimpData        *new     = GIMP_DATA_GET_CLASS (data)->duplicate (data);
      GimpDataPrivate *private = GIMP_DATA_GET_PRIVATE (new);

      g_object_set (new,
                    "name",      NULL,
                    "writable",  GIMP_DATA_GET_CLASS (new)->save != NULL,
                    "deletable", TRUE,
                    NULL);

      g_clear_object (&private->file);

      return new;
    }

  return NULL;
}

/**
 * gimp_data_make_internal:
 * @data:       a #GimpData object.
 * @collection: internal collection title @data belongs to.
 *
 * Mark @data as "internal" to Gimp, which means that it will not be
 * saved to disk.  Note that if you do this, later calls to
 * gimp_data_save() and gimp_data_delete_from_disk() will
 * automatically return successfully without giving any warning.
 *
 * The @collection shall be an untranslated globally unique string
 * that identifies the internal object collection across sessions.
 **/
void
gimp_data_make_internal (GimpData    *data,
                         const gchar *collection)
{
  GimpDataPrivate *private;

  g_return_if_fail (GIMP_IS_DATA (data));

  private = GIMP_DATA_GET_PRIVATE (data);

  g_clear_object (&private->file);

  g_free (private->collection);
  private->collection = g_strdup (collection);

  private->writable  = FALSE;
  private->deletable = FALSE;
  private->internal  = TRUE;
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
 * Returns: -1 if @data1 compares before @data2,
 *                0 if they compare equal,
 *                1 if @data1 compares after @data2.
 **/
gint
gimp_data_compare (GimpData *data1,
                   GimpData *data2)
{
  g_return_val_if_fail (GIMP_IS_DATA (data1), 0);
  g_return_val_if_fail (GIMP_IS_DATA (data2), 0);
  g_return_val_if_fail (GIMP_DATA_GET_CLASS (data1)->compare ==
                        GIMP_DATA_GET_CLASS (data2)->compare, 0);

  return GIMP_DATA_GET_CLASS (data1)->compare (data1, data2);
}

/**
 * gimp_data_identify:
 * @data:        a #GimpData object.
 * @name:        name of the #GimpData object.
 * @collection:  text uniquely identifying the collection @data belongs to.
 * @is_internal: whether this is internal data.
 *
 * Determine whether (@name, @collection, @is_internal) uniquely identify @data.
 *
 * Returns: %TRUE if the triplet identifies @data, %FALSE otherwise.
 **/
gboolean
gimp_data_identify (GimpData    *data,
                    const gchar *name,
                    const gchar *collection,
                    gboolean     is_internal)
{
  gchar    *current_collection = gimp_data_get_collection (data);
  gboolean  identified;

  identified = (is_internal == gimp_data_is_internal (data)      &&
                g_strcmp0 (collection, current_collection) == 0  &&
                /* Internal data have unique collection names. Moreover
                 * their names can be localized so it should not be
                 * relied upon for comparison.
                 */
                (is_internal ? TRUE : g_strcmp0 (name, gimp_object_get_name (GIMP_OBJECT (data))) == 0));

  g_free (current_collection);

  return identified;
}

/**
 * gimp_data_get_identifiers:
 * @data:        a #GimpData object.
 * @name:        name of the #GimpData object.
 * @collection:  text uniquely identifying the collection @data belongs to.
 * @is_internal: whether this is internal data.
 *
 * Generates a triplet of identifiers which, together, should uniquely identify
 * this @data.
 * @name will be the same value as gimp_object_get_name() and @is_internal the
 * same value as returned by gimp_data_is_internal(), except that it is not
 * enough because two data from different sources may end up having the same
 * name. Nevertheless all data names within a single collection of data are
 * unique. @collection therefore identifies the source collection. And these 3
 * identifiers together are enough to identify a GimpData.
 *
 * Internally the collection will likely be a single file name, therefore
 * @collection will be constructed from the file name (if it exists, or an
 * opaque identifier string otherwise, for internal data). Nevertheless you
 * should not take this for granted and should always use this string as an
 * opaque identifier only to be reused in gimp_data_identify().
 **/
void
gimp_data_get_identifiers (GimpData  *data,
                           gchar    **name,
                           gchar    **collection,
                           gboolean  *is_internal)
{
  *collection  = gimp_data_get_collection (data);
  *name        = g_strdup (gimp_object_get_name (GIMP_OBJECT (data)));
  *is_internal = gimp_data_is_internal (data);
}
/**
 * gimp_data_error_quark:
 *
 * This function is used to implement the GIMP_DATA_ERROR macro. It
 * shouldn't be called directly.
 *
 * Returns: the #GQuark to identify error in the GimpData error domain.
 **/
GQuark
gimp_data_error_quark (void)
{
  return g_quark_from_static_string ("gimp-data-error-quark");
}
