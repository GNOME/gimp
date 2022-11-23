/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * PNM reading and writing code Copyright (C) 1996 Erik Nygren
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

/*
 * The pnm reading and writing code was written from scratch by Erik Nygren
 * (nygren@mit.edu) based on the specifications in the man pages and
 * does not contain any code from the netpbm or pbmplus distributions.
 *
 * 2006: pbm saving written by Martin K Collins (martin@mkcollins.org)
 * 2015: pfm reading written by Tobias Ellinghaus (me@houz.org)
 */

#include "config.h"

#include <setjmp.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define LOAD_PROC      "file-pnm-load"
#define PNM_SAVE_PROC  "file-pnm-save"
#define PBM_SAVE_PROC  "file-pbm-save"
#define PGM_SAVE_PROC  "file-pgm-save"
#define PPM_SAVE_PROC  "file-ppm-save"
#define PFM_SAVE_PROC  "file-pfm-save"
#define PLUG_IN_BINARY "file-pnm"
#define PLUG_IN_ROLE   "ligma-file-pnm"


/* Declare local data types
 */

typedef enum
{
  FILE_TYPE_PNM,
  FILE_TYPE_PBM,
  FILE_TYPE_PGM,
  FILE_TYPE_PPM,
  FILE_TYPE_PFM
} FileType;

typedef struct _PNMScanner PNMScanner;
typedef struct _PNMInfo    PNMInfo;
typedef struct _PNMRowInfo PNMRowInfo;

typedef void     (* PNMLoaderFunc)  (PNMScanner    *scanner,
                                     PNMInfo       *info,
                                     GeglBuffer    *buffer);
typedef gboolean (* PNMSaverowFunc) (PNMRowInfo    *info,
                                     guchar        *data,
                                     GError       **error);

struct _PNMScanner
{
  GInputStream *input;          /* The input stream of the file being read */
  gchar         cur;            /* The current character in the input stream */
  gint          eof;            /* Have we reached end of file? */
  gchar        *inbuf;          /* Input buffer - initially 0 */
  gint          inbufsize;      /* Size of input buffer */
  gint          inbufvalidsize; /* Size of input buffer with valid data */
  gint          inbufpos;       /* Position in input buffer */
};

struct _PNMInfo
{
  gint          xres, yres;     /* The size of the image */
  gboolean      float_format;   /* Whether it is a floating point format */
  gint          maxval;         /* For integer format image files, the max value
                                 * which we need to normalize to */
  gfloat        scale_factor;   /* PFM files have a scale factor */
  gint          np;             /* Number of image planes (0 for pbm) */
  gboolean      asciibody;      /* 1 if ascii body, 0 if raw body */
  jmp_buf       jmpbuf;         /* Where to jump to on an error loading */

  PNMLoaderFunc loader;         /* Routine to use to load the pnm body */
};

/* Contains the information needed to write out PNM rows */
struct _PNMRowInfo
{
  GOutputStream *output;        /* Output stream               */
  gchar         *rowbuf;        /* Buffer for writing out rows */
  gint           xres;          /* X resolution                */
  gint           np;            /* Number of planes            */
  gint           bpc;           /* Bytes per color             */
  guchar        *red;           /* Colormap red                */
  guchar        *grn;           /* Colormap green              */
  guchar        *blu;           /* Colormap blue               */
  gboolean       zero_is_black; /* index zero is black (PBM only) */
};

#define BUFLEN 512              /* The input buffer size for data returned
                                 * from the scanner.  Note that lines
                                 * aren't allowed to be over 256 characters
                                 * by the spec anyways so this shouldn't
                                 * be an issue. */


typedef struct _Pnm      Pnm;
typedef struct _PnmClass PnmClass;

struct _Pnm
{
  LigmaPlugIn      parent_instance;
};

struct _PnmClass
{
  LigmaPlugInClass parent_class;
};


#define PNM_TYPE  (pnm_get_type ())
#define PNM (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PNM_TYPE, Pnm))

GType                   pnm_get_type         (void) G_GNUC_CONST;

static GList          * pnm_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * pnm_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * pnm_load             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * pnm_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static LigmaImage      * load_image           (GFile                *file,
                                              GError              **error);
static gint             save_image           (GFile                *file,
                                              LigmaImage            *image,
                                              LigmaDrawable         *drawable,
                                              FileType              file_type,
                                              GObject              *config,
                                              GError              **error);

static gboolean         save_dialog          (LigmaProcedure        *procedure,
                                              GObject              *config);

static void             pnm_load_ascii       (PNMScanner           *scan,
                                              PNMInfo              *info,
                                              GeglBuffer           *buffer);
static void             pnm_load_raw         (PNMScanner           *scan,
                                              PNMInfo              *info,
                                              GeglBuffer           *buffer);
static void             pnm_load_rawpbm      (PNMScanner           *scan,
                                              PNMInfo              *info,
                                              GeglBuffer           *buffer);
static void             pnm_load_rawpfm      (PNMScanner           *scan,
                                              PNMInfo              *info,
                                              GeglBuffer           *buffer);

static gboolean     pnmsaverow_ascii         (PNMRowInfo           *ri,
                                              guchar               *data,
                                              GError              **error);
static gboolean     pnmsaverow_raw           (PNMRowInfo           *ri,
                                              guchar               *data,
                                              GError              **error);
static gboolean     pnmsaverow_raw_pbm       (PNMRowInfo           *ri,
                                              guchar               *data,
                                              GError              **error);
static gboolean     pnmsaverow_ascii_pbm     (PNMRowInfo           *ri,
                                              guchar               *data,
                                              GError              **error);
static gboolean     pnmsaverow_ascii_indexed (PNMRowInfo           *ri,
                                              guchar               *data,
                                              GError              **error);
static gboolean     pnmsaverow_raw_indexed   (PNMRowInfo           *ri,
                                              guchar               *data,
                                              GError              **error);

static PNMScanner * pnmscanner_create        (GInputStream         *input);
static void         pnmscanner_destroy       (PNMScanner           *s);
static void         pnmscanner_createbuffer  (PNMScanner           *s,
                                              gint                  bufsize);
static void         pnmscanner_getchar       (PNMScanner           *s);
static void         pnmscanner_eatwhitespace (PNMScanner           *s);
static void         pnmscanner_gettoken      (PNMScanner           *s,
                                              gchar                *buf,
                                              gint                  bufsize);
static void         pnmscanner_getsmalltoken (PNMScanner           *s,
                                              gchar                *buf);

