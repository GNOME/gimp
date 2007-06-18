/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * file-open.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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

#include <glib-object.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#define R_OK 4
#endif

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
#include "plug-in/plug-in-icc-profile.h"


#include "file-open.h"
#include "file-procedure.h"
#include "file-utils.h"
#include "gimprecentlist.h"

#include "gimp-intl.h"


static void  file_open_sanitize_image       (GimpImage    *image,
                                             gboolean      as_new);
static void  file_open_handle_color_profile (GimpImage    *image,
                                             GimpContext  *context,
                                             GimpProgress *progress,
                                             GimpRunMode   run_mode);


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

  if (! file_proc)
    file_proc = file_procedure_find (gimp->plug_in_manager->load_procs, uri,
                                     error);

  if (! file_proc)
    return NULL;

  filename = file_utils_filename_from_uri (uri);

  if (filename)
    {
      /* check if we are opening a file */
      if (g_file_test (filename, G_FILE_TEST_EXISTS))
        {
          if (! g_file_test (filename, G_FILE_TEST_IS_REGULAR))
            {
              g_free (filename);
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Not a regular file"));
              return NULL;
            }

          if (g_access (filename, R_OK) != 0)
            {
              g_free (filename);
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_ACCES,
                           g_strerror (errno));
              return NULL;
            }
        }
    }
  else
    {
      filename = g_strdup (uri);
    }

  return_vals =
    gimp_pdb_execute_procedure_by_name (gimp->pdb,
                                        context, progress,
                                        GIMP_OBJECT (file_proc)->name,
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

          if (mime_type)
            *mime_type = file_proc->mime_type;
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("%s plug-in returned SUCCESS but did not "
                         "return an image"),
                       gimp_plug_in_procedure_get_label (file_proc));
          *status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (*status != GIMP_PDB_CANCEL)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("%s plug-In could not open image"),
                   gimp_plug_in_procedure_get_label (file_proc));
    }

  g_value_array_free (return_vals);

  if (image)
    file_open_handle_color_profile (image, context, progress, run_mode);

  return image;
}

