/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * PostScript file plugin
 * PostScript writing and GhostScript interfacing code
 * Copyright (C) 1997-98 Peter Kirchgessner
 * (email: peter@kirchgessner.net, WWW: http://www.kirchgessner.net)
 *
 * Added controls for TextAlphaBits and GraphicsAlphaBits
 *   George White <aa056@chebucto.ns.ca>
 *
 * Added Ascii85 encoding
 *   Austin Donnelly <austin@gimp.org>
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
 *
 */

/* Event history:
 * V 0.90, PK, 28-Mar-97: Creation.
 * V 0.91, PK, 03-Apr-97: Clip everything outside BoundingBox.
 *             24-Apr-97: Multi page read support.
 * V 1.00, PK, 30-Apr-97: PDF support.
 * V 1.01, PK, 05-Oct-97: Parse rc-file.
 * V 1.02, GW, 09-Oct-97: Antialiasing support.
 *         PK, 11-Oct-97: No progress bars when running non-interactive.
 *                        New procedure file_ps_load_setargs to set
 *                        load-arguments non-interactively.
 *                        If GS_OPTIONS are not set, use at least "-dSAFER"
 * V 1.03, nn, 20-Dec-97: Initialize some variables
 * V 1.04, PK, 20-Dec-97: Add Encapsulated PostScript output and preview
 * V 1.05, PK, 21-Sep-98: Write b/w-images (indexed) using image-operator
 * V 1.06, PK, 22-Dec-98: Fix problem with writing color PS files.
 *                        Ghostview may hang when displaying the files.
 * V 1.07, PK, 14-Sep-99: Add resolution to image
 * V 1.08, PK, 16-Jan-2000: Add PostScript-Level 2 by Austin Donnelly
 * V 1.09, PK, 15-Feb-2000: Force showpage on EPS-files
 *                          Add "RunLength" compression
 *                          Fix problem with "Level 2" toggle
 * V 1.10, PK, 15-Mar-2000: For load EPSF, allow negative Bounding Box Values
 *                          Save PS: don't start lines of image data with %%
 *                          to prevent problems with stupid PostScript
 *                          analyzer programs (Stanislav Brabec)
 *                          Add BeginData/EndData comments
 *                          Save PS: Set default rotation to 0
 * V 1.11, PK, 20-Aug-2000: Fix problem with BoundingBox recognition
 *                          for Mac files.
 *                          Fix problem with loop when reading not all
 *                          images of a multi page file.
 *         PK, 31-Aug-2000: Load PS: Add checks for space in filename.
 * V 1.12  PK, 19-Jun-2001: Fix problem with command line switch --
 *                          (reported by Ferenc Wagner)
 * V 1.13  PK, 07-Apr-2002: Fix problem with DOS binary EPS files
 * V 1.14  PK, 14-May-2002: Workaround EPS files of Adb. Ill. 8.0
 * V 1.15  PK, 04-Oct-2002: Be more accurate with using BoundingBox
 * V 1.16  PK, 22-Jan-2004: Don't use popen(), use g_spawn_async_with_pipes()
 *                          or g_spawn_sync().
 * V 1.17  PK, 19-Sep-2004: Fix problem with interpretation of bounding box
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include <ghostscript/ierrors.h>
#include <ghostscript/iapi.h>
#include <ghostscript/gdevdsp.h>

#define VERSIO 1.17
static const gchar dversio[] = "v1.17  19-Sep-2004";

#define LOAD_PS_PROC         "file-ps-load"
#define LOAD_EPS_PROC        "file-eps-load"
#define LOAD_PS_SETARGS_PROC "file-ps-load-setargs"
#define LOAD_PS_THUMB_PROC   "file-ps-load-thumb"
#define SAVE_PS_PROC         "file-ps-save"
#define SAVE_EPS_PROC        "file-eps-save"
#define PLUG_IN_BINARY       "file-ps"
#define PLUG_IN_ROLE         "gimp-file-ps"


#define STR_LENGTH     64
#define MIN_RESOLUTION 5
#define MAX_RESOLUTION 8192

/* Load info */
typedef struct
{
  guint     resolution;        /* resolution (dpi) at which to run ghostscript */
  guint     width, height;     /* desired size (ghostscript may ignore this) */
  gboolean  use_bbox;          /* 0: use width/height, 1: try to use BoundingBox */
  gchar     pages[STR_LENGTH]; /* Pages to load (eg.: 1,3,5-7) */
  gint      pnm_type;          /* 4: pbm, 5: pgm, 6: ppm, 7: automatic */
  gint      textalpha;         /* antialiasing: 1,2, or 4 TextAlphaBits */
  gint      graphicsalpha;     /* antialiasing: 1,2, or 4 GraphicsAlphaBits */
} PSLoadVals;

static PSLoadVals plvals =
{
  100,         /* 100 dpi                        */
  826, 1170,   /* default width/height (A4)      */
  TRUE,        /* try to use BoundingBox         */
  "1",         /* pages to load                  */
  6,           /* use ppm (color)               */
  1,           /* don't use text antialiasing     */
  1            /* don't use graphics antialiasing */
};

/* Widgets for width and height of PostScript image to
*  be loaded, so that they can be updated when desired resolution is
*  changed
*/
static GtkWidget *ps_width_spinbutton;
static GtkWidget *ps_height_spinbutton;

/* Save info  */
typedef struct
{
  gdouble    width, height;      /* Size of image */
  gdouble    x_offset, y_offset; /* Offset to image on page */
  gboolean   unit_mm;            /* Unit of measure (0: inch, 1: mm) */
  gboolean   keep_ratio;         /* Keep aspect ratio */
  gint       rotate;             /* Rotation (0, 90, 180, 270) */
  gint       level;              /* PostScript Level */
  gboolean   eps;                /* Encapsulated PostScript flag */
  gboolean   preview;            /* Preview Flag */
  gint       preview_size;       /* Preview size */
} PSSaveVals;

static PSSaveVals psvals =
{
  287.0, 200.0,   /* Image size (A4) */
  5.0, 5.0,       /* Offset */
  TRUE,           /* Unit is mm */
  TRUE,           /* Keep edge ratio */
  0,              /* Rotate */
  2,              /* PostScript Level */
  FALSE,          /* Encapsulated PostScript flag */
  FALSE,          /* Preview flag */
  256             /* Preview size */
};

static const char hex[] = "0123456789abcdef";


/* Declare some local functions.
 */
static void      query            (void);
static void      run              (const gchar       *name,
                                   gint               nparams,
                                   const GimpParam   *param,
                                   gint              *nreturn_vals,
                                   GimpParam        **return_vals);

static gint32    load_image       (const gchar       *filename,
                                   GError           **error);
static gboolean  save_image       (GFile             *file,
                                   gint32             image_ID,
                                   gint32             drawable_ID,
                                   GError           **error);

static gboolean  save_ps_header   (GOutputStream     *output,
                                   GFile             *file,
                                   GError           **error);
static gboolean  save_ps_setup    (GOutputStream     *output,
                                   gint32             drawable_ID,
                                   gint               width,
                                   gint               height,
                                   gint               bpp,
                                   GError           **error);
static gboolean  save_ps_trailer  (GOutputStream     *output,
                                   GError           **error);

static gboolean  save_ps_preview  (GOutputStream     *output,
                                   gint32             drawable_ID,
                                   GError           **error);

static gboolean  save_gray        (GOutputStream     *output,
                                   gint32             image_ID,
                                   gint32             drawable_ID,
                                   GError           **error);
static gboolean  save_bw          (GOutputStream     *output,
                                   gint32             image_ID,
                                   gint32             drawable_ID,
                                   GError           **error);
static gboolean  save_index       (GOutputStream     *output,
                                   gint32             image_ID,
                                   gint32             drawable_ID,
                                   GError           **error);
static gboolean  save_rgb         (GOutputStream     *output,
                                   gint32             image_ID,
                                   gint32             drawable_ID,
                                   GError           **error);

static gboolean  print            (GOutputStream     *output,
                                   GError           **error,
                                   const gchar       *format,
                                   ...) G_GNUC_PRINTF (3, 4);

static gint32    create_new_image (const gchar       *filename,
                                   guint              pagenum,
                                   guint              width,
                                   guint              height,
                                   GimpImageBaseType  type,
                                   gint32            *layer_ID);

static void      check_load_vals  (void);
static void      check_save_vals  (void);

static gint      page_in_list     (gchar             *list,
                                   guint              pagenum);

static gint      get_bbox         (const gchar       *filename,
                                   gint              *x0,
                                   gint              *y0,
                                   gint              *x1,
                                   gint              *y1);

static FILE    * ps_open         (const gchar       *filename,
                                  const PSLoadVals  *loadopt,
                                  gint              *llx,
                                  gint              *lly,
                                  gint              *urx,
                                  gint              *ury,
                                  gboolean          *is_epsf);

static void      ps_close         (FILE              *ifp);

static gboolean  skip_ps          (FILE              *ifp);

static gint32    load_ps          (const gchar       *filename,
                                   guint              pagenum,
                                   FILE              *ifp,
                                   gint               llx,
                                   gint               lly,
                                   gint               urx,
                                   gint               ury);

static void      dither_grey      (const guchar      *grey,
                                   guchar            *bw,
                                   gint               npix,
                                   gint               linecount);


/* Dialog-handling */

static gint32    count_ps_pages             (const gchar *filename);
static gboolean  load_dialog                (const gchar *filename);
static void      load_pages_entry_callback  (GtkWidget   *widget,
                                             gpointer     data);

static gboolean  resolution_change_callback (GtkAdjustment *adjustment,
                                             gpointer   data);

typedef struct
{
  GtkAdjustment *adjustment[4];
  gint           level;
} SaveDialogVals;

static gboolean  save_dialog              (void);
static void      save_unit_toggle_update  (GtkWidget *widget,
                                           gpointer   data);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


/* The run mode */
static GimpRunMode l_run_mode;

static void compress_packbits (int            nin,
                               unsigned char *src,
                               int           *nout,
                               unsigned char *dst);


static guint32  ascii85_buf       = 0;
static gint     ascii85_len       = 0;
static gint     ascii85_linewidth = 0;

static GimpPageSelectorTarget ps_pagemode = GIMP_PAGE_SELECTOR_TARGET_LAYERS;

static void
ascii85_init (void)
{
  ascii85_len       = 0;
  ascii85_linewidth = 0;
}

static gboolean
ascii85_flush (GOutputStream  *output,
               GError        **error)
{
  gchar    c[5];
  gint     i;
  gboolean zero_case = (ascii85_buf == 0);
  GString  *string   = g_string_new (NULL);

  static gint max_linewidth = 75;

  for (i = 4; i >= 0; i--)
    {
      c[i] = (ascii85_buf % 85) + '!';
      ascii85_buf /= 85;
    }

  /* check for special case: "!!!!!" becomes "z", but only if not
   * at end of data. */
  if (zero_case && (ascii85_len == 4))
    {
      if (ascii85_linewidth >= max_linewidth)
        {
          g_string_append_c (string, '\n');

          ascii85_linewidth = 0;
        }

      g_string_append_c (string, 'z');

      ascii85_linewidth++;
    }
  else
    {
      for (i = 0; i < ascii85_len + 1; i++)
        {
          if ((ascii85_linewidth >= max_linewidth) && (c[i] != '%'))
            {
              g_string_append_c (string, '\n');

              ascii85_linewidth = 0;
            }

          g_string_append_c (string, c[i]);

          ascii85_linewidth++;
        }
    }

  ascii85_len = 0;
  ascii85_buf = 0;

  if (string->len > 0 &&
      ! g_output_stream_write_all (output,
                                   string->str, string->len, NULL,
                                   NULL, error))
    {
      g_string_free (string, TRUE);

      return FALSE;
    }

  g_string_free (string, TRUE);

  return TRUE;
}

static inline gboolean
ascii85_out (GOutputStream  *output,
             guchar          byte,
             GError        **error)
{
  if (ascii85_len == 4)
    if (! ascii85_flush (output, error))
      return FALSE;

  ascii85_buf <<= 8;
  ascii85_buf |= byte;
  ascii85_len++;

  return TRUE;
}

static gboolean
ascii85_nout (GOutputStream  *output,
              gint            n,
              guchar         *uptr,
              GError        **error)
{
  while (n-- > 0)
    {
      if (! ascii85_out (output, *uptr, error))
        return FALSE;

      uptr++;
    }

  return TRUE;
}

static gboolean
ascii85_done (GOutputStream  *output,
              GError        **error)
{
  if (ascii85_len)
    {
      /* zero any unfilled buffer portion, then flush */
      ascii85_buf <<= (8 * (4 - ascii85_len));

      if (! ascii85_flush (output, error))
        return FALSE;
    }

  if (! print (output, error, "~>\n"))
    return FALSE;

  return TRUE;
}


static void
compress_packbits (int nin,
                   unsigned char *src,
                   int *nout,
                   unsigned char *dst)

{
 unsigned char c;
 int nrepeat, nliteral;
 unsigned char *run_start;
 unsigned char *start_dst = dst;
 unsigned char *last_literal = NULL;

 for (;;)
 {
   if (nin <= 0) break;

   run_start = src;
   c = *run_start;

   /* Search repeat bytes */
   if ((nin > 1) && (c == src[1]))
   {
     nrepeat = 1;
     nin -= 2;
     src += 2;
     while ((nin > 0) && (c == *src))
     {
       nrepeat++;
       src++;
       nin--;
       if (nrepeat == 127) break; /* Maximum repeat */
     }

     /* Add two-byte repeat to last literal run ? */
     if (   (nrepeat == 1)
         && (last_literal != NULL) && (((*last_literal)+1)+2 <= 128))
     {
       *last_literal += 2;
       *(dst++) = c;
       *(dst++) = c;
       continue;
     }

     /* Add repeat run */
     *(dst++) = (unsigned char)((-nrepeat) & 0xff);
     *(dst++) = c;
     last_literal = NULL;
     continue;
   }
   /* Search literal bytes */
   nliteral = 1;
   nin--;
   src++;

   for (;;)
   {
     if (nin <= 0) break;

     if ((nin >= 2) && (src[0] == src[1])) /* A two byte repeat ? */
       break;

     nliteral++;
     nin--;
     src++;
     if (nliteral == 128) break; /* Maximum literal run */
   }

   /* Could be added to last literal run ? */
   if ((last_literal != NULL) && (((*last_literal)+1)+nliteral <= 128))
   {
     *last_literal += nliteral;
   }
   else
   {
     last_literal = dst;
     *(dst++) = (unsigned char)(nliteral-1);
   }
   while (nliteral-- > 0) *(dst++) = *(run_start++);
 }
 *nout = dst - start_dst;
}


typedef struct
{
  goffset eol;
  goffset begin_data;
} PS_DATA_POS;

static PS_DATA_POS ps_data_pos = { 0, 0 };

static gboolean
ps_begin_data (GOutputStream  *output,
               GError        **error)
{
  /*                                 %%BeginData: 123456789012 ASCII Bytes */
  if (! print (output, error, "%s", "%%BeginData:                         "))
    return FALSE;

  ps_data_pos.eol = g_seekable_tell (G_SEEKABLE (output));

  if (! print (output, error, "\n"))
    return FALSE;

  ps_data_pos.begin_data = g_seekable_tell (G_SEEKABLE (output));

  return TRUE;
}

static gboolean
ps_end_data (GOutputStream  *output,
             GError        **error)
{
  goffset end_data;
  gchar   s[64];

  if ((ps_data_pos.begin_data > 0) && (ps_data_pos.eol > 0))
    {
      end_data = g_seekable_tell (G_SEEKABLE (output));

      if (end_data > 0)
        {
          g_snprintf (s, sizeof (s),
                      "%ld ASCII Bytes", end_data - ps_data_pos.begin_data);

          if (! g_seekable_seek (G_SEEKABLE (output),
                                 ps_data_pos.eol - strlen (s), G_SEEK_SET,
                                 NULL, error))
            return FALSE;

          if (! print (output, error, "%s", s))
            return FALSE;

          if (! g_seekable_seek (G_SEEKABLE (output),
                                 end_data, G_SEEK_SET,
                                 NULL, error))
            return FALSE;
        }
    }

  if (! print (output, error, "%s\n", "%%EndData"))
    return FALSE;

  return TRUE;
}


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

  static const GimpParamDef set_load_args[] =
  {
    { GIMP_PDB_INT32,  "resolution", "Resolution to interpret image (dpi)"     },
    { GIMP_PDB_INT32,  "width",      "Desired width"                           },
    { GIMP_PDB_INT32,  "height",     "Desired height"                          },
    { GIMP_PDB_INT32,  "check-bbox", "0: Use width/height, 1: Use BoundingBox" },
    { GIMP_PDB_STRING, "pages",      "Pages to load (e.g.: 1,3,5-7)"           },
    { GIMP_PDB_INT32,  "coloring",   "4: b/w, 5: grey, 6: color image, 7: automatic" },
    { GIMP_PDB_INT32,  "text-alpha-bits",    "1, 2, or 4" },
    { GIMP_PDB_INT32,  "graphic-alpha-bits", "1, 2, or 4" }
  };

  static const GimpParamDef thumb_args[] =
  {
    { GIMP_PDB_STRING, "filename",     "The name of the file to load"  },
    { GIMP_PDB_INT32,  "thumb-size",   "Preferred thumbnail size"      }
  };
  static const GimpParamDef thumb_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image",         "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to export" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to export the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to export the image in" },
    { GIMP_PDB_FLOAT,    "width",        "Width of the image in PostScript file (0: use input image size)" },
    { GIMP_PDB_FLOAT,    "height",       "Height of image in PostScript file (0: use input image size)" },
    { GIMP_PDB_FLOAT,    "x-offset",     "X-offset to image from lower left corner" },
    { GIMP_PDB_FLOAT,    "y-offset",     "Y-offset to image from lower left corner" },
    { GIMP_PDB_INT32,    "unit",         "Unit for width/height/offset. 0: inches, 1: millimeters" },
    { GIMP_PDB_INT32,    "keep-ratio",   "0: use width/height, 1: keep aspect ratio" },
    { GIMP_PDB_INT32,    "rotation",     "0, 90, 180, 270" },
    { GIMP_PDB_INT32,    "eps-flag",     "0: PostScript, 1: Encapsulated PostScript" },
    { GIMP_PDB_INT32,    "preview",      "0: no preview, >0: max. size of preview" },
    { GIMP_PDB_INT32,    "level",        "1: PostScript Level 1, 2: PostScript Level 2" }
  };

  gimp_install_procedure (LOAD_PS_PROC,
                          "load PostScript documents",
                          "load PostScript documents",
                          "Peter Kirchgessner <peter@kirchgessner.net>",
                          "Peter Kirchgessner",
                          dversio,
                          N_("PostScript document"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PS_PROC, "application/postscript");
  gimp_register_magic_load_handler (LOAD_PS_PROC,
                                    "ps",
                                    "",
                                    "0,string,%!,0,long,0xc5d0d3c6");

  gimp_install_procedure (LOAD_EPS_PROC,
                          "load Encapsulated PostScript images",
                          "load Encapsulated PostScript images",
                          "Peter Kirchgessner <peter@kirchgessner.net>",
                          "Peter Kirchgessner",
                          dversio,
                          N_("Encapsulated PostScript image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_EPS_PROC, "image/x-eps");
  gimp_register_magic_load_handler (LOAD_EPS_PROC,
                                    "eps",
                                    "",
                                    "0,string,%!,0,long,0xc5d0d3c6");

  gimp_install_procedure (LOAD_PS_SETARGS_PROC,
                          "set additional parameters for procedure file-ps-load",
                          "set additional parameters for procedure file-ps-load",
                          "Peter Kirchgessner <peter@kirchgessner.net>",
                          "Peter Kirchgessner",
                          dversio,
                          NULL,
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (set_load_args), 0,
                          set_load_args, NULL);

  gimp_install_procedure (LOAD_PS_THUMB_PROC,
                          "Loads a small preview from a PostScript or PDF document",
                          "",
                          "Peter Kirchgessner <peter@kirchgessner.net>",
                          "Peter Kirchgessner",
                          dversio,
                          NULL,
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (thumb_args),
                          G_N_ELEMENTS (thumb_return_vals),
                          thumb_args, thumb_return_vals);

  gimp_register_thumbnail_loader (LOAD_PS_PROC,  LOAD_PS_THUMB_PROC);
  gimp_register_thumbnail_loader (LOAD_EPS_PROC, LOAD_PS_THUMB_PROC);

  gimp_install_procedure (SAVE_PS_PROC,
                          "export image as PostScript document",
                          "PostScript exporting handles all image types except "
                          "those with alpha channels.",
                          "Peter Kirchgessner <peter@kirchgessner.net>",
                          "Peter Kirchgessner",
                          dversio,
                          N_("PostScript document"),
                          "RGB, GRAY, INDEXED",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PS_PROC, "application/postscript");
  gimp_register_file_handler_uri (SAVE_PS_PROC);
  gimp_register_save_handler (SAVE_PS_PROC, "ps", "");

  gimp_install_procedure (SAVE_EPS_PROC,
                          "export image as Encapsulated PostScript image",
                          "PostScript exporting handles all image types except "
                          "those with alpha channels.",
                          "Peter Kirchgessner <peter@kirchgessner.net>",
                          "Peter Kirchgessner",
                          dversio,
                          N_("Encapsulated PostScript image"),
                          "RGB, GRAY, INDEXED",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_EPS_PROC, "application/x-eps");
  gimp_register_file_handler_uri (SAVE_EPS_PROC);
  gimp_register_save_handler (SAVE_EPS_PROC, "eps", "");
}

static void
ps_set_save_size (PSSaveVals *vals,
                  gint32      image_ID)
{
  gdouble  xres, yres, factor, iw, ih;
  guint    width, height;
  GimpUnit unit;

  gimp_image_get_resolution (image_ID, &xres, &yres);

  if ((xres < 1e-5) || (yres < 1e-5))
    xres = yres = 72.0;

  /* Calculate size of image in inches */
  width  = gimp_image_width (image_ID);
  height = gimp_image_height (image_ID);
  iw = width  / xres;
  ih = height / yres;

  unit = gimp_image_get_unit (image_ID);
  factor = gimp_unit_get_factor (unit);

  if (factor == 0.0254 ||
      factor == 0.254 ||
      factor == 2.54 ||
      factor == 25.4)
    {
      vals->unit_mm = TRUE;
    }

  if (vals->unit_mm)
    {
      iw *= 25.4;
      ih *= 25.4;
    }

  vals->width  = iw;
  vals->height = ih;
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
  GimpPDBStatusType  status        = GIMP_PDB_SUCCESS;
  gint32             image_ID      = -1;
  gint32             drawable_ID   = -1;
  gint32             orig_image_ID = -1;
  GimpExportReturn   export        = GIMP_EXPORT_CANCEL;
  GError            *error         = NULL;

  l_run_mode = run_mode = param[0].data.d_int32;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PS_PROC)  == 0 ||
      strcmp (name, LOAD_EPS_PROC) == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data (LOAD_PS_PROC, &plvals);

          if (! load_dialog (param[1].data.d_string))
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 3)
            status = GIMP_PDB_CALLING_ERROR;
          else    /* Get additional interpretation arguments */
            gimp_get_data (LOAD_PS_PROC, &plvals);
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /* Possibly retrieve data */
          gimp_get_data (LOAD_PS_PROC, &plvals);
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          check_load_vals ();
          image_ID = load_image (param[1].data.d_string, &error);

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

      /*  Store plvals data  */
      if (status == GIMP_PDB_SUCCESS)
        gimp_set_data (LOAD_PS_PROC, &plvals, sizeof (PSLoadVals));
    }
  else if (strcmp (name, LOAD_PS_THUMB_PROC) == 0)
    {
      if (nparams < 2)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          gint size = param[1].data.d_int32;

          /*  We should look for an embedded preview but for now we
           *  just load the document at a small resolution and the
           *  first page only.
           */

          plvals.resolution = size / 4;
          plvals.width      = size;
          plvals.height     = size;
          strcpy (plvals.pages, "1");

          check_load_vals ();
          image_ID = load_image (param[0].data.d_string, &error);

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
    }
  else if (strcmp (name, SAVE_PS_PROC)  == 0 ||
           strcmp (name, SAVE_EPS_PROC) == 0)
    {
      psvals.eps = strcmp (name, SAVE_PS_PROC);

      image_ID    = orig_image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /* eventually export the image */
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_ID, &drawable_ID,
                                      psvals.eps ? "EPS" : "PostScript",
                                      GIMP_EXPORT_CAN_HANDLE_RGB  |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED);

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

          ps_set_save_size (&psvals, orig_image_ID);

          /*  First acquire information with a dialog  */
          if (! save_dialog ())
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 15)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              psvals.width        = param[5].data.d_float;
              psvals.height       = param[6].data.d_float;
              psvals.x_offset     = param[7].data.d_float;
              psvals.y_offset     = param[8].data.d_float;
              psvals.unit_mm      = (param[9].data.d_int32 != 0);
              psvals.keep_ratio   = (param[10].data.d_int32 != 0);
              psvals.rotate       = param[11].data.d_int32;
              psvals.eps          = (param[12].data.d_int32 != 0);
              psvals.preview      = (param[13].data.d_int32 != 0);
              psvals.preview_size = param[13].data.d_int32;
              psvals.level        = param[14].data.d_int32;
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
          if ((psvals.width == 0.0) || (psvals.height == 0.0))
            ps_set_save_size (&psvals, orig_image_ID);

          check_save_vals ();

          if (save_image (g_file_new_for_uri (param[3].data.d_string),
                          image_ID, drawable_ID,
                          &error))
            {
              /*  Store psvals data  */
              gimp_set_data (name, &psvals, sizeof (PSSaveVals));
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }
  else if (strcmp (name, LOAD_PS_SETARGS_PROC) == 0)
    {
      /*  Make sure all the arguments are there!  */
      if (nparams != 8)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          plvals.resolution = param[0].data.d_int32;
          plvals.width      = param[1].data.d_int32;
          plvals.height     = param[2].data.d_int32;
          plvals.use_bbox   = param[3].data.d_int32;
          if (param[4].data.d_string != NULL)
            strncpy (plvals.pages, param[4].data.d_string,
                     sizeof (plvals.pages));
          else
            plvals.pages[0] = '\0';
          plvals.pages[sizeof (plvals.pages) - 1] = '\0';
          plvals.pnm_type      = param[5].data.d_int32;
          plvals.textalpha     = param[6].data.d_int32;
          plvals.graphicsalpha = param[7].data.d_int32;
          check_load_vals ();

          gimp_set_data (LOAD_PS_PROC, &plvals, sizeof (PSLoadVals));
        }
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
load_image (const gchar  *filename,
            GError      **error)
{
  gint32    image_ID = 0;
  gint32   *image_list, *nl;
  guint     page_count;
  FILE     *ifp;
  gchar    *temp;
  gint      llx, lly, urx, ury;
  gint      k, n_images, max_images, max_pagenum;
  gboolean  is_epsf;

#ifdef PS_DEBUG
  g_print ("load_image:\n resolution = %d\n", plvals.resolution);
  g_print (" %dx%d pixels\n", plvals.width, plvals.height);
  g_print (" BoundingBox: %d\n", plvals.use_bbox);
  g_print (" Coloring: %d\n", plvals.pnm_type);
  g_print (" TextAlphaBits: %d\n", plvals.textalpha);
  g_print (" GraphicsAlphaBits: %d\n", plvals.graphicsalpha);
#endif

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  /* Try to see if PostScript file is available */
  ifp = g_fopen (filename, "r");
  if (ifp == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }
  fclose (ifp);

  ifp = ps_open (filename, &plvals, &llx, &lly, &urx, &ury, &is_epsf);
  if (!ifp)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR,
                   _("Could not interpret PostScript file '%s'"),
                   gimp_filename_to_utf8 (filename));
      return -1;
    }

  image_list = g_new (gint32, 10);
  n_images = 0;
  max_images = 10;

  max_pagenum = 9999;  /* Try to get the maximum pagenumber to read */
  if (is_epsf)
    max_pagenum = 1;

  if (!page_in_list (plvals.pages, max_pagenum)) /* Is there a limit in list ? */
    {
      max_pagenum = -1;
      for (temp = plvals.pages; *temp != '\0'; temp++)
        {
          if ((*temp < '0') || (*temp > '9'))
            continue; /* Search next digit */
          sscanf (temp, "%d", &k);
          if (k > max_pagenum)
            max_pagenum = k;
          while ((*temp >= '0') && (*temp <= '9'))
            temp++;
          temp--;
        }

      if (max_pagenum < 1)
        max_pagenum = 9999;
    }

  /* Load all images */
  for (page_count = 1; page_count <= max_pagenum; page_count++)
    {
      if (page_in_list (plvals.pages, page_count))
        {
          image_ID = load_ps (filename, page_count, ifp, llx, lly, urx, ury);
          if (image_ID == -1)
            break;

          gimp_image_set_resolution (image_ID,
                                     (gdouble) plvals.resolution,
                                     (gdouble) plvals.resolution);

          if (n_images == max_images)
            {
              nl = (gint32 *) g_realloc (image_list,
                                         (max_images+10)*sizeof (gint32));
              if (nl == NULL) break;
              image_list = nl;
              max_images += 10;
            }
          image_list[n_images++] = image_ID;
        }
      else  /* Skip an image */
        {
          image_ID = -1;
          if (! skip_ps (ifp))
            break;
        }
    }

  ps_close (ifp);

  if (ps_pagemode == GIMP_PAGE_SELECTOR_TARGET_LAYERS)
    {
      for (k = 0; k < n_images; k++)
        {
          gchar *name;

          if (k == 0)
            {
              image_ID = image_list[0];

              name = g_strdup_printf (_("%s-pages"), filename);
              gimp_image_set_filename (image_ID, name);
              g_free (name);
            }
          else
            {
              gint32 current_layer;
              gint32 tmp_ID;

              tmp_ID = gimp_image_get_active_drawable (image_list[k]);

              name = gimp_item_get_name (tmp_ID);

              current_layer = gimp_layer_new_from_drawable (tmp_ID, image_ID);
              gimp_item_set_name (current_layer, name);
              gimp_image_insert_layer (image_ID, current_layer, -1, -1);
              gimp_image_delete (image_list[k]);

              g_free (name);
            }
        }

      gimp_image_undo_enable (image_ID);
    }
  else
    {
      /* Display images in reverse order.
       * The last will be displayed by GIMP itself
       */
      for (k = n_images - 1; k >= 0; k--)
        {
          gimp_image_undo_enable (image_list[k]);
          gimp_image_clean_all (image_list[k]);

          if (l_run_mode != GIMP_RUN_NONINTERACTIVE && k > 0)
            gimp_display_new (image_list[k]);
        }

      image_ID = (n_images > 0) ? image_list[0] : -1;
    }

  g_free (image_list);

  return image_ID;
}


