/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * X10 and X11 bitmap (XBM) loading and saving file filter for the GIMP.
 * XBM code Copyright (C) 1998 Gordon Matzigkeit
 *
 * The XBM reading and writing code was written from scratch by Gordon
 * Matzigkeit <gord@gnu.org> based on the XReadBitmapFile(3X11) manual
 * page distributed with X11R6 and by staring at valid XBM files.  It
 * does not contain any code written for other XBM file loaders.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Release 1.0, 1998-02-04, Gordon Matzigkeit <gord@gnu.org>:
 *   - Load and save X10 and X11 bitmaps.
 *   - Allow the user to specify the C identifier prefix.
 *
 * TODO:
 *   - Parsing is very tolerant, and the algorithms are quite hairy, so
 *     load_image should be carefully tested to make sure there are no XBM's
 *     that fail.
 *   - Allow the user to specify a hotspot, and preserve it when loading.
 */

/* Set this for debugging. */
/* #define VERBOSE 2 */

#include "config.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "libgimp/stdplugins-intl.h"

/* Wear your GIMP with pride! */
#define DEFAULT_USE_COMMENT TRUE
#define MAX_COMMENT 72

/* C identifier prefix. */
#define DEFAULT_PREFIX "bitmap"
#define MAX_PREFIX 24

/* Whether or not to save as X10 bitmap. */
#define DEFAULT_X10_FORMAT FALSE

typedef struct _XBMSaveVals
{
  gchar comment[MAX_COMMENT + 1];
  gint x10_format;
  gint x_hot;
  gint y_hot;
  gchar prefix[MAX_PREFIX + 1];
} XBMSaveVals;

static XBMSaveVals xsvals =
{
  "###",		/* comment */
  DEFAULT_X10_FORMAT,		/* x10_format */
  -1,				/* x_hot */
  -1,				/* y_hot */
  DEFAULT_PREFIX,		/* prefix */
};


typedef struct _XBMSaveInterface
{
  gint run;
} XBMSaveInterface;

static XBMSaveInterface xsint =
{
  FALSE				/* run */
};


/* Declare some local functions.
 */
static void   query        (void);
static void   run          (char    *name,
			    int      nparams,
			    GParam  *param,
			    int     *nreturn_vals,
			    GParam **return_vals);
static void   init_gtk     (void);
static gint32 load_image   (char   *filename);
static gint   save_image   (char   *filename,
			    gint32  image_ID, 
			    gint32  drawable_ID);
static gint   save_dialog  (gint32  drawable_ID);
static void   close_callback         (GtkWidget *widget,
				      gpointer   data);
static void   save_ok_callback       (GtkWidget *widget,
				      gpointer   data);
static void   save_toggle_update     (GtkWidget *widget,
				      gpointer   data);
static void   comment_entry_callback (GtkWidget *widget,
				      gpointer   data);
static void   prefix_entry_callback  (GtkWidget *widget,
				      gpointer   data);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MAIN ()

#ifdef VERBOSE
static int verbose = VERBOSE;
#endif


