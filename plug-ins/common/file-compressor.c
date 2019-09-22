/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Daniel Risacher, magnus@alum.mit.edu
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Minor changes to support file magic */
/* 4 Oct 1997 -- Risacher */

/* compressor plug-in for GIMP           */
/* based on gz.c which in turn is        */
/* loosley based on url.c by             */
/* Josh MacDonald, jmacd@cs.berkeley.edu */

/* and, very loosely on hrz.c by */
/* Albert Cahalan <acahalan at cs.uml.edu> */

/* LZMA compression code is based on code by Lasse Collin which was
 * placed in the public-domain. */

/* This is reads and writes compressed image files for GIMP
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
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>
#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"

#include <zlib.h>
#include <bzlib.h>
#include <lzma.h>


/* Author 1: Josh MacDonald (url.c)          */
/* Author 2: Daniel Risacher (gz.c)          */
/* Author 3: Michael Natterer (compressor.c) */

/* According to USAF Lt Steve Werhle, US DoD software development
 * contracts average about $25 USD per source line of code (SLOC).  By
 * that metric, I figure this plug-in is worth about $10,000 USD */
/* But you got it free.   Magic of Gnu. */

typedef gboolean (*LoadFn) (const char *infile,
                            const char *outfile);
typedef gboolean (*SaveFn) (const char *infile,
                            const char *outfile);

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
  LoadFn       load_fn;

  const gchar *save_proc;
  const gchar *save_blurb;
  const gchar *save_help;
  SaveFn       save_fn;
};


static void                query          (void);
static void                run            (const gchar        *name,
                                           gint                nparams,
                                           const GimpParam    *param,
                                           gint               *nreturn_vals,
                                           GimpParam         **return_vals);

static GimpPDBStatusType   save_image     (const Compressor   *compressor,
                                           const gchar        *filename,
                                           gint32              image_ID,
                                           gint32              drawable_ID,
                                           gint32              run_mode,
                                           GError            **error);
static gint32              load_image     (const Compressor   *compressor,
                                           const gchar        *filename,
                                           gint32              run_mode,
                                           GimpPDBStatusType  *status,
                                           GError            **error);

static gboolean            valid_file     (const gchar        *filename);
static const gchar       * find_extension (const Compressor   *compressor,
                                           const gchar        *filename);

static gboolean            gzip_load      (const char         *infile,
                                           const char         *outfile);
static gboolean            gzip_save      (const char         *infile,
                                           const char         *outfile);

static gboolean            bzip2_load     (const char         *infile,
                                           const char         *outfile);
static gboolean            bzip2_save     (const char         *infile,
                                           const char         *outfile);

static gboolean            xz_load        (const char         *infile,
                                           const char         *outfile);
static gboolean            xz_save        (const char         *infile,
                                           const char         *outfile);
static goffset             get_file_info  (const gchar        *filename);


static const Compressor compressors[] =
{
  {
    N_("gzip archive"),
    "application/x-gzip",
    "xcf.gz,xcfgz", /* FIXME "xcf.gz,gz,xcfgz" */
    "0,string,\037\213",
    ".xcfgz",
    ".gz",

    "file-gz-load",
    "loads files compressed with gzip",
    "This procedure loads files in the gzip compressed format.",
    gzip_load,

    "file-gz-save",
    "saves files compressed with gzip",
    "This procedure saves files in the gzip compressed format.",
    gzip_save
  },

  {
    N_("bzip archive"),
    "application/x-bzip",
    "xcf.bz2,xcfbz2", /* FIXME "xcf.bz2,bz2,xcfbz2" */
    "0,string,BZh",
    ".xcfbz2",
    ".bz2",

    "file-bz2-load",
    "loads files compressed with bzip2",
    "This procedure loads files in the bzip2 compressed format.",
    bzip2_load,

    "file-bz2-save",
    "saves files compressed with bzip2",
    "This procedure saves files in the bzip2 compressed format.",
    bzip2_save
  },

  {
    N_("xz archive"),
    "application/x-xz",
    "xcf.xz,xcfxz", /* FIXME "xcf.xz,xz,xcfxz" */
    "0,string,\3757zXZ\x00",
    ".xcfxz",
    ".xz",

    "file-xz-load",
    "loads files compressed with xz",
    "This procedure loads files in the xz compressed format.",
    xz_load,

    "file-xz-save",
    "saves files compressed with xz",
    "This procedure saves files in the xz compressed format.",
    xz_save
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
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name entered"             }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" },
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
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
  GError            *error  = NULL;
  gint32             image_ID;
  gint               i;

  run_mode = param[0].data.d_int32;

  INIT_I18N();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  /*  We handle PDB errors by forwarding them to the caller in
   *  our return values.
   */
  gimp_plugin_set_pdb_error_handler (GIMP_PDB_ERROR_HANDLER_PLUGIN);

  for (i = 0; i < G_N_ELEMENTS (compressors); i++)
    {
      const Compressor *compressor = &compressors[i];

      if (! strcmp (name, compressor->load_proc))
        {
          image_ID = load_image (compressor,
                                 param[1].data.d_string,
                                 param[0].data.d_int32,
                                 &status, &error);

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
                                 param[0].data.d_int32,
                                 &error);

          break;
        }
    }

  if (i == G_N_ELEMENTS (compressors))
    status = GIMP_PDB_CALLING_ERROR;

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}

