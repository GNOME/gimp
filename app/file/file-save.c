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
#define W_OK 2
#define access(f,p) _access(f,p)
#endif

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpcoreconfig.h"
#include "core/gimpdocuments.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"

#include "pdb/procedural_db.h"

#include "plug-in/plug-in.h"
#include "plug-in/plug-in-proc.h"

#include "file-save.h"
#include "file-utils.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


/*  public functions  */

GimpPDBStatusType
file_save (GimpImage     *gimage,
	   const gchar   *uri,
	   const gchar   *raw_filename,
           PlugInProcDef *file_proc,
           GimpRunMode    run_mode,
	   gboolean       set_uri)
{
  ProcRecord        *proc;
  Argument          *args;
  Argument          *return_vals;
  GimpPDBStatusType  status;
  gint               i;
  gchar             *filename;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), GIMP_PDB_CALLING_ERROR);

  if (gimp_image_active_drawable (gimage) == NULL)
    return GIMP_PDB_EXECUTION_ERROR;

  /*  set the image's save_proc if we got passed one, otherwise
   *  try to find a matching file_proc
   */
  if (file_proc)
    {
      gimp_image_set_save_proc (gimage, file_proc);
    }
  else
    {
      file_proc = gimp_image_get_save_proc (gimage);

      if (! file_proc)
        file_proc = file_utils_find_proc (gimage->gimp->save_procs,
                                          raw_filename);

      if (! file_proc)
        {
          g_message (_("Save failed.\n"
                       "%s: Unknown file type."),
                     uri);

          return GIMP_PDB_CANCEL;  /* inhibits error messages by caller */
        }
    }

  filename = g_filename_from_uri (uri, NULL, NULL);

  if (filename)
    {
      /* check if we are saving to a file */
      if (g_file_test (filename, G_FILE_TEST_EXISTS))
        {
          if (! g_file_test (filename, G_FILE_TEST_IS_REGULAR))
            {
              g_message (_("Save failed.\n"
                           "%s is not a regular file."),
                         uri);
              g_free (filename);

              return GIMP_PDB_CANCEL;  /* inhibits error messages by caller */
            }

          if (access (filename, W_OK) != 0)
            {
              g_message (_("Save failed.\n"
                           "%s: %s."),
                         uri,
                         g_strerror (errno));
              g_free (filename);

              return GIMP_PDB_CANCEL;  /* inhibits error messages by caller */
            }
        }
    }

  /* ref the image, so it can't get deleted during save */
  g_object_ref (G_OBJECT (gimage));

  proc = plug_in_proc_def_get_proc (file_proc);

  args = g_new0 (Argument, proc->num_args);

  for (i = 0; i < proc->num_args; i++)
    args[i].arg_type = proc->args[i].arg_type;

  args[0].value.pdb_int     = run_mode;
  args[1].value.pdb_int     = gimp_image_get_ID (gimage);
  args[2].value.pdb_int     = gimp_item_get_ID (GIMP_ITEM (gimp_image_active_drawable (gimage)));
  args[3].value.pdb_pointer = (gpointer) (filename ? filename : uri);
  args[4].value.pdb_pointer = (gpointer) raw_filename;

  return_vals = procedural_db_execute (gimage->gimp, proc->name, args);

  if (filename)
    g_free (filename);

  status = return_vals[0].value.pdb_int;

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpImagefile *imagefile;

      /*  set this image to clean  */
      gimp_image_clean_all (gimage);

      imagefile = gimp_documents_add (GIMP_DOCUMENTS (gimage->gimp->documents),
                                      uri);

      if (set_uri)
	{
	  /*  set the image title  */
	  gimp_image_set_uri (gimage, uri);
	}

      /* Write a thumbnail for the saved image, where appropriate */
      if (gimage->gimp->config->thumbnail_size != GIMP_THUMBNAIL_SIZE_NONE)
	{
          if (set_uri)
            {
              gimp_imagefile_save_thumbnail (imagefile, gimage);
            }
          else
            {
              gchar *saved_uri;

              saved_uri = GIMP_OBJECT (gimage)->name;
              GIMP_OBJECT (gimage)->name = (gchar *) uri;

              gimp_imagefile_save_thumbnail (imagefile, gimage);

              GIMP_OBJECT (gimage)->name = saved_uri;
            }
	}
    }

  g_free (return_vals);
  g_free (args);

  g_object_unref (G_OBJECT (gimage));

  return status;
}