static void
query ()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32,  "run_mode",     "Interactive, non-interactive" },
    { PARAM_STRING, "filename",     "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name entered" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE,  "image",        "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);


  static GParamDef save_args[] =
  {
    { PARAM_INT32,    "run_mode",     "Interactive, non-interactive" },
    { PARAM_IMAGE,    "image",        "Input image" },
    { PARAM_DRAWABLE, "drawable",     "Drawable to save" },
    { PARAM_STRING,   "filename",     "The name of the file to save" },
    { PARAM_STRING,   "raw_filename", "The name entered" },
    { PARAM_STRING,   "comment",      "Image description (maximum 72 bytes)" },
    { PARAM_INT32,    "x10",          "Save in X10 format" },
    { PARAM_INT32,    "x_hot",        "X coordinate of hotspot" },
    { PARAM_INT32,    "y_hot",        "Y coordinate of hotspot" },
    { PARAM_STRING,   "prefix",       "Identifier prefix [determined from filename]"},
  } ;
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_xbm_load",
                          _("Load a file in X10 or X11 bitmap (XBM) file format"),
                          _("Load a file in X10 or X11 bitmap (XBM) file format.  XBM is a lossless format for flat black-and-white (two color indexed) images."),
                          "Gordon Matzigkeit",
                          "Gordon Matzigkeit",
                          "1998",
                          "<Load>/XBM",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);


  gimp_install_procedure ("file_xbm_save",
                          "Save a file in X10 or X11 bitmap (XBM) file format",
                          "Save a file in X10 or X11 bitmap (XBM) file format.  XBM is a lossless format for flat black-and-white (two color indexed) images.",
			  "Gordon Matzigkeit",
                          "Gordon Matzigkeit",
                          "1998",
                          "<Save>/XBM",
			  "INDEXED",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_load_handler ("file_xbm_load", "xbm,icon,bitmap", "");
  gimp_register_save_handler ("file_xbm_save", "xbm,icon,bitmap", "");
}


static void
init_prefix (char *filename)
{
  char *p, *prefix;
  int len;

  /* Mangle the filename to get the prefix. */
  prefix = strrchr (filename, '/');
  if (prefix)
    prefix ++;
  else
    prefix = filename;

  /* Strip any extension. */
  p = strrchr (prefix, '.');
  if (p && p != prefix)
    len = MIN (MAX_PREFIX, p - prefix);
  else
    len = MAX_PREFIX;

  memset (xsvals.prefix, 0, sizeof (xsvals.prefix));
  strncpy (xsvals.prefix, prefix, len);
}


static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GStatusType status = STATUS_SUCCESS;
  GRunModeType run_mode;
  gint32 image_ID;
  gint32 drawable_ID;
  GimpExportReturnType export = EXPORT_CANCEL;

  INIT_I18N_UI();
  strncpy(xsvals.comment, _("Made with Gimp"), MAX_COMMENT);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

#ifdef VERBOSE
  if (verbose)
    printf ("XBM: RUN %s\n", name);
#endif

  if (strcmp (name, "file_xbm_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
          values[0].data.d_status = STATUS_SUCCESS;
          values[1].type = PARAM_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          values[0].data.d_status = STATUS_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, "file_xbm_save") == 0)
    {
      image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;
    
      /*  eventually export the image */ 
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	case RUN_WITH_LAST_VALS:
	  init_gtk ();
	  export = gimp_export_image (&image_ID, &drawable_ID, "XBM", CAN_HANDLE_INDEXED);
	  if (export == EXPORT_CANCEL)
	    {
	      *nreturn_vals = 1;
	      values[0].data.d_status = STATUS_EXECUTION_ERROR;
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
	  gimp_get_data ("file_xbm_save", &xsvals);

	  /* Always override the prefix with the filename. */
	  init_prefix (param[3].data.d_string);

	  /*  First acquire information with a dialog  */
	  if (! save_dialog (drawable_ID))
	    return;

	  break;

	case RUN_NONINTERACTIVE:
	  /*  Make sure all the required arguments are there!  */
	  if (nparams < 5)
	    status = STATUS_CALLING_ERROR;
	  if (status == STATUS_SUCCESS)
	    {
	      int i = 5;

	      if (nparams > i)
		{
		  memset (xsvals.comment, 0, sizeof (xsvals.comment));
		  strncpy (xsvals.comment, param[i].data.d_string,
			   MAX_COMMENT);
		}

	      i ++;
	      if (nparams > i)
		xsvals.x10_format = (param[i].data.d_int32) ? TRUE : FALSE;

	      i += 2;
	      if (nparams > i)
		{
		  /* They've asked for a hotspot. */
		  xsvals.x_hot = param[i - 1].data.d_int32;
		  xsvals.y_hot = param[i].data.d_int32;
		}

	      i ++;
	      if (nparams > i)
		{
		  memset (xsvals.prefix, 0, sizeof (xsvals.prefix));
		  strncpy (xsvals.prefix, param[i].data.d_string,
			   MAX_PREFIX);
		}
	      else
		init_prefix (param[3].data.d_string);

	      i ++;
	      /* Too many arguments. */
	      if (nparams > i)
		status = STATUS_CALLING_ERROR;
	    }
	  break;

	case RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_xbm_save", &xsvals);
	  break;

	default:
	  break;
	}

      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string, image_ID, drawable_ID))
	{
	  /*  Store xsvals data  */
	  gimp_set_data ("file_xbm_save", &xsvals, sizeof (xsvals));
	  values[0].data.d_status = STATUS_SUCCESS;
	}
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;

      if (export == EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
}



