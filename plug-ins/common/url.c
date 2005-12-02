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
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/wait.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define TIMEOUT "300"
#define BUFSIZE 1024

static void    query (void);
static void    run   (const gchar      *name,
                      gint              nparams,
                      const GimpParam  *param,
                      gint             *nreturn_vals,
                      GimpParam       **return_vals);

static gint32  load_image (const gchar       *filename,
                           GimpRunMode        run_mode,
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
    { GIMP_PDB_INT32,  "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw_filename", "The name entered" }
  };

  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  gimp_install_procedure ("file_url_load",
                          "loads files given a URL",
                          "You need to have GNU Wget installed.",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1997",
                          N_("URL"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_plugin_icon_register ("file_url_load",
                             GIMP_ICON_TYPE_STOCK_ID, GIMP_STOCK_WEB);
  gimp_register_load_handler ("file_url_load",
                              "",
                              "http:,https:,ftp:");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, "file_url_load") == 0)
    {
      image_ID = load_image (param[2].data.d_string, run_mode, &status);

      if (image_ID != -1 &&
          status == GIMP_PDB_SUCCESS)
        {
          *nreturn_vals = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static gint32
load_image (const gchar       *filename,
            GimpRunMode        run_mode,
            GimpPDBStatusType *status)
{
  gint32    image_ID;
  gchar    *ext = strrchr (filename, '.');
  gchar    *tmpname;
  gint      pid;
  gint      p[2];
  gboolean  name_image = FALSE;

  if (!ext || ext[1] == 0 || strchr (ext, '/'))
    {
      tmpname = gimp_temp_name ("xxx");
    }
  else
    {
      tmpname = gimp_temp_name (ext + 1);
      name_image = TRUE;
    }

  if (pipe (p) != 0)
    {
      g_message ("pipe() failed: %s", g_strerror (errno));
      g_free (tmpname);
      *status = GIMP_PDB_EXECUTION_ERROR;
      return -1;
    }

  if ((pid = fork()) < 0)
    {
      g_message ("fork() failed: %s", g_strerror (errno));
      g_free (tmpname);
      *status = GIMP_PDB_EXECUTION_ERROR;
      return -1;
    }
  else if (pid == 0)
    {
      close (p[0]);
      close (2);
      dup (p[1]);
      close (p[1]);

#ifdef HAVE_PUTENV
      /* produce deterministic output */
      putenv ("LANGUAGE=C");
      putenv ("LC_ALL=C");
      putenv ("LANG=C");
#endif

      execlp ("wget",
              "wget", "-v", "-e", "server-response=off", "-T", TIMEOUT,
              filename, "-O", tmpname, NULL);
      g_message ("exec() failed: wget: %s", g_strerror (errno));
      g_free (tmpname);
      _exit (127);
    }
  else
    {
      FILE     *input;
      gchar     buf[BUFSIZE];
      gboolean  seen_resolve = FALSE;
      gboolean  connected = FALSE;
      gboolean  redirect = FALSE;
      gboolean  file_found = FALSE;
      gchar     sizestr[32];
      gint      size = 0;
      gchar    *size_msg;
      gchar    *message;
      gint      i, j;
      gchar     dot;
      gint      kilobytes = 0;
      gboolean  finished = FALSE;

      gboolean  debug = FALSE;

#define DEBUG(x) if (debug) g_printerr (x)

      close (p[1]);

      input = fdopen (p[0], "r");

      /*  hardcoded and not-really-foolproof scanning of wget putput  */

    wget_begin:
      /*  Eat any Location lines */
      if (redirect && fgets (buf, BUFSIZE, input) == NULL)
        {
          g_message ("wget exited abnormally on URL '%s'", filename);
          g_free (tmpname);
          *status = GIMP_PDB_EXECUTION_ERROR;
          return -1;
        }

      redirect = FALSE;

      if (fgets (buf, BUFSIZE, input) == NULL)
        {
          /*  no message here because failing on the first line means
           *  that wget was not found
           */
          g_free (tmpname);
          *status = GIMP_PDB_EXECUTION_ERROR;
          return -1;
        }

      DEBUG (buf);

      /*  The second line is the local copy of the file  */
      if (fgets (buf, BUFSIZE, input) == NULL)
        {
          g_message ("wget exited abnormally on URL '%s'", filename);
          g_free (tmpname);
          *status = GIMP_PDB_EXECUTION_ERROR;
          return -1;
        }

      DEBUG (buf);

      /*  The third line is "Connecting to..."  */
      gimp_progress_init ("Connecting to server... "
                          "(timeout is "TIMEOUT" seconds)");

    read_connect:
      if (fgets (buf, BUFSIZE, input) == NULL)
        {
          g_message ("wget exited abnormally on URL '%s'", filename);
          g_free (tmpname);
          *status = GIMP_PDB_EXECUTION_ERROR;
          return -1;
        }
      else if (strstr (buf, "connected"))
        {
          connected = TRUE;
        }
      /* newer wgets have a "Resolving foo" line, so eat it */
      else if (!seen_resolve && strstr (buf, "Resolving"))
        {
          seen_resolve = TRUE;
          goto read_connect;
        }

      DEBUG (buf);

      /*  The fourth line is either the network request or an error  */
      gimp_progress_init ("Opening URL... "
                          "(timeout is "TIMEOUT" seconds)");

      if (fgets (buf, BUFSIZE, input) == NULL)
        {
          g_message ("wget exited abnormally on URL '%s'", filename);
          g_free (tmpname);
          *status = GIMP_PDB_EXECUTION_ERROR;
          return -1;
        }
      else if (! connected)
        {
          g_message ("A network error occured: %s", buf);

          DEBUG (buf);

          g_free (tmpname);
          *status = GIMP_PDB_EXECUTION_ERROR;
          return -1;
        }
      else if (strstr (buf, "302 Found"))
        {
          DEBUG (buf);

          connected = FALSE;
          seen_resolve = FALSE;

          redirect = TRUE;
          goto wget_begin;
        }

      DEBUG (buf);

      /*  The fifth line is either the length of the file or an error  */
      if (fgets (buf, BUFSIZE, input) == NULL)
        {
          g_message ("wget exited abnormally on URL '%s'", filename);
          g_free (tmpname);
          *status = GIMP_PDB_EXECUTION_ERROR;
          return -1;
        }
      else if (strstr (buf, "Length"))
        {
          file_found = TRUE;
        }
      else
        {
          g_message ("A network error occured: %s", buf);

          DEBUG (buf);

          g_free (tmpname);
          *status = GIMP_PDB_EXECUTION_ERROR;
          return -1;
        }

      DEBUG (buf);

      if (sscanf (buf, "Length: %31s", sizestr) != 1)
        {
          g_message ("Could not parse wget's file length message");
          g_free (tmpname);
          *status = GIMP_PDB_EXECUTION_ERROR;
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

      if (size > 0)
        size_msg = g_strdup_printf ("%d bytes", size);
      else
        size_msg = g_strdup ("unknown amount");

      /*  Start the actual download...  */
      message = g_strdup_printf ("Downloading %s of image data... "
                                 "(timeout is "TIMEOUT" seconds)", size_msg);
      gimp_progress_init (message);
      g_free (message);
      g_free (size_msg);

      /*  Switch to byte parsing wget's output...  */

      while (1)
        {
          dot = fgetc (input);

          if (feof (input))
            break;

          if (debug)
            {
              fputc (dot, stderr);
              fflush (stderr);
            }

          if (dot == '.')  /* one kilobyte */
            {
              kilobytes++;

              if (size > 0)
                gimp_progress_update ((gdouble) (kilobytes * 1024) /
                                      (gdouble) size);
            }
          else if (dot == ':')  /* the time string contains a ':' */
            {
              fgets (buf, BUFSIZE, input);

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
          g_message ("wget exited before finishing downloading URL\n'%s'",
                     filename);
          unlink (tmpname);
          g_free (tmpname);
          *status = GIMP_PDB_EXECUTION_ERROR;
          return -1;
        }
    }

  image_ID = gimp_file_load (run_mode, tmpname, tmpname);

  unlink (tmpname);
  g_free (tmpname);

  if (image_ID != -1)
    {
      *status = GIMP_PDB_SUCCESS;
      if (name_image)
        gimp_image_set_filename (image_ID, filename);
      else
        gimp_image_set_filename (image_ID, "");
    }
  else
    *status = GIMP_PDB_EXECUTION_ERROR;

  return image_ID;
}