#define pnmscanner_eof(s)   ((s)->eof)
#define pnmscanner_input(s) ((s)->input)

/* Checks for a fatal error */
#define CHECK_FOR_ERROR(predicate, jmpbuf, ...) \
        if ((predicate)) \
          { g_message (__VA_ARGS__); longjmp ((jmpbuf), 1); }


G_DEFINE_TYPE (Pnm, pnm, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (PNM_TYPE)
DEFINE_STD_SET_I18N


static const struct
{
  gchar         name;
  gint          np;
  gint          asciibody;
  gint          maxval;
  PNMLoaderFunc loader;
} pnm_types[] =
{
  { '1', 0, 1,   1, pnm_load_ascii  },  /* ASCII PBM             */
  { '2', 1, 1, 255, pnm_load_ascii  },  /* ASCII PGM             */
  { '3', 3, 1, 255, pnm_load_ascii  },  /* ASCII PPM             */
  { '4', 0, 0,   1, pnm_load_rawpbm },  /* RAW   PBM             */
  { '5', 1, 0, 255, pnm_load_raw    },  /* RAW   PGM             */
  { '6', 3, 0, 255, pnm_load_raw    },  /* RAW   PPM             */
  { 'F', 3, 0,   0, pnm_load_rawpfm },  /* RAW   PFM (color)     */
  { 'f', 1, 0,   0, pnm_load_rawpfm },  /* RAW   PFM (grayscale) */
  {  0 , 0, 0,   0, NULL}
};


static void
pnm_class_init (PnmClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pnm_query_procedures;
  plug_in_class->create_procedure = pnm_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
pnm_init (Pnm *pnm)
{
}

static GList *
pnm_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (PNM_SAVE_PROC));
  list = g_list_append (list, g_strdup (PBM_SAVE_PROC));
  list = g_list_append (list, g_strdup (PGM_SAVE_PROC));
  list = g_list_append (list, g_strdup (PPM_SAVE_PROC));
  list = g_list_append (list, g_strdup (PFM_SAVE_PROC));

  return list;
}

static LigmaProcedure *
pnm_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           pnm_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("PNM Image"));

      ligma_procedure_set_documentation (procedure,
                                        "Loads files in the PNM file format",
                                        "This plug-in loads files in the "
                                        "various Netpbm portable file formats.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Erik Nygren",
                                      "Erik Nygren",
                                      "1996");

      ligma_file_procedure_set_handles_remote (LIGMA_FILE_PROCEDURE (procedure),
                                              TRUE);
      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-portable-anymap");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "pnm,ppm,pgm,pbm,pfm");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0,string,P1,0,string,P2,0,string,P3,"
                                      "0,string,P4,0,string,P5,0,string,P6,"
                                      "0,string,PF,0,string,Pf");
    }
  else if (! strcmp (name, PNM_SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           pnm_save,
                                           GINT_TO_POINTER (FILE_TYPE_PNM),
                                           NULL);

      ligma_procedure_set_image_types (procedure, "RGB, GRAY, INDEXED");

      ligma_procedure_set_menu_label (procedure, _("PNM image"));

      ligma_procedure_set_documentation (procedure,
                                        "Exports files in the PNM file format",
                                        "PNM export handles all image types "
                                        "without transparency.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Erik Nygren",
                                      "Erik Nygren",
                                      "1996");

      ligma_file_procedure_set_handles_remote (LIGMA_FILE_PROCEDURE (procedure),
                                              TRUE);
      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-portable-anymap");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "pnm");

      LIGMA_PROC_ARG_BOOLEAN (procedure, "raw",
                             "Raw",
                             "TRUE for raw output, FALSE for ascii output",
                             TRUE,
                             G_PARAM_READWRITE);
    }
  else if (! strcmp (name, PBM_SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           pnm_save,
                                           GINT_TO_POINTER (FILE_TYPE_PBM),
                                           NULL);

      ligma_procedure_set_image_types (procedure, "RGB, GRAY, INDEXED");

      ligma_procedure_set_menu_label (procedure, _("PBM image"));

      ligma_procedure_set_documentation (procedure,
                                        "Exports files in the PBM file format",
                                        "PBM exporting produces mono images "
                                        "without transparency.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Erik Nygren",
                                      "Erik Nygren",
                                      "1996");

      ligma_file_procedure_set_handles_remote (LIGMA_FILE_PROCEDURE (procedure),
                                              TRUE);
      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-portable-bitmap");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "pbm");

      LIGMA_PROC_ARG_BOOLEAN (procedure, "raw",
                             "Raw",
                             "TRUE for raw output, FALSE for ascii output",
                             TRUE,
                             G_PARAM_READWRITE);
    }
  else if (! strcmp (name, PGM_SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           pnm_save,
                                           GINT_TO_POINTER (FILE_TYPE_PGM),
                                           NULL);

      ligma_procedure_set_image_types (procedure, "RGB, GRAY, INDEXED");

      ligma_procedure_set_menu_label (procedure, _("PGM image"));

      ligma_procedure_set_documentation (procedure,
                                        "Exports files in the PGM file format",
                                        "PGM exporting produces grayscale "
                                        "images without transparency.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Erik Nygren",
                                      "Erik Nygren",
                                      "1996");

      ligma_file_procedure_set_handles_remote (LIGMA_FILE_PROCEDURE (procedure),
                                              TRUE);
      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-portable-graymap");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "pgm");

      LIGMA_PROC_ARG_BOOLEAN (procedure, "raw",
                             "Raw",
                             "TRUE for raw output, FALSE for ascii output",
                             TRUE,
                             G_PARAM_READWRITE);
    }
  else if (! strcmp (name, PPM_SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           pnm_save,
                                           GINT_TO_POINTER (FILE_TYPE_PPM),
                                           NULL);

      ligma_procedure_set_image_types (procedure, "RGB, GRAY, INDEXED");

      ligma_procedure_set_menu_label (procedure, _("PPM image"));

      ligma_procedure_set_documentation (procedure,
                                        "Exports files in the PPM file format",
                                        "PPM export handles RGB images "
                                        "without transparency.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Erik Nygren",
                                      "Erik Nygren",
                                      "1996");

      ligma_file_procedure_set_handles_remote (LIGMA_FILE_PROCEDURE (procedure),
                                              TRUE);
      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-portable-pixmap");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "ppm");

      LIGMA_PROC_ARG_BOOLEAN (procedure, "raw",
                             "Raw",
                             "TRUE for raw output, FALSE for ascii output",
                             TRUE,
                             G_PARAM_READWRITE);
    }
  else if (! strcmp (name, PFM_SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           pnm_save,
                                           GINT_TO_POINTER (FILE_TYPE_PFM),
                                           NULL);

      ligma_procedure_set_image_types (procedure, "RGB, GRAY, INDEXED");

      ligma_procedure_set_menu_label (procedure, _("PFM image"));

      ligma_procedure_set_documentation (procedure,
                                        "Exports files in the PFM file format",
                                        "PFM export handles all images "
                                        "without transparency.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Mukund Sivaraman",
                                      "Mukund Sivaraman",
                                      "2015");

      ligma_file_procedure_set_handles_remote (LIGMA_FILE_PROCEDURE (procedure),
                                              TRUE);
      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-portable-floatmap");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "pfm");
    }

  return procedure;
}

