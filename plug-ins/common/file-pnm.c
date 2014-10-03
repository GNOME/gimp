/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * The pnm reading and writing code was written from scratch by Erik Nygren
 * (nygren@mit.edu) based on the specifications in the man pages and
 * does not contain any code from the netpbm or pbmplus distributions.
 *
 * 2006: pbm saving written by Martin K Collins (martin@mkcollins.org)
 */

#include "config.h"

#include <setjmp.h>
#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-pnm-load"
#define PNM_SAVE_PROC  "file-pnm-save"
#define PBM_SAVE_PROC  "file-pbm-save"
#define PGM_SAVE_PROC  "file-pgm-save"
#define PPM_SAVE_PROC  "file-ppm-save"
#define PLUG_IN_BINARY "file-pnm"
#define PLUG_IN_ROLE   "gimp-file-pnm"


/* Declare local data types
 */

typedef struct _PNMScanner PNMScanner;
typedef struct _PNMInfo    PNMInfo;
typedef struct _PNMRowInfo PNMRowInfo;

typedef void     (* PNMLoaderFunc)  (PNMScanner    *scanner,
                                     PNMInfo       *info,
                                     GeglBuffer    *buffer);
typedef gboolean (* PNMSaverowFunc) (PNMRowInfo    *info,
                                     const guchar  *data,
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
  gint          maxval;         /* For ascii image files, the max value
                                 * which we need to normalize to */
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
  guchar        *red;           /* Colormap red                */
  guchar        *grn;           /* Colormap green              */
  guchar        *blu;           /* Colormap blue               */
  gboolean       zero_is_black; /* index zero is black (PBM only) */
};

/* Save info  */
typedef struct
{
  gint  raw;            /*  raw or ascii  */
} PNMSaveVals;

#define BUFLEN 512              /* The input buffer size for data returned
                                 * from the scanner.  Note that lines
                                 * aren't allowed to be over 256 characters
                                 * by the spec anyways so this shouldn't
                                 * be an issue. */

#define SAVE_COMMENT_STRING "# CREATOR: GIMP PNM Filter Version 1.1\n"


/* Declare some local functions.
 */
static void       query      (void);
static void       run        (const gchar      *name,
                              gint              nparams,
                              const GimpParam  *param,
                              gint             *nreturn_vals,
                              GimpParam       **return_vals);
static gint32     load_image (GFile            *file,
                              GError          **error);
static gint       save_image (GFile            *file,
                              gint32            image_ID,
                              gint32            drawable_ID,
                              gboolean          pbm,
                              GError          **error);

static gboolean   save_dialog              (void);

static void       pnm_load_ascii           (PNMScanner    *scan,
                                            PNMInfo       *info,
                                            GeglBuffer    *buffer);
static void       pnm_load_raw             (PNMScanner    *scan,
                                            PNMInfo       *info,
                                            GeglBuffer    *buffer);
static void       pnm_load_rawpbm          (PNMScanner    *scan,
                                            PNMInfo       *info,
                                            GeglBuffer    *buffer);

static gboolean   pnmsaverow_ascii         (PNMRowInfo    *ri,
                                            const guchar  *data,
                                            GError       **error);
static gboolean   pnmsaverow_raw           (PNMRowInfo    *ri,
                                            const guchar  *data,
                                            GError       **error);
static gboolean   pnmsaverow_raw_pbm       (PNMRowInfo    *ri,
                                            const guchar  *data,
                                            GError       **error);
static gboolean   pnmsaverow_ascii_pbm     (PNMRowInfo    *ri,
                                            const guchar  *data,
                                            GError       **error);
static gboolean   pnmsaverow_ascii_indexed (PNMRowInfo    *ri,
                                            const guchar  *data,
                                            GError       **error);
static gboolean   pnmsaverow_raw_indexed   (PNMRowInfo    *ri,
                                            const guchar  *data,
                                            GError       **error);

static void       pnmscanner_destroy       (PNMScanner    *s);
static void       pnmscanner_createbuffer  (PNMScanner    *s,
                                            gint           bufsize);
static void       pnmscanner_getchar       (PNMScanner    *s);
static void       pnmscanner_eatwhitespace (PNMScanner    *s);
static void       pnmscanner_gettoken      (PNMScanner    *s,
                                            gchar         *buf,
                                            gint           bufsize);
