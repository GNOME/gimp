/* GIF loading file filter for The GIMP version 1.0/1.1
 *
 *    - Adam D. Moss
 *    - Peter Mattis
 *    - Spencer Kimball
 *
 *      Based around original GIF code by David Koblas.
 *
 *
 * Version 1.0.0 - 99/03/20
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
 * 99/03/20
 * 1.00.00 - GIF load-only code split from main GIF plugin.
 *
 * For previous revision information, please consult the comments
 * in the 'gif' plugin.
 */

/*
 * TODO (more *'s means more important!)
 *
 * - PDB stuff for comments
 *
 * - Remove unused colourmap entries for GRAYSCALE images.
 */

/* Copyright notice for code which this plugin was long ago derived from */
/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993, David Koblas.  (koblas@netcom.com)    | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */


#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/stdplugins-intl.h"


/* uncomment the line below for a little debugging info */
/* #define GIFDEBUG yesplease */


/* Does the version of GIMP we're compiling for support
   data attachments to images?  ('Parasites') */
#ifdef _PARASITE_H_
#define FACEHUGGERS aieee
#endif
/* PS: I know that technically facehuggers aren't parasites,
   the pupal-forms are.  But facehuggers are ky00te. */




/* Declare some local functions.
 */
static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
static gint32 load_image (char   *filename);




static guchar   used_cmap[3][256];
static GRunModeType run_mode;
static guchar   highest_used_index;
static gboolean promote_to_rgb   = FALSE;
static guchar   gimp_cmap[768];
#ifdef FACEHUGGERS
Parasite*      comment_parasite = NULL;
#endif




GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
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

  INIT_I18N();

  gimp_install_procedure ("file_gif_load",
                          _("loads files of Compuserve GIF file format"),
                          "FIXME: write help for gif_load",
                          "Spencer Kimball, Peter Mattis, Adam Moss, David Koblas",
                          "Spencer Kimball, Peter Mattis, Adam Moss, David Koblas",
                          "1995-1997",
                          "<Load>/GIF",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_register_magic_load_handler ("file_gif_load", "gif", "", "0,string,GIF8");
}


static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  gint32 image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 2;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_gif_load") == 0)
    {
      INIT_I18N_UI();

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
#ifdef GIFDEBUG
      g_print ("GIF: Highest used index is %d\n", highest_used_index);
#endif
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
      g_message (_("GIF: can't open \"%s\"\n"), filename);
      return -1;
    }

  name_buf = g_malloc (strlen (filename) + 11);

  if (run_mode != RUN_NONINTERACTIVE)
    {
      sprintf (name_buf, _("Loading %s:"), filename);
      gimp_progress_init (name_buf);
      g_free (name_buf);
    }

  if (!ReadOK (fd, buf, 6))
    {
      g_message (_("GIF: error reading magic number\n"));
      return -1;
    }

  if (strncmp ((char *) buf, "GIF", 3) != 0)
    {
      g_message (_("GIF: not a GIF file\n"));
      return -1;
    }

  strncpy (version, (char *) buf + 3, 3);
  version[3] = '\0';

  if ((strcmp (version, "87a") != 0) && (strcmp (version, "89a") != 0))
    {
      g_message (_("GIF: bad version number, not '87a' or '89a'\n"));
      return -1;
    }

  if (!ReadOK (fd, buf, 7))
    {
      g_message (_("GIF: failed to read screen descriptor\n"));
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
	  g_message (_("GIF: error reading global colormap\n"));
	  return -1;
	}
    }

  if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49)
    {
      g_message (_("GIF: warning - non-square pixels\n"));
    }


  highest_used_index = 0;
      

  for (;;)
    {
      if (!ReadOK (fd, &c, 1))
	{
	  g_message (_("GIF: EOF / read error on image data\n"));
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
	      g_message (_("GIF: EOF / read error on extension function code\n"));
	      return image_ID; /* will be -1 if failed on first image! */
	    }
	  DoExtension (fd, c);
	  continue;
	}

      if (c != ',')
	{
	  /* Not a valid start character */
	  g_message (_("GIF: bogus character 0x%02x, ignoring\n"), (int) c);
	  continue;
	}

      ++imageCount;

      if (!ReadOK (fd, buf, 9))
	{
	  g_message (_("GIF: couldn't read left/top/width/height\n"));
	  return image_ID; /* will be -1 if failed on first image! */
	}

      useGlobalColormap = !BitSet (buf[8], LOCALCOLORMAP);

      bitPixel = 1 << ((buf[8] & 0x07) + 1);

      if (!useGlobalColormap)
	{
	  if (ReadColorMap (fd, bitPixel, localColorMap, &grayScale))
	    {
	      g_message (_("GIF: error reading local colormap\n"));
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

#ifdef FACEHUGGERS
      if (comment_parasite != NULL)
	{
	  gimp_image_attach_parasite (image_ID, comment_parasite);
	  parasite_free (comment_parasite);
	  comment_parasite = NULL;
	}
#endif

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
	  g_message (_("GIF: bad colormap\n"));
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
#ifdef FACEHUGGERS
	  if (comment_parasite != NULL)
	    {
	      parasite_free (comment_parasite);
	    }
	    
	  comment_parasite = parasite_new ("gimp-comment",PARASITE_PERSISTENT,
					    strlen(buf)+1, (void*)buf);
#else
	  if (showComment)
	    g_print ("GIF: gif comment: %s\n", buf);
#endif
	}
      return TRUE;
      break;
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
      break;
    default:
      str = (char *)buf;
      sprintf ((char *)buf, "UNKNOWN (0x%02x)", label);
      break;
    }