static gboolean
save_image (GFile   *file,
            gint32   image_ID,
            gint32   drawable_ID,
            GError **error)
{
  GOutputStream *output;
  GimpImageType  drawable_type;

  drawable_type = gimp_drawable_type (drawable_ID);

  /*  Make sure we're not exporting an image with an alpha channel  */
  if (gimp_drawable_has_alpha (drawable_ID))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("PostScript export cannot handle images with alpha channels"));
      return FALSE;
    }

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
    case GIMP_GRAY_IMAGE:
    case GIMP_RGB_IMAGE:
      break;

    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Cannot operate on unknown image types."));
      return FALSE;
      break;
    }

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (output)
    {
      GOutputStream *buffered;

      buffered = g_buffered_output_stream_new (output);
      g_object_unref (output);

      output = buffered;
    }
  else
    {
      return FALSE;
    }

  if (! save_ps_header (output, file, error))
    goto fail;

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
      if (! save_index (output, image_ID, drawable_ID, error))
        goto fail;
      break;

    case GIMP_GRAY_IMAGE:
      if (! save_gray (output, image_ID, drawable_ID, error))
        goto fail;
      break;

    case GIMP_RGB_IMAGE:
      if (! save_rgb (output, image_ID, drawable_ID, error))
        goto fail;
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  if (! save_ps_trailer (output, error))
    goto fail;

  if (! g_output_stream_close (output, NULL, error))
    goto fail;

  g_object_unref (output);

  return TRUE;

 fail:

  g_object_unref (output);

  return FALSE;
}