/* Return the value of a digit. */
static gint
getval (int c, 
	int base)
{
  static guchar *digits = "0123456789abcdefABCDEF";
  int val;

  /* Include uppercase hex digits. */
  if (base == 16)
    base = 22;

  /* Find a match. */
  for (val = 0; val < base; val ++)
    if (c == digits[val])
      return (val < 16) ? val : (val - 6);
  return -1;
}


/* Same as fgetc, but skip C-style comments and insert whitespace. */
static gint
cpp_fgetc (FILE *fp)
{
  int comment, c;

  /* FIXME: insert whitespace as advertised. */
  comment = 0;
  do
    {
      c = fgetc (fp);
      if (comment)
	{
	  if (c == '*')
	    /* In a comment, with potential to leave. */
	    comment = 1;
	  else if (comment == 1 && c == '/')
	    /* Leaving a comment. */
	    comment = 0;
	  else
	    /* In a comment, with no potential to leave. */
	    comment = 2;
	}
      else
	{
	  /* Not in a comment. */
	  if (c == '/')
	    {
	      /* Potential to enter a comment. */
	      c = fgetc (fp);
	      if (c == '*')
		/* Entered a comment, with no potential to leave. */
		comment = 2;
	      else
		{
		  /* Just a slash in the open. */
		  ungetc (c, fp);
		  c = '/';
		}
	    }
	}
    }
  while (comment && c != EOF);
  return c;
}


/* Match a string with a file. */
static gint
match (FILE *fp, 
       char *s)
{
  int c;

  do
    {
      c = fgetc (fp);
      if (c == *s)
	s ++;
      else
	break;
    }
  while (c != EOF && *s);

  if (!*s)
    return TRUE;

  if (c != EOF)
    ungetc (c, fp);
  return FALSE;
}


/* Read the next integer from the file, skipping all non-integers. */
static gint
get_int (FILE *fp)
{
  int digval, base, val, c;

  do
    c = cpp_fgetc (fp);
  while (c != EOF && !isdigit (c));

  if (c == EOF)
    return 0;

  /* Check for the base. */
  if (c == '0')
    {
      c = fgetc (fp);
      if (c == 'x' || c == 'X')
	{
	  c = fgetc (fp);
	  base = 16;
	}
      else if (isdigit (c))
	base = 8;
      else
	{
	  ungetc (c, fp);
	  return 0;
	}
    }
  else
    base = 10;

  val = 0;
  for (;;)
    {
      digval = getval (c, base);
      if (digval == -1)
	{
	  ungetc (c, fp);
	  break;
	}
      val *= base;
      val += digval;
      c = fgetc (fp);
    }

  return val;
}


