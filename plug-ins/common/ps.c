/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * PostScript file plugin
 * PostScript writing and GhostScript interfacing code
 * Copyright (C) 1997 Peter Kirchgessner
 * (email: pkirchg@aol.com, WWW: http://members.aol.com/pkirchg)
 *
 * Added controls for TextAlphaBits and GraphicsAlphaBits
 *   George White <aa056@chebucto.ns.ca>
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
 */
#define VERSIO                                               1.04
static char dversio[] =                                    "v1.04  20-Dec-97";
static char ident[] = "@(#) GIMP PostScript/PDF file-plugin v1.04  20-Dec-97";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#define STR_LENGTH 64

/* Load info */
typedef struct
{
  guint resolution;     /* resolution (dpi) at which to run ghostscript */
  guint width, height;  /* desired size (ghostscript may ignore this) */
  gint use_bbox;        /* 0: use width/height, 1: try to use BoundingBox */
  char pages[STR_LENGTH];/* Pages to load (eg.: 1,3,5-7) */
  gint pnm_type;        /* 4: pbm, 5: pgm, 6: ppm, 7: automatic */
  gint textalpha;       /* antialiasing: 1,2, or 4 TextAlphaBits */
  gint graphicsalpha;   /* antialiasing: 1,2, or 4 GraphicsAlphaBits */
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
  gint unit_mm;               /* Unit of measure (0: inch, 1: mm) */
  gint keep_ratio;            /* Keep aspect ratio */
  gint rotate;                /* Rotation (0, 90, 180, 270) */
  gint eps;                   /* Encapsulated PostScript flag */
  gint preview;               /* Preview Flag */
  gint preview_size;          /* Preview size */
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
  90,             /* Rotate */
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
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);

static gint32 load_image (char *filename);
static gint   save_image (char *filename,
                          gint32  image_ID,
                          gint32  drawable_ID);

static gint save_gray  (FILE *ofp,
                        gint32 image_ID,
                        gint32 drawable_ID);
static gint save_index (FILE *ofp,
                        gint32 image_ID,
                        gint32 drawable_ID);
static gint save_rgb   (FILE *ofp,
                        gint32 image_ID,
                        gint32 drawable_ID);

static gint32 create_new_image (char *filename, guint pagenum,
                guint width, guint height,
                GImageType type, gint32 *layer_ID, GDrawable **drawable,
                GPixelRgn *pixel_rgn);

static void   check_load_vals (void);
static void   check_save_vals (void);

static char  *ftoa (char *format, double r);

static gint   page_in_list (char *list, guint pagenum);

static gint   get_bbox (char *filename,
                        int *x0, int *y0, int *x1, int *y1);

static FILE  *ps_open (char *filename,
                       const PSLoadVals *loadopt,
                       int *llx, int* lly, int* urx, int* ury);

static void   ps_close (FILE *ifp);

static gint32 skip_ps (FILE *ifp);

static gint32 load_ps (char *filename,
                       guint pagenum,
                       FILE *ifp,
                       int llx, int lly, int urx, int ury);

static void save_ps_header (FILE *ofp,
                            char *filename);
static void save_ps_setup (FILE *ofp,
                           gint32 drawable_ID,
                           int width,
                           int height,
                           int bpp);
static void save_ps_trailer (FILE *ofp);
static void save_ps_preview (FILE *ofp,
                             gint32 drawable_ID);
static void dither_grey (unsigned char *grey,
                         unsigned char *bw,
                         int npix,
                         int linecount);


/* Dialog-handling */
typedef struct
{
  GtkWidget *dialog;
  GtkWidget *entry[4];
  int use_bbox;
  int dataformat[4];
  int textalphabits[3];
  int graphicsalphabits[3];
} LoadDialogVals;

static gint   load_dialog              (void);
static void   load_close_callback      (GtkWidget *widget,
                                        gpointer   data);
static void   load_ok_callback         (GtkWidget *widget,
                                        gpointer   data);
static void   load_toggle_update       (GtkWidget *widget,
                                        gpointer   data);

typedef struct
{
  GtkWidget *dialog;
  GtkWidget *entry[4];
  GtkWidget *psize_entry;
  int keep_ratio;
  int unit[2];
  int rot[4];
  int eps;
  int preview;
  int preview_size;
} SaveDialogVals;

static gint   save_dialog              (void);
static void   save_close_callback      (GtkWidget *widget,
                                        gpointer   data);
static void   save_ok_callback         (GtkWidget *widget,
                                        gpointer   data);
static void   save_toggle_update       (GtkWidget *widget,
                                        gpointer   data);
static void   save_mm_toggle_update    (GtkWidget *widget,
                                        gpointer   data);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


/* The run mode */
static GRunModeType l_run_mode;


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
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals)
                               / sizeof (load_return_vals[0]);

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
  static int nset_load_args = sizeof (set_load_args) / sizeof (set_load_args[0]);

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename",
            "The name of the file to save the image in" },
    { PARAM_FLOAT, "width", "Width of the image in PostScript file" },
    { PARAM_FLOAT, "height", "Height of image in PostScript file" },
    { PARAM_FLOAT, "x_offset", "X-offset to image from lower left corner" },
    { PARAM_FLOAT, "y_offset", "Y-offset to image from lower left corner" },
    { PARAM_INT32, "unit", "Unit for width/height/offset. 0: inches, 1: millimeters" },
    { PARAM_INT32, "keep_ratio", "0: use width/height, 1: keep aspect ratio" },
    { PARAM_INT32, "rotation", "0, 90, 180, 270" },
    { PARAM_INT32, "eps_flag", "0: PostScript, 1: Encapsulated PostScript" },
    { PARAM_INT32, "preview", "0: no preview, >0: max. size of preview" }
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_ps_load",
                          "load file of PostScript/PDF file format",
                          "load file of PostScript/PDF file format",
                          "Peter Kirchgessner <pkirchg@aol.com>",
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
                          "Peter Kirchgessner <pkirchg@aol.com>",
                          "Peter Kirchgessner",
                          dversio,
                          NULL,
                          NULL,
                          PROC_PLUG_IN,
                          nset_load_args, 0,
                          set_load_args, NULL);

  gimp_install_procedure ("file_ps_save",
                          "save file in PostScript file format",
                          "PostScript saving handles all image types except \
those with alpha channels.",
                          "Peter Kirchgessner <pkirchg@aol.com>",
                          "Peter Kirchgessner",
                          dversio,
                          "<Save>/PostScript",
                          "RGB*, GRAY*, INDEXED*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  /* Register file plugin by plugin name and handable extensions */
  gimp_register_magic_load_handler ("file_ps_load", "ps,eps,pdf", "",
                                    "0,string,%!,0,string,%PDF");
  gimp_register_save_handler ("file_ps_save", "ps,eps", "");
}


