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
 */

/* Set this for debugging. */
/* #define VERBOSE 2 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Wear your GIMP with pride! */
#define DEFAULT_USE_COMMENT TRUE
#define MAX_COMMENT         72
#define MAX_MASK_EXT        32

/* C identifier prefix. */
#define DEFAULT_PREFIX "bitmap"
#define MAX_PREFIX     24

/* Whether or not to save as X10 bitmap. */
#define DEFAULT_X10_FORMAT FALSE

typedef struct _XBMSaveVals
{
  gchar    comment[MAX_COMMENT + 1];
  gint     x10_format;
  gint     use_hot;
  gint     x_hot;
  gint     y_hot;
  gchar    prefix[MAX_PREFIX + 1];
  gboolean write_mask;
  gchar    mask_ext[MAX_MASK_EXT + 1];
} XBMSaveVals;

static XBMSaveVals xsvals =
{
  "###",		/* comment */
  DEFAULT_X10_FORMAT,	/* x10_format */
  FALSE,
  0,			/* x_hot */
  0,			/* y_hot */
  DEFAULT_PREFIX,	/* prefix */
  FALSE,                /* write_mask */
  "_mask"
};

typedef struct _XBMSaveInterface
{
  gint run;
} XBMSaveInterface;

static XBMSaveInterface xsint =
{
  FALSE			/* run */
};


/* Declare some local functions.
 */
static void   query   (void);
static void   run     (gchar      *name,
		       gint        nparams,
		       GimpParam  *param,
		       gint       *nreturn_vals,
		       GimpParam **return_vals);

static gint32 load_image              (gchar     *filename);
static gint   save_image              (gchar     *filename,
				       gchar     *prefix,
				       gchar     *comment,
				       gboolean   save_mask,
				       gint32     image_ID, 
				       gint32     drawable_ID);
static gint   save_dialog             (gint32     drawable_ID);
static void   save_ok_callback        (GtkWidget *widget,
				       gpointer   data);
static void   comment_entry_callback  (GtkWidget *widget,
				       gpointer   data);
static void   prefix_entry_callback   (GtkWidget *widget,
				       gpointer   data);
static void   mask_ext_entry_callback (GtkWidget *widget,
				       gpointer   data);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

#ifdef VERBOSE
static int verbose = VERBOSE;
#endif

static void
query (void)
{
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw_filename", "The name entered" }
  };
  static gint nload_args = sizeof (load_args) / sizeof (load_args[0]);

  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",        "Output image" }
  };
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  static GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",       "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",          "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",       "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",       "The name of the file to save" },
    { GIMP_PDB_STRING,   "raw_filename",   "The name entered" },
    { GIMP_PDB_STRING,   "comment",        "Image description (maximum 72 bytes)" },
    { GIMP_PDB_INT32,    "x10",            "Save in X10 format" },
    { GIMP_PDB_INT32,    "x_hot",          "X coordinate of hotspot" },
    { GIMP_PDB_INT32,    "y_hot",          "Y coordinate of hotspot" },
    { GIMP_PDB_STRING,   "prefix",         "Identifier prefix [determined from filename]"},
    { GIMP_PDB_INT32,    "write_mask",     "(0 = ignore, 1 = save as extra file)" },
    { GIMP_PDB_STRING,   "mask_extension", "Extension of the mask file" }
  } ;
  static gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_xbm_load",
                          "Load a file in X10 or X11 bitmap (XBM) file format",
                          "Load a file in X10 or X11 bitmap (XBM) file format.  XBM is a lossless format for flat black-and-white (two color indexed) images.",
                          "Gordon Matzigkeit",
                          "Gordon Matzigkeit",
                          "1998",
                          "<Load>/XBM",
			  NULL,
                          GIMP_PLUGIN,
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
                          GIMP_PLUGIN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_load_handler ("file_xbm_load",
			      "xbm,icon,bitmap",
			      "");
  gimp_register_save_handler ("file_xbm_save",
			      "xbm,icon,bitmap",
			      "");
}