static LigmaValueArray *
pnm_load (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
pnm_save (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaProcedureConfig *config;
  FileType             file_type   = GPOINTER_TO_INT (run_data);
  LigmaPDBStatusType    status      = LIGMA_PDB_SUCCESS;
  LigmaExportReturn     export      = LIGMA_EXPORT_CANCEL;
  const gchar         *format_name = NULL;
  GError              *error       = NULL;

  gegl_init (NULL, NULL);

  config = ligma_procedure_create_config (procedure);
  ligma_procedure_config_begin_export (config, image, run_mode, args, NULL);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      switch (file_type)
        {
        case FILE_TYPE_PNM:
          format_name = "PNM";
          export = ligma_export_image (&image, &n_drawables, &drawables, "PNM",
                                      LIGMA_EXPORT_CAN_HANDLE_RGB  |
                                      LIGMA_EXPORT_CAN_HANDLE_GRAY |
                                      LIGMA_EXPORT_CAN_HANDLE_INDEXED);
          break;

        case FILE_TYPE_PBM:
          format_name = "PBM";
          export = ligma_export_image (&image, &n_drawables, &drawables, "PBM",
                                      LIGMA_EXPORT_CAN_HANDLE_BITMAP);
          break;

        case FILE_TYPE_PGM:
          format_name = "PGM";
          export = ligma_export_image (&image, &n_drawables, &drawables, "PGM",
                                      LIGMA_EXPORT_CAN_HANDLE_GRAY);
          break;

        case FILE_TYPE_PPM:
          format_name = "PPM";
          export = ligma_export_image (&image, &n_drawables, &drawables, "PPM",
                                      LIGMA_EXPORT_CAN_HANDLE_RGB |
                                      LIGMA_EXPORT_CAN_HANDLE_INDEXED);
          break;

        case FILE_TYPE_PFM:
          format_name = "PFM";
          export = ligma_export_image (&image, &n_drawables, &drawables, "PFM",
                                      LIGMA_EXPORT_CAN_HANDLE_RGB |
                                      LIGMA_EXPORT_CAN_HANDLE_GRAY);
          break;
        }

      if (export == LIGMA_EXPORT_CANCEL)
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("%s format does not support multiple layers."),
                   format_name);

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }

  if (file_type != FILE_TYPE_PFM &&
      run_mode  == LIGMA_RUN_INTERACTIVE)
    {
      if (! save_dialog (procedure, G_OBJECT (config)))
        status = LIGMA_PDB_CANCEL;
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      if (! save_image (file, image, drawables[0], file_type, G_OBJECT (config),
                        &error))
        {
          status = LIGMA_PDB_EXECUTION_ERROR;
        }
    }

  ligma_procedure_config_end_export (config, image, file, status);
  g_object_unref (config);

  if (export == LIGMA_EXPORT_EXPORT)
    {
      ligma_image_delete (image);
      g_free (drawables);
    }

  return ligma_procedure_new_return_values (procedure, status, error);
}