/*  Attempts to load a thumbnail by using a registered thumbnail loader.  */
GimpImage *
file_open_thumbnail (Gimp          *gimp,
                     GimpContext   *context,
                     GimpProgress  *progress,
                     const gchar   *uri,
                     gint           size,
                     const gchar  **mime_type,
                     gint          *image_width,
                     gint          *image_height)
{
  GimpPlugInProcedure *file_proc;
  GimpProcedure       *procedure;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (image_width != NULL, NULL);
  g_return_val_if_fail (image_height != NULL, NULL);

  *image_width  = 0;
  *image_height = 0;

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
                                            context, progress,
                                            GIMP_OBJECT (procedure)->name,
                                            G_TYPE_STRING,   filename,
                                            GIMP_TYPE_INT32, size,
                                            G_TYPE_NONE);

      g_free (filename);

      status = g_value_get_enum (&return_vals->values[0]);

      if (status == GIMP_PDB_SUCCESS)
        {
          image = gimp_value_get_image (&return_vals->values[1], gimp);

          if (return_vals->n_values >= 3)
            {
              *image_width  = MAX (0, g_value_get_int (&return_vals->values[2]));
              *image_height = MAX (0, g_value_get_int (&return_vals->values[3]));
            }

          if (image)
            {
              file_open_sanitize_image (image, FALSE);

              *mime_type = file_proc->mime_type;

#ifdef GIMP_UNSTABLE
              g_printerr ("opened thumbnail at %d x %d\n",
                          image->width, image->height);
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
      gimp_create_display (image->gimp, image, GIMP_UNIT_PIXEL, 1.0);

      if (! as_new)
        {
          GimpDocumentList *documents = GIMP_DOCUMENT_LIST (gimp->documents);
          GimpImagefile    *imagefile;

          imagefile = gimp_document_list_add_uri (documents, uri, mime_type);

          /*  can only create a thumbnail if the passed uri and the
           *  resulting image's uri match.
           */
          if (strcmp (uri, gimp_image_get_uri (image)) == 0)
            {
              /*  no need to save a thumbnail if there's a good one already  */
              if (! gimp_imagefile_check_thumbnail (imagefile))
                {
                  gimp_imagefile_save_thumbnail (imagefile, mime_type, image);
                }
            }

          if (gimp->config->save_document_history)
            gimp_recent_list_add_uri (uri, mime_type);
        }

      /*  the display owns the image now  */
      g_object_unref (image);
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
      GList *list;
      gint   n_visible = 0;

      gimp_image_undo_disable (new_image);

      for (list = GIMP_LIST (new_image->layers)->list;
           list;
           list = g_list_next (list))
        {
          if (! merge_visible)
            layers = g_list_prepend (layers, list->data);

          if (gimp_item_get_visible (list->data))
            {
              n_visible++;

              if (! layers)
                layers = g_list_prepend (layers, list->data);
            }
        }

      if (merge_visible && n_visible > 1)
        {
          GimpLayer *layer;

          g_list_free (layers);

          layer = gimp_image_merge_visible_layers (new_image, context,
                                                   GIMP_CLIP_TO_IMAGE, FALSE);

          layers = g_list_prepend (NULL, layer);
        }

      if (layers)
        {
          gchar *basename = file_utils_uri_display_basename (uri);

          for (list = layers; list; list = g_list_next (list))
            {
              GimpLayer *layer = list->data;
              GimpItem  *item;

              item = gimp_item_convert (GIMP_ITEM (layer), dest_image,
                                        G_TYPE_FROM_INSTANCE (layer),
                                        TRUE);

              if (layers->next == NULL)
                {
                  gimp_object_set_name (GIMP_OBJECT (item), basename);
                }
              else
                {
                  gchar *name = g_strdup_printf ("%s - %s", basename,
                                                 gimp_object_get_name (GIMP_OBJECT (layer)));

                  gimp_object_take_name (GIMP_OBJECT (item), name);
                }

              list->data = item;
            }

          g_free (basename);

          gimp_document_list_add_uri (GIMP_DOCUMENT_LIST (gimp->documents),
                                      uri, mime_type);

          if (gimp->config->save_document_history)
            gimp_recent_list_add_uri (uri, mime_type);
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
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
      GimpPDBStatusType  status;

      image = file_open_with_display (gimp,
                                      gimp_get_user_context (gimp),
                                      NULL,
                                      uri, as_new,
                                      &status, &error);

      if (image)
        {
          success = TRUE;
        }
      else if (status != GIMP_PDB_CANCEL)
        {
          gchar *filename = file_utils_uri_display_name (uri);

          gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR,
                        _("Opening '%s' failed: %s"),
                        filename, error->message);
          g_clear_error (&error);

          g_free (filename);
        }

      g_free (uri);
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
    gimp_object_set_name (GIMP_OBJECT (image), NULL);

  /* clear all undo steps */
  gimp_image_undo_free (image);

  /* make sure that undo is enabled */
  while (image->undo_freeze_count)
    gimp_image_undo_thaw (image);

  /* set the image to clean  */
  gimp_image_clean_all (image);

  gimp_image_invalidate_layer_previews (image);
  gimp_image_invalidate_channel_previews (image);
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (image));
}

static void
file_open_profile_apply_rgb (GimpImage    *image,
                             GimpContext  *context,
                             GimpProgress *progress,
                             GimpRunMode   run_mode)
{
  GError *error = NULL;

  if (gimp_image_base_type (image) != GIMP_GRAY &&
      ! plug_in_icc_profile_apply_rgb (image, context, progress,
                                       run_mode, &error))
    {
      gimp_message (image->gimp, G_OBJECT (progress),
                    GIMP_MESSAGE_WARNING, error->message);
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
    }
}
