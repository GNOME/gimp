/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core/core-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-babl.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdocumentlist.h"
#include "core/gimpimage.h"
#include "core/gimpimage-merge.h"
#include "core/gimpimage-profile.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimagefile.h"
#include "core/gimplayer.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "pdb/gimppdb.h"

#include "plug-in/gimppluginmanager.h"
#include "plug-in/gimppluginprocedure.h"
#include "plug-in/gimppluginerror.h"
#include "plug-in/plug-in-icc-profile.h"

#include "file-open.h"
#include "file-procedure.h"
#include "file-remote.h"
#include "file-utils.h"
#include "gimp-file.h"

#include "gimp-intl.h"


static void     file_open_sanitize_image       (GimpImage                 *image,
                                                gboolean                   as_new);
static void     file_open_convert_items        (GimpImage                 *dest_image,
                                                const gchar               *basename,
                                                GList                     *items);
static void     file_open_handle_color_profile (GimpImage                 *image,
                                                GimpContext               *context,
                                                GimpProgress              *progress,
                                                GimpRunMode                run_mode);
static GList *  file_open_get_layers           (const GimpImage           *image,
                                                gboolean                   merge_visible,
                                                gint                      *n_visible);
static gboolean file_open_file_proc_is_import  (const GimpPlugInProcedure *file_proc);


/*  public functions  */

GimpImage *
file_open_image (Gimp                *gimp,
                 GimpContext         *context,
                 GimpProgress        *progress,
                 GFile               *file,
                 GFile               *entered_file,
                 gboolean             as_new,
                 GimpPlugInProcedure *file_proc,
                 GimpRunMode          run_mode,
                 GimpPDBStatusType   *status,
                 const gchar        **mime_type,
                 GError             **error)
{
  GimpValueArray *return_vals;
  GimpImage      *image       = NULL;
  GFile          *local_file  = NULL;
  gchar          *path        = NULL;
  gchar          *entered_uri = NULL;
  GError         *my_error    = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_FILE (entered_file), NULL);
  g_return_val_if_fail (status != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  *status = GIMP_PDB_EXECUTION_ERROR;

  if (! file_proc)
    file_proc = file_procedure_find (gimp->plug_in_manager->load_procs, file,
                                     error);

  if (! file_proc)
    return NULL;

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
          return NULL;
        }

      if (! g_file_info_get_attribute_boolean (info,
                                               G_FILE_ATTRIBUTE_ACCESS_CAN_READ))
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Premission denied"));
          return NULL;
        }
    }

  if (! g_file_is_native (file) &&
      ! file_remote_mount_file (gimp, file, progress, &my_error))
    {
      if (my_error)
        g_propagate_error (error, my_error);
      else
        *status = GIMP_PDB_CANCEL;

      return NULL;
    }

  if (! file_proc->handles_uri)
    {
      path = g_file_get_path (file);

      if (! path)
        {
          local_file = file_remote_download_image (gimp, file, progress,
                                                   &my_error);

          if (! local_file)
            {
              if (my_error)
                g_propagate_error (error, my_error);
              else
                *status = GIMP_PDB_CANCEL;

              return NULL;
            }

          path = g_file_get_path (local_file);
        }
    }

  if (! path)
    path = g_file_get_uri (file);

  entered_uri = g_file_get_uri (entered_file);

  if (! entered_uri)
    entered_uri = g_strdup (path);

  return_vals =
    gimp_pdb_execute_procedure_by_name (gimp->pdb,
                                        context, progress, error,
                                        gimp_object_get_name (file_proc),
                                        GIMP_TYPE_INT32, run_mode,
                                        G_TYPE_STRING,   path,
                                        G_TYPE_STRING,   entered_uri,
                                        G_TYPE_NONE);

  g_free (path);
  g_free (entered_uri);

  *status = g_value_get_enum (gimp_value_array_index (return_vals, 0));

  if (*status == GIMP_PDB_SUCCESS)
    image = gimp_value_get_image (gimp_value_array_index (return_vals, 1),
                                  gimp);

  if (local_file)
    {
      if (image)
        gimp_image_set_file (image, file);

      g_file_delete (local_file, NULL, NULL);
      g_object_unref (local_file);
    }

  if (*status == GIMP_PDB_SUCCESS)
    {
      if (image)
        {
          file_open_sanitize_image (image, as_new);

          /* Only set the load procedure if it hasn't already been set. */
          if (! gimp_image_get_load_proc (image))
            gimp_image_set_load_proc (image, file_proc);

          file_proc = gimp_image_get_load_proc (image);

          if (mime_type)
            *mime_type = file_proc->mime_type;
        }
      else
        {
          if (error && ! *error)
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         _("%s plug-in returned SUCCESS but did not "
                           "return an image"),
                         gimp_plug_in_procedure_get_label (file_proc));

          *status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (*status != GIMP_PDB_CANCEL)
    {
      if (error && ! *error)
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("%s plug-In could not open image"),
                     gimp_plug_in_procedure_get_label (file_proc));
    }

  gimp_value_array_unref (return_vals);

  if (image)
    {
      file_open_handle_color_profile (image, context, progress, run_mode);

      if (file_open_file_proc_is_import (file_proc))
        {
          /* Remember the import source */
          gimp_image_set_imported_file (image, file);

          /* We shall treat this file as an Untitled file */
          gimp_image_set_file (image, NULL);
        }
    }

  return image;
}