static void       pnmscanner_getsmalltoken (PNMScanner    *s,
                                            gchar         *buf);

static PNMScanner * pnmscanner_create      (GInputStream  *input);


#define pnmscanner_eof(s)   ((s)->eof)
#define pnmscanner_input(s) ((s)->input)

/* Checks for a fatal error */
#define CHECK_FOR_ERROR(predicate, jmpbuf, ...) \
        if ((predicate)) \
          { g_message (__VA_ARGS__); longjmp ((jmpbuf), 1); }

static const struct
{
  gchar         name;
  gint          np;
  gint          asciibody;
  gint          maxval;
  PNMLoaderFunc loader;
} pnm_types[] =
{
  { '1', 0, 1,   1, pnm_load_ascii  },  /* ASCII PBM */
  { '2', 1, 1, 255, pnm_load_ascii  },  /* ASCII PGM */
  { '3', 3, 1, 255, pnm_load_ascii  },  /* ASCII PPM */
  { '4', 0, 0,   1, pnm_load_rawpbm },  /* RAW   PBM */
  { '5', 1, 0, 255, pnm_load_raw    },  /* RAW   PGM */
  { '6', 3, 0, 255, pnm_load_raw    },  /* RAW   PPM */
  {  0 , 0, 0,   0, NULL}
};

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static PNMSaveVals psvals =
{
  TRUE     /* raw? or ascii */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save"             },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" },
    { GIMP_PDB_INT32,    "raw",          "TRUE for raw output, FALSE for ascii output" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads files in the PNM file format",
                          "This plug-in loads files in the various Netpbm portable file formats.",
                          "Erik Nygren",
                          "Erik Nygren",
                          "1996",
                          N_("PNM Image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-portable-anymap");
  gimp_register_file_handler_uri (LOAD_PROC);
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "pnm,ppm,pgm,pbm",
                                    "",
                                    "0,string,P1,0,string,P2,0,string,P3,0,"
                                    "string,P4,0,string,P5,0,string,P6");

  gimp_install_procedure (PNM_SAVE_PROC,
                          "Saves files in the PNM file format",
                          "PNM saving handles all image types without transparency.",
                          "Erik Nygren",
                          "Erik Nygren",
                          "1996",
                          N_("PNM image"),
                          "RGB, GRAY, INDEXED",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_install_procedure (PBM_SAVE_PROC,
                          "Saves files in the PBM file format",
                          "PBM saving produces mono images without transparency.",
                          "Martin K Collins",
                          "Erik Nygren",
                          "2006",
                          N_("PBM image"),
                          "RGB, GRAY, INDEXED",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_install_procedure (PGM_SAVE_PROC,
                          "Saves files in the PGM file format",
                          "PGM saving produces grayscale images without transparency.",
                          "Erik Nygren",
                          "Erik Nygren",
                          "1996",
                          N_("PGM image"),
                          "RGB, GRAY, INDEXED",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_install_procedure (PPM_SAVE_PROC,
                          "Saves files in the PPM file format",
                          "PPM saving handles RGB images without transparency.",
                          "Erik Nygren",
                          "Erik Nygren",
                          "1996",
                          N_("PPM image"),
                          "RGB, GRAY, INDEXED",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (PNM_SAVE_PROC, "image/x-portable-anymap");
  gimp_register_file_handler_mime (PBM_SAVE_PROC, "image/x-portable-bitmap");
  gimp_register_file_handler_mime (PGM_SAVE_PROC, "image/x-portable-graymap");
  gimp_register_file_handler_mime (PPM_SAVE_PROC, "image/x-portable-pixmap");

  gimp_register_file_handler_uri (PNM_SAVE_PROC);
  gimp_register_file_handler_uri (PBM_SAVE_PROC);
  gimp_register_file_handler_uri (PGM_SAVE_PROC);
  gimp_register_file_handler_uri (PPM_SAVE_PROC);

  gimp_register_save_handler (PNM_SAVE_PROC, "pnm", "");
  gimp_register_save_handler (PBM_SAVE_PROC, "pbm", "");
  gimp_register_save_handler (PGM_SAVE_PROC, "pgm", "");
  gimp_register_save_handler (PPM_SAVE_PROC, "ppm", "");
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
  gint32             drawable_ID;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GError            *error  = NULL;
  gboolean           pbm    = FALSE;  /* flag for PBM output */

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (g_file_new_for_uri (param[1].data.d_string),
                             &error);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, PNM_SAVE_PROC) == 0 ||
           strcmp (name, PBM_SAVE_PROC) == 0 ||
           strcmp (name, PGM_SAVE_PROC) == 0 ||
           strcmp (name, PPM_SAVE_PROC) == 0)
    {
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          if (strcmp (name, PNM_SAVE_PROC) == 0)
            {
              export = gimp_export_image (&image_ID, &drawable_ID, "PNM",
                                          GIMP_EXPORT_CAN_HANDLE_RGB  |
                                          GIMP_EXPORT_CAN_HANDLE_GRAY |
                                          GIMP_EXPORT_CAN_HANDLE_INDEXED);
            }
          else if (strcmp (name, PBM_SAVE_PROC) == 0)
            {
              export = gimp_export_image (&image_ID, &drawable_ID, "PBM",
                                          GIMP_EXPORT_CAN_HANDLE_BITMAP);
              pbm = TRUE;  /* gimp has no mono image type so hack it */
            }
          else if (strcmp (name, PGM_SAVE_PROC) == 0)
            {
              export = gimp_export_image (&image_ID, &drawable_ID, "PGM",
                                          GIMP_EXPORT_CAN_HANDLE_GRAY);
            }
          else
            {
              export = gimp_export_image (&image_ID, &drawable_ID, "PPM",
                                          GIMP_EXPORT_CAN_HANDLE_RGB |
                                          GIMP_EXPORT_CAN_HANDLE_INDEXED);
            }

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }
        break;

        default:
          break;
        }

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data (name, &psvals);

          /*  First acquire information with a dialog  */
          if (! save_dialog ())
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 6)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              psvals.raw = (param[5].data.d_int32) ? TRUE : FALSE;
              pbm = (strcmp (name, PBM_SAVE_PROC) == 0);
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /*  Possibly retrieve data  */
          gimp_get_data (name, &psvals);
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (save_image (g_file_new_for_uri (param[3].data.d_string),
                          image_ID, drawable_ID, pbm,
                          &error))
            {
              /*  Store psvals data  */
              gimp_set_data (name, &psvals, sizeof (PNMSaveVals));
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}

