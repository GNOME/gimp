/* The GIMP -- an image manipulation program
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
 */
#define VERSIO 1.08
static char dversio[] = "v1.08  16-Jan-2000";
static char ident[] = "@(#) GIMP PostScript/PDF file-plugin v1.08  16-Jan-2000";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#ifdef G_OS_WIN32
#include <process.h>		/* For _getpid() */

#define USE_REAL_OUTPUTFILE
#endif

#define STR_LENGTH 64

/* Load info */
typedef struct
{
  guint resolution;        /* resolution (dpi) at which to run ghostscript */
  guint width, height;     /* desired size (ghostscript may ignore this) */
  gint  use_bbox;          /* 0: use width/height, 1: try to use BoundingBox */
  gchar pages[STR_LENGTH]; /* Pages to load (eg.: 1,3,5-7) */
  gint  pnm_type;          /* 4: pbm, 5: pgm, 6: ppm, 7: automatic */
  gint  textalpha;         /* antialiasing: 1,2, or 4 TextAlphaBits */
  gint  graphicsalpha;     /* antialiasing: 1,2, or 4 GraphicsAlphaBits */
} PSLoadVals;

typedef struct
{
  gint  run;  /*  run  */
} PSLoadInterface;

static PSLoadVals plvals =
{
  100,                  /* 100 dpi */
  826, 1170,            /* default width/height (A4) */
  1,                    /* try to use BoundingBox */
  "1-99",               /* pages to load */
  6,                    /* use ppm (colour) */
  1,                    /* dont use text antialiasing */
  1                     /* dont use graphics antialiasing */
};

static PSLoadInterface plint =
{
  FALSE     /* run */
};


/* Save info  */
typedef struct
{
  gdouble width, height;      /* Size of image */
  gdouble x_offset, y_offset; /* Offset to image on page */
  gint    unit_mm;            /* Unit of measure (0: inch, 1: mm) */
  gint    keep_ratio;         /* Keep aspect ratio */
  gint    rotate;             /* Rotation (0, 90, 180, 270) */
  gint    level;              /* PostScript Level */
  gint    eps;                /* Encapsulated PostScript flag */
  gint    preview;            /* Preview Flag */
  gint    preview_size;       /* Preview size */
} PSSaveVals;

typedef struct
{
  gint  run;  /*  run  */
} PSSaveInterface;

static PSSaveVals psvals =
{
  287.0, 200.0,   /* Image size (A4) */
  5.0, 5.0,       /* Offset */
  1,              /* Unit is mm */
  1,              /* Keep edge ratio */
  0,              /* Rotate */
  2,              /* PostScript Level */
  0,              /* Encapsulated PostScript flag */
  0,              /* Preview flag */
  256             /* Preview size */
};

static PSSaveInterface psint =
{
  FALSE     /* run */
};


/* Declare some local functions.
 */
static void   query      (void);
static void   run        (gchar   *name,
                          gint     nparams,
                          GParam  *param,
                          gint    *nreturn_vals,
                          GParam **return_vals);

static gint32 load_image (gchar   *filename);
static gint   save_image (gchar   *filename,
                          gint32   image_ID,
                          gint32   drawable_ID);

static gint save_gray  (FILE   *ofp,
                        gint32  image_ID,
                        gint32  drawable_ID);
static gint save_bw    (FILE   *ofp,
                        gint32  image_ID,
                        gint32  drawable_ID);
static gint save_index (FILE   *ofp,
                        gint32  image_ID,
                        gint32  drawable_ID);
static gint save_rgb   (FILE   *ofp,
                        gint32  image_ID,
                        gint32  drawable_ID);

static gint32 create_new_image (gchar      *filename,
				guint       pagenum,
				guint       width,
				guint       height,
				GImageType  type,
				gint32     *layer_ID,
				GDrawable **drawable,
				GPixelRgn  *pixel_rgn);

static void   check_load_vals  (void);
static void   check_save_vals  (void);

static gint   page_in_list  (gchar *list,
			     guint  pagenum);

static gint   get_bbox      (gchar *filename,
			     gint  *x0,
			     gint  *y0,
			     gint  *x1,
			     gint  *y1);

static FILE  *ps_open       (gchar            *filename,
			     const PSLoadVals *loadopt,
			     gint             *llx,
			     gint             *lly,
			     int              *urx,
			     gint             *ury);

static void   ps_close      (FILE *ifp);

static gint32 skip_ps       (FILE *ifp);

static gint32 load_ps       (gchar *filename,
			     guint  pagenum,
			     FILE  *ifp,
			     gint   llx,
			     gint   lly,
			     gint   urx,
			     gint   ury);

static void save_ps_header  (FILE   *ofp,
			     gchar  *filename);
static void save_ps_setup   (FILE   *ofp,
			     gint32  drawable_ID,
			     gint    width,
			     gint    height,
			     gint    bpp);
static void save_ps_trailer (FILE   *ofp);
static void save_ps_preview (FILE   *ofp,
                             gint32  drawable_ID);
static void dither_grey     (guchar *grey,
			     guchar *bw,
			     gint    npix,
			     gint    linecount);


/* Dialog-handling */

static void   init_gtk                  (void);

static gint   load_dialog               (void);
static void   load_ok_callback          (GtkWidget *widget,
					 gpointer   data);
static void   load_pages_entry_callback (GtkWidget *widget,
					 gpointer   data);

typedef struct
{
  GtkObject *adjustment[4];
  gint       level;
} SaveDialogVals;

static gint   save_dialog              (void);
static void   save_ok_callback         (GtkWidget *widget,
                                        gpointer   data);
static void   save_unit_toggle_update  (GtkWidget *widget,
                                        gpointer   data);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


/* The run mode */
static GRunModeType l_run_mode;


static guint32 ascii85_buf;
static int ascii85_len = 0;
static int ascii85_linewidth = 0;

static void
ascii85_init (void)
{
  ascii85_len = 0;
  ascii85_linewidth = 0;
}

static void
ascii85_flush (FILE *ofp)
{
  char c[5];
  int i;
  gboolean zero_case = (ascii85_buf == 0);

  for (i=4; i >= 0; i--)
    {
      c[i] = (ascii85_buf % 85) + '!';
      ascii85_buf /= 85;
    }
  /* check for special case: "!!!!!" becomes "z", but only if not
   * at end of data. */
  if (zero_case && (ascii85_len == 4))
    {
      putc ('z', ofp);
      ascii85_linewidth++;
    }
  else
    {
      ascii85_linewidth += ascii85_len+1;
      for (i=0; i < ascii85_len+1; i++)
	putc (c[i], ofp);
    }

  if (ascii85_linewidth >= 75)
    {
      putc ('\n', ofp);
      ascii85_linewidth = 0;
    }

  ascii85_len = 0;
  ascii85_buf = 0;
}

static inline void
ascii85_out (unsigned char byte, FILE *ofp)
{
  if (ascii85_len == 4)
    ascii85_flush (ofp);

  ascii85_buf <<= 8;
  ascii85_buf |= byte;
  ascii85_len++;
}

static void
ascii85_done (FILE *ofp)
{
  if (ascii85_len)
    {
      /* zero any unfilled buffer portion, then flush */
      ascii85_buf <<= (8 * (4-ascii85_len));
      ascii85_flush (ofp);
    }

  putc ('~', ofp);
  putc ('>', ofp);
  putc ('\n', ofp);
}



MAIN ()