static LigmaImage *
load_image (GFile   *file,
            GError **error)
{
  GInputStream    *input;
  GeglBuffer      *buffer;
  LigmaImage * volatile image = NULL;
  LigmaLayer       *layer;
  char             buf[BUFLEN + 4];  /* buffer for random things like scanning */
  PNMInfo         *pnminfo;
  PNMScanner      *volatile scan;
  int             ctr;
  LigmaPrecision   precision;

  ligma_progress_init_printf (_("Opening '%s'"),
                             g_file_get_parse_name (file));

  input = G_INPUT_STREAM (g_file_read (file, NULL, error));
  if (! input)
    return NULL;

  /* allocate the necessary structures */
  pnminfo = g_new (PNMInfo, 1);

  scan = NULL;
  /* set error handling */
  if (setjmp (pnminfo->jmpbuf))
    {
      /* If we get here, we had a problem reading the file */
      if (scan)
        pnmscanner_destroy (scan);

      g_object_unref (input);
      g_free (pnminfo);

      if (image)
        ligma_image_delete (image);

      return NULL;
    }

  if (! (scan = pnmscanner_create (input)))
    longjmp (pnminfo->jmpbuf, 1);

  /* Get magic number */
  pnmscanner_gettoken (scan, buf, BUFLEN);
  CHECK_FOR_ERROR (pnmscanner_eof (scan), pnminfo->jmpbuf,
                   _("Premature end of file."));
  CHECK_FOR_ERROR ((buf[0] != 'P' || buf[2]), pnminfo->jmpbuf,
                   _("Invalid file."));

  /* Look up magic number to see what type of PNM this is */
  for (ctr = 0; pnm_types[ctr].name; ctr++)
    if (buf[1] == pnm_types[ctr].name)
      {
        pnminfo->np           = pnm_types[ctr].np;
        pnminfo->asciibody    = pnm_types[ctr].asciibody;
        pnminfo->float_format = g_ascii_tolower (pnm_types[ctr].name) == 'f';
        pnminfo->maxval       = pnm_types[ctr].maxval;
        pnminfo->loader       = pnm_types[ctr].loader;
      }

  if (! pnminfo->loader)
    {
      g_message (_("File not in a supported format."));
      longjmp (pnminfo->jmpbuf, 1);
    }

  pnmscanner_gettoken (scan, buf, BUFLEN);
  CHECK_FOR_ERROR (pnmscanner_eof (scan), pnminfo->jmpbuf,
                   _("Premature end of file."));
  pnminfo->xres = g_ascii_isdigit(*buf) ? atoi (buf) : 0;
  CHECK_FOR_ERROR (pnminfo->xres <= 0, pnminfo->jmpbuf,
                   _("Invalid X resolution."));
  CHECK_FOR_ERROR (pnminfo->xres > LIGMA_MAX_IMAGE_SIZE, pnminfo->jmpbuf,
                   _("Image width is larger than LIGMA can handle."));

  pnmscanner_gettoken (scan, buf, BUFLEN);
  CHECK_FOR_ERROR (pnmscanner_eof (scan), pnminfo->jmpbuf,
                   _("Premature end of file."));
  pnminfo->yres = g_ascii_isdigit (*buf) ? atoi (buf) : 0;
  CHECK_FOR_ERROR (pnminfo->yres <= 0, pnminfo->jmpbuf,
                   _("Invalid Y resolution."));
  CHECK_FOR_ERROR (pnminfo->yres > LIGMA_MAX_IMAGE_SIZE, pnminfo->jmpbuf,
                   _("Image height is larger than LIGMA can handle."));

  if (pnminfo->float_format)
    {
      gchar *endptr = NULL;

      pnmscanner_gettoken (scan, buf, BUFLEN);
      CHECK_FOR_ERROR (pnmscanner_eof (scan), pnminfo->jmpbuf,
                       _("Premature end of file."));

      pnminfo->scale_factor = g_ascii_strtod (buf, &endptr);
      CHECK_FOR_ERROR (endptr == NULL || *endptr != 0 || errno == ERANGE,
                       pnminfo->jmpbuf, _("Bogus scale factor."));
      CHECK_FOR_ERROR (! isnormal (pnminfo->scale_factor),
                       pnminfo->jmpbuf, _("Unsupported scale factor."));
      precision = LIGMA_PRECISION_FLOAT_LINEAR;
    }
  else if (pnminfo->np != 0)         /* pbm's don't have a maxval field */
    {
      pnmscanner_gettoken (scan, buf, BUFLEN);
      CHECK_FOR_ERROR (pnmscanner_eof (scan), pnminfo->jmpbuf,
                       _("Premature end of file."));

      pnminfo->maxval = g_ascii_isdigit (*buf) ? atoi (buf) : 0;
      CHECK_FOR_ERROR (((pnminfo->maxval<=0) || (pnminfo->maxval>65535)),
                       pnminfo->jmpbuf, _("Unsupported maximum value."));
      if (pnminfo->maxval < 256)
        {
          precision = LIGMA_PRECISION_U8_NON_LINEAR;
        }
      else
        {
          precision = LIGMA_PRECISION_U16_NON_LINEAR;
        }
    }
  else
    {
      precision = LIGMA_PRECISION_U8_NON_LINEAR;
    }

  /* Create a new image of the proper size and associate the filename
   * with it.
   */
  image = ligma_image_new_with_precision (pnminfo->xres, pnminfo->yres,
                                         (pnminfo->np >= 3) ? LIGMA_RGB : LIGMA_GRAY,
                                         precision);

  ligma_image_set_file (image, file);

  layer = ligma_layer_new (image, _("Background"),
                          pnminfo->xres, pnminfo->yres,
                          (pnminfo->np >= 3 ?
                           LIGMA_RGB_IMAGE : LIGMA_GRAY_IMAGE),
                          100,
                          ligma_image_get_default_new_layer_mode (image));
  ligma_image_insert_layer (image, layer, NULL, 0);

  buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

  pnminfo->loader (scan, pnminfo, buffer);

  /* Destroy the scanner */
  pnmscanner_destroy (scan);

  g_object_unref (buffer);
  g_free (pnminfo);
  g_object_unref (input);

  return image;
}

static void
pnm_load_ascii (PNMScanner *scan,
                PNMInfo    *info,
                GeglBuffer *buffer)
{
  gint      bpc;
  guchar   *data, *d;
  gushort  *s;
  gint      x, y, i, b;
  gint      start, end, scanlines;
  gint      np;
  gchar     buf[BUFLEN];
  gboolean  aborted = FALSE;

  np = (info->np) ? (info->np) : 1;

  if (info->maxval > 255)
    bpc = 2;
  else
    bpc = 1;

  /* No overflow as long as ligma_tile_height() < 1365 = 2^(31 - 18) / 6 */
  data = g_new (guchar, ligma_tile_height () * info->xres * np * bpc);

  /* Buffer reads to increase performance */
  pnmscanner_createbuffer (scan, 4096);

  for (y = 0; y < info->yres; y += scanlines)
    {
      start = y;
      end   = y + ligma_tile_height ();
      end   = MIN (end, info->yres);

      scanlines = end - start;

      d = data;
      s = (gushort *)d;

      for (i = 0; i < scanlines; i++)
        for (x = 0; x < info->xres; x++)
          {
            for (b = 0; b < np; b++)
              {
                if (aborted)
                  {
                    d[b] = 0;
                    continue;
                  }

                /* Truncated files will just have all 0's
                   at the end of the images */
                if (pnmscanner_eof (scan))
                  {
                    g_message (_("Premature end of file."));
                    aborted = TRUE;

                    d[b] = 0;
                    continue;
                  }

                if (info->np)
                  pnmscanner_gettoken (scan, buf, BUFLEN);
                else
                  pnmscanner_getsmalltoken (scan, buf);

                if (info->maxval == 1)
                  {
                    if (info->np)
                      d[b] = (*buf == '0') ? 0x00 : 0xff;
                    else
                      d[b] = (*buf == '0') ? 0xff : 0x00; /* invert for PBM */
                  }
                else if (bpc > 1)
                  {
                    s[b] = (65535.0 * (((gdouble) (g_ascii_isdigit (*buf) ?
                                                   atoi (buf) : 0))
                                       / (gdouble) (info->maxval)));
                  }
                else
                  {
                    d[b] = (255.0 * (((gdouble) (g_ascii_isdigit (*buf) ?
                                                 atoi (buf) : 0))
                                     / (gdouble) (info->maxval)));
                  }
              }

            if (bpc > 1)
              {
                s += np;
              }
            else
              {
                d += np;
              }
          }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y, info->xres, scanlines), 0,
                       NULL, data, GEGL_AUTO_ROWSTRIDE);

      ligma_progress_update ((double) y / (double) info->yres);
    }

  g_free (data);

  ligma_progress_update (1.0);
}