static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)

{
  static GParam values[2];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint32 image_ID = -1;
  int k;

  l_run_mode = run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_ps_load") == 0)
    {
      *nreturn_vals = 2;
      values[1].type = PARAM_IMAGE;
      values[1].data.d_image = -1;

      switch (run_mode)
      {
        case RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data ("file_ps_load", &plvals);

          if (!load_dialog ()) return;
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

        status = (image_ID != -1) ? STATUS_SUCCESS : STATUS_EXECUTION_ERROR;

        /*  Store plvals data  */
        if (status == STATUS_SUCCESS)
          gimp_set_data ("file_ps_load", &plvals, sizeof (PSLoadVals));
      }
      values[0].data.d_status = status;
      values[1].data.d_image = image_ID;
    }
  else if (strcmp (name, "file_ps_save") == 0)
    {
      switch (run_mode)
        {
        case RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data ("file_ps_save", &psvals);

          /* About to save an EPS-file ? Switch on eps-flag in dialog */
          k = strlen (param[3].data.d_string);
          if ((k >= 4) && (strcmp (param[3].data.d_string+k-4, ".eps") == 0))
            psvals.eps = 1;

          /*  First acquire information with a dialog  */
          if (! save_dialog ())
            return;
          break;

        case RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 14)
          {
            status = STATUS_CALLING_ERROR;
          }
          else
          {
            psvals.width = param[5].data.d_float;
            psvals.height = param[6].data.d_float;
            psvals.x_offset = param[7].data.d_float;
            psvals.y_offset = param[8].data.d_float;
            psvals.unit_mm = (param[9].data.d_int32 != 0);
            psvals.keep_ratio = (param[10].data.d_int32 != 0);
            psvals.rotate = param[11].data.d_int32;
            psvals.eps = param[12].data.d_int32;
            psvals.preview = (param[13].data.d_int32 != 0);
            psvals.preview_size = param[13].data.d_int32;
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
        check_save_vals ();
        if (save_image (param[3].data.d_string, param[1].data.d_int32,
                        param[2].data.d_int32))
        {
          /*  Store psvals data  */
          gimp_set_data ("file_ps_save", &psvals, sizeof (PSSaveVals));
        }
        else
        {
          status = STATUS_EXECUTION_ERROR;
        }
      }
      values[0].data.d_status = status;
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
        plvals.width = param[1].data.d_int32;
        plvals.height = param[2].data.d_int32;
        plvals.use_bbox = param[3].data.d_int32;
        if (param[4].data.d_string != NULL)
          strncpy (plvals.pages,param[4].data.d_string,sizeof (plvals.pages));
        else
          plvals.pages[0] = '\0';
        plvals.pages[sizeof (plvals.pages) - 1] = '\0';
        plvals.pnm_type = param[5].data.d_int32;
        plvals.textalpha = param[6].data.d_int32;
        plvals.graphicsalpha = param[7].data.d_int32;
        check_load_vals ();
        gimp_set_data ("file_ps_load", &plvals, sizeof (PSLoadVals));
      }
      values[0].data.d_status = status;
    }
}


static gint32
load_image (char *filename)