static gint32
load_image (GFile   *file,
            GError **error)
{
  GInputStream    *input;
  GeglBuffer      *buffer;
  gint32 volatile  image_ID = -1;
  gint32           layer_ID;
  char             buf[BUFLEN + 4];  /* buffer for random things like scanning */
  PNMInfo         *pnminfo;
  PNMScanner      *volatile scan;
  int             ctr;

  gimp_progress_init_printf (_("Opening '%s'"),
                             g_file_get_parse_name (file));

  input = G_INPUT_STREAM (g_file_read (file, NULL, error));
  if (! input)
    return -1;

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

      if (image_ID != -1)
        gimp_image_delete (image_ID);

      return -1;
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
        pnminfo->np        = pnm_types[ctr].np;
        pnminfo->asciibody = pnm_types[ctr].asciibody;
        pnminfo->maxval    = pnm_types[ctr].maxval;
        pnminfo->loader    = pnm_types[ctr].loader;
      }

  if (!pnminfo->loader)
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
  CHECK_FOR_ERROR (pnminfo->xres > GIMP_MAX_IMAGE_SIZE, pnminfo->jmpbuf,
                   _("Image width is larger than GIMP can handle."));

  pnmscanner_gettoken (scan, buf, BUFLEN);
  CHECK_FOR_ERROR (pnmscanner_eof (scan), pnminfo->jmpbuf,
                   _("Premature end of file."));
  pnminfo->yres = g_ascii_isdigit (*buf) ? atoi (buf) : 0;
  CHECK_FOR_ERROR (pnminfo->yres <= 0, pnminfo->jmpbuf,
                   _("Invalid Y resolution."));
  CHECK_FOR_ERROR (pnminfo->yres > GIMP_MAX_IMAGE_SIZE, pnminfo->jmpbuf,
                   _("Image height is larger than GIMP can handle."));

  if (pnminfo->np != 0)         /* pbm's don't have a maxval field */
    {
      pnmscanner_gettoken (scan, buf, BUFLEN);
      CHECK_FOR_ERROR (pnmscanner_eof (scan), pnminfo->jmpbuf,
                       _("Premature end of file."));

      pnminfo->maxval = g_ascii_isdigit (*buf) ? atoi (buf) : 0;
      CHECK_FOR_ERROR (((pnminfo->maxval<=0) || (pnminfo->maxval>65535)),
                       pnminfo->jmpbuf, _("Unsupported maximum value."));
    }

  /* Create a new image of the proper size and associate the filename with it.
   */
  image_ID = gimp_image_new (pnminfo->xres, pnminfo->yres,
                             (pnminfo->np >= 3) ? GIMP_RGB : GIMP_GRAY);
  gimp_image_set_filename (image_ID, g_file_get_uri (file));

  layer_ID = gimp_layer_new (image_ID, _("Background"),
                             pnminfo->xres, pnminfo->yres,
                             (pnminfo->np >= 3 ?
                              GIMP_RGB_IMAGE : GIMP_GRAY_IMAGE),
                             100, GIMP_NORMAL_MODE);
  gimp_image_insert_layer (image_ID, layer_ID, -1, 0);

  buffer = gimp_drawable_get_buffer (layer_ID);

  pnminfo->loader (scan, pnminfo, buffer);

  /* Destroy the scanner */
  pnmscanner_destroy (scan);

  g_object_unref (buffer);
  g_free (pnminfo);
  g_object_unref (input);

  return image_ID;
}

