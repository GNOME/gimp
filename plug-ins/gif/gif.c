/* GIF loading and saving file filter for The GIMP version 1.0
 *
 *    - Adam D. Moss
 *    - Peter Mattis
 *    - Spencer Kimball
 *
 *      Based around original GIF code by David Koblas.
 *
 *
 * Version 2.0.3 - 98/05/18
 *                        Adam D. Moss - <adam@gimp.org> <adam@foxbox.org>
 */
/*
 * This filter uses code taken from the "giftopnm" and "ppmtogif" programs
 *    which are part of the "netpbm" package.
 */
/*
 *  "The Graphics Interchange Format(c) is the Copyright property of
 *  CompuServe Incorporated.  GIF(sm) is a Service Mark property of
 *  CompuServe Incorporated." 
 */

/*
 * REVISION HISTORY
 *
 *
 * 98/05/18
 * 2.00.03 - If we did manage to decode at least one frame of a
 *           gif, be sure to display the resulting image even if
 *           it terminated abruptly.
 *
 * 98/04/28
 * 2.00.02 - Fixed a bug with (ms) tag parsing.
 *
 * 98/03/16
 * 2.00.01 - Fixed a long-standing bug when loading GIFs which layer
 *           opaque frames onto transparent ones.
 *
 * 98/03/15
 * 2.00.00 - No longer beta.  Uses the current GIMP brush background
 *           colour as the transparent-index colour for viewers that
 *           don't know about transparency, instead of magenta.  Note
 *           that this is by no means likely to actually work, but
 *           is more likely to do so if your image has been freshly
 *           to-index'd before saving.
 *
 *           Also added some analysis to gif-reading to prevent the
 *           number of encoded bits being pumped up inadvertantly for
 *           successive load/saves of the same image.  [Adam]
 *
 * 97/12/11
 * 1.99.18 - Bleh.  Disposals should specify how the next frame will
 *           be composed with this frame, NOT how this frame will
 *           be composed with the previous frame.  Fixed.  [Adam]
 *
 * 97/11/30
 * 1.99.17 - No more bogus transparency indices in animated GIFs,
 *           hopefully.  Saved files are better-behaved, sometimes
 *           smaller.  [Adam]
 *
 * 97/09/29
 * 1.99.16 - Added a dialog for the user to choose what to do if
 *           one of the layers of the image extends beyond the image
 *           borders - crop or cancel.  Added code behind it.
 *
 *           Corrected the number of bits for the base image to be
 *           on the generous side.  Hopefully we can no longer generate
 *           GIFs which make XV barf.
 *
 *           Now a lot more careful about whether we choose to encode
 *           as a GIF87a or a GIF89a.  Hopefully does everything by the
 *           book.  It should now be nigh-on impossible to torture the
 *           plugin into generating a GIF which isn't extremely well
 *           behaved with respect to the GIF89a specification.
 *
 *           Fixed(?) a lot of dialog callback details, should now be
 *           happy with window deletion (GTK+970925).  Fixed the
 *           cancellation of a save.  [Adam]
 *
 * 97/09/16
 * 1.99.15 - Hey!  We can now cope with loading images which change
 *           colourmap between frames.  This plugin will never save
 *           such abominations of nature while I still live, though.
 *           There should be no noncorrupt GIF in the universe which
 *           GIMP can't load and play now.  [Adam]
 *
 * 97/09/14
 * 1.99.14 - Added a check for layers whose extents don't lay
 *           within the image boundaries, which would make it a
 *           lot harder to generate badly-behaved GIFs.  Doesn't
 *           do anything about it yet, but it should crop all layers
 *           to the image boundaries.  Also, there's now a (separate)
 *           animation-preview plugin!  [Adam]
 *
 * 97/08/29
 * 1.99.13 - Basic ability to embed GIF comments within saved images.
 *           Fixed a bug with encoding the number of loops in a GIF file -
 *           would have been important, but we're not using that feature
 *           yet anyway.  ;)
 *           Subtly improved dialog layout a little. [Adam]
 *
 * 97/07/25
 * 1.99.12 - Fixed attempts to load GIFs which don't exist.  Made a
 *           related cosmetic adjustment. [Adam]
 *
 * 97/07/10
 * 1.99.11 - Fixed a bug with loading and saving GIFs where the bottom
 *           layer wasn't the same size as the image. [Adam]
 *
 * 97/07/06
 * 1.99.10 - New 'save' dialog, now most of the default behaviour of
 *           animated GIF saving is user-settable (looping, default
 *           time between frames, etc.)
 *           PDB entry for saving is no longer compatible.  Fortunately
 *           I don't think that anyone is using file_gif_save in
 *           scripts.  [Adam]
 *
 * 97/07/05
 * 1.99.9  - More animated GIF work: now loads and saves frame disposal
 *           information.  This is neat and will also allow some delta
 *           stuff in the future.
 *           The disposal-method is kept in the layer name, like the timing
 *           info.
 *           (replace) - this frame replaces whatever else has been shown
 *                       so far.
 *           (combine) - this frame builds apon the previous frame.
 *           If a disposal method is not specified, it is assumed to mean
 *           "don't care."  [Adam]
 *
 * 97/07/04
 * 1.99.8  - Can save per-frame timing information too, now.  The time
 *           for which a frame is visible is specified within the layer name
 *           as i,e. (250ms).  If a frame doesn't have this timing value
 *           it defaults to lasting 100ms. [Adam]
 *
 * 97/07/02
 * 1.99.7  - For animated GIFs, fixed the saving of timing information for
 *           frames which couldn't be made transparent.
 *           Added the loading of timing information into the layer
 *           names.  Adjusted GIMP's GIF magic number very slightly. [Adam]
 *
 * 97/06/30
 * 1.99.6  - Now saves GRAY and GRAYA images, albeit not always
 *           optimally (yet). [Adam]
 *
 * 97/06/25
 * 1.99.5  - Good, the transparancy-on-big-architectures bug is
 *           fixed.  Cleaned up some stuff.
 *           (Adam D. Moss, adam@foxbox.org)
 *
 * 97/06/23
 * 1.99.4  - Trying to fix some endianness/word-size problems with
 *           transparent gif-saving on some architectures... does
 *           this help?  (Adam D. Moss, adam@foxbox.org)
 *
 * 97/05/18
 * 1.99.3  - Fixed the problem with GIFs getting loop extensions even
 *           if they only had one frame (thanks to Zach for noticing -
 *           git!  :) )  (Adam D. Moss, adam@foxbox.org)
 *
 * 97/05/17
 * 1.99.2  - Can now save animated GIFs.  Correctly handles saving of
 *           image offsets.  Uses N*tscape extentions to loop infinitely.
 *           Some notable shortcomings - see TODO list below.
 *           (Adam D. Moss, adam@foxbox.org)
 *
 * 97/05/16
 * 1.99.1  - Implemented image offsets in animated GIF loading.  Requires
 *           a fix to gimp_layer_translate in libgimp/gimplayer.c if used
 *           with GIMP versions <= 0.99.10.  Started work on saving animated
 *           GIFs.  Started TODO list.  (Adam D. Moss, adam@foxbox.org)
 *
 * 97/05/15
 * 1.99.0  - Started revision log.  GIF plugin now loads/saves INDEXED
 *           and INDEXEDA images with correct transparency where possible.
 *           Loads multi-image (animated) GIFs as a framestack implemented
 *           in GIMP layers.  Some bug fixes to original code, some new bugs
 *           cheerfully added.  (Adam D. Moss, adam@foxbox.org)
 *
 * Previous versions - load/save INDEXED images.
 *           (Peter Mattis & Spencer Kimball, gimp@scam.xcf.berkeley.edu)
 */

/*
 * TODO (more *'s means more important!)
 *
 * - animated GIF optimization routines (seperate plugin)
 *
 * - Show GIF comments in a popup window?
 *
 * - PDB stuff for comments
 *
 * - 'requantize' option for INDEXEDA images which really have 256 colours
 *   in them
 *
 * - Be a bit smarter about finding unused/superfluous colour indices for
 *   lossless colour crunching of INDEXEDA images.  (Specifically, look
 *   for multiple indices which correspond to the same physical colour.)
 *
 * - Tidy up parameters for the GIFEncode routines
 *
 * - Remove unused colourmap entries for GRAYSCALE images.
 *
 * - All the warning messages etc. which the plugin prints to STDOUT should
 *   pop up in windows.
 *
 * - * Button to activate the animation preview plugin from the GIF-save
 *     dialog.
 *
 */

/* Copyright notice for code which this plugin was derived from - */
/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993, David Koblas.  (koblas@netcom.com)    | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"


/* Wear your GIMP with pride! */
#define DEFAULT_COMMENT "Made with GIMP"


typedef struct
{
  int interlace;
  int loop;
  int default_delay;
  int default_dispose;
} GIFSaveVals;

typedef struct
{
  gint run;
} GIFSaveInterface;



/* Declare some local functions.
 */
static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
static gint32 load_image (char   *filename);
static gint   save_image (char   *filename,
			  gint32  image_ID,
			  gint32  drawable_ID);

static gboolean
boundscheck (gint32 image_ID);
static gboolean
badbounds_dialog ( void );

static void
cropok_callback (GtkWidget *widget,
		 gpointer   data);
static void
cropclose_callback (GtkWidget *widget,
		    gpointer   data);
static void
cropcancel_callback (GtkWidget *widget,
		     gpointer   data);

static gint   save_dialog ( gint32 image_ID );

static void   save_close_callback  (GtkWidget *widget,
				    gpointer   data);
