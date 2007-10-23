/* GIF loading file filter for GIMP 2.x
 * +-------------------------------------------------------------------+
 * |  Copyright Adam D. Moss, Peter Mattis, Spencer Kimball            |
 * +-------------------------------------------------------------------+
 * Version 1.50.4 - 2003/06/03
 *                        Adam D. Moss - <adam@gimp.org> <adam@foxbox.org>
 */

/* Copyright notice for old GIF code from which this plugin was long ago */
/* derived (David Koblas has kindly granted permission to relicense):    */
/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993, David Koblas.  (koblas@extra.com)     | */
/* +-------------------------------------------------------------------+ */
/* Also...
 * 'This filter uses code taken from the "giftopnm" and "ppmtogif" programs
 *    which are part of the "netpbm" package.'
 */
/* Additionally...
 *  "The Graphics Interchange Format(c) is the Copyright property of
 *  CompuServe Incorporated.  GIF(sm) is a Service Mark property of
 *  CompuServe Incorporated."
 */

/*
 * REVISION HISTORY
 *
 * 2003/06/03
 * 1.50.04 - When initializing the LZW state, watch out for a completely
 *     bogus input_code_size [based on fix by Raphael Quinet]
 *     Also, fix a stupid old bug when clearing the code table between
 *     subimages.  (Enables us to deal better with errors when the stream is
 *     corrupted pretty early in a subimage.) [adam]
 *     Minor-version-bump to distinguish between gimp1.2/1.4 branches.
 *
 * 2000/03/31
 * 1.00.03 - Just mildly more useful comments/messages concerning frame
 *     disposals.
 *
 * 1999/11/20
 * 1.00.02 - Fixed a couple of possible infinite loops where an
 *     error condition was not being checked.  Also changed some g_message()s
 *     back to g_warning()s as they should be (don't get carried away with
 *     the user feedback fellahs, no-one wants to be told of every single
 *     corrupt byte and block in its own little window.  :-( ).
 *
 * 1999/11/11
 * 1.00.01 - Fixed an uninitialized variable which has been around
 *     forever... thanks to jrb@redhat.com for noticing that there
 *     was a problem somewhere!
 *
 * 1999/03/20
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

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC        "file-gif-load"
#define LOAD_THUMB_PROC  "file-gif-load-thumb"


/* uncomment the line below for a little debugging info */
/* #define GIFDEBUG yesplease */


/* Declare some local functions.
 */
static void   query      (void);
static void   run        (const gchar      *name,
                          gint              nparams,
                          const GimpParam  *param,
                          gint             *nreturn_vals,
                          GimpParam       **return_vals);
static gint32 load_image (const gchar      *filename,
                          gboolean          thumbnail);


static guchar        used_cmap[3][256];
static guchar        highest_used_index;
static gboolean      promote_to_rgb   = FALSE;
static guchar        gimp_cmap[768];
static GimpParasite *comment_parasite = NULL;


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name entered"             }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",        "Output image"                 }
  };
  static const GimpParamDef thumb_args[] =
  {
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_INT32,  "thumb-size",   "Preferred thumbnail size"     }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads files of Compuserve GIF file format",
                          "FIXME: write help for gif_load",
                          "Spencer Kimball, Peter Mattis, Adam Moss, David Koblas",
                          "Spencer Kimball, Peter Mattis, Adam Moss, David Koblas",
                          "1995-2006",
                          N_("GIF image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/gif");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "gif",
                                    "",
                                    "0,string,GIF8");

  gimp_install_procedure (LOAD_THUMB_PROC,
                          "Loads only the first frame of a GIF image, to be "
                          "used as a thumbnail",
                          "",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          NULL,
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (thumb_args),
                          G_N_ELEMENTS (load_return_vals),
                          thumb_args, load_return_vals);

  gimp_register_thumbnail_loader (LOAD_PROC, LOAD_THUMB_PROC);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image_ID;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (param[1].data.d_string, FALSE);
    }
  else if (strcmp (name, LOAD_THUMB_PROC) == 0)
    {
      image_ID = load_image (param[0].data.d_string, TRUE);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (image_ID != -1)
        {
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
          if (! promote_to_rgb)
            gimp_image_set_colormap (image_ID,
                                     gimp_cmap, highest_used_index + 1);

          *nreturn_vals = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;
}


#define MAXCOLORMAPSIZE  256

#define CM_RED           0
#define CM_GREEN         1
#define CM_BLUE          2

#define MAX_LZW_BITS     12

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
  CMap         ColorMap;
  unsigned int BitPixel;
  unsigned int ColorResolution;
  unsigned int Background;
  unsigned int AspectRatio;
  /*
   **
   */
  int          GrayScale;
} GifScreen;