static void
pnm_load_ascii (PNMScanner *scan,
                PNMInfo    *info,
                GeglBuffer *buffer)
{
  guchar   *data, *d;
  gint      x, y, i, b;
  gint      start, end, scanlines;
  gint      np;
  gchar     buf[BUFLEN];
  gboolean  aborted = FALSE;

  np = (info->np) ? (info->np) : 1;
  /* No overflow as long as gimp_tile_height() < 2730 = 2^(31 - 18) / 3 */
  data = g_new (guchar, gimp_tile_height () * info->xres * np);

  /* Buffer reads to increase performance */
  pnmscanner_createbuffer (scan, 4096);

  for (y = 0; y < info->yres; y += scanlines)
    {
      start = y;
      end   = y + gimp_tile_height ();
      end   = MIN (end, info->yres);

      scanlines = end - start;

      d = data;

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

                switch (info->maxval)
                  {
                  case 255:
                    d[b] = g_ascii_isdigit (*buf) ? atoi (buf) : 0;
                    break;

                  case 1:
                    if (info->np)
                      d[b] = (*buf == '0') ? 0x00 : 0xff;
                    else
                      d[b] = (*buf == '0') ? 0xff : 0x00; /* invert for PBM */
                    break;

                  default:
                    d[b] = (255.0 * (((gdouble) (g_ascii_isdigit (*buf) ?
                                                 atoi (buf) : 0))
                                     / (gdouble) (info->maxval)));
                  }
              }

            d += np;
          }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y, info->xres, scanlines), 0,
                       NULL, data, GEGL_AUTO_ROWSTRIDE);

      gimp_progress_update ((double) y / (double) info->yres);
    }

  g_free (data);

  gimp_progress_update (1.0);
}

static void
pnm_load_raw (PNMScanner *scan,
              PNMInfo    *info,
              GeglBuffer *buffer)
{
  GInputStream *input;
  gint          bpc;
  guchar       *data, *bdata, *d, *b;
  gint          x, y, i;
  gint          start, end, scanlines;

  if (info->maxval > 255)
    bpc = 2;
  else
    bpc = 1;

  data = g_new (guchar, gimp_tile_height () * info->xres * info->np * bpc);

  bdata = NULL;
  if (bpc > 1)
    bdata = g_new (guchar, gimp_tile_height () * info->xres * info->np);

  input = pnmscanner_input (scan);

  for (y = 0; y < info->yres; y += scanlines)
    {
      start = y;
      end = y + gimp_tile_height ();
      end = MIN (end, info->yres);
      scanlines = end - start;
      d = data;
      b = bdata;

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

                  b[x] = MIN (v, info->maxval); /* guard against overflow */
                  b[x] = 255.0 * (gdouble) v / (gdouble) info->maxval;
                }
              b += info->xres * info->np;
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

      if (bpc > 1)
        {
          gegl_buffer_set (buffer,
                           GEGL_RECTANGLE (0, y, info->xres, scanlines), 0,
                           NULL, bdata, GEGL_AUTO_ROWSTRIDE);
        }
      else
        {
          gegl_buffer_set (buffer,
                           GEGL_RECTANGLE (0, y, info->xres, scanlines), 0,
                           NULL, data, GEGL_AUTO_ROWSTRIDE);
        }

      gimp_progress_update ((double) y / (double) info->yres);
    }

  g_free (data);
  g_free (bdata);

  gimp_progress_update (1.0);
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
  data = g_new (guchar, gimp_tile_height () * info->xres);
  buf = g_new (guchar, rowlen);

  for (y = 0; y < info->yres; y += scanlines)
    {
      start = y;
      end = y + gimp_tile_height ();
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

      gimp_progress_update ((double) y / (double) info->yres);
    }

  g_free (buf);
  g_free (data);

  gimp_progress_update (1.0);
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
                    const guchar  *data,
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
                      const guchar  *data,
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
                const guchar  *data,
                GError       **error)
{
  return output_write (ri->output, data, ri->xres * ri->np, error);
}