/* Check (and correct) the load values plvals */
static void
check_load_vals (void)
{
  if (plvals.resolution < MIN_RESOLUTION)
    plvals.resolution = MIN_RESOLUTION;
  else if (plvals.resolution > MAX_RESOLUTION)
    plvals.resolution = MAX_RESOLUTION;

  if (plvals.width < 2)
    plvals.width = 2;
  if (plvals.height < 2)
    plvals.height = 2;
  plvals.use_bbox = (plvals.use_bbox != 0);
  if (plvals.pages[0] == '\0')
    strcpy (plvals.pages, "1-99");
  if ((plvals.pnm_type < 4) || (plvals.pnm_type > 7))
    plvals.pnm_type = 6;
  if (   (plvals.textalpha != 1) && (plvals.textalpha != 2)
      && (plvals.textalpha != 4))
    plvals.textalpha = 1;
  if (   (plvals.graphicsalpha != 1) && (plvals.graphicsalpha != 2)
      && (plvals.graphicsalpha != 4))
    plvals.graphicsalpha = 1;
}


/* Check (and correct) the save values psvals */
static void
check_save_vals (void)
{
  int i;

  i = psvals.rotate;
  if ((i != 0) && (i != 90) && (i != 180) && (i != 270))
    psvals.rotate = 90;
  if (psvals.preview_size <= 0)
    psvals.preview = FALSE;
}


/* Check if a page is in a given list */
static gint
page_in_list (gchar *list,
              guint  page_num)
{
  char tmplist[STR_LENGTH], *c0, *c1;
  int state, start_num, end_num;
#define READ_STARTNUM  0
#define READ_ENDNUM    1
#define CHK_LIST(a,b,c) {int low=(a),high=(b),swp; \
  if ((low>0) && (high>0)) { \
  if (low>high) {swp=low; low=high; high=swp;} \
  if ((low<=(c))&&(high>=(c))) return (1); } }

  if ((list == NULL) || (*list == '\0'))
    return 1;

  strncpy (tmplist, list, STR_LENGTH);
  tmplist[STR_LENGTH-1] = '\0';

  c0 = c1 = tmplist;
  while (*c1)    /* Remove all whitespace and break on unsupported characters */
    {
      if ((*c1 >= '0') && (*c1 <= '9'))
        {
          *(c0++) = *c1;
        }
      else if ((*c1 == '-') || (*c1 == ','))
        { /* Try to remove double occurrences of these characters */
          if (c0 == tmplist)
            {
              *(c0++) = *c1;
            }
          else
            {
              if (*(c0-1) != *c1)
                *(c0++) = *c1;
            }
        }
      else
        break;

      c1++;
    }

  if (c0 == tmplist)
    return 1;

  *c0 = '\0';

  /* Now we have a comma separated list like 1-4-1,-3,1- */

  start_num = end_num = -1;
  state = READ_STARTNUM;
  for (c0 = tmplist; *c0 != '\0'; c0++)
    {
      switch (state)
        {
        case READ_STARTNUM:
          if (*c0 == ',')
            {
              if ((start_num > 0) && (start_num == (int)page_num))
                return -1;
              start_num = -1;
            }
          else if (*c0 == '-')
            {
              if (start_num < 0) start_num = 1;
              state = READ_ENDNUM;
            }
          else /* '0' - '9' */
            {
              if (start_num < 0) start_num = 0;
              start_num *= 10;
              start_num += *c0 - '0';
            }
          break;

        case READ_ENDNUM:
          if (*c0 == ',')
            {
              if (end_num < 0) end_num = 9999;
              CHK_LIST (start_num, end_num, (int)page_num);
              start_num = end_num = -1;
              state = READ_STARTNUM;
            }
          else if (*c0 == '-')
            {
              CHK_LIST (start_num, end_num, (int)page_num);
              start_num = end_num;
              end_num = -1;
            }
          else /* '0' - '9' */
            {
              if (end_num < 0) end_num = 0;
              end_num *= 10;
              end_num += *c0 - '0';
            }
          break;
        }
    }
  if (state == READ_STARTNUM)
    {
      if (start_num > 0)
        return (start_num == (int) page_num);
    }
  else
    {
      if (end_num < 0) end_num = 9999;
      CHK_LIST (start_num, end_num, (int)page_num);
    }

  return 0;
#undef CHK_LIST
}


/* A function like fgets, but treats single CR-character as line break. */
/* As a line break the newline-character is returned. */
static char *psfgets (char *s, int size, FILE *stream)