static struct
{
  int transparent;
  int delayTime;
  int inputFlag;
  int disposal;
} Gif89 = { -1, -1, -1, 0 };

static int ReadColorMap (FILE *, int, CMap, int *);
static int DoExtension  (FILE *, int);
static int GetDataBlock (FILE *, unsigned char *);
static int GetCode      (FILE *, int, int);
static int LZWReadByte  (FILE *, int, int);
static gint32 ReadImage (FILE *, const gchar *,
                         int, int, CMap, int, int, int, int,
                         guint, guint, guint, guint);


static gint32
load_image (const gchar *filename,
            gboolean     thumbnail)
{
  FILE     *fd;
  guchar    buf[16];
  guchar    c;
  CMap      localColorMap;
  gint      grayScale;
  gboolean  useGlobalColormap;
  gint      bitPixel;
  gint      imageCount = 0;
  gchar     version[4];
  gint32    image_ID = -1;

  fd = g_fopen (filename, "rb");
  if (!fd)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  if (!ReadOK (fd, buf, 6))
    {
      g_message ("Error reading magic number");
      return -1;
    }

  if (strncmp ((gchar *) buf, "GIF", 3) != 0)
    {
      g_message (_("This is not a GIF file"));
      return -1;
    }

  strncpy (version, (gchar *) buf + 3, 3);
  version[3] = '\0';

  if ((strcmp (version, "87a") != 0) && (strcmp (version, "89a") != 0))
    {
      g_message ("Bad version number, not '87a' or '89a'");
      return -1;
    }

  if (!ReadOK (fd, buf, 7))
    {
      g_message ("Failed to read screen descriptor");
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
      if (ReadColorMap (fd, GifScreen.BitPixel, GifScreen.ColorMap,
                        &GifScreen.GrayScale))
        {
          g_message ("Error reading global colormap");
          return -1;
        }
    }

  if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49)
    {
      g_message (_("Non-square pixels.  Image might look squashed."));
    }


  highest_used_index = 0;

  for (;;)
    {
      if (!ReadOK (fd, &c, 1))
        {
          g_message ("EOF / read error on image data");
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
              g_message ("EOF / read error on extension function code");
              return image_ID; /* will be -1 if failed on first image! */
            }
          DoExtension (fd, c);
          continue;
        }

      if (c != ',')
        {
          /* Not a valid start character */
          g_printerr ("GIF: bogus character 0x%02x, ignoring.\n", (int) c);
          continue;
        }

      ++imageCount;

      if (!ReadOK (fd, buf, 9))
        {
          g_message ("Couldn't read left/top/width/height");
          return image_ID; /* will be -1 if failed on first image! */
        }

      useGlobalColormap = !BitSet (buf[8], LOCALCOLORMAP);

      bitPixel = 1 << ((buf[8] & 0x07) + 1);

      if (! useGlobalColormap)
        {
          if (ReadColorMap (fd, bitPixel, localColorMap, &grayScale))
            {
              g_message ("Error reading local colormap");
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
                                GifScreen.Height);
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
                                GifScreen.Height);
        }

      if (comment_parasite != NULL)
        {
          if (! thumbnail)
            gimp_image_parasite_attach (image_ID, comment_parasite);

          gimp_parasite_free (comment_parasite);
          comment_parasite = NULL;
        }

      /* If we are loading a thumbnail, we stop after the first frame. */
      if (thumbnail)
        break;
    }

  return image_ID;
}