/* Writes out indexed raw rows */
static gboolean
pnmsaverow_raw_indexed (PNMRowInfo    *ri,
                        const guchar  *data,
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
                  const guchar  *data,
                  GError       **error)
{
  gint   i;
  gchar *rbcur = ri->rowbuf;

  for (i = 0; i < ri->xres * ri->np; i++)
    {
      sprintf ((gchar *) rbcur,"%d\n", 0xff & *(data++));
      rbcur += strlen (rbcur);
    }

  return output_write (ri->output, ri->rowbuf, strlen ((gchar *) ri->rowbuf),
                       error);
}

/* Writes out RGB and grayscale ascii rows */
static gboolean
pnmsaverow_ascii_indexed (PNMRowInfo    *ri,
                          const guchar  *data,
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
save_image (GFile     *file,
            gint32     image_ID,
            gint32     drawable_ID,
            gboolean   pbm,
            GError   **error)
{
  GOutputStream *output;
  GeglBuffer    *buffer;
  const Babl    *format;
  const gchar   *header_string = NULL;
  GimpImageType  drawable_type;
  PNMRowInfo     rowinfo;
  PNMSaverowFunc saverow = NULL;
  guchar         red[256];
  guchar         grn[256];
  guchar         blu[256];
  guchar        *data;
  guchar        *d;
  gchar         *rowbuf;
  gchar          buf[BUFLEN];
  gint           np = 0;
  gint           xres, yres;
  gint           ypos, yend;
  gint           rowbufsize = 0;

  /*  Make sure we're not saving an image with an alpha channel  */
  if (gimp_drawable_has_alpha (drawable_ID))
    {
      g_message (_("Cannot save images with alpha channel."));
      return FALSE;
    }

  gimp_progress_init_printf (_("Saving '%s'"),
                             g_file_get_parse_name (file));

  /* open the file */
  output = G_OUTPUT_STREAM (g_file_replace (file, NULL, FALSE, 0, NULL, error));
  if (! output)
    return FALSE;

  buffer = gimp_drawable_get_buffer (drawable_ID);

  xres = gegl_buffer_get_width  (buffer);
  yres = gegl_buffer_get_height (buffer);

  drawable_type = gimp_drawable_type (drawable_ID);

  /* write out magic number */
  if (!psvals.raw)
    {
      if (pbm)
        {
          header_string = "P1\n";
          format = babl_format ("Y' u8");
          np = 0;
          rowbufsize = xres + (int) (xres / 70) + 1;
          saverow = pnmsaverow_ascii_pbm;
        }
      else
        {
          switch (drawable_type)
            {
            case GIMP_GRAY_IMAGE:
              header_string = "P2\n";
              format = babl_format ("Y' u8");
              np = 1;
              rowbufsize = xres * 4;
              saverow = pnmsaverow_ascii;
              break;

            case GIMP_RGB_IMAGE:
              header_string = "P3\n";
              format = babl_format ("R'G'B' u8");
              np = 3;
              rowbufsize = xres * 12;
              saverow = pnmsaverow_ascii;
              break;

            case GIMP_INDEXED_IMAGE:
              header_string = "P3\n";
              format = gegl_buffer_get_format (buffer);
              np = 1;
              rowbufsize = xres * 12;
              saverow = pnmsaverow_ascii_indexed;
              break;

            default:
              g_warning ("PNM: Unknown drawable_type\n");
              return FALSE;
            }
        }
    }
  else
    {
      if (pbm)
        {
          header_string = "P4\n";
          format = babl_format ("Y' u8");
          np = 0;
          rowbufsize = (gint) ceil ((gdouble) xres / 8.0);
          saverow = pnmsaverow_raw_pbm;
        }
      else
        {
          switch (drawable_type)
            {
            case GIMP_GRAY_IMAGE:
              header_string = "P5\n";
              format = babl_format ("Y' u8");
              np = 1;
              rowbufsize = xres;
              saverow = pnmsaverow_raw;
              break;

            case GIMP_RGB_IMAGE:
              header_string = "P6\n";
              format = babl_format ("R'G'B' u8");
              np = 3;
              rowbufsize = xres * 3;
              saverow = pnmsaverow_raw;
              break;

            case GIMP_INDEXED_IMAGE:
              header_string = "P6\n";
              format = gegl_buffer_get_format (buffer);
              np = 1;
              rowbufsize = xres * 3;
              saverow = pnmsaverow_raw_indexed;
              break;

            default:
              g_warning ("PNM: Unknown drawable_type\n");
              return FALSE;
            }
        }
    }

  if (! output_write (output, header_string, strlen (header_string), error))
    {
      g_object_unref (output);
      g_object_unref (buffer);
      return FALSE;
    }

  rowinfo.zero_is_black = FALSE;

  if (drawable_type == GIMP_INDEXED_IMAGE)
    {
      guchar *cmap;
      gint    num_colors;

      cmap = gimp_image_get_colormap (image_ID, &num_colors);

      if (pbm)
        {
          /*  Test which of the two colors is white and which is black  */
          switch (num_colors)
            {
            case 1:
              rowinfo.zero_is_black = (GIMP_RGB_LUMINANCE (cmap[0],
                                                           cmap[1],
                                                           cmap[2]) < 128);
              break;

            case 2:
              rowinfo.zero_is_black = (GIMP_RGB_LUMINANCE (cmap[0],
                                                           cmap[1],
                                                           cmap[2]) <
                                       GIMP_RGB_LUMINANCE (cmap[3],
                                                           cmap[4],
                                                           cmap[5]));
              break;

            default:
              g_warning ("Images saved as PBM should be black/white");
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

  /* write out comment string */
  if (! output_write (output, SAVE_COMMENT_STRING, strlen (SAVE_COMMENT_STRING),
                      error))
    {
      g_object_unref (output);
      g_object_unref (buffer);
      return FALSE;
    }

  /* write out resolution and maxval */
  if (pbm)
    g_snprintf (buf, sizeof (buf), "%d %d\n", xres, yres);
  else
    g_snprintf (buf, sizeof (buf), "%d %d\n255\n", xres, yres);

  if (! output_write (output, buf, strlen (buf), error))
    {
      g_object_unref (output);
      g_object_unref (buffer);
      return FALSE;
    }

  /* allocate a buffer for retrieving information from the pixel region  */
  data = g_new (guchar,
                gimp_tile_height () * xres *
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
      if ((ypos % gimp_tile_height ()) == 0)
        {
          yend = ypos + gimp_tile_height ();
          yend = MIN (yend, yres);

          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (0, ypos, xres, yend - ypos), 1.0,
                           NULL, data,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          d = data;
        }

      if (! saverow (&rowinfo, d, error))
        {
          g_free (rowbuf);
          g_free (data);
          g_object_unref (output);
          g_object_unref (buffer);
          return FALSE;
        }

      d += xres * (np ? np : 1);

      if ((ypos % 20) == 0)
        gimp_progress_update ((double) ypos / (double) yres);
    }

  g_free (rowbuf);
  g_free (data);

  g_object_unref (buffer);
  g_object_unref (output);

  gimp_progress_update (1.0);

  return TRUE;
}

static gboolean
save_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  gboolean   run;

  dialog = gimp_export_dialog_new (_("PNM"), PLUG_IN_BINARY, PNM_SAVE_PROC);

  /*  file save type  */
  frame = gimp_int_radio_group_new (TRUE, _("Data formatting"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &psvals.raw, psvals.raw,

                                    _("Raw"),   TRUE,  NULL,
                                    _("ASCII"), FALSE, NULL,

                                    NULL);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

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

  while (!(s->eof) && (state != -1))
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