{
  int c;
  char *sptr = s;

  if (size <= 0)
    return NULL;

  if (size == 1)
    {
      *s = '\0';
      return NULL;
    }

  c = getc (stream);
  if (c == EOF)
    return NULL;

  for (;;)
    {
      /* At this point we have space in sptr for at least two characters */
      if (c == '\n')    /* Got end of line (UNIX line end) ? */
        {
          *(sptr++) = '\n';
          break;
        }
      else if (c == '\r')  /* Got a carriage return. Check next character */
        {
          c = getc (stream);
          if ((c == EOF) || (c == '\n')) /* EOF or DOS line end ? */
            {
              *(sptr++) = '\n';  /* Return UNIX line end */
              break;
            }
          else  /* Single carriage return. Return UNIX line end. */
            {
              ungetc (c, stream);  /* Save the extra character */
              *(sptr++) = '\n';
              break;
            }
        }
      else   /* no line end character */
        {
          *(sptr++) = (char)c;
          size--;
        }
      if (size == 1)
        break;  /* Only space for the nul-character ? */

      c = getc (stream);
      if (c == EOF)
        break;
    }

  *sptr = '\0';

  return s;
}


/* Get the BoundingBox of a PostScript file. On success, 0 is returned. */
/* On failure, -1 is returned. */
static gint
get_bbox (const gchar *filename,
          gint        *x0,
          gint        *y0,
          gint        *x1,
          gint        *y1)
{
  char line[1024], *src;
  FILE *ifp;
  int retval = -1;

  ifp = g_fopen (filename, "rb");
  if (ifp == NULL)
    return -1;

  for (;;)
    {
      if (psfgets (line, sizeof (line)-1, ifp) == NULL) break;
      if ((line[0] != '%') || (line[1] != '%')) continue;
      src = &(line[2]);
      while ((*src == ' ') || (*src == '\t')) src++;
      if (strncmp (src, "BoundingBox", 11) != 0) continue;
      src += 11;
      while ((*src == ' ') || (*src == '\t') || (*src == ':')) src++;
      if (strncmp (src, "(atend)", 7) == 0) continue;
      if (sscanf (src, "%d%d%d%d", x0, y0, x1, y1) == 4)
        retval = 0;
      break;
    }
  fclose (ifp);

  return retval;
}

static gchar *pnmfile;

/* Open the PostScript file. On failure, NULL is returned. */
/* The filepointer returned will give a PNM-file generated */
/* by the PostScript-interpreter. */
static FILE *
ps_open (const gchar      *filename,
         const PSLoadVals *loadopt,
         gint             *llx,
         gint             *lly,
         gint             *urx,
         gint             *ury,
         gboolean         *is_epsf)
{
  const gchar  *driver;
  GPtrArray    *cmdA;
  gchar       **pcmdA;
  FILE         *fd_popen = NULL;
  FILE         *eps_file;
  gint          width, height;
  gint          resolution;
  gint          x0, y0, x1, y1;
  gint          offx = 0;
  gint          offy = 0;
  gboolean      is_pdf;
  gboolean      maybe_epsf = FALSE;
  int           code;
  void         *instance;

  resolution = loadopt->resolution;
  *llx = *lly = 0;
  width = loadopt->width;
  height = loadopt->height;
  *urx = width - 1;
  *ury = height - 1;

  /* Check if the file is a PDF. For PDF, we can't set geometry */
  is_pdf = FALSE;

  /* Check if it is a EPS-file */
  *is_epsf = FALSE;

  eps_file = g_fopen (filename, "rb");

  if (eps_file != NULL)
    {
      gchar hdr[512];

      fread (hdr, 1, sizeof(hdr), eps_file);
      is_pdf = (strncmp (hdr, "%PDF", 4) == 0);

      if (!is_pdf)  /* Check for EPSF */
        {
          char *adobe, *epsf;
          int ds = 0;
          static unsigned char doseps[5] = { 0xc5, 0xd0, 0xd3, 0xc6, 0 };

          hdr[sizeof(hdr)-1] = '\0';
          adobe = strstr (hdr, "PS-Adobe-");
          epsf = strstr (hdr, "EPSF-");

          if ((adobe != NULL) && (epsf != NULL))
            ds = epsf - adobe;

          *is_epsf = ((ds >= 11) && (ds <= 15));

          /* Illustrator uses negative values in BoundingBox without marking */
          /* files as EPSF. Try to handle that. */
          maybe_epsf =
            (strstr (hdr, "%%Creator: Adobe Illustrator(R) 8.0") != 0);

          /* Check DOS EPS binary file */
          if ((!*is_epsf) && (strncmp (hdr, (char *)doseps, 4) == 0))
            *is_epsf = 1;
        }

      fclose (eps_file);
    }

  if ((!is_pdf) && (loadopt->use_bbox))    /* Try the BoundingBox ? */
    {
      if (get_bbox (filename, &x0, &y0, &x1, &y1) == 0)
        {
          if (maybe_epsf && ((x0 < 0) || (y0 < 0)))
            *is_epsf = 1;

          if (*is_epsf)  /* Handle negative BoundingBox for EPSF */
            {
              offx = -x0; x1 += offx; x0 += offx;
              offy = -y0; y1 += offy; y0 += offy;
            }
          if ((x0 >= 0) && (y0 >= 0) && (x1 > x0) && (y1 > y0))
            {
               *llx = (int)((x0/72.0) * resolution + 0.0001);
               *lly = (int)((y0/72.0) * resolution + 0.0001);
               /* Use upper bbox values as image size */
               width = (int)((x1/72.0) * resolution + 0.5);
               height = (int)((y1/72.0) * resolution + 0.5);
               /* Pixel coordinates must be one less */
               *urx = width - 1;
               *ury = height - 1;
               if (*urx < *llx) *urx = *llx;
               if (*ury < *lly) *ury = *lly;
            }
        }
    }

  switch (loadopt->pnm_type)
    {
    case 4:
      driver = "pbmraw";
      break;
    case 5:
      driver = "pgmraw";
      break;
    case 7:
      driver = "pnmraw";
      break;
    default:
      driver = "ppmraw";
      break;
    }

  /* For instance, the Win32 port of ghostscript doesn't work correctly when
   * using standard output as output file.
   * Thus, use a real output file.
   */
  pnmfile = gimp_temp_name ("pnm");

  /* Build command array */
  cmdA = g_ptr_array_new ();

  g_ptr_array_add (cmdA, g_strdup (g_get_prgname ()));
  g_ptr_array_add (cmdA, g_strdup_printf ("-sDEVICE=%s", driver));
  g_ptr_array_add (cmdA, g_strdup_printf ("-r%d", resolution));

  if (is_pdf)
    {
      /* Acrobat Reader honors CropBox over MediaBox, so let's match that
       * behavior.
       */
      g_ptr_array_add (cmdA, g_strdup ("-dUseCropBox"));
    }
  else
    {
      /* For PDF, we can't set geometry */
      g_ptr_array_add (cmdA, g_strdup_printf ("-g%dx%d", width, height));
    }

  /* Antialiasing not available for PBM-device */
  if ((loadopt->pnm_type != 4) && (loadopt->textalpha != 1))
    g_ptr_array_add (cmdA, g_strdup_printf ("-dTextAlphaBits=%d",
                                            loadopt->textalpha));
  if ((loadopt->pnm_type != 4) && (loadopt->graphicsalpha != 1))
    g_ptr_array_add (cmdA, g_strdup_printf ("-dGraphicsAlphaBits=%d",
                                            loadopt->graphicsalpha));
  g_ptr_array_add (cmdA, g_strdup ("-q"));
  g_ptr_array_add (cmdA, g_strdup ("-dBATCH"));
  g_ptr_array_add (cmdA, g_strdup ("-dNOPAUSE"));

  /* If no additional options specified, use at least -dSAFER */
  if (g_getenv ("GS_OPTIONS") == NULL)
    g_ptr_array_add (cmdA, g_strdup ("-dSAFER"));

  /* Output file name */
  g_ptr_array_add (cmdA, g_strdup_printf ("-sOutputFile=%s", pnmfile));

  /* Offset command for gs to get image part with negative x/y-coord. */
  if ((offx != 0) || (offy != 0))
    {
      g_ptr_array_add (cmdA, g_strdup ("-c"));
      g_ptr_array_add (cmdA, g_strdup_printf ("%d", offx));
      g_ptr_array_add (cmdA, g_strdup_printf ("%d", offy));
      g_ptr_array_add (cmdA, g_strdup ("translate"));
    }

  /* input file name */
  g_ptr_array_add (cmdA, g_strdup ("-f"));
  g_ptr_array_add (cmdA, g_strdup (filename));

  if (*is_epsf)
    {
      g_ptr_array_add (cmdA, g_strdup ("-c"));
      g_ptr_array_add (cmdA, g_strdup ("showpage"));
    }

  g_ptr_array_add (cmdA, g_strdup ("-c"));
  g_ptr_array_add (cmdA, g_strdup ("quit"));
  g_ptr_array_add (cmdA, NULL);

  pcmdA = (gchar **) cmdA->pdata;

#ifdef PS_DEBUG
  {
    gchar **p = pcmdA;
    g_print ("Passing args (argc=%d):\n", cmdA->len - 1);

    while (*p)
      {
        g_print ("%s\n", *p);
        p++;
      }
  }
#endif

  code = gsapi_new_instance (&instance, NULL);
  if (code == 0) {
    code = gsapi_init_with_args (instance, cmdA->len - 1, pcmdA);
    code = gsapi_exit (instance);
    gsapi_delete_instance (instance);
  }

  /* Don't care about exit status of ghostscript. */
  /* Just try to read what it wrote. */

  fd_popen = g_fopen (pnmfile, "rb");

  g_ptr_array_free (cmdA, FALSE);
  g_strfreev (pcmdA);

  return fd_popen;
}


/* Close the PNM-File of the PostScript interpreter */
static void
ps_close (FILE *ifp)
{
 /* If a real outputfile was used, close the file and remove it. */
  fclose (ifp);
  g_unlink (pnmfile);
}


/* Read the header of a raw PNM-file and return type (4-6) or -1 on failure */
static gint
read_pnmraw_type (FILE *ifp,
                  gint *width,
                  gint *height,
                  gint *maxval)
{
  int frst, scnd, thrd;
  gint  pnmtype;
  gchar line[1024];

  /* GhostScript may write some informational messages infront of the header. */
  /* We are just looking at a Px\n in the input stream. */
  frst = getc (ifp);
  scnd = getc (ifp);
  thrd = getc (ifp);
  for (;;)
    {
      if (thrd == EOF) return -1;
#ifdef G_OS_WIN32
      if (thrd == '\r') thrd = getc (ifp);
#endif
      if ((thrd == '\n') && (frst == 'P') && (scnd >= '1') && (scnd <= '6'))
        break;
      frst = scnd;
      scnd = thrd;
      thrd = getc (ifp);
    }
  pnmtype = scnd - '0';
  /* We don't use the ASCII-versions */
  if ((pnmtype >= 1) && (pnmtype <= 3))
    return -1;

  /* Read width/height */
  for (;;)
    {
      if (fgets (line, sizeof (line)-1, ifp) == NULL)
        return -1;
      if (line[0] != '#')
        break;
    }
  if (sscanf (line, "%d%d", width, height) != 2)
    return -1;

  *maxval = 255;

  if (pnmtype != 4)  /* Read maxval */
    {
      for (;;)
        {
          if (fgets (line, sizeof (line)-1, ifp) == NULL)
            return -1;
          if (line[0] != '#')
            break;
        }
      if (sscanf (line, "%d", maxval) != 1)
        return -1;
    }

  return pnmtype;
}


/* Create an image. Sets layer_ID, drawable and rgn. Returns image_ID */
static gint32
create_new_image (const gchar        *filename,
                  guint               pagenum,
                  guint               width,
                  guint               height,
                  GimpImageBaseType   type,
                  gint32             *layer_ID)
{
  gint32         image_ID;
  GimpImageType  gdtype;
  gchar         *tmp;

  switch (type)
    {
    case GIMP_GRAY:
      gdtype = GIMP_GRAY_IMAGE;
      break;
    case GIMP_INDEXED:
      gdtype = GIMP_INDEXED_IMAGE;
      break;
    case GIMP_RGB:
    default:
      gdtype = GIMP_RGB_IMAGE;
    }

  image_ID = gimp_image_new_with_precision (width, height, type,
                                            GIMP_PRECISION_U8_GAMMA);
  gimp_image_undo_disable (image_ID);

  tmp = g_strdup_printf ("%s-%d", filename, pagenum);
  gimp_image_set_filename (image_ID, tmp);
  g_free (tmp);

  tmp = g_strdup_printf (_("Page %d"), pagenum);
  *layer_ID = gimp_layer_new (image_ID, tmp, width, height,
                              gdtype,
                              100,
                              gimp_image_get_default_new_layer_mode (image_ID));
  g_free (tmp);

  gimp_image_insert_layer (image_ID, *layer_ID, -1, 0);

  return image_ID;
}


