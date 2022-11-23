/* LIGMA - The GNU Image Manipulation Program
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

/* compressor plug-in for LIGMA           */
/* based on gz.c which in turn is        */
/* loosely based on url.c by             */
/* Josh MacDonald, jmacd@cs.berkeley.edu */

/* and, very loosely on hrz.c by */
/* Albert Cahalan <acahalan at cs.uml.edu> */

/* LZMA compression code is based on code by Lasse Collin which was
 * placed in the public-domain. */

/* This is reads and writes compressed image files for LIGMA
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
 * user to type sometimes when they otherwise wouldn't need to.
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

#include <libligma/ligma.h>

#include "libligma/stdplugins-intl.h"

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

typedef gboolean (* LoadFn) (GFile *infile,
                             GFile *outfile);
typedef gboolean (* SaveFn) (GFile *infile,
                             GFile *outfile);

typedef struct _CompressorEntry CompressorEntry;

struct _CompressorEntry
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


typedef struct _Compressor      Compressor;
typedef struct _CompressorClass CompressorClass;

struct _Compressor
{
  LigmaPlugIn      parent_instance;
};

struct _CompressorClass
{
  LigmaPlugInClass parent_class;
};


#define COMPRESSOR_TYPE  (compressor_get_type ())
#define COMPRESSOR (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), COMPRESSOR_TYPE, Compressor))

GType                   compressor_get_type         (void) G_GNUC_CONST;

static GList          * compressor_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * compressor_create_procedure (LigmaPlugIn           *plug_in,
                                                     const gchar          *name);

static LigmaValueArray * compressor_save             (LigmaProcedure        *procedure,
                                                     LigmaRunMode           run_mode,
                                                     LigmaImage            *image,
                                                     gint                  n_drawables,
                                                     LigmaDrawable        **drawables,
                                                     GFile                *file,
                                                     const LigmaValueArray *args,
                                                     gpointer              run_data);
static LigmaValueArray * compressor_load             (LigmaProcedure        *procedure,
                                                     LigmaRunMode           run_mode,
                                                     GFile                *file,
                                                     const LigmaValueArray *args,
                                                     gpointer              run_data);

static LigmaImage         * load_image     (const CompressorEntry *compressor,
                                           GFile                 *file,
                                           gint32                 run_mode,
                                           LigmaPDBStatusType     *status,
                                           GError               **error);
static LigmaPDBStatusType   save_image     (const CompressorEntry *compressor,
                                           GFile                 *file,
                                           LigmaImage             *image,
                                           gint                   n_drawables,
                                           LigmaDrawable         **drawables,
                                           gint32                 run_mode,
                                           GError               **error);

static gboolean            valid_file     (GFile                 *file);
static const gchar       * find_extension (const CompressorEntry *compressor,
                                           const gchar           *filename);

static gboolean            gzip_load      (GFile                 *infile,
                                           GFile                 *outfile);
static gboolean            gzip_save      (GFile                 *infile,
                                           GFile                 *outfile);

static gboolean            bzip2_load     (GFile                 *infile,
                                           GFile                 *outfile);
static gboolean            bzip2_save     (GFile                 *infile,
                                           GFile                 *outfile);

static gboolean            xz_load        (GFile                 *infile,
                                           GFile                 *outfile);
static gboolean            xz_save        (GFile                 *infile,
                                           GFile                 *outfile);
static goffset             get_file_info  (GFile                 *file);


G_DEFINE_TYPE (Compressor, compressor, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (COMPRESSOR_TYPE)
DEFINE_STD_SET_I18N


static const CompressorEntry compressors[] =
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


static void
compressor_class_init (CompressorClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = compressor_query_procedures;
  plug_in_class->create_procedure = compressor_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
compressor_init (Compressor *compressor)
{
}

static GList *
compressor_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;
  gint   i;

  for (i = 0; i < G_N_ELEMENTS (compressors); i++)
    {
      const CompressorEntry *compressor = &compressors[i];

      list = g_list_append (list, g_strdup (compressor->load_proc));
      list = g_list_append (list, g_strdup (compressor->save_proc));
    }

  return list;
}

static LigmaProcedure *
compressor_create_procedure (LigmaPlugIn  *plug_in,
                             const gchar *name)
{
  LigmaProcedure *procedure = NULL;
  gint           i;

  for (i = 0; i < G_N_ELEMENTS (compressors); i++)
    {
      const CompressorEntry *compressor = &compressors[i];

      if (! strcmp (name, compressor->load_proc))
        {
          procedure = ligma_load_procedure_new (plug_in, name,
                                               LIGMA_PDB_PROC_TYPE_PLUGIN,
                                               compressor_load,
                                               (gpointer) compressor, NULL);

          ligma_procedure_set_documentation (procedure,
                                            compressor->load_blurb,
                                            compressor->load_help,
                                            name);

          ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                          compressor->magic);
        }
      else if (! strcmp (name, compressor->save_proc))
        {
          procedure = ligma_save_procedure_new (plug_in, name,
                                               LIGMA_PDB_PROC_TYPE_PLUGIN,
                                               compressor_save,
                                               (gpointer) compressor, NULL);

          ligma_procedure_set_image_types (procedure, "RGB*, GRAY*, INDEXED*");

          ligma_procedure_set_documentation (procedure,
                                            compressor->save_blurb,
                                            compressor->save_help,
                                            name);
        }

      if (procedure)
        {
          ligma_procedure_set_menu_label (procedure, compressor->file_type);

          ligma_procedure_set_attribution (procedure,
                                          "Daniel Risacher",
                                          "Daniel Risacher, Spencer Kimball "
                                          "and Peter Mattis",
                                          "1995-1997");

          ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                              compressor->mime_type);
          ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                              compressor->extensions);

          return procedure;
        }
    }

  return NULL;
}

static LigmaValueArray *
compressor_load (LigmaProcedure        *procedure,
                 LigmaRunMode           run_mode,
                 GFile                *file,
                 const LigmaValueArray *args,
                 gpointer              run_data)
{
  const CompressorEntry *compressor = run_data;
  LigmaValueArray        *return_vals;
  LigmaPDBStatusType      status;
  LigmaImage             *image;
  GError                *error = NULL;

  /*  We handle PDB errors by forwarding them to the caller in
   *  our return values.
   */
  ligma_plug_in_set_pdb_error_handler (ligma_procedure_get_plug_in (procedure),
                                      LIGMA_PDB_ERROR_HANDLER_PLUGIN);

  image = load_image (compressor, file, run_mode,
                      &status, &error);

  return_vals = ligma_procedure_new_return_values (procedure, status, error);

  if (image && status == LIGMA_PDB_SUCCESS)
    LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