static void
query (void)
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name of the file to load" }
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static gint nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  static GParamDef set_load_args[] =
  {
    { PARAM_INT32, "resolution", "Resolution to interprete image (dpi)" },
    { PARAM_INT32, "width", "Desired width" },
    { PARAM_INT32, "height", "Desired height" },
    { PARAM_INT32, "check_bbox", "0: Use width/height, 1: Use BoundingBox" },
    { PARAM_STRING, "pages", "Pages to load (e.g.: 1,3,5-7)" },
    { PARAM_INT32, "coloring", "4: b/w, 5: grey, 6: colour image, 7: automatic" },
    { PARAM_INT32, "TextAlphaBits", "1, 2, or 4" },
    { PARAM_INT32, "GraphicsAlphaBits", "1, 2, or 4" }
  };
  static gint nset_load_args = (sizeof (set_load_args) /
				sizeof (set_load_args[0]));

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename",
            "The name of the file to save the image in" },
    { PARAM_FLOAT, "width", "Width of the image in PostScript file (0: use input image size)" },
    { PARAM_FLOAT, "height", "Height of image in PostScript file (0: use input image size)" },
    { PARAM_FLOAT, "x_offset", "X-offset to image from lower left corner" },
    { PARAM_FLOAT, "y_offset", "Y-offset to image from lower left corner" },
    { PARAM_INT32, "unit", "Unit for width/height/offset. 0: inches, 1: millimeters" },
    { PARAM_INT32, "keep_ratio", "0: use width/height, 1: keep aspect ratio" },
    { PARAM_INT32, "rotation", "0, 90, 180, 270" },
    { PARAM_INT32, "eps_flag", "0: PostScript, 1: Encapsulated PostScript" },
    { PARAM_INT32, "preview", "0: no preview, >0: max. size of preview" },
    { PARAM_INT32, "level", "1: PostScript Level 1, 2: PostScript Level 2" }
  };
  static gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  INIT_I18N();

  gimp_install_procedure ("file_ps_load",
                          "load file of PostScript/PDF file format",
                          "load file of PostScript/PDF file format",
                          "Peter Kirchgessner <peter@kirchgessner.net>",
                          "Peter Kirchgessner",
                          dversio,
                          "<Load>/PostScript",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_ps_load_setargs",
                          "set additional parameters for procedure file_ps_load",
                          "set additional parameters for procedure file_ps_load",
                          "Peter Kirchgessner <peter@kirchgessner.net>",
                          "Peter Kirchgessner",
                          dversio,
                          NULL,
                          NULL,
                          PROC_PLUG_IN,
                          nset_load_args, 0,
                          set_load_args, NULL);

  gimp_install_procedure ("file_ps_save",
                          "save file in PostScript file format",
                          "PostScript saving handles all image types except those with alpha channels.",
                          "Peter Kirchgessner <pkirchg@aol.com>",
                          "Peter Kirchgessner",
                          dversio,
                          "<Save>/PostScript",
                          "RGB, GRAY, INDEXED",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_ps_load",
				    "ps,eps,pdf",
				    "",
                                    "0,string,%!,0,string,%PDF");
  gimp_register_save_handler       ("file_ps_save",
				    "ps,eps",
				    "");
}


#ifdef GIMP_HAVE_RESOLUTION_INFO
static void
ps_set_save_size (PSSaveVals *vals,
                  gint32      image_ID)
{
  gdouble  xres, yres, factor, iw, ih;
  guint    width, height;
  GimpUnit unit;

  gimp_image_get_resolution (image_ID, &xres, &yres);
  if ((xres < GIMP_MIN_RESOLUTION) || (yres < GIMP_MIN_RESOLUTION))
    {
      xres = yres = 72.0;
    }
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
#endif

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
  gint32        image_ID = -1;
  gint32        drawable_ID = -1;
  gint32        orig_image_ID = -1;
  GimpExportReturnType export = EXPORT_CANCEL;
  gint k;

  l_run_mode = run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_EXECUTION_ERROR;

  if (strcmp (name, "file_ps_load") == 0)
    {
      INIT_I18N_UI();

      switch (run_mode)
	{
        case RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data ("file_ps_load", &plvals);

          if (! load_dialog ())
	    status = STATUS_CANCEL;
          break;

        case RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 3)
            status = STATUS_CALLING_ERROR;
          else    /* Get additional interpretation arguments */
            gimp_get_data ("file_ps_load", &plvals);
          break;

        case RUN_WITH_LAST_VALS:
          /* Possibly retrieve data */
          gimp_get_data ("file_ps_load", &plvals);
          break;

        default:
          break;
	}

      if (status == STATUS_SUCCESS)
	{
	  check_load_vals ();
	  image_ID = load_image (param[1].data.d_string);

	  if (image_ID != -1)
	    {
	      *nreturn_vals = 2;
	      values[1].type         = PARAM_IMAGE;
	      values[1].data.d_image = image_ID;
	    }
	  else
	    {
	      status = STATUS_EXECUTION_ERROR;
	    }
	}

      /*  Store plvals data  */
      if (status == STATUS_SUCCESS)
	gimp_set_data ("file_ps_load", &plvals, sizeof (PSLoadVals));
    }
  else if (strcmp (name, "file_ps_save") == 0)
    {
      INIT_I18N_UI();

      image_ID = orig_image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /* eventually export the image */ 
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	case RUN_WITH_LAST_VALS:
	  init_gtk ();
	  export = gimp_export_image (&image_ID, &drawable_ID, "PS", 
				      (CAN_HANDLE_RGB |
				       CAN_HANDLE_GRAY |
				       CAN_HANDLE_INDEXED));
	  if (export == EXPORT_CANCEL)
	    {
	      values[0].data.d_status = STATUS_CANCEL;
	      return;
	    }
	  break;
	default:
	  break;
	}

      switch (run_mode)
        {
        case RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data ("file_ps_save", &psvals);

          /* About to save an EPS-file ? Switch on eps-flag in dialog */
          k = strlen (param[3].data.d_string);
          if ((k >= 4) && (strcmp (param[3].data.d_string+k-4, ".eps") == 0))
            psvals.eps = 1;

#ifdef GIMP_HAVE_RESOLUTION_INFO
          ps_set_save_size (&psvals, orig_image_ID);
#endif
          /*  First acquire information with a dialog  */
          if (! save_dialog ())
            status = STATUS_CANCEL;
          break;

        case RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 15)
	    {
	      status = STATUS_CALLING_ERROR;
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
	      psvals.eps          = param[12].data.d_int32;
	      psvals.preview      = (param[13].data.d_int32 != 0);
	      psvals.preview_size = param[13].data.d_int32;
	      psvals.level        = param[14].data.d_int32;
	    }
          break;

        case RUN_WITH_LAST_VALS:
          /*  Possibly retrieve data  */
          gimp_get_data ("file_ps_save", &psvals);
          break;

        default:
          break;
        }

      if (status == STATUS_SUCCESS)
	{
#ifdef GIMP_HAVE_RESOLUTION_INFO
	  if ((psvals.width == 0.0) || (psvals.height == 0.0))
	    ps_set_save_size (&psvals, orig_image_ID);
#endif
	  check_save_vals ();
	  if (save_image (param[3].data.d_string, image_ID, drawable_ID))
	    {
	      /*  Store psvals data  */
	      gimp_set_data ("file_ps_save", &psvals, sizeof (PSSaveVals));
	    }
	  else
	    {
	      status = STATUS_EXECUTION_ERROR;
	    }
	}

      if (export == EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else if (strcmp (name, "file_ps_load_setargs") == 0)
    {
      /*  Make sure all the arguments are there!  */
      if (nparams != 8)
	{
	  status = STATUS_CALLING_ERROR;
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
	  gimp_set_data ("file_ps_load", &plvals, sizeof (PSLoadVals));
	}
    }
  else
    {
      status = STATUS_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}


static gint32
load_image (gchar *filename)
{
  gint32 image_ID, *image_list, *nl;
  guint page_count;
  FILE *ifp;
  char *temp;
  int  llx, lly, urx, ury;
  int  k, n_images, max_images, max_pagenum;

#ifdef PS_DEBUG
  g_print ("load_image:\n resolution = %d\n", plvals.resolution);
  g_print (" %dx%d pixels\n", plvals.width, plvals.height);
  g_print (" BoundingBox: %d\n", plvals.use_bbox);
  g_print (" Colouring: %d\n", plvals.pnm_type);
  g_print (" TextAlphaBits: %d\n", plvals.textalpha);
  g_print (" GraphicsAlphaBits: %d\n", plvals.graphicsalpha);
#endif

  /* Try to see if PostScript file is available */
  ifp = fopen (filename, "r");
  if (ifp == NULL)
    {
      g_message (_("PS: can't open file for reading"));
      return (-1);
    }
  fclose (ifp);

  if (l_run_mode != RUN_NONINTERACTIVE)
    {
      temp = g_strdup_printf (_("Interpreting and Loading %s:"), filename);
      gimp_progress_init (temp);
      g_free (temp);
    }

  ifp = ps_open (filename, &plvals, &llx, &lly, &urx, &ury);
  if (!ifp)
    {
      g_message (_("PS: can't interprete file"));
      return (-1);
    }

  image_list = (gint32 *)g_malloc (10 * sizeof (gint32));
  n_images = 0;
  max_images = 10;

  max_pagenum = 9999;  /* Try to get the maximum pagenumber to read */
  if (!page_in_list (plvals.pages, max_pagenum)) /* Is there a limit in list ? */
    {
      max_pagenum = -1;
      for (temp = plvals.pages; *temp != '\0'; temp++)
	{
	  if ((*temp < '0') || (*temp > '9')) continue; /* Search next digit */
	  sscanf (temp, "%d", &k);
	  if (k > max_pagenum) max_pagenum = k;
	  while ((*temp >= '0') && (*temp <= '9')) temp++;
	  temp--;
	}
      if (max_pagenum < 1) max_pagenum = 9999;
    }

  /* Load all images */
  for (page_count = 1; page_count <= max_pagenum; page_count++)
    {
      if (page_in_list (plvals.pages, page_count))
	{
	  image_ID = load_ps (filename, page_count, ifp, llx, lly, urx, ury);
	  if (image_ID == -1) break;
#ifdef GIMP_HAVE_RESOLUTION_INFO
	  gimp_image_set_resolution (image_ID, 
				     (double) plvals.resolution,
				     (double) plvals.resolution);
#endif
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
	  image_ID = skip_ps (ifp);
	  if (image_ID == -1) break;
	}
    }

  ps_close (ifp);

  /* Display images in reverse order. The last will be displayed by GIMP itself*/
  if (l_run_mode != RUN_NONINTERACTIVE)
    {
      for (k = n_images-1; k >= 1; k--)
	gimp_display_new (image_list[k]);
    }

  image_ID = (n_images > 0) ? image_list[0] : -1;
  g_free (image_list);

  return (image_ID);
}


static gint
save_image (gchar  *filename,
            gint32  image_ID,
            gint32  drawable_ID)
{
  FILE* ofp;
  GDrawableType drawable_type;
  gint retval;
  char *temp = ident; /* Just to satisfy lint/gcc */

  /* initialize */

  retval = 0;

  drawable_type = gimp_drawable_type (drawable_ID);

  /*  Make sure we're not saving an image with an alpha channel  */
  if (gimp_drawable_has_alpha (drawable_ID))
    {
      g_message (_("PostScript save cannot handle images with alpha channels"));
      return FALSE;
    }

  switch (drawable_type)
    {
    case INDEXED_IMAGE:
    case GRAY_IMAGE:
    case RGB_IMAGE:
      break;
    default:
      g_message (_("PS: cannot operate on unknown image types"));
      return (FALSE);
      break;
    }

  /* Open the output file. */
  ofp = fopen (filename, "wb");
  if (!ofp)
    {
      g_message (_("PS: can't open file for writing"));
      return (FALSE);
    }

  if (l_run_mode != RUN_NONINTERACTIVE)
    {
      temp = g_strdup_printf (_("Saving %s:"), filename);
      gimp_progress_init (temp);
      g_free (temp);
    }

  save_ps_header (ofp, filename);

  if (drawable_type == GRAY_IMAGE)
    retval = save_gray (ofp, image_ID, drawable_ID);
  else if (drawable_type == INDEXED_IMAGE)
    retval = save_index (ofp, image_ID, drawable_ID);
  else if (drawable_type == RGB_IMAGE)
    retval = save_rgb (ofp, image_ID, drawable_ID);

  save_ps_trailer (ofp);

  fclose (ofp);

  return (retval);
}


/* Check (and correct) the load values plvals */
static void
check_load_vals (void)
{
  if (plvals.resolution < 5)
    plvals.resolution = 5;
  else if (plvals.resolution > 1440)
    plvals.resolution = 1440;

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

  if ((list == NULL) || (*list == '\0')) return (1);

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
	{ /* Try to remove double occurances of these characters */
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
      else break;
      c1++;
    }
  if (c0 == tmplist) return (1);
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
	      if ((start_num > 0) && (start_num == (int)page_num)) return (-1);
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
	return (start_num == (int)page_num);
    }
  else
    {
      if (end_num < 0) end_num = 9999;
      CHK_LIST (start_num, end_num, (int)page_num);
    }
  return (0);
#undef CHK_LIST
}