static GimpPDBStatusType
save_image (const Compressor  *compressor,
            const gchar       *filename,
            gint32             image_ID,
            gint32             drawable_ID,
            gint32             run_mode,
            GError           **error)
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

      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s", gimp_get_pdb_error ());

      return GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_progress_init_printf (_("Compressing '%s'"),
                             gimp_filename_to_utf8 (filename));

  if (!compressor->save_fn (tmpname, filename))
    {
      g_unlink (tmpname);
      g_free (tmpname);

      return GIMP_PDB_EXECUTION_ERROR;
    }

  g_unlink (tmpname);
  gimp_progress_update (1.0);
  g_free (tmpname);

  /* ask the core to save a thumbnail for compressed XCF files */
  if (strcmp (ext, ".xcf") == 0)
    gimp_file_save_thumbnail (image_ID, filename);

  return GIMP_PDB_SUCCESS;
}

static gint32
load_image (const Compressor   *compressor,
            const gchar        *filename,
            gint32              run_mode,
            GimpPDBStatusType  *status,
            GError            **error)
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

  if (!compressor->load_fn (filename, tmpname))
    {
      g_free (tmpname);
      *status = GIMP_PDB_EXECUTION_ERROR;
      return -1;
    }

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
      /* Forward the return status of the underlining plug-in for the
       * given format.
       */
      *status = gimp_get_pdb_status ();

      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s", gimp_get_pdb_error ());
    }

  return image_ID;
}

