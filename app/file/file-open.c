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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gegl.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#define R_OK 4
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core/core-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdocumentlist.h"
#include "core/gimpimage.h"
#include "core/gimpimage-merge.h"
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
                 const gchar         *uri,
                 const gchar         *entered_filename,
                 gboolean             as_new,
                 GimpPlugInProcedure *file_proc,
                 GimpRunMode          run_mode,
                 GimpPDBStatusType   *status,
                 const gchar        **mime_type,
                 GError             **error)
{
  GValueArray *return_vals;
  gchar       *filename;
  GimpImage   *image = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (status != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  *status = GIMP_PDB_EXECUTION_ERROR;

  filename = file_utils_filename_from_uri (uri);

  if (filename)
    {
      /* check if we are opening a file */
      if (g_file_test (filename, G_FILE_TEST_EXISTS))
        {
          if (! g_file_test (filename, G_FILE_TEST_IS_REGULAR))
            {
              g_free (filename);
              g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
				   _("Not a regular file"));
              return NULL;
            }

          if (g_access (filename, R_OK) != 0)
            {
              g_free (filename);
              g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_ACCES,
				   g_strerror (errno));
              return NULL;
            }
        }
    }
  else
    {
      filename = g_strdup (uri);
    }

  if (! file_proc)
    file_proc = file_procedure_find (gimp->plug_in_manager->load_procs, uri,
                                     error);

  if (! file_proc)
    {
      g_free (filename);
      return NULL;
    }

  return_vals =
    gimp_pdb_execute_procedure_by_name (gimp->pdb,
                                        context, progress, error,
                                        gimp_object_get_name (file_proc),
                                        GIMP_TYPE_INT32, run_mode,
                                        G_TYPE_STRING,   filename,
                                        G_TYPE_STRING,   entered_filename,
                                        G_TYPE_NONE);

  g_free (filename);

  *status = g_value_get_enum (&return_vals->values[0]);

  if (*status == GIMP_PDB_SUCCESS)
    {
      image = gimp_value_get_image (&return_vals->values[1], gimp);

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

  g_value_array_free (return_vals);

  if (image)
    {
      file_open_handle_color_profile (image, context, progress, run_mode);

      if (file_open_file_proc_is_import (file_proc))
        {
          /* Remember the import source */
          gimp_image_set_imported_uri (image, uri);

          /* We shall treat this file as an Untitled file */
          gimp_image_set_uri (image, NULL);
        }
    }

  return image;
}

/**
 * file_open_thumbnail:
 * @gimp:
 * @context:
 * @progress:
 * @uri:          the URI of the image file
 * @size:         requested size of the thumbnail
 * @mime_type:    return location for image MIME type
 * @image_width:  return location for image width
 * @image_height: return location for image height
 * @type:         return location for image type (set to -1 if unknown)
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
                     const gchar    *uri,
                     gint            size,
                     const gchar   **mime_type,
                     gint           *image_width,
                     gint           *image_height,
                     GimpImageType  *type,
                     gint           *num_layers,
                     GError        **error)
{
  GimpPlugInProcedure *file_proc;
  GimpProcedure       *procedure;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (image_width != NULL, NULL);
  g_return_val_if_fail (image_height != NULL, NULL);
  g_return_val_if_fail (type != NULL, NULL);
  g_return_val_if_fail (num_layers != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  *image_width  = 0;
  *image_height = 0;
  *type         = -1;
  *num_layers   = -1;

  file_proc = file_procedure_find (gimp->plug_in_manager->load_procs, uri,
                                   NULL);

  if (! file_proc || ! file_proc->thumb_loader)
    return NULL;

  procedure = gimp_pdb_lookup_procedure (gimp->pdb, file_proc->thumb_loader);

  if (procedure && procedure->num_args >= 2 && procedure->num_values >= 1)
    {
      GimpPDBStatusType  status;
      GValueArray       *return_vals;
      gchar             *filename;
      GimpImage         *image = NULL;

      filename = file_utils_filename_from_uri (uri);

      if (! filename)
        filename = g_strdup (uri);

      return_vals =
        gimp_pdb_execute_procedure_by_name (gimp->pdb,
                                            context, progress, error,
                                            gimp_object_get_name (procedure),
                                            G_TYPE_STRING,   filename,
                                            GIMP_TYPE_INT32, size,
                                            G_TYPE_NONE);

      g_free (filename);

      status = g_value_get_enum (&return_vals->values[0]);

      if (status == GIMP_PDB_SUCCESS &&
          GIMP_VALUE_HOLDS_IMAGE_ID (&return_vals->values[1]))
        {
          image = gimp_value_get_image (&return_vals->values[1], gimp);

          if (return_vals->n_values >= 3 &&
              G_VALUE_HOLDS_INT (&return_vals->values[2]) &&
              G_VALUE_HOLDS_INT (&return_vals->values[3]))
            {
              *image_width  = MAX (0,
                                   g_value_get_int (&return_vals->values[2]));
              *image_height = MAX (0,
                                   g_value_get_int (&return_vals->values[3]));

              if (return_vals->n_values >= 5 &&
                  G_VALUE_HOLDS_INT (&return_vals->values[4]))
                {
                  gint value = g_value_get_int (&return_vals->values[4]);

                  if (gimp_enum_get_value (GIMP_TYPE_IMAGE_TYPE, value,
                                           NULL, NULL, NULL, NULL))
                    {
                      *type = value;
                    }
                }

              if (return_vals->n_values >= 6 &&
                  G_VALUE_HOLDS_INT (&return_vals->values[5]))
                {
                  *num_layers = MAX (0,
                                     g_value_get_int (&return_vals->values[5]));
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

      g_value_array_free (return_vals);

      return image;
    }

  return NULL;
}

GimpImage *
file_open_with_display (Gimp               *gimp,
                        GimpContext        *context,
                        GimpProgress       *progress,
                        const gchar        *uri,
                        gboolean            as_new,
                        GimpPDBStatusType  *status,
                        GError            **error)
{
  return file_open_with_proc_and_display (gimp, context, progress,
                                          uri, uri, as_new, NULL,
                                          status, error);
}

GimpImage *
file_open_with_proc_and_display (Gimp                *gimp,
                                 GimpContext         *context,
                                 GimpProgress        *progress,
                                 const gchar         *uri,
                                 const gchar         *entered_filename,
                                 gboolean             as_new,
                                 GimpPlugInProcedure *file_proc,
                                 GimpPDBStatusType   *status,
                                 GError             **error)
{
  GimpImage   *image;
  const gchar *mime_type = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (status != NULL, NULL);

  image = file_open_image (gimp, context, progress,
                           uri,
                           entered_filename,
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
          GimpObject *layer    = gimp_image_get_layer_iter (image)->data;
          gchar      *basename = file_utils_uri_display_basename (uri);

          gimp_item_rename (GIMP_ITEM (layer), basename, NULL);
          gimp_image_undo_free (image);
          gimp_image_clean_all (image);

          g_free (basename);
        }

      if (gimp_create_display (image->gimp, image, GIMP_UNIT_PIXEL, 1.0))
        {
          /*  the display owns the image now  */
          g_object_unref (image);
        }

      if (! as_new)
        {
          GimpDocumentList *documents = GIMP_DOCUMENT_LIST (gimp->documents);
          GimpImagefile    *imagefile;
          const gchar      *any_uri;

          imagefile = gimp_document_list_add_uri (documents, uri, mime_type);

          /*  can only create a thumbnail if the passed uri and the
           *  resulting image's uri match. Use any_uri() here so we
           *  create thumbnails for both XCF and imported images.
           */
          any_uri = gimp_image_get_any_uri (image);

          if (any_uri && ! strcmp (uri, any_uri))
            {
              /*  no need to save a thumbnail if there's a good one already  */
              if (! gimp_imagefile_check_thumbnail (imagefile))
                {
                  gimp_imagefile_save_thumbnail (imagefile, mime_type, image);
                }
            }
        }

      /*  announce that we opened this image  */
      gimp_image_opened (image->gimp, uri);
    }

  return image;
}