static gchar *
init_prefix (gchar *filename)
{
  gchar *p, *prefix;
  gint len;

  prefix = g_basename (filename);

  /* Strip any extension. */
  p = strrchr (prefix, '.');
  if (p && p != prefix)
    len = MIN (MAX_PREFIX, p - prefix);
  else
    len = MAX_PREFIX;

  memset (xsvals.prefix, 0, sizeof (xsvals.prefix));
  strncpy (xsvals.prefix, prefix, len);

  return xsvals.prefix;
}

static void
run (gchar      *name,
     gint        nparams,
     GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam   values[2];
  GimpRunModeType    run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image_ID;
  gint32             drawable_ID;
  GimpParasite      *parasite;
  gchar             *mask_filename = NULL;
  GimpExportReturnType export = EXPORT_CANCEL;

  INIT_I18N_UI();
  strncpy (xsvals.comment, _("Created with The GIMP"), MAX_COMMENT);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

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
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, "file_xbm_save") == 0)
    {
      image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */ 
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_ui_init ("xbm", FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "XBM",
				      CAN_HANDLE_INDEXED |
				      CAN_HANDLE_ALPHA);
	  if (export == EXPORT_CANCEL)
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
	case GIMP_RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_xbm_save", &xsvals);

	  /* Always override the prefix with the filename. */
	  mask_filename = g_strdup (init_prefix (param[3].data.d_string));
	  break;

	case GIMP_RUN_NONINTERACTIVE:
	  /*  Make sure all the required arguments are there!  */
	  if (nparams < 5)
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	    }
	  else
	    {
	      gint i = 5;

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
		  xsvals.use_hot = TRUE;
		  xsvals.x_hot = param[i - 1].data.d_int32;
		  xsvals.y_hot = param[i].data.d_int32;
		}

	      mask_filename = g_strdup (init_prefix (param[3].data.d_string));

	      i ++;
	      if (nparams > i)
		{
		  memset (xsvals.prefix, 0, sizeof (xsvals.prefix));
		  strncpy (xsvals.prefix, param[i].data.d_string,
			   MAX_PREFIX);
		}

	      i += 2;
	      if (nparams > i)
		{
		  xsvals.write_mask = param[i - 1].data.d_int32;
		  memset (xsvals.mask_ext, 0, sizeof (xsvals.mask_ext));
		  strncpy (xsvals.mask_ext, param[i].data.d_string,
			   MAX_MASK_EXT);
		}

	      i ++;
	      /* Too many arguments. */
	      if (nparams > i)
		status = GIMP_PDB_CALLING_ERROR;
	    }
	  break;

	default:
	  break;
	}

      /* Get the parasites */
      parasite = gimp_image_parasite_find (image_ID, "gimp-comment");

      if (parasite)
	{
	  gpointer data;
	  gint     size;

	  data = gimp_parasite_data (parasite);
	  size = gimp_parasite_data_size (parasite);

	  strncpy (xsvals.comment, data, MIN (size, MAX_COMMENT));
	  xsvals.comment[MIN (size, MAX_COMMENT) + 1] = 0;

	  gimp_parasite_free (parasite);
	}

      parasite = gimp_image_parasite_find (image_ID, "hot-spot");

      if (parasite)
	{
	  gpointer data;
	  gint     x, y;

	  data = gimp_parasite_data (parasite);

	  if (sscanf (data, "%i %i", &x, &y) == 2)
	    {
	      xsvals.use_hot = TRUE;
	      xsvals.x_hot = x;
	      xsvals.y_hot = y;
	    }

	  gimp_parasite_free (parasite);
	}

      if (run_mode == GIMP_RUN_INTERACTIVE)
	{
	  /*  Acquire information with a dialog  */
	  if (! save_dialog (drawable_ID))
	    status = GIMP_PDB_CANCEL;
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  gchar *temp;
	  gchar *mask_prefix;
	  gchar *dirname;

	  temp = mask_filename;

	  if ((dirname = g_dirname (param[3].data.d_string)) != NULL)
	    {
	      mask_filename = g_strdup_printf ("%s/%s%s.xbm",
					       dirname, temp, xsvals.mask_ext);
	      g_free (dirname);
	    }
	  else
	    {
	      mask_filename = g_strdup_printf ("%s%s.xbm",
					       temp, xsvals.mask_ext);
	    }

	  g_free (temp);

	  /* Change any non-alphanumeric prefix characters to underscores. */
	  temp = xsvals.prefix;
	  while (*temp)
	    {
	      if (!isalnum (*temp))
		*temp = '_';
	      temp ++;
	    }

	  mask_prefix = g_strdup_printf ("%s%s", xsvals.prefix, xsvals.mask_ext);

	  if (save_image (param[3].data.d_string,
			  xsvals.prefix,
			  xsvals.comment,
			  FALSE,
			  image_ID, drawable_ID) &&

	      xsvals.write_mask &&

	      save_image (mask_filename,
			  mask_prefix,
			  xsvals.comment,
			  TRUE,
			  image_ID, drawable_ID))
	    {
	      /*  Store xsvals data  */
	      gimp_set_data ("file_xbm_save", &xsvals, sizeof (xsvals));
	    }
	  else
	    {
	      status = GIMP_PDB_EXECUTION_ERROR;
	    }

	  g_free (mask_prefix);
	  g_free (mask_filename);
	}

      if (export == EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}


