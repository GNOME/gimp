/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#ifdef __EMX__
#include <process.h>
#endif
#include "config.h"
#include "libgimp/gimp.h"
#include "libgimp/stdplugins-intl.h"

/* Author: Josh MacDonald. */

static void   query      (void);
static void   run        (gchar       *name,
                          gint         nparams,
                          GParam      *param,
                          gint        *nreturn_vals,
                          GParam     **return_vals);
static gint32 load_image (gchar       *filename,
			  GStatusType *status /* return value */);

GPlugInInfo PLUG_IN_INFO =
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
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name entered" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static gint nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  INIT_I18N();

  gimp_install_procedure ("file_url_load",
                          _("loads files given a URL"),
                          _("You need to have GNU Wget installed."),
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1997",
                          "<Load>/URL",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_register_load_handler ("file_url_load",
			      "",
			      "http:,ftp:");
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType  run_mode;
  GStatusType   status = STATUS_SUCCESS;
  gint32        image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_EXECUTION_ERROR;

  if (strcmp (name, "file_url_load") == 0)
    {
      image_ID = load_image (param[2].data.d_string, &status);

      if (image_ID != -1 &&
	  status == STATUS_SUCCESS)
	{
	  *nreturn_vals = 2;
	  values[1].type         = PARAM_IMAGE;
	  values[1].data.d_image = image_ID;
	}
    }
  else
    {
      status = STATUS_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static gint32
load_image (gchar       *filename,
	    GStatusType *status)
{
  GParam* params;
  gint retvals;
  gchar *ext = strrchr (filename, '.');
  gchar *tmpname;
  gint pid;
  gint process_status;

  if (!ext || ext[1] == 0 || strchr(ext, '/'))
    {
      g_message ("url: can't open URL without an extension\n");
      *status = STATUS_CALLING_ERROR;
      return -1;
    }

  params = gimp_run_procedure ("gimp_temp_name",
			       &retvals,
			       PARAM_STRING, ext + 1,
			       PARAM_END);

  tmpname = g_strdup (params[1].data.d_string);
  gimp_destroy_params (params, retvals);

#ifndef __EMX__
  if ((pid = fork()) < 0)
    {
      g_message ("url: fork failed: %s\n", g_strerror(errno));
      g_free (tmpname);
      *status = STATUS_EXECUTION_ERROR;
      return -1;
    }
  else if (pid == 0)
    {
      execlp ("wget", "wget", filename, "-O", tmpname, NULL);
      g_message ("url: exec failed: wget: %s\n", g_strerror(errno));
      g_free (tmpname);
      _exit(127);
    }
  else
#else /* __EMX__ */
  pid = spawnlp (P_NOWAIT, "wget", "wget", filename, "-O", tmpname, NULL);
  if (pid == -1)
    {
      g_message ("url: spawn failed: %s\n", g_strerror(errno));
      g_free (tmpname);
      *status = STATUS_EXECUTION_ERROR;
      return -1;
    }
#endif
    {
      waitpid (pid, &process_status, 0);

      if (!WIFEXITED (process_status) ||
	  WEXITSTATUS (process_status) != 0)
	{
	  g_message ("url: wget exited abnormally on URL %s\n", filename);
	  g_free (tmpname);
	  *status = STATUS_EXECUTION_ERROR;
	  return -1;
	}
    }

  params = gimp_run_procedure ("gimp_file_load",
			       &retvals,
			       PARAM_INT32, 0,
			       PARAM_STRING, tmpname,
			       PARAM_STRING, tmpname,
			       PARAM_END);

  unlink (tmpname);
  g_free (tmpname);

  *status = params[0].data.d_status;

  if (params[0].data.d_status != STATUS_SUCCESS)
    {
      return -1;
    }
  else
    {
      gimp_image_set_filename (params[1].data.d_int32, filename);
      return params[1].data.d_int32;
    }
}
