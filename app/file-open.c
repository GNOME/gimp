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

#include "core/gimpimage.h"

#include "file-open.h"
#include "file-utils.h"
#include "plug_in.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


GSList *load_procs = NULL;


/*  public functions  */

GimpImage *
file_open_image (Gimp          *gimp,
		 const gchar   *filename,
		 const gchar   *raw_filename,
		 const gchar   *open_mode,
		 PlugInProcDef *file_proc,
		 RunModeType    run_mode,
		 gint          *status)
{
  ProcRecord    *proc;
  Argument      *args;
  Argument      *return_vals;
  gint           gimage_id;
  gint           i;
  struct stat    statbuf;

  *status = GIMP_PDB_CANCEL;  /* inhibits error messages by caller */

  if (! file_proc)
    file_proc = file_proc_find (load_procs, filename);

  if (! file_proc)
    {
      /*  no errors when making thumbnails  */
      if (run_mode == RUN_INTERACTIVE)
	g_message (_("%s failed.\n"
		     "%s: Unknown file type."),
		   open_mode, filename);

      return NULL;
    }

  /* check if we are opening a file */
  if (stat (filename, &statbuf) == 0)
    {
      uid_t euid;
      gid_t egid;

      if (! (statbuf.st_mode & S_IFREG))
	{
	  /*  no errors when making thumbnails  */
	  if (run_mode == RUN_INTERACTIVE)
	    g_message (_("%s failed.\n"
			 "%s is not a regular file."),
		       open_mode, filename);

	  return NULL;
	}

      euid = geteuid ();
      egid = getegid ();

      if (! ((statbuf.st_mode & S_IRUSR) ||

	     ((statbuf.st_mode & S_IRGRP) &&
	      (statbuf.st_uid != euid)) ||

	     ((statbuf.st_mode & S_IROTH) &&
	      (statbuf.st_uid != euid) &&
	      (statbuf.st_gid != egid))))
	{
	  /*  no errors when making thumbnails  */
	  if (run_mode == RUN_INTERACTIVE)
	    g_message (_("%s failed.\n"
			 "%s: Permission denied."),
		       open_mode, filename);

	  return NULL;
	}
    }

  proc = &file_proc->db_info;

  args = g_new0 (Argument, proc->num_args);

  for (i = 0; i < proc->num_args; i++)
    args[i].arg_type = proc->args[i].arg_type;

  args[0].value.pdb_int     = run_mode;
  args[1].value.pdb_pointer = (gchar *) filename;
  args[2].value.pdb_pointer = (gchar *) raw_filename;

  return_vals = procedural_db_execute (gimp, proc->name, args);

  *status   = return_vals[0].value.pdb_int;
  gimage_id = return_vals[1].value.pdb_int;

  procedural_db_destroy_args (return_vals, proc->num_values);
  g_free (args);

  if (*status == GIMP_PDB_SUCCESS && gimage_id != -1)
    {
      return gimp_image_get_by_ID (gimage_id);
    }

  return NULL;
}

gchar *
file_open_absolute_filename (const gchar *name)
{
  PlugInProcDef *proc;
  GSList        *procs;
  GSList        *prefixes;
  gchar         *absolute;
  gchar         *current;

  g_return_val_if_fail (name != NULL, NULL);

  /*  check for prefixes like http or ftp  */
  for (procs = load_procs; procs; procs = g_slist_next (procs))
    {
      proc = (PlugInProcDef *)procs->data;

      for (prefixes = proc->prefixes_list;
	   prefixes;
	   prefixes = g_slist_next (prefixes))
	{
	  if (strncmp (name, prefixes->data, strlen (prefixes->data)) == 0)
	    return g_strdup (name);
	}
     }

  if (g_path_is_absolute (name))
    return g_strdup (name);

  current = g_get_current_dir ();
  absolute = g_strconcat (current, G_DIR_SEPARATOR_S, name, NULL);
  g_free (current);

  return absolute;
}
