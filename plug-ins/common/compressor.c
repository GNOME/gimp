/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Daniel Risacher, magnus@alum.mit.edu
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

/* Minor changes to support file magic */
/* 4 Oct 1997 -- Risacher */

/* gzip plug-in for the gimp */
/* loosley based on url.c by */
/* Josh MacDonald, jmacd@cs.berkeley.edu */

/* and, very loosely on hrz.c by */
/* Albert Cahalan <acahalan at cs.uml.edu> */

/* This is reads and writes gziped image files for the Gimp
 *
 * You need to have gzip installed for it to work.
 *
 * It should work with file names of the form
 * filename.foo.gz where foo is some already-recognized extension
 *
 * and it also works for names of the form
 * filename.xcfgz - which is equivalent to
 * filename.xcf.gz
 *
 * I added the xcfgz bit because having a default extension of xcf.gz
 * can confuse the file selection dialog box somewhat, forcing the
 * user to type sometimes when he/she otherwise wouldn't need to.
 *
 * I later decided I didn't like it because I don't like to bloat
 * the file-extension namespace.  But I left in the recognition
 * feature/bug so if people want to use files named foo.xcfgz by
 * default, they can just hack their pluginrc file.
 *
 * to do this hack, change :
 *                      "xcf.gz,gz,xcfgz"
 * to
 *                      "xcfgz,gz,xcf.gz"
 *
 *
 * -Dan Risacher, 0430 CDT, 26 May 1997
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <sys/stat.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#include <libgimp/gimp.h>

#ifdef G_OS_WIN32
#define STRICT
#define WinMain WinMain_foo
#include <windows.h>
#include <io.h> /* _get_osfhandle */
#undef WinMain
#endif

#include "libgimp/stdplugins-intl.h"


/* Author 1: Josh MacDonald (url.c) */
/* Author 2: Daniel Risacher (gz.c) */

/* According to USAF Lt Steve Werhle, US DoD software development
 * contracts average about $25 USD per source line of code (SLOC).  By
 * that metric, I figure this plug-in is worth about $10,000 USD */
/* But you got it free.   Magic of Gnu. */


static void          query       (void);
static void          run         (const gchar       *name,
				  gint               nparams,
				  const GimpParam   *param,
				  gint              *nreturn_vals,
				  GimpParam        **return_vals);

static gint32        load_image  (const gchar       *filename,
				  gint32             run_mode,
				  GimpPDBStatusType *status /* return value */);
static GimpPDBStatusType   save_image  (const gchar *filename,
                                        gint32       image_ID,
                                        gint32       drawable_ID,
                                        gint32       run_mode);

static gboolean      valid_file     (const gchar    *filename);
static const gchar * find_extension (const gchar    *filename);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename", "The name of the file to load" },
    { GIMP_PDB_STRING, "raw_filename", "The name entered" }
  };
  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" },
  };

  static GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Drawable to save" },
    { GIMP_PDB_STRING, "filename", "The name of the file to save the image in" },
    { GIMP_PDB_STRING, "raw_filename", "The name of the file to save the image in" }
  };

  gimp_install_procedure ("file_gz_load",
                          "loads files compressed with gzip",
                          "You need to have gzip installed.",
                          "Daniel Risacher",
                          "Daniel Risacher, Spencer Kimball and Peter Mattis",
                          "1995-1997",
                          "<Load>/gzip",
			  NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_install_procedure ("file_gz_save",
                          "saves files compressed with gzip",
                          "You need to have gzip installed.",
                          "Daniel Risacher",
                          "Daniel Risacher, Spencer Kimball and Peter Mattis",
                          "1995-1997",
                          "<Save>/gzip",
			  "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_gz_load",
				    "xcf.gz,gz,xcfgz",
				    "",
				    "0,string,\037\213");
  gimp_register_save_handler       ("file_gz_save",
				    "xcf.gz,gz,xcfgz",
				    "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam values[2];
  GimpRunMode  run_mode;
  GimpPDBStatusType   status = GIMP_PDB_SUCCESS;
  gint32        image_ID;

  run_mode = param[0].data.d_int32;

  INIT_I18N();

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, "file_gz_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string,
			     param[0].data.d_int32,
			     &status);

      if (image_ID != -1 &&
	  status == GIMP_PDB_SUCCESS)
	{
	  *nreturn_vals = 2;
	  values[1].type         = GIMP_PDB_IMAGE;
	  values[1].data.d_image = image_ID;
	}
    }
  else if (strcmp (name, "file_gz_save") == 0)
    {
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  break;
	case GIMP_RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 4)
	    status = GIMP_PDB_CALLING_ERROR;
	  break;
	case GIMP_RUN_WITH_LAST_VALS:
	  break;

	default:
	  break;
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  status = save_image (param[3].data.d_string,
			       param[1].data.d_int32,
			       param[2].data.d_int32,
			       param[0].data.d_int32);
	 }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static GimpPDBStatusType
