/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * file-open.c
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

#include "core/core-types.h"

#include "gegl/ligma-babl.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmadocumentlist.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-merge.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaimagefile.h"
#include "core/ligmalayer.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmaprogress.h"

#include "pdb/ligmapdb.h"

#include "plug-in/ligmapluginmanager-file.h"
#include "plug-in/ligmapluginprocedure.h"

#include "file-import.h"
#include "file-open.h"
#include "file-remote.h"
#include "ligma-file.h"

#include "ligma-intl.h"


static void     file_open_sanitize_image       (LigmaImage           *image,
                                                gboolean             as_new);
static void     file_open_convert_items        (LigmaImage           *dest_image,
                                                const gchar         *basename,
                                                GList               *items);
static GList *  file_open_get_layers           (LigmaImage           *image,
                                                gboolean             merge_visible,
                                                gint                *n_visible);
static gboolean file_open_file_proc_is_import  (LigmaPlugInProcedure *file_proc);


/*  public functions  */

LigmaImage *
file_open_image (Ligma                *ligma,
                 LigmaContext         *context,
                 LigmaProgress        *progress,
                 GFile               *file,
                 gboolean             as_new,
                 LigmaPlugInProcedure *file_proc,
                 LigmaRunMode          run_mode,
                 LigmaPDBStatusType   *status,
                 const gchar        **mime_type,
                 GError             **error)
{
  LigmaValueArray *return_vals;
  GFile          *orig_file;
  LigmaImage      *image       = NULL;
  GFile          *local_file  = NULL;
  gboolean        mounted     = TRUE;
  GError         *my_error    = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (status != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  *status = LIGMA_PDB_EXECUTION_ERROR;

  orig_file = file;

  if (! g_file_is_native (file) &&
      ! file_remote_mount_file (ligma, file, progress, &my_error))
    {
      if (my_error)
        {
          g_printerr ("%s: mounting remote volume failed, trying to download "
                      "the file: %s\n",
                      G_STRFUNC, my_error->message);
          g_clear_error (&my_error);

          mounted = FALSE;
        }
      else
        {
          *status = LIGMA_PDB_CANCEL;

          return NULL;
        }
    }

  /* FIXME enable these tests for remote files again, needs testing */
  if (g_file_is_native (file) &&
      g_file_query_exists (file, NULL))
    {
      GFileInfo *info;

      info = g_file_query_info (file,
                                G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
                                G_FILE_QUERY_INFO_NONE,
                                NULL, error);
      if (! info)
        return NULL;

      if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR)
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Not a regular file"));
          g_object_unref (info);
          return NULL;
        }

      if (! g_file_info_get_attribute_boolean (info,
                                               G_FILE_ATTRIBUTE_ACCESS_CAN_READ))
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Permission denied"));
          g_object_unref (info);
          return NULL;
        }

      g_object_unref (info);
    }

  if (! file_proc)
    file_proc = ligma_plug_in_manager_file_procedure_find (ligma->plug_in_manager,
                                                          LIGMA_FILE_PROCEDURE_GROUP_OPEN,
                                                          file, error);

  if (! file_proc || ! file_proc->handles_remote || ! mounted)
    {
      gchar *my_path = g_file_get_path (file);

      if (! my_path)
        {
          g_clear_error (error);

          local_file = file_remote_download_image (ligma, file, progress,
                                                   &my_error);

          if (! local_file)
            {
              if (my_error)
                g_propagate_error (error, my_error);
              else
                *status = LIGMA_PDB_CANCEL;

              return NULL;
            }

          /*  if we don't have a file proc yet, try again on the local
           *  file
           */
          if (! file_proc)
            file_proc = ligma_plug_in_manager_file_procedure_find (ligma->plug_in_manager,
                                                                  LIGMA_FILE_PROCEDURE_GROUP_OPEN,
                                                                  local_file, error);

          file = local_file;
        }

      g_free (my_path);
    }

  if (! file_proc)
    {
      if (local_file)
        {
          g_file_delete (local_file, NULL, NULL);
          g_object_unref (local_file);
        }

      return NULL;
    }

  if (progress)
    g_object_add_weak_pointer (G_OBJECT (progress), (gpointer) &progress);

  return_vals =
    ligma_pdb_execute_procedure_by_name (ligma->pdb,
                                        context, progress, error,
                                        ligma_object_get_name (file_proc),
                                        LIGMA_TYPE_RUN_MODE, run_mode,
                                        G_TYPE_FILE,        file,
                                        G_TYPE_NONE);

  if (progress)
    g_object_remove_weak_pointer (G_OBJECT (progress), (gpointer) &progress);

  *status = g_value_get_enum (ligma_value_array_index (return_vals, 0));

  if (*status == LIGMA_PDB_SUCCESS && ! file_proc->generic_file_proc)
    image = g_value_get_object (ligma_value_array_index (return_vals, 1));

  if (local_file)
    {
      if (image)
        ligma_image_set_file (image, orig_file);

      g_file_delete (local_file, NULL, NULL);
      g_object_unref (local_file);
    }

  if (*status == LIGMA_PDB_SUCCESS)
    {
      if (image)
        {
          /* Only set the load procedure if it hasn't already been set. */
          if (! ligma_image_get_load_proc (image))
            ligma_image_set_load_proc (image, file_proc);

          file_proc = ligma_image_get_load_proc (image);

          if (mime_type)
            *mime_type = g_slist_nth_data (file_proc->mime_types_list, 0);
        }
      else if (! file_proc->generic_file_proc)
        {
          if (error && ! *error)
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         _("%s plug-in returned SUCCESS but did not "
                           "return an image"),
                         ligma_procedure_get_label (LIGMA_PROCEDURE (file_proc)));

          *status = LIGMA_PDB_EXECUTION_ERROR;
        }
    }
  else if (*status != LIGMA_PDB_CANCEL)
    {
      if (error && ! *error)
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("%s plug-in could not open image"),
                     ligma_procedure_get_label (LIGMA_PROCEDURE (file_proc)));
    }

  ligma_value_array_unref (return_vals);

  if (image)
    {
      ligma_image_undo_disable (image);

      if (file_open_file_proc_is_import (file_proc))
        {
          file_import_image (image, context, orig_file,
                             run_mode == LIGMA_RUN_INTERACTIVE,
                             progress);
        }

      /* Enables undo again */
      file_open_sanitize_image (image, as_new);
    }

  return image;
}