static gint
load_image (char *filename)
{
  FILE *fp;
  gint32 image_ID, layer_ID;

  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  guchar *data;
  int width, height, intbits;
  int c, i, j, k;
  int tileheight, rowoffset;

  char *name_buf;

  guchar cmap[] =
  {
    0x00, 0x00, 0x00,		/* black */
    0xff, 0xff, 0xff		/* white */
  };

  fp = fopen (filename, "rb");
  if (!fp)
    {
      g_message (_("XBM: cannot open \"%s\"\n"), filename);
      return -1;
    }

  name_buf = g_malloc (strlen (filename) + 11);
  sprintf (name_buf, _("Loading %s:"), filename);
  gimp_progress_init (name_buf);
  g_free (name_buf);

  /* Loosely parse the header */
  intbits = height = width = 0;
  c = ' ';
  do
    {

      if (isspace (c))
	{
	  if (match (fp, "char"))
	    {
	      c = fgetc (fp);
	      if (isspace (c))
		{
		  intbits = 8;
		  continue;
		}
	    }
	  else if (match (fp, "short"))
	    {
	      c = fgetc (fp);
	      if (isspace (c))
		{
		  intbits = 16;
		  continue;
		}
	    }
	}

      if (c == '_')
	{
	  if (match (fp, "width"))
	    {
	      c = fgetc (fp);
	      if (isspace (c))
		{
		  width = get_int (fp);
		  continue;
		}
	    }
	  else if (match (fp, "height"))
	    {
	      c = fgetc (fp);
	      if (isspace (c))
		{
		  height = get_int (fp);
		  continue;
		}
	    }
	}

      c = cpp_fgetc (fp);
    }
  while (c != '{' && c != EOF);

  if (c == EOF)
    {
      g_message (_("XBM: cannot read header (ftell == %ld)\n"), ftell (fp));
      return -1;
    }

  if (width == 0)
    {
      g_message (_("XBM: no image width specified\n"));
      return -1;
    }

  if (height == 0)
    {
      g_message (_("XBM: no image height specified\n"));
      return -1;
    }

  if (intbits == 0)
    {
      g_message (_("XBM: no image data type specified\n"));
      return -1;
    }

  image_ID = gimp_image_new (width, height, INDEXED);
  gimp_image_set_filename (image_ID, filename);

  /* Set a black-and-white colormap. */
  gimp_image_set_cmap (image_ID, cmap, 2);

  layer_ID = gimp_layer_new (image_ID,
			     _("Background"),
			     width, height,
			     INDEXED_IMAGE,
			     100,
			     NORMAL_MODE);
  gimp_image_add_layer (image_ID, layer_ID, 0);

  drawable = gimp_drawable_get (layer_ID);

  /* Prepare the pixel region. */
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, TRUE, FALSE);

  /* Allocate the data. */
  tileheight = gimp_tile_height ();
  data = (guchar *) g_malloc (width * tileheight);

  for (i = 0; i < height; i += tileheight)
    {
      tileheight = MIN (tileheight, height - i);

#ifdef VERBOSE
      if (verbose > 1)
	printf ("XBM: reading %dx(%d+%d) pixel region\n", width, i,
		tileheight);
#endif

      /* Parse the data from the file */
      for (j = 0; j < tileheight; j ++)
	{
	  /* Read each row. */
	  rowoffset = j * width;
	  for (k = 0; k < width; k ++)
	    {
	      /* Expand each integer into INTBITS pixels. */
	      if (k % intbits == 0)
		{
		  c = get_int (fp);

		  /* Flip all the bits so that 1's become black and
                     0's become white. */
		  c ^= 0xffff;
		}

	      data[rowoffset + k] = c & 1;
	      c >>= 1;
	    }
	}

      /* Put the data into the image. */
      gimp_progress_update ((double) (i + tileheight) / (double) height);
      gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0, i, width, tileheight);
    }

  g_free (data);

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  fclose (fp);
  return image_ID;
}


static void
close_callback (GtkWidget *widget,
		gpointer   data)
{
  gtk_main_quit ();
}

static int gtk_initialized = FALSE;


static void
not_bw_dialog (void)
{
  GtkWidget *dlg, *button, *hbbox, *label, *frame, *vbox;

  if (!gtk_initialized)
    {
      fprintf (stderr, _("XBM: can only save two color indexed images\n"));
      return;
    }

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("XBM Warning"));
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) close_callback,
		      dlg);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      (GtkSignalFunc) close_callback,
		      dlg);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  the warning message  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE,
		      TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  label = gtk_label_new (_("The image which you are trying to save as\n"
			   "an XBM contains more than two colors.\n\n"
			   "Please convert it to a black and white\n"
			   "(1-bit) indexed image and try again."));
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);
  gtk_widget_show (vbox);
  gtk_widget_show (frame);
  gtk_widget_show (dlg);
  gtk_main ();
  gtk_widget_destroy (GTK_WIDGET (dlg));
  gdk_flush ();
}