compressor_save (LigmaProcedure        *procedure,
                 LigmaRunMode           run_mode,
                 LigmaImage            *image,
                 gint                  n_drawables,
                 LigmaDrawable        **drawables,
                 GFile                *file,
                 const LigmaValueArray *args,
                 gpointer              run_data)
{
  const CompressorEntry *compressor = run_data;
  LigmaPDBStatusType      status;
  GError                *error = NULL;

  /*  We handle PDB errors by forwarding them to the caller in
   *  our return values.
   */
  ligma_plug_in_set_pdb_error_handler (ligma_procedure_get_plug_in (procedure),
                                      LIGMA_PDB_ERROR_HANDLER_PLUGIN);

  status = save_image (compressor, file, image, n_drawables, drawables,
                       run_mode, &error);

  return ligma_procedure_new_return_values (procedure, status, error);
}

static LigmaPDBStatusType
save_image (const CompressorEntry  *compressor,
            GFile                  *file,
            LigmaImage              *image,
            gint                    n_drawables,
            LigmaDrawable          **drawables,
            gint32                  run_mode,
            GError                **error)
{
  const gchar *ext;
  GFile       *tmp_file;

  ext = find_extension (compressor, g_file_peek_path (file));

  if (! ext)
    {
      g_message (_("No sensible file extension, saving as compressed XCF."));
      ext = ".xcf";
    }

  /* get a temp name with the right extension and save into it. */

  tmp_file = ligma_temp_file (ext + 1);

  if (! (ligma_file_save (run_mode,
                         image,
                         n_drawables,
                         (const LigmaItem **) drawables,
                         tmp_file) &&
         valid_file (tmp_file)))
    {
      g_file_delete (tmp_file, NULL, NULL);
      g_object_unref (tmp_file);

      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s", ligma_pdb_get_last_error (ligma_get_pdb ()));

      return LIGMA_PDB_EXECUTION_ERROR;
    }

  ligma_progress_init_printf (_("Compressing '%s'"),
                             ligma_file_get_utf8_name  (file));

  if (! compressor->save_fn (tmp_file, file))
    {
      g_file_delete (tmp_file, NULL, NULL);
      g_object_unref (tmp_file);

      return LIGMA_PDB_EXECUTION_ERROR;
    }

  g_file_delete (tmp_file, NULL, NULL);
  g_object_unref (tmp_file);

  ligma_progress_update (1.0);

  /* ask the core to save a thumbnail for compressed XCF files */
  if (strcmp (ext, ".xcf") == 0)
    ligma_file_save_thumbnail (image, file);

  return LIGMA_PDB_SUCCESS;
}