static void
pnm_load_raw (PNMScanner *scan,
              PNMInfo    *info,
              GeglBuffer *buffer)
{
  GInputStream *input;
  gint          bpc;
  guchar       *data, *d;
  gushort      *s;
  gint          x, y, i;
  gint          start, end, scanlines;

  if (info->maxval > 255)
    bpc = 2;
  else
    bpc = 1;

  /* No overflow as long as ligma_tile_height() < 1365 = 2^(31 - 18) / 6 */
  data = g_new (guchar, ligma_tile_height () * info->xres * info->np * bpc);

  input = pnmscanner_input (scan);

  for (y = 0; y < info->yres; y += scanlines)
    {
      start = y;
      end = y + ligma_tile_height ();
      end = MIN (end, info->yres);
      scanlines = end - start;
      d = data;
      s = (gushort *)data;

      for (i = 0; i < scanlines; i++)
        {
          gsize   bytes_read;
          GError *error = NULL;

          if (g_input_stream_read_all (input, d, info->xres * info->np * bpc,
                                       &bytes_read, NULL, &error))
            {
              CHECK_FOR_ERROR (info->xres * info->np * bpc != bytes_read,
                               info->jmpbuf,
                               _("Premature end of file."));
            }
          else
            {
              CHECK_FOR_ERROR (FALSE, info->jmpbuf, "%s", error->message);
            }

          if (bpc > 1)
            {
              for (x = 0; x < info->xres * info->np; x++)
                {
                  int v;

                  v = *d++ << 8;
                  v += *d++;

                  s[x] = MIN (v, info->maxval); /* guard against overflow */
                  s[x] = 65535.0 * (gdouble) v / (gdouble) info->maxval;
                }
              s += info->xres * info->np;
            }
          else
            {
              if (info->maxval != 255)      /* Normalize if needed */
                {
                  for (x = 0; x < info->xres * info->np; x++)
                    {
                      d[x] = MIN (d[x], info->maxval); /* guard against overflow */
                      d[x] = 255.0 * (gdouble) d[x] / (gdouble) info->maxval;
                    }
                }

              d += info->xres * info->np;
            }
        }

      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, y, info->xres, scanlines), 0,
                       NULL, data, GEGL_AUTO_ROWSTRIDE);

      ligma_progress_update ((double) y / (double) info->yres);
    }

  g_free (data);

  ligma_progress_update (1.0);
}

static void
pnm_load_rawpbm (PNMScanner *scan,
                 PNMInfo    *info,
                 GeglBuffer *buffer)
{
  GInputStream *input;
  guchar       *buf;
  guchar        curbyte;
  guchar       *data, *d;
  gint          x, y, i;
  gint          start, end, scanlines;
  gint          rowlen, bufpos;

  input = pnmscanner_input (scan);

  rowlen = (int)ceil ((double)(info->xres)/8.0);
  data = g_new (guchar, ligma_tile_height () * info->xres);
  buf = g_new (guchar, rowlen);

  for (y = 0; y < info->yres; y += scanlines)
    {
      start = y;
      end = y + ligma_tile_height ();
      end = MIN (end, info->yres);
      scanlines = end - start;
      d = data;

      for (i = 0; i < scanlines; i++)
        {
          gsize   bytes_read;
          GError *error = NULL;

          if (g_input_stream_read_all (input, buf, rowlen,
                                       &bytes_read, NULL, &error))
            {
              CHECK_FOR_ERROR (rowlen != bytes_read,
                               info->jmpbuf,
                               _("Premature end of file."));
            }
          else
            {
              CHECK_FOR_ERROR (FALSE, info->jmpbuf, "%s", error->message);
            }

          bufpos = 0;
          curbyte = buf[0];

          for (x = 0; x < info->xres; x++)
            {
              if ((x % 8) == 0)
                curbyte = buf[bufpos++];
              d[x] = (curbyte & 0x80) ? 0x00 : 0xff;
              curbyte <<= 1;
            }

          d += info->xres;
        }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y, info->xres, scanlines), 0,
                       NULL, data, GEGL_AUTO_ROWSTRIDE);

      ligma_progress_update ((double) y / (double) info->yres);
    }

  g_free (buf);
  g_free (data);

  ligma_progress_update (1.0);
}

static void
pnm_load_rawpfm (PNMScanner *scan,
                 PNMInfo    *info,
                 GeglBuffer *buffer)
{
  GInputStream *input;
  gfloat       *data;
  gint          x, y;
  gboolean      swap_byte_order;

  swap_byte_order =
    (info->scale_factor >= 0.0) ^ (G_BYTE_ORDER == G_BIG_ENDIAN);

  data = g_new (gfloat, info->xres * info->np);

  input = pnmscanner_input (scan);

  for (y = info->yres - 1; y >= 0; y--)
    {
      gsize   bytes_read;
      GError *error = NULL;

      if (g_input_stream_read_all (input, data,
                                   info->xres * info->np * sizeof (float),
                                   &bytes_read, NULL, &error))
        {
          CHECK_FOR_ERROR
              (info->xres * info->np * sizeof (float) != bytes_read,
               info->jmpbuf, _("Premature end of file."));
        }
      else
        {
          CHECK_FOR_ERROR (FALSE, info->jmpbuf, "%s", error->message);
        }

      for (x = 0; x < info->xres * info->np; x++)
        {
          if (swap_byte_order)
            {
              union { gfloat f; guint32 i; } v;

              v.f = data[x];
              v.i = GUINT32_SWAP_LE_BE (v.i);
              data[x] = v.f;
            }

          /* let's see if this is what people want, the PFM specs are
           * a little vague about what the scale factor should be used
           * for
           */
          data[x] *= fabsf (info->scale_factor);
          /* Keep values smaller than zero. That is in line with what
           * the TIFF loader does. If the user doesn't want the
           * negative numbers they have to get rid of them afterwards
           */
          /* data[x] = fmaxf (0.0f, fminf (FLT_MAX, data[x])); */
        }

        gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, y, info->xres, 1), 0,
                       NULL, data, GEGL_AUTO_ROWSTRIDE);

      if (y % 32 == 0)
        ligma_progress_update ((double) (info->yres - y) / (double) info->yres);
    }

  g_free (data);

  ligma_progress_update (1.0);
}

static gboolean
output_write (GOutputStream *output,
              gconstpointer  buffer,
              gsize          count,
              GError       **error)
{
  return g_output_stream_write_all (output, buffer, count, NULL, NULL, error);
}