save_image (const gchar *filename,
	    gint32       image_ID,
	    gint32       drawable_ID,
	    gint32       run_mode)
{
  const gchar *ext;
  gchar       *tmpname;
#ifndef G_OS_WIN32
  FILE        *f;
  gint         pid;
  gint         wpid;
  gint         process_status;
#else
  FILE        *in;
  FILE        *out;
  STARTUPINFO         startupinfo;
  PROCESS_INFORMATION processinfo;
#endif

  if (NULL == (ext = find_extension (filename)))
    {
      g_message (_("No sensible extension, saving as compressed XCF."));
      ext = ".xcf";
    }

  /* get a temp name with the right extension and save into it. */

  tmpname = gimp_temp_name (ext + 1);

  if (! (gimp_file_save (run_mode,
			 image_ID,
			 drawable_ID,
			 tmpname,
			 tmpname) && valid_file (tmpname)))
    {
      unlink (tmpname);
      g_free (tmpname);
      return GIMP_PDB_EXECUTION_ERROR;
    }

#ifndef G_OS_WIN32
  /* fork off a gzip process */
  if ((pid = fork ()) < 0)
    {
      g_message ("fork() failed: %s", g_strerror (errno));
      g_free (tmpname);
      return GIMP_PDB_EXECUTION_ERROR;
    }
  else if (pid == 0)
    {
      if (!(f = fopen (filename, "w")))
	{
	  g_message (_("Could not open '%s' for writing: %s"),
                     gimp_filename_to_utf8 (filename), g_strerror (errno));
	  g_free (tmpname);
	  _exit (127);
	}

      /* make stdout for this process be the output file */
      if (-1 == dup2 (fileno (f), fileno (stdout)))
	g_message ("dup2() failed: %s", g_strerror (errno));

      /* and gzip into it */
      execlp ("gzip", "gzip", "-cfn", tmpname, NULL);
      g_message ("execlp(\"gzip -cfn\") failed: %s", g_strerror (errno));
      g_free (tmpname);
      _exit(127);
    }
  else
    {
      wpid = waitpid (pid, &process_status, 0);

      if ((wpid < 0)
	  || !WIFEXITED (process_status)
	  || (WEXITSTATUS (process_status) != 0))
	{
	  g_message ("gzip exited abnormally on file '%s'",
                     gimp_filename_to_utf8 (tmpname));
	  g_free (tmpname);
	  return 0;
	}
    }
#else  /* G_OS_WIN32 */
  in = fopen (tmpname, "rb");
  out = fopen (filename, "wb");

  startupinfo.cb = sizeof (STARTUPINFO);
  startupinfo.lpReserved = NULL;
  startupinfo.lpDesktop = NULL;
  startupinfo.lpTitle = NULL;
  startupinfo.dwFlags =
    STARTF_FORCEOFFFEEDBACK | STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
  startupinfo.wShowWindow = SW_SHOWMINNOACTIVE;
  startupinfo.cbReserved2 = 0;
  startupinfo.lpReserved2 = NULL;
  startupinfo.hStdInput = (HANDLE) _get_osfhandle (fileno (in));
  startupinfo.hStdOutput = (HANDLE)  _get_osfhandle (fileno (out));
  startupinfo.hStdError = GetStdHandle (STD_ERROR_HANDLE);

  if (!CreateProcess (NULL, "minigzip", NULL, NULL,
		      TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL,
		      &startupinfo, &processinfo))
    {
      g_message ("CreateProcess failed. Minigzip.exe not in the path?");
      g_free (tmpname);
      _exit (127);
    }
  CloseHandle (processinfo.hThread);
  WaitForSingleObject (processinfo.hProcess, INFINITE);
#endif /* G_OS_WIN32 */

  unlink (tmpname);
  g_free (tmpname);

  return GIMP_PDB_SUCCESS;
}

