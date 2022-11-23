/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadata.c
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma-memsize.h"
#include "ligmadata.h"
#include "ligmatag.h"
#include "ligmatagged.h"

#include "ligma-intl.h"


enum
{
  DIRTY,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_FILE,
  PROP_WRITABLE,
  PROP_DELETABLE,
  PROP_MIME_TYPE
};


struct _LigmaDataPrivate
{
  GFile  *file;
  GQuark  mime_type;
  guint   writable  : 1;
  guint   deletable : 1;
  guint   dirty     : 1;
  guint   internal  : 1;
  gint    freeze_count;
  gint64  mtime;

  /* Identifies the LigmaData object across sessions. Used when there
   * is not a filename associated with the object.
   */
  gchar  *identifier;

  GList  *tags;
};

#define LIGMA_DATA_GET_PRIVATE(obj) (((LigmaData *) (obj))->priv)


static void       ligma_data_tagged_iface_init (LigmaTaggedInterface *iface);

static void       ligma_data_constructed       (GObject             *object);
static void       ligma_data_finalize          (GObject             *object);
static void       ligma_data_set_property      (GObject             *object,
                                               guint                property_id,
                                               const GValue        *value,
                                               GParamSpec          *pspec);
static void       ligma_data_get_property      (GObject             *object,
                                               guint                property_id,
                                               GValue              *value,
                                               GParamSpec          *pspec);

static void       ligma_data_name_changed      (LigmaObject          *object);
static gint64     ligma_data_get_memsize       (LigmaObject          *object,
                                               gint64              *gui_size);

static gboolean   ligma_data_is_name_editable  (LigmaViewable        *viewable);

static void       ligma_data_real_dirty        (LigmaData            *data);
static LigmaData * ligma_data_real_duplicate    (LigmaData            *data);
static gint       ligma_data_real_compare      (LigmaData            *data1,
                                               LigmaData            *data2);

static gboolean   ligma_data_add_tag           (LigmaTagged          *tagged,
                                               LigmaTag             *tag);
static gboolean   ligma_data_remove_tag        (LigmaTagged          *tagged,
                                               LigmaTag             *tag);
static GList    * ligma_data_get_tags          (LigmaTagged          *tagged);
static gchar    * ligma_data_get_identifier    (LigmaTagged          *tagged);
static gchar    * ligma_data_get_checksum      (LigmaTagged          *tagged);


G_DEFINE_TYPE_WITH_CODE (LigmaData, ligma_data, LIGMA_TYPE_VIEWABLE,
                         G_ADD_PRIVATE (LigmaData)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_TAGGED,
                                                ligma_data_tagged_iface_init))

#define parent_class ligma_data_parent_class

static guint data_signals[LAST_SIGNAL] = { 0 };