/**
 * file_open_thumbnail:
 * @ligma:
 * @context:
 * @progress:
 * @file:         an image file
 * @size:         requested size of the thumbnail
 * @mime_type:    return location for image MIME type
 * @image_width:  return location for image width
 * @image_height: return location for image height
 * @format:       return location for image format (set to NULL if unknown)
 * @num_layers:   return location for number of layers
 *                (set to -1 if the number of layers is not known)
 * @error:
 *
 * Attempts to load a thumbnail by using a registered thumbnail loader.
 *
 * Returns: the thumbnail image
 */
LigmaImage *
file_open_thumbnail (Ligma           *ligma,
                     LigmaContext    *context,
                     LigmaProgress   *progress,
                     GFile          *file,
                     gint            size,
                     const gchar   **mime_type,
                     gint           *image_width,
                     gint           *image_height,
                     const Babl    **format,
                     gint           *num_layers,
                     GError        **error)
{
  LigmaPlugInProcedure *file_proc;
  LigmaProcedure       *procedure;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (image_width != NULL, NULL);
  g_return_val_if_fail (image_height != NULL, NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (num_layers != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  *image_width  = 0;
  *image_height = 0;
  *format       = NULL;
  *num_layers   = -1;

  file_proc = ligma_plug_in_manager_file_procedure_find (ligma->plug_in_manager,
                                                        LIGMA_FILE_PROCEDURE_GROUP_OPEN,
                                                        file, NULL);

  if (! file_proc || ! file_proc->thumb_loader)
    return NULL;

  procedure = ligma_pdb_lookup_procedure (ligma->pdb, file_proc->thumb_loader);

  if (procedure && procedure->num_args >= 2 && procedure->num_values >= 1)
    {
      LigmaPDBStatusType  status;
      LigmaValueArray    *return_vals;
      LigmaImage         *image = NULL;
      gchar             *uri   = NULL;

      uri = g_file_get_uri (file);

      return_vals =
        ligma_pdb_execute_procedure_by_name (ligma->pdb,
                                            context, progress, error,
                                            ligma_object_get_name (procedure),
                                            G_TYPE_FILE, file,
                                            G_TYPE_INT,  size,
                                            G_TYPE_NONE);

      g_free (uri);

      status = g_value_get_enum (ligma_value_array_index (return_vals, 0));

      if (status == LIGMA_PDB_SUCCESS &&
          LIGMA_VALUE_HOLDS_IMAGE (ligma_value_array_index (return_vals, 1)))
        {
          image = g_value_get_object (ligma_value_array_index (return_vals, 1));

          if (ligma_value_array_length (return_vals) >= 3 &&
              G_VALUE_HOLDS_INT (ligma_value_array_index (return_vals, 2)) &&
              G_VALUE_HOLDS_INT (ligma_value_array_index (return_vals, 3)))
            {
              *image_width =
                MAX (0, g_value_get_int (ligma_value_array_index (return_vals, 2)));

              *image_height =
                MAX (0, g_value_get_int (ligma_value_array_index (return_vals, 3)));

              if (ligma_value_array_length (return_vals) >= 5 &&
                  G_VALUE_HOLDS_INT (ligma_value_array_index (return_vals, 4)))
                {
                  gint value = g_value_get_int (ligma_value_array_index (return_vals, 4));

                  switch (value)
                    {
                    case LIGMA_RGB_IMAGE:
                      *format = ligma_babl_format (LIGMA_RGB,
                                                  LIGMA_PRECISION_U8_NON_LINEAR,
                                                  FALSE, NULL);
                      break;

                    case LIGMA_RGBA_IMAGE:
                      *format = ligma_babl_format (LIGMA_RGB,
                                                  LIGMA_PRECISION_U8_NON_LINEAR,
                                                  TRUE, NULL);
                      break;

                    case LIGMA_GRAY_IMAGE:
                      *format = ligma_babl_format (LIGMA_GRAY,
                                                  LIGMA_PRECISION_U8_NON_LINEAR,
                                                  FALSE, NULL);
                      break;

                    case LIGMA_GRAYA_IMAGE:
                      *format = ligma_babl_format (LIGMA_GRAY,
                                                  LIGMA_PRECISION_U8_NON_LINEAR,
                                                  TRUE, NULL);
                      break;

                    case LIGMA_INDEXED_IMAGE:
                    case LIGMA_INDEXEDA_IMAGE:
                      {
                        const Babl *rgb;
                        const Babl *rgba;

                        babl_new_palette ("-ligma-indexed-format-dummy",
                                          &rgb, &rgba);

                        if (value == LIGMA_INDEXED_IMAGE)
                          *format = rgb;
                        else
                          *format = rgba;
                      }
                      break;

                    default:
                      break;
                    }
                }

              if (ligma_value_array_length (return_vals) >= 6 &&
                  G_VALUE_HOLDS_INT (ligma_value_array_index (return_vals, 5)))
                {
                  *num_layers =
                    MAX (0, g_value_get_int (ligma_value_array_index (return_vals, 5)));
                }
            }

          if (image)
            {
              file_open_sanitize_image (image, FALSE);

              *mime_type = g_slist_nth_data (file_proc->mime_types_list, 0);

#ifdef LIGMA_UNSTABLE
              g_printerr ("opened thumbnail at %d x %d\n",
                          ligma_image_get_width  (image),
                          ligma_image_get_height (image));
#endif
            }
        }

      ligma_value_array_unref (return_vals);

      return image;
    }

  return NULL;
}

LigmaImage *
file_open_with_display (Ligma               *ligma,
                        LigmaContext        *context,
                        LigmaProgress       *progress,
                        GFile              *file,
                        gboolean            as_new,
                        GObject            *monitor,
                        LigmaPDBStatusType  *status,
                        GError            **error)
{
  return file_open_with_proc_and_display (ligma, context, progress,
                                          file, as_new, NULL,
                                          monitor,
                                          status, error);
}

LigmaImage *
file_open_with_proc_and_display (Ligma                *ligma,
                                 LigmaContext         *context,
                                 LigmaProgress        *progress,
                                 GFile               *file,
                                 gboolean             as_new,
                                 LigmaPlugInProcedure *file_proc,
                                 GObject             *monitor,
                                 LigmaPDBStatusType   *status,
                                 GError             **error)
{
  LigmaImage   *image;
  const gchar *mime_type = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (monitor == NULL || G_IS_OBJECT (monitor), NULL);
  g_return_val_if_fail (status != NULL, NULL);

  image = file_open_image (ligma, context, progress,
                           file,
                           as_new,
                           file_proc,
                           LIGMA_RUN_INTERACTIVE,
                           status,
                           &mime_type,
                           error);

  if (image)
    {
      /* If the file was imported we want to set the layer name to the
       * file name. For now, assume that multi-layered imported images
       * have named the layers already, so only rename the layer of
       * single-layered imported files. Note that this will also
       * rename already named layers from e.g. single-layered PSD
       * files. To solve this properly, we would need new file plug-in
       * API.
       */
      if (! file_proc)
        file_proc = ligma_image_get_load_proc (image);

      if (file_open_file_proc_is_import (file_proc) &&
          ligma_image_get_n_layers (image) == 1)
        {
          LigmaObject *layer = ligma_image_get_layer_iter (image)->data;
          gchar      *basename;

          basename = g_path_get_basename (ligma_file_get_utf8_name (file));

          ligma_item_rename (LIGMA_ITEM (layer), basename, NULL);
          ligma_image_undo_free (image);
          ligma_image_clean_all (image);

          g_free (basename);
        }

      if (ligma_create_display (image->ligma, image, LIGMA_UNIT_PIXEL, 1.0,
                               monitor))
        {
          /*  the display owns the image now  */
          g_object_unref (image);
        }

      if (! as_new)
        {
          LigmaDocumentList *documents = LIGMA_DOCUMENT_LIST (ligma->documents);
          LigmaImagefile    *imagefile;
          GFile            *any_file;

          imagefile = ligma_document_list_add_file (documents, file, mime_type);

          /*  can only create a thumbnail if the passed file and the
           *  resulting image's file match. Use any_file() here so we
           *  create thumbnails for both XCF and imported images.
           */
          any_file = ligma_image_get_any_file (image);

          if (any_file && g_file_equal (file, any_file))
            {
              /*  no need to save a thumbnail if there's a good one already  */
              if (! ligma_imagefile_check_thumbnail (imagefile))
                {
                  ligma_imagefile_save_thumbnail (imagefile, mime_type, image,
                                                 NULL);
                }
            }
        }

      /*  announce that we opened this image  */
      ligma_image_opened (image->ligma, file);
    }

  return image;
}

GList *
file_open_layers (Ligma                *ligma,
                  LigmaContext         *context,
                  LigmaProgress        *progress,
                  LigmaImage           *dest_image,
                  gboolean             merge_visible,
                  GFile               *file,
                  LigmaRunMode          run_mode,
                  LigmaPlugInProcedure *file_proc,
                  LigmaPDBStatusType   *status,
                  GError             **error)
{
  LigmaImage   *new_image;
  GList       *layers    = NULL;
  const gchar *mime_type = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (LIGMA_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (status != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  new_image = file_open_image (ligma, context, progress,
                               file, FALSE,
                               file_proc,
                               run_mode,
                               status, &mime_type, error);

  if (new_image)
    {
      gint n_visible = 0;

      ligma_image_undo_disable (new_image);

      layers = file_open_get_layers (new_image, merge_visible, &n_visible);

      if (merge_visible && n_visible > 1)
        {
          g_list_free (layers);

          layers = ligma_image_merge_visible_layers (new_image, context,
                                                    LIGMA_CLIP_TO_IMAGE,
                                                    FALSE, FALSE,
                                                    NULL);
          layers = g_list_copy (layers);
        }

      if (layers)
        {
          gchar *basename;

          basename = g_path_get_basename (ligma_file_get_utf8_name (file));
          file_open_convert_items (dest_image, basename, layers);
          g_free (basename);

          ligma_document_list_add_file (LIGMA_DOCUMENT_LIST (ligma->documents),
                                       file, mime_type);
        }
      else
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Image doesn't contain any layers"));
          *status = LIGMA_PDB_EXECUTION_ERROR;
        }

      g_object_unref (new_image);
    }

  return g_list_reverse (layers);
}


/*  This function is called for filenames passed on the command-line
 *  or from the D-Bus service.
 */
gboolean
file_open_from_command_line (Ligma     *ligma,
                             GFile    *file,
                             gboolean  as_new,
                             GObject  *monitor)

{
  LigmaImage         *image;
  LigmaDisplay       *display;
  LigmaPDBStatusType  status;
  gboolean           success = FALSE;
  GError            *error   = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (monitor == NULL || G_IS_OBJECT (monitor), FALSE);

  display = ligma_get_empty_display (ligma);

  /* show the progress in the last opened display, see bug #704896 */
  if (! display)
    display = ligma_context_get_display (ligma_get_user_context (ligma));

  if (display)
    g_object_add_weak_pointer (G_OBJECT (display), (gpointer) &display);

  image = file_open_with_display (ligma,
                                  ligma_get_user_context (ligma),
                                  LIGMA_PROGRESS (display),
                                  file, as_new,
                                  monitor,
                                  &status, &error);

  if (image)
    {
      success = TRUE;

      g_object_set_data_full (G_OBJECT (ligma), LIGMA_FILE_OPEN_LAST_FILE_KEY,
                              g_object_ref (file),
                              (GDestroyNotify) g_object_unref);
    }
  else if (status != LIGMA_PDB_SUCCESS && status != LIGMA_PDB_CANCEL && display)
    {
      ligma_message (ligma, G_OBJECT (display), LIGMA_MESSAGE_ERROR,
                    _("Opening '%s' failed: %s"),
                    ligma_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  if (display)
    g_object_remove_weak_pointer (G_OBJECT (display), (gpointer) &display);

  return success;
}


/*  private functions  */

static void
file_open_sanitize_image (LigmaImage *image,
                          gboolean   as_new)
{
  if (as_new)
    ligma_image_set_file (image, NULL);

  /* clear all undo steps */
  ligma_image_undo_free (image);

  /* make sure that undo is enabled */
  while (! ligma_image_undo_is_enabled (image))
    ligma_image_undo_thaw (image);

  /* Set the image to clean. Note that export dirtiness is not set to
   * clean here; we can only consider export clean after the first
   * export
   */
  ligma_image_clean_all (image);

  /* Make sure the projection is completely constructed from valid
   * layers, this is needed in case something triggers projection or
   * image preview creation before all layers are loaded, see bug #767663.
   */
  ligma_image_invalidate_all (image);

  /* Make sure all image states are up-to-date */
  ligma_image_flush (image);
}

/* Converts items from one image to another */
static void
file_open_convert_items (LigmaImage   *dest_image,
                         const gchar *basename,
                         GList       *items)
{
  GList *list;

  for (list = items; list; list = g_list_next (list))
    {
      LigmaItem *src = list->data;
      LigmaItem *item;

      item = ligma_item_convert (src, dest_image, G_TYPE_FROM_INSTANCE (src));

      if (g_list_length (items) == 1)
        {
          ligma_object_set_name (LIGMA_OBJECT (item), basename);
        }
      else
        {
          ligma_object_set_name (LIGMA_OBJECT (item),
                                ligma_object_get_name (src));
        }

      list->data = item;
    }
}

static GList *
file_open_get_layers (LigmaImage *image,
                      gboolean   merge_visible,
                      gint      *n_visible)
{
  GList *iter   = NULL;
  GList *layers = NULL;

  for (iter = ligma_image_get_layer_iter (image);
       iter;
       iter = g_list_next (iter))
    {
      LigmaItem *item = iter->data;

      if (! merge_visible)
        layers = g_list_prepend (layers, item);

      if (ligma_item_get_visible (item))
        {
          if (n_visible)
            (*n_visible)++;

          if (! layers)
            layers = g_list_prepend (layers, item);
        }
    }

  return layers;
}

static gboolean
file_open_file_proc_is_import (LigmaPlugInProcedure *file_proc)
{
  return !(file_proc &&
           file_proc->mime_types &&
           strcmp (file_proc->mime_types, "image/x-xcf") == 0);
}