/**
 * file_open_thumbnail:
 * @gimp:
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
 * Return value: the thumbnail image
 */
GimpImage *
file_open_thumbnail (Gimp           *gimp,
                     GimpContext    *context,
                     GimpProgress   *progress,
                     GFile          *file,
                     gint            size,
                     const gchar   **mime_type,
                     gint           *image_width,
                     gint           *image_height,
                     const Babl    **format,
                     gint           *num_layers,
                     GError        **error)
{
  GimpPlugInProcedure *file_proc;
  GimpProcedure       *procedure;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
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

  file_proc = file_procedure_find (gimp->plug_in_manager->load_procs, file,
                                   NULL);

  if (! file_proc || ! file_proc->thumb_loader)
    return NULL;

  procedure = gimp_pdb_lookup_procedure (gimp->pdb, file_proc->thumb_loader);

  if (procedure && procedure->num_args >= 2 && procedure->num_values >= 1)
    {
      GimpPDBStatusType  status;
      GimpValueArray    *return_vals;
      GimpImage         *image = NULL;
      gchar             *path  = NULL;

      if (! file_proc->handles_uri)
        path = g_file_get_path (file);

      if (! path)
        path = g_file_get_uri (file);

      return_vals =
        gimp_pdb_execute_procedure_by_name (gimp->pdb,
                                            context, progress, error,
                                            gimp_object_get_name (procedure),
                                            G_TYPE_STRING,   path,
                                            GIMP_TYPE_INT32, size,
                                            G_TYPE_NONE);

      g_free (path);

      status = g_value_get_enum (gimp_value_array_index (return_vals, 0));

      if (status == GIMP_PDB_SUCCESS &&
          GIMP_VALUE_HOLDS_IMAGE_ID (gimp_value_array_index (return_vals, 1)))
        {
          image = gimp_value_get_image (gimp_value_array_index (return_vals, 1),
                                        gimp);

          if (gimp_value_array_length (return_vals) >= 3 &&
              G_VALUE_HOLDS_INT (gimp_value_array_index (return_vals, 2)) &&
              G_VALUE_HOLDS_INT (gimp_value_array_index (return_vals, 3)))
            {
              *image_width =
                MAX (0, g_value_get_int (gimp_value_array_index (return_vals, 2)));

              *image_height =
                MAX (0, g_value_get_int (gimp_value_array_index (return_vals, 3)));

              if (gimp_value_array_length (return_vals) >= 5 &&
                  G_VALUE_HOLDS_INT (gimp_value_array_index (return_vals, 4)))
                {
                  gint value = g_value_get_int (gimp_value_array_index (return_vals, 4));

                  switch (value)
                    {
                    case GIMP_RGB_IMAGE:
                      *format = gimp_babl_format (GIMP_RGB,
                                                  GIMP_PRECISION_U8_GAMMA,
                                                  FALSE);
                      break;

                    case GIMP_RGBA_IMAGE:
                      *format = gimp_babl_format (GIMP_RGB,
                                                  GIMP_PRECISION_U8_GAMMA,
                                                  TRUE);
                      break;

                    case GIMP_GRAY_IMAGE:
                      *format = gimp_babl_format (GIMP_GRAY,
                                                  GIMP_PRECISION_U8_GAMMA,
                                                  FALSE);
                      break;

                    case GIMP_GRAYA_IMAGE:
                      *format = gimp_babl_format (GIMP_GRAY,
                                                  GIMP_PRECISION_U8_GAMMA,
                                                  TRUE);
                      break;

                    case GIMP_INDEXED_IMAGE:
                    case GIMP_INDEXEDA_IMAGE:
                      {
                        const Babl *rgb;
                        const Babl *rgba;

                        babl_new_palette ("-gimp-indexed-format-dummy",
                                          &rgb, &rgba);

                        if (value == GIMP_INDEXED_IMAGE)
                          *format = rgb;
                        else
                          *format = rgba;
                      }
                      break;

                    default:
                      break;
                    }
                }

              if (gimp_value_array_length (return_vals) >= 6 &&
                  G_VALUE_HOLDS_INT (gimp_value_array_index (return_vals, 5)))
                {
                  *num_layers =
                    MAX (0, g_value_get_int (gimp_value_array_index (return_vals, 5)));
                }
            }

          if (image)
            {
              file_open_sanitize_image (image, FALSE);

              *mime_type = file_proc->mime_type;

#ifdef GIMP_UNSTABLE
              g_printerr ("opened thumbnail at %d x %d\n",
                          gimp_image_get_width  (image),
                          gimp_image_get_height (image));
#endif
            }
        }

      gimp_value_array_unref (return_vals);

      return image;
    }

  return NULL;
}