static LigmaImage *
load_image (const CompressorEntry  *compressor,
            GFile                  *file,
            gint32                  run_mode,
            LigmaPDBStatusType      *status,
            GError                **error)
{
  LigmaImage   *image;
  const gchar *ext;
  GFile       *tmp_file;

  ext = find_extension (compressor, g_file_peek_path (file));

  if (! ext)
    {
      g_message (_("No sensible file extension, "
                   "attempting to load with file magic."));
      ext = ".foo";
    }

  /* find a temp name */
  tmp_file = ligma_temp_file (ext + 1);

  if (! compressor->load_fn (file, tmp_file))
    {
      g_object_unref (tmp_file);
      *status = LIGMA_PDB_EXECUTION_ERROR;
      return NULL;
    }

  /* now that we uncompressed it, load the temp file */

  image = ligma_file_load (run_mode, tmp_file);

  g_file_delete (tmp_file, NULL, NULL);
  g_object_unref (tmp_file);

  if (image)
    {
      *status = LIGMA_PDB_SUCCESS;

      ligma_image_set_file (image, file);
    }
  else
    {
      /* Forward the return status of the underlining plug-in for the
       * given format.
       */
      *status = ligma_pdb_get_last_status (ligma_get_pdb ());

      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s", ligma_pdb_get_last_error (ligma_get_pdb ()));
    }

  return image;
}

static gboolean
valid_file (GFile *file)
{
  GStatBuf  buf;
  gboolean  valid;

  valid = g_stat (g_file_peek_path (file), &buf) == 0 && buf.st_size > 0;

  return valid;
}

static const gchar *
find_extension (const CompressorEntry *compressor,
                const gchar           *filename)
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
gzip_load (GFile *infile,
           GFile *outfile)
{
  gboolean  ret;
  int       fd;
  gzFile    in;
  FILE     *out;
  char      buf[16384];
  int       len;

  ret = FALSE;
  in = NULL;
  out = NULL;

  fd = g_open (g_file_peek_path (infile), O_RDONLY | _O_BINARY, 0);
  if (fd == -1)
    goto out;

  in = gzdopen (fd, "rb");
  if (! in)
    {
      close (fd);
      goto out;
    }

  out = g_fopen (g_file_peek_path (outfile), "wb");
  if (! out)
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
gzip_save (GFile *infile,
           GFile *outfile)
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

  in = g_fopen (g_file_peek_path (infile), "rb");
  if (! in)
    goto out;

  fd = g_open (g_file_peek_path (outfile), O_CREAT | O_WRONLY | O_TRUNC | _O_BINARY, 0664);
  if (fd == -1)
    goto out;

  out = gzdopen (fd, "wb");
  if (! out)
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

      ligma_progress_update ((tot += len) * 1.0 / file_size);
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
bzip2_load (GFile *infile,
           GFile  *outfile)
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

  fd = g_open (g_file_peek_path (infile), O_RDONLY | _O_BINARY, 0);
  if (fd == -1)
    goto out;

  in = BZ2_bzdopen (fd, "rb");
  if (!in)
    {
      close (fd);
      goto out;
    }

  out = g_fopen (g_file_peek_path (outfile), "wb");
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
bzip2_save (GFile *infile,
            GFile *outfile)
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

  in = g_fopen (g_file_peek_path (infile), "rb");
  if (!in)
    goto out;

  fd = g_open (g_file_peek_path (outfile), O_CREAT | O_WRONLY | O_TRUNC | _O_BINARY, 0664);
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

      ligma_progress_update ((tot += len) * 1.0 / file_size);
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
xz_load (GFile *infile,
         GFile *outfile)
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

  in = g_fopen (g_file_peek_path (infile), "rb");
  if (!in)
    goto out;

  out = g_fopen (g_file_peek_path (outfile), "wb");
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
xz_save (GFile *infile,
         GFile *outfile)
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

  in = g_fopen (g_file_peek_path (infile), "rb");
  if (!in)
    goto out;

  file_size = get_file_info (infile);
  out = g_fopen (g_file_peek_path (outfile), "wb");
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

          ligma_progress_update ((tot += strm.avail_in) * 1.0 / file_size);
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
get_file_info (GFile *file)
{
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

  return size;
}