{
 gint32 image_ID, *image_list, *nl;
 guint page_count;
 FILE *ifp;
 char *temp,*format;
 int  llx, lly, urx, ury;
 int  k, n_images, max_images, max_pagenum;

#ifdef PS_DEBUG
 printf ("load_image:\n resolution = %d\n", plvals.resolution);
 printf (" %dx%d pixels\n", plvals.width, plvals.height);
 printf (" BoundingBox: %d\n", plvals.use_bbox);
 printf (" Colouring: %d\n", plvals.pnm_type);
 printf (" TextAlphaBits: %d\n", plvals.textalpha);
 printf (" GraphicsAlphaBits: %d\n", plvals.graphicsalpha);
#endif

 /* Try to see if PostScript file is available */
 ifp = fopen (filename, "r");
 if (ifp == NULL)
 {
   g_message ("can't open file for reading");
   return (-1);
 }
 fclose (ifp);

 if (l_run_mode != RUN_NONINTERACTIVE)
 {
   format = "Interpreting and Loading %s:";
   temp = g_malloc (strlen (format) + strlen (filename) + 5);
   sprintf (temp, format, filename);
   gimp_progress_init (temp);
   g_free (temp);
 }

 ifp = ps_open (filename, &plvals, &llx, &lly, &urx, &ury);
 if (!ifp)
 {
   g_message ("can't interprete file");
   return (-1);
 }

 image_list = (gint32 *)g_malloc (10 * sizeof (gint32));
 if (image_list == NULL)
 {
   g_message ("out of memory");
   return (-1);
 }
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
     if (n_images == max_images)
     {
       nl = (gint32 *)g_realloc (image_list, (max_images+10)*sizeof (gint32));
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
save_image (char *filename,
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
    g_message ("PostScript save cannot handle images with alpha channels");
    return FALSE;
  }

  switch (drawable_type)
  {
    case INDEXED_IMAGE:
    case GRAY_IMAGE:
    case RGB_IMAGE:
      break;
    default:
      g_message ("cannot operate on unknown image types");
      return (FALSE);
      break;
  }

  /* Open the output file. */
  ofp = fopen (filename, "wb");
  if (!ofp)
  {
    g_message ("cant open file for writing");
    return (FALSE);
  }

  if (l_run_mode != RUN_NONINTERACTIVE)
  {
    temp = g_malloc (strlen (filename) + 11);
    sprintf (temp, "Saving %s:", filename);
    gimp_progress_init (temp);
    g_free (temp);
  }

  save_ps_header (ofp, filename);

  if (drawable_type == GRAY_IMAGE)
    retval = save_gray (ofp,image_ID, drawable_ID);
  else if (drawable_type == INDEXED_IMAGE)
    retval = save_index (ofp,image_ID, drawable_ID);
  else if (drawable_type == RGB_IMAGE)
    retval = save_rgb (ofp,image_ID, drawable_ID);

  save_ps_trailer (ofp);

  fclose (ofp);

  return (retval);
}


/* Check (and correct) the load values plvals */
static void
check_load_vals (void)

{
  if (plvals.resolution < 5) plvals.resolution = 5;
  else if (plvals.resolution > 1440) plvals.resolution = 1440;

  if (plvals.width < 2) plvals.width = 2;
  if (plvals.height < 2) plvals.height = 2;
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

{int i;

 i = psvals.rotate;
 if ((i != 0) && (i != 90) && (i != 180) && (i != 270))
   psvals.rotate = 90;
 if (psvals.preview_size <= 0) psvals.preview = 0;
}


/* Convert float to ascii for use in labels (cuts off trailing blanks). */
/* The pointer returned is only valid up to the next call of the function. */
static char *ftoa (char *format,
                   double r)

{static char buffer[32];
 register int n;

 sprintf (buffer, format, r);
 n = strlen (buffer)-1;
 while ((n >= 0) && (buffer[n] == ' '))
   buffer[n--] = '\0';
 return (buffer);
}


/* Check if a page is in a given list */
static gint
page_in_list (char *list,
              guint page_num)

{char tmplist[STR_LENGTH], *c0, *c1;
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
get_bbox (char *filename,
          int *x0,
          int *y0,
          int *x1,
          int *y1)

{char line[1024], *src;
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


/* Open the PostScript file. On failure, NULL is returned. */
/* The filepointer returned will give a PNM-file generated */
/* by the PostScript-interpreter. */
static FILE *
ps_open (char *filename,
         const PSLoadVals *loadopt,
         int *llx,
         int *lly,
         int *urx,
         int *ury)

{char *cmd, *gs, *gs_opts, *driver, *pnmfile;
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
 fd_popen = fopen (filename, "r");
 if (fd_popen != NULL)
 {char hdr[4];

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
 pnmfile = "-";

 gs = getenv ("GS_PROG");
 if (gs == NULL) gs = "gs";

 gs_opts = getenv ("GS_OPTIONS");
 if (gs_opts == NULL)
   gs_opts = "-dSAFER";
 else
   gs_opts = "";  /* Ghostscript will add these options */

 cmd = g_malloc (strlen (filename) + strlen (gs) + strlen (gs_opts)
                 + strlen (pnmfile) + 256 );
 if (cmd == NULL) return (NULL);

 TextAlphaBits[0] = GraphicsAlphaBits[0] = geometry[0] = '\0';

 /* Antialiasing not available for PBM-device */
 if ((loadopt->pnm_type != 4) && (loadopt->textalpha != 1))
   sprintf (TextAlphaBits, "-dTextAlphaBits=%d ", (int)loadopt->textalpha);

 if ((loadopt->pnm_type != 4) && (loadopt->graphicsalpha != 1))
   sprintf (GraphicsAlphaBits, "-dGraphicsAlphaBits=%d ",
            (int)loadopt->graphicsalpha);

 if (!is_pdf)    /* For PDF, we cant set geometry */
   sprintf (geometry,"-g%dx%d ", width, height);

 sprintf (cmd, "%s -sDEVICE=%s -r%d %s%s%s-q -dNOPAUSE %s \
-sOutputFile=%s %s -c quit", gs, driver, resolution, geometry,
          TextAlphaBits, GraphicsAlphaBits, gs_opts, pnmfile, filename);
#ifdef PS_DEBUG
 printf ("Going to start ghostscript with:\n%s\n", cmd);
#endif
 /* Start the command and use a pipe for reading the PNM-file. */
 /* If someone does not like the pipe (or it does not work), just start */
 /* ghostscript with a real outputfile. When ghostscript has finished,  */
 /* open the outputfile and return its filepointer. But be sure         */
 /* to close and remove the file within ps_close().                     */
 fd_popen = popen (cmd, "r");
 g_free (cmd);

 return (fd_popen);
}


/* Close the PNM-File of the PostScript interpreter */
static void
ps_close (FILE *ifp)

{
 /* Finish reading from pipe. */
 /* If a real outputfile was used, close the file and remove it. */
 pclose (ifp);
}


/* Read the header of a raw PNM-file and return type (4-6) or -1 on failure */
static gint
read_pnmraw_type (FILE *ifp,
                  int *width,
                  int *height,
                  int *maxval)

{register int frst, scnd, thrd;
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
create_new_image (char *filename,
                  guint pagenum,
                  guint width,
                  guint height,
                  GImageType type,
                  gint32 *layer_ID,
                  GDrawable **drawable,
                  GPixelRgn *pixel_rgn)

{gint32 image_ID;
 GDrawableType gdtype;
 char *tmp;

 if (type == GRAY) gdtype = GRAY_IMAGE;
 else if (type == INDEXED) gdtype = INDEXED_IMAGE;
 else gdtype = RGB_IMAGE;

 image_ID = gimp_image_new (width, height, type);
 if ((tmp = g_malloc (strlen (filename) + 32)) != NULL)
 {
   sprintf (tmp, "%s-pg%ld", filename, (long)pagenum);
   gimp_image_set_filename (image_ID, tmp);
   g_free (tmp);
 }
 else
   gimp_image_set_filename (image_ID, filename);

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

{register int k, c;
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
load_ps (char *filename,
         guint pagenum,
         FILE *ifp,
         int llx,
         int lly,
         int urx,
         int ury)

{register unsigned char *dest;
 unsigned char *data, *bitline = NULL, *byteline = NULL, *byteptr, *temp;
 unsigned char bit2byte[256*8];
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
   bitline = (unsigned char *)g_malloc (nread);
   if (bitline == NULL) return (-1);
   byteline = (unsigned char *)g_malloc (nread*8);
   if (byteline == NULL) { g_free (bitline); return (-1); }

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
   if (byteline == NULL) return (-1);
 }
 else if (pnmtype == 6)  /* Portable Pixmap */
 {
   imagetype = RGB;
   nread = width * 3;
   bpp = 3;
   byteline = (unsigned char *)g_malloc (nread);
   if (byteline == NULL) return (-1);
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
static void save_ps_header (FILE *ofp,
                            char *filename)

{time_t cutime = time (NULL);

  fprintf (ofp, "%%!PS-Adobe-3.0%s\n", psvals.eps ? " EPSF-3.0" : "");
  fprintf (ofp, "%%%%Creator: GIMP PostScript file plugin V %4.2f \
by Peter Kirchgessner\n", VERSIO);
  fprintf (ofp, "%%%%Title: %s\n", filename);
  fprintf (ofp, "%%%%CreationDate: %s", ctime (&cutime));
  fprintf (ofp, "%%%%DocumentData: Clean7Bit\n");
  if (psvals.eps) fprintf (ofp, "%%%%LanguageLevel: 2\n");
  fprintf (ofp, "%%%%Pages: 1\n");
}


/* Write out transformation for image */
static void
save_ps_setup (FILE *ofp,
               gint32 drawable_ID,
               int width,
               int height,
               int bpp)

{double x_offset, y_offset, x_size, y_size;
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
  { x_size = width_inch; y_size = height_inch; }
  else
  { y_size = width_inch; x_size = height_inch; }

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
    case 180: dx = x_size*72.0; dy = 0.0; break;
    case 270: dx = x_size*72.0; dy = y_size*72.0; break;
  }
  if ((dx != 0.0) || (dy != 0.0))
    fprintf (ofp, "%% Translate to begin of first scanline\n%f %f translate\n",
             dx, dy);
  if (psvals.rotate)
    fprintf (ofp, "%d rotate\n", (int)psvals.rotate);
  fprintf (ofp, "%f %f scale\n", 72.0*width_inch, -72.0*height_inch);

  /* Write the PostScript procedures to read the image */
  fprintf (ofp, "%% Variable to keep one line of raster data\n");
  fprintf (ofp, "/scanline %d %d mul string def\n", width, bpp);
  fprintf (ofp, "%% Image geometry\n%d %d 8\n", width, height);
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
dither_grey (unsigned char *grey,
             unsigned char *bw,
             int npix,
             int linecount)

{register unsigned char *greyptr, *bwptr, mask;
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
   if (fs_error != NULL)
   {
     memset ((char *)fs_error, 0, (npix+2)*sizeof (int));
     fs_error++;
   }

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
save_ps_preview (FILE *ofp,
                 gint32 drawable_ID)

{register unsigned char *bwptr, *greyptr;
 GDrawableType drawable_type;
 GDrawable *drawable;
 GPixelRgn src_rgn;
 int width, height, x, y, nbsl, out_count;
 int nchar_pl = 72, src_y;
 double f1, f2;
 unsigned char *grey, *bw, *src_row, *src_ptr;
 unsigned char *cmap;
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

 grey = (unsigned char *)g_malloc (width);
 if (grey == NULL) return;
 bw = (unsigned char *)g_malloc (nbsl);
 if (bw == NULL) return;
 src_row = (unsigned char *)g_malloc (drawable->width * drawable->bpp);
 if (src_row == NULL) return;

 fprintf (ofp, "%%%%BeginPreview: %d %d 1 %d\n", width, height,
          ((nbsl*2+nchar_pl-1)/nchar_pl)*height);

 gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, drawable->width,
                      drawable->height, FALSE, FALSE);

 cmap = NULL;     /* Check if we need a colour table */
 if (gimp_drawable_type (drawable_ID) == INDEXED_IMAGE)
   cmap = (unsigned char *)
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
save_gray  (FILE *ofp,
            gint32 image_ID,
            gint32 drawable_ID)

{ int height, width, i, j;
  int tile_height;
  unsigned char *data, *src;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  static char *hex = "0123456789abcdef";

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  width = drawable->width;
  height = drawable->height;
  tile_height = gimp_tile_height ();
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (unsigned char *)g_malloc (tile_height * width * drawable->bpp);

  /* Set up transformation in PostScript */
  save_ps_setup (ofp, drawable_ID, width, height, 1);

  /* Write read image procedure */
  fprintf (ofp, "{ currentfile scanline readhexstring pop }\n");
  fprintf (ofp, "image\n");

#define GET_GRAY_TILE(begin) \
  {int scan_lines; \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gimp_pixel_rgn_get_rect (&pixel_rgn, begin, 0, i, width, scan_lines); \
    src = begin; }

  for (i = 0; i < height; i++)
  {
    if ((i % tile_height) == 0) GET_GRAY_TILE (data); /* Get more data */
    for (j = 0; j < width; j++)
    {
      putc (hex[(*src) >> 4], ofp);
      putc (hex[(*(src++)) & 0x0f], ofp);
      if (((j+1) % 39) == 0) putc ('\n', ofp);
    }
    putc ('\n', ofp);
    if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
      gimp_progress_update ((double) i / (double) height);
  }
  fprintf (ofp, "showpage\n");
  g_free (data);

  gimp_drawable_detach (drawable);

  if (ferror (ofp))
  {
    g_message ("write error occured");
    return (FALSE);
  }
  return (TRUE);
#undef GET_GRAY_TILE
}


static gint
save_index (FILE *ofp,
            gint32 image_ID,
            gint32 drawable_ID)

{ int height, width, i, j;
  int ncols;
  int tile_height;
  unsigned char *cmap;
  unsigned char *data, *src;
  char coltab[256*6], *ct;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  static char *hex = "0123456789abcdef";
  static char *background = "000000";

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  width = drawable->width;
  height = drawable->height;
  tile_height = gimp_tile_height ();
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (unsigned char *)g_malloc (tile_height * width * drawable->bpp);

  cmap = gimp_image_get_cmap (image_ID, &ncols);

  ct = coltab;
  for (j = 0; j < 256; j++)
  {
    if (j >= ncols)
    {
      memcpy (ct, background, 6);
      ct += 6;
    }
    else
    {
      *(ct++) = (unsigned char)hex[(*cmap) >> 4];
      *(ct++) = (unsigned char)hex[(*(cmap++)) & 0x0f];
      *(ct++) = (unsigned char)hex[(*cmap) >> 4];
      *(ct++) = (unsigned char)hex[(*(cmap++)) & 0x0f];
      *(ct++) = (unsigned char)hex[(*cmap) >> 4];
      *(ct++) = (unsigned char)hex[(*(cmap++)) & 0x0f];
    }
  }

  /* Set up transformation in PostScript */
  save_ps_setup (ofp, drawable_ID, width, height, 3);

  /* Write read image procedure */
  fprintf (ofp, "{ currentfile scanline readhexstring pop } false 3\n");
  fprintf (ofp, "colorimage\n");

#define GET_INDEX_TILE(begin) \
  {int scan_lines; \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gimp_pixel_rgn_get_rect (&pixel_rgn, begin, 0, i, width, scan_lines); \
    src = begin; }

  for (i = 0; i < height; i++)
  {
    if ((i % tile_height) == 0) GET_INDEX_TILE (data); /* Get more data */
    for (j = 0; j < width; j++)
    {
      fwrite (coltab+(*(src++))*6, 6, 1, ofp);
      if (((j+1) % 13) == 0) putc ('\n', ofp);
    }
    putc ('\n', ofp);
    if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
      gimp_progress_update ((double) i / (double) height);
  }
  fprintf (ofp, "showpage\n");

  g_free (data);

  gimp_drawable_detach (drawable);

  if (ferror (ofp))
  {
    g_message ("write error occured");
    return (FALSE);
  }
  return (TRUE);
#undef GET_INDEX_TILE
}


static gint
save_rgb (FILE *ofp,
          gint32 image_ID,
          gint32 drawable_ID)

{
  int height, width, tile_height;
  int i, j;
  unsigned char *data, *src;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  static char *hex = "0123456789abcdef";

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  width = drawable->width;
  height = drawable->height;
  tile_height = gimp_tile_height ();
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (unsigned char *)g_malloc (tile_height * width * drawable->bpp);

  /* Set up transformation in PostScript */
  save_ps_setup (ofp, drawable_ID, width, height, 3);

  /* Write read image procedure */
  fprintf (ofp, "{ currentfile scanline readhexstring pop } false 3\n");
  fprintf (ofp, "colorimage\n");

#define GET_RGB_TILE(begin) \
  {int scan_lines; \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gimp_pixel_rgn_get_rect (&pixel_rgn, begin, 0, i, width, scan_lines); \
    src = begin; }

  for (i = 0; i < height; i++)
  {
    if ((i % tile_height) == 0) GET_RGB_TILE (data); /* Get more data */
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
    if ((l_run_mode != RUN_NONINTERACTIVE) && ((i % 20) == 0))
      gimp_progress_update ((double) i / (double) height);
  }
  fprintf (ofp, "showpage\n");
  g_free (data);

  gimp_drawable_detach (drawable);

  if (ferror (ofp))
  {
    g_message ("write error occured");
    return (FALSE);
  }
  return (TRUE);
#undef GET_RGB_TILE
}


/*  Load interface functions  */

static gint
load_dialog (void)

{
  LoadDialogVals *vals;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *table;
  GSList *group;
  gchar **argv;
  gint argc;
  char buffer[STR_LENGTH];
  static char *label_text[] = { "Resolution:", "Width:", "Height:", "Pages:" };
  static char *radio_text[] = { "b/w", "gray", "colour", "automatic" };
  static char *alias_text[] = { "none", "weak", "strong" };
  int j, n_prop, alias, *alpha_bits;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("load");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  vals = g_malloc (sizeof (*vals));

  vals->dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (vals->dialog), "Load PostScript");
  gtk_window_position (GTK_WINDOW (vals->dialog), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (vals->dialog), "destroy",
                      (GtkSignalFunc) load_close_callback,
                      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) load_ok_callback,
                      vals);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dialog)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             (GtkSignalFunc) gtk_widget_destroy,
                             GTK_OBJECT (vals->dialog));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dialog)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dialog)->vbox), hbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /* Rendering */
  frame = gtk_frame_new ("Rendering");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Resolution/Width/Height/Pages labels */
  n_prop = sizeof (label_text)/sizeof (label_text[0]);
  table = gtk_table_new (n_prop, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  for (j = 0; j < n_prop; j++)
  {
    label = gtk_label_new (label_text[j]);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, j, j+1,
                      GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show (label);
  }

  /* Resolution/Width/Height/Pages Entries */
  for (j = 0; j < n_prop; j++)
  {
    vals->entry[j] = gtk_entry_new ();
    gtk_widget_set_usize (vals->entry[j], 80, 0);
    if      (j == 0) sprintf (buffer, "%d", (int)plvals.resolution);
    else if (j == 1) sprintf (buffer, "%d", (int)plvals.width);
    else if (j == 2) sprintf (buffer, "%d", (int)plvals.height);
    else if (j == 3) strcpy (buffer, plvals.pages);
    gtk_entry_set_text (GTK_ENTRY (vals->entry[j]), buffer);
    gtk_table_attach (GTK_TABLE (table), vals->entry[j], 1, 2, j, j+1,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_widget_show (vals->entry[j]);
  }

  toggle = gtk_check_button_new_with_label ("try BoundingBox");
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  vals->use_bbox = (plvals.use_bbox != 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) load_toggle_update,
                      &(vals->use_bbox));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), vals->use_bbox);
  gtk_widget_show (toggle);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Colouring */
  frame = gtk_frame_new ("Colouring");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  group = NULL;
  for (j = 0; j < 4; j++)
  {
    toggle = gtk_radio_button_new_with_label (group, radio_text[j]);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
    vals->dataformat[j] = (plvals.pnm_type == j+4);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (GtkSignalFunc) load_toggle_update,
                        &(vals->dataformat[j]));
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
                                 vals->dataformat[j]);
    gtk_widget_show (toggle);
  }

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dialog)->vbox), hbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  for (alias = 0; alias < 2; alias++)
  {
    alpha_bits = alias ? &(vals->graphicsalphabits[0])
                       : &(vals->textalphabits[0]);
    frame = gtk_frame_new (alias ? "Graphic antialiasing":"Text antialiasing");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_border_width (GTK_CONTAINER (frame), 10);
    gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (vbox), 5);
    gtk_container_add (GTK_CONTAINER (frame), vbox);

    group = NULL;
    for (j = 0; j < 3; j++)
    {
      toggle = gtk_radio_button_new_with_label (group, alias_text[j]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
      gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      alpha_bits[j] = alias ? (plvals.graphicsalpha == (1 << j))
                            : (plvals.textalpha == (1 << j));
      gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                          (GtkSignalFunc) load_toggle_update, alpha_bits+j);
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), alpha_bits[j]);
      gtk_widget_show (toggle);
    }

    gtk_widget_show (vbox);
    gtk_widget_show (frame);
  }

  gtk_widget_show (vals->dialog);

  gtk_main ();
  gdk_flush ();

  g_free (vals);

  return plint.run;
}