static int
ReadColorMap (FILE *fd,
              int   number,
              CMap  buffer,
              int  *format)
{
  guchar rgb[3];
  gint   flag;
  gint   i;

  flag = TRUE;

  for (i = 0; i < number; ++i)
    {
      if (!ReadOK (fd, rgb, sizeof (rgb)))
        {
          g_message ("Bad colormap");
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
  static guchar  buf[256];
  gchar         *str;

  switch (label)
    {
    case 0x01:                  /* Plain Text Extension */
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

      while (GetDataBlock (fd, (unsigned char *) buf) > 0)
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
    case 0xff:                  /* Application Extension */
      str = "Application Extension";
      break;
    case 0xfe:                  /* Comment Extension */
      str = "Comment Extension";
      while (GetDataBlock (fd, (unsigned char *) buf) > 0)
        {
          gchar *comment = (gchar *) buf;

          if (!g_utf8_validate (comment, -1, NULL))
            continue;

          if (comment_parasite)
            gimp_parasite_free (comment_parasite);

          comment_parasite = gimp_parasite_new ("gimp-comment",
                                                GIMP_PARASITE_PERSISTENT,
                                                strlen (comment) + 1, comment);
        }
      return TRUE;
      break;
    case 0xf9:                  /* Graphic Control Extension */
      str = "Graphic Control Extension";
      (void) GetDataBlock (fd, (unsigned char *) buf);
      Gif89.disposal = (buf[0] >> 2) & 0x7;
      Gif89.inputFlag = (buf[0] >> 1) & 0x1;
      Gif89.delayTime = LM_to_uint (buf[1], buf[2]);
      if ((buf[0] & 0x1) != 0)
        Gif89.transparent = buf[3];
      else
        Gif89.transparent = -1;

      while (GetDataBlock (fd, (unsigned char *) buf) > 0)
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

  while (GetDataBlock (fd, (unsigned char *) buf) > 0)
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
      g_message ("Error in getting DataBlock size");
      return -1;
    }

  ZeroDataBlock = count == 0;

  if ((count != 0) && (!ReadOK (fd, buf, count)))
    {
      g_message ("Error in reading DataBlock");
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
  int i, j, ret, count;

  if (flag)
    {
      curbit = 0;
      lastbit = 0;
      done = FALSE;
      last_byte = 2;
      return 0;
    }

  if ((curbit + code_size) >= lastbit)
    {
      if (done)
        {
          if (curbit >= lastbit)
            {
              g_message ("Ran off the end of my bits");
              gimp_quit ();
            }
          return -1;
        }

      buf[0] = buf[last_byte - 2];
      buf[1] = buf[last_byte - 1];

      if ((count = GetDataBlock (fd, &buf[2])) <= 0)
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
LZWReadByte (FILE *fd,
             int   just_reset_LZW,
             int   input_code_size)
{
  static int fresh = FALSE;
  int code, incode;
  static int code_size, set_code_size;
  static int max_code, max_code_size;
  static int firstcode, oldcode;
  static int clear_code, end_code;
  static int table[2][(1 << MAX_LZW_BITS)];
  static int stack[(1 << (MAX_LZW_BITS)) * 2], *sp;
  register int i;

  if (just_reset_LZW)
    {
      if (input_code_size > MAX_LZW_BITS)
        {
          g_message("Value out of range for code size (corrupted file?)");
          return -1;
        }

      set_code_size = input_code_size;
      code_size = set_code_size + 1;
      clear_code = 1 << set_code_size;
      end_code = clear_code + 1;
      max_code_size = 2 * clear_code;
      max_code = clear_code + 2;

      GetCode (fd, 0, TRUE);

      fresh = TRUE;

      sp = stack;

      for (i = 0; i < clear_code; ++i)
        {
          table[0][i] = 0;
          table[1][i] = i;
        }
      for (; i < (1 << MAX_LZW_BITS); ++i)
        {
          table[0][i] = 0;
          table[1][i] = 0;
        }

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
          for (; i < (1 << MAX_LZW_BITS); ++i)
            {
              table[0][i] = 0;
              table[1][i] = 0;
            }
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
              g_message ("Circular table entry.  Corrupt file.");
              gimp_quit ();
            }
          code = table[0][code];
        }

      *sp++ = firstcode = table[1][code];

      if ((code = max_code) < (1 << MAX_LZW_BITS))
        {
          table[0][code] = oldcode;
          table[1][code] = firstcode;
          ++max_code;
          if ((max_code >= max_code_size) &&
              (max_code_size < (1 << MAX_LZW_BITS)))
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
ReadImage (FILE        *fd,
           const gchar *filename,
           gint         len,
           gint         height,
           CMap         cmap,
           gint         ncols,
           gint         format,
           gint         interlace,
           gint         number,
           guint        leftpos,
           guint        toppos,
           guint        screenwidth,
           guint        screenheight)
{
  static gint32 image_ID   = -1;
  static gint frame_number = 1;

  gint32 layer_ID;
  GimpPixelRgn pixel_rgn;
  GimpDrawable *drawable;
  guchar *dest, *temp;
  guchar c;
  gint xpos = 0, ypos = 0, pass = 0;
  gint cur_progress, max_progress;
  gint v;
  gint i, j;
  gchar *framename;
  gchar *framename_ptr;
  gboolean alpha_frame = FALSE;
  static int previous_disposal;

  /* Guard against bogus frame size */
  if (len < 1 || height < 1)
    {
      g_message ("Bogus frame dimensions");
      return -1;
    }

  /*
   **  Initialize the Compression routines
   */
  if (!ReadOK (fd, &c, 1))
    {
      g_message ("EOF / read error on image data");
      return -1;
    }

  if (LZWReadByte (fd, TRUE, c) < 0)
    {
      g_message ("Error while reading");
      return -1;
    }

  if (frame_number == 1)
    {
      /* Guard against bogus logical screen size values */
      if (screenwidth == 0)
        screenwidth = len;

      if (screenheight == 0)
        screenheight = height;

      image_ID = gimp_image_new (screenwidth, screenheight, GIMP_INDEXED);
      gimp_image_set_filename (image_ID, filename);

      for (i = 0, j = 0; i < ncols; i++)
        {
          used_cmap[0][i] = gimp_cmap[j++] = cmap[0][i];
          used_cmap[1][i] = gimp_cmap[j++] = cmap[1][i];
          used_cmap[2][i] = gimp_cmap[j++] = cmap[2][i];
        }

      gimp_image_set_colormap (image_ID, gimp_cmap, ncols);

      if (Gif89.delayTime < 0)
        framename = g_strdup (_("Background"));
      else
        framename = g_strdup_printf (_("Background (%d%s)"),
                                     10 * Gif89.delayTime, "ms");

      previous_disposal = Gif89.disposal;

      if (Gif89.transparent == -1)
        {
          layer_ID = gimp_layer_new (image_ID, framename,
                                     len, height,
                                     GIMP_INDEXED_IMAGE, 100, GIMP_NORMAL_MODE);
        }
      else
        {
          layer_ID = gimp_layer_new (image_ID, framename,
                                     len, height,
                                     GIMP_INDEXEDA_IMAGE, 100, GIMP_NORMAL_MODE);
          alpha_frame=TRUE;
        }

      g_free (framename);
    }
  else /* NOT FIRST FRAME */
    {
      gimp_progress_set_text_printf (_("Opening '%s' (frame %d)"),
                                     gimp_filename_to_utf8 (filename),
                                     frame_number);
      gimp_progress_pulse ();

       /* If the colourmap is now different, we have to promote to RGB! */
      if (!promote_to_rgb)
        {
          for (i = 0; i < ncols; i++)
            {
              if ((used_cmap[0][i] != cmap[0][i]) ||
                  (used_cmap[1][i] != cmap[1][i]) ||
                  (used_cmap[2][i] != cmap[2][i]))
                {
                  /* Everything is RGB(A) from now on... sigh. */
                  promote_to_rgb = TRUE;

                  /* Promote everything we have so far into RGB(A) */
#ifdef GIFDEBUG
                  g_print ("GIF: Promoting image to RGB...\n");
#endif
                  gimp_image_convert_rgb (image_ID);

                  break;
                }
            }
        }

      if (Gif89.delayTime < 0)
        framename = g_strdup_printf (_("Frame %d"), frame_number);
      else
        framename = g_strdup_printf (_("Frame %d (%d%s)"),
                                     frame_number, 10 * Gif89.delayTime, "ms");

      switch (previous_disposal)
        {
        case 0x00:
          break; /* 'don't care' */
        case 0x01:
          framename_ptr = framename;
          framename = g_strconcat (framename, " (combine)", NULL);
          g_free (framename_ptr);
          break;
        case 0x02:
          framename_ptr = framename;
          framename = g_strconcat (framename, " (replace)", NULL);
          g_free (framename_ptr);
          break;
        case 0x03:  /* Rarely-used, and unhandled by many
                       loaders/players (including GIMP: we treat as
                       'combine' mode). */
          framename_ptr = framename;
          framename = g_strconcat (framename, " (combine) (!)", NULL);
          g_free (framename_ptr);
          break;
        case 0x04: /* I've seen a composite of this type. stvo_online_banner2.gif */
        case 0x05:
        case 0x06: /* I've seen a composite of this type. bn31.Gif */
        case 0x07:
          framename_ptr = framename;
          framename = g_strconcat (framename, " (unknown disposal)", NULL);
          g_free (framename_ptr);
          g_message (_("GIF: Undocumented GIF composite type %d is "
                       "not handled.  Animation might not play or "
                       "re-save perfectly."),
                     previous_disposal);
          break;
        default:
          g_message ("Disposal word got corrupted.  Bug.");
          break;
        }
      previous_disposal = Gif89.disposal;

      layer_ID = gimp_layer_new (image_ID, framename,
                                 len, height,
                                 promote_to_rgb ?
                                 GIMP_RGBA_IMAGE : GIMP_INDEXEDA_IMAGE,
                                 100, GIMP_NORMAL_MODE);
      alpha_frame = TRUE;
      g_free (framename);
    }

  frame_number++;

  gimp_image_add_layer (image_ID, layer_ID, 0);
  gimp_layer_translate (layer_ID, (gint) leftpos, (gint) toppos);

  drawable = gimp_drawable_get (layer_ID);

  cur_progress = 0;
  max_progress = height;

  if (alpha_frame)
    dest = (guchar *) g_malloc (len * height *
                                (promote_to_rgb ? 4 : 2));
  else
    dest = (guchar *) g_malloc (len * height);

#ifdef GIFDEBUG
    g_print ("GIF: reading %d by %d%s GIF image, ncols=%d\n",
             len, height, interlace ? " interlaced" : "", ncols);
#endif

  if (!alpha_frame && promote_to_rgb)
    {
      /* I don't see how one would easily construct a GIF in which
         this could happen, but it's a mad mad world. */
      g_message ("Ouch!  Can't handle non-alpha RGB frames.\n"
                 "Please file a bug report in GIMP's bugzilla.");
      gimp_quit ();
    }

  while ((v = LZWReadByte (fd, FALSE, c)) >= 0)
    {
      if (alpha_frame)
        {
          if (((guchar) v > highest_used_index) && !(v == Gif89.transparent))
            highest_used_index = (guchar) v;

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
          if ((guchar) v > highest_used_index)
            highest_used_index = (guchar) v;

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

          if (frame_number == 1)
            {
              cur_progress++;
              if ((cur_progress % 16) == 0)
                gimp_progress_update ((gdouble) cur_progress /
                                      (gdouble) max_progress);
            }
        }

      if (ypos >= height)
        break;
    }

fini:
  if (LZWReadByte (fd, FALSE, c) >= 0)
    g_print ("GIF: too much input data, ignoring extra...\n");

  gimp_pixel_rgn_init (&pixel_rgn, drawable,
                       0, 0, drawable->width, drawable->height, TRUE, FALSE);
  gimp_pixel_rgn_set_rect (&pixel_rgn, dest,
                           0, 0, drawable->width, drawable->height);

  g_free (dest);

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  return image_ID;
}
