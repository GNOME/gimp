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

/* Author: Josh MacDonald. */

#include "config.h"

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

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define TIMEOUT "300"

static void   query (void);
static void   run   (gchar      *name,
		     gint        nparams,
		     GimpParam  *param,
		     gint       *nreturn_vals,
		     GimpParam **return_vals);

static gint32   load_image (gchar             *filename,
			    GimpRunModeType    run_mode,
			    GimpPDBStatusType *status /* return value */);

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
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name entered" }
  };
  static gint nload_args = sizeof (load_args) / sizeof (load_args[0]);

  static GimpParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" }
  };
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  gimp_install_procedure ("file_url_load",
                          "loads files given a URL",
                          "You need to have GNU Wget installed.",
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
run (gchar      *name,
     gint        nparams,
     GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam  values[2];
  GimpRunModeType   run_mode;
  GimpPDBStatusType status = STATUS_SUCCESS;
  gint32            image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_EXECUTION_ERROR;

  if (strcmp (name, "file_url_load") == 0)
    {
      image_ID = load_image (param[2].data.d_string,
			     run_mode,
			     &status);

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
load_image (gchar             *filename,
	    GimpRunModeType    run_mode,
	    GimpPDBStatusType *status)
{
  GimpParam *params;
  gint       retvals;
  gchar     *ext = strrchr (filename, '.');
  gchar     *tmpname;
  gint       pid;
  gint       process_status;
  gint       p[2];

  if (!ext || ext[1] == 0 || strchr(ext, '/'))
    {
      g_message ("url: can't open URL without an extension");
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
  if (pipe (p) != 0)
    {
      g_message ("url: pipe() failed: %s", g_strerror (errno));
      g_free (tmpname);
      *status = STATUS_EXECUTION_ERROR;
      return -1;
    }

  if ((pid = fork()) < 0)
    {
      g_message ("url: fork() failed: %s", g_strerror (errno));
      g_free (tmpname);
      *status = STATUS_EXECUTION_ERROR;
      return -1;
    }
  else if (pid == 0)
    {
      close (p[0]);
      close (2);
      dup (p[1]);
      close (p[1]);
      putenv ("LC_ALL=C");  /* produce deterministic output */
      execlp ("wget", "wget", "-T", TIMEOUT, filename, "-O", tmpname, NULL);
      g_message ("url: exec() failed: wget: %s", g_strerror (errno));
      g_free (tmpname);
      _exit (127);
    }
  else
    {
      if (run_mode == RUN_NONINTERACTIVE)
	{
	  waitpid (pid, &process_status, 0);

	  if (!WIFEXITED (process_status) ||
	      WEXITSTATUS (process_status) != 0)
	    {
	      g_message ("url: wget exited abnormally on URL %s", filename);
	      g_free (tmpname);
	      *status = STATUS_EXECUTION_ERROR;
	      return -1;
	    }
	}
      else
	{
	  FILE     *input;
	  gint      bufsize = strlen (filename) + 1024;
	  gchar     buf[bufsize + 1];
	  gboolean  connected = FALSE;
	  gboolean  file_found = FALSE;
	  gchar     sizestr[32];
	  gint      size = 0;
	  gchar    *message;
	  gint      i, j;
	  gchar     dot;
	  gint      kilobytes = 0;
	  gboolean  finished = FALSE;

	  gboolean  debug = FALSE;

#define DEBUG(x) if (debug) fprintf (stderr, (x))

	  close (p[1]);

	  input = fdopen (p[0], "r");

	  /*  hardcoded and not-really-foolproof scanning of wget putput  */

	  if (fgets (buf, bufsize, input) == NULL)
	    {
	      /*  no message here because failing on the first line means
	       *  that wget was not found
	       */
	      g_free (tmpname);
	      *status = STATUS_EXECUTION_ERROR;
	      return -1;
	    }

	  DEBUG (buf);

	  /*  The second line is the local copy of the file  */
	  if (fgets (buf, bufsize, input) == NULL)
	    {
	      g_message ("url: wget exited abnormally on URL\n%s", filename);
	      g_free (tmpname);
	      *status = STATUS_EXECUTION_ERROR;
	      return -1;
	    }

	  DEBUG (buf);

	  /*  The third line is "Connecting to..."  */
	  gimp_progress_init ("Connecting to server... "
			      "(timeout is "TIMEOUT" seconds)");

	  if (fgets (buf, bufsize, input) == NULL)
	    {
	      g_message ("url: wget exited abnormally on URL\n%s", filename);
	      g_free (tmpname);
	      *status = STATUS_EXECUTION_ERROR;
	      return -1;
	    }
	  else if (strstr (buf, "connected"))
	    {
	      connected = TRUE;
	    }

	  DEBUG (buf);

	  /*  The fourth line is either the network request or an error  */
	  gimp_progress_init ("Opening URL... "
			      "(timeout is "TIMEOUT" seconds)");

	  if (fgets (buf, bufsize, input) == NULL)
	    {
	      g_message ("url: wget exited abnormally on URL\n%s", filename);
	      g_free (tmpname);
	      *status = STATUS_EXECUTION_ERROR;
	      return -1;
	    }
	  else if (! connected)
	    {
	      g_message ("url: a network error occured: %s", buf);

	      DEBUG (buf);

	      g_free (tmpname);
	      *status = STATUS_EXECUTION_ERROR;
	      return -1;
	    }

	  DEBUG (buf);

	  /*  The fifth line is either the length of the file or an error  */
	  if (fgets (buf, bufsize, input) == NULL)
	    {
	      g_message ("url: wget exited abnormally on URL\n%s", filename);
	      g_free (tmpname);
	      *status = STATUS_EXECUTION_ERROR;
	      return -1;
	    }
	  else if (strstr (buf, "Length"))
	    {
	      file_found = TRUE;
	    }
	  else
	    {
	      g_message ("url: a network error occured: %s", buf);

	      DEBUG (buf);

	      g_free (tmpname);
	      *status = STATUS_EXECUTION_ERROR;
	      return -1;
	    }

	  DEBUG (buf);

	  if (sscanf (buf, "Length: %31s", sizestr) != 1)
	    {
	      g_message ("url: could not parse wget's file length message");
	      g_free (tmpname);
	      *status = STATUS_EXECUTION_ERROR;
	      return -1;
	    }

	  /*  strip away commas  */
	  for (i = 0, j = 0; i < sizeof (sizestr); i++, j++)
	    {
	      if (sizestr[i] == ',')
		i++;

	      sizestr[j] = sizestr[i];

	      if (sizestr[j] == '\0')
		break;
	    }

	  size = atoi (sizestr);

	  /*  Start the actual download...  */
	  message = g_strdup_printf ("Downloading %d bytes of image data... "
				     "(timeout is "TIMEOUT" seconds)", size);
	  gimp_progress_init (message);
	  g_free (message);

	  /*  Switch to byte parsing wget's output...  */

	  while (1)
	    {
	      dot = fgetc (input);

	      if (dot == EOF)
		break;

	      if (debug)
		{
		  fputc (dot, stderr);
		  fflush (stderr);
		}

	      if (dot == '.')  /* one kilobyte */
		{
		  kilobytes++;
		  gimp_progress_update ((gdouble) (kilobytes * 1024) /
					(gdouble) size);
		}
	      else if (dot == ':')  /* the time string contains a ':' */
		{
		  fgets (buf, bufsize, input);

		  DEBUG (buf);

		  if (! strstr (buf, "error"))
		    {
		      finished = TRUE;
		      gimp_progress_update (1.0);
		    }

		  break;
		}
	    }

	  if (! finished)
	    {
	      g_message ("url: wget exited before finishing downloading URL\n%s",
			 filename);
	      unlink (tmpname);
	      g_free (tmpname);
	      *status = STATUS_EXECUTION_ERROR;
	      return -1;
	    }
	}
    }
#else /* __EMX__ */
  {
    pid = spawnlp (P_NOWAIT,
		   "wget",
		   "wget", "-T", TIMEOUT, filename, "-O", tmpname, NULL);

    if (pid == -1)
      {
	g_message ("url: spawn failed: %s", g_strerror (errno));
	g_free (tmpname);
	*status = STATUS_EXECUTION_ERROR;
	return -1;
      }

    waitpid (pid, &process_status, 0);

    if (!WIFEXITED (process_status) ||
	WEXITSTATUS (process_status) != 0)
      {
	g_message ("url: wget exited abnormally on URL %s", filename);
	g_free (tmpname);
	*status = STATUS_EXECUTION_ERROR;
	return -1;
      }
  }
#endif

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