/* Get the BoundingBox of a PostScript file. On success, 0 is returned. */
/* On failure, -1 is returned. */
static gint
get_bbox (gchar *filename,
          gint  *x0,
          gint  *y0,
          gint  *x1,
          gint  *y1)
{
  char line[1024], *src;
  FILE *ifp;
  int retval = -1;

  ifp = fopen (filename, "rb");
  if (ifp == NULL) return (-1);

  for (;;)
    {
      if (fgets (line, sizeof (line)-1, ifp) == NULL) break;
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
  return (retval);
}

static gchar *pnmfile;
#ifdef G_OS_WIN32
static gchar *indirfile = NULL;
#endif

/* Open the PostScript file. On failure, NULL is returned. */
/* The filepointer returned will give a PNM-file generated */
/* by the PostScript-interpreter. */
static FILE *
ps_open (gchar            *filename,
         const PSLoadVals *loadopt,
         gint             *llx,
         gint             *lly,
         gint             *urx,
         gint             *ury)
{
  char *cmd, *gs, *gs_opts, *driver;
  FILE *fd_popen;
  int width, height, resolution;
  int x0, y0, x1, y1;
  int is_pdf;
  char TextAlphaBits[64], GraphicsAlphaBits[64], geometry[32];

  resolution = loadopt->resolution;
  *llx = *lly = 0;
  width = loadopt->width;
  height = loadopt->height;
  *urx = width-1;
  *ury = height-1;

  /* Check if the file is a PDF. For PDF, we cant set geometry */
  is_pdf = 0;
#ifndef __EMX__
  fd_popen = fopen (filename, "r");
#else
  fd_popen = fopen (filename, "rb");
#endif
  if (fd_popen != NULL)
    {
      char hdr[4];

      fread (hdr, 1, 4, fd_popen);
      is_pdf = (strncmp (hdr, "%PDF", 4) == 0);
      fclose (fd_popen);
    }

  if ((!is_pdf) && (loadopt->use_bbox))    /* Try the BoundingBox ? */
    {
      if (   (get_bbox (filename, &x0, &y0, &x1, &y1) == 0)
	     && (x0 >= 0) && (y0 >= 0) && (x1 > x0) && (y1 > y0))
	{
	  *llx = (int)((x0/72.0) * resolution + 0.01);
	  *lly = (int)((y0/72.0) * resolution + 0.01);
	  *urx = (int)((x1/72.0) * resolution + 0.01);
	  *ury = (int)((y1/72.0) * resolution + 0.01);
	  width = *urx + 1;
	  height = *ury + 1;
	}
    }
  if (loadopt->pnm_type == 4) driver = "pbmraw";
  else if (loadopt->pnm_type == 5) driver = "pgmraw";
  else if (loadopt->pnm_type == 7) driver = "pnmraw";
  else driver = "ppmraw";

#ifdef USE_REAL_OUTPUTFILE
  /* For instance, the Win32 port of ghostscript doesn't work correctly when
   * using standard output as output file.
   * Thus, use a real output file.
   */
  pnmfile = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "p%lx",
			     g_get_tmp_dir (), getpid ());
#else
  pnmfile = "-";
#endif

  gs = getenv ("GS_PROG");
#ifndef G_OS_WIN32
  if (gs == NULL) gs = "gs";
#else
  /* We want the console ghostscript application. It should be in the PATH */
  if (gs == NULL)
    gs = "gswin32c";
  /* Quote the filename in case it contains spaces. Ignore memory leak,
   * this is a short-lived plug-in.
   */
  filename = g_strdup_printf ("\"%s\"", filename);
#endif

  gs_opts = getenv ("GS_OPTIONS");
  if (gs_opts == NULL)
    gs_opts = "-dSAFER";
  else
    gs_opts = "";  /* Ghostscript will add these options */

  TextAlphaBits[0] = GraphicsAlphaBits[0] = geometry[0] = '\0';

  /* Antialiasing not available for PBM-device */
  if ((loadopt->pnm_type != 4) && (loadopt->textalpha != 1))
    sprintf (TextAlphaBits, "-dTextAlphaBits=%d ", (int)loadopt->textalpha);

  if ((loadopt->pnm_type != 4) && (loadopt->graphicsalpha != 1))
    sprintf (GraphicsAlphaBits, "-dGraphicsAlphaBits=%d ",
	     (int)loadopt->graphicsalpha);

  if (!is_pdf)    /* For PDF, we cant set geometry */
    sprintf (geometry,"-g%dx%d ", width, height);

  cmd = g_strdup_printf ("%s -sDEVICE=%s -r%d %s%s%s-q -dNOPAUSE %s \
-sOutputFile=%s %s -c quit",
			 gs, driver, resolution, geometry,
			 TextAlphaBits, GraphicsAlphaBits,
			 gs_opts, pnmfile, filename);
#ifdef PS_DEBUG
  g_print ("Going to start ghostscript with:\n%s\n", cmd);
#endif

#ifndef USE_REAL_OUTPUTFILE
  /* Start the command and use a pipe for reading the PNM-file. */
#ifndef __EMX__
  fd_popen = popen (cmd, "r");
#else
  fd_popen = popen (cmd, "rb");
#endif
#else
  /* If someone does not like the pipe (or it does not work), just start */
  /* ghostscript with a real outputfile. When ghostscript has finished,  */
  /* open the outputfile and return its filepointer. But be sure         */
  /* to close and remove the file within ps_close().                     */
#ifdef G_OS_WIN32
  /* Work around braindead command line length limit.
   * Hmm, I wonder if this is necessary, but it doesn't harm...
   */
  if (strlen (cmd) >= 100)
    {
      FILE *indf;
      indirfile = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "i%lx",
				   g_get_tmp_dir (), getpid ());
      indf = fopen (indirfile, "w");
      fprintf (indf, "%s\n", cmd + strlen (gs) + 1);
      sprintf (cmd, "%s @%s", gs, indirfile);
      fclose (indf);
    }