/* Writes out mono raw rows */
static gboolean
pnmsaverow_raw_pbm (PNMRowInfo    *ri,
                    guchar        *data,
                    GError       **error)
{
  gint    b, p  = 0;
  gchar  *rbcur = ri->rowbuf;
  gint32  len   = (gint) ceil ((gdouble) (ri->xres) / 8.0);

  for (b = 0; b < len; b++) /* each output byte */
    {
      gint i;

      rbcur[b] = 0;

      for (i = 0; i < 8; i++) /* each bit in this byte */
        {
          if (p >= ri->xres)
            break;

          if (data[p] != ri->zero_is_black)
            rbcur[b] |= (char) (1 << (7 - i));

          p++;
        }
    }

  return output_write (ri->output, ri->rowbuf, len, error);
}

/* Writes out mono ascii rows */
static gboolean
pnmsaverow_ascii_pbm (PNMRowInfo    *ri,
                      guchar        *data,
                      GError       **error)
{
  static gint  line_len = 0;  /* ascii pbm lines must be <= 70 chars long */
  gint32       len = 0;
  gint         i;
  gchar       *rbcur = ri->rowbuf;

  for (i = 0; i < ri->xres; i++)
    {
      if (line_len > 69)
        {
          rbcur[i] = '\n';
          line_len = 0;
          len++;
          rbcur++;
        }

      if (data[i] == ri->zero_is_black)
        rbcur[i] = '0';
      else
        rbcur[i] = '1';

      line_len++;
      len++;
    }

  *(rbcur+i) = '\n';

  return output_write (ri->output, ri->rowbuf, len, error);
}

/* Writes out RGB and grayscale raw rows */
static gboolean
pnmsaverow_raw (PNMRowInfo    *ri,
                guchar        *data,
                GError       **error)
{
  gint i;
  if (ri->bpc == 2)
    {
      gushort *d = (gushort *)data;
      for  (i = 0; i < ri->xres * ri->np; i++)
        {
          *d = g_htons(*d);
          d++;
        }
    }
  return output_write (ri->output, data, ri->xres * ri->np * ri->bpc, error);
}

/* Writes out RGB and grayscale float rows */
static gboolean
pnmsaverow_float (PNMRowInfo    *ri,
                  const float   *data,
                  GError       **error)
{
  return output_write (ri->output, data,
                       ri->xres * ri->np * sizeof (float),
                       error);
}

/* Writes out indexed raw rows */
static gboolean
pnmsaverow_raw_indexed (PNMRowInfo    *ri,
                        guchar        *data,
                        GError       **error)
{
  gint   i;
  gchar *rbcur = ri->rowbuf;

  for (i = 0; i < ri->xres; i++)
    {
      *(rbcur++) = ri->red[*data];
      *(rbcur++) = ri->grn[*data];
      *(rbcur++) = ri->blu[*(data++)];
    }

  return output_write (ri->output, ri->rowbuf, ri->xres * 3, error);
}

/* Writes out RGB and grayscale ascii rows */
static gboolean
pnmsaverow_ascii (PNMRowInfo    *ri,
                  guchar        *data,
                  GError       **error)
{
  gint   i;
  gchar *rbcur = ri->rowbuf;
  gushort *sdata = (gushort *)data;

  for (i = 0; i < ri->xres * ri->np; i++)
    {
      if (ri->bpc == 2)
        {
          sprintf ((gchar *) rbcur,"%d\n", 0xffff & *(sdata++));
        }
      else
        {
          sprintf ((gchar *) rbcur,"%d\n", 0xff & *(data++));
        }
      rbcur += strlen (rbcur);
    }

  return output_write (ri->output, ri->rowbuf, rbcur - ri->rowbuf,
                       error);
}

/* Writes out RGB and grayscale ascii rows */
static gboolean
pnmsaverow_ascii_indexed (PNMRowInfo    *ri,
                          guchar        *data,
                          GError       **error)
{
  gint   i;
  gchar *rbcur = ri->rowbuf;

  for (i = 0; i < ri->xres; i++)
    {
      sprintf (rbcur, "%d\n", 0xff & ri->red[*(data)]);
      rbcur += strlen (rbcur);
      sprintf (rbcur, "%d\n", 0xff & ri->grn[*(data)]);
      rbcur += strlen (rbcur);
      sprintf (rbcur, "%d\n", 0xff & ri->blu[*(data++)]);
      rbcur += strlen (rbcur);
    }

  return output_write (ri->output, ri->rowbuf, strlen ((char *) ri->rowbuf),
                       error);
}