GimpImage *
file_open_with_display (Gimp               *gimp,
                        GimpContext        *context,
                        GimpProgress       *progress,
                        GFile              *file,
                        gboolean            as_new,
                        GObject            *screen,
                        gint                monitor,
                        GimpPDBStatusType  *status,
                        GError            **error)
{
  return file_open_with_proc_and_display (gimp, context, progress,
                                          file, file, as_new, NULL,
                                          screen, monitor,
                                          status, error);
}

GimpImage *
file_open_with_proc_and_display (Gimp                *gimp,
                                 GimpContext         *context,
                                 GimpProgress        *progress,
                                 GFile               *file,
                                 GFile               *entered_file,
                                 gboolean             as_new,
                                 GimpPlugInProcedure *file_proc,
                                 GObject             *screen,
                                 gint                 monitor,
                                 GimpPDBStatusType   *status,
                                 GError             **error)
{
  GimpImage   *image;
  const gchar *mime_type = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_FILE (entered_file), NULL);
  g_return_val_if_fail (screen == NULL || G_IS_OBJECT (screen), NULL);
  g_return_val_if_fail (status != NULL, NULL);

  image = file_open_image (gimp, context, progress,
                           file,
                           entered_file,
                           as_new,
                           file_proc,
                           GIMP_RUN_INTERACTIVE,
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
        file_proc = gimp_image_get_load_proc (image);

      if (file_open_file_proc_is_import (file_proc) &&
          gimp_image_get_n_layers (image) == 1)
        {
          GimpObject *layer = gimp_image_get_layer_iter (image)->data;
          gchar      *basename;

          basename = g_path_get_basename (gimp_file_get_utf8_name (file));

          gimp_item_rename (GIMP_ITEM (layer), basename, NULL);
          gimp_image_undo_free (image);
          gimp_image_clean_all (image);

          g_free (basename);
        }

      if (gimp_create_display (image->gimp, image, GIMP_UNIT_PIXEL, 1.0,
                               screen, monitor))
        {
          /*  the display owns the image now  */
          g_object_unref (image);
        }

      if (! as_new)
        {
          GimpDocumentList *documents = GIMP_DOCUMENT_LIST (gimp->documents);
          GimpImagefile    *imagefile;
          GFile            *any_file;

          imagefile = gimp_document_list_add_file (documents, file, mime_type);

          /*  can only create a thumbnail if the passed file and the
           *  resulting image's file match. Use any_file() here so we
           *  create thumbnails for both XCF and imported images.
           */
          any_file = gimp_image_get_any_file (image);

          if (any_file && g_file_equal (file, any_file))
            {
              /*  no need to save a thumbnail if there's a good one already  */
              if (! gimp_imagefile_check_thumbnail (imagefile))
                {
                  gimp_imagefile_save_thumbnail (imagefile, mime_type, image,
                                                 NULL);
                }
            }
        }

      /*  announce that we opened this image  */
      gimp_image_opened (image->gimp, file);
    }

  return image;
}