/* Skip PNM image generated from PostScript file. */
/* Return TRUE on success, FALSE otherwise.       */
static gboolean
skip_ps (FILE *ifp)
{
  guchar  buf[8192];
  gsize   len;
  gint    pnmtype, width, height, maxval, bpl;

  pnmtype = read_pnmraw_type (ifp, &width, &height, &maxval);

  if (pnmtype == 4)    /* Portable bitmap */
    bpl = (width + 7) / 8;
  else if (pnmtype == 5)
    bpl = width;
  else if (pnmtype == 6)
    bpl = width * 3;
  else
    return FALSE;

  len = bpl * height;
  while (len)
    {
      gsize  bytes = fread (buf, 1, MIN (len, sizeof (buf)), ifp);

      if (bytes < MIN (len, sizeof (buf)))
        return FALSE;

      len -= bytes;
    }

  return TRUE;
}


/* Load PNM image generated from PostScript file */
static gint32
load_ps (const gchar *filename,
         guint        pagenum,
         FILE        *ifp,
         gint         llx,
         gint         lly,
         gint         urx,
         gint         ury)
{
  guchar *dest;
  guchar *data, *bitline = NULL, *byteline = NULL, *byteptr, *temp;
  guchar bit2byte[256*8];
  int width, height, tile_height, scan_lines, total_scan_lines;
  int image_width, image_height;
  int skip_left, skip_bottom;
  int i, j, pnmtype, maxval, bpp, nread;
  GimpImageBaseType imagetype;
  gint32 layer_ID, image_ID;
  GeglBuffer *buffer = NULL;
  int err = 0, e;

  pnmtype = read_pnmraw_type (ifp, &width, &height, &maxval);

  if ((width == urx+1) && (height == ury+1))  /* gs respected BoundingBox ? */
    {
      skip_left = llx;    skip_bottom = lly;
      image_width = width - skip_left;
      image_height = height - skip_bottom;
    }
  else
    {
      skip_left = skip_bottom = 0;
      image_width = width;
      image_height = height;
    }
  if (pnmtype == 4)   /* Portable Bitmap */
    {
      imagetype = GIMP_INDEXED;
      nread = (width+7)/8;
      bpp = 1;
      bitline = g_new (guchar, nread);
      byteline = g_new (guchar, nread * 8);

      /* Get an array for mapping 8 bits in a byte to 8 bytes */
      temp = bit2byte;
      for (j = 0; j < 256; j++)
        for (i = 7; i >= 0; i--)
          *(temp++) = ((j & (1 << i)) != 0);
    }
  else if (pnmtype == 5)  /* Portable Greymap */
    {
      imagetype = GIMP_GRAY;
      nread = width;
      bpp = 1;
      byteline = g_new (guchar, nread);
    }
  else if (pnmtype == 6)  /* Portable Pixmap */
    {
      imagetype = GIMP_RGB;
      nread = width * 3;
      bpp = 3;
      byteline = g_new (guchar, nread);
    }
  else
    return -1;

  image_ID = create_new_image (filename, pagenum,
                               image_width, image_height, imagetype,
                               &layer_ID);
  buffer = gimp_drawable_get_buffer (layer_ID);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * image_width * bpp);

  dest = data;
  total_scan_lines = scan_lines = 0;

  if (pnmtype == 4)   /* Read bitimage ? Must be mapped to indexed */
    {
      const guchar BWColorMap[2*3] = { 255, 255, 255, 0, 0, 0 };

      gimp_image_set_colormap (image_ID, BWColorMap, 2);

      for (i = 0; i < height; i++)
        {
          e = (fread (bitline, 1, nread, ifp) != nread);
          if (total_scan_lines >= image_height)
            continue;
          err |= e;
          if (err)
            break;

          j = width;   /* Map 1 byte of bitimage to 8 bytes of indexed image */
          temp = bitline;
          byteptr = byteline;
          while (j >= 8)
            {
              memcpy (byteptr, bit2byte + *(temp++)*8, 8);
              byteptr += 8;
              j -= 8;
            }
          if (j > 0)
            memcpy (byteptr, bit2byte + *temp*8, j);

          memcpy (dest, byteline+skip_left, image_width);
          dest += image_width;
          scan_lines++;
          total_scan_lines++;

          if ((scan_lines == tile_height) || ((i + 1) == image_height))
            {
              gegl_buffer_set (buffer,
                               GEGL_RECTANGLE (0, i-scan_lines+1,
                                               image_width, scan_lines),
                               0,
                               NULL,
                               data,
                               GEGL_AUTO_ROWSTRIDE);
              scan_lines = 0;
              dest = data;
            }

          if ((i % 20) == 0)
            gimp_progress_update ((gdouble) (i + 1) / (gdouble) image_height);

          if (err)
            break;
        }
    }
  else   /* Read gray/rgb-image */
    {
      for (i = 0; i < height; i++)
        {
          e = (fread (byteline, bpp, width, ifp) != width);
          if (total_scan_lines >= image_height)
            continue;
          err |= e;
          if (err)
            break;

          memcpy (dest, byteline+skip_left*bpp, image_width*bpp);
          dest += image_width*bpp;
          scan_lines++;
          total_scan_lines++;

          if ((scan_lines == tile_height) || ((i + 1) == image_height))
            {
              gegl_buffer_set (buffer,
                               GEGL_RECTANGLE (0, i-scan_lines+1,
                                               image_width, scan_lines),
                               0,
                               NULL,
                               data,
                               GEGL_AUTO_ROWSTRIDE);
              scan_lines = 0;
              dest = data;
            }

          if ((i % 20) == 0)
            gimp_progress_update ((gdouble) (i + 1) / (gdouble) image_height);

          if (err)
            break;
        }
    }
  gimp_progress_update (1.0);

  g_free (data);
  g_free (byteline);
  g_free (bitline);

  if (err)
    g_message ("EOF encountered on reading");

  g_object_unref (buffer);

  return (err ? -1 : image_ID);
}


/* Write out the PostScript file header */
static gboolean
save_ps_header (GOutputStream  *output,
                GFile          *file,
                GError        **error)
{
  gchar  *basename = g_path_get_basename (gimp_file_get_utf8_name (file));
  time_t  cutime   = time (NULL);

  if (! print (output, error,
               "%%!PS-Adobe-3.0%s\n", psvals.eps ? " EPSF-3.0" : ""))
    goto fail;

  if (! print (output, error,
               "%%%%Creator: GIMP PostScript file plug-in V %4.2f "
               "by Peter Kirchgessner\n", VERSIO))
    goto fail;

  if (! print (output, error,
               "%%%%Title: %s\n"
               "%%%%CreationDate: %s"
               "%%%%DocumentData: Clean7Bit\n",
               basename, ctime (&cutime)))
    goto fail;

  if (psvals.eps || (psvals.level > 1))
    if (! print (output, error,"%%%%LanguageLevel: 2\n"))
      goto fail;

  if (! print (output, error, "%%%%Pages: 1\n"))
    goto fail;

  g_free (basename);

  return TRUE;

 fail:

  g_free (basename);

  return FALSE;
}


/* Write out transformation for image */
static gboolean
save_ps_setup (GOutputStream  *output,
               gint32          drawable_ID,
               gint            width,
               gint            height,
               gint            bpp,
               GError        **error)
{
  gdouble x_offset, y_offset, x_size, y_size;
  gdouble urx, ury;
  gdouble width_inch, height_inch;
  gdouble f1, f2, dx, dy;
  gint    xtrans, ytrans;
  gint    i_urx, i_ury;
  gchar   tmpbuf1[G_ASCII_DTOSTR_BUF_SIZE];
  gchar   tmpbuf2[G_ASCII_DTOSTR_BUF_SIZE];

  /* initialize */

  dx = 0.0;
  dy = 0.0;

  x_offset    = psvals.x_offset;
  y_offset    = psvals.y_offset;
  width_inch  = fabs (psvals.width);
  height_inch = fabs (psvals.height);

  if (psvals.unit_mm)
    {
      x_offset /= 25.4; y_offset /= 25.4;
      width_inch /= 25.4; height_inch /= 25.4;
    }

  if (psvals.keep_ratio)   /* Proportions to keep ? */
    {                        /* Fit the image into the allowed size */
      f1 = width_inch / width;
      f2 = height_inch / height;
      if (f1 < f2)
        height_inch = width_inch * (gdouble) height / (gdouble) width;
      else
        width_inch = fabs (height_inch) * (gdouble) width / (gdouble) height;
    }

  if ((psvals.rotate == 0) || (psvals.rotate == 180))
    {
      x_size = width_inch;
      y_size = height_inch;
    }
  else
    {
      y_size = width_inch;
      x_size = height_inch;
    }

  /* Round up upper right corner only for non-integer values */
  urx = (x_offset + x_size) * 72.0;
  ury = (y_offset + y_size) * 72.0;
  i_urx = (gint) urx;
  i_ury = (gint) ury;
  if (urx != (gdouble) i_urx) i_urx++;  /* Check for non-integer value */
  if (ury != (gdouble) i_ury) i_ury++;

  if (! print (output, error,
               "%%%%BoundingBox: %d %d %d %d\n"
               "%%%%EndComments\n",
               (gint) (x_offset * 72.0), (gint) (y_offset * 72.0), i_urx, i_ury))
    return FALSE;

  if (psvals.preview && (psvals.preview_size > 0))
    {
      if (! save_ps_preview (output, drawable_ID, error))
        return FALSE;
    }

  if (! print (output, error,
               "%%%%BeginProlog\n"
               "%% Use own dictionary to avoid conflicts\n"
               "10 dict begin\n"
               "%%%%EndProlog\n"
               "%%%%Page: 1 1\n"
               "%% Translate for offset\n"
               "%s %s translate\n",
               g_ascii_dtostr (tmpbuf1, sizeof (tmpbuf1), x_offset * 72.0),
               g_ascii_dtostr (tmpbuf2, sizeof (tmpbuf2), y_offset * 72.0)))
    return FALSE;

  /* Calculate translation to startpoint of first scanline */
  switch (psvals.rotate)
    {
    case   0: dx = 0.0; dy = y_size * 72.0;
      break;
    case  90: dx = dy = 0.0;
      break;
    case 180: dx = x_size * 72.0; dy = 0.0;
      break;
    case 270: dx = x_size * 72.0; dy = y_size * 72.0;
      break;
    }

  if ((dx != 0.0) || (dy != 0.0))
    {
      if (! print (output, error,
                   "%% Translate to begin of first scanline\n"
                   "%s %s translate\n",
                   g_ascii_dtostr (tmpbuf1, sizeof (tmpbuf1), dx),
                   g_ascii_dtostr (tmpbuf2, sizeof (tmpbuf2), dy)))
        return FALSE;
    }

  if (psvals.rotate)
    if (! print (output, error, "%d rotate\n", (gint) psvals.rotate))
      return FALSE;

  if (! print (output, error,
               "%s %s scale\n",
               g_ascii_dtostr (tmpbuf1, sizeof (tmpbuf1),  72.0 * width_inch),
               g_ascii_dtostr (tmpbuf2, sizeof (tmpbuf2), -72.0 * height_inch)))
    return FALSE;

  /* Write the PostScript procedures to read the image */
  if (psvals.level <= 1)
    {
      if (! print (output, error,
                   "%% Variable to keep one line of raster data\n"))
        return FALSE;

      if (bpp == 1)
        {
          if (! print (output, error,
                       "/scanline %d string def\n", (width + 7) / 8))
            return FALSE;
        }
      else
        {
          if (! print (output, error,
                       "/scanline %d %d mul string def\n", width, bpp / 8))
            return FALSE;
        }
    }

  if (! print (output, error,
               "%% Image geometry\n%d %d %d\n"
               "%% Transformation matrix\n",
               width, height, (bpp == 1) ? 1 : 8))
    return FALSE;

  xtrans = ytrans = 0;
  if (psvals.width  < 0.0) { width  = -width;  xtrans = -width;  }
  if (psvals.height < 0.0) { height = -height; ytrans = -height; }

  if (! print (output, error,
               "[ %d 0 0 %d %d %d ]\n", width, height, xtrans, ytrans))
    return FALSE;

  return TRUE;
}