static gboolean
save_image (GFile         *file,
            LigmaImage     *image,
            LigmaDrawable  *drawable,
            FileType       file_type,
            GObject       *config,
            GError       **error)
{
  gboolean       status = FALSE;
  GOutputStream *output = NULL;
  GeglBuffer    *buffer = NULL;
  const Babl    *format;
  const gchar   *header_string = NULL;
  LigmaImageType  drawable_type;
  PNMRowInfo     rowinfo;
  PNMSaverowFunc saverow = NULL;
  guchar         red[256];
  guchar         grn[256];
  guchar         blu[256];
  gchar          buf[BUFLEN];
  gint           np = 0;
  gint           xres, yres;
  gint           ypos, yend;
  gint           rowbufsize = 0;
  gchar         *comment    = NULL;
  gboolean       config_raw = TRUE;

  if (file_type != FILE_TYPE_PFM)
    g_object_get (config,
                  "raw", &config_raw,
                  NULL);

  /*  Make sure we're not saving an image with an alpha channel  */
  if (ligma_drawable_has_alpha (drawable))
    {
      g_message (_("Cannot export images with alpha channel."));
      goto out;
    }

  ligma_progress_init_printf (_("Exporting '%s'"),
                             g_file_get_parse_name (file));

  /* open the file */
  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (! output)
    goto out;

  buffer = ligma_drawable_get_buffer (drawable);

  xres = gegl_buffer_get_width  (buffer);
  yres = gegl_buffer_get_height (buffer);

  drawable_type = ligma_drawable_type (drawable);

  switch (ligma_image_get_precision (image))
    {
    case LIGMA_PRECISION_U8_LINEAR:
    case LIGMA_PRECISION_U8_NON_LINEAR:
    case LIGMA_PRECISION_U8_PERCEPTUAL:
      rowinfo.bpc = 1;
      break;
    default:
      rowinfo.bpc = 2;
      break;
    }

  /* write out magic number */
  if (file_type != FILE_TYPE_PFM && ! config_raw)
    {
      if (file_type == FILE_TYPE_PBM)
        {
          header_string = "P1\n";
          format = gegl_buffer_get_format (buffer);
          np = 0;
          rowbufsize = xres + (int) (xres / 70) + 1;
          saverow = pnmsaverow_ascii_pbm;
        }
      else
        {
          switch (drawable_type)
            {
            case LIGMA_GRAY_IMAGE:
              header_string = "P2\n";
              if (rowinfo.bpc == 1)
                {
                  format = babl_format ("Y' u8");
                  rowbufsize = xres * 4;
                }
              else
                {
                  format = babl_format ("Y' u16");
                  rowbufsize = xres * 6;
                }
              np = 1;
              saverow = pnmsaverow_ascii;
              break;

            case LIGMA_RGB_IMAGE:
              header_string = "P3\n";
              if (rowinfo.bpc == 1)
                {
                  format = babl_format ("R'G'B' u8");
                  rowbufsize = xres * 12;
                }
              else
                {
                  format = babl_format ("R'G'B' u16");
                  rowbufsize = xres * 18;
                }
              np = 3;
              saverow = pnmsaverow_ascii;
              break;

            case LIGMA_INDEXED_IMAGE:
              header_string = "P3\n";
              format = gegl_buffer_get_format (buffer);
              np = 1;
              rowbufsize = xres * 12;
              saverow = pnmsaverow_ascii_indexed;
              break;

            default:
              g_warning ("PNM: Unknown drawable_type\n");
              goto out;
            }
        }
    }
  else if (file_type != FILE_TYPE_PFM)
    {
      if (file_type == FILE_TYPE_PBM)
        {
          header_string = "P4\n";
          format = gegl_buffer_get_format (buffer);
          np = 0;
          rowbufsize = (gint) ceil ((gdouble) xres / 8.0);
          saverow = pnmsaverow_raw_pbm;
        }
      else
        {
          switch (drawable_type)
            {
            case LIGMA_GRAY_IMAGE:
              header_string = "P5\n";
              if (rowinfo.bpc == 1)
                {
                  format = babl_format ("Y' u8");
                  rowbufsize = xres;
                }
              else
                {
                  format = babl_format ("Y' u16");
                  rowbufsize = xres * 2;
                }
              np = 1;
              saverow = pnmsaverow_raw;
              break;

            case LIGMA_RGB_IMAGE:
              header_string = "P6\n";
              if (rowinfo.bpc == 1)
                {
                  format = babl_format ("R'G'B' u8");
                  rowbufsize = xres * 3;
                }
              else
                {
                  format = babl_format ("R'G'B' u16");
                  rowbufsize = xres * 6;
                }
              np = 3;
              saverow = pnmsaverow_raw;
              break;

            case LIGMA_INDEXED_IMAGE:
              header_string = "P6\n";
              format = gegl_buffer_get_format (buffer);
              np = 1;
              rowbufsize = xres * 3;
              saverow = pnmsaverow_raw_indexed;
              break;

            default:
              g_warning ("PNM: Unknown drawable_type\n");
              goto out;
            }
        }
    }
  else
    {
      switch (drawable_type)
        {
        case LIGMA_GRAY_IMAGE:
          header_string = "Pf\n";
          format = babl_format ("Y float");
          np = 1;
          break;

        case LIGMA_RGB_IMAGE:
          header_string = "PF\n";
          format = babl_format ("RGB float");
          np = 3;
          break;

        default:
          g_warning ("PFM: Unknown drawable_type\n");
          goto out;
        }
    }

  if (! output_write (output, header_string, strlen (header_string), error))
    goto out;

  rowinfo.zero_is_black = FALSE;

  if (drawable_type == LIGMA_INDEXED_IMAGE)
    {
      guchar *cmap;
      gint    num_colors;

      cmap = ligma_image_get_colormap (image, &num_colors);

      if (file_type == FILE_TYPE_PBM)
        {
          /*  Test which of the two colors is white and which is black  */
          switch (num_colors)
            {
            case 1:
              rowinfo.zero_is_black = (LIGMA_RGB_LUMINANCE (cmap[0],
                                                           cmap[1],
                                                           cmap[2]) < 128);
              break;

            case 2:
              rowinfo.zero_is_black = (LIGMA_RGB_LUMINANCE (cmap[0],
                                                           cmap[1],
                                                           cmap[2]) <
                                       LIGMA_RGB_LUMINANCE (cmap[3],
                                                           cmap[4],
                                                           cmap[5]));
              break;

            default:
              g_warning ("Images exported as PBM should be black/white");
              break;
            }
        }
      else
        {
          const guchar *c = cmap;
          gint          i;

          for (i = 0; i < num_colors; i++)
            {
              red[i] = *c++;
              grn[i] = *c++;
              blu[i] = *c++;
            }

          rowinfo.red = red;
          rowinfo.grn = grn;
          rowinfo.blu = blu;
        }

      g_free (cmap);
    }

  if (file_type != FILE_TYPE_PFM)
    {
      /* write out comment string */
      comment = g_strdup_printf("# Created by LIGMA version %s PNM plug-in\n",
                                LIGMA_VERSION);

      if (! output_write (output, comment, strlen (comment), error))
        goto out;
    }

  /* write out resolution and maxval */
  if (file_type == FILE_TYPE_PBM)
    {
      g_snprintf (buf, sizeof (buf), "%d %d\n", xres, yres);
    }
  else if (file_type != FILE_TYPE_PFM)
    {
      g_snprintf (buf, sizeof (buf), "%d %d\n%d\n", xres, yres,
                  rowinfo.bpc == 1 ? 255 : 65535);
    }
  else
    {
      g_snprintf (buf, sizeof (buf), "%d %d\n%s\n", xres, yres,
                  G_BYTE_ORDER == G_BIG_ENDIAN ? "1.0" : "-1.0");
    }

  if (! output_write (output, buf, strlen (buf), error))
    goto out;

  if (file_type != FILE_TYPE_PFM)
    {
      guchar *data;
      guchar *d;
      gchar  *rowbuf = NULL;

      /* allocate a buffer for retrieving information from the pixel region  */
      data = g_new (guchar,
                    ligma_tile_height () * xres *
                    babl_format_get_bytes_per_pixel (format));

      rowbuf = g_new (gchar, rowbufsize + 1);

      rowinfo.output = output;
      rowinfo.rowbuf = rowbuf;
      rowinfo.xres   = xres;
      rowinfo.np     = np;

      d = NULL; /* only to please the compiler */

      /* Write the body out */
      for (ypos = 0; ypos < yres; ypos++)
        {
          if ((ypos % ligma_tile_height ()) == 0)
            {
              yend = ypos + ligma_tile_height ();
              yend = MIN (yend, yres);

              gegl_buffer_get (buffer,
                               GEGL_RECTANGLE (0, ypos, xres, yend - ypos), 1.0,
                               format, data,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

              d = data;
            }

          if (! saverow (&rowinfo, d, error))
            {
              g_free (rowbuf);
              g_free (data);
              goto out;
            }

          d += xres * (np ? np : 1) * rowinfo.bpc;

          if (ypos % 32 == 0)
            ligma_progress_update ((double) ypos / (double) yres);
        }

      g_free (rowbuf);
      g_free (data);
    }
  else
    {
      /* allocate a buffer for retrieving information from the pixel
       * region
       */
      gfloat *data = g_new (gfloat, xres * np);

      rowinfo.output = output;
      rowinfo.rowbuf = NULL;
      rowinfo.xres   = xres;
      rowinfo.np     = np;

      /* Write the body out in reverse row order */
      for (ypos = yres - 1; ypos >= 0; ypos--)
        {
          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (0, ypos, xres, 1), 1.0,
                           format, data,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          if (! pnmsaverow_float (&rowinfo, data, error))
            {
              g_free (data);
              goto out;
            }

          if (ypos % 32 == 0)
            ligma_progress_update ((double) (yres - ypos) / (double) yres);
        }

      g_free (data);
    }

  ligma_progress_update (1.0);
  status = TRUE;

 out:
  if (! status)
    {
      GCancellable  *cancellable = g_cancellable_new ();

      g_cancellable_cancel (cancellable);
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);
    }

  if (comment)
    g_free (comment);
  if (buffer)
    g_object_unref (buffer);
  if (output)
    g_object_unref (output);

  return status;
}

