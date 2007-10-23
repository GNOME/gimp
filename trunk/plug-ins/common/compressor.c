/* GIMP - The GNU Image Manipulation Program
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

/* compressor plug-in for GIMP           */
/* based on gz.c which in turn is        */
/* loosley based on url.c by             */
/* Josh MacDonald, jmacd@cs.berkeley.edu */

/* and, very loosely on hrz.c by */
/* Albert Cahalan <acahalan at cs.uml.edu> */

/* This is reads and writes compressed image files for GIMP
 *
 * You need to have gzip or bzip2 installed for it to work.
 *
 * It should work with file names of the form
 * filename.foo.[gz|bz2] where foo is some already-recognized extension
 *
 * and it also works for names of the form
 * filename.xcf[gz|bz2] - which is equivalent to
 * filename.xcf.[gz|bz2]
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

#include <string.h>
#include <errno.h>

#include <sys/types.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include <libgimp/gimp.h>

#ifdef G_OS_WIN32
#define STRICT
#define WinMain WinMain_foo
#include <windows.h>
#include <io.h> /* _get_osfhandle */
#undef WinMain
#endif

#include "libgimp/stdplugins-intl.h"


/* Author 1: Josh MacDonald (url.c)          */
/* Author 2: Daniel Risacher (gz.c)          */
/* Author 3: Michael Natterer (compressor.c) */

/* According to USAF Lt Steve Werhle, US DoD software development
 * contracts average about $25 USD per source line of code (SLOC).  By
 * that metric, I figure this plug-in is worth about $10,000 USD */
/* But you got it free.   Magic of Gnu. */


typedef struct _Compressor Compressor;

struct _Compressor
{
  const gchar *file_type;
  const gchar *mime_type;
  const gchar *extensions;
  const gchar *magic;
  const gchar *xcf_extension;
  const gchar *generic_extension;

  const gchar *load_proc;
  const gchar *load_blurb;
  const gchar *load_help;
  const gchar *load_program;
  const gchar *load_options;
  const gchar *load_program_win32;

  const gchar *save_proc;
  const gchar *save_blurb;
  const gchar *save_help;
  const gchar *save_program;
  const gchar *save_options;
  const gchar *save_program_win32;
};


static void                query          (void);
static void                run            (const gchar       *name,
                                           gint               nparams,
                                           const GimpParam   *param,
                                           gint              *nreturn_vals,
                                           GimpParam        **return_vals);

static GimpPDBStatusType   save_image     (const Compressor  *compressor,
                                           const gchar       *filename,
                                           gint32             image_ID,
                                           gint32             drawable_ID,
                                           gint32             run_mode);
static gint32              load_image     (const Compressor  *compressor,
                                           const gchar       *filename,
                                           gint32             run_mode,
                                           GimpPDBStatusType *status);

static gboolean            valid_file     (const gchar       *filename);
static const gchar       * find_extension (const Compressor  *compressor,
                                           const gchar       *filename);


static const Compressor compressors[] =
{
  {
    N_("gzip archive"),
    "application/x-gzip",
    "xcf.gz,gz,xcfgz",
    "0,string,\037\213",
    ".xcfgz",
    ".gz",

    "file-gz-load",
    "loads files compressed with gzip",
    "You need to have gzip installed.",
    "gzip", "-cfd",
    "minigzip -d",

    "file-gz-save",
    "saves files compressed with gzip",
    "You need to have gzip installed.",
    "gzip", "-cfn",
    "minigzip"
  },

  {
    N_("bzip archive"),
    "application/x-bzip",
    "xcf.bz2,bz2,xcfbz2",
    "0,string,BZh",
    ".xcfbz2",
    ".bz2",

    "file-bz2-load",
    "loads files compressed with bzip2",
    "You need to have bzip2 installed.",
    "bzip2", "-cfd",
    "bzip2 -cfd",

    "file-bz2-save",
    "saves files compressed with bzip2",
    "You need to have bzip2 installed",
    "bzip2", "-cf",
    "bzip2 -cf"
  }
};