#endif
  system (cmd);
#ifndef __EMX__
  fd_popen = fopen (pnmfile, "r");
#else
  fd_popen = fopen (pnmfile, "rb");
#endif
#endif
  g_free (cmd);

  return (fd_popen);
}


/* Close the PNM-File of the PostScript interpreter */
static void
ps_close (FILE *ifp)
{
#ifndef USE_REAL_OUTPUTFILE
  /* Finish reading from pipe. */
  pclose (ifp);
#else
 /* If a real outputfile was used, close the file and remove it. */
  fclose (ifp);
  unlink (pnmfile);
#ifdef G_OS_WIN32
  if (indirfile != NULL)
    unlink (indirfile);
#endif
#endif
}


/* Read the header of a raw PNM-file and return type (4-6) or -1 on failure */
static gint
read_pnmraw_type (FILE *ifp,
                  gint *width,
                  gint *height,
                  gint *maxval)
{
  register int frst, scnd, thrd;
  gint pnmtype;
  char line[1024];

  /* GhostScript may write some informational messages infront of the header. */
  /* We are just looking at a Px\n in the input stream. */
  frst = getc (ifp);
  scnd = getc (ifp);
  thrd = getc (ifp);
  for (;;)
    {
      if (thrd == EOF) return (-1);
#if defined (__EMX__) || defined (WIN32)
      if (thrd == '\r') thrd = getc (ifp);
#endif
      if ((thrd == '\n') && (frst == 'P') && (scnd >= '1') && (scnd <= '6'))
	break;
      frst = scnd;
      scnd = thrd;
      thrd = getc (ifp);
    }
  pnmtype = scnd - '0';
  /* We dont use the ASCII-versions */
  if ((pnmtype >= 1) && (pnmtype <= 3)) return (-1);

  /* Read width/height */
  for (;;)
    {
      if (fgets (line, sizeof (line)-1, ifp) == NULL) return (-1);
      if (line[0] != '#') break;
    }
  if (sscanf (line, "%d%d", width, height) != 2) return (-1);
  *maxval = 255;

  if (pnmtype != 4)  /* Read maxval */
    {
      for (;;)
	{
	  if (fgets (line, sizeof (line)-1, ifp) == NULL) return (-1);
	  if (line[0] != '#') break;
	}
      if (sscanf (line, "%d", maxval) != 1) return (-1);
    }
  return (pnmtype);
}


/* Create an image. Sets layer_ID, drawable and rgn. Returns image_ID */
static gint32
create_new_image (gchar       *filename,
                  guint        pagenum,
                  guint        width,
                  guint        height,
                  GImageType   type,
                  gint32      *layer_ID,
                  GDrawable  **drawable,
                  GPixelRgn   *pixel_rgn)
{
  gint32 image_ID;
  GDrawableType gdtype;
  char *tmp;

  if (type == GRAY) gdtype = GRAY_IMAGE;
  else if (type == INDEXED) gdtype = INDEXED_IMAGE;
  else gdtype = RGB_IMAGE;

  image_ID = gimp_image_new (width, height, type);
  tmp = g_strdup_printf ("%s-pg%ld", filename, (long)pagenum);
  gimp_image_set_filename (image_ID, tmp);
  g_free (tmp);

  *layer_ID = gimp_layer_new (image_ID, "Background", width, height,
			      gdtype, 100, NORMAL_MODE);
  gimp_image_add_layer (image_ID, *layer_ID, 0);

  *drawable = gimp_drawable_get (*layer_ID);
  gimp_pixel_rgn_init (pixel_rgn, *drawable, 0, 0, (*drawable)->width,
		       (*drawable)->height, TRUE, FALSE);

  return (image_ID);
}


/* Skip PNM image generated from PostScript file. */
/* Returns 0 on success, -1 on failure. */
static gint32
skip_ps (FILE *ifp)
{
  register int k, c;
  int i, pnmtype, width, height, maxval, bpl;

  pnmtype = read_pnmraw_type (ifp, &width, &height, &maxval);

  if (pnmtype == 4)    /* Portable bitmap */
    bpl = (width + 7)/8;
  else if (pnmtype == 5)
    bpl = width;
  else if (pnmtype == 6)
    bpl = width*3;
  else
    return (-1);

  for (i = 0; i < height; i++)
    {
      k = bpl;  c = EOF;
      while (k-- > 0) c = getc (ifp);
      if (c == EOF) return (-1);

      if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
	gimp_progress_update ((double)(i+1) / (double)height);
    }

  return (0);
}