static void   save_ok_callback     (GtkWidget *widget,
				    gpointer   data);
static void   save_cancel_callback     (GtkWidget *widget,
					gpointer   data);
static void   save_windelete_callback     (GtkWidget *widget,
					   gpointer   data);
static void   save_toggle_update   (GtkWidget *widget,
				    gpointer   data);
static void   save_entry_callback  (GtkWidget *widget,
                                    gpointer   data);
static void   comment_entry_callback  (GtkWidget *widget,
				       gpointer   data);



static gint     radio_pressed[3];

static guchar   used_cmap[3][256];
static gboolean can_crop;
static guchar   highest_used_index;
static gboolean promote_to_rgb = FALSE;
static guchar   gimp_cmap[768];




GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static GIFSaveVals gsvals =
{
  FALSE,   /* interlace */
  TRUE,    /* loop infinitely */
  100,     /* default_delay between frames (100ms) */
  0        /* default_dispose = "don't care" */
};

static GIFSaveInterface gsint =
{
  FALSE   /*  run  */
};



MAIN ()

static void
query ()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name entered" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Image to save" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name entered" },
    { PARAM_INT32, "interlace", "Save as interlaced" },
    { PARAM_INT32, "loop", "(animated gif) loop infinitely" },
    { PARAM_INT32, "default_delay", "(animated gif) Default delay between framese in milliseconds" },
    { PARAM_INT32, "default_dispose", "(animated gif) Default disposal type (0=`don't care`, 1=combine, 2=replace)" }
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_gif_load",
                          "loads files of Compuserve GIF file format",
                          "FIXME: write help for gif_load",
                          "Spencer Kimball, Peter Mattis, Adam Moss, David Koblas",
                          "Spencer Kimball, Peter Mattis, Adam Moss, David Koblas",
                          "1995-1997",
                          "<Load>/GIF",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_gif_save",
                          "saves files in Compuserve GIF file format",
                          "FIXME: write help for gif_save",
                          "Spencer Kimball, Peter Mattis, Adam Moss, David Koblas",
                          "Spencer Kimball, Peter Mattis, Adam Moss, David Koblas",
                          "1995-1997",
                          "<Save>/GIF",
			  "INDEXED*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_gif_load", "gif", "", "0,string,GIF8");
  gimp_register_save_handler ("file_gif_save", "gif", "");
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
  gchar **argv;
  gint argc;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 2;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_gif_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      /* The GIF format only tells you how many bits per pixel
       *  are in the image, not the actual number of used indices (D'OH!)
       *
       * So if we're not careful, repeated load/save of a transparent GIF
       *  without intermediate indexed->RGB->indexed pumps up the number of
       *  bits used, as we add an index each time for the transparent
       *  colour.  Ouch.  We either do some heavier analysis at save-time,
       *  or trim down the number of GIMP colours at load-time.  We do the
       *  latter for now.
       */
      g_print ("GIF: Highest used index is %d\n", highest_used_index);
      if (!promote_to_rgb)
	{
	  gimp_image_set_cmap (image_ID, gimp_cmap, highest_used_index+1);
	}

      if (image_ID != -1)
        {
          values[0].data.d_status = STATUS_SUCCESS;
          values[1].type = PARAM_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          values[0].data.d_status = STATUS_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, "file_gif_save") == 0)
    {
      argc = 1;
      argv = g_new (gchar *, 1);
      argv[0] = g_strdup ("gif");
      
      gtk_init (&argc, &argv);
      gtk_rc_parse (gimp_gtkrc ());
 

      if (boundscheck(param[1].data.d_int32))
	/* The image may or may not have had layers out of
	   bounds, but the user didn't mind cropping it down. */
	{
	  switch (run_mode)
	    {
	    case RUN_INTERACTIVE:
	      {
		/*  Possibly retrieve data  */
		gimp_get_data ("file_gif_save", &gsvals);
		
		/*  First acquire information with a dialog  */
		radio_pressed[0] =
		  radio_pressed[1] =
		  radio_pressed[2] = FALSE;
		
		radio_pressed[gsvals.default_dispose] = TRUE;
		
		if (! save_dialog (param[1].data.d_int32))
		  {
		    *nreturn_vals = 1;
		    values[0].data.d_status = STATUS_EXECUTION_ERROR;
		    fflush(stdout);
		    return;
		  }

		if (radio_pressed[0]) gsvals.default_dispose = 0x00;
		else
		  if (radio_pressed[1]) gsvals.default_dispose = 0x01;
		  else
		    if (radio_pressed[2]) gsvals.default_dispose = 0x02;
	      }
	      break;
	      
	    case RUN_NONINTERACTIVE:
	      /*  Make sure all the arguments are there!  */
	      if (nparams != 9)
		status = STATUS_CALLING_ERROR;
	      if (status == STATUS_SUCCESS)
		{
		  gsvals.interlace = (param[5].data.d_int32) ? TRUE : FALSE;
		  gsvals.loop = (param[6].data.d_int32) ? TRUE : FALSE;
		  gsvals.default_delay = param[7].data.d_int32;
		  gsvals.default_dispose = param[8].data.d_int32;
		}
	      break;
	      
	    case RUN_WITH_LAST_VALS:
	      /*  Possibly retrieve data  */
	      gimp_get_data ("file_gif_save", &gsvals);
	      break;
	      
	    default:
	      break;
	    }

	  *nreturn_vals = 1;
	  if (save_image (param[3].data.d_string,
			  param[1].data.d_int32,
			  param[2].data.d_int32))
	    {
	      /*  Store psvals data  */
	      gimp_set_data ("file_gif_save", &gsvals, sizeof (GIFSaveVals));
	      
	      values[0].data.d_status = STATUS_SUCCESS;
	    }
	  else
	    values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
      else /* Some layers were out of bounds and the user wishes
	      to abort.  */
	{
	  *nreturn_vals = 1;
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
    }
}


#define MAXCOLORMAPSIZE  256

#define CM_RED           0
#define CM_GREEN         1
#define CM_BLUE          2

#define MAX_LWZ_BITS     12

#define INTERLACE          0x40
#define LOCALCOLORMAP      0x80
#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len) (fread(buffer, len, 1, file) != 0)
#define LM_to_uint(a,b)         (((b)<<8)|(a))

#define GRAYSCALE        1
#define COLOR            2

typedef unsigned char CMap[3][MAXCOLORMAPSIZE];

static struct
{
  unsigned int Width;
  unsigned int Height;
  CMap ColorMap;
  unsigned int BitPixel;
  unsigned int ColorResolution;
  unsigned int Background;
  unsigned int AspectRatio;
  /*
   **
   */
  int GrayScale;
} GifScreen;

static struct
{
  int transparent;
  int delayTime;
  int inputFlag;
  int disposal;
} Gif89 = { -1, -1, -1, 0 };

int verbose = FALSE;
int showComment = TRUE;
char *globalcomment = NULL;
gint globalusecomment = TRUE;

static int ReadColorMap (FILE *, int, CMap, int *);
static int DoExtension (FILE *, int);
static int GetDataBlock (FILE *, unsigned char *);
static int GetCode (FILE *, int, int);
static int LWZReadByte (FILE *, int, int);
static gint32 ReadImage (FILE *, char *, int, int, CMap, int, int, int, int,
			 guint, guint, guint, guint);


static gint32
load_image (char *filename)
{
  FILE *fd;
  char * name_buf;
  unsigned char buf[16];
  unsigned char c;
  CMap localColorMap;
  int grayScale;
  int useGlobalColormap;
  int bitPixel;
  int imageCount = 0;
  char version[4];
  gint32 image_ID = -1;

  fd = fopen (filename, "rb");
  if (!fd)
    {
      g_message ("GIF: can't open \"%s\"\n", filename);
      return -1;
    }

  name_buf = g_malloc (strlen (filename) + 11);

  sprintf (name_buf, "Loading %s:", filename);
  gimp_progress_init (name_buf);
  g_free (name_buf);

  if (!ReadOK (fd, buf, 6))
    {
      g_message ("GIF: error reading magic number\n");
      return -1;
    }

  if (strncmp ((char *) buf, "GIF", 3) != 0)
    {
      g_message ("GIF: not a GIF file\n");
      return -1;
    }

  strncpy (version, (char *) buf + 3, 3);
  version[3] = '\0';

  if ((strcmp (version, "87a") != 0) && (strcmp (version, "89a") != 0))
    {
      g_message ("GIF: bad version number, not '87a' or '89a'\n");
      return -1;
    }

  if (!ReadOK (fd, buf, 7))
    {
      g_message ("GIF: failed to read screen descriptor\n");
      return -1;
    }

  GifScreen.Width = LM_to_uint (buf[0], buf[1]);
  GifScreen.Height = LM_to_uint (buf[2], buf[3]);
  GifScreen.BitPixel = 2 << (buf[4] & 0x07);
  GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
  GifScreen.Background = buf[5];
  GifScreen.AspectRatio = buf[6];

  if (BitSet (buf[4], LOCALCOLORMAP))
    {
      /* Global Colormap */
      if (ReadColorMap (fd, GifScreen.BitPixel, GifScreen.ColorMap, &GifScreen.GrayScale))
	{
	  g_message ("GIF: error reading global colormap\n");
	  return -1;
	}
    }

  if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49)
    {
      g_message ("GIF: warning - non-square pixels\n");
    }


  highest_used_index = 0;
      

  for (;;)
    {
      if (!ReadOK (fd, &c, 1))
	{
	  g_message ("GIF: EOF / read error on image data\n");
	  return image_ID; /* will be -1 if failed on first image! */
	}

      if (c == ';')
	{
	  /* GIF terminator */
	  return image_ID;
	}

      if (c == '!')
	{
	  /* Extension */
	  if (!ReadOK (fd, &c, 1))
	    {
	      g_message ("GIF: OF / read error on extention function code\n");
	      return image_ID; /* will be -1 if failed on first image! */
	    }
	  DoExtension (fd, c);
	  continue;
	}

      if (c != ',')
	{
	  /* Not a valid start character */
	  g_message ("GIF: bogus character 0x%02x, ignoring\n", (int) c);
	  continue;
	}

      ++imageCount;

      if (!ReadOK (fd, buf, 9))
	{
	  g_message ("GIF: couldn't read left/top/width/height\n");
	  return image_ID; /* will be -1 if failed on first image! */
	}

      useGlobalColormap = !BitSet (buf[8], LOCALCOLORMAP);

      bitPixel = 1 << ((buf[8] & 0x07) + 1);

      if (!useGlobalColormap)
	{
	  if (ReadColorMap (fd, bitPixel, localColorMap, &grayScale))
	    {
	      g_message ("GIF: error reading local colormap\n");
	      return image_ID; /* will be -1 if failed on first image! */
	    }
	  image_ID = ReadImage (fd, filename, LM_to_uint (buf[4], buf[5]),
				LM_to_uint (buf[6], buf[7]),
				localColorMap, bitPixel,
				grayScale,
				BitSet (buf[8], INTERLACE), imageCount,
				(guint) LM_to_uint (buf[0], buf[1]),
				(guint) LM_to_uint (buf[2], buf[3]),
				GifScreen.Width,
				GifScreen.Height
				);
	}
      else
	{
	  image_ID = ReadImage (fd, filename, LM_to_uint (buf[4], buf[5]),
				LM_to_uint (buf[6], buf[7]),
				GifScreen.ColorMap, GifScreen.BitPixel,
				GifScreen.GrayScale,
				BitSet (buf[8], INTERLACE), imageCount,
				(guint) LM_to_uint (buf[0], buf[1]),
				(guint) LM_to_uint (buf[2], buf[3]),
				GifScreen.Width,
				GifScreen.Height
				);
	}

    }

  return image_ID;
}