static gboolean
save_ps_trailer (GOutputStream  *output,
                 GError        **error)
{
  return print (output, error,
                "%%%%Trailer\n"
                "end\n%%%%EOF\n");
}

/* Do a Floyd-Steinberg dithering on a grayscale scanline. */
/* linecount must keep the counter for the actual scanline (0, 1, 2, ...). */
/* If linecount is less than zero, all used memory is freed. */

static void
dither_grey (const guchar *grey,
             guchar       *bw,
             gint          npix,
             gint          linecount)
{
  static gboolean  do_init_arrays = TRUE;
  static gint     *fs_error = NULL;
  static gint      limit[1278];
  static gint      east_error[256];
  static gint      seast_error[256];
  static gint      south_error[256];
  static gint      swest_error[256];

  const guchar *greyptr;
  guchar       *bwptr, mask;
  gint         *fse;
  gint          x, greyval, fse_inline;

  if (linecount <= 0)
    {
      g_free (fs_error);

      if (linecount < 0)
        return;

      fs_error = g_new0 (gint, npix + 2);

      /* Initialize some arrays that speed up dithering */
      if (do_init_arrays)
        {
          gint i;

          do_init_arrays = FALSE;

          for (i = 0, x = -511; x <= 766; i++, x++)
            limit[i] = (x < 0) ? 0 : ((x > 255) ? 255 : x);

          for (greyval = 0; greyval < 256; greyval++)
            {
              east_error[greyval] = (greyval < 128) ?
                ((greyval * 79) >> 8) : (((greyval - 255) * 79) >> 8);
              seast_error[greyval] = (greyval < 128) ?
                ((greyval * 34) >> 8) : (((greyval - 255) * 34) >> 8);
              south_error[greyval] = (greyval < 128) ?
                ((greyval * 56) >> 8) : (((greyval - 255) * 56) >> 8);
              swest_error[greyval] = (greyval < 128) ?
                ((greyval * 12) >> 8) : (((greyval - 255) * 12) >> 8);
            }
        }
    }

  g_return_if_fail (fs_error != NULL);

  memset (bw, 0, (npix + 7) / 8); /* Initialize with white */

  greyptr = grey;
  bwptr = bw;
  mask = 0x80;

  fse_inline = fs_error[1];

  for (x = 0, fse = fs_error + 1; x < npix; x++, fse++)
    {
      greyval =
        limit[*(greyptr++) + fse_inline + 512];  /* 0 <= greyval <= 255 */

      if (greyval < 128)
        *bwptr |= mask;  /* Set a black pixel */

      /* Error distribution */
      fse_inline = east_error[greyval] + fse[1];
      fse[1] = seast_error[greyval];
      fse[0] += south_error[greyval];
      fse[-1] += swest_error[greyval];

      mask >>= 1;   /* Get mask for next b/w-pixel */

      if (!mask)
        {
          mask = 0x80;
          bwptr++;
        }
    }
}

/* Write a device independent screen preview */
static gboolean
save_ps_preview (GOutputStream  *output,
                 gint32          drawable_ID,
                 GError        **error)
{
  GimpImageType  drawable_type;
  GeglBuffer    *buffer = NULL;
  const Babl    *format;
  gint           bpp;
  guchar        *bwptr, *greyptr;
  gint           width, height, x, y, nbsl, out_count;
  gint           nchar_pl = 72, src_y;
  gdouble        f1, f2;
  guchar        *grey, *bw, *src_row, *src_ptr;
  guchar        *cmap;
  gint           ncols, cind;

  if (psvals.preview_size <= 0)
    return TRUE;

  buffer = gimp_drawable_get_buffer (drawable_ID);
  cmap = NULL;

  drawable_type = gimp_drawable_type (drawable_ID);
  switch (drawable_type)
    {
    case GIMP_GRAY_IMAGE:
      format = babl_format ("Y' u8");
      break;

    case GIMP_INDEXED_IMAGE:
      cmap = gimp_image_get_colormap (gimp_item_get_image (drawable_ID),
                                      &ncols);
      format = gimp_drawable_get_format (drawable_ID);
      break;

    case GIMP_RGB_IMAGE:
    default:
      format = babl_format ("R'G'B' u8");
      break;
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  width = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  /* Calculate size of preview */
  if ((width > psvals.preview_size) ||
      (height > psvals.preview_size))
    {
      f1 = (gdouble) psvals.preview_size / (gdouble) width;
      f2 = (gdouble) psvals.preview_size / (gdouble) height;

      if (f1 < f2)
        {
          width = psvals.preview_size;
          height *= f1;
          if (height <= 0)
            height = 1;
        }
      else
        {
          height = psvals.preview_size;
          width *= f1;
          if (width <= 0)
            width = 1;
        }
    }

  nbsl = (width + 7) / 8;  /* Number of bytes per scanline in bitmap */

  grey    = g_new (guchar, width);
  bw      = g_new (guchar, nbsl);
  src_row = g_new (guchar, gegl_buffer_get_width (buffer) * bpp);

  if (! print (output, error,
               "%%%%BeginPreview: %d %d 1 %d\n",
               width, height,
               ((nbsl * 2 + nchar_pl - 1) / nchar_pl) * height))
    goto fail;

  for (y = 0; y < height; y++)
    {
      /* Get a scanline from the input image and scale it to the desired
         width */
      src_y = (y * gegl_buffer_get_height (buffer)) / height;
      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (0, src_y,
                                       gegl_buffer_get_width (buffer), 1),
                       1.0, format, src_row,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      greyptr = grey;
      if (bpp == 3)   /* RGB-image */
        {
          for (x = 0; x < width; x++)
            {                       /* Convert to grey */
              src_ptr = src_row + ((x * gegl_buffer_get_width (buffer)) / width) * 3;
              *(greyptr++) = (3*src_ptr[0] + 6*src_ptr[1] + src_ptr[2]) / 10;
            }
        }
      else if (cmap)    /* Indexed image */
        {
          for (x = 0; x < width; x++)
            {
              src_ptr = src_row + ((x * gegl_buffer_get_width (buffer)) / width);
              cind = *src_ptr;   /* Get color index and convert to grey */
              src_ptr = (cind >= ncols) ? cmap : (cmap + 3*cind);
              *(greyptr++) = (3*src_ptr[0] + 6*src_ptr[1] + src_ptr[2]) / 10;
            }
        }
      else             /* Grey image */
        {
          for (x = 0; x < width; x++)
            *(greyptr++) = *(src_row + ((x * gegl_buffer_get_width (buffer)) / width));
        }

      /* Now we have a grayscale line for the desired width. */
      /* Dither it to b/w */
      dither_grey (grey, bw, width, y);

      /* Write out the b/w line */
      out_count = 0;
      bwptr = bw;
      for (x = 0; x < nbsl; x++)
        {
          if (out_count == 0)
            if (! print (output, error, "%% "))
              goto fail;

          if (! print (output, error, "%02x", *(bwptr++)))
            goto fail;

          out_count += 2;
          if (out_count >= nchar_pl)
            {
              if (! print (output, error, "\n"))
                goto fail;

              out_count = 0;
            }
        }

      if (! print (output, error, "\n"))
        goto fail;

      if ((y % 20) == 0)
        gimp_progress_update ((gdouble) y / (gdouble) height);
    }

  gimp_progress_update (1.0);

  if (! print (output, error, "%%%%EndPreview\n"))
    goto fail;

  dither_grey (grey, bw, width, -1);
  g_free (src_row);
  g_free (bw);
  g_free (grey);

  g_object_unref (buffer);

  return TRUE;

 fail:

  g_free (src_row);
  g_free (bw);
  g_free (grey);

  g_object_unref (buffer);

  return FALSE;
}

static gboolean
save_gray (GOutputStream  *output,
           gint32          image_ID,
           gint32          drawable_ID,
           GError        **error)
{
  GeglBuffer *buffer = NULL;
  const Babl *format;
  gint        bpp;
  gint        height, width, i, j;
  gint        tile_height;
  guchar     *data;
  guchar     *src;
  guchar     *packb = NULL;
  gboolean    level2 = (psvals.level > 1);

  buffer = gimp_drawable_get_buffer (drawable_ID);
  format = babl_format ("Y' u8");
  bpp    = babl_format_get_bytes_per_pixel (format);
  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  tile_height = gimp_tile_height ();

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (guchar *) g_malloc (tile_height * width * bpp);

  /* Set up transformation in PostScript */
  if (! save_ps_setup (output, drawable_ID, width, height, 1 * 8, error))
    goto fail;

  /* Write read image procedure */
  if (! level2)
    {
      if (! print (output, error,
                   "{ currentfile scanline readhexstring pop }\n"))
        goto fail;
    }
  else
    {
      if (! print (output, error,
                   "currentfile /ASCII85Decode filter /RunLengthDecode filter\n"))
        goto fail;

      ascii85_init ();

      /* Allocate buffer for packbits data. Worst case: Less than 1% increase */
      packb = (guchar *) g_malloc ((width * 105) / 100 + 2);
    }

  if (! ps_begin_data (output, error))
    goto fail;

  if (! print (output, error, "image\n"))
    goto fail;

#define GET_GRAY_TILE(begin) \
  { gint scan_lines;                                                    \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scan_lines),  \
                     1.0, format, begin,                                \
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);             \
    src = begin; }

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0)
        GET_GRAY_TILE (data); /* Get more data */

      if (! level2)
        {
          for (j = 0; j < width; j++)
            {
              if (! print (output, error, "%c", hex[(*src) >> 4]))
                goto fail;

              if (! print (output, error, "%c", hex[(*(src++)) & 0x0f]))
                goto fail;

              if (((j + 1) % 39) == 0)
                if (! print (output, error, "\n"))
                  goto fail;
           }

          if (! print (output, error, "\n"))
            goto fail;
        }
      else
        {
          gint nout;

          compress_packbits (width, src, &nout, packb);

          if (! ascii85_nout (output, nout, packb, error))
            goto fail;

          src += width;
        }

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) height);
    }

  gimp_progress_update (1.0);

  if (level2)
    {
      /* Write EOD of RunLengthDecode filter */
      if (! ascii85_out (output, 128, error))
        goto fail;

      if (! ascii85_done (output, error))
        goto fail;
    }

  if (! ps_end_data (output, error))
    return FALSE;

  if (! print (output, error, "showpage\n"))
    goto fail;

  g_free (data);
  g_free (packb);

  g_object_unref (buffer);

  return TRUE;

 fail:

  g_free (data);
  g_free (packb);

  g_object_unref (buffer);

  return FALSE;

#undef GET_GRAY_TILE
}