/* Return the value of a digit. */
static gint
getval (gint c, 
	gint base)
{
  static guchar *digits = "0123456789abcdefABCDEF";
  gint val;

  /* Include uppercase hex digits. */
  if (base == 16)
    base = 22;

  /* Find a match. */
  for (val = 0; val < base; val ++)
    if (c == digits[val])
      return (val < 16) ? val : (val - 6);
  return -1;
}


/* Get a comment */
static gchar *
fgetcomment (FILE *fp)
{
  GString *str = NULL;
  gint comment, c;

  comment = 0;
  do
    {
      c = fgetc (fp);
      if (comment)
	{
	  if (c == '*')
	    {
	      /* In a comment, with potential to leave. */
	      comment = 1;
	    }
	  else if (comment == 1 && c == '/')
	    {
	      gchar *retval;

	      /* Leaving a comment. */
	      comment = 0;

	      retval = g_strstrip (g_strdup (str->str));
	      g_string_free (str, TRUE);
	      return retval;
	    }
	  else
	    {
	      /* In a comment, with no potential to leave. */
	      comment = 2;
	      g_string_append_c (str, c);
	    }
	}
      else
	{
	  /* Not in a comment. */
	  if (c == '/')
	    {
	      /* Potential to enter a comment. */
	      c = fgetc (fp);
	      if (c == '*')
		{
		  /* Entered a comment, with no potential to leave. */
		  comment = 2;
		  str = g_string_new (NULL);
		}
	      else
		{
		  /* put everything back and return */
		  ungetc (c, fp);
		  c = '/';
		  ungetc (c, fp);
		  return NULL;
		}
	    }
	  else if (isspace (c))
	    {
	      /* Skip leading whitespace */
	      continue;
	    }
	}
    }
  while (comment && c != EOF);

  if (str)
    g_string_free (str, TRUE);

  return NULL;
}