static int
ReadColorMap (FILE *fd,
	      int   number,
	      CMap  buffer,
	      int  *format)
{
  int i;
  unsigned char rgb[3];
  int flag;

  flag = TRUE;

  for (i = 0; i < number; ++i)
    {
      if (!ReadOK (fd, rgb, sizeof (rgb)))
	{
	  g_message ("GIF: bad colormap\n");
	  return TRUE;
	}

      buffer[CM_RED][i] = rgb[0];
      buffer[CM_GREEN][i] = rgb[1];
      buffer[CM_BLUE][i] = rgb[2];

      flag &= (rgb[0] == rgb[1] && rgb[1] == rgb[2]);
    }

  *format = (flag) ? GRAYSCALE : COLOR;

  return FALSE;
}

static int
DoExtension (FILE *fd,
	     int   label)
{
  static guchar buf[256];
  char *str;

  switch (label)
    {
    case 0x01:			/* Plain Text Extension */
      str = "Plain Text Extension";
#ifdef notdef
      if (GetDataBlock (fd, (unsigned char *) buf) == 0)
	;

      lpos = LM_to_uint (buf[0], buf[1]);
      tpos = LM_to_uint (buf[2], buf[3]);
      width = LM_to_uint (buf[4], buf[5]);
      height = LM_to_uint (buf[6], buf[7]);
      cellw = buf[8];
      cellh = buf[9];
      foreground = buf[10];
      background = buf[11];

      while (GetDataBlock (fd, (unsigned char *) buf) != 0)
	{
	  PPM_ASSIGN (image[ypos][xpos],
		      cmap[CM_RED][v],
		      cmap[CM_GREEN][v],
		      cmap[CM_BLUE][v]);
	  ++index;
	}

      return FALSE;
#else
      break;
#endif
    case 0xff:			/* Application Extension */
      str = "Application Extension";
      break;
    case 0xfe:			/* Comment Extension */
      str = "Comment Extension";
      while (GetDataBlock (fd, (unsigned char *) buf) != 0)
	{
	  if (showComment)
	    g_print ("GIF: gif comment: %s\n", buf);
	}
      return FALSE;
    case 0xf9:			/* Graphic Control Extension */
      str = "Graphic Control Extension";
      (void) GetDataBlock (fd, (unsigned char *) buf);
      Gif89.disposal = (buf[0] >> 2) & 0x7;
      Gif89.inputFlag = (buf[0] >> 1) & 0x1;
      Gif89.delayTime = LM_to_uint (buf[1], buf[2]);
      if ((buf[0] & 0x1) != 0)
	Gif89.transparent = buf[3];
      else
	Gif89.transparent = -1;

      while (GetDataBlock (fd, (unsigned char *) buf) != 0)
	;
      return FALSE;
    default:
      str = (char *)buf;
      sprintf ((char *)buf, "UNKNOWN (0x%02x)", label);
      break;
    }

  g_print ("GIF: got a '%s' extension\n", str);

  while (GetDataBlock (fd, (unsigned char *) buf) != 0)
    ;

  return FALSE;
}

int ZeroDataBlock = FALSE;

static int
GetDataBlock (FILE          *fd,
	      unsigned char *buf)
{
  unsigned char count;

  if (!ReadOK (fd, &count, 1))
    {
      g_message ("GIF: error in getting DataBlock size\n");
      return -1;
    }

  ZeroDataBlock = count == 0;

  if ((count != 0) && (!ReadOK (fd, buf, count)))
    {
      g_message ("GIF: error in reading DataBlock\n");
      return -1;
    }

  return count;
}

static int
GetCode (FILE *fd,
	 int   code_size,
	 int   flag)
{
  static unsigned char buf[280];
  static int curbit, lastbit, done, last_byte;
  int i, j, ret;
  unsigned char count;

  if (flag)
    {
      curbit = 0;
      lastbit = 0;
      done = FALSE;
      return 0;
    }

  if ((curbit + code_size) >= lastbit)
    {
      if (done)
	{
	  if (curbit >= lastbit)
	    {
	      g_message ("GIF: ran off the end of by bits\n");
	      gimp_quit ();
	    }
	  return -1;
	}
      buf[0] = buf[last_byte - 2];
      buf[1] = buf[last_byte - 1];

      if ((count = GetDataBlock (fd, &buf[2])) == 0)
	done = TRUE;

      last_byte = 2 + count;
      curbit = (curbit - lastbit) + 16;
      lastbit = (2 + count) * 8;
    }

  ret = 0;
  for (i = curbit, j = 0; j < code_size; ++i, ++j)
    ret |= ((buf[i / 8] & (1 << (i % 8))) != 0) << j;

  curbit += code_size;

  return ret;
}

static int
LWZReadByte (FILE *fd,
	     int   flag,
	     int   input_code_size)
{
  static int fresh = FALSE;
  int code, incode;
  static int code_size, set_code_size;
  static int max_code, max_code_size;
  static int firstcode, oldcode;
  static int clear_code, end_code;
  static int table[2][(1 << MAX_LWZ_BITS)];
  static int stack[(1 << (MAX_LWZ_BITS)) * 2], *sp;
  register int i;

  if (flag)
    {
      set_code_size = input_code_size;
      code_size = set_code_size + 1;
      clear_code = 1 << set_code_size;
      end_code = clear_code + 1;
      max_code_size = 2 * clear_code;
      max_code = clear_code + 2;

      GetCode (fd, 0, TRUE);

      fresh = TRUE;

      for (i = 0; i < clear_code; ++i)
	{
	  table[0][i] = 0;
	  table[1][i] = i;
	}
      for (; i < (1 << MAX_LWZ_BITS); ++i)
	table[0][i] = table[1][0] = 0;

      sp = stack;

      return 0;
    }
  else if (fresh)
    {
      fresh = FALSE;
      do
	{
	  firstcode = oldcode =
	    GetCode (fd, code_size, FALSE);
	}
      while (firstcode == clear_code);
      return firstcode;
    }

  if (sp > stack)
    return *--sp;

  while ((code = GetCode (fd, code_size, FALSE)) >= 0)
    {
      if (code == clear_code)
	{
	  for (i = 0; i < clear_code; ++i)
	    {
	      table[0][i] = 0;
	      table[1][i] = i;
	    }
	  for (; i < (1 << MAX_LWZ_BITS); ++i)
	    table[0][i] = table[1][i] = 0;
	  code_size = set_code_size + 1;
	  max_code_size = 2 * clear_code;
	  max_code = clear_code + 2;
	  sp = stack;
	  firstcode = oldcode =
	    GetCode (fd, code_size, FALSE);
	  return firstcode;
	}
      else if (code == end_code)
	{
	  int count;
	  unsigned char buf[260];

	  if (ZeroDataBlock)
	    return -2;

	  while ((count = GetDataBlock (fd, buf)) > 0)
	    ;

	  if (count != 0)
	    g_print ("GIF: missing EOD in data stream (common occurence)");
	  return -2;
	}

      incode = code;

      if (code >= max_code)
	{
	  *sp++ = firstcode;
	  code = oldcode;
	}

      while (code >= clear_code)
	{
	  *sp++ = table[1][code];
	  if (code == table[0][code])
	    {
	      g_message ("GIF: circular table entry BIG ERROR\n");
	      gimp_quit ();
	    }
	  code = table[0][code];
	}

      *sp++ = firstcode = table[1][code];

      if ((code = max_code) < (1 << MAX_LWZ_BITS))
	{
	  table[0][code] = oldcode;
	  table[1][code] = firstcode;
	  ++max_code;
	  if ((max_code >= max_code_size) &&
	      (max_code_size < (1 << MAX_LWZ_BITS)))
	    {
	      max_code_size *= 2;
	      ++code_size;
	    }
	}

      oldcode = incode;

      if (sp > stack)
	return *--sp;
    }
  return code;
}