#ifdef GIFDEBUG
  g_print ("GIF: got a '%s'\n", str);
#endif

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
      g_message (_("GIF: error in getting DataBlock size\n"));
      return -1;
    }

  ZeroDataBlock = count == 0;

  if ((count != 0) && (!ReadOK (fd, buf, count)))
    {
      g_message (_("GIF: error in reading DataBlock\n"));
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
	      g_message (_("GIF: ran off the end of by bits\n"));
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
	    g_print (_("GIF: missing EOD in data stream (common occurence)"));
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
	      g_message (_("GIF: circular table entry BIG ERROR\n"));
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
      g_message (_("GIF: EOF / read error on image data\n"));
      return -1;
    }

  if (LWZReadByte (fd, TRUE, c) < 0)
    {
      g_message (_("GIF: error while reading\n"));
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
	strcpy(framename, _("Background"));
      else
	sprintf(framename, _("Background (%dms)"), 10*Gif89.delayTime);

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
#ifdef GIFDEBUG
		  g_print ("GIF: Promoting image to RGB...\n");
#endif
		  gimp_run_procedure("gimp_convert_rgb", &nreturn_vals,
				     PARAM_IMAGE, image_ID,
				     PARAM_END);
		  break;
		}
	    }
	}

      if (Gif89.delayTime < 0)
	sprintf(framename, _("Frame %d"), frame_number);
      else
	sprintf(framename, _("Frame %d (%dms)"),
		frame_number, 10*Gif89.delayTime);

      switch (previous_disposal)
	{
	case 0x00: break; /* 'don't care' */
	case 0x01: strcat(framename,_(" (combine)")); break;
	case 0x02: strcat(framename,_(" (replace)")); break;
	case 0x03: strcat(framename,_(" (combine)")); break;
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	  strcat(framename,_(" (unknown disposal)"));
	  g_message (_("GIF: Hmm... please forward this GIF to the "
		     "GIF plugin author!\n  (adam@foxbox.org)\n"));
	  break;
	default: g_message (_("GIF: Something got corrupted.\n")); break;
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
    g_print (_("GIF: reading %d by %d%s GIF image, ncols=%d\n"),
	     len, height, interlace ? _(" interlaced") : "", ncols);

  if (!alpha_frame && promote_to_rgb)
    {
      g_message (_("GIF: Ouchie!  Can't handle non-alpha RGB frames.\n     Please mail the plugin author.  (adam@gimp.org)\n"));
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

      xpos++;
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
		  pass++;
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
	      ypos++;
	    }

	  if (run_mode != RUN_NONINTERACTIVE)
	    {
	      cur_progress++;
	      if ((cur_progress % 16) == 0)
		gimp_progress_update ((double) cur_progress / (double) max_progress);
	    }
	}
      if (ypos >= height)
	break;
    }

fini:
  if (LWZReadByte (fd, FALSE, c) >= 0)
    g_print (_("GIF: too much input data, ignoring extra...\n"));

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



int rowstride;
guchar *pixels;
int cur_progress;
int max_progress;




/* public */



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

#ifdef COMPATIBLE		/* But wrong! */
#define MAXCODE(Mn_bits)        ((code_int) 1 << (Mn_bits) - 1)
#else /*COMPATIBLE */
#define MAXCODE(Mn_bits)        (((code_int) 1 << (Mn_bits)) - 1)
#endif /*COMPATIBLE */


const code_int hsize = HSIZE;	/* the original reason for this being
				   variable was "for dynamic table sizing",
				   but since it was never actually changed
				   I made it const   --Adam. */


/* The End */