static gint
save_image (char   *filename,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  GDrawable *drawable;
  GPixelRgn pixel_rgn;
  FILE *fp;

  int width, height, colors, dark;
  int intbits, lineints, need_comma, nints, rowoffset, tileheight;
  int c, i, j, k, thisbit;

  guchar *data, *cmap, *name_buf;
  char *prefix, *p, *intfmt;

  drawable = gimp_drawable_get (drawable_ID);
  width = drawable->width;
  height = drawable->height;
  cmap = gimp_image_get_cmap (image_ID, &colors);

#if 0
  printf ("XBM: run `gdb xbm' and `attach %d'\n", getpid ());
  kill (getpid (), 19);
#endif

  if (gimp_drawable_type (drawable_ID) != INDEXED_IMAGE || colors > 2)
    {
      /* The image is not black-and-white. */
      not_bw_dialog ();
      return FALSE;
    }

  name_buf = (guchar *) g_malloc (strlen (filename) + 11);
  sprintf (name_buf, _("Saving %s:"), filename);
  gimp_progress_init (name_buf);
  g_free (name_buf);

  /* Figure out which color is black, and which is white. */
  dark = 0;
  if (colors > 1)
    {
      int first, second;

      /* Maybe the second color is darker than the first. */
      first = (cmap[0] * cmap[0]) + (cmap[1] * cmap[1]) + (cmap[2] * cmap[2]);
      second = (cmap[3] * cmap[3]) + (cmap[4] * cmap[4]) + (cmap[5] * cmap[5]);

      if (second < first)
	dark = 1;
    }

  /* Now actually save the data. */
  fp = fopen (filename, "w");
  if (!fp)
    {
      g_message (_("XBM: cannot create \"%s\"\n"), filename);
      return FALSE;
    }

  /* Maybe write the image comment. */
  if (*xsvals.comment)
    fprintf (fp, "/* %s */\n", xsvals.comment);

  /* Change any non-alphanumeric prefix characters to underscores. */
  prefix = xsvals.prefix;
  p = prefix;
  while (*p)
    {
      if (!isalnum (*p))
	*p = '_';
      p ++;
    }

  /* Write out the image height and width. */
  fprintf (fp, "#define %s_width %d\n", prefix, width);
  fprintf (fp, "#define %s_height %d\n", prefix, height);

  /* Write out the hotspot, if any. */
  if (xsvals.x_hot >= 0 && xsvals.y_hot >= 0)
    {
      fprintf (fp, "#define %s_x_hot %d\n", prefix, xsvals.x_hot);
      fprintf (fp, "#define %s_y_hot %d\n", prefix, xsvals.y_hot);
    }

  /* Now write the actual data. */
  if (xsvals.x10_format)
    {
      /* We can fit 9 hex shorts on a single line. */
      lineints = 9;
      intbits = 16;
      intfmt = " 0x%04x";
    }
  else
    {
      /* We can fit 12 hex chars on a single line. */
      lineints = 12;
      intbits = 8;
      intfmt = " 0x%02x";
    }

  fprintf (fp, "static %s %s_bits[] = {\n  ",
	   xsvals.x10_format ? "short" : "char", prefix);

  /* Allocate a new set of pixels. */
  tileheight = gimp_tile_height ();
  data = (guchar *) g_malloc(width * tileheight);

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height,
		       FALSE, FALSE);

  /* Write out the integers. */
  need_comma = 0;
  nints = 0;
  for (i = 0; i < height; i += tileheight)
    {
      /* Get a horizontal slice of the image. */
      tileheight = MIN (tileheight, height - i);
      gimp_pixel_rgn_get_rect (&pixel_rgn, data, 0, i, width, tileheight);

#ifdef VERBOSE
      if (verbose > 1)
	printf ("TGA: writing %dx(%d+%d) pixel region\n",
		width, i, tileheight);
#endif

      for (j = 0; j < tileheight; j ++)
	{
	  /* Write out a row at a time. */
	  rowoffset = j * width;
	  c = 0;
	  thisbit = 0;

	  for (k = 0; k < width; k ++)
	    {
	      if (k != 0 && thisbit == intbits)
		{
		  /* Output a completed integer. */
		  if (need_comma)
		    fputc (',', fp);
		  need_comma = 1;

		  /* Maybe start a new line. */
		  if (nints ++ >= lineints)
		    {
		      nints = 1;
		      fputs ("\n  ", fp);
		    }
		  fprintf (fp, intfmt, c);

		  /* Start a new integer. */
		  c = 0;
		  thisbit = 0;
		}

	      /* Pack INTBITS pixels into an integer. */
	      c |= ((data[rowoffset + k] == dark) ? 1 : 0) << (thisbit ++);
	    }

	  if (thisbit != 0)
	    {
	      /* Write out the last oddball int. */
	      if (need_comma)
		fputc (',', fp);
	      need_comma = 1;

	      /* Maybe start a new line. */
	      if (nints ++ == lineints)
		{
		  nints = 1;
		  fputs ("\n  ", fp);
		}
	      fprintf (fp, intfmt, c);
	    }
	}

      gimp_progress_update ((double) (i + tileheight) / (double) height);
    }

  /* Write the trailer. */
  fprintf (fp, " };\n");
  fclose (fp);
  return TRUE;
}