static gint32
ReadImage (FILE *fd,
	   char *filename,
	   int   len,
	   int   height,
	   CMap  cmap,
	   int   ncols,
	   int   format,
	   int   interlace,
	   int   number,
	   guint   leftpos,
	   guint   toppos,
	   guint screenwidth,
	   guint screenheight)
{
  static gint32 image_ID;
  static gint frame_number = 1;

  gint32 layer_ID;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  guchar *dest, *temp;
  guchar c;
  gint xpos = 0, ypos = 0, pass = 0;
  gint cur_progress, max_progress;
  gint v;
  gint i, j;
  gchar framename[200]; /* FIXME */
  gboolean alpha_frame = FALSE;
  int nreturn_vals;
  static int previous_disposal;



  /*
   **  Initialize the Compression routines
   */
  if (!ReadOK (fd, &c, 1))
    {
      g_message ("GIF: EOF / read error on image data\n");
      return -1;
    }

  if (LWZReadByte (fd, TRUE, c) < 0)
    {
      g_message ("GIF: error while reading\n");
      return -1;
    }

  if (frame_number == 1 )
    {
      image_ID = gimp_image_new (screenwidth, screenheight, INDEXED);
      gimp_image_set_filename (image_ID, filename);

      for (i = 0, j = 0; i < ncols; i++)
	{
	  used_cmap[0][i] = gimp_cmap[j++] = cmap[0][i];
	  used_cmap[1][i] = gimp_cmap[j++] = cmap[1][i];
	  used_cmap[2][i] = gimp_cmap[j++] = cmap[2][i];
	}

      gimp_image_set_cmap (image_ID, gimp_cmap, ncols);

      if (Gif89.delayTime < 0)
	strcpy(framename, "Background");
      else
	sprintf(framename, "Background (%dms)", 10*Gif89.delayTime);

      previous_disposal = Gif89.disposal;

      if (Gif89.transparent == -1)
	{
	  layer_ID = gimp_layer_new (image_ID, framename,
				     len, height,
				     INDEXED_IMAGE, 100, NORMAL_MODE);
	}
      else
	{
	  layer_ID = gimp_layer_new (image_ID, framename,
				     len, height,
				     INDEXEDA_IMAGE, 100, NORMAL_MODE);
	  alpha_frame=TRUE;
	}
    }
  else /* NOT FIRST FRAME */
    {
      /* If the colourmap is now different, we have to promote to
	 RGB! */
      if (!promote_to_rgb)
	{
	  for (i=0;i<ncols;i++)
	    {
	      if (
		  (used_cmap[0][i] != cmap[0][i]) ||
		  (used_cmap[1][i] != cmap[1][i]) ||
		  (used_cmap[2][i] != cmap[2][i])
		  )
		{ /* Everything is RGB(A) from now on... sigh. */
		  promote_to_rgb = TRUE;
		  
		  /* Promote everything we have so far into RGB(A) */
		  g_print ("GIF: Promoting image to RGB...\n");
		  gimp_run_procedure("gimp_convert_rgb", &nreturn_vals,
				     PARAM_IMAGE, image_ID,
				     PARAM_END);
		  break;
		}
	    }
	}

      if (Gif89.delayTime < 0)
	sprintf(framename, "Frame %d", frame_number);
      else
	sprintf(framename, "Frame %d (%dms)",
		frame_number, 10*Gif89.delayTime);

      switch (previous_disposal)
	{
	case 0x00: break; /* 'don't care' */
	case 0x01: strcat(framename," (combine)"); break;
	case 0x02: strcat(framename," (replace)"); break;
	case 0x03: strcat(framename," (combine)"); break;
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	  strcat(framename," (unknown disposal)");
	  g_message ("GIF: Hmm... please forward this GIF to the "
		     "GIF plugin author!\n  (adam@foxbox.org)\n");
	  break;
	default: g_message ("GIF: Something got corrupted.\n"); break;
	}

      previous_disposal = Gif89.disposal;

      layer_ID = gimp_layer_new (image_ID, framename,
				 len, height,
				 promote_to_rgb ? RGBA_IMAGE : INDEXEDA_IMAGE,
				 100, NORMAL_MODE);
      alpha_frame = TRUE;
    }

  frame_number++;

  gimp_image_add_layer (image_ID, layer_ID, 0);
  gimp_layer_translate (layer_ID, (gint)leftpos, (gint)toppos);

  drawable = gimp_drawable_get (layer_ID);

  cur_progress = 0;
  max_progress = height;

  if (alpha_frame)
    dest = (guchar *) g_malloc (len * height *
				(promote_to_rgb ? 4 : 2));
  else
    dest = (guchar *) g_malloc (len * height);

  if (verbose)
    g_print ("GIF: reading %d by %d%s GIF image, ncols=%d\n",
	     len, height, interlace ? " interlaced" : "", ncols);

  if (!alpha_frame && promote_to_rgb)
    {
      g_message ("GIF: Ouchie!  Can't handle non-alpha RGB frames.\n     Please mail the plugin author.  (adam@gimp.org)\n");
      exit(-1);
    }

  while ((v = LWZReadByte (fd, FALSE, c)) >= 0)
    {
      if (alpha_frame)
	{
	  if (((guchar)v > highest_used_index) && !(v == Gif89.transparent))
	    highest_used_index = (guchar)v;

	  if (promote_to_rgb)
	    {
	      temp = dest + ( (ypos * len) + xpos ) * 4;
	      *(temp  ) = (guchar) cmap[0][v];
	      *(temp+1) = (guchar) cmap[1][v];
	      *(temp+2) = (guchar) cmap[2][v];
	      *(temp+3) = (guchar) ((v == Gif89.transparent) ? 0 : 255);
	    }
	  else
	    {
	      temp = dest + ( (ypos * len) + xpos ) * 2;
	      *temp = (guchar) v;
	      *(temp+1) = (guchar) ((v == Gif89.transparent) ? 0 : 255);
	    }
	}
      else
	{
	  if ((guchar)v > highest_used_index)
	    highest_used_index = (guchar)v;

	  temp = dest + (ypos * len) + xpos;
	  *temp = (guchar) v;
	}

      ++xpos;
      if (xpos == len)
	{
	  xpos = 0;
	  if (interlace)
	    {
	      switch (pass)
		{
		case 0:
		case 1:
		  ypos += 8;
		  break;
		case 2:
		  ypos += 4;
		  break;
		case 3:
		  ypos += 2;
		  break;
		}

	      if (ypos >= height)
		{
		  ++pass;
		  switch (pass)
		    {
		    case 1:
		      ypos = 4;
		      break;
		    case 2:
		      ypos = 2;
		      break;
		    case 3:
		      ypos = 1;
		      break;
		    default:
		      goto fini;
		    }
		}
	    }
	  else
	    {
	      ++ypos;
	    }

	  cur_progress++;
	  if ((cur_progress % 16) == 0)
	    gimp_progress_update ((double) cur_progress / (double) max_progress);
	}
      if (ypos >= height)
	break;
    }

fini:
  if (LWZReadByte (fd, FALSE, c) >= 0)
    g_print ("GIF: too much input data, ignoring extra...\n");

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, FALSE);
  gimp_pixel_rgn_set_rect (&pixel_rgn, dest, 0, 0, drawable->width, drawable->height);

  g_free (dest);

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  return image_ID;
}



/* ppmtogif.c - read a portable pixmap and produce a GIF file
**
** Based on GIFENCOD by David Rowley <mgardi@watdscu.waterloo.edu>.A
** Lempel-Zim compression based on "compress".
**
** Modified by Marcel Wijkstra <wijkstra@fwi.uva.nl>
**
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** The Graphics Interchange Format(c) is the Copyright property of
** CompuServe Incorporated.  GIF(sm) is a Service Mark property of
** CompuServe Incorporated.
*/

#define MAXCOLORS 256

/*
 * Pointer to function returning an int
 */
typedef int (*ifunptr) (int, int);

/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
typedef int code_int;

#ifdef SIGNED_COMPARE_SLOW
typedef unsigned long int count_int;
typedef unsigned short int count_short;
#else /*SIGNED_COMPARE_SLOW */
typedef long int count_int;
#endif /*SIGNED_COMPARE_SLOW */



static int find_unused_ia_colour (guchar *pixels,
				  int numpixels);

void special_flatten_indexed_alpha (guchar *pixels,
				    int *transparent,
				    int *colors,
				    int numpixels);
static int colorstobpp (int);
static int GetPixel (int, int);
static void BumpPixel (void);
static int GIFNextPixel (ifunptr);

static void GIFEncodeHeader (FILE *, gboolean, int, int, int, int, int, int,
			     int *, int *, int *, ifunptr);
static void GIFEncodeGraphicControlExt (FILE *, int, int, int, int, int, int,
					int, int, int, int *, int *, int *,
					ifunptr);
static void GIFEncodeImageData (FILE *, int, int, int, int, int, int,
				int *, int *, int *, ifunptr, gint, gint);
static void GIFEncodeClose (FILE *, int, int, int, int, int, int,
			    int *, int *, int *, ifunptr);