GList *
file_open_layers (Gimp                *gimp,
                  GimpContext         *context,
                  GimpProgress        *progress,
                  GimpImage           *dest_image,
                  gboolean             merge_visible,
                  GFile               *file,
                  GimpRunMode          run_mode,
                  GimpPlugInProcedure *file_proc,
                  GimpPDBStatusType   *status,
                  GError             **error)
{
  GimpImage   *new_image;
  GList       *layers    = NULL;
  const gchar *mime_type = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (status != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  new_image = file_open_image (gimp, context, progress,
                               file, file, FALSE,
                               file_proc,
                               run_mode,
                               status, &mime_type, error);

  if (new_image)
    {
      gint n_visible = 0;

      gimp_image_undo_disable (new_image);

      layers = file_open_get_layers (new_image, merge_visible, &n_visible);

      if (merge_visible && n_visible > 1)
        {
          GimpLayer *layer;

          g_list_free (layers);

          layer = gimp_image_merge_visible_layers (new_image, context,
                                                   GIMP_CLIP_TO_IMAGE,
                                                   FALSE, FALSE);

          layers = g_list_prepend (NULL, layer);
        }

      if (layers)
        {
          gchar *basename;

          basename = g_path_get_basename (gimp_file_get_utf8_name (file));
          file_open_convert_items (dest_image, basename, layers);
          g_free (basename);

          gimp_document_list_add_file (GIMP_DOCUMENT_LIST (gimp->documents),
                                       file, mime_type);
        }
      else
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
			       _("Image doesn't contain any layers"));
          *status = GIMP_PDB_EXECUTION_ERROR;
        }

      g_object_unref (new_image);
    }

  return g_list_reverse (layers);
}


/*  This function is called for filenames passed on the command-line
 *  or from the D-Bus service.
 */
gboolean
file_open_from_command_line (Gimp     *gimp,
                             GFile    *file,
                             gboolean  as_new,
                             GObject  *screen,
                             gint      monitor)