static void
load_close_callback (GtkWidget *widget,
                     gpointer   data)

{
  gtk_main_quit ();
}


static void
load_ok_callback (GtkWidget *widget,
                  gpointer   data)

{LoadDialogVals *vals = (LoadDialogVals *)data;
 int nelem;

  /* Read resolution */
  plvals.resolution = atoi (gtk_entry_get_text (GTK_ENTRY (vals->entry[0])));

  /* Read width */
  plvals.width = atoi (gtk_entry_get_text (GTK_ENTRY (vals->entry[1])));

  /* Read height */
  plvals.height = atoi (gtk_entry_get_text (GTK_ENTRY (vals->entry[2])));

  /* Read Pages */
  nelem = sizeof (plvals.pages);
  strncpy (plvals.pages, gtk_entry_get_text(GTK_ENTRY (vals->entry[3])),nelem);
  plvals.pages[nelem-1] = '\0';

  /* Read try BoundingBox */
  plvals.use_bbox = (vals->use_bbox != 0);

  /* Read colouring */
  if (vals->dataformat[0] == 1) plvals.pnm_type = 4;
  else if (vals->dataformat[1] == 1) plvals.pnm_type = 5;
  else if (vals->dataformat[3] == 1) plvals.pnm_type = 7;
  else plvals.pnm_type = 6;

  /* Read TextAlphaBits */
  if (vals->textalphabits[0] == 1) plvals.textalpha = 1;
  else if (vals->textalphabits[1] == 1) plvals.textalpha = 2;
  else if (vals->textalphabits[2] == 1) plvals.textalpha = 4;
  else plvals.textalpha = 1;

  /* Read GraphicsAlphaBits */
  if (vals->graphicsalphabits[0] == 1) plvals.graphicsalpha = 1;
  else if (vals->graphicsalphabits[1] == 1) plvals.graphicsalpha = 2;
  else if (vals->graphicsalphabits[2] == 1) plvals.graphicsalpha = 4;
  else  plvals.graphicsalpha = 1;

  plint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (vals->dialog));
}