static void GIFEncodeLoopExt (FILE *, int, int, int, int, int, int,
			      int *, int *, int *, ifunptr, guint);
static void GIFEncodeCommentExt (FILE *, char *);

int rowstride;
guchar *pixels;
int cur_progress;
int max_progress;

static void Putword (int, FILE *);
static void compress (int, FILE *, ifunptr);
static void output (code_int);
static void cl_block (void);
static void cl_hash (count_int);
static void writeerr (void);
static void char_init (void);
static void char_out (int);
static void flush_char (void);



static int find_unused_ia_colour (guchar *pixels,
				  int numpixels)
{
  int i;
  gboolean ix_used[256];

  /*
  g_message ("GIF: Image has >=256 colors - attempting to reduce...\n");
  */

  for (i=0;i<256;i++)
    ix_used[i] = (gboolean)FALSE;

  for (i=0;i<numpixels;i++)
    if (pixels[i*2+1]) ix_used[pixels[i*2]] = (gboolean)TRUE;
  
  for (i=0;i<256;i++)
    if (ix_used[i] == (gboolean)FALSE)
      {

	g_message ("GIF: Found unused colour index %d.\n",(int)i);

	return i;
      }

  g_message ("GIF: Couldn't simply reduce colours further.  Saving as opaque.\n");
  return (-1);
}


void
special_flatten_indexed_alpha (guchar *pixels,
			       int *transparent,
			       int *colors,
			       int numpixels)
{
  guint32 i;

  /* Each transparent pixel in the image is mapped to a uniform value for
     encoding, if image already has <=255 colours */
  
  if ((*transparent) == -1) /* tough, no indices left for the trans. index */
    {
      for (i=0; i<numpixels; i++)
	pixels[i] = pixels[i*2];
    }
  else  /* make transparent */
    {
      for (i=0; i<numpixels; i++)
	{
	  if (!(pixels[i*2+1] & 128))
	    {
	      pixels[i] = (guchar)(*transparent);
	    }
	  else
	    {
	      pixels[i] = pixels[i*2];
	    }
	}
      if ((*colors) < 256)
	(*colors) += 1;
    }


  /* Pixel data now takes half as much space (the alpha data has been
     discarded) */
	  /*  pixels = g_realloc (pixels, numpixels);*/
}


int
parse_ms_tag (char *str)
{
  gint sum = 0;
  gint offset = 0;
  gint length;

  length = strlen(str);

find_another_bra:

  while ((offset<length) && (str[offset]!='('))
    offset++;
  
  if (offset>=length)
    return(-1);

  if (!isdigit(str[++offset]))
    goto find_another_bra;

  do
    {
      sum *= 10;
      sum += str[offset] - '0';
      offset++;
    }
  while ((offset<length) && (isdigit(str[offset])));  

  if (length-offset <= 2)
    return(-3);

  if ((toupper(str[offset]) != 'M') || (toupper(str[offset+1]) != 'S'))
    return(-4);

  return (sum);
}


int
parse_disposal_tag (char *str)
{
  gint offset = 0;
  gint length;

  length = strlen(str);

  while ((offset+9)<=length)
    {
      if (strncmp(&str[offset],"(combine)",9)==0) 
	return(0x01);
      if (strncmp(&str[offset],"(replace)",9)==0) 
	return(0x02);
      offset++;
    }

  return (gsvals.default_dispose);
}


static gboolean
boundscheck (gint32 image_ID)
{
  GDrawable *drawable;
  gint32 *layers;   
  gint nlayers;
  gint nreturn_vals;
  gint i;
  gint offset_x, offset_y;

  /* get a list of layers for this image_ID */
  layers = gimp_image_get_layers (image_ID, &nlayers);  


  /*** Iterate through the layers to make sure they're all ***/
  /*** within the bounds of the image                      ***/

  for (i=0;i<nlayers;i++)
    {
      drawable = gimp_drawable_get (layers[i]);
      gimp_drawable_offsets (layers[i], &offset_x, &offset_y);

      if ((offset_x < 0) ||
	  (offset_y < 0) ||
	  (offset_x+drawable->width > gimp_image_width(image_ID)) ||
	  (offset_y+drawable->height > gimp_image_height(image_ID)))
	{
	  g_free (layers);
	  gimp_drawable_detach(drawable);

	  /* Image has illegal bounds - ask the user what it wants to do */

	  if (badbounds_dialog())
	    {
	      gimp_run_procedure("gimp_crop", &nreturn_vals,
				 PARAM_IMAGE, image_ID,
				 PARAM_INT32, gimp_image_width(image_ID),
				 PARAM_INT32, gimp_image_height(image_ID),
				 PARAM_INT32, 0,
				 PARAM_INT32, 0,
				 PARAM_END);
	      return TRUE;
	    }
	  else
	    {
	      return FALSE;
	    }
	}
      else
	gimp_drawable_detach(drawable);
    }

  g_free (layers);

  return TRUE;
}


static gint
save_image (char   *filename,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  FILE *outfile;
  int Red[MAXCOLORS];
  int Green[MAXCOLORS];
  int Blue[MAXCOLORS];
  guchar *cmap;
  guint rows, cols;
  int BitsPerPixel;
  int colors;
  char *temp_buf;
  int i;
  int transparent;
  gint offset_x, offset_y;

  gint32 *layers;   
  int nlayers;

  gboolean is_gif89 = FALSE;

  int Delay89;
  int Disposal;
  char *layer_name;

  unsigned char bgred, bggreen, bgblue;



  /* get a list of layers for this image_ID */
  layers = gimp_image_get_layers (image_ID, &nlayers);  


  drawable_type = gimp_drawable_type (layers[0]);


  /* If the image has multiple layers (i.e. will be animated), a comment,
     or transparency, then it must be encoded as a GIF89a file, not a vanilla
     GIF87a. */
  if (nlayers>1)
    is_gif89 = TRUE;
  if (globalusecomment)
    is_gif89 = TRUE;

  switch (drawable_type)
    {
    case INDEXEDA_IMAGE:
      is_gif89 = TRUE;
    case INDEXED_IMAGE:
      cmap = gimp_image_get_cmap (image_ID, &colors);
      
      gimp_palette_get_background(&bgred, &bggreen, &bgblue);

      for (i = 0; i < colors; i++)
	{
	  Red[i] = *cmap++;
	  Green[i] = *cmap++;
	  Blue[i] = *cmap++;
	}
      for ( ; i < 256; i++)
	{
	  Red[i] = bgred;
	  Green[i] = bggreen;
	  Blue[i] = bgblue;
	}
      break;
    case GRAYA_IMAGE:
      is_gif89 = TRUE;
    case GRAY_IMAGE:
      colors = 256;                   /* FIXME: Not ideal. */
      for ( i = 0;  i < 256; i++)
	{
	  Red[i] = Green[i] = Blue[i] = i;
	}
      break;

    default:
      g_message ("GIF: Sorry, can't save RGB images as GIFs - convert to INDEXED\nor GRAY first.\n");
      return FALSE;
      break;
    }


  /* init the progress meter */    
  temp_buf = g_malloc (strlen (filename) + 11);
  sprintf (temp_buf, "Saving %s:", filename);
  gimp_progress_init (temp_buf);
  g_free (temp_buf);
  

  /* open the destination file for writing */
  outfile = fopen (filename, "wb");
  if (!outfile)
    {
      g_message ("GIF: can't open %s\n", filename);
      return FALSE;
    }


  /* write the GIFheader */

  if (colors < 256)
    {
      BitsPerPixel = colorstobpp (colors +
				  ((drawable_type==INDEXEDA_IMAGE) ? 1 : 0));
    }
  else
    {
      BitsPerPixel = colorstobpp (256);
      g_print ("GIF: Too many colours?\n");
    }

  cols = gimp_image_width(image_ID);
  rows = gimp_image_height(image_ID);
  GIFEncodeHeader (outfile, is_gif89, cols, rows, gsvals.interlace, 0,
		   transparent, BitsPerPixel, Red, Green, Blue, GetPixel);


  /* If the image has multiple layers it'll be made into an
     animated GIF, so write out the infinite-looping extension */
  if ((nlayers > 1) && (gsvals.loop))
    GIFEncodeLoopExt (outfile, cols, rows, gsvals.interlace, 0, transparent,
		      BitsPerPixel, Red, Green, Blue, GetPixel, 0);

  /* Write comment extension - mustn't be written before the looping ext. */
  if (globalusecomment)
    {
      GIFEncodeCommentExt (outfile, globalcomment);
    }



  /*** Now for each layer in the image, save an image in a compound GIF ***/
  /************************************************************************/

  i = nlayers-1;

  while (i >= 0)
    {
      drawable_type = gimp_drawable_type (layers[i]);
      drawable = gimp_drawable_get (layers[i]);
      gimp_drawable_offsets (layers[i], &offset_x, &offset_y);
      cols = drawable->width;
      rows = drawable->height;
      rowstride = drawable->width;
      
      gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
			   drawable->width, drawable->height, FALSE, FALSE);

      cur_progress = 0;
      max_progress = drawable->height;
      
      pixels = (guchar *) g_malloc (drawable->width *
				    drawable->height *
				    (((drawable_type == INDEXEDA_IMAGE)||
				      (drawable_type == GRAYA_IMAGE)) ? 2:1) );
      
      gimp_pixel_rgn_get_rect (&pixel_rgn, pixels, 0, 0,
			       drawable->width, drawable->height);


      /* sort out whether we need to do transparency jiggery-pokery */
      if ((drawable_type == INDEXEDA_IMAGE)||(drawable_type == GRAYA_IMAGE))
	{
	  /* Try to find an entry which isn't actually used in the
	     image, for a transparency index. */
	  
	  transparent =
	    find_unused_ia_colour(pixels,
				  drawable->width * drawable->height);

	  special_flatten_indexed_alpha (pixels,
					 &transparent,
					 &colors,
					 drawable->width * drawable->height);
	}
      else
	transparent = -1;
      
      
      BitsPerPixel = colorstobpp (colors);

      
      if (is_gif89)
	{
	  if (i>0)
	    {
	      layer_name = gimp_layer_get_name(layers[i-1]);
	      Disposal = parse_disposal_tag(layer_name);
	      g_free(layer_name);
	    }
	  else
	    Disposal = gsvals.default_dispose;

	  layer_name = gimp_layer_get_name(layers[i]);
	  Delay89 = parse_ms_tag(layer_name);
	  g_free(layer_name);

	  if (Delay89 < 0)
	    Delay89 = gsvals.default_delay/10;
	  else
	    Delay89 /= 10;

	  GIFEncodeGraphicControlExt (outfile, Disposal, Delay89, nlayers,
				      cols, rows, gsvals.interlace, 0,
				      transparent, BitsPerPixel, Red, Green,
				      Blue, GetPixel);
	}

      GIFEncodeImageData (outfile, cols, rows, gsvals.interlace, 0,
			  transparent,
			  BitsPerPixel, Red, Green, Blue, GetPixel,
			  offset_x, offset_y);

      
      
      gimp_drawable_detach (drawable);
      
      g_free (pixels);

      i--;
    }
  
  g_free(layers);

  GIFEncodeClose (outfile, cols, rows, gsvals.interlace, 0, transparent,
		  BitsPerPixel, Red, Green, Blue, GetPixel);

  return TRUE;
}