{
  GimpImage         *image;
  GimpObject        *display;
  GimpPDBStatusType  status;
  gboolean           success = FALSE;
  GError            *error   = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (screen == NULL || G_IS_OBJECT (screen), FALSE);

  display = gimp_get_empty_display (gimp);

  /* show the progress in the last opened display, see bug #704896 */
  if (! display)
    display = gimp_context_get_display (gimp_get_user_context (gimp));

  if (display)
    g_object_add_weak_pointer (G_OBJECT (display), (gpointer) &display);

  image = file_open_with_display (gimp,
                                  gimp_get_user_context (gimp),
                                  GIMP_PROGRESS (display),
                                  file, as_new,
                                  screen, monitor,
                                  &status, &error);

  if (image)
    {
      success = TRUE;

      g_object_set_data_full (G_OBJECT (gimp), GIMP_FILE_OPEN_LAST_FILE_KEY,
                              g_object_ref (file),
                              (GDestroyNotify) g_object_unref);
    }
  else if (status != GIMP_PDB_CANCEL && display)
    {
      gimp_message (gimp, G_OBJECT (display), GIMP_MESSAGE_ERROR,
                    _("Opening '%s' failed: %s"),
                    gimp_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  if (display)
    g_object_remove_weak_pointer (G_OBJECT (display), (gpointer) &display);

  return success;
}


/*  private functions  */

static void
file_open_sanitize_image (GimpImage *image,
                          gboolean   as_new)
{
  if (as_new)
    gimp_image_set_file (image, NULL);

  /* clear all undo steps */
  gimp_image_undo_free (image);

  /* make sure that undo is enabled */
  while (! gimp_image_undo_is_enabled (image))
    gimp_image_undo_thaw (image);

  /* Set the image to clean. Note that export dirtiness is not set to
   * clean here; we can only consider export clean after the first
   * export
   */
  gimp_image_clean_all (image);

#if 0
  /* XXX this is not needed any longer, remove it when sure */

  /* make sure the entire projection is properly constructed, because
   * load plug-ins are not required to call gimp_drawable_update() or
   * anything.
   */
  gimp_image_invalidate (image,
                         0, 0,
                         gimp_image_get_width  (image),
                         gimp_image_get_height (image));
  gimp_image_flush (image);

  /* same for drawable previews */
  gimp_image_invalidate_previews (image);
#endif
}

/* Converts items from one image to another */
static void
file_open_convert_items (GimpImage   *dest_image,
                         const gchar *basename,
                         GList       *items)
{
  GList *list;

  for (list = items; list; list = g_list_next (list))
    {
      GimpItem *src = list->data;
      GimpItem *item;

      item = gimp_item_convert (src, dest_image, G_TYPE_FROM_INSTANCE (src));

      if (g_list_length (items) == 1)
        {
          gimp_object_set_name (GIMP_OBJECT (item), basename);
        }
      else
        {
          gimp_object_set_name (GIMP_OBJECT (item),
                                gimp_object_get_name (src));
        }

      list->data = item;
    }
}

static void
file_open_profile_apply_rgb (GimpImage    *image,
                             GimpContext  *context,
                             GimpProgress *progress,
                             GimpRunMode   run_mode)
{
  GimpColorConfig *config = image->gimp->config->color_management;
  GError          *error  = NULL;

  if (gimp_image_get_base_type (image) == GIMP_GRAY)
    return;

  if (config->mode == GIMP_COLOR_MANAGEMENT_OFF)
    return;

  if (! plug_in_icc_profile_apply_rgb (image, context, progress, run_mode,
                                       &error))
    {
      if (error->domain == GIMP_PLUG_IN_ERROR &&
          error->code   == GIMP_PLUG_IN_NOT_FOUND)
        {
          gchar *msg = g_strdup_printf ("%s\n\n%s",
                                        error->message,
                                        _("Color management has been disabled. "
                                          "It can be enabled again in the "
                                          "Preferences dialog."));

          g_object_set (config, "mode", GIMP_COLOR_MANAGEMENT_OFF, NULL);

          gimp_message_literal (image->gimp, G_OBJECT (progress),
                                GIMP_MESSAGE_WARNING, msg);
          g_free (msg);
        }
      else
        {
          gimp_message_literal (image->gimp, G_OBJECT (progress),
                                GIMP_MESSAGE_ERROR, error->message);
        }

      g_error_free (error);
    }
}

static void
file_open_handle_color_profile (GimpImage    *image,
                                GimpContext  *context,
                                GimpProgress *progress,
                                GimpRunMode   run_mode)
{
  if (gimp_image_get_icc_profile (image))
    {
      gimp_image_undo_disable (image);

      switch (image->gimp->config->color_profile_policy)
        {
        case GIMP_COLOR_PROFILE_POLICY_ASK:
          if (run_mode == GIMP_RUN_INTERACTIVE)
            file_open_profile_apply_rgb (image, context, progress,
                                         GIMP_RUN_INTERACTIVE);
          break;

        case GIMP_COLOR_PROFILE_POLICY_KEEP:
          break;

        case GIMP_COLOR_PROFILE_POLICY_CONVERT:
          file_open_profile_apply_rgb (image, context, progress,
                                       GIMP_RUN_NONINTERACTIVE);
          break;
        }

      gimp_image_clean_all (image);
      gimp_image_undo_enable (image);
    }
}

static GList *
file_open_get_layers (const GimpImage *image,
                      gboolean         merge_visible,
                      gint            *n_visible)
{
  GList *iter   = NULL;
  GList *layers = NULL;

  for (iter = gimp_image_get_layer_iter (image);
       iter;
       iter = g_list_next (iter))
    {
      GimpItem *item = iter->data;

      if (! merge_visible)
        layers = g_list_prepend (layers, item);

      if (gimp_item_get_visible (item))
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
file_open_file_proc_is_import (const GimpPlugInProcedure *file_proc)
{
  return !(file_proc &&
           file_proc->mime_type &&
           strcmp (file_proc->mime_type, "image/xcf") == 0);
}