GList *
file_open_layers (Gimp                *gimp,
                  GimpContext         *context,
                  GimpProgress        *progress,
                  GimpImage           *dest_image,
                  gboolean             merge_visible,
                  const gchar         *uri,
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
  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (status != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  new_image = file_open_image (gimp, context, progress,
                               uri, uri, FALSE,
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
          gchar *basename = file_utils_uri_display_basename (uri);

          file_open_convert_items (dest_image, basename, layers);
          g_free (basename);

          gimp_document_list_add_uri (GIMP_DOCUMENT_LIST (gimp->documents),
                                      uri, mime_type);
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
file_open_from_command_line (Gimp        *gimp,
                             const gchar *filename,
                             gboolean     as_new)
{
  GError   *error   = NULL;
  gchar    *uri;
  gboolean  success = FALSE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  /* we accept URI or filename */
  uri = file_utils_any_to_uri (gimp, filename, &error);

  if (uri)
    {
      GimpImage         *image;
      GimpObject        *display = gimp_get_empty_display (gimp);
      GimpPDBStatusType  status;

      /* show the progress in the last opened display, see bug #704896 */
      if (! display)
        display = gimp_context_get_display (gimp_get_user_context (gimp));

      if (display)
        g_object_add_weak_pointer (G_OBJECT (display), (gpointer) &display);

      image = file_open_with_display (gimp,
                                      gimp_get_user_context (gimp),
                                      GIMP_PROGRESS (display),
                                      uri, as_new,
                                      &status, &error);

      if (image)
        {
          success = TRUE;

          g_object_set_data_full (G_OBJECT (gimp), GIMP_FILE_OPEN_LAST_URI_KEY,
                                  uri, (GDestroyNotify) g_free);
        }
      else if (status != GIMP_PDB_CANCEL && display)
        {
          gchar *filename = file_utils_uri_display_name (uri);

          gimp_message (gimp, G_OBJECT (display), GIMP_MESSAGE_ERROR,
                        _("Opening '%s' failed: %s"),
                        filename, error->message);
          g_clear_error (&error);

          g_free (filename);
          g_free (uri);
        }

      if (display)
        g_object_remove_weak_pointer (G_OBJECT (display), (gpointer) &display);
    }
  else
    {
      g_printerr ("conversion filename -> uri failed: %s\n",
                  error->message);
      g_clear_error (&error);
    }

  return success;
}


/*  private functions  */

static void
file_open_sanitize_image (GimpImage *image,
                          gboolean   as_new)
{
  if (as_new)
    gimp_image_set_uri (image, NULL);

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

  if (gimp_image_base_type (image) == GIMP_GRAY)
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
  if (gimp_image_parasite_find (image, "icc-profile"))
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