static gboolean
valid_file (const gchar *filename)
{
  GStatBuf buf;

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

static gboolean
gzip_load (const char *infile,
           const char *outfile)
{
  gboolean ret;
  int      fd;
  gzFile   in;
  FILE    *out;
  char     buf[16384];
  int      len;

  ret = FALSE;
  in = NULL;
  out = NULL;

  fd = g_open (infile, O_RDONLY | _O_BINARY, 0);
  if (fd == -1)
    goto out;

  in = gzdopen (fd, "rb");
  if (!in)
    {
      close (fd);
      goto out;
    }

  out = g_fopen (outfile, "wb");
  if (!out)
    goto out;

  while (TRUE)
    {
      len = gzread (in, buf, sizeof buf);

      if (len < 0)
        break;
      else if (len == 0)
        {
          ret = TRUE;
          break;
        }

      if (fwrite(buf, 1, len, out) != len)
        break;
    }

 out:
  /* There is no need to close(fd) as it is closed by gzclose(). */
  if (in)
    if (gzclose (in) != Z_OK)
      ret = FALSE;

  if (out)
    fclose (out);

  return ret;
}

static gboolean
gzip_save (const char *infile,
           const char *outfile)
{
  gboolean  ret;
  FILE     *in;
  int       fd;
  gzFile    out;
  char      buf[16384];
  int       len;
  goffset   tot = 0, file_size;

  ret = FALSE;
  in = NULL;
  out = NULL;

  in = g_fopen (infile, "rb");
  if (!in)
    goto out;

  fd = g_open (outfile, O_CREAT | O_WRONLY | O_TRUNC | _O_BINARY, 0664);
  if (fd == -1)
    goto out;

  out = gzdopen (fd, "wb");
  if (!out)
    {
      close (fd);
      goto out;
    }

  file_size = get_file_info (infile);
  while (TRUE)
    {
      len = fread (buf, 1, sizeof buf, in);
      if (ferror (in))
        break;

      if (len < 0)
        break;
      else if (len == 0)
        {
          ret = TRUE;
          break;
        }

      if (gzwrite (out, buf, len) != len)
        break;

      gimp_progress_update ((tot += len) * 1.0 / file_size);
    }

 out:
  if (in)
    fclose (in);

  /* There is no need to close(fd) as it is closed by gzclose(). */
  if (out)
    if (gzclose (out) != Z_OK)
      ret = FALSE;

  return ret;
}

static gboolean
bzip2_load (const char *infile,
            const char *outfile)
{
  gboolean  ret;
  int       fd;
  BZFILE   *in;
  FILE     *out;
  char      buf[16384];
  int       len;

  ret = FALSE;
  in = NULL;
  out = NULL;

  fd = g_open (infile, O_RDONLY | _O_BINARY, 0);
  if (fd == -1)
    goto out;

  in = BZ2_bzdopen (fd, "rb");
  if (!in)
    {
      close (fd);
      goto out;
    }

  out = g_fopen (outfile, "wb");
  if (!out)
    goto out;

  while (TRUE)
    {
      len = BZ2_bzread (in, buf, sizeof buf);

      if (len < 0)
        break;
      else if (len == 0)
        {
          ret = TRUE;
          break;
        }

      if (fwrite(buf, 1, len, out) != len)
        break;
    }

 out:
  /* TODO: Check this in the case of BZ2_bzclose(): */
  /* There is no need to close(fd) as it is closed by BZ2_bzclose(). */
  if (in)
    BZ2_bzclose (in);

  if (out)
    fclose (out);

  return ret;
}

static gboolean
bzip2_save (const char *infile,
            const char *outfile)
{
  gboolean  ret;
  FILE     *in;
  int       fd;
  BZFILE   *out;
  char      buf[16384];
  int       len;
  goffset   tot = 0, file_size;

  ret = FALSE;
  in = NULL;
  out = NULL;

  in = g_fopen (infile, "rb");
  if (!in)
    goto out;

  fd = g_open (outfile, O_CREAT | O_WRONLY | O_TRUNC | _O_BINARY, 0664);
  if (fd == -1)
    goto out;

  out = BZ2_bzdopen (fd, "wb");
  if (!out)
    {
      close (fd);
      goto out;
    }

  file_size = get_file_info (infile);
  while (TRUE)
    {
      len = fread (buf, 1, sizeof buf, in);
      if (ferror (in))
        break;

      if (len < 0)
        break;
      else if (len == 0)
        {
          ret = TRUE;
          break;
        }

      if (BZ2_bzwrite (out, buf, len) != len)
        break;

      gimp_progress_update ((tot += len) * 1.0 / file_size);
    }

 out:
  if (in)
    fclose (in);

  /* TODO: Check this in the case of BZ2_bzclose(): */
  /* There is no need to close(fd) as it is closed by BZ2_bzclose(). */
  if (out)
    BZ2_bzclose (out);

  return ret;
}

static gboolean
xz_load (const char *infile,
         const char *outfile)
{
  gboolean     ret;
  FILE        *in;
  FILE        *out;
  lzma_stream  strm = LZMA_STREAM_INIT;
  lzma_action  action;
  guint8       inbuf[BUFSIZ];
  guint8       outbuf[BUFSIZ];
  lzma_ret     status;

  ret = FALSE;
  in = NULL;
  out = NULL;

  in = g_fopen (infile, "rb");
  if (!in)
    goto out;

  out = g_fopen (outfile, "wb");
  if (!out)
    goto out;

  if (lzma_stream_decoder (&strm, UINT64_MAX, 0) != LZMA_OK)
    goto out;

  strm.next_in = NULL;
  strm.avail_in = 0;
  strm.next_out = outbuf;
  strm.avail_out = sizeof outbuf;

  action = LZMA_RUN;
  status = LZMA_OK;

  while (status == LZMA_OK)
    {
      /* Fill the input buffer if it is empty. */
      if ((strm.avail_in == 0) && (!feof(in)))
        {
          strm.next_in = inbuf;
          strm.avail_in = fread (inbuf, 1, sizeof inbuf, in);

          if (ferror (in))
            goto out;

          /* Once the end of the input file has been reached, we need to
             tell lzma_code() that no more input will be coming and that
             it should finish the encoding. */
          if (feof (in))
            action = LZMA_FINISH;
        }

      status = lzma_code (&strm, action);

      if ((strm.avail_out == 0) || (status == LZMA_STREAM_END))
        {
          /* When lzma_code() has returned LZMA_STREAM_END, the output
             buffer is likely to be only partially full. Calculate how
             much new data there is to be written to the output file. */
          size_t write_size = sizeof outbuf - strm.avail_out;

          if (fwrite (outbuf, 1, write_size, out) != write_size)
            goto out;

          /* Reset next_out and avail_out. */
          strm.next_out = outbuf;
          strm.avail_out = sizeof outbuf;
        }
    }

  if (status != LZMA_STREAM_END)
    goto out;

  lzma_end (&strm);
  ret = TRUE;

 out:
  if (in)
    fclose (in);

  if (out)
    fclose (out);

  return ret;
}

static gboolean
xz_save (const char *infile,
         const char *outfile)
{
  gboolean     ret;
  FILE        *in;
  FILE        *out;
  lzma_stream  strm = LZMA_STREAM_INIT;
  lzma_action  action;
  guint8       inbuf[BUFSIZ];
  guint8       outbuf[BUFSIZ];
  lzma_ret     status;
  goffset      tot = 0, file_size;

  ret = FALSE;
  in = NULL;
  out = NULL;

  in = g_fopen (infile, "rb");
  if (!in)
    goto out;

  file_size = get_file_info (infile);
  out = g_fopen (outfile, "wb");
  if (!out)
    goto out;

  if (lzma_easy_encoder (&strm,
                         LZMA_PRESET_DEFAULT,
                         LZMA_CHECK_CRC64) != LZMA_OK)
    goto out;

  strm.next_in = NULL;
  strm.avail_in = 0;
  strm.next_out = outbuf;
  strm.avail_out = sizeof outbuf;

  action = LZMA_RUN;
  status = LZMA_OK;

  while (status == LZMA_OK)
    {
      /* Fill the input buffer if it is empty. */
      if ((strm.avail_in == 0) && (!feof(in)))
        {
          strm.next_in = inbuf;
          strm.avail_in = fread (inbuf, 1, sizeof inbuf, in);

          if (ferror (in))
            goto out;

          /* Once the end of the input file has been reached, we need to
             tell lzma_code() that no more input will be coming and that
             it should finish the encoding. */
          if (feof (in))
            action = LZMA_FINISH;

          gimp_progress_update ((tot += strm.avail_in) * 1.0 / file_size);
        }

      status = lzma_code (&strm, action);

      if ((strm.avail_out == 0) || (status == LZMA_STREAM_END))
        {
          /* When lzma_code() has returned LZMA_STREAM_END, the output
             buffer is likely to be only partially full. Calculate how
             much new data there is to be written to the output file. */
          size_t write_size = sizeof outbuf - strm.avail_out;

          if (fwrite (outbuf, 1, write_size, out) != write_size)
            goto out;

          /* Reset next_out and avail_out. */
          strm.next_out = outbuf;
          strm.avail_out = sizeof outbuf;
        }
    }

  if (status != LZMA_STREAM_END)
    goto out;

  lzma_end (&strm);
  ret = TRUE;

 out:
  if (in)
    fclose (in);

  if (out)
    fclose (out);

  return ret;
}

/* get file size from a filename */
static goffset
get_file_info (const gchar *filename)
{
  GFile     *file = g_file_new_for_path (filename);
  GFileInfo *info;
  goffset    size = 1;

  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_STANDARD_SIZE,
                            G_FILE_QUERY_INFO_NONE,
                            NULL, NULL);

  if (info)
    {
      size = g_file_info_get_size (info);

      g_object_unref (info);
    }

  g_object_unref (file);

  return size;
}
