/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * file-save.c
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
#define W_OK 2
#endif

#include "core/core-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdocumentlist.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "pdb/gimppdb.h"

#include "plug-in/gimppluginprocedure.h"

#include "file-save.h"
#include "file-utils.h"
#include "gimprecentlist.h"

#include "gimp-intl.h"


/*  public functions  */

GimpPDBStatusType
file_save (GimpImage           *image,
           GimpContext         *context,
           GimpProgress        *progress,
           const gchar         *uri,
           GimpPlugInProcedure *file_proc,
           GimpRunMode          run_mode,
           gboolean             save_a_copy,
           GError             **error)
{
  GimpDrawable      *drawable;
  GValueArray       *return_vals;
  GimpPDBStatusType  status;
  gchar             *filename;
  gint32             image_ID;
  gint32             drawable_ID;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress),
                        GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (uri != NULL, GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (file_proc),
                        GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (error == NULL || *error == NULL,
                        GIMP_PDB_CALLING_ERROR);

  drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    return GIMP_PDB_EXECUTION_ERROR;

  filename = file_utils_filename_from_uri (uri);

  if (filename)
    {
      /* check if we are saving to a file */
      if (g_file_test (filename, G_FILE_TEST_EXISTS))
        {
          if (! g_file_test (filename, G_FILE_TEST_IS_REGULAR))
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Not a regular file"));
              status = GIMP_PDB_EXECUTION_ERROR;
              goto out;
            }

          if (g_access (filename, W_OK) != 0)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_ACCES,
                           g_strerror (errno));
              status = GIMP_PDB_EXECUTION_ERROR;
              goto out;
            }
        }
    }
  else
    {
      filename = g_strdup (uri);
    }

  /* ref the image, so it can't get deleted during save */
  g_object_ref (image);

  image_ID    = gimp_image_get_ID (image);
  drawable_ID = gimp_item_get_ID (GIMP_ITEM (drawable));

  return_vals =
    gimp_pdb_execute_procedure_by_name (image->gimp->pdb,
                                        context, progress,
                                        GIMP_OBJECT (file_proc)->name,
                                        GIMP_TYPE_INT32,       run_mode,
                                        GIMP_TYPE_IMAGE_ID,    image_ID,
                                        GIMP_TYPE_DRAWABLE_ID, drawable_ID,
                                        G_TYPE_STRING,         filename,
                                        G_TYPE_STRING,         uri,
                                        G_TYPE_NONE);

  status = g_value_get_enum (&return_vals->values[0]);

  g_value_array_free (return_vals);

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpDocumentList *documents;
      GimpImagefile    *imagefile;

      if (save_a_copy)
        {
          /*  remember the "save-a-copy" filename for the next invocation  */
          g_object_set_data_full (G_OBJECT (image), "gimp-image-save-a-copy",
                                  g_strdup (uri),
                                  (GDestroyNotify) g_free);
        }
      else
        {
          /*  reset the "save-a-copy" filename when the image URI changes  */
          if (strcmp (uri, gimp_image_get_uri (image)))
            g_object_set_data (G_OBJECT (image),
                               "gimp-image-save-a-copy", NULL);

          gimp_image_set_uri (image, uri);
          gimp_image_set_save_proc (image, file_proc);
          gimp_image_clean_all (image);
        }

      gimp_image_saved (image, uri);

      documents = GIMP_DOCUMENT_LIST (image->gimp->documents);
      imagefile = gimp_document_list_add_uri (documents,
                                              uri,
                                              file_proc->mime_type);

      /* only save a thumbnail if we are saving as XCF, see bug #25272 */
      if (GIMP_PROCEDURE (file_proc)->proc_type == GIMP_INTERNAL)
        gimp_imagefile_save_thumbnail (imagefile, file_proc->mime_type, image);

      if (image->gimp->config->save_document_history)
        gimp_recent_list_add_uri (uri, file_proc->mime_type);
    }
  else if (status != GIMP_PDB_CANCEL)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("%s plug-in could not save image"),
                   gimp_plug_in_procedure_get_label (file_proc));
    }

  g_object_unref (image);

 out:
  g_free (filename);

  return status;
}