/* Load PNM image generated from PostScript file */
static gint32
load_ps (gchar *filename,
         guint  pagenum,
         FILE  *ifp,
         gint   llx,
         gint   lly,
         gint   urx,
         gint   ury)
{
  register guchar *dest;
  guchar *data, *bitline = NULL, *byteline = NULL, *byteptr, *temp;
  guchar bit2byte[256*8];
  int width, height, tile_height, scan_lines, total_scan_lines;
  int image_width, image_height;
  int skip_left, skip_bottom;
  int i, j, pnmtype, maxval, bpp, nread;
  GImageType imagetype;
  gint32 layer_ID, image_ID;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
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
      imagetype = INDEXED;
      nread = (width+7)/8;
      bpp = 1;
      bitline = (guchar *)g_malloc (nread);
      byteline = (guchar *)g_malloc (nread*8);

      /* Get an array for mapping 8 bits in a byte to 8 bytes */
      temp = bit2byte;
      for (j = 0; j < 256; j++)
	for (i = 7; i >= 0; i--)
	  *(temp++) = ((j & (1 << i)) != 0);
    }
  else if (pnmtype == 5)  /* Portable Greymap */
    {
      imagetype = GRAY;
      nread = width;
      bpp = 1;
      byteline = (unsigned char *)g_malloc (nread);
    }
  else if (pnmtype == 6)  /* Portable Pixmap */
    {
      imagetype = RGB;
      nread = width * 3;
      bpp = 3;
      byteline = (guchar *)g_malloc (nread);
    }
  else return (-1);

  image_ID = create_new_image (filename, pagenum,
			       image_width, image_height, imagetype,
			       &layer_ID, &drawable, &pixel_rgn);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * image_width * bpp);

  dest = data;
  total_scan_lines = scan_lines = 0;

  if (pnmtype == 4)   /* Read bitimage ? Must be mapped to indexed */
    {static unsigned char BWColorMap[2*3] = { 255, 255, 255, 0, 0, 0 };

    gimp_image_set_cmap (image_ID, BWColorMap, 2);

    for (i = 0; i < height; i++)
      {
	e = (fread (bitline, 1, nread, ifp) != nread);
	if (total_scan_lines >= image_height) continue;
	err |= e;
	if (err) break;

	j = width;      /* Map 1 byte of bitimage to 8 bytes of indexed image */
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

	if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
	  gimp_progress_update ((double)(i+1) / (double)image_height);

	if ((scan_lines == tile_height) || ((i+1) == image_height))
	  {
	    gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0, i-scan_lines+1,
				     image_width, scan_lines);
	    scan_lines = 0;
	    dest = data;
	  }
	if (err) break;
      }
    }
  else   /* Read gray/rgb-image */
    {
      for (i = 0; i < height; i++)
	{
	  e = (fread (byteline, bpp, width, ifp) != width);
	  if (total_scan_lines >= image_height) continue;
	  err |= e;
	  if (err) break;

	  memcpy (dest, byteline+skip_left*bpp, image_width*bpp);
	  dest += image_width*bpp;
	  scan_lines++;
	  total_scan_lines++;

	  if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
	    gimp_progress_update ((double)(i+1) / (double)image_height);

	  if ((scan_lines == tile_height) || ((i+1) == image_height))
	    {
	      gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0, i-scan_lines+1,
				       image_width, scan_lines);
	      scan_lines = 0;
	      dest = data;
	    }
	  if (err) break;
	}
    }

  g_free (data);
  if (byteline) g_free (byteline);
  if (bitline) g_free (bitline);

  if (err)
    g_message ("EOF encountered on reading");

  gimp_drawable_flush (drawable);

  return (err ? -1 : image_ID);
}


/* Write out the PostScript file header */
static void save_ps_header (FILE  *ofp,
                            gchar *filename)
{
  time_t cutime = time (NULL);

  fprintf (ofp, "%%!PS-Adobe-3.0%s\n", psvals.eps ? " EPSF-3.0" : "");
  fprintf (ofp, "%%%%Creator: GIMP PostScript file plugin V %4.2f \
by Peter Kirchgessner\n", VERSIO);
  fprintf (ofp, "%%%%Title: %s\n", filename);
  fprintf (ofp, "%%%%CreationDate: %s", ctime (&cutime));
  fprintf (ofp, "%%%%DocumentData: Clean7Bit\n");
  if (psvals.eps || (psvals.level > 1)) fprintf (ofp,"%%%%LanguageLevel: 2\n");
  fprintf (ofp, "%%%%Pages: 1\n");
}


/* Write out transformation for image */
static void
save_ps_setup (FILE   *ofp,
               gint32  drawable_ID,
               gint    width,
               gint    height,
               gint    bpp)
{
  double x_offset, y_offset, x_size, y_size;
  double x_scale, y_scale;
  double width_inch, height_inch;
  double f1, f2, dx, dy;
  int xtrans, ytrans;

  /* initialize */

  dx = 0.0;
  dy = 0.0;

  x_offset = psvals.x_offset;
  y_offset = psvals.y_offset;
  width_inch = fabs (psvals.width);
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
	height_inch = width_inch * (double)(height)/(double)(width);
      else
	width_inch = fabs (height_inch) * (double)(width)/(double)(height);
    }
  if ((psvals.rotate == 0) || (psvals.rotate == 180))
    {
      x_size = width_inch; y_size = height_inch;
    }
  else
  {
    y_size = width_inch; x_size = height_inch;
  }

  fprintf (ofp, "%%%%BoundingBox: %d %d %d %d\n",(int)(x_offset*72.0),
           (int)(y_offset*72.0), (int)((x_offset+x_size)*72.0)+1,
           (int)((y_offset+y_size)*72.0)+1);
  fprintf (ofp, "%%%%EndComments\n");

  if (psvals.preview && (psvals.preview_size > 0))
    {
      save_ps_preview (ofp, drawable_ID);
    }

  fprintf (ofp, "%%%%BeginProlog\n");
  fprintf (ofp, "%% Use own dictionary to avoid conflicts\n");
  fprintf (ofp, "5 dict begin\n");
  fprintf (ofp, "%%%%EndProlog\n");
  fprintf (ofp, "%%%%Page: 1 1\n");
  fprintf (ofp, "%% Translate for offset\n");
  fprintf (ofp, "%f %f translate\n", x_offset*72.0, y_offset*72.0);

  /* Calculate translation to startpoint of first scanline */
  switch (psvals.rotate)
    {
    case   0: dx = 0.0; dy = y_size*72.0;
      break;
    case  90: dx = dy = 0.0;
      x_scale = 72.0 * width_inch;
      y_scale = -72.0 * height_inch;
      break;
    case 180: dx = x_size*72.0; dy = 0.0;
      break;
    case 270: dx = x_size*72.0; dy = y_size*72.0;
      break;
    }
  if ((dx != 0.0) || (dy != 0.0))
    fprintf (ofp, "%% Translate to begin of first scanline\n%f %f translate\n",
             dx, dy);
  if (psvals.rotate)
    fprintf (ofp, "%d rotate\n", (int)psvals.rotate);
  fprintf (ofp, "%f %f scale\n", 72.0*width_inch, -72.0*height_inch);

  /* Write the PostScript procedures to read the image */
  if (psvals.level <= 1)
  {
    fprintf (ofp, "%% Variable to keep one line of raster data\n");
    if (bpp == 1)
      fprintf (ofp, "/scanline %d string def\n", (width+7)/8);
    else
      fprintf (ofp, "/scanline %d %d mul string def\n", width, bpp/8);
  }
  fprintf (ofp, "%% Image geometry\n%d %d %d\n", width, height,
           (bpp == 1) ? 1 : 8);
  fprintf (ofp, "%% Transformation matrix\n");
  xtrans = ytrans = 0;
  if (psvals.width < 0.0) { width = -width; xtrans = -width; }
  if (psvals.height < 0.0) { height = -height; ytrans = -height; }
  fprintf (ofp, "[ %d 0 0 %d %d %d ]\n", width, height, xtrans, ytrans);
}


static void
save_ps_trailer (FILE *ofp)
{
  fprintf (ofp, "%%%%Trailer\n");
  fprintf (ofp, "end\n%%%%EOF\n");
}

/* Do a Floyd-Steinberg dithering on a greyscale scanline. */
/* linecount must keep the counter for the actual scanline (0, 1, 2, ...). */
/* If linecount is less than zero, all used memory is freed. */