static gboolean
save_bw (GOutputStream  *output,
         gint32          image_ID,
         gint32          drawable_ID,
         GError        **error)
{
  GeglBuffer *buffer = NULL;
  const Babl *format;
  gint        bpp;
  gint        height, width, i, j;
  gint        ncols, nbsl, nwrite;
  gint        tile_height;
  guchar     *cmap, *ct;
  guchar     *data, *src;
  guchar     *packb = NULL;
  guchar     *scanline, *dst, mask;
  guchar     *hex_scanline;
  gboolean    level2 = (psvals.level > 1);

  cmap = gimp_image_get_colormap (image_ID, &ncols);

  buffer = gimp_drawable_get_buffer (drawable_ID);
  format = gimp_drawable_get_format (drawable_ID);
  bpp    = babl_format_get_bytes_per_pixel (format);
  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  tile_height = gimp_tile_height ();

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = g_new (guchar, tile_height * width * bpp);

  nbsl = (width + 7) / 8;

  scanline     = g_new (guchar, nbsl + 1);
  hex_scanline = g_new (guchar, (nbsl + 1) * 2);

  /* Set up transformation in PostScript */
  if (! save_ps_setup (output, drawable_ID, width, height, 1, error))
    goto fail;

  /* Write read image procedure */
  if (! level2)
    {
      if (! print (output, error,
                   "{ currentfile scanline readhexstring pop }\n"))
        goto fail;
    }
  else
    {
      if (! print (output, error,
                   "currentfile /ASCII85Decode filter /RunLengthDecode filter\n"))
        goto fail;

      ascii85_init ();

      /* Allocate buffer for packbits data. Worst case: Less than 1% increase */
      packb = g_new (guchar, ((nbsl+1) * 105) / 100 + 2);
    }

  if (! ps_begin_data (output, error))
    goto fail;

  if (! print (output, error, "image\n"))
    goto fail;

#define GET_BW_TILE(begin) \
  { gint scan_lines;                                                    \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scan_lines),  \
                     1.0, format, begin,                                \
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);             \
    src = begin; }

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0)
        GET_BW_TILE (data); /* Get more data */

      dst = scanline;
      memset (dst, 0, nbsl);
      mask = 0x80;

      /* Build a bitmap for a scanline */
      for (j = 0; j < width; j++)
        {
          ct = cmap + *(src++)*3;
          if (ct[0] || ct[1] || ct[2])
            *dst |= mask;

          if (mask == 0x01)
            {
              mask = 0x80; dst++;
            }
          else
            {
              mask >>= 1;
            }
        }

      if (! level2)
        {
          /* Convert to hexstring */
          for (j = 0; j < nbsl; j++)
            {
              hex_scanline[j * 2]     = (guchar) hex[scanline[j] >> 4];
              hex_scanline[j * 2 + 1] = (guchar) hex[scanline[j] & 0x0f];
            }

          /* Write out hexstring */
          j = nbsl * 2;
          dst = hex_scanline;

          while (j > 0)
            {
              nwrite = (j > 78) ? 78 : j;

              if (! g_output_stream_write_all (output,
                                               dst, nwrite, NULL,
                                               NULL, error))
                goto fail;

              if (! print (output, error, "\n"))
                goto fail;

              j -= nwrite;
              dst += nwrite;
            }
        }
      else
        {
          gint nout;

          compress_packbits (nbsl, scanline, &nout, packb);

          if (! ascii85_nout (output, nout, packb, error))
            goto fail;
        }

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) height);
    }

  gimp_progress_update (1.0);

  if (level2)
    {
      /* Write EOD of RunLengthDecode filter */
      if (! ascii85_out (output, 128, error))
        goto fail;

      if (! ascii85_done (output, error))
        goto fail;
    }

  if (! ps_end_data (output, error))
    goto fail;

  if (! print (output, error, "showpage\n"))
    goto fail;

  g_free (hex_scanline);
  g_free (scanline);
  g_free (data);
  g_free (packb);

  g_object_unref (buffer);

  return TRUE;

 fail:

  g_free (hex_scanline);
  g_free (scanline);
  g_free (data);
  g_free (packb);

  g_object_unref (buffer);

  return FALSE;

#undef GET_BW_TILE
}

static gboolean
save_index (GOutputStream  *output,
            gint32          image_ID,
            gint32          drawable_ID,
            GError        **error)
{
  GeglBuffer *buffer = NULL;
  const Babl *format;
  gint        bpp;
  gint        height, width, i, j;
  gint        ncols, bw;
  gint        tile_height;
  guchar     *cmap, *cmap_start;
  guchar     *data, *src;
  guchar     *packb = NULL;
  guchar     *plane = NULL;
  gchar       coltab[256 * 6], *ct;
  gboolean    level2 = (psvals.level > 1);

  cmap = cmap_start = gimp_image_get_colormap (image_ID, &ncols);

  ct = coltab;
  bw = 1;
  for (j = 0; j < 256; j++)
    {
      if (j >= ncols)
        {
          memset (ct, 0, 6);
          ct += 6;
        }
      else
        {
          bw &= ((cmap[0] == 0)   && (cmap[1] == 0)   && (cmap[2] == 0)) ||
                ((cmap[0] == 255) && (cmap[1] == 255) && (cmap[2] == 255));

          *(ct++) = (guchar) hex[(*cmap) >> 4];
          *(ct++) = (guchar) hex[(*(cmap++)) & 0x0f];
          *(ct++) = (guchar) hex[(*cmap) >> 4];
          *(ct++) = (guchar) hex[(*(cmap++)) & 0x0f];
          *(ct++) = (guchar) hex[(*cmap) >> 4];
          *(ct++) = (guchar) hex[(*(cmap++)) & 0x0f];
        }
    }

  if (bw)
    return save_bw (output, image_ID, drawable_ID, error);

  buffer = gimp_drawable_get_buffer (drawable_ID);
  format = gimp_drawable_get_format (drawable_ID);
  bpp    = babl_format_get_bytes_per_pixel (format);
  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  tile_height = gimp_tile_height ();

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (guchar *) g_malloc (tile_height * width * bpp);

  /* Set up transformation in PostScript */
  if (! save_ps_setup (output, drawable_ID, width, height, 3 * 8, error))
    goto fail;

  /* Write read image procedure */
  if (! level2)
    {
      if (! print (output, error,
                   "{ currentfile scanline readhexstring pop } false 3\n"))
        goto fail;
    }
  else
    {
      if (! print (output, error,
                   "%% Strings to hold RGB-samples per scanline\n"
                   "/rstr %d string def\n"
                   "/gstr %d string def\n"
                   "/bstr %d string def\n",
                   width, width, width))
        goto fail;

      if (! print (output, error,
                   "{currentfile /ASCII85Decode filter /RunLengthDecode filter\
 rstr readstring pop}\n"
                   "{currentfile /ASCII85Decode filter /RunLengthDecode filter\
 gstr readstring pop}\n"
                   "{currentfile /ASCII85Decode filter /RunLengthDecode filter\
 bstr readstring pop}\n"
                   "true 3\n"))
        goto fail;

      /* Allocate buffer for packbits data. Worst case: Less than 1% increase */
      packb = (guchar *) g_malloc ((width * 105) / 100 + 2);
      plane = (guchar *) g_malloc (width);
    }

  if (! ps_begin_data (output, error))
    goto fail;

  if (! print (output, error, "colorimage\n"))
    goto fail;

#define GET_INDEX_TILE(begin) \
  { gint scan_lines;                                                    \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scan_lines),  \
                     1.0, format, begin,                                \
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);             \
    src = begin; }

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0)
        GET_INDEX_TILE (data); /* Get more data */

      if (! level2)
        {
          for (j = 0; j < width; j++)
            {
              if (! g_output_stream_write_all (output,
                                               coltab + (*(src++)) * 6, 6, NULL,
                                               NULL, error))
                goto fail;

              if (((j + 1) % 13) == 0)
                if (! print (output, error, "\n"))
                  goto fail;
            }

          if (! print (output, error, "\n"))
            goto fail;
        }
      else
        {
          gint rgb;

          for (rgb = 0; rgb < 3; rgb++)
            {
              guchar *src_ptr   = src;
              guchar *plane_ptr = plane;
              gint    nout;

              for (j = 0; j < width; j++)
                *(plane_ptr++) = cmap_start[3 * *(src_ptr++) + rgb];

              compress_packbits (width, plane, &nout, packb);

              ascii85_init ();

              if (! ascii85_nout (output, nout, packb, error))
                goto fail;

              /* Write EOD of RunLengthDecode filter */
              if (! ascii85_out (output, 128, error))
                goto fail;

              if (! ascii85_done (output, error))
                goto fail;
            }

          src += width;
        }

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) height);
    }

  gimp_progress_update (1.0);

  if (! ps_end_data (output, error))
    goto fail;

  if (! print (output, error, "showpage\n"))
    goto fail;

  g_free (data);
  g_free (packb);
  g_free (plane);

  g_object_unref (buffer);

  return TRUE;

 fail:

  g_free (data);
  g_free (packb);
  g_free (plane);

  g_object_unref (buffer);

  return FALSE;

#undef GET_INDEX_TILE
}

static gboolean
save_rgb (GOutputStream  *output,
          gint32          image_ID,
          gint32          drawable_ID,
          GError        **error)
{
  GeglBuffer *buffer = NULL;
  const Babl *format;
  gint        bpp;
  gint        height, width, tile_height;
  gint        i, j;
  guchar     *data, *src;
  guchar     *packb = NULL;
  guchar     *plane = NULL;
  gboolean    level2 = (psvals.level > 1);

  buffer = gimp_drawable_get_buffer (drawable_ID);
  format = babl_format ("R'G'B' u8");
  bpp    = babl_format_get_bytes_per_pixel (format);
  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  tile_height = gimp_tile_height ();

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = g_new (guchar, tile_height * width * bpp);

  /* Set up transformation in PostScript */
  if (! save_ps_setup (output, drawable_ID, width, height, 3 * 8, error))
    goto fail;

  /* Write read image procedure */
  if (! level2)
    {
      if (! print (output, error,
                   "{ currentfile scanline readhexstring pop } false 3\n"))
        goto fail;
    }
  else
    {
      if (! print (output, error,
                   "%% Strings to hold RGB-samples per scanline\n"
                   "/rstr %d string def\n"
                   "/gstr %d string def\n"
                   "/bstr %d string def\n",
                   width, width, width))
        goto fail;

      if (! print (output, error,
                   "{currentfile /ASCII85Decode filter /RunLengthDecode filter\
 rstr readstring pop}\n"
                   "{currentfile /ASCII85Decode filter /RunLengthDecode filter\
 gstr readstring pop}\n"
                   "{currentfile /ASCII85Decode filter /RunLengthDecode filter\
 bstr readstring pop}\n"
                   "true 3\n"))
        goto fail;

      /* Allocate buffer for packbits data. Worst case: Less than 1% increase */
      packb = g_new (guchar, (width * 105) / 100 + 2);
      plane = g_new (guchar, width);
    }

  if (! ps_begin_data (output, error))
    goto fail;

  if (! print (output, error, "colorimage\n"))
    goto fail;

#define GET_RGB_TILE(begin) \
  { gint scan_lines;                                                    \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scan_lines),  \
                     1.0, format, begin,                                \
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);             \
    src = begin; }

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0)
        GET_RGB_TILE (data); /* Get more data */

      if (! level2)
        {
          for (j = 0; j < width; j++)
            {
              if (! print (output, error, "%c",
                           hex[(*src) >> 4]))        /* Red */
                goto fail;

              if (! print (output, error, "%c",
                           hex[(*(src++)) & 0x0f]))
                goto fail;

              if (! print (output, error, "%c",
                           hex[(*src) >> 4]))        /* Green */
                goto fail;

              if (! print (output, error, "%c",
                           hex[(*(src++)) & 0x0f]))
                goto fail;

              if (! print (output, error, "%c",
                           hex[(*src) >> 4]))        /* Blue */
                goto fail;

              if (! print (output, error, "%c",
                           hex[(*(src++)) & 0x0f]))
                goto fail;

              if (((j+1) % 13) == 0)
                if (! print (output, error, "\n"))
                  goto fail;
            }

          if (! print (output, error, "\n"))
            goto fail;
        }
      else
        {
          gint rgb;

          for (rgb = 0; rgb < 3; rgb++)
            {
              guchar *src_ptr   = src + rgb;
              guchar *plane_ptr = plane;
              gint    nout;

              for (j = 0; j < width; j++)
                {
                  *(plane_ptr++) = *src_ptr;
                  src_ptr += 3;
                }

              compress_packbits (width, plane, &nout, packb);

              ascii85_init ();

              if (! ascii85_nout (output, nout, packb, error))
                goto fail;

              /* Write EOD of RunLengthDecode filter */
              if (! ascii85_out (output, 128, error))
                goto fail;

              if (! ascii85_done (output, error))
                goto fail;
            }

          src += 3 * width;
        }

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) height);
    }

  gimp_progress_update (1.0);

  if (! ps_end_data (output, error))
    goto fail;

  if (! print (output, error, "showpage\n"))
    goto fail;

  g_free (data);
  g_free (packb);
  g_free (plane);

  g_object_unref (buffer);

  return TRUE;

 fail:

  g_free (data);
  g_free (packb);
  g_free (plane);

  g_object_unref (buffer);

  return FALSE;

#undef GET_RGB_TILE
}

static gboolean
print (GOutputStream  *output,
       GError        **error,
       const gchar    *format,
       ...)
{
  va_list  args;
  gboolean success;

  va_start (args, format);
  success = g_output_stream_vprintf (output, NULL, NULL,
                                     error, format, args);
  va_end (args);

  return success;
}

/*  Load interface functions  */