static void
load_toggle_update (GtkWidget *widget,
                    gpointer   data)

{
  int *toggle_val;

  toggle_val = (int *) data;

  *toggle_val = ((GTK_TOGGLE_BUTTON (widget)->active) != 0);
}


/*  Save interface functions  */

static gint
save_dialog (void)

{
  SaveDialogVals *vals;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *frame, *uframe;
  GtkWidget *hbox, *vbox, *uvbox;
  GtkWidget *main_vbox[2];
  GtkWidget *label;
  GtkWidget *table;
  GSList *group;
  gchar **argv;
  gint argc;
  static char *label_text[] = { "Width:", "Height:", "X-offset:", "Y-offset:" };
  static char *radio_text[] = { "0", "90", "180", "270" };
  static char *unit_text[] = { "Inch", "Millimeter" };
  char tmp[80];
  int j, idata;
  double rdata;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("save");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  vals = g_malloc (sizeof (*vals));

  vals->dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (vals->dialog), "Save PostScript");
  gtk_window_position (GTK_WINDOW (vals->dialog), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (vals->dialog), "destroy",
                      (GtkSignalFunc) save_close_callback,
                      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) save_ok_callback,
                      vals);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dialog)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             (GtkSignalFunc) gtk_widget_destroy,
                             GTK_OBJECT (vals->dialog));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dialog)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_show (button);

  /* Main hbox */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dialog)->vbox), hbox,
                      FALSE, TRUE, 0);
  main_vbox[0] = main_vbox[1] = NULL;

  for (j = 0; j < sizeof (main_vbox) / sizeof (main_vbox[0]); j++)
  {
    main_vbox[j] = gtk_vbox_new (FALSE, 0);
    gtk_container_border_width (GTK_CONTAINER (main_vbox[j]), 0);
    gtk_box_pack_start (GTK_BOX (hbox), main_vbox[j], TRUE, TRUE, 0);
  }

  /* Image Size */
  frame = gtk_frame_new ("Image Size");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_box_pack_start (GTK_BOX (main_vbox[0]), frame, FALSE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Width/Height/X-/Y-offset labels */
  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  for (j = 0; j < 4; j++)
  {
    label = gtk_label_new (label_text[j]);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, j, j+1,
                      GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show (label);
  }

  /* Width/Height/X-off/Y-off Entries */
  for (j = 0; j < 4; j++)
  {
    vals->entry[j] = gtk_entry_new ();
    gtk_widget_set_usize (vals->entry[j], 50, 0);
    if      (j == 0) rdata = psvals.width;
    else if (j == 1) rdata = psvals.height;
    else if (j == 2) rdata = psvals.x_offset;
    else             rdata = psvals.y_offset;
    gtk_entry_set_text (GTK_ENTRY (vals->entry[j]), ftoa ("%-8.2f", rdata));
    gtk_table_attach (GTK_TABLE (table), vals->entry[j], 1, 2, j, j+1,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_widget_show (vals->entry[j]);
  }

  toggle = gtk_check_button_new_with_label ("keep aspect ratio");
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  vals->keep_ratio = (psvals.keep_ratio != 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) save_toggle_update,
                      &(vals->keep_ratio));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), vals->keep_ratio);
  gtk_widget_show (toggle);

  /* Unit */
  uframe = gtk_frame_new ("Unit");
  gtk_frame_set_shadow_type (GTK_FRAME (uframe), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (uframe), 5);
  gtk_box_pack_start (GTK_BOX (vbox), uframe, FALSE, FALSE, 0);
  uvbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (uvbox), 5);
  gtk_container_add (GTK_CONTAINER (uframe), uvbox);

  group = NULL;
  for (j = 0; j < 2; j++)
  {
    toggle = gtk_radio_button_new_with_label (group, unit_text[j]);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (uvbox), toggle, FALSE, FALSE, 0);
    vals->unit[j] = (psvals.unit_mm == j);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (j == 0) ? (GtkSignalFunc) save_toggle_update :
                        (GtkSignalFunc) save_mm_toggle_update,
                        (j == 0) ? (gpointer)(&(vals->unit[j])) :
                        (gpointer)vals);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
                                 vals->unit[j]);
    gtk_widget_show (toggle);
  }
  gtk_widget_show (uvbox);
  gtk_widget_show (uframe);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Rotation */
  frame = gtk_frame_new ("Rotation");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_box_pack_start (GTK_BOX (main_vbox[1]), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  group = NULL;
  for (j = 0; j < 4; j++)
  {
    toggle = gtk_radio_button_new_with_label (group, radio_text[j]);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
    gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
    vals->rot[j] = (psvals.rotate == j*90);
    gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                        (GtkSignalFunc) save_toggle_update,
                        &(vals->rot[j]));
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
                                 vals->rot[j]);
    gtk_widget_show (toggle);
  }

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Format */
  frame = gtk_frame_new ("Output");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_box_pack_start (GTK_BOX (main_vbox[1]), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label ("Encapsulated PostScript");
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  vals->eps = (psvals.eps != 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) save_toggle_update,
                      &(vals->eps));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), vals->eps);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label ("Preview");
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  vals->preview = psvals.preview;
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) save_toggle_update,
                      &(vals->preview));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), vals->preview);
  gtk_widget_show (toggle);

  /* Preview size label/entry */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  j = 0;
  label = gtk_label_new ("Preview size");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, j, j+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Entry */
  j= 0;
  vals->psize_entry = gtk_entry_new ();
  gtk_widget_set_usize (vals->psize_entry, 50, 0);
  idata = psvals.preview_size;
  if (idata < 0) idata = 0;
  sprintf (tmp, "%d", idata);
  gtk_entry_set_text (GTK_ENTRY (vals->psize_entry), tmp);
  gtk_table_attach (GTK_TABLE (table), vals->psize_entry, 1, 2, j, j+1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (vals->psize_entry);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  for (j = 0; j < sizeof (main_vbox) / sizeof (main_vbox[0]); j++)
    gtk_widget_show (main_vbox[j]);
  gtk_widget_show (hbox);
  gtk_widget_show (vals->dialog);

  gtk_main ();
  gdk_flush ();

  g_free (vals);

  return psint.run;
}