static void
dither_grey (guchar *grey,
             guchar *bw,
             gint    npix,
             gint    linecount)
{
  register guchar *greyptr, *bwptr, mask;
  register int *fse;
  int x, greyval, fse_inline;
  static int *fs_error = NULL;
  static int do_init_arrays = 1;
  static int limit_array[1278];
  static int east_error[256],seast_error[256],south_error[256],swest_error[256];
  int *limit = &(limit_array[512]);

  if (linecount <= 0)
    {
      if (fs_error) g_free (fs_error-1);
      if (linecount < 0) return;
      fs_error = (int *)g_malloc ((npix+2)*sizeof (int));
      memset ((char *)fs_error, 0, (npix+2)*sizeof (int));
      fs_error++;

      /* Initialize some arrays that speed up dithering */
      if (do_init_arrays)
	{
	  do_init_arrays = 0;
	  for (x = -511; x <= 766; x++)
	    limit[x] = (x < 0) ? 0 : ((x > 255) ? 255 : x);
	  for (greyval = 0; greyval < 256; greyval++)
	    {
	      east_error[greyval] = (greyval < 128) ? ((greyval * 79) >> 8)
		: (((greyval-255)*79) >> 8);
	      seast_error[greyval] = (greyval < 128) ? ((greyval * 34) >> 8)
		: (((greyval-255)*34) >> 8);
	      south_error[greyval] = (greyval < 128) ? ((greyval * 56) >> 8)
		: (((greyval-255)*56) >> 8);
	      swest_error[greyval] = (greyval < 128) ? ((greyval * 12) >> 8)
		: (((greyval-255)*12) >> 8);
	    }
	}
    }
  if (fs_error == NULL) return;

  memset (bw, 0, (npix+7)/8); /* Initialize with white */

  greyptr = grey;
  bwptr = bw;
  mask = 0x80;
  fse_inline = fs_error[0];
  for (x = 0, fse = fs_error; x < npix; x++, fse++)
    {
      greyval = limit[*(greyptr++) + fse_inline];  /* 0 <= greyval <= 255 */
      if (greyval < 128) *bwptr |= mask;  /* Set a black pixel */

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

/* Write a device independant screen preview */
static void
save_ps_preview (FILE   *ofp,
                 gint32  drawable_ID)
{
  register guchar *bwptr, *greyptr;
  GDrawableType drawable_type;
  GDrawable *drawable;
  GPixelRgn src_rgn;
  int width, height, x, y, nbsl, out_count;
  int nchar_pl = 72, src_y;
  double f1, f2;
  guchar *grey, *bw, *src_row, *src_ptr;
  guchar *cmap;
  gint ncols, cind;

  if (psvals.preview_size <= 0) return;

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);

  /* Calculate size of preview */
  if (   (drawable->width <= psvals.preview_size)
	 && (drawable->height <= psvals.preview_size))
    {
      width = drawable->width;
      height = drawable->height;
    }
  else
    {
      f1 = (double)psvals.preview_size / (double)drawable->width;
      f2 = (double)psvals.preview_size / (double)drawable->height;
      if (f1 < f2)
	{
	  width = psvals.preview_size;
	  height = drawable->height * f1;
	  if (height <= 0) height = 1;
	}
      else
	{
	  height = psvals.preview_size;
	  width = drawable->width * f1;
	  if (width <= 0) width = 1;
	}
    }

  nbsl = (width+7)/8;  /* Number of bytes per scanline in bitmap */

  grey = (guchar *)g_malloc (width);
  bw = (guchar *)g_malloc (nbsl);
  src_row = (guchar *)g_malloc (drawable->width * drawable->bpp);

  fprintf (ofp, "%%%%BeginPreview: %d %d 1 %d\n", width, height,
	   ((nbsl*2+nchar_pl-1)/nchar_pl)*height);

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, drawable->width,
		       drawable->height, FALSE, FALSE);

  cmap = NULL;     /* Check if we need a colour table */
  if (gimp_drawable_type (drawable_ID) == INDEXED_IMAGE)
    cmap = (guchar *)
      gimp_image_get_cmap (gimp_drawable_image_id (drawable_ID), &ncols);

  for (y = 0; y < height; y++)
    {
      /* Get a scanline from the input image and scale it to the desired width */
      src_y = (y * drawable->height) / height;
      gimp_pixel_rgn_get_row (&src_rgn, src_row, 0, src_y, drawable->width);

      greyptr = grey;
      if (drawable->bpp == 3)   /* RGB-image */
	{
	  for (x = 0; x < width; x++)
	    {                       /* Convert to grey */
	      src_ptr = src_row + ((x * drawable->width) / width) * 3;
	      *(greyptr++) = (3*src_ptr[0] + 6*src_ptr[1] + src_ptr[2]) / 10;
	    }
	}
      else if (cmap)    /* Indexed image */
	{
	  for (x = 0; x < width; x++)
	    {
	      src_ptr = src_row + ((x * drawable->width) / width);
	      cind = *src_ptr;   /* Get colour index and convert to grey */
	      src_ptr = (cind >= ncols) ? cmap : (cmap + 3*cind);
	      *(greyptr++) = (3*src_ptr[0] + 6*src_ptr[1] + src_ptr[2]) / 10;
	    }
	}
      else             /* Grey image */
	{
	  for (x = 0; x < width; x++)
	    *(greyptr++) = *(src_row + ((x * drawable->width) / width));
	}

      /* Now we have a greyscale line for the desired width. */
      /* Dither it to b/w */
      dither_grey (grey, bw, width, y);

      /* Write out the b/w line */
      out_count = 0;
      bwptr = bw;
      for (x = 0; x < nbsl; x++)
	{
	  if (out_count == 0) fprintf (ofp, "%% ");
	  fprintf (ofp, "%02x", *(bwptr++));
	  out_count += 2;
	  if (out_count >= nchar_pl)
	    {
	      fprintf (ofp, "\n");
	      out_count = 0;
	    }
	}
      if (out_count != 0)
	fprintf (ofp, "\n");

      if ((l_run_mode != RUN_NONINTERACTIVE) && ((y % 20) == 0))
	gimp_progress_update ((double)(y) / (double)height);
    }

  fprintf (ofp, "%%%%EndPreview\n");

  dither_grey (grey, bw, width, -1);
  g_free (src_row);
  g_free (bw);
  g_free (grey);

  gimp_drawable_detach (drawable);
}

static gint
save_gray  (FILE   *ofp,
            gint32  image_ID,
            gint32  drawable_ID)
{
  int height, width, i, j;
  int tile_height;
  unsigned char *data, *src;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  static char *hex = "0123456789abcdef";
  int level2 = (psvals.level > 1);

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  width = drawable->width;
  height = drawable->height;
  tile_height = gimp_tile_height ();
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (guchar *)g_malloc (tile_height * width * drawable->bpp);

  /* Set up transformation in PostScript */
  save_ps_setup (ofp, drawable_ID, width, height, 1*8);

  /* Write read image procedure */
  if (!level2)
  {
    fprintf (ofp, "{ currentfile scanline readhexstring pop }\n");
  }
  else
  {
    fprintf (ofp, "currentfile /ASCII85Decode filter\n");
    ascii85_init ();
  }
  fprintf (ofp, "image\n");

#define GET_GRAY_TILE(begin) \
  {int scan_lines; \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gimp_pixel_rgn_get_rect (&pixel_rgn, begin, 0, i, width, scan_lines); \
    src = begin; }

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0) GET_GRAY_TILE (data); /* Get more data */
      if (!level2)
	{
	  for (j = 0; j < width; j++)
	    {
	      putc (hex[(*src) >> 4], ofp);
	      putc (hex[(*(src++)) & 0x0f], ofp);
	      if (((j+1) % 39) == 0) putc ('\n', ofp);
	    }
	  putc ('\n', ofp);
	}
      else
	{
	  for (j = 0; j < width; j++)
	    ascii85_out (*(src++), ofp);
	}
      if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
	gimp_progress_update ((double) i / (double) height);
    }
  if (level2) ascii85_done (ofp);
  fprintf (ofp, "showpage\n");
  g_free (data);

  gimp_drawable_detach (drawable);

  if (ferror (ofp))
    {
      g_message (_("write error occured"));
      return (FALSE);
    }
  return (TRUE);
#undef GET_GRAY_TILE
}


