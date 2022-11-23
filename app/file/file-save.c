/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * file-save.c
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

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmadocumentlist.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmaimagefile.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmaprogress.h"

#include "pdb/ligmapdb.h"

#include "plug-in/ligmapluginprocedure.h"

#include "file-remote.h"
#include "file-save.h"
#include "ligma-file.h"

#include "ligma-intl.h"


/*  public functions  */

LigmaPDBStatusType
file_save (Ligma                *ligma,
           LigmaImage           *image,
           LigmaProgress        *progress,
           GFile               *file,
           LigmaPlugInProcedure *file_proc,
           LigmaRunMode          run_mode,
           gboolean             change_saved_state,
           gboolean             export_backward,
           gboolean             export_forward,
           GError             **error)
{
  LigmaValueArray    *return_vals;
  GFile             *orig_file;
  LigmaPDBStatusType  status     = LIGMA_PDB_EXECUTION_ERROR;
  GFile             *local_file = NULL;
  gboolean           mounted    = TRUE;
  GError            *my_error   = NULL;
  GList             *drawables_list;
  LigmaDrawable     **drawables  = NULL;
  gint               n_drawables;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), LIGMA_PDB_CALLING_ERROR);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), LIGMA_PDB_CALLING_ERROR);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress),
                        LIGMA_PDB_CALLING_ERROR);
  g_return_val_if_fail (G_IS_FILE (file), LIGMA_PDB_CALLING_ERROR);
  g_return_val_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (file_proc),
                        LIGMA_PDB_CALLING_ERROR);
  g_return_val_if_fail ((export_backward && export_forward) == FALSE,
                        LIGMA_PDB_CALLING_ERROR);
  g_return_val_if_fail (error == NULL || *error == NULL,
                        LIGMA_PDB_CALLING_ERROR);

  orig_file = file;

  /*  ref image and file, so they can't get deleted during save  */
  g_object_ref (image);
  g_object_ref (orig_file);

  ligma_image_saving (image);

  drawables_list = ligma_image_get_selected_drawables (image);

  if (drawables_list)
    {
      GList *iter;
      gint   i;

      n_drawables = g_list_length (drawables_list);
      drawables = g_new (LigmaDrawable *, n_drawables);
      for (iter = drawables_list, i = 0; iter; iter = iter->next, i++)
        drawables[i] = iter->data;

      g_list_free (drawables_list);
    }
  else
    {
      g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("There is no active layer to save"));
      goto out;
    }

  /* FIXME enable these tests for remote files again, needs testing */
  if (g_file_is_native (file) &&
      g_file_query_exists (file, NULL))
    {
      GFileInfo *info;

      info = g_file_query_info (file,
                                G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
                                G_FILE_QUERY_INFO_NONE,
                                NULL, error);
      if (! info)
        {
          /* extra paranoia */
          if (error && ! *error)
            g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                 _("Failed to get file information"));
          goto out;
        }

      if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR)
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Not a regular file"));
          g_object_unref (info);
          goto out;
        }

      if (! g_file_info_get_attribute_boolean (info,
                                               G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE))
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Permission denied"));
          g_object_unref (info);
          goto out;
        }

      g_object_unref (info);
    }

  if (! g_file_is_native (file) &&
      ! file_remote_mount_file (ligma, file, progress, &my_error))
    {
      if (my_error)
        {
          g_printerr ("%s: mounting remote volume failed, trying to upload"
                      "the file: %s\n",
                      G_STRFUNC, my_error->message);
          g_clear_error (&my_error);

          mounted = FALSE;
        }
      else
        {
          status = LIGMA_PDB_CANCEL;

          goto out;
        }
    }

  if (! file_proc->handles_remote || ! mounted)
    {
      gchar *my_path = g_file_get_path (file);

      if (! my_path)
        {
          local_file = file_remote_upload_image_prepare (ligma, file, progress,
                                                         &my_error);

          if (! local_file)
            {
              if (my_error)
                g_propagate_error (error, my_error);
              else
                status = LIGMA_PDB_CANCEL;

              goto out;
            }

          file = local_file;
        }

      g_free (my_path);
    }

  return_vals =
    ligma_pdb_execute_procedure_by_name (image->ligma->pdb,
                                        ligma_get_user_context (ligma),
                                        progress, error,
                                        ligma_object_get_name (file_proc),
                                        LIGMA_TYPE_RUN_MODE,     run_mode,
                                        LIGMA_TYPE_IMAGE,        image,
                                        G_TYPE_INT,             n_drawables,
                                        LIGMA_TYPE_OBJECT_ARRAY, drawables,
                                        G_TYPE_FILE,            file,
                                        G_TYPE_NONE);
  status = g_value_get_enum (ligma_value_array_index (return_vals, 0));

  ligma_value_array_unref (return_vals);
  g_clear_pointer (&drawables, g_free);

  if (local_file)
    {
      if (status == LIGMA_PDB_SUCCESS)
        {
          GError *my_error = NULL;

          if (! file_remote_upload_image_finish (ligma, orig_file, local_file,
                                                 progress, &my_error))
            {
              status = LIGMA_PDB_EXECUTION_ERROR;

              if (my_error)
                g_propagate_error (error, my_error);
              else
                status = LIGMA_PDB_CANCEL;
            }
        }

      g_file_delete (local_file, NULL, NULL);
      g_object_unref (local_file);
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      LigmaDocumentList *documents;
      LigmaImagefile    *imagefile;

      if (change_saved_state)
        {
          ligma_image_set_file (image, orig_file);
          ligma_image_set_save_proc (image, file_proc);

          /* Forget the import source when we save. We interpret a
           * save as that the user is not interested in being able
           * to quickly export back to the original any longer
           */
          ligma_image_set_imported_file (image, NULL);

          ligma_image_clean_all (image);
        }
      else if (export_backward)
        {
          /* We exported the image back to its imported source,
           * change nothing about export/import flags, only set
           * the export state to clean
           */
          ligma_image_export_clean_all (image);
        }
      else if (export_forward)
        {
          /* Remember the last entered Export URI for the image. We
           * only need to do this explicitly when exporting. It
           * happens implicitly when saving since the LigmaObject name
           * of a LigmaImage is the last-save URI
           */
          ligma_image_set_exported_file (image, orig_file);
          ligma_image_set_export_proc (image, file_proc);

          /* An image can not be considered both exported and imported
           * at the same time, so stop consider it as imported now
           * that we consider it exported.
           */
          ligma_image_set_imported_file (image, NULL);

          ligma_image_export_clean_all (image);
        }

      if (export_backward || export_forward)
        ligma_image_exported (image, orig_file);
      else
        ligma_image_saved (image, orig_file);

      documents = LIGMA_DOCUMENT_LIST (image->ligma->documents);

      imagefile = ligma_document_list_add_file (documents, orig_file,
                                               g_slist_nth_data (file_proc->mime_types_list, 0));

      /* only save a thumbnail if we are saving as XCF, see bug #25272 */
      if (LIGMA_PROCEDURE (file_proc)->proc_type == LIGMA_PDB_PROC_TYPE_INTERNAL)
        ligma_imagefile_save_thumbnail (imagefile,
                                       g_slist_nth_data (file_proc->mime_types_list, 0),
                                       image,
                                       NULL);
    }
  else if (status != LIGMA_PDB_CANCEL)
    {
      if (error && *error == NULL)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("%s plug-in could not save image"),
                       ligma_procedure_get_label (LIGMA_PROCEDURE (file_proc)));
        }
    }

  ligma_image_flush (image);

 out:
  g_object_unref (orig_file);
  g_object_unref (image);

  return status;
}