/* Same as fgetc, but skip C-style comments and insert whitespace. */
static gint
cpp_fgetc (FILE *fp)
{
  gint comment, c;

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
match (FILE  *fp, 
       gchar *s)
{
  gint c;

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
load_image (gchar *filename)
{
  FILE *fp;
  gint32 image_ID, layer_ID;

  GimpPixelRgn  pixel_rgn;
  GimpDrawable *drawable;
  guchar *data;
  gint    intbits;
  gint    width = 0;
  gint    height = 0;
  gint    x_hot = 0;
  gint    y_hot = 0;
  gint    c, i, j, k;
  gint    tileheight, rowoffset;

  gchar *name_buf;
  gchar *comment;

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

  name_buf = g_strdup_printf (_("Loading %s:"), filename);
  gimp_progress_init (name_buf);
  g_free (name_buf);

  comment = fgetcomment (fp);

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
	  else if (match (fp, "x_hot"))
	    {
	      c = fgetc (fp);
	      if (isspace (c))
		{
		  x_hot = get_int (fp);
		  continue;
		}
	    }
	  else if (match (fp, "y_hot"))
	    {
	      c = fgetc (fp);
	      if (isspace (c))
		{
		  y_hot = get_int (fp);
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

  image_ID = gimp_image_new (width, height, GIMP_INDEXED);
  gimp_image_set_filename (image_ID, filename);

  if (comment)
    {
      GimpParasite *parasite;

      parasite = gimp_parasite_new ("gimp-comment",
				    GIMP_PARASITE_PERSISTENT,
				    strlen (comment) + 1, (gpointer) comment);
      gimp_image_parasite_attach (image_ID, parasite);
      gimp_parasite_free (parasite);

      g_free (comment);
    }

  x_hot = CLAMP (x_hot, 0, width);
  y_hot = CLAMP (y_hot, 0, height);

  if (x_hot > 0 || y_hot > 0)
    {
      GimpParasite *parasite;
      gchar        *str;

      str = g_strdup_printf ("%d %d", x_hot, y_hot);
      parasite = gimp_parasite_new ("hot-spot",
				    GIMP_PARASITE_PERSISTENT,
				    strlen (str) + 1, (gpointer) str);
      g_free (str);
      gimp_image_parasite_attach (image_ID, parasite);
      gimp_parasite_free (parasite);
    }

  /* Set a black-and-white colormap. */
  gimp_image_set_cmap (image_ID, cmap, 2);

  layer_ID = gimp_layer_new (image_ID,
			     _("Background"),
			     width, height,
			     GIMP_INDEXED_IMAGE,
			     100,
			     GIMP_NORMAL_MODE);
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

static gboolean
save_image (gchar    *filename,
	    gchar    *prefix,
	    gchar    *comment,
	    gboolean  save_mask,
	    gint32    image_ID,
	    gint32    drawable_ID)
{
  GimpDrawable *drawable;
  GimpPixelRgn  pixel_rgn;
  FILE *fp;

  gint width, height, colors, dark;
  gint intbits, lineints, need_comma, nints, rowoffset, tileheight;
  gint c, i, j, k, thisbit;

  gboolean has_alpha;
  gint     bpp;

  guchar *data, *cmap, *name_buf, *intfmt;

  drawable = gimp_drawable_get (drawable_ID);
  width  = drawable->width;
  height = drawable->height;
  cmap = gimp_image_get_cmap (image_ID, &colors);

  if (!gimp_drawable_is_indexed (drawable_ID) || colors > 2)
    {
      /* The image is not black-and-white. */
      g_message (_("The image which you are trying to save as\n"
		   "an XBM contains more than two colors.\n\n"
		   "Please convert it to a black and white\n"
		   "(1-bit) indexed image and try again."));
      return FALSE;
    }

  has_alpha = gimp_drawable_has_alpha (drawable_ID);

  if (!has_alpha && save_mask)
    {
      g_message (_("You cannot save a cursor mask for an image\n"
		   "which has no alpha channel."));
      return FALSE;
    }

  bpp = gimp_drawable_bpp (drawable_ID);

  name_buf = g_strdup_printf (_("Saving %s:"), filename);
  gimp_progress_init (name_buf);
  g_free (name_buf);

  /* Figure out which color is black, and which is white. */
  dark = 0;
  if (colors > 1)
    {
      gint first, second;

      /* Maybe the second color is darker than the first. */
      first  = (cmap[0] * cmap[0]) + (cmap[1] * cmap[1]) + (cmap[2] * cmap[2]);
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
  if (*comment)
    fprintf (fp, "/* %s */\n", comment);

  /* Write out the image height and width. */
  fprintf (fp, "#define %s_width %d\n",  prefix, width);
  fprintf (fp, "#define %s_height %d\n", prefix, height);

  /* Write out the hotspot, if any. */
  if (xsvals.use_hot)
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
	   xsvals.x10_format ? "unsigned short" : "unsigned char", prefix);

  /* Allocate a new set of pixels. */
  tileheight = gimp_tile_height ();
  data = (guchar *) g_malloc (width * tileheight * bpp);

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
	  rowoffset = j * width * bpp;
	  c = 0;
	  thisbit = 0;

	  for (k = 0; k < width * bpp; k += bpp)
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
	      if (save_mask)
		{
		  c |= ((data[rowoffset + k + 1] < 128) ? 0 : 1) << (thisbit ++);
		}
	      else
		{
		  if (has_alpha && (data[rowoffset + k + 1] < 128))
		    c |= 0 << (thisbit ++);
		  else
		    c |= ((data[rowoffset + k] == dark) ? 1 : 0) << (thisbit ++);
		}
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

static gint
save_dialog (gint32 drawable_ID)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *toggle;
  GtkWidget *table;
  GtkWidget *entry;
  GtkWidget *spinbutton;
  GtkObject *adj;

  dlg = gimp_dialog_new (_("Save as XBM"), "xbm",
			 gimp_standard_help_func, "filters/xbm.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), save_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* parameter settings */
  frame = gtk_frame_new (_("XBM Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /*  X10 format  */
  toggle = gtk_check_button_new_with_label (_("X10 Format Bitmap"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &xsvals.x10_format);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), xsvals.x10_format);
  gtk_widget_show (toggle);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* prefix */
  entry = gtk_entry_new_with_max_length (MAX_PREFIX);
  gtk_entry_set_text (GTK_ENTRY (entry), xsvals.prefix);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Identifier Prefix:"), 1.0, 0.5,
			     entry, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      GTK_SIGNAL_FUNC (prefix_entry_callback),
                      NULL);

  /* comment string. */
  entry = gtk_entry_new_with_max_length (MAX_COMMENT);
  gtk_widget_set_usize (entry, 240, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), xsvals.comment);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Comment:"), 1.0, 0.5,
			     entry, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      GTK_SIGNAL_FUNC (comment_entry_callback),
                      NULL);

  /* hotspot toggle */
  toggle = gtk_check_button_new_with_label (_("Write Hot Spot Values"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &xsvals.use_hot);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), xsvals.use_hot);
  gtk_widget_show (toggle);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  gtk_object_set_data (GTK_OBJECT (toggle), "set_sensitive", table);
  gtk_widget_set_sensitive (table, xsvals.use_hot);

  spinbutton = gimp_spin_button_new (&adj, xsvals.x_hot, 0,
				     gimp_drawable_width (drawable_ID) - 1,
				     1, 1, 1, 0, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &xsvals.x_hot);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Hot Spot X:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  spinbutton = gimp_spin_button_new (&adj, xsvals.y_hot, 0,
				     gimp_drawable_height (drawable_ID) - 1,
				     1, 1, 1, 0, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &xsvals.y_hot);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Y:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  /* mask file */
  frame = gtk_frame_new (_("Mask File"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  toggle = gtk_check_button_new_with_label (_("Write Extra Mask File"));
  gtk_table_attach_defaults (GTK_TABLE (table), toggle, 0, 2, 0, 1);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &xsvals.write_mask);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), xsvals.write_mask);
  gtk_widget_show (toggle);

  entry = gtk_entry_new_with_max_length (MAX_MASK_EXT);
  gtk_entry_set_text (GTK_ENTRY (entry), xsvals.mask_ext);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Mask File Extension:"), 1.0, 0.5,
			     entry, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      GTK_SIGNAL_FUNC (mask_ext_entry_callback),
                      NULL);

  gtk_object_set_data (GTK_OBJECT (toggle), "set_sensitive", entry);
  gtk_widget_set_sensitive (entry, xsvals.write_mask);

  gtk_widget_set_sensitive (frame, gimp_drawable_has_alpha (drawable_ID));

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
  strncpy (xsvals.prefix,
	   gtk_entry_get_text (GTK_ENTRY (widget)), MAX_PREFIX);
}

static void
mask_ext_entry_callback (GtkWidget *widget,
		       gpointer   data)
{
  memset (xsvals.prefix, 0, sizeof (xsvals.mask_ext));
  strncpy (xsvals.mask_ext,
	   gtk_entry_get_text (GTK_ENTRY (widget)), MAX_MASK_EXT);
}

static void
save_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  xsint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