static gboolean
badbounds_dialog ( void )
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *vbox;


  can_crop = FALSE;


  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "GIF Warning");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) cropclose_callback,
		      dlg);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      (GtkSignalFunc) cropclose_callback,
		      dlg);

  /*  Action area  */
  button = gtk_button_new_with_label ("Crop");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) cropok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) cropcancel_callback,
                      dlg);
  /*  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
			     */
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);


  /*  the warning message  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  
  label= gtk_label_new(
		       "The image which you are trying to save as a GIF\n"
		       "contains layers which extend beyond the actual\n"
		       "borders of the image.  This isn't allowed in GIFs,\n"
		       "I'm afraid.\n\n"
		       "You may choose whether to crop all of the layers to\n"
		       "the image borders, or cancel this save."
		       );
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_widget_show(label);

  gtk_widget_show(vbox);

  gtk_widget_show(frame);

  gtk_widget_show(dlg);
  

  gtk_main ();

  gtk_widget_destroy (GTK_WIDGET(dlg));

  gdk_flush ();

  return can_crop;
}


static gint
save_dialog ( gint32 image_ID )
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *innerframe;
  GtkWidget *innervbox;
  GSList *group = NULL;

  gchar buffer[10];
  gint32 nlayers;


  gimp_image_get_layers (image_ID, &nlayers);


  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Save as GIF");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) save_close_callback,
		      NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      (GtkSignalFunc) save_windelete_callback,
		      dlg);


  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) save_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  /*  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg)); */
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) save_cancel_callback,
                      dlg);
			     
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);


  /*  regular gif parameter settings  */
  frame = gtk_frame_new ("GIF Options");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label ("Interlace");
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &gsvals.interlace);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), gsvals.interlace);
  gtk_widget_show (toggle);

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  toggle = gtk_check_button_new_with_label ("GIF Comment: ");
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &globalusecomment);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), 1);
  gtk_widget_show (toggle);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize (entry, 240, 0);
  if (globalcomment!=NULL) g_free(globalcomment);
  globalcomment = g_malloc(1+strlen(DEFAULT_COMMENT));
  strcpy(globalcomment, DEFAULT_COMMENT);
  gtk_entry_set_text (GTK_ENTRY (entry), globalcomment);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      (GtkSignalFunc) comment_entry_callback,
                      NULL);
  gtk_widget_show (entry);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);


  /*  additional animated gif parameter settings  */
  frame = gtk_frame_new ("Animated GIF Options");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label ("Loop");
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) save_toggle_update,
		      &gsvals.loop);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), gsvals.loop);
  gtk_widget_show (toggle);


  /* default_delay entry field */
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  label = gtk_label_new ("Default delay between frames where unspecified: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_set_usize (entry, 80, 0);
  sprintf (buffer, "%d", gsvals.default_delay);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      (GtkSignalFunc) save_entry_callback,
                      NULL);
  gtk_widget_show (entry);

  label = gtk_label_new (" milliseconds");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox);


  /* Disposal selector */
  innerframe = gtk_frame_new ("Default disposal where unspecified");
  gtk_frame_set_shadow_type (GTK_FRAME (innerframe), GTK_SHADOW_IN);
  gtk_container_border_width (GTK_CONTAINER (innerframe), 10);
  gtk_box_pack_start (GTK_BOX (vbox), innerframe, TRUE, TRUE, 0);
  innervbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (innervbox), 5);
  gtk_container_add (GTK_CONTAINER (innerframe), innervbox);

  toggle = gtk_radio_button_new_with_label (group,"Don't care");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (innervbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) save_toggle_update,
                      &radio_pressed[0]);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), radio_pressed[0]);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group,"One frame per layer (replace)");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (innervbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) save_toggle_update,
                      &radio_pressed[2]);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), radio_pressed[2]);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group,"Make frame from cumulative layers (combine)");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (innervbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) save_toggle_update,
                      &radio_pressed[1]);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), radio_pressed[1]);
  gtk_widget_show (toggle);
  
  gtk_widget_show (innervbox);
  gtk_widget_show (innerframe);



  gtk_widget_show (vbox);

  /* If the image has only one layer it can't be animated, so
     desensitize the animation options. */

  if (nlayers == 1) gtk_widget_set_sensitive (frame, FALSE);

  gtk_widget_show (frame);



  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return gsint.run;
}


static int
colorstobpp (int colors)
{
  int bpp;

  if (colors <= 2)
    bpp = 1;
  else if (colors <= 4)
    bpp = 2;
  else if (colors <= 8)
    bpp = 3;
  else if (colors <= 16)
    bpp = 4;
  else if (colors <= 32)
    bpp = 5;
  else if (colors <= 64)
    bpp = 6;
  else if (colors <= 128)
    bpp = 7;
  else if (colors <= 256)
    bpp = 8;
  else
    {
      g_message ("GIF: too many colors: %d\n", colors);
      return 8;
    }

  return bpp;
}


static int
GetPixel (int x,
	  int y)
{
  return *(pixels + (rowstride * (long) y) + (long) x);
}


/*****************************************************************************
 *
 * GIFENCODE.C    - GIF Image compression interface
 *
 * GIFEncode( FName, GHeight, GWidth, GInterlace, Background, Transparent,
 *            BitsPerPixel, Red, Green, Blue, GetPixel )
 *
 *****************************************************************************/

static int Width, Height;
static int curx, cury;
static long CountDown;
static int Pass = 0;
static int Interlace;

/*
 * Bump the 'curx' and 'cury' to point to the next pixel
 */
static void
BumpPixel ()
{
  /*
   * Bump the current X position
   */
  ++curx;

  /*
   * If we are at the end of a scan line, set curx back to the beginning
   * If we are interlaced, bump the cury to the appropriate spot,
   * otherwise, just increment it.
   */
  if (curx == Width)
    {
      cur_progress++;
      if ((cur_progress % 16) == 0)
	gimp_progress_update ((double) cur_progress / (double) max_progress);

      curx = 0;

      if (!Interlace)
	++cury;
      else
	{
	  switch (Pass)
	    {

	    case 0:
	      cury += 8;
	      if (cury >= Height)
		{
		  ++Pass;
		  cury = 4;
		}
	      break;

	    case 1:
	      cury += 8;
	      if (cury >= Height)
		{
		  ++Pass;
		  cury = 2;
		}
	      break;

	    case 2:
	      cury += 4;
	      if (cury >= Height)
		{
		  ++Pass;
		  cury = 1;
		}
	      break;

	    case 3:
	      cury += 2;
	      break;
	    }
	}
    }
}

/*
 * Return the next pixel from the image
 */
static int
GIFNextPixel (ifunptr getpixel)
{
  int r;

  if (CountDown == 0)
    return EOF;

  --CountDown;

  r = (*getpixel) (curx, cury);

  BumpPixel ();

  return r;
}

/* public */