const GimpPlugInInfo PLUG_IN_INFO =
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
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name entered"             }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" },
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save"             },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to "
                                         "save the image in"            },
    { GIMP_PDB_STRING,   "raw-filename", "The name entered"             },
  };

  gint i;

  for (i = 0; i < G_N_ELEMENTS (compressors); i++)
    {
      const Compressor *compressor = &compressors[i];

      gimp_install_procedure (compressor->load_proc,
                              compressor->load_blurb,
                              compressor->load_help,
                              "Daniel Risacher",
                              "Daniel Risacher, Spencer Kimball and Peter Mattis",
                              "1995-1997",
                              compressor->file_type,
                              NULL,
                              GIMP_PLUGIN,
                              G_N_ELEMENTS (load_args),
                              G_N_ELEMENTS (load_return_vals),
                              load_args, load_return_vals);

      gimp_register_file_handler_mime (compressor->load_proc,
                                       compressor->mime_type);
      gimp_register_magic_load_handler (compressor->load_proc,
                                        compressor->extensions,
                                        "",
                                        compressor->magic);

      gimp_install_procedure (compressor->save_proc,
                              compressor->save_blurb,
                              compressor->save_help,
                              "Daniel Risacher",
                              "Daniel Risacher, Spencer Kimball and Peter Mattis",
                              "1995-1997",
                              compressor->file_type,
                              "RGB*, GRAY*, INDEXED*",
                              GIMP_PLUGIN,
                              G_N_ELEMENTS (save_args), 0,
                              save_args, NULL);

      gimp_register_file_handler_mime (compressor->save_proc,
                                       compressor->mime_type);
      gimp_register_save_handler (compressor->save_proc,
                                  compressor->extensions, "");
    }
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image_ID;
  gint               i;

  run_mode = param[0].data.d_int32;

  INIT_I18N();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  for (i = 0; i < G_N_ELEMENTS (compressors); i++)
    {
      const Compressor *compressor = &compressors[i];

      if (! strcmp (name, compressor->load_proc))
        {
          image_ID = load_image (compressor,
                                 param[1].data.d_string,
                                 param[0].data.d_int32,
                                 &status);

          if (image_ID != -1 && status == GIMP_PDB_SUCCESS)
            {
              *nreturn_vals = 2;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_ID;
            }

          break;
        }
      else if (! strcmp (name, compressor->save_proc))
        {
          switch (run_mode)
            {
            case GIMP_RUN_INTERACTIVE:
              break;
            case GIMP_RUN_NONINTERACTIVE:
              /*  Make sure all the arguments are there!  */
              if (nparams != 5)
                status = GIMP_PDB_CALLING_ERROR;
              break;
            case GIMP_RUN_WITH_LAST_VALS:
              break;

            default:
              break;
            }

          if (status == GIMP_PDB_SUCCESS)
            status = save_image (compressor,
                                 param[3].data.d_string,
                                 param[1].data.d_int32,
                                 param[2].data.d_int32,
                                 param[0].data.d_int32);

          break;
        }
    }

  if (i == G_N_ELEMENTS (compressors))
    status = GIMP_PDB_CALLING_ERROR;

  values[0].data.d_status = status;
}