static void
save_close_callback (GtkWidget *widget,
                     gpointer   data)

{
  gtk_main_quit ();
}


static void
save_ok_callback (GtkWidget *widget,
                  gpointer   data)

{SaveDialogVals *vals = (SaveDialogVals *)data;
 double r;
 int k, ival;

  /* Read width */
  k = sscanf (gtk_entry_get_text (GTK_ENTRY (vals->entry[0])), "%lf", &r);
  if (k == 1) psvals.width = r;

  /* Read height */
  k = sscanf (gtk_entry_get_text (GTK_ENTRY (vals->entry[1])), "%lf", &r);
  if (k == 1) psvals.height = r;

  /* Read x-offset */
  k = sscanf (gtk_entry_get_text (GTK_ENTRY (vals->entry[2])), "%lf", &r);
  if (k == 1) psvals.x_offset = r;

  /* Read y-offset */
  k = sscanf (gtk_entry_get_text (GTK_ENTRY (vals->entry[3])), "%lf", &r);
  if (k == 1) psvals.y_offset = r;

  /* Read keep aspect ratio */
  psvals.keep_ratio = (vals->keep_ratio != 0);

  /* Read unit */
  if (vals->unit[0] == 1) psvals.unit_mm = 0;
  else psvals.unit_mm = 1;

  /* Read rotation */
  if (vals->rot[1] == 1) psvals.rotate = 90;
  else if (vals->rot[2] == 1) psvals.rotate = 180;
  else if (vals->rot[3] == 1) psvals.rotate = 270;
  else psvals.rotate = 0;

  /* Read EPS flag */
  psvals.eps = (vals->eps != 0);

  /* Read Preview flag */
  psvals.preview = (vals->preview != 0);

  /* Read preview size */
  k = sscanf (gtk_entry_get_text (GTK_ENTRY (vals->psize_entry)), "%d", &ival);
  if (k == 1) psvals.preview_size = ival;

  psint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (vals->dialog));
}


