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

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <io.h>
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWGRP
#define S_IWGRP (_S_IWRITE>>3)
#define S_IWOTH (_S_IWRITE>>6)
#endif
#ifndef S_IRGRP
#define S_IRGRP (_S_IREAD>>3)
#define S_IROTH (_S_IREAD>>6)
#endif
#define uid_t gint
#define gid_t gint
#define geteuid() 0
#define getegid() 0
#endif

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpcoreconfig.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "docindex.h"
#include "gui/menus.h"

#include "file-save.h"
#include "file-utils.h"
#include "plug_in.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


GSList *save_procs = NULL;


/*  public functions  */

GimpPDBStatusType
file_save (GimpImage   *gimage,
	   const gchar *filename,
	   const gchar *raw_filename,
           RunModeType  run_mode,
	   gboolean     set_filename)
{
  PlugInProcDef *file_proc;
  ProcRecord    *proc;
  Argument      *args;
  Argument      *return_vals;
  gint           status;
  gint           i;
  struct stat    statbuf;

  if (gimp_image_active_drawable (gimage) == NULL)
    return GIMP_PDB_EXECUTION_ERROR;

  file_proc = gimp_image_get_save_proc (gimage);

  if (!file_proc)
    file_proc = file_proc_find (save_procs, raw_filename);

  if (!file_proc)
    {
      g_message (_("Save failed.\n"
		   "%s: Unknown file type."),
		 filename);

      return GIMP_PDB_CANCEL;  /* inhibits error messages by caller */
    }

  /* check if we are saving to a file */
  if (stat (filename, &statbuf) == 0)
    {
      uid_t euid;
      gid_t egid;

      if (! (statbuf.st_mode & S_IFREG))
        {
	  g_message (_("Save failed.\n"
		       "%s is not a regular file."),
		     filename);

          return GIMP_PDB_CANCEL;  /* inhibits error messages by caller */
        }

      euid = geteuid ();
      egid = getegid ();

      if (! ((statbuf.st_mode & S_IWUSR) ||

             ((statbuf.st_mode & S_IWGRP) &&
              (statbuf.st_uid != euid)) ||

             ((statbuf.st_mode & S_IWOTH) &&
              (statbuf.st_uid != euid) &&
              (statbuf.st_gid != egid))))
        {
	  g_message (_("Save failed.\n"
		       "%s: Permission denied."),
		     filename);

          return GIMP_PDB_CANCEL;  /* inhibits error messages by caller */
        }
    }

  /* ref the image, so it can't get deleted during save */
  g_object_ref (G_OBJECT (gimage));

  proc = &file_proc->db_info;

  args = g_new0 (Argument, proc->num_args);

  for (i = 0; i < proc->num_args; i++)
    args[i].arg_type = proc->args[i].arg_type;

  args[0].value.pdb_int     = run_mode;
  args[1].value.pdb_int     = gimp_image_get_ID (gimage);
  args[2].value.pdb_int     = gimp_drawable_get_ID (gimp_image_active_drawable (gimage));
  args[3].value.pdb_pointer = (gpointer) filename;
  args[4].value.pdb_pointer = (gpointer) raw_filename;

  return_vals = procedural_db_execute (gimage->gimp, proc->name, args);

  status = return_vals[0].value.pdb_int;

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  set this image to clean  */
      gimp_image_clean_all (gimage);

      gimp_documents_add (gimage->gimp, filename);
      document_index_add (filename);
      menus_last_opened_add (filename);

      /*  use the same plug-in for this image next time  */
      /* DISABLED - gets stuck on first saved format... needs
	 attention --Adam */
      /* gimage_set_save_proc(gimage, file_proc); */

      /* Write a thumbnail for the saved image, where appropriate */
      switch (gimage->gimp->config->thumbnail_mode)
	{
	case 0:
	  break;
	default:
	  {
	    TempBuf *tempbuf;

	    tempbuf = make_thumb_tempbuf (gimage);
	    file_save_thumbnail (gimage, filename, tempbuf);
	  }
	}

      if (set_filename)
	{
	  /*  set the image title  */
	  gimp_object_set_name (GIMP_OBJECT (gimage), filename);
	}
    }

  g_free (return_vals);
  g_free (args);

  g_object_unref (G_OBJECT (gimage));

  return status;
}

/* Set "gimage"s save handler to "save_proc", then save the image.  
 * Hide the dialog if all went well, otherwise make the user knows an
 * error happened and leave the dialog up.  Make sure it's sensitive.
 */
gboolean
file_save_with_proc (GimpImage     *gimage,
		     const gchar   *full_filename,
		     const gchar   *raw_filename,
		     PlugInProcDef *save_proc,
		     gboolean       set_filename)
{
  gint     status  = GIMP_PDB_EXECUTION_ERROR;
  gboolean success = FALSE;

  if (gimage != NULL)
    {
      gimp_image_set_save_proc (gimage, save_proc);
      status = file_save (gimage,
                          full_filename,
                          raw_filename,
                          RUN_INTERACTIVE,
                          set_filename);

      if (status == GIMP_PDB_SUCCESS)
        success = TRUE;
    }

  /* If there was an error but file_save() didn't print an error
   * message, then we'd better.
   */
  if (status != GIMP_PDB_SUCCESS && status != GIMP_PDB_CANCEL)
    g_message (_("Save failed.\n%s"), full_filename);

  return success;
}