static gboolean
save_dialog (LigmaProcedure *procedure,
             GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  gboolean   run;

  dialog = ligma_procedure_dialog_new (procedure,
                                      LIGMA_PROCEDURE_CONFIG (config),
                                      _("Export Image as PNM"));

  /*  file save type  */
  frame = ligma_prop_boolean_radio_frame_new (config, "raw",
                                             _("Data formatting"),
                                             _("_Raw"),
                                             _("_ASCII"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = ligma_procedure_dialog_run (LIGMA_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}


/**************** FILE SCANNER UTILITIES **************/

/* pnmscanner_create ---
 *    Creates a new scanner based on a file descriptor.  The
 *    look ahead buffer is one character initially.
 */
static PNMScanner *
pnmscanner_create (GInputStream *input)
{
  PNMScanner *s;
  gsize       bytes_read;

  s = g_new (PNMScanner, 1);

  s->input = input;
  s->inbuf = NULL;
  s->eof   = FALSE;

  if (! g_input_stream_read_all (input, &s->cur, 1,
                                 &bytes_read, NULL, NULL) ||
      bytes_read != 1)
    {
      s->eof = TRUE;
    }

  return s;
}

/* pnmscanner_destroy ---
 *    Destroys a scanner and its resources.  Doesn't close the fd.
 */
static void
pnmscanner_destroy (PNMScanner *s)
{
  if (s->inbuf)
    g_free (s->inbuf);

  g_free (s);
}

/* pnmscanner_createbuffer ---
 *    Creates a buffer so we can do buffered reads.
 */
static void
pnmscanner_createbuffer (PNMScanner *s,
                         gint        bufsize)
{
  gsize bytes_read;

  s->inbuf          = g_new (gchar, bufsize);
  s->inbufsize      = bufsize;
  s->inbufpos       = 0;
  s->inbufvalidsize = 0;

  g_input_stream_read_all (s->input, s->inbuf, bufsize,
                           &bytes_read, NULL, NULL);

  s->inbufvalidsize = bytes_read;
}

/* pnmscanner_gettoken ---
 *    Gets the next token, eating any leading whitespace.
 */
static void
pnmscanner_gettoken (PNMScanner *s,
                     gchar      *buf,
                     gint        bufsize)
{
  gint ctr = 0;

  pnmscanner_eatwhitespace (s);

  while (! s->eof                   &&
         ! g_ascii_isspace (s->cur) &&
         (s->cur != '#')            &&
         (ctr < bufsize))
    {
      buf[ctr++] = s->cur;
      pnmscanner_getchar (s);
    }

  buf[ctr] = '\0';
}

/* pnmscanner_getsmalltoken ---
 *    Gets the next char, eating any leading whitespace.
 */
static void
pnmscanner_getsmalltoken (PNMScanner *s,
                          gchar      *buf)
{
  pnmscanner_eatwhitespace (s);

  if (! s->eof && ! g_ascii_isspace (s->cur) && (s->cur != '#'))
    {
      *buf = s->cur;
      pnmscanner_getchar (s);
    }
}

/* pnmscanner_getchar ---
 *    Reads a character from the input stream
 */
static void
pnmscanner_getchar (PNMScanner *s)
{
  if (s->inbuf)
    {
      s->cur = s->inbuf[s->inbufpos++];

      if (s->inbufpos >= s->inbufvalidsize)
        {
          if (s->inbufpos > s->inbufvalidsize)
            {
              s->eof = 1;
            }
          else
            {
              gsize bytes_read;

              g_input_stream_read_all (s->input, s->inbuf, s->inbufsize,
                                       &bytes_read, NULL, NULL);

              s->inbufvalidsize = bytes_read;
            }

          s->inbufpos = 0;
        }
    }
  else
    {
      gsize bytes_read;

      s->eof = FALSE;

      if (! g_input_stream_read_all (s->input, &s->cur, 1,
                                     &bytes_read, NULL, NULL) ||
          bytes_read != 1)
        {
          s->eof = TRUE;
        }
    }
}

/* pnmscanner_eatwhitespace ---
 *    Eats up whitespace from the input and returns when done or eof.
 *    Also deals with comments.
 */
static void
pnmscanner_eatwhitespace (PNMScanner *s)
{
  gint state = 0;

  while (! (s->eof) && (state != -1))
    {
      switch (state)
        {
        case 0:  /* in whitespace */
          if (s->cur == '#')
            {
              state = 1;  /* goto comment */
              pnmscanner_getchar (s);
            }
          else if (! g_ascii_isspace (s->cur))
            {
              state = -1;
            }
          else
            {
              pnmscanner_getchar (s);
            }
          break;

        case 1:  /* in comment */
          if (s->cur == '\n')
            state = 0;  /* goto whitespace */
          pnmscanner_getchar (s);
          break;
        }
    }
}