static void
save_toggle_update (GtkWidget *widget,
                    gpointer   data)

{
  int *toggle_val;

  toggle_val = (int *) data;

  *toggle_val = ((GTK_TOGGLE_BUTTON (widget)->active) != 0);
}


static void
save_mm_toggle_update (GtkWidget *widget,
                        gpointer   data)

{ double factor = 0.0, r;
  SaveDialogVals *vals = (SaveDialogVals *)data;
  int newval, oldval = vals->unit[1];
  int mm_to_inch, inch_to_mm, j, k;

  newval = vals->unit[1] = ((GTK_TOGGLE_BUTTON (widget)->active) != 0);
  mm_to_inch = (oldval == 1) && (newval == 0);
  inch_to_mm = (oldval == 0) && (newval == 1);
  if (mm_to_inch) factor = 1.0 / 25.4;
  else if (inch_to_mm) factor = 25.4;
  if (factor != 0.0)
  {
    for (j = 0; j < 4; j++)
    {
      k = sscanf (gtk_entry_get_text (GTK_ENTRY (vals->entry[j])), "%lf", &r);
      if (k == 1)
      {
        gtk_entry_set_text (GTK_ENTRY(vals->entry[j]),ftoa("%-8.2f",r*factor));
        gtk_widget_show (vals->entry[j]);
      }
    }
  }
}