static void 
init_gtk ()
{
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("xbm");
  
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  gtk_initialized = TRUE;
}

static gint
save_dialog (gint32  drawable_ID)
{
  GtkWidget *dlg, *hbbox, *button, *toggle, *label, *entry, *frame, *hbox, *vbox;

  xsint.run = FALSE;

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("Save as XBM"));
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) close_callback,
		      NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      (GtkSignalFunc) close_callback,
		      dlg);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) save_ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);


  /* parameter settings */
  frame = gtk_frame_new (_("XBM Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_widget_show (frame);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* comment string. */
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new (_("Description: "));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new_with_max_length (MAX_COMMENT);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_set_usize (entry, 240, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), xsvals.comment);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      (GtkSignalFunc) comment_entry_callback,
                      NULL);
  gtk_widget_show (entry);

  gtk_widget_show (hbox);

  /*  X10 format  */
  toggle = gtk_check_button_new_with_label (_("X10 format bitmap"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &xsvals.x10_format);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), xsvals.x10_format);
  gtk_widget_show (toggle);

  /* prefix */
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new (_("Identifier prefix: "));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new_with_max_length (MAX_PREFIX);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), xsvals.prefix);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      (GtkSignalFunc) prefix_entry_callback,
                      NULL);
  gtk_widget_show (entry);

  gtk_widget_show (hbox);

  /* Done. */
  gtk_widget_show (vbox);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return xsint.run;
}


/* Update the comment string. */
static void
comment_entry_callback (GtkWidget *widget,
			gpointer   data)
{
  memset (xsvals.comment, 0, sizeof (xsvals.comment));
  strncpy (xsvals.comment,
	   gtk_entry_get_text (GTK_ENTRY (widget)), MAX_COMMENT);
}


static void
prefix_entry_callback (GtkWidget *widget,
		       gpointer   data)
{
  memset (xsvals.prefix, 0, sizeof (xsvals.prefix));
  strncpy (xsvals.prefix, gtk_entry_get_text (GTK_ENTRY (widget)), MAX_PREFIX);
}


static void
save_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  xsint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}


static void
save_toggle_update (GtkWidget *widget,
		    gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

/*
Local Variables:
compile-command:"gcc -Wall -Wmissing-prototypes -g -O -o xbm xbm.c -lgimp -lgtk -lgdk -lglib -lm"
End:
*/