static gint
save_bw (FILE   *ofp,
         gint32  image_ID,
         gint32  drawable_ID)
{
  int height, width, i, j;
  int ncols, nbsl, nwrite;
  int tile_height;
  guchar *cmap, *ct;
  guchar *data, *src;
  guchar *scanline, *dst, mask;
  guchar *hex_scanline;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  static char *hex = "0123456789abcdef";
  int level2 = (psvals.level > 1);

  cmap = gimp_image_get_cmap (image_ID, &ncols);

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  width = drawable->width;
  height = drawable->height;
  tile_height = gimp_tile_height ();
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (guchar *)g_malloc (tile_height * width * drawable->bpp);
  nbsl = (width+7)/8;
  scanline = (char *)g_malloc (nbsl + 1);
  hex_scanline = (char *)g_malloc ((nbsl + 1)*2);

  /* Set up transformation in PostScript */
  save_ps_setup (ofp, drawable_ID, width, height, 1);

  /* Write read image procedure */
  if (!level2)
  {
    fprintf (ofp, "{ currentfile scanline readhexstring pop }\n");
  }
  else
  {
    fprintf (ofp, "currentfile /ASCII85Decode filter\n");
    ascii85_init ();
  }
  fprintf (ofp, "image\n");

#define GET_BW_TILE(begin) \
  {int scan_lines; \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gimp_pixel_rgn_get_rect (&pixel_rgn, begin, 0, i, width, scan_lines); \
    src = begin; }

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0) GET_BW_TILE (data); /* Get more data */
      dst = scanline;
      memset (dst, 0, nbsl);
      mask = 0x80;
      /* Build a bitmap for a scanline */
      for (j = 0; j < width; j++)
	{
	  ct = cmap + *(src++)*3;
	  if (ct[0] || ct[1] || ct[2])
	    *dst |= mask;
	  if (mask == 0x01) { mask = 0x80; dst++; } else mask >>= 1;
	}
      if (!level2)
	{
	  /* Convert to hexstring */
	  for (j = 0; j < nbsl; j++)
	    {
	      hex_scanline[j*2] = (unsigned char)hex[scanline[j] >> 4];
	      hex_scanline[j*2+1] = (unsigned char)hex[scanline[j] & 0x0f];
	    }
	  /* Write out hexstring */
	  j = nbsl * 2;
	  dst = hex_scanline;
	  while (j > 0)
	    {
	      nwrite = (j > 78) ? 78 : j;
	      fwrite (dst, nwrite, 1, ofp);
	      putc ('\n', ofp);
	      j -= nwrite;
	      dst += nwrite;
	    }
	}
      else
	{
	  dst = scanline;
	  for (j = 0; j < nbsl; j++)
	    ascii85_out (*(dst++), ofp);
	}
      if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
	gimp_progress_update ((double) i / (double) height);
    }
  if (level2) ascii85_done (ofp);
  fprintf (ofp, "showpage\n");

  g_free (hex_scanline);
  g_free (scanline);
  g_free (data);

  gimp_drawable_detach (drawable);

  if (ferror (ofp))
    {
      g_message (_("write error occured"));
      return (FALSE);
    }
  return (TRUE);
#undef GET_BW_TILE
}


static gint
save_index (FILE   *ofp,
            gint32  image_ID,
            gint32  drawable_ID)
{
  int height, width, i, j;
  int ncols, bw;
  int tile_height;
  unsigned char *cmap, *cmap_start;
  unsigned char *data, *src;
  char coltab[256*6], *ct;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  static char *hex = "0123456789abcdef";
  static char *background = "000000";
  int level2 = (psvals.level > 1);

  cmap = cmap_start = gimp_image_get_cmap (image_ID, &ncols);

  ct = coltab;
  bw = 1;
  for (j = 0; j < 256; j++)
    {
      if (j >= ncols)
	{
	  memcpy (ct, background, 6);
	  ct += 6;
	}
      else
	{
	  bw &=    ((cmap[0] == 0) && (cmap[1] == 0) && (cmap[2] == 0))
            || ((cmap[0] == 255) && (cmap[1] == 255) && (cmap[2] == 255));
	  *(ct++) = (guchar)hex[(*cmap) >> 4];
	  *(ct++) = (guchar)hex[(*(cmap++)) & 0x0f];
	  *(ct++) = (guchar)hex[(*cmap) >> 4];
	  *(ct++) = (guchar)hex[(*(cmap++)) & 0x0f];
	  *(ct++) = (guchar)hex[(*cmap) >> 4];
	  *(ct++) = (guchar)hex[(*(cmap++)) & 0x0f];
	}
    }
  if (bw) return (save_bw (ofp, image_ID, drawable_ID));

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  width = drawable->width;
  height = drawable->height;
  tile_height = gimp_tile_height ();
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (guchar *)g_malloc (tile_height * width * drawable->bpp);

  /* Set up transformation in PostScript */
  save_ps_setup (ofp, drawable_ID, width, height, 3*8);

  /* Write read image procedure */
  if (!level2)
  {
    fprintf (ofp, "{ currentfile scanline readhexstring pop } false 3\n");
  }
  else
  {
    fprintf (ofp, "currentfile /ASCII85Decode filter false 3\n");
    ascii85_init ();
  }
  fprintf (ofp, "colorimage\n");

#define GET_INDEX_TILE(begin) \
  {int scan_lines; \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gimp_pixel_rgn_get_rect (&pixel_rgn, begin, 0, i, width, scan_lines); \
    src = begin; }

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0) GET_INDEX_TILE (data); /* Get more data */
      if (!level2)
	{
	  for (j = 0; j < width; j++)
	    {
	      fwrite (coltab+(*(src++))*6, 6, 1, ofp);
	      if (((j+1) % 13) == 0) putc ('\n', ofp);
	    }
	  putc ('\n', ofp);
	}
      else
	{
	  for (j = 0; j < width; j++)
	    {
	      cmap = cmap_start + 3 * (*(src++));
	      ascii85_out (*(cmap++), ofp);
	      ascii85_out (*(cmap++), ofp);
	      ascii85_out (*(cmap++), ofp);
	    }
	}
      if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
	gimp_progress_update ((double) i / (double) height);
    }
  if (level2) ascii85_done (ofp);
  fprintf (ofp, "showpage\n");

  g_free (data);

  gimp_drawable_detach (drawable);

  if (ferror (ofp))
    {
      g_message (_("write error occured"));
      return (FALSE);
    }
  return (TRUE);
#undef GET_INDEX_TILE
}


static gint
save_rgb (FILE   *ofp,
          gint32  image_ID,
          gint32  drawable_ID)
{
  int height, width, tile_height;
  int i, j;
  guchar *data, *src;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  static char *hex = "0123456789abcdef";
  int level2 = (psvals.level > 1);

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  width = drawable->width;
  height = drawable->height;
  tile_height = gimp_tile_height ();
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (guchar *)g_malloc (tile_height * width * drawable->bpp);

  /* Set up transformation in PostScript */
  save_ps_setup (ofp, drawable_ID, width, height, 3*8);

  /* Write read image procedure */
  if (!level2)
  {
    fprintf (ofp, "{ currentfile scanline readhexstring pop } false 3\n");
  }
  else
  {
    fprintf (ofp, "currentfile /ASCII85Decode filter false 3\n");
    ascii85_init ();
  }
  fprintf (ofp, "colorimage\n");

#define GET_RGB_TILE(begin) \
  {int scan_lines; \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gimp_pixel_rgn_get_rect (&pixel_rgn, begin, 0, i, width, scan_lines); \
    src = begin; }

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0) GET_RGB_TILE (data); /* Get more data */
      if (!level2)
	{
	  for (j = 0; j < width; j++)
	    {
	      putc (hex[(*src) >> 4], ofp);        /* Red */
	      putc (hex[(*(src++)) & 0x0f], ofp);
	      putc (hex[(*src) >> 4], ofp);        /* Green */
	      putc (hex[(*(src++)) & 0x0f], ofp);
	      putc (hex[(*src) >> 4], ofp);        /* Blue */
	      putc (hex[(*(src++)) & 0x0f], ofp);
	      if (((j+1) % 13) == 0) putc ('\n', ofp);
	    }
	  putc ('\n', ofp);
	}
      else
	{
	  for (j = 0; j < width; j++)
	    {
	      ascii85_out (*(src++), ofp);
	      ascii85_out (*(src++), ofp);
	      ascii85_out (*(src++), ofp);
	    }
	}
      if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
	gimp_progress_update ((double) i / (double) height);
    }
  if (level2) ascii85_done (ofp);
  fprintf (ofp, "showpage\n");
  g_free (data);

  gimp_drawable_detach (drawable);

  if (ferror (ofp))
    {
      g_message (_("write error occured"));
      return (FALSE);
    }
  return (TRUE);
#undef GET_RGB_TILE
}

static void 
init_gtk (void)
{
  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("ps");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
}

/*  Load interface functions  */