static void
GIFEncodeHeader (FILE    *fp,
		 gboolean gif89,
		 int      GWidth,
		 int      GHeight,
		 int      GInterlace,
		 int      Background,
		 int      Transparent,
		 int      BitsPerPixel,
		 int      Red[],
		 int      Green[],
		 int      Blue[],
		 ifunptr  GetPixel)
{
  int B;
  int RWidth, RHeight;
  int LeftOfs, TopOfs;
  int Resolution;
  int ColorMapSize;
  int InitCodeSize;
  int i;

  Interlace = GInterlace;

  ColorMapSize = 1 << BitsPerPixel;

  RWidth = Width = GWidth;
  RHeight = Height = GHeight;
  LeftOfs = TopOfs = 0;

  Resolution = BitsPerPixel;

  /*
   * Calculate number of bits we are expecting
   */
  CountDown = (long) Width *(long) Height;

  /*
   * Indicate which pass we are on (if interlace)
   */
  Pass = 0;

  /*
   * The initial code size
   */
  if (BitsPerPixel <= 1)
    InitCodeSize = 2;
  else
    InitCodeSize = BitsPerPixel;

  /*
   * Set up the current x and y position
   */
  curx = cury = 0;

  /*
   * Write the Magic header
   */
  fwrite (gif89 ? "GIF89a" : "GIF87a", 1, 6, fp);

  /*
   * Write out the screen width and height
   */
  Putword (RWidth, fp);
  Putword (RHeight, fp);

  /*
   * Indicate that there is a global colour map
   */
  B = 0x80;			/* Yes, there is a color map */

  /*
   * OR in the resolution
   */
  B |= (Resolution - 1) << 5;

  /*
   * OR in the Bits per Pixel
   */
  B |= (BitsPerPixel - 1);

  /*
   * Write it out
   */
  fputc (B, fp);

  /*
   * Write out the Background colour
   */
  fputc (Background, fp);

  /*
   * Byte of 0's (future expansion)
   */
  fputc (0, fp);

  /*
   * Write out the Global Colour Map
   */
  for (i = 0; i < ColorMapSize; ++i)
    {
      fputc (Red[i], fp);
      fputc (Green[i], fp);
      fputc (Blue[i], fp);
    }
}


static void
GIFEncodeGraphicControlExt (FILE    *fp,
			    int      Disposal,
			    int      Delay89,
			    int      NumFramesInImage,
			    int      GWidth,
			    int      GHeight,
			    int      GInterlace,
			    int      Background,
			    int      Transparent,
			    int      BitsPerPixel,
			    int      Red[],
			    int      Green[],
			    int      Blue[],
			    ifunptr  GetPixel)
{
  int RWidth, RHeight;
  int LeftOfs, TopOfs;
  int Resolution;
  int ColorMapSize;
  int InitCodeSize;

  Interlace = GInterlace;

  ColorMapSize = 1 << BitsPerPixel;

  RWidth = Width = GWidth;
  RHeight = Height = GHeight;
  LeftOfs = TopOfs = 0;

  Resolution = BitsPerPixel;

  /*
   * Calculate number of bits we are expecting
   */
  CountDown = (long) Width *(long) Height;

  /*
   * Indicate which pass we are on (if interlace)
   */
  Pass = 0;

  /*
   * The initial code size
   */
  if (BitsPerPixel <= 1)
    InitCodeSize = 2;
  else
    InitCodeSize = BitsPerPixel;

  /*
   * Set up the current x and y position
   */
  curx = cury = 0;

  /*
   * Write out extension for transparent colour index, if necessary.
   */
  if ( (Transparent >= 0) || (NumFramesInImage > 1) )
    {
      /* Extension Introducer - fixed. */
      fputc ('!', fp);
      /* Graphic Control Label - fixed. */
      fputc (0xf9, fp);
      /* Block Size - fixed. */
      fputc (4, fp);
      
      /* Packed Fields - XXXdddut (d=disposal, u=userInput, t=transFlag) */
      /*                    s8421                                        */
      fputc ( ((Transparent >= 0) ? 0x01 : 0x00) /* TRANSPARENCY */

	      /* DISPOSAL */
	      | ((NumFramesInImage > 1) ? (Disposal << 2) : 0x00 ),
	      /* 0x03 or 0x01 build frames cumulatively */
	      /* 0x02 clears frame before drawing */
	      /* 0x00 'don't care' */

	      fp);
      
      fputc (Delay89 & 255, fp);
      fputc ((Delay89>>8) & 255, fp);

      fputc (Transparent, fp);
      fputc (0, fp);
    }
}


static void
GIFEncodeImageData (FILE    *fp,
		    int      GWidth,
		    int      GHeight,
		    int      GInterlace,
		    int      Background,
		    int      Transparent,
		    int      BitsPerPixel,
		    int      Red[],
		    int      Green[],
		    int      Blue[],
		    ifunptr  GetPixel,
		    gint     offset_x,
		    gint     offset_y)
{
  int RWidth, RHeight;
  int LeftOfs, TopOfs;
  int Resolution;
  int ColorMapSize;
  int InitCodeSize;

  Interlace = GInterlace;

  ColorMapSize = 1 << BitsPerPixel;

  RWidth = Width = GWidth;
  RHeight = Height = GHeight;
  LeftOfs = (int) offset_x;
  TopOfs = (int) offset_y;

  Resolution = BitsPerPixel;

  /*
   * Calculate number of bits we are expecting
   */
  CountDown = (long) Width *(long) Height;

  /*
   * Indicate which pass we are on (if interlace)
   */
  Pass = 0;

  /*
   * The initial code size
   */
  if (BitsPerPixel <= 1)
    InitCodeSize = 2;
  else
    InitCodeSize = BitsPerPixel;

  /*
   * Set up the current x and y position
   */
  curx = cury = 0;

#if 0
  /*
   * Write an Image separator
   */
  fputc (',', fp);

  /*
   * Write the Image header
   */

  Putword (LeftOfs, fp);
  Putword (TopOfs, fp);
  Putword (Width, fp);
  Putword (Height, fp);

  /*
   * Write out whether or not the image is interlaced
   */
  if (Interlace)
    fputc (0x40, fp);
  else
    fputc (0x00, fp);

  /*
   * Write out the initial code size
   */
  fputc (InitCodeSize, fp);

  /*
   * Go and actually compress the data
   */
  compress (InitCodeSize + 1, fp, GetPixel);

  /*
   * Write out a Zero-length packet (to end the series)
   */
  fputc (0, fp);


  /***************************/
  Interlace = GInterlace;
  ColorMapSize = 1 << BitsPerPixel;
  RWidth = Width = GWidth;
  RHeight = Height = GHeight;
  LeftOfs = TopOfs = 0;
  Resolution = BitsPerPixel;

  CountDown = (long) Width *(long) Height;
  Pass = 0;
  /*
   * The initial code size
   */
  if (BitsPerPixel <= 1)
    InitCodeSize = 2;
  else
    InitCodeSize = BitsPerPixel;
  /*
   * Set up the current x and y position
   */
  curx = cury = 0;

#endif


  cur_progress = 0;


  /*
   * Write an Image separator
   */
  fputc (',', fp);

  /*
   * Write the Image header
   */

  Putword (LeftOfs, fp);
  Putword (TopOfs, fp);
  Putword (Width, fp);
  Putword (Height, fp);

  /*
   * Write out whether or not the image is interlaced
   */
  if (Interlace)
    fputc (0x40, fp);
  else
    fputc (0x00, fp);

  /*
   * Write out the initial code size
   */
  fputc (InitCodeSize, fp);

  /*
   * Go and actually compress the data
   */
  compress (InitCodeSize + 1, fp, GetPixel);

  /*
   * Write out a Zero-length packet (to end the series)
   */
  fputc (0, fp);
}


static void
GIFEncodeClose (FILE    *fp,
		int      GWidth,
		int      GHeight,
		int      GInterlace,
		int      Background,
		int      Transparent,
		int      BitsPerPixel,
		int      Red[],
		int      Green[],
		int      Blue[],
		ifunptr  GetPixel)
{
  /*
   * Write the GIF file terminator
   */
  fputc (';', fp);

  /*
   * And close the file
   */
  fclose (fp);
}


static void
GIFEncodeLoopExt (FILE    *fp,
		  int      GWidth,
		  int      GHeight,
		  int      GInterlace,
		  int      Background,
		  int      Transparent,
		  int      BitsPerPixel,
		  int      Red[],
		  int      Green[],
		  int      Blue[],
		  ifunptr  GetPixel,
		  guint    num_loops)
{
  fputc(0x21,fp);
  fputc(0xff,fp);
  fputc(0x0b,fp);
  fputs("NETSCAPE2.0",fp);
  fputc(0x03,fp);
  fputc(0x01,fp);
  Putword(num_loops,fp);
  fputc(0x00,fp);

  /* NOTE: num_loops==0 means 'loop infinitely' */
}


static void GIFEncodeCommentExt (FILE *fp, char *comment)
{
  if (comment==NULL||strlen(comment)<1)
    {
      g_print ("GIF: warning: no comment given - comment block not written.\n");
      return;
    }

  fputc(0x21,fp);
  fputc(0xfe,fp);
  fputc(strlen(comment),fp);
  fputs((const char *)comment,fp);
  fputc(0x00,fp);
}



/*
 * Write out a word to the GIF file
 */
static void
Putword (int   w,
	 FILE *fp)
{
  fputc (w & 0xff, fp);
  fputc ((w / 256) & 0xff, fp);
}


/***************************************************************************
 *
 *  GIFCOMPR.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu)
 *
 ***************************************************************************/

/*
 * General DEFINEs
 */

#define GIF_BITS    12

#define HSIZE  5003		/* 80% occupancy */

#ifdef NO_UCHAR
typedef char char_type;
#else /*NO_UCHAR */
typedef unsigned char char_type;
#endif /*NO_UCHAR */

/*

 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */
#include <ctype.h>

#define ARGVAL() (*++(*argv) || (--argc && *++argv))

static int n_bits;		/* number of bits/code */
static int maxbits = GIF_BITS;	/* user settable max # bits/code */
static code_int maxcode;	/* maximum code, given n_bits */
static code_int maxmaxcode = (code_int) 1 << GIF_BITS;	/* should NEVER generate this code */
#ifdef COMPATIBLE		/* But wrong! */
#define MAXCODE(Mn_bits)        ((code_int) 1 << (Mn_bits) - 1)
#else /*COMPATIBLE */
#define MAXCODE(Mn_bits)        (((code_int) 1 << (Mn_bits)) - 1)
#endif /*COMPATIBLE */