static GimpPDBStatusType
save_image (const Compressor *compressor,
            const gchar      *filename,
            gint32            image_ID,
            gint32            drawable_ID,
            gint32            run_mode)
{
  const gchar *ext;
  gchar       *tmpname;

  ext = find_extension (compressor, filename);

  if (! ext)
    {
      g_message (_("No sensible file extension, saving as compressed XCF."));
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
      g_unlink (tmpname);
      g_free (tmpname);
      return GIMP_PDB_EXECUTION_ERROR;
    }

#ifndef G_OS_WIN32
  {
    gint pid;

    /* fork off a compressor process */
    if ((pid = fork ()) < 0)
      {
        g_message ("fork() failed: %s", g_strerror (errno));
        g_free (tmpname);

        return GIMP_PDB_EXECUTION_ERROR;
      }
    else if (pid == 0)
      {
        FILE *f;

        if (!(f = g_fopen (filename, "wb")))
          {
            g_message (_("Could not open '%s' for writing: %s"),
                       gimp_filename_to_utf8 (filename), g_strerror (errno));
            g_free (tmpname);
            _exit (127);
          }

        /* make stdout for this process be the output file */
        if (dup2 (fileno (f), fileno (stdout)) == -1)
          g_message ("dup2() failed: %s", g_strerror (errno));

        /* and compress into it */
        execlp (compressor->save_program,
                compressor->save_program,
                compressor->save_options, tmpname, NULL);

        g_message ("execlp(\"%s %s\") failed: %s",
                   compressor->save_program,
                   compressor->save_options,
                   g_strerror (errno));
        g_free (tmpname);
        _exit(127);
      }
    else
      {
        gint wpid;
        gint process_status;

        wpid = waitpid (pid, &process_status, 0);

        if ((wpid < 0)
            || !WIFEXITED (process_status)
            || (WEXITSTATUS (process_status) != 0))
          {
            g_message ("%s exited abnormally on file '%s'",
                       compressor->save_program,
                       gimp_filename_to_utf8 (tmpname));
            g_free (tmpname);

            return GIMP_PDB_EXECUTION_ERROR;
          }
      }
  }
#else  /* G_OS_WIN32 */
  {
    FILE                *in;
    FILE                *out;
    STARTUPINFO          startupinfo;
    PROCESS_INFORMATION  processinfo;

    in  = g_fopen (tmpname, "rb");
    out = g_fopen (filename, "wb");

    if (in == NULL)
      {
        g_message (_("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (tmpname), g_strerror (errno));
        g_free (tmpname);

        return GIMP_PDB_EXECUTION_ERROR;
      }

    if (out == NULL)
      {
        g_message (_("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
        g_free (tmpname);
        
        return GIMP_PDB_EXECUTION_ERROR;
      }

    startupinfo.cb          = sizeof (STARTUPINFO);
    startupinfo.lpReserved  = NULL;
    startupinfo.lpDesktop   = NULL;
    startupinfo.lpTitle     = NULL;
    startupinfo.dwFlags     = (STARTF_FORCEOFFFEEDBACK |
                               STARTF_USESHOWWINDOW    |
                               STARTF_USESTDHANDLES);
    startupinfo.wShowWindow = SW_SHOWMINNOACTIVE;
    startupinfo.cbReserved2 = 0;
    startupinfo.lpReserved2 = NULL;
    startupinfo.hStdInput   = (HANDLE) _get_osfhandle (fileno (in));
    startupinfo.hStdOutput  = (HANDLE) _get_osfhandle (fileno (out));
    startupinfo.hStdError   = GetStdHandle (STD_ERROR_HANDLE);

    if (! CreateProcess (NULL, compressor->save_program_win32, NULL, NULL,
                         TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL,
                         &startupinfo, &processinfo))
      {
        g_message ("CreateProcess failed: %d", GetLastError ());
        g_free (tmpname);

        return GIMP_PDB_EXECUTION_ERROR;
      }

    CloseHandle (processinfo.hThread);
    WaitForSingleObject (processinfo.hProcess, INFINITE);

    fclose (in);
    fclose (out);
  }
#endif /* G_OS_WIN32 */

  g_unlink (tmpname);
  g_free (tmpname);

  /* ask the core to save a thumbnail for compressed XCF files */
  if (strcmp (ext, ".xcf") == 0)
    gimp_file_save_thumbnail (image_ID, filename);

  return GIMP_PDB_SUCCESS;
}

static gint32
load_image (const Compressor  *compressor,
            const gchar       *filename,
            gint32             run_mode,
            GimpPDBStatusType *status)
{
  gint32       image_ID;
  const gchar *ext;
  gchar       *tmpname;

  ext = find_extension (compressor, filename);

  if (! ext)
    {
      g_message (_("No sensible file extension, "
                   "attempting to load with file magic."));
      ext = ".foo";
    }

  /* find a temp name */
  tmpname = gimp_temp_name (ext + 1);

#ifndef G_OS_WIN32
  {
    gint pid;

    /* fork off a compressor and wait for it */
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

        if (! (f = g_fopen (tmpname, "wb")))
          {
            g_message (_("Could not open '%s' for writing: %s"),
                       gimp_filename_to_utf8 (tmpname), g_strerror (errno));
            g_free (tmpname);
            _exit(127);
          }

        /* make stdout for this child process be the temp file */
        if (dup2 (fileno (f), fileno (stdout)) == -1)
          {
            g_free (tmpname);
            g_message ("dup2() failed: %s", g_strerror (errno));
          }

        /* and uncompress into it */
        execlp (compressor->load_program,
                compressor->load_program,
                compressor->load_options, filename, NULL);

        g_message ("execlp(\"%s %s\") failed: %s",
                   compressor->load_program,
                   compressor->load_options,
                   g_strerror (errno));
        g_free (tmpname);
        _exit(127);
      }
    else  /* parent process */
      {
        gint wpid;
        gint process_status;

        wpid = waitpid (pid, &process_status, 0);

        if ((wpid < 0)
            || !WIFEXITED (process_status)
            || (WEXITSTATUS (process_status) != 0))
          {
            g_message ("%s exited abnormally on file '%s'",
                       compressor->load_program,
                       gimp_filename_to_utf8 (filename));
            g_free (tmpname);

            *status = GIMP_PDB_EXECUTION_ERROR;
            return -1;
          }
      }
  }
#else
  {
    FILE                *in;
    FILE                *out;
    SECURITY_ATTRIBUTES  secattr;
    STARTUPINFO          startupinfo;
    PROCESS_INFORMATION  processinfo;

    in  = g_fopen (filename, "rb");
    out = g_fopen (tmpname, "wb");

    if (in == NULL)
      {
        g_message (_("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
        g_free (tmpname);
       
        *status = GIMP_PDB_EXECUTION_ERROR;
        return -1;
      }

    if (out == NULL)
      {
        g_message (_("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (tmpname), g_strerror (errno));
        g_free (tmpname);

        *status = GIMP_PDB_EXECUTION_ERROR;
        return -1;
      }

    startupinfo.cb          = sizeof (STARTUPINFO);
    startupinfo.lpReserved  = NULL;
    startupinfo.lpDesktop   = NULL;
    startupinfo.lpTitle     = NULL;
    startupinfo.dwFlags     = (STARTF_FORCEOFFFEEDBACK |
                               STARTF_USESHOWWINDOW    |
                               STARTF_USESTDHANDLES);
    startupinfo.wShowWindow = SW_SHOWMINNOACTIVE;
    startupinfo.cbReserved2 = 0;
    startupinfo.lpReserved2 = NULL;
    startupinfo.hStdInput   = (HANDLE) _get_osfhandle (fileno (in));
    startupinfo.hStdOutput  = (HANDLE) _get_osfhandle (fileno (out));
    startupinfo.hStdError   = GetStdHandle (STD_ERROR_HANDLE);

    if (! CreateProcess (NULL, compressor->load_program_win32, NULL, NULL,
                         TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL,
                         &startupinfo, &processinfo))
      {
        g_message ("CreateProcess failed: %d", GetLastError ());
        g_free (tmpname);

        *status = GIMP_PDB_EXECUTION_ERROR;
        return -1;
      }

    CloseHandle (processinfo.hThread);
    WaitForSingleObject (processinfo.hProcess, INFINITE);

    fclose (in);
    fclose (out);
  }
#endif /* G_OS_WIN32 */

  /* now that we uncompressed it, load the temp file */

  image_ID = gimp_file_load (run_mode, tmpname, tmpname);

  g_unlink (tmpname);
  g_free (tmpname);

  if (image_ID != -1)
    {
      *status = GIMP_PDB_SUCCESS;
      gimp_image_set_filename (image_ID, filename);
    }
  else
    {
      *status = GIMP_PDB_EXECUTION_ERROR;
    }

  return image_ID;
}

static gboolean
valid_file (const gchar *filename)
{
  struct stat buf;

  return g_stat (filename, &buf) == 0 && buf.st_size > 0;
}

static const gchar *
find_extension (const Compressor *compressor,
                const gchar      *filename)
{
  gchar *filename_copy;
  gchar *ext;

  /* we never free this copy - aren't we evil! */
  filename_copy = g_strdup (filename);

  /* find the extension, boy! */
  ext = strrchr (filename_copy, '.');

  while (TRUE)
    {
      if (!ext || ext[1] == '\0' || strchr (ext, G_DIR_SEPARATOR))
        {
          return NULL;
        }

      if (0 == g_ascii_strcasecmp (ext, compressor->xcf_extension))
        {
          return ".xcf";  /* we've found it */
        }
      if (0 != g_ascii_strcasecmp (ext, compressor->generic_extension))
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