static gint32
load_image (const gchar       *filename,
	    gint32             run_mode,
	    GimpPDBStatusType *status /* return value */)
{
  gint32       image_ID;
  const gchar *ext;
  gchar       *tmpname;
#ifndef G_OS_WIN32
  gint         pid;
  gint         wpid;
  gint         process_status;
#else
  FILE        *in, *out;
  SECURITY_ATTRIBUTES secattr;
  STARTUPINFO         startupinfo;
  PROCESS_INFORMATION processinfo;
#endif

  if (NULL == (ext = find_extension (filename)))
    {
      g_message (_("No sensible extension, attempting to load "
                   "with file magic."));
      ext = ".foo";
    }

  /* find a temp name */
  tmpname = gimp_temp_name (ext + 1);

#ifndef G_OS_WIN32
/* fork off a g(un)zip and wait for it */
  if ((pid = fork ()) < 0)
    {
      g_message ("fork() failed: %s", g_strerror (errno));
      g_free (tmpname);
      *status = GIMP_PDB_EXECUTION_ERROR;
      return -1;
    }
  else if (pid == 0)  /* child process */
    {
      FILE *f;

      if (! (f = fopen (tmpname, "w")))
        {
          g_message (_("Could not open '%s' for writing: %s"),
                     gimp_filename_to_utf8 (tmpname), g_strerror (errno));
          g_free (tmpname);
          _exit(127);
        }

      /* make stdout for this child process be the temp file */
      if (-1 == dup2 (fileno (f), fileno (stdout)))
	{
	  g_free (tmpname);
	  g_message ("dup2() failed: %s", g_strerror (errno));
	}

      /* and unzip into it */
      execlp ("gzip", "gzip", "-cfd", filename, NULL);
      g_message ("execlp(\"gzip -cfd\") failed: %s", g_strerror (errno));
      g_free (tmpname);
      _exit(127);
    }
  else  /* parent process */
    {
      wpid = waitpid (pid, &process_status, 0);

      if ((wpid < 0)
	  || !WIFEXITED (process_status)
	  || (WEXITSTATUS (process_status) != 0))
	{
	  g_message ("gzip exited abnormally on file '%s'",
                     gimp_filename_to_utf8 (filename));
	  g_free (tmpname);
	  *status = GIMP_PDB_EXECUTION_ERROR;
	  return -1;
	}
    }
#else
  in = fopen (filename, "rb");
  out = fopen (tmpname, "wb");

  startupinfo.cb = sizeof (STARTUPINFO);
  startupinfo.lpReserved = NULL;
  startupinfo.lpDesktop = NULL;
  startupinfo.lpTitle = NULL;
  startupinfo.dwFlags =
    STARTF_FORCEOFFFEEDBACK | STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
  startupinfo.wShowWindow = SW_SHOWMINNOACTIVE;
  startupinfo.cbReserved2 = 0;
  startupinfo.lpReserved2 = NULL;
  startupinfo.hStdInput = (HANDLE) _get_osfhandle (fileno (in));
  startupinfo.hStdOutput = (HANDLE) _get_osfhandle (fileno (out));
  startupinfo.hStdError = GetStdHandle (STD_ERROR_HANDLE);

  if (!CreateProcess (NULL, "minigzip -d", NULL, NULL,
		      TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL,
		      &startupinfo, &processinfo))
    {
      g_message ("CreateProcess failed: %d", GetLastError ());
      g_free (tmpname);
      _exit (127);
    }
  CloseHandle (processinfo.hThread);
  WaitForSingleObject (processinfo.hProcess, INFINITE);
  fclose (in);
  fclose (out);
#endif /* G_OS_WIN32 */

  /* now that we un-gziped it, load the temp file */

  image_ID = gimp_file_load (run_mode, tmpname, tmpname);

  unlink (tmpname);
  g_free (tmpname);

  if (image_ID != -1)
    {
      *status = GIMP_PDB_SUCCESS;
      gimp_image_set_filename (image_ID, filename);
    }
  else
    *status = GIMP_PDB_EXECUTION_ERROR;

  return image_ID;
}

static gboolean
valid_file (const gchar *filename)
{
  gint stat_res;
  struct stat buf;

  stat_res = stat(filename, &buf);

  if ((0 == stat_res) && (buf.st_size > 0))
    return TRUE;
  else
    return FALSE;
}

static const gchar *
find_extension (const gchar *filename)
{
  gchar *filename_copy;
  gchar *ext;

  /* we never free this copy - aren't we evil! */
  filename_copy = g_strdup (filename);

  /* find the extension, boy! */
  ext = strrchr (filename_copy, '.');

  while (TRUE)
    {
      if (!ext || ext[1] == '\0' || strchr (ext, '/'))
	{
	  return NULL;
	}
      if (0 == g_ascii_strcasecmp (ext, ".xcfgz"))
	{
	  return ".xcf";  /* we've found it */
	}
      if (0 != g_ascii_strcasecmp (ext,".gz"))
	{
	  return ext;
	}
      else
	{
	  /* we found ".gz" so strip it, loop back, and look again */
	  *ext = '\0';
	  ext = strrchr (filename_copy, '.');
	}
    }
}