static count_int htab[HSIZE];
static unsigned short codetab[HSIZE];
#define HashTabOf(i)       htab[i]
#define CodeTabOf(i)    codetab[i]

const code_int hsize = HSIZE;	/* the original reason for this being
				   variable was "for dynamic table sizing",
				   but since it was never actually changed
				   I made it const   --Adam. */

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**GIF_BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i) CodeTabOf(i)
#define tab_suffixof(i)        ((char_type*)(htab))[i]
#define de_stack               ((char_type*)&tab_suffixof((code_int)1<<GIF_BITS))

static code_int free_ent = 0;	/* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int clear_flg = 0;

static int offset;
static long int in_count = 1;	/* length of input */
static long int out_count = 0;	/* # of codes output (for debugging) */

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

static int g_init_bits;
static FILE *g_outfile;

static int ClearCode;
static int EOFCode;


static unsigned long cur_accum;
static int cur_bits;

static unsigned long masks[] =
{0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
 0x001F, 0x003F, 0x007F, 0x00FF,
 0x01FF, 0x03FF, 0x07FF, 0x0FFF,
 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF};


static void
compress (int      init_bits,
	  FILE    *outfile,
	  ifunptr  ReadValue)
{
  register long fcode;
  register code_int i /* = 0 */ ;
  register int c;
  register code_int ent;
  register code_int disp;
  register code_int hsize_reg;
  register int hshift;


  /*
   * Set up the globals:  g_init_bits - initial number of bits
   *                      g_outfile   - pointer to output file
   */
  g_init_bits = init_bits;
  g_outfile = outfile;

  cur_bits = 0;
  cur_accum = 0;

  /*
   * Set up the necessary values
   */
  offset = 0;
  out_count = 0;
  clear_flg = 0;
  in_count = 1;

  ClearCode = (1 << (init_bits - 1));
  EOFCode = ClearCode + 1;
  free_ent = ClearCode + 2;


  /* Had some problems here... should be okay now.  --Adam */
  n_bits = g_init_bits;
  maxcode = MAXCODE (n_bits);



  char_init ();

  ent = GIFNextPixel (ReadValue);

  hshift = 0;
  for (fcode = (long) hsize; fcode < 65536L; fcode *= 2L)
    ++hshift;
  hshift = 8 - hshift;		/* set hash code range bound */

  hsize_reg = hsize;
  cl_hash ((count_int) hsize_reg);	/* clear hash table */

  output ((code_int) ClearCode);


#ifdef SIGNED_COMPARE_SLOW
  while ((c = GIFNextPixel (ReadValue)) != (unsigned) EOF)
    {
#else /*SIGNED_COMPARE_SLOW */
  while ((c = GIFNextPixel (ReadValue)) != EOF)
    {				/* } */
#endif /*SIGNED_COMPARE_SLOW */

      ++in_count;

      fcode = (long) (((long) c << maxbits) + ent);
      i = (((code_int) c << hshift) ^ ent);	/* xor hashing */

      if (HashTabOf (i) == fcode)
	{
	  ent = CodeTabOf (i);
	  continue;
	}
      else if ((long) HashTabOf (i) < 0)	/* empty slot */
	goto nomatch;
      disp = hsize_reg - i;	/* secondary hash (after G. Knott) */
      if (i == 0)
	disp = 1;
    probe:
      if ((i -= disp) < 0)
	i += hsize_reg;

      if (HashTabOf (i) == fcode)
	{
	  ent = CodeTabOf (i);
	  continue;
	}
      if ((long) HashTabOf (i) > 0)
	goto probe;
    nomatch:
      output ((code_int) ent);
      ++out_count;
      ent = c;
#ifdef SIGNED_COMPARE_SLOW
      if ((unsigned) free_ent < (unsigned) maxmaxcode)
	{
#else /*SIGNED_COMPARE_SLOW */
      if (free_ent < maxmaxcode)
	{			/* } */
#endif /*SIGNED_COMPARE_SLOW */
	  CodeTabOf (i) = free_ent++;	/* code -> hashtable */
	  HashTabOf (i) = fcode;
	}
      else
	cl_block ();
    }
  /*
   * Put out the final code.
   */
  output ((code_int) ent);
  ++out_count;
  output ((code_int) EOFCode);
}

/*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (long)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a GIF_BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static void
output (code_int code)
{
  cur_accum &= masks[cur_bits];

  if (cur_bits > 0)
    cur_accum |= ((long) code << cur_bits);
  else
    cur_accum = code;

  cur_bits += n_bits;

  while (cur_bits >= 8)
    {
      char_out ((unsigned int) (cur_accum & 0xff));
      cur_accum >>= 8;
      cur_bits -= 8;
    }

  /*
   * If the next entry is going to be too big for the code size,
   * then increase it, if possible.
   */
  if (free_ent > maxcode || clear_flg)
    {

      if (clear_flg)
	{

	  maxcode = MAXCODE (n_bits = g_init_bits);
	  clear_flg = 0;

	}
      else
	{

	  ++n_bits;
	  if (n_bits == maxbits)
	    maxcode = maxmaxcode;
	  else
	    maxcode = MAXCODE (n_bits);
	}
    }

  if (code == EOFCode)
    {
      /*
       * At EOF, write the rest of the buffer.
       */
      while (cur_bits > 0)
	{
	  char_out ((unsigned int) (cur_accum & 0xff));
	  cur_accum >>= 8;
	  cur_bits -= 8;
	}

      flush_char ();

      fflush (g_outfile);

      if (ferror (g_outfile))
	writeerr ();
    }
}

/*
 * Clear out the hash table
 */
static void
cl_block ()			/* table clear for block compress */
{

  cl_hash ((count_int) hsize);
  free_ent = ClearCode + 2;
  clear_flg = 1;

  output ((code_int) ClearCode);
}

static void
cl_hash (register count_int hsize)			/* reset code table */
{

  register count_int *htab_p = htab + hsize;

  register long i;
  register long m1 = -1;

  i = hsize - 16;
  do
    {				/* might use Sys V memset(3) here */
      *(htab_p - 16) = m1;
      *(htab_p - 15) = m1;
      *(htab_p - 14) = m1;
      *(htab_p - 13) = m1;
      *(htab_p - 12) = m1;
      *(htab_p - 11) = m1;
      *(htab_p - 10) = m1;
      *(htab_p - 9) = m1;
      *(htab_p - 8) = m1;
      *(htab_p - 7) = m1;
      *(htab_p - 6) = m1;
      *(htab_p - 5) = m1;
      *(htab_p - 4) = m1;
      *(htab_p - 3) = m1;
      *(htab_p - 2) = m1;
      *(htab_p - 1) = m1;
      htab_p -= 16;
    }
  while ((i -= 16) >= 0);

  for (i += 16; i > 0; --i)
    *--htab_p = m1;
}

static void
writeerr ()
{
  g_message ("GIF: error writing output file\n");
  return;
}

/******************************************************************************
 *
 * GIF Specific routines
 *
 ******************************************************************************/

/*
 * Number of characters so far in this 'packet'
 */
static int a_count;

/*
 * Set up the 'byte output' routine
 */
static void
char_init ()
{
  a_count = 0;
}

/*
 * Define the storage for the packet accumulator
 */
static char accum[256];

/*
 * Add a character to the end of the current packet, and if it is 254
 * characters, flush the packet to disk.
 */
static void
char_out (int c)
{
  accum[a_count++] = c;
  if (a_count >= 254)
    flush_char ();
}

/*
 * Flush the packet to disk, and reset the accumulator
 */
static void
flush_char ()
{
  if (a_count > 0)
    {
      fputc (a_count, g_outfile);
      fwrite (accum, 1, a_count, g_outfile);
      a_count = 0;
    }
}



/* crop dialog functions */

static void
cropok_callback (GtkWidget *widget,
		 gpointer   data)
{
  can_crop = TRUE;
  gtk_main_quit ();
}

static void
cropcancel_callback (GtkWidget *widget,
		     gpointer   data)
{
  can_crop = FALSE;
  gtk_main_quit ();
}

static void
cropclose_callback (GtkWidget *widget,
		     gpointer   data)
{
  gtk_main_quit ();
}


/*  Save interface functions  */

static void
save_close_callback (GtkWidget *widget,
		     gpointer   data)
{
  gtk_main_quit ();
}

static void
save_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  gsint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
save_cancel_callback (GtkWidget *widget,
		      gpointer   data)
{
  gsint.run = FALSE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
save_windelete_callback (GtkWidget *widget,
			 gpointer   data)
{
  gsint.run = FALSE;
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

static void
save_entry_callback (GtkWidget *widget,
		     gpointer   data)
{
  gsvals.default_delay = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));

  if (gsvals.default_delay < 0)
    gsvals.default_delay = 0;

  if (gsvals.default_delay > 65000)
    gsvals.default_delay = 65000;
}

static void
comment_entry_callback (GtkWidget *widget,
			gpointer   data)
{
  gint ssize;

  ssize = strlen(gtk_entry_get_text (GTK_ENTRY (widget)));

  /* Temporary kludge for overlength strings - just return */
  if (ssize>240)
    {
      g_message ("GIF save: Your comment string is too long.\n");
      return;
    }

  if (globalcomment!=NULL) g_free(globalcomment);
  globalcomment = g_malloc(ssize+1);

  strcpy(globalcomment, gtk_entry_get_text (GTK_ENTRY (widget)));

  /* g_print ("COMMENT: %s\n",globalcomment); */
}

/* The End */