static gint32
count_ps_pages (const gchar *filename)
{
  FILE   *psfile;
  gchar  *extension;
  gchar   buf[1024];
  gint32  num_pages      = 0;
  gint32  showpage_count = 0;

  extension = strrchr (filename, '.');
  if (extension)
    {
      extension = g_ascii_strdown (extension + 1, -1);

      if (strcmp (extension, "eps") == 0)
        {
          g_free (extension);
          return 1;
        }

      g_free (extension);
    }

  psfile = g_fopen (filename, "r");

  if (psfile == NULL)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return 0;
    }

  while (num_pages == 0 && !feof (psfile))
    {
      fgets (buf, sizeof (buf), psfile);

      if (strncmp (buf + 2, "Pages:", 6) == 0)
        sscanf (buf + strlen ("%%Pages:"), "%d", &num_pages);
      else if (strncmp (buf, "showpage", 8) == 0)
        showpage_count++;
    }

  if (feof (psfile) && num_pages < 1 && showpage_count > 0)
    num_pages = showpage_count;

  fclose (psfile);

  return num_pages;
}

static gboolean
load_dialog (const gchar *filename)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *hbox;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *grid;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;
  GtkWidget     *entry    = NULL;
  GtkWidget     *target   = NULL;
  GtkWidget     *toggle;
  GtkWidget     *selector = NULL;
  gint32         page_count;
  gchar         *range    = NULL;
  gboolean       run;

  page_count = count_ps_pages (filename);

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Import from PostScript"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PS_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Import"), GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  if (page_count > 1)
    {
      selector = gimp_page_selector_new ();
      gtk_box_pack_start (GTK_BOX (main_vbox), selector, TRUE, TRUE, 0);
      gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (selector),
                                      page_count);
      gimp_page_selector_set_target (GIMP_PAGE_SELECTOR (selector),
                                     ps_pagemode);

      gtk_widget_show (selector);

      g_signal_connect_swapped (selector, "activate",
                                G_CALLBACK (gtk_window_activate_default),
                                dialog);
    }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Rendering */
  frame = gimp_frame_new (_("Rendering"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Resolution/Width/Height/Pages labels */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  adj = (GtkAdjustment *) gtk_adjustment_new (plvals.resolution,
                                              MIN_RESOLUTION, MAX_RESOLUTION,
                                              1, 10, 0);
  spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("Resolution:"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (resolution_change_callback),
                    &plvals.resolution);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &plvals.resolution);

  adj = (GtkAdjustment *) gtk_adjustment_new (plvals.width,
                                              1, GIMP_MAX_IMAGE_SIZE,
                                              1, 10, 0);
  ps_width_spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("_Width:"), 0.0, 0.5,
                            ps_width_spinbutton, 1);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &plvals.width);

  adj = (GtkAdjustment *) gtk_adjustment_new (plvals.height,
                                              1, GIMP_MAX_IMAGE_SIZE,
                                              1, 10, 0);
  ps_height_spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("_Height:"), 0.0, 0.5,
                            ps_height_spinbutton, 1);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &plvals.height);

  if (page_count == 0)
    {
      entry = gtk_entry_new ();
      gtk_widget_set_size_request (entry, 80, -1);
      gtk_entry_set_text (GTK_ENTRY (entry), plvals.pages);
      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                                _("Pages:"), 0.0, 0.5,
                                entry, 1);

      g_signal_connect (entry, "changed",
                        G_CALLBACK (load_pages_entry_callback),
                        NULL);
      gimp_help_set_help_data (GTK_WIDGET (entry),
                               _("Pages to load (e.g.: 1-4 or 1,3,5-7)"), NULL);

      target = gtk_combo_box_text_new ();
      gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (target),
                                      GIMP_PAGE_SELECTOR_TARGET_LAYERS,
                                      _("Layers"));
      gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (target),
                                      GIMP_PAGE_SELECTOR_TARGET_IMAGES,
                                      _("Images"));
      gtk_combo_box_set_active (GTK_COMBO_BOX (target), (int) ps_pagemode);
      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                                _("Open as"), 0.0, 0.5,
                                target, 1);
    }

  toggle = gtk_check_button_new_with_label (_("Try Bounding Box"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), plvals.use_bbox);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &plvals.use_bbox);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Coloring */
  frame = gimp_int_radio_group_new (TRUE, _("Coloring"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &plvals.pnm_type, plvals.pnm_type,

                                    _("B/W"),       4, NULL,
                                    _("Gray"),      5, NULL,
                                    _("Color"),     6, NULL,
                                    _("Automatic"), 7, NULL,

                                    NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gimp_int_radio_group_new (TRUE, _("Text antialiasing"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &plvals.textalpha, plvals.textalpha,

                                    C_("antialiasing", "None"), 1, NULL,
                                    _("Weak"),                  2, NULL,
                                    _("Strong"),                4, NULL,

                                    NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  frame = gimp_int_radio_group_new (TRUE, _("Graphic antialiasing"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &plvals.graphicsalpha, plvals.graphicsalpha,

                                    C_("antialiasing", "None"), 1, NULL,
                                    _("Weak"),                  2, NULL,
                                    _("Strong"),                4, NULL,

                                    NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (selector)
    {
      range = gimp_page_selector_get_selected_range (GIMP_PAGE_SELECTOR (selector));

      if (strlen (range) < 1)
        {
          gimp_page_selector_select_all (GIMP_PAGE_SELECTOR (selector));
          range = gimp_page_selector_get_selected_range (GIMP_PAGE_SELECTOR (selector));
        }

      strncpy (plvals.pages, range, sizeof (plvals.pages));
      plvals.pages[strlen (range)] = '\0';

      ps_pagemode = gimp_page_selector_get_target (GIMP_PAGE_SELECTOR (selector));
    }
  else if (page_count == 0)
    {
      ps_pagemode = gtk_combo_box_get_active (GTK_COMBO_BOX (target));
    }
  else
    {
      strncpy (plvals.pages, "1", 1);
      plvals.pages[1] = '\0';
      ps_pagemode = GIMP_PAGE_SELECTOR_TARGET_IMAGES;
    }

  gtk_widget_destroy (dialog);

  return run;
}

static void
load_pages_entry_callback (GtkWidget *widget,
                           gpointer   data)
{
  gsize nelem = sizeof (plvals.pages);

  strncpy (plvals.pages, gtk_entry_get_text (GTK_ENTRY (widget)), nelem);
  plvals.pages[nelem-1] = '\0';
}


/*  Save interface functions  */

static gboolean
save_dialog (void)
{
  SaveDialogVals *vals;
  GtkWidget      *dialog;
  GtkWidget      *toggle;
  GtkWidget      *frame, *uframe;
  GtkWidget      *hbox, *vbox;
  GtkWidget      *main_vbox[2];
  GtkWidget      *grid;
  GtkWidget      *spinbutton;
  GtkAdjustment  *adj;
  gint            j;
  gboolean        run;

  vals = g_new (SaveDialogVals, 1);
  vals->level = (psvals.level > 1);

  dialog = gimp_export_dialog_new (_("PostScript"), PLUG_IN_BINARY, SAVE_PS_PROC);

  /* Main hbox */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      hbox, FALSE, FALSE, 0);
  main_vbox[0] = main_vbox[1] = NULL;

  for (j = 0; j < G_N_ELEMENTS (main_vbox); j++)
    {
      main_vbox[j] = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
      gtk_box_pack_start (GTK_BOX (hbox), main_vbox[j], FALSE, TRUE, 0);
      gtk_widget_show (main_vbox[j]);
    }

  /* Image Size */
  frame = gimp_frame_new (_("Image Size"));
  gtk_box_pack_start (GTK_BOX (main_vbox[0]), frame, FALSE, TRUE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Width/Height/X-/Y-offset labels */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  vals->adjustment[0] = (GtkAdjustment *)
    gtk_adjustment_new (psvals.width,
                        1e-5, GIMP_MAX_IMAGE_SIZE, 1, 10, 0);
  spinbutton = gtk_spin_button_new (vals->adjustment[0], 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Width:"), 0.0, 0.5,
                            spinbutton, 1);
  g_signal_connect (vals->adjustment[0], "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &psvals.width);

  vals->adjustment[1] = (GtkAdjustment *)
    gtk_adjustment_new (psvals.height,
                        1e-5, GIMP_MAX_IMAGE_SIZE, 1, 10, 0);
  spinbutton = gtk_spin_button_new (vals->adjustment[1], 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("_Height:"), 0.0, 0.5,
                            spinbutton, 1);
  g_signal_connect (vals->adjustment[1], "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &psvals.height);

  vals->adjustment[2] = (GtkAdjustment *)
    gtk_adjustment_new (psvals.x_offset,
                        0.0, GIMP_MAX_IMAGE_SIZE, 1, 10, 0);
  spinbutton = gtk_spin_button_new (vals->adjustment[2], 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("_X offset:"), 0.0, 0.5,
                            spinbutton, 1);
  g_signal_connect (vals->adjustment[2], "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &psvals.x_offset);

  vals->adjustment[3] = (GtkAdjustment *)
    gtk_adjustment_new (psvals.y_offset,
                        0.0, GIMP_MAX_IMAGE_SIZE, 1, 10, 0);
  spinbutton = gtk_spin_button_new (vals->adjustment[3], 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("_Y offset:"), 0.0, 0.5,
                            spinbutton, 1);
  g_signal_connect (vals->adjustment[3], "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &psvals.y_offset);

  toggle = gtk_check_button_new_with_mnemonic (_("_Keep aspect ratio"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), psvals.keep_ratio);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("When toggled, the resulting image will be "
                             "scaled to fit into the given size without "
                             "changing the aspect ratio."),
                           "#keep_aspect_ratio"),

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &psvals.keep_ratio);

  /* Unit */
  uframe = gimp_int_radio_group_new (TRUE, _("Unit"),
                                     G_CALLBACK (save_unit_toggle_update),
                                     vals, psvals.unit_mm,

                                     _("_Inch"),       FALSE, NULL,
                                     _("_Millimeter"), TRUE,  NULL,

                                     NULL);

  gtk_box_pack_start (GTK_BOX (main_vbox[0]), uframe, TRUE, TRUE, 0);
  gtk_widget_show (uframe);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Rotation */
  frame = gimp_int_radio_group_new (TRUE, _("Rotation"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &psvals.rotate, psvals.rotate,

                                    "_0",   0,   NULL,
                                    "_90",  90,  NULL,
                                    "_180", 180, NULL,
                                    "_270", 270, NULL,

                                    NULL);

  gtk_box_pack_start (GTK_BOX (main_vbox[1]), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* Format */
  frame = gimp_frame_new (_("Output"));
  gtk_box_pack_start (GTK_BOX (main_vbox[1]), frame, TRUE, TRUE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_mnemonic (_("_PostScript level 2"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), vals->level);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &vals->level);

  toggle = gtk_check_button_new_with_mnemonic (_("_Encapsulated PostScript"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), psvals.eps);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &psvals.eps);

  toggle = gtk_check_button_new_with_mnemonic (_("P_review"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), psvals.preview);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &psvals.preview);

  /* Preview size label/entry */
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  g_object_bind_property (toggle, "active",
                          grid,   "sensitive",
                          G_BINDING_SYNC_CREATE);

  adj = (GtkAdjustment *) gtk_adjustment_new (psvals.preview_size,
                                              0, 1024, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("Preview _size:"), 1.0, 0.5,
                            spinbutton, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &psvals.preview_size);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  gtk_widget_show (hbox);
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  psvals.level = (vals->level) ? 2 : 1;

  g_free (vals);

  return run;
}

static void
save_unit_toggle_update (GtkWidget *widget,
                         gpointer   data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      SaveDialogVals *vals = (SaveDialogVals *) data;
      gdouble         factor;
      gdouble         value;
      gint            unit_mm;
      gint            i;

      unit_mm = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                    "gimp-item-data"));

      psvals.unit_mm = unit_mm;

      if (unit_mm)
        factor = 25.4;
      else
        factor = 1.0 / 25.4;

      for (i = 0; i < 4; i++)
        {
          value = gtk_adjustment_get_value (vals->adjustment[i]) * factor;

          gtk_adjustment_set_value (vals->adjustment[i], value);
        }
    }
}

static gboolean
resolution_change_callback (GtkAdjustment *adjustment,
                            gpointer       data)
{
  guint   *old_resolution = (guint *) data;
  gdouble  ratio;

  if (*old_resolution)
    ratio = (gdouble) gtk_adjustment_get_value (adjustment) / *old_resolution;
  else
    ratio = 1.0;

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (ps_width_spinbutton),
                             gtk_spin_button_get_value (GTK_SPIN_BUTTON (ps_width_spinbutton)) * ratio);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (ps_height_spinbutton),
                             gtk_spin_button_get_value (GTK_SPIN_BUTTON (ps_height_spinbutton)) * ratio);

  return TRUE;

}
