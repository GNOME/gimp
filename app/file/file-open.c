/* The GIMP -- an image manipulation program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <io.h>
#define R_OK 4
#define access(f,p) _access(f,p)
#endif

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimagefile.h"
#include "core/gimpdocumentlist.h"

#include "pdb/procedural_db.h"

#include "plug-in/plug-in.h"
#include "plug-in/plug-in-proc.h"

#include "file-open.h"
#include "file-utils.h"

#include "gimp-intl.h"


/*  public functions  */

GimpImage *
file_open_image (Gimp               *gimp,
                 GimpContext        *context,
		 const gchar        *uri,
		 const gchar        *entered_filename,
		 PlugInProcDef      *file_proc,
		 GimpRunMode         run_mode,
		 GimpPDBStatusType  *status,
                 GError            **error)
{
  const ProcRecord *proc;
  Argument         *args;
  Argument         *return_vals;
  gint              gimage_id;
  gint              i;
  gchar            *filename;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (status != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  *status = GIMP_PDB_EXECUTION_ERROR;

  if (! file_proc)
    file_proc = file_utils_find_proc (gimp->load_procs, uri);

  if (! file_proc)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unknown file type"));
      return NULL;
    }

  filename = g_filename_from_uri (uri, NULL, NULL);

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

          if (access (filename, R_OK) != 0)
            {
              g_free (filename);
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_ACCES,
                           g_strerror (errno));
              return NULL;
            }
        }
    }

  proc = plug_in_proc_def_get_proc (file_proc);

  args = g_new0 (Argument, proc->num_args);

  for (i = 0; i < proc->num_args; i++)
    args[i].arg_type = proc->args[i].arg_type;

  args[0].value.pdb_int     = run_mode;
  args[1].value.pdb_pointer = filename ? filename : (gchar *) uri;
  args[2].value.pdb_pointer = (gchar *) entered_filename;

  return_vals = procedural_db_execute (gimp, context, proc->name, args);

  if (filename)
    g_free (filename);

  *status   = return_vals[0].value.pdb_int;
  gimage_id = return_vals[1].value.pdb_int;

  procedural_db_destroy_args (return_vals, proc->num_values);
  g_free (args);

  if (*status == GIMP_PDB_SUCCESS)
    {
      if (gimage_id != -1)
        {
          GimpImage *gimage = gimp_image_get_by_ID (gimp, gimage_id);

          /* clear all undo steps */
          gimp_image_undo_free (gimage);

          /* make sure that undo is enabled */
          while (gimage->undo_freeze_count)
            gimp_image_undo_thaw (gimage);

          /* set the image to clean  */
          gimp_image_clean_all (gimage);

          gimp_image_invalidate_layer_previews (gimage);
          gimp_image_invalidate_channel_previews (gimage);
          gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));

          return gimage;
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Plug-In returned SUCCESS but did not "
                         "return an image"));
          *status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (*status != GIMP_PDB_CANCEL)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Plug-In could not open image"));
    }

  return NULL;
}

GimpImage *
file_open_with_display (Gimp               *gimp,
                        GimpContext        *context,
                        const gchar        *uri,
                        GimpPDBStatusType  *status,
                        GError            **error)
{
  return file_open_with_proc_and_display (gimp, context,
                                          uri, uri, NULL, status, error);
}

GimpImage *
file_open_with_proc_and_display (Gimp               *gimp,
                                 GimpContext        *context,
                                 const gchar        *uri,
                                 const gchar        *entered_filename,
                                 PlugInProcDef      *file_proc,
                                 GimpPDBStatusType  *status,
                                 GError            **error)
{
  GimpImage *gimage;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (status != NULL, NULL);

  gimage = file_open_image (gimp, context,
                            uri,
                            entered_filename,
                            file_proc,
                            GIMP_RUN_INTERACTIVE,
                            status,
                            error);

  if (gimage)
    {
      GimpDocumentList *documents;
      GimpImagefile    *imagefile;

      gimp_create_display (gimage->gimp, gimage, 1.0);

      documents = GIMP_DOCUMENT_LIST (gimp->documents);
      imagefile = gimp_document_list_add_uri (documents, uri);

      /*  can only create a thumbnail if the passed uri and the
       *  resulting image's uri match.
       */
      if (! strcmp (uri, gimp_image_get_uri (gimage)))
        {
          /* save a thumbnail of every opened image */
          gimp_imagefile_save_thumbnail (imagefile, gimage);
        }

      /*  the display owns the image now  */
      g_object_unref (gimage);
    }

  return gimage;
}