static void
ligma_data_class_init (LigmaDataClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  data_signals[DIRTY] =
    g_signal_new ("dirty",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDataClass, dirty),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed        = ligma_data_constructed;
  object_class->finalize           = ligma_data_finalize;
  object_class->set_property       = ligma_data_set_property;
  object_class->get_property       = ligma_data_get_property;

  ligma_object_class->name_changed  = ligma_data_name_changed;
  ligma_object_class->get_memsize   = ligma_data_get_memsize;

  viewable_class->name_editable    = TRUE;
  viewable_class->is_name_editable = ligma_data_is_name_editable;

  klass->dirty                     = ligma_data_real_dirty;
  klass->save                      = NULL;
  klass->get_extension             = NULL;
  klass->copy                      = NULL;
  klass->duplicate                 = ligma_data_real_duplicate;
  klass->compare                   = ligma_data_real_compare;

  g_object_class_install_property (object_class, PROP_FILE,
                                   g_param_spec_object ("file", NULL, NULL,
                                                        G_TYPE_FILE,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_WRITABLE,
                                   g_param_spec_boolean ("writable", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_DELETABLE,
                                   g_param_spec_boolean ("deletable", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_MIME_TYPE,
                                   g_param_spec_string ("mime-type", NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_data_tagged_iface_init (LigmaTaggedInterface *iface)
{
  iface->add_tag        = ligma_data_add_tag;
  iface->remove_tag     = ligma_data_remove_tag;
  iface->get_tags       = ligma_data_get_tags;
  iface->get_identifier = ligma_data_get_identifier;
  iface->get_checksum   = ligma_data_get_checksum;
}

static void
ligma_data_init (LigmaData *data)
{
  LigmaDataPrivate *private = ligma_data_get_instance_private (data);

  data->priv = private;

  private->writable  = TRUE;
  private->deletable = TRUE;
  private->dirty     = TRUE;

  /*  freeze the data object during construction  */
  ligma_data_freeze (data);
}

static void
ligma_data_constructed (GObject *object)
{
  LigmaDataPrivate *private = LIGMA_DATA_GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  if (! LIGMA_DATA_GET_CLASS (object)->save)
    private->writable = FALSE;

  ligma_data_thaw (LIGMA_DATA (object));
}

static void
ligma_data_finalize (GObject *object)
{
  LigmaDataPrivate *private = LIGMA_DATA_GET_PRIVATE (object);

  g_clear_object (&private->file);

  if (private->tags)
    {
      g_list_free_full (private->tags, (GDestroyNotify) g_object_unref);
      private->tags = NULL;
    }

  g_clear_pointer (&private->identifier, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_data_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  LigmaData        *data    = LIGMA_DATA (object);
  LigmaDataPrivate *private = LIGMA_DATA_GET_PRIVATE (data);

  switch (property_id)
    {
    case PROP_FILE:
      ligma_data_set_file (data,
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
ligma_data_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  LigmaDataPrivate *private = LIGMA_DATA_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_FILE:
      g_value_set_object (value, private->file);
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
ligma_data_name_changed (LigmaObject *object)
{
  LigmaDataPrivate *private = LIGMA_DATA_GET_PRIVATE (object);

  private->dirty = TRUE;

  if (LIGMA_OBJECT_CLASS (parent_class)->name_changed)
    LIGMA_OBJECT_CLASS (parent_class)->name_changed (object);
}

static gint64
ligma_data_get_memsize (LigmaObject *object,
                       gint64     *gui_size)
{
  LigmaDataPrivate *private = LIGMA_DATA_GET_PRIVATE (object);
  gint64           memsize = 0;

  memsize += ligma_g_object_get_memsize (G_OBJECT (private->file));

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
ligma_data_is_name_editable (LigmaViewable *viewable)
{
  return ligma_data_is_writable (LIGMA_DATA (viewable)) &&
         ! ligma_data_is_internal (LIGMA_DATA (viewable));
}

static void
ligma_data_real_dirty (LigmaData *data)
{
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (data));

  /* Emit the "name-changed" to signal general dirtiness, our name
   * changed implementation will also set the "dirty" flag to TRUE.
   */
  ligma_object_name_changed (LIGMA_OBJECT (data));
}

static LigmaData *
ligma_data_real_duplicate (LigmaData *data)
{
  if (LIGMA_DATA_GET_CLASS (data)->copy)
    {
      LigmaData *new = g_object_new (G_OBJECT_TYPE (data), NULL);

      ligma_data_copy (new, data);

      return new;
    }

  return NULL;
}

static gint
ligma_data_real_compare (LigmaData *data1,
                        LigmaData *data2)
{
  LigmaDataPrivate *private1 = LIGMA_DATA_GET_PRIVATE (data1);
  LigmaDataPrivate *private2 = LIGMA_DATA_GET_PRIVATE (data2);

  /*  move the internal objects (like the FG -> BG) gradient) to the top  */
  if (private1->internal != private2->internal)
    return private1->internal ? -1 : 1;

  /*  keep user-deletable objects above system resource files  */
  if (private1->deletable != private2->deletable)
    return private1->deletable ? -1 : 1;

  return ligma_object_name_collate ((LigmaObject *) data1,
                                   (LigmaObject *) data2);
}

static gboolean
ligma_data_add_tag (LigmaTagged *tagged,
                   LigmaTag    *tag)
{
  LigmaDataPrivate *private = LIGMA_DATA_GET_PRIVATE (tagged);
  GList           *list;

  for (list = private->tags; list; list = g_list_next (list))
    {
      LigmaTag *this = LIGMA_TAG (list->data);

      if (ligma_tag_equals (tag, this))
        return FALSE;
    }

  private->tags = g_list_prepend (private->tags, g_object_ref (tag));

  return TRUE;
}

static gboolean
ligma_data_remove_tag (LigmaTagged *tagged,
                      LigmaTag    *tag)
{
  LigmaDataPrivate *private = LIGMA_DATA_GET_PRIVATE (tagged);
  GList           *list;

  for (list = private->tags; list; list = g_list_next (list))
    {
      LigmaTag *this = LIGMA_TAG (list->data);

      if (ligma_tag_equals (tag, this))
        {
          private->tags = g_list_delete_link (private->tags, list);
          g_object_unref (this);
          return TRUE;
        }
    }

  return FALSE;
}

static GList *
ligma_data_get_tags (LigmaTagged *tagged)
{
  LigmaDataPrivate *private = LIGMA_DATA_GET_PRIVATE (tagged);

  return private->tags;
}

static gchar *
ligma_data_get_identifier (LigmaTagged *tagged)
{
  LigmaDataPrivate *private    = LIGMA_DATA_GET_PRIVATE (tagged);
  gchar           *identifier = NULL;

  if (private->file)
    {
      const gchar *data_dir = ligma_data_directory ();
      const gchar *ligma_dir = ligma_directory ();
      gchar       *path     = g_file_get_path (private->file);
      gchar       *tmp;

      if (g_str_has_prefix (path, data_dir))
        {
          tmp = g_strconcat ("${ligma_data_dir}",
                             path + strlen (data_dir),
                             NULL);
          identifier = g_filename_to_utf8 (tmp, -1, NULL, NULL, NULL);
          g_free (tmp);
        }
      else if (g_str_has_prefix (path, ligma_dir))
        {
          tmp = g_strconcat ("${ligma_dir}",
                             path + strlen (ligma_dir),
                             NULL);
          identifier = g_filename_to_utf8 (tmp, -1, NULL, NULL, NULL);
          g_free (tmp);
        }
      else if (g_str_has_prefix (path, MYPAINT_BRUSHES_DIR))
        {
          tmp = g_strconcat ("${mypaint_brushes_dir}",
                             path + strlen (MYPAINT_BRUSHES_DIR),
                             NULL);
          identifier = g_filename_to_utf8 (tmp, -1, NULL, NULL, NULL);
          g_free (tmp);
        }
      else
        {
          identifier = g_filename_to_utf8 (path, -1,
                                           NULL, NULL, NULL);
        }

      if (! identifier)
        {
          g_printerr ("%s: failed to convert '%s' to utf8.\n",
                      G_STRFUNC, path);
          identifier = g_strdup (path);
        }

      g_free (path);
    }
  else if (private->internal)
    {
      identifier = g_strdup (private->identifier);
    }

  return identifier;
}

static gchar *
ligma_data_get_checksum (LigmaTagged *tagged)
{
  return NULL;
}

/**
 * ligma_data_save:
 * @data:  object whose contents are to be saved.
 * @error: return location for errors or %NULL
 *
 * Save the object.  If the object is marked as "internal", nothing
 * happens.  Otherwise, it is saved to disk, using the file name set
 * by ligma_data_set_file().  If the save is successful, the object is
 * marked as not dirty.  If not, an error message is returned using
 * the @error argument.
 *
 * Returns: %TRUE if the object is internal or the save is successful.
 **/
gboolean
ligma_data_save (LigmaData  *data,
                GError   **error)
{
  LigmaDataPrivate *private;
  gboolean         success = FALSE;

  g_return_val_if_fail (LIGMA_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  private = LIGMA_DATA_GET_PRIVATE (data);

  g_return_val_if_fail (private->writable == TRUE, FALSE);

  if (private->internal)
    {
      private->dirty = FALSE;
      return TRUE;
    }

  g_return_val_if_fail (G_IS_FILE (private->file), FALSE);

  if (LIGMA_DATA_GET_CLASS (data)->save)
    {
      GOutputStream *output;

      output = G_OUTPUT_STREAM (g_file_replace (private->file,
                                                NULL, FALSE, G_FILE_CREATE_NONE,
                                                NULL, error));

      if (output)
        {
          success = LIGMA_DATA_GET_CLASS (data)->save (data, output, error);

          if (success)
            {
              if (! g_output_stream_close (output, NULL, error))
                {
                  g_prefix_error (error,
                                  _("Error saving '%s': "),
                                  ligma_file_get_utf8_name (private->file));
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
                                  ligma_file_get_utf8_name (private->file));
                }
              else
                {
                  g_set_error (error, LIGMA_DATA_ERROR, LIGMA_DATA_ERROR_WRITE,
                               _("Error saving '%s'"),
                               ligma_file_get_utf8_name (private->file));
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
 * ligma_data_dirty:
 * @data: a #LigmaData object.
 *
 * Marks @data as dirty.  Unless the object is frozen, this causes
 * its preview to be invalidated, and emits a "dirty" signal.  If the
 * object is frozen, the function has no effect.
 **/
void
ligma_data_dirty (LigmaData *data)
{
  LigmaDataPrivate *private;

  g_return_if_fail (LIGMA_IS_DATA (data));

  private = LIGMA_DATA_GET_PRIVATE (data);

  if (private->freeze_count == 0)
    g_signal_emit (data, data_signals[DIRTY], 0);
}

void
ligma_data_clean (LigmaData *data)
{
  LigmaDataPrivate *private;

  g_return_if_fail (LIGMA_IS_DATA (data));

  private = LIGMA_DATA_GET_PRIVATE (data);

  private->dirty = FALSE;
}

gboolean
ligma_data_is_dirty (LigmaData *data)
{
  LigmaDataPrivate *private;

  g_return_val_if_fail (LIGMA_IS_DATA (data), FALSE);

  private = LIGMA_DATA_GET_PRIVATE (data);

  return private->dirty;
}

/**
 * ligma_data_freeze:
 * @data: a #LigmaData object.
 *
 * Increments the freeze count for the object.  A positive freeze count
 * prevents the object from being treated as dirty.  Any call to this
 * function must be followed eventually by a call to ligma_data_thaw().
 **/
void
ligma_data_freeze (LigmaData *data)
{
  LigmaDataPrivate *private;

  g_return_if_fail (LIGMA_IS_DATA (data));

  private = LIGMA_DATA_GET_PRIVATE (data);

  private->freeze_count++;
}

/**
 * ligma_data_thaw:
 * @data: a #LigmaData object.
 *
 * Decrements the freeze count for the object.  If the freeze count
 * drops to zero, the object is marked as dirty, and the "dirty"
 * signal is emitted.  It is an error to call this function without
 * having previously called ligma_data_freeze().
 **/
void
ligma_data_thaw (LigmaData *data)
{
  LigmaDataPrivate *private;

  g_return_if_fail (LIGMA_IS_DATA (data));

  private = LIGMA_DATA_GET_PRIVATE (data);

  g_return_if_fail (private->freeze_count > 0);

  private->freeze_count--;

  if (private->freeze_count == 0)
    ligma_data_dirty (data);
}

gboolean
ligma_data_is_frozen (LigmaData *data)
{
  LigmaDataPrivate *private;

  g_return_val_if_fail (LIGMA_IS_DATA (data), FALSE);

  private = LIGMA_DATA_GET_PRIVATE (data);

  return private->freeze_count > 0;
}

/**
 * ligma_data_delete_from_disk:
 * @data:  a #LigmaData object.
 * @error: return location for errors or %NULL
 *
 * Deletes the object from disk.  If the object is marked as "internal",
 * nothing happens.  Otherwise, if the file exists whose name has been
 * set by ligma_data_set_file(), it is deleted.  Obviously this is
 * a potentially dangerous function, which should be used with care.
 *
 * Returns: %TRUE if the object is internal to Ligma, or the deletion is
 *          successful.
 **/
gboolean
ligma_data_delete_from_disk (LigmaData  *data,
                            GError   **error)
{
  LigmaDataPrivate *private;

  g_return_val_if_fail (LIGMA_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  private = LIGMA_DATA_GET_PRIVATE (data);

  g_return_val_if_fail (private->file      != NULL, FALSE);
  g_return_val_if_fail (private->deletable == TRUE, FALSE);

  if (private->internal)
    return TRUE;

  return g_file_delete (private->file, NULL, error);
}

const gchar *
ligma_data_get_extension (LigmaData *data)
{
  g_return_val_if_fail (LIGMA_IS_DATA (data), NULL);

  if (LIGMA_DATA_GET_CLASS (data)->get_extension)
    return LIGMA_DATA_GET_CLASS (data)->get_extension (data);

  return NULL;
}

/**
 * ligma_data_set_file:
 * @data:     A #LigmaData object
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
ligma_data_set_file (LigmaData *data,
                    GFile    *file,
                    gboolean  writable,
                    gboolean  deletable)
{
  LigmaDataPrivate *private;
  gchar           *path;

  g_return_if_fail (LIGMA_IS_DATA (data));
  g_return_if_fail (G_IS_FILE (file));

  path = g_file_get_path (file);

  g_return_if_fail (path != NULL);
  g_return_if_fail (g_path_is_absolute (path));

  g_free (path);

  private = LIGMA_DATA_GET_PRIVATE (data);

  if (private->internal)
    return;

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
      if (! LIGMA_DATA_GET_CLASS (data)->save)
        private->writable = FALSE;
    }
}

GFile *
ligma_data_get_file (LigmaData *data)
{
  LigmaDataPrivate *private;

  g_return_val_if_fail (LIGMA_IS_DATA (data), NULL);

  private = LIGMA_DATA_GET_PRIVATE (data);

  return private->file;
}

/**
 * ligma_data_create_filename:
 * @data:     a #Ligmadata object.
 * @dest_dir: directory in which to create a file name.
 *
 * This function creates a unique file name to be used for saving
 * a representation of @data in the directory @dest_dir.  If the
 * user does not have write permission in @dest_dir, then @data
 * is marked as "not writable", so you should check on this before
 * assuming that @data can be saved.
 **/
void
ligma_data_create_filename (LigmaData *data,
                           GFile    *dest_dir)
{
  LigmaDataPrivate *private;
  gchar           *safename;
  gchar           *basename;
  GFile           *file;
  gint             i;
  gint             unum  = 1;
  GError          *error = NULL;

  g_return_if_fail (LIGMA_IS_DATA (data));
  g_return_if_fail (G_IS_FILE (dest_dir));

  private = LIGMA_DATA_GET_PRIVATE (data);

  if (private->internal)
    return;

  safename = g_strstrip (g_strdup (ligma_object_get_name (data)));

  if (safename[0] == '.')
    safename[0] = '-';

  for (i = 0; safename[i]; i++)
    if (strchr ("\\/*?\"`'<>{}|\n\t ;:$^&", safename[i]))
      safename[i] = '-';

  basename = g_strconcat (safename, ligma_data_get_extension (data), NULL);

  file = g_file_get_child_for_display_name (dest_dir, basename, &error);
  g_free (basename);

  if (! file)
    {
      g_warning ("ligma_data_create_filename:\n"
                 "g_file_get_child_for_display_name() failed for '%s': %s",
                 ligma_object_get_name (data), error->message);
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
                                  ligma_data_get_extension (data));

      file = g_file_get_child_for_display_name (dest_dir, basename, NULL);
      g_free (basename);
    }

  g_free (safename);

  ligma_data_set_file (data, file, TRUE, TRUE);

  g_object_unref (file);
}

static const gchar *tag_blacklist[] = { "brushes",
                                        "dynamics",
                                        "patterns",
                                        "palettes",
                                        "gradients",
                                        "tool-presets" };

/**
 * ligma_data_set_folder_tags:
 * @data:          a #Ligmadata object.
 * @top_directory: the top directory of the currently processed data
 *                 hierarchy.
 *
 * Sets tags based on all folder names below top_directory. So if the
 * data's filename is e.g.
 * /home/foo/.config/LIGMA/X.Y/brushes/Flowers/Roses/rose.gbr, it will
 * add "Flowers" and "Roses" tags.
 *
 * if the top directory (as passed, or as derived from the data's
 * filename) does not end with one of the default data directory names
 * (brushes, patterns etc), its name will be added as tag too.
 **/
void
ligma_data_set_folder_tags (LigmaData *data,
                           GFile    *top_directory)
{
  LigmaDataPrivate *private;
  gchar           *tmp;
  gchar           *dirname;
  gchar           *top_path;

  g_return_if_fail (LIGMA_IS_DATA (data));
  g_return_if_fail (G_IS_FILE (top_directory));

  private = LIGMA_DATA_GET_PRIVATE (data);

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
      LigmaTag *tag      = ligma_tag_new (basename);

      ligma_tag_set_internal (tag, TRUE);
      ligma_tagged_add_tag (LIGMA_TAGGED (data), tag);
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
          LigmaTag *tag = ligma_tag_new (basename);

          ligma_tag_set_internal (tag, TRUE);
          ligma_tagged_add_tag (LIGMA_TAGGED (data), tag);
          g_object_unref (tag);
        }

      g_free (basename);
      g_free (dirname);
    }
}

const gchar *
ligma_data_get_mime_type (LigmaData *data)
{
  LigmaDataPrivate *private;

  g_return_val_if_fail (LIGMA_IS_DATA (data), NULL);

  private = LIGMA_DATA_GET_PRIVATE (data);

  return g_quark_to_string (private->mime_type);
}

gboolean
ligma_data_is_writable (LigmaData *data)
{
  LigmaDataPrivate *private;

  g_return_val_if_fail (LIGMA_IS_DATA (data), FALSE);

  private = LIGMA_DATA_GET_PRIVATE (data);

  return private->writable;
}

gboolean
ligma_data_is_deletable (LigmaData *data)
{
  LigmaDataPrivate *private;

  g_return_val_if_fail (LIGMA_IS_DATA (data), FALSE);

  private = LIGMA_DATA_GET_PRIVATE (data);

  return private->deletable;
}

void
ligma_data_set_mtime (LigmaData *data,
                     gint64    mtime)
{
  LigmaDataPrivate *private;

  g_return_if_fail (LIGMA_IS_DATA (data));

  private = LIGMA_DATA_GET_PRIVATE (data);

  private->mtime = mtime;
}

gint64
ligma_data_get_mtime (LigmaData *data)
{
  LigmaDataPrivate *private;

  g_return_val_if_fail (LIGMA_IS_DATA (data), 0);

  private = LIGMA_DATA_GET_PRIVATE (data);

  return private->mtime;
}

gboolean
ligma_data_is_copyable (LigmaData *data)
{
  g_return_val_if_fail (LIGMA_IS_DATA (data), FALSE);

  return LIGMA_DATA_GET_CLASS (data)->copy != NULL;
}

/**
 * ligma_data_copy:
 * @data:     a #LigmaData object
 * @src_data: the #LigmaData object to copy from
 *
 * Copies @src_data to @data.  Only the object data is  copied:  the
 * name, file name, preview, etc. are not copied.
 **/
void
ligma_data_copy (LigmaData *data,
                LigmaData *src_data)
{
  g_return_if_fail (LIGMA_IS_DATA (data));
  g_return_if_fail (LIGMA_IS_DATA (src_data));
  g_return_if_fail (LIGMA_DATA_GET_CLASS (data)->copy != NULL);
  g_return_if_fail (LIGMA_DATA_GET_CLASS (data)->copy ==
                    LIGMA_DATA_GET_CLASS (src_data)->copy);

  if (data != src_data)
    LIGMA_DATA_GET_CLASS (data)->copy (data, src_data);
}

gboolean
ligma_data_is_duplicatable (LigmaData *data)
{
  g_return_val_if_fail (LIGMA_IS_DATA (data), FALSE);

  if (LIGMA_DATA_GET_CLASS (data)->duplicate == ligma_data_real_duplicate)
    return ligma_data_is_copyable (data);
  else
    return LIGMA_DATA_GET_CLASS (data)->duplicate != NULL;
}

/**
 * ligma_data_duplicate:
 * @data: a #LigmaData object
 *
 * Creates a copy of @data, if possible.  Only the object data is
 * copied:  the newly created object is not automatically given an
 * object name, file name, preview, etc.
 *
 * Returns: (nullable) (transfer full): the newly created copy, or %NULL if
 *          @data cannot be copied.
 **/
LigmaData *
ligma_data_duplicate (LigmaData *data)
{
  g_return_val_if_fail (LIGMA_IS_DATA (data), NULL);

  if (ligma_data_is_duplicatable (data))
    {
      LigmaData        *new     = LIGMA_DATA_GET_CLASS (data)->duplicate (data);
      LigmaDataPrivate *private = LIGMA_DATA_GET_PRIVATE (new);

      g_object_set (new,
                    "name",      NULL,
                    "writable",  LIGMA_DATA_GET_CLASS (new)->save != NULL,
                    "deletable", TRUE,
                    NULL);

      g_clear_object (&private->file);

      return new;
    }

  return NULL;
}

/**
 * ligma_data_make_internal:
 * @data: a #LigmaData object.
 *
 * Mark @data as "internal" to Ligma, which means that it will not be
 * saved to disk.  Note that if you do this, later calls to
 * ligma_data_save() and ligma_data_delete_from_disk() will
 * automatically return successfully without giving any warning.
 *
 * The identifier name shall be an untranslated globally unique string
 * that identifies the internal object across sessions.
 **/
void
ligma_data_make_internal (LigmaData    *data,
                         const gchar *identifier)
{
  LigmaDataPrivate *private;

  g_return_if_fail (LIGMA_IS_DATA (data));

  private = LIGMA_DATA_GET_PRIVATE (data);

  g_clear_object (&private->file);

  g_free (private->identifier);
  private->identifier = g_strdup (identifier);

  private->writable  = FALSE;
  private->deletable = FALSE;
  private->internal  = TRUE;
}

gboolean
ligma_data_is_internal (LigmaData *data)
{
  LigmaDataPrivate *private;

  g_return_val_if_fail (LIGMA_IS_DATA (data), FALSE);

  private = LIGMA_DATA_GET_PRIVATE (data);

  return private->internal;
}

/**
 * ligma_data_compare:
 * @data1: a #LigmaData object.
 * @data2: another #LigmaData object.
 *
 * Compares two data objects for use in sorting. Objects marked as
 * "internal" come first, then user-writable objects, then system data
 * files. In these three groups, the objects are sorted alphabetically
 * by name, using ligma_object_name_collate().
 *
 * Returns: -1 if @data1 compares before @data2,
 *                0 if they compare equal,
 *                1 if @data1 compares after @data2.
 **/
gint
ligma_data_compare (LigmaData *data1,
                   LigmaData *data2)
{
  g_return_val_if_fail (LIGMA_IS_DATA (data1), 0);
  g_return_val_if_fail (LIGMA_IS_DATA (data2), 0);
  g_return_val_if_fail (LIGMA_DATA_GET_CLASS (data1)->compare ==
                        LIGMA_DATA_GET_CLASS (data2)->compare, 0);

  return LIGMA_DATA_GET_CLASS (data1)->compare (data1, data2);
}

/**
 * ligma_data_error_quark:
 *
 * This function is used to implement the LIGMA_DATA_ERROR macro. It
 * shouldn't be called directly.
 *
 * Returns: the #GQuark to identify error in the LigmaData error domain.
 **/
GQuark
ligma_data_error_quark (void)
{
  return g_quark_from_static_string ("ligma-data-error-quark");
}