static gint
load_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *pages_entry;
  GtkWidget *toggle;

  init_gtk ();

  dialog = gimp_dialog_new (_("Load PostScript"), "ps",
			    gimp_plugin_help_func, "filters/ps.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    _("OK"), load_ok_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);

  main_vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), main_vbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Rendering */
  frame = gtk_frame_new (_("Rendering"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Resolution/Width/Height/Pages labels */
  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adj, plvals.resolution,
				     5, 1440, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Resolution:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &plvals.resolution);

  spinbutton = gimp_spin_button_new (&adj, plvals.width,
				     1, GIMP_MAX_IMAGE_SIZE, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Width:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &plvals.width);

  spinbutton = gimp_spin_button_new (&adj, plvals.height,
				     1, GIMP_MAX_IMAGE_SIZE, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Height:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &plvals.height);

  pages_entry = gtk_entry_new ();
  gtk_widget_set_usize (pages_entry, 80, 0);
  gtk_entry_set_text (GTK_ENTRY (pages_entry), plvals.pages);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Pages:"), 1.0, 0.5,
			     pages_entry, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (pages_entry), "changed",
		      GTK_SIGNAL_FUNC (load_pages_entry_callback),
		      NULL);

  toggle = gtk_check_button_new_with_label (_("Try Bounding Box"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                      &plvals.use_bbox);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), plvals.use_bbox);
  gtk_widget_show (toggle);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Colouring */
  frame = gimp_radio_group_new2 (TRUE, _("Coloring"),
				 gimp_radio_button_update,
				 &plvals.pnm_type, (gpointer) plvals.pnm_type,

				 _("B/W"),       (gpointer) 4, NULL,
				 _("Gray"),      (gpointer) 5, NULL,
				 _("Color"),     (gpointer) 6, NULL,
				 _("Automatic"), (gpointer) 7, NULL,

				 NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (TRUE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gimp_radio_group_new2 (TRUE, _("Text Antialiasing"),
				 gimp_radio_button_update,
				 &plvals.textalpha, (gpointer) plvals.textalpha,

				 _("None"),   (gpointer) 1, NULL,
				 _("Weak"),   (gpointer) 2, NULL,
				 _("Strong"), (gpointer) 4, NULL,

				 NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  frame = gimp_radio_group_new2 (TRUE, _("Graphic Antialiasing"),
				 gimp_radio_button_update,
				 &plvals.graphicsalpha,
				 (gpointer) plvals.graphicsalpha,

				 _("None"),   (gpointer) 1, NULL,
				 _("Weak"),   (gpointer) 2, NULL,
				 _("Strong"), (gpointer) 4, NULL,

				 NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  gtk_main ();
  gdk_flush ();

  return plint.run;
}


static void
load_ok_callback (GtkWidget *widget,
                  gpointer   data)
{
  plint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
load_pages_entry_callback (GtkWidget *widget,
			   gpointer   data)
{
  gint nelem = sizeof (plvals.pages);

  strncpy (plvals.pages, gtk_entry_get_text (GTK_ENTRY (widget)), nelem);
  plvals.pages[nelem-1] = '\0';
}

/*  Save interface functions  */

static gint
save_dialog (void)
{
  SaveDialogVals *vals;
  GtkWidget *dialog;
  GtkWidget *toggle;
  GtkWidget *frame, *uframe;
  GtkWidget *hbox, *vbox;
  GtkWidget *main_vbox[2];
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;
  gint       j;

  vals = g_new (SaveDialogVals, 1);

  vals->level = (psvals.level > 1);

  dialog = gimp_dialog_new (_("Save as PostScript"), "ps",
			    gimp_plugin_help_func, "filters/ps.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    _("OK"), save_ok_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);

  /* Main hbox */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox,
                      FALSE, FALSE, 0);
  main_vbox[0] = main_vbox[1] = NULL;

  for (j = 0; j < sizeof (main_vbox) / sizeof (main_vbox[0]); j++)
    {
      main_vbox[j] = gtk_vbox_new (FALSE, 4);
      gtk_box_pack_start (GTK_BOX (hbox), main_vbox[j], TRUE, TRUE, 0);
    }

  /* Image Size */
  frame = gtk_frame_new (_("Image Size"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (main_vbox[0]), frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Width/Height/X-/Y-offset labels */
  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&vals->adjustment[0], psvals.width,
				     1e-5, GIMP_MAX_IMAGE_SIZE, 1, 10, 0, 1, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Width:"), 1.0, 0.5,
			     spinbutton, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (vals->adjustment[0]), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &psvals.width);

  spinbutton = gimp_spin_button_new (&vals->adjustment[1], psvals.height,
				     1e-5, GIMP_MAX_IMAGE_SIZE, 1, 10, 0, 1, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Height:"), 1.0, 0.5,
			     spinbutton, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (vals->adjustment[1]), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &psvals.height);

  spinbutton = gimp_spin_button_new (&vals->adjustment[2], psvals.x_offset,
				     1e-5, GIMP_MAX_IMAGE_SIZE, 1, 10, 0, 1, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("X-Offset:"), 1.0, 0.5,
			     spinbutton, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (vals->adjustment[2]), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &psvals.x_offset);

  spinbutton = gimp_spin_button_new (&vals->adjustment[3], psvals.y_offset,
				     1e-5, GIMP_MAX_IMAGE_SIZE, 1, 10, 0, 1, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Y-Offset:"), 1.0, 0.5,
			     spinbutton, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (vals->adjustment[3]), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &psvals.y_offset);

  toggle = gtk_check_button_new_with_label (_("Keep Aspect Ratio"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                      &psvals.keep_ratio);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), psvals.keep_ratio);
  gtk_widget_show (toggle);

  /* Unit */
  uframe = gimp_radio_group_new2 (TRUE, _("Unit"),
				  save_unit_toggle_update,
				  vals, (gpointer) psvals.unit_mm,

				  _("Inch"),       (gpointer) FALSE, NULL,
				  _("Millimeter"), (gpointer) TRUE, NULL,

				  NULL);
  gtk_box_pack_end (GTK_BOX (vbox), uframe, FALSE, FALSE, 0);
  gtk_widget_show (uframe);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Rotation */
  frame = gimp_radio_group_new2 (TRUE, _("Rotation"),
				 gimp_radio_button_update,
				 &psvals.rotate, (gpointer) psvals.rotate,

				 "0",   (gpointer) 0, NULL,
				 "90",  (gpointer) 90, NULL,
				 "180", (gpointer) 180, NULL,
				 "270", (gpointer) 270, NULL,

				 NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox[1]), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* Format */
  frame = gtk_frame_new (_("Output"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (main_vbox[1]), frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label (_("PostScript Level 2"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                      &(vals->level));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), vals->level);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Encapsulated PostScript"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                      &psvals.eps);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), psvals.eps);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                      &psvals.preview);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), psvals.preview);
  gtk_widget_show (toggle);

  /* Preview size label/entry */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  gtk_object_set_data (GTK_OBJECT (toggle), "set_sensitive", table);
  gtk_widget_set_sensitive (table, psvals.preview);

  spinbutton = gimp_spin_button_new (&adj, psvals.preview_size,
				     0, 1024, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Preview Size:"), 1.0, 0.5,
			     spinbutton, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &psvals.preview_size);
  gtk_widget_show (spinbutton);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  for (j = 0; j < sizeof (main_vbox) / sizeof (main_vbox[0]); j++)
    gtk_widget_show (main_vbox[j]);

  gtk_widget_show (hbox);
  gtk_widget_show (dialog);

  gtk_main ();
  gdk_flush ();

  psvals.level = (vals->level < 1);

  g_free (vals);

  return psint.run;
}

static void
save_ok_callback (GtkWidget *widget,
                  gpointer   data)
{
  psint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
save_unit_toggle_update (GtkWidget *widget,
			 gpointer   data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      SaveDialogVals *vals;
      gdouble factor;
      gdouble value;
      gint    unit_mm;
      gint    i;

      vals    = (SaveDialogVals *) data;
      unit_mm = (gint) gtk_object_get_user_data (GTK_OBJECT (widget));

      psvals.unit_mm = unit_mm;

      if (unit_mm)
	factor = 25.4;
      else
	factor = 1.0 / 25.4;

      for (i = 0; i < 4; i++)
	{
	  value = GTK_ADJUSTMENT (vals->adjustment[i])->value * factor;

	  gtk_adjustment_set_value (GTK_ADJUSTMENT (vals->adjustment[i]), value);
	}
    }
}
