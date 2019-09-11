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
 * - Remove unused colormap entries for GRAYSCALE images.
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


typedef struct _Gif      Gif;
typedef struct _GifClass GifClass;

struct _Gif
{
  GimpPlugIn      parent_instance;
};

struct _GifClass
{
  GimpPlugInClass parent_class;
};


#define GIF_TYPE  (gif_get_type ())
#define GIF (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIF_TYPE, Gif))

GType                   gif_get_type         (void) G_GNUC_CONST;

static GList          * gif_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * gif_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * gif_load             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * gif_load_thumb       (GimpProcedure        *procedure,
                                              GFile                *file,
                                              gint                  size,
                                              const GimpValueArray *args,
                                              gpointer              run_data);

static GimpImage      * load_image           (GFile                *file,
                                              gboolean              thumbnail,
                                              GError              **error);


G_DEFINE_TYPE (Gif, gif, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GIF_TYPE)


static guchar        used_cmap[3][256];
static guchar        highest_used_index;
static gboolean      promote_to_rgb   = FALSE;
static guchar        gimp_cmap[768];
static GimpParasite *comment_parasite = NULL;


static void
gif_class_init (GifClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = gif_query_procedures;
  plug_in_class->create_procedure = gif_create_procedure;
}

static void
gif_init (Gif *gif)
{
}

static GList *
gif_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static GimpProcedure *
gif_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           gif_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("GIF image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads files of Compuserve GIF "
                                        "file format",
                                        "FIXME: write help for gif_load",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball, Peter Mattis, "
                                      "Adam Moss, David Koblas",
                                      "Spencer Kimball, Peter Mattis, "
                                      "Adam Moss, David Koblas",
                                      "1995-2006");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/gif");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "gif");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,GIF8");

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                gif_load_thumb, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Loads only the first frame of a "
                                        "GIF image, to be "
                                        "used as a thumbnail",
                                        "",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Sven Neumann",
                                      "Sven Neumann",
                                      "2006");
    }

  return procedure;
}


static GimpValueArray *
gif_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  image = load_image (file, FALSE, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  /* The GIF format only tells you how many bits per pixel are in the
   *  image, not the actual number of used indices (D'OH!)
   *
   * So if we're not careful, repeated load/save of a transparent GIF
   *  without intermediate indexed->RGB->indexed pumps up the number
   *  of bits used, as we add an index each time for the transparent
   *  color.  Ouch.  We either do some heavier analysis at save-time,
   *  or trim down the number of GIMP colors at load-time.  We do the
   *  latter for now.
   */
#ifdef GIFDEBUG
  g_print ("GIF: Highest used index is %d\n", highest_used_index);
#endif

  if (! promote_to_rgb)
    gimp_image_set_colormap (image,
                             gimp_cmap, highest_used_index + 1);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
gif_load_thumb (GimpProcedure        *procedure,
                GFile                *file,
                gint                  size,
                const GimpValueArray *args,
                gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  image = load_image (file, TRUE, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  if (! promote_to_rgb)
    gimp_image_set_colormap (image, gimp_cmap, highest_used_index + 1);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);
  GIMP_VALUES_SET_INT   (return_vals, 2, gimp_image_width  (image));
  GIMP_VALUES_SET_INT   (return_vals, 3, gimp_image_height (image));

  gimp_value_array_truncate (return_vals, 4);

  return return_vals;
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

typedef guchar CMap[3][MAXCOLORMAPSIZE];

static struct
{
  guint Width;
  guint Height;
  CMap  ColorMap;
  guint BitPixel;
  guint ColorResolution;
  guint Background;
  guint AspectRatio;
  /*
   **
   */
  gint  GrayScale;
} GifScreen;

static struct
{
  gint transparent;
  gint delayTime;
  gint inputFlag;
  gint disposal;
} Gif89 = { -1, -1, -1, 0 };

static gboolean ReadColorMap (FILE        *fd,
                              gint         number,
                              CMap         buffer,
                              gint        *format);
static gint     DoExtension  (FILE        *fd,
                              gint         label);
static gint     GetDataBlock (FILE        *fd,
                              guchar      *buf);
static gint     GetCode      (FILE        *fd,
                              gint         code_size,
                              gboolean     flag);
static gint     LZWReadByte  (FILE        *fd,
                              gint         just_reset_LZW,
                              gint         input_code_size);
static gboolean ReadImage    (FILE        *fd,
                              GFile       *file,
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
                              guint        screenheight,
                              GimpImage  **image);


static GimpImage *
load_image (GFile     *file,
            gboolean   thumbnail,
            GError   **error)
{
  FILE      *fd;
  gchar     *filename;
  guchar     buf[16];
  guchar     c;
  CMap       localColorMap;
  gint       grayScale;
  gboolean   useGlobalColormap;
  gint       bitPixel;
  gint       imageCount = 0;
  gchar      version[4];
  GimpImage *image      = NULL;
  gboolean   status;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  filename = g_file_get_path (file);
  fd = g_fopen (filename, "rb");
  g_free (filename);

  if (! fd)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  if (! ReadOK (fd, buf, 6))
    {
      g_message ("Error reading magic number");
      fclose (fd);
      return NULL;
    }

  if (strncmp ((gchar *) buf, "GIF", 3) != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s", _("This is not a GIF file"));
      fclose (fd);
      return NULL;
    }

  g_strlcpy (version, (gchar *) buf + 3, 4);

  if ((strcmp (version, "87a") != 0) && (strcmp (version, "89a") != 0))
    {
      g_message ("Bad version number, not '87a' or '89a'");
      fclose (fd);
      return NULL;
    }

  if (! ReadOK (fd, buf, 7))
    {
      g_message ("Failed to read screen descriptor");
      fclose (fd);
      return NULL;
    }

  GifScreen.Width           = LM_to_uint (buf[0], buf[1]);
  GifScreen.Height          = LM_to_uint (buf[2], buf[3]);
  GifScreen.BitPixel        = 2 << (buf[4] & 0x07);
  GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
  GifScreen.Background      = buf[5];
  GifScreen.AspectRatio     = buf[6];

  if (BitSet (buf[4], LOCALCOLORMAP))
    {
      /* Global Colormap */
      if (! ReadColorMap (fd, GifScreen.BitPixel, GifScreen.ColorMap,
                          &GifScreen.GrayScale))
        {
          g_message ("Error reading global colormap");
          fclose (fd);
          return NULL;
        }
    }

  if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49)
    {
      g_message (_("Non-square pixels.  Image might look squashed."));
    }


  highest_used_index = 0;

  while (TRUE)
    {
      if (! ReadOK (fd, &c, 1))
        {
          g_message ("EOF / read error on image data");
          fclose (fd);
          return image; /* will be NULL if failed on first image! */
        }

      if (c == ';')
        {
          /* GIF terminator */
          fclose (fd);
          return image;
        }

      if (c == '!')
        {
          /* Extension */
          if (! ReadOK (fd, &c, 1))
            {
              g_message ("EOF / read error on extension function code");
              fclose (fd);
              return image; /* will be NULL if failed on first image! */
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

      if (! ReadOK (fd, buf, 9))
        {
          g_message ("Couldn't read left/top/width/height");
          fclose (fd);
          return image; /* will be NULL if failed on first image! */
        }

      useGlobalColormap = !BitSet (buf[8], LOCALCOLORMAP);

      bitPixel = 1 << ((buf[8] & 0x07) + 1);

      if (! useGlobalColormap)
        {
          if (! ReadColorMap (fd, bitPixel, localColorMap, &grayScale))
            {
              g_message ("Error reading local colormap");
              fclose (fd);
              return image; /* will be NULL if failed on first image! */
            }

          status = ReadImage (fd, file, LM_to_uint (buf[4], buf[5]),
                              LM_to_uint (buf[6], buf[7]),
                              localColorMap, bitPixel,
                              grayScale,
                              BitSet (buf[8], INTERLACE), imageCount,
                              (guint) LM_to_uint (buf[0], buf[1]),
                              (guint) LM_to_uint (buf[2], buf[3]),
                              GifScreen.Width,
                              GifScreen.Height,
                              &image);
        }
      else
        {
          status = ReadImage (fd, file, LM_to_uint (buf[4], buf[5]),
                              LM_to_uint (buf[6], buf[7]),
                              GifScreen.ColorMap, GifScreen.BitPixel,
                              GifScreen.GrayScale,
                              BitSet (buf[8], INTERLACE), imageCount,
                              (guint) LM_to_uint (buf[0], buf[1]),
                              (guint) LM_to_uint (buf[2], buf[3]),
                              GifScreen.Width,
                              GifScreen.Height,
                              &image);
        }

      if (!status)
        {
          break;
        }

      if (comment_parasite != NULL)
        {
          if (! thumbnail)
            gimp_image_attach_parasite (image, comment_parasite);

          gimp_parasite_free (comment_parasite);
          comment_parasite = NULL;
        }

      /* If we are loading a thumbnail, we stop after the first frame. */
      if (thumbnail)
        break;
    }

  fclose (fd);

  return image;
}

static gboolean
ReadColorMap (FILE *fd,
              gint  number,
              CMap  buffer,
              gint *format)
{
  guchar rgb[3];
  gint   flag;
  gint   i;

  flag = TRUE;

  for (i = 0; i < number; ++i)
    {
      if (! ReadOK (fd, rgb, sizeof (rgb)))
        return FALSE;

      buffer[CM_RED][i] = rgb[0];
      buffer[CM_GREEN][i] = rgb[1];
      buffer[CM_BLUE][i] = rgb[2];

      flag &= (rgb[0] == rgb[1] && rgb[1] == rgb[2]);
    }

  *format = (flag) ? GRAYSCALE : COLOR;

  return TRUE;
}

static gint
DoExtension (FILE *fd,
             gint  label)
{
  static guchar  buf[256];
#ifdef GIFDEBUG
  gchar         *str;
#endif

  switch (label)
    {
    case 0x01:                  /* Plain Text Extension */
#ifdef GIFDEBUG
      str = "Plain Text Extension";
#endif

#ifdef notdef
      if (GetDataBlock (fd, (guchar *) buf) == 0)
        ;

      lpos       = LM_to_uint (buf[0], buf[1]);
      tpos       = LM_to_uint (buf[2], buf[3]);
      width      = LM_to_uint (buf[4], buf[5]);
      height     = LM_to_uint (buf[6], buf[7]);
      cellw      = buf[8];
      cellh      = buf[9];
      foreground = buf[10];
      background = buf[11];

      while (GetDataBlock (fd, (guchar *) buf) > 0)
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
#ifdef GIFDEBUG
      str = "Application Extension";
#endif
      break;
    case 0xfe:                  /* Comment Extension */
#ifdef GIFDEBUG
      str = "Comment Extension";
#endif
      while (GetDataBlock (fd, (guchar *) buf) > 0)
        {
          gchar *comment = (gchar *) buf;

          if (! g_utf8_validate (comment, -1, NULL))
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
#ifdef GIFDEBUG
      str = "Graphic Control Extension";
#endif
      (void) GetDataBlock (fd, (guchar *) buf);
      Gif89.disposal  = (buf[0] >> 2) & 0x7;
      Gif89.inputFlag = (buf[0] >> 1) & 0x1;
      Gif89.delayTime = LM_to_uint (buf[1], buf[2]);
      if ((buf[0] & 0x1) != 0)
        Gif89.transparent = buf[3];
      else
        Gif89.transparent = -1;

      while (GetDataBlock (fd, (guchar *) buf) > 0);

      return FALSE;
      break;

    default:
#ifdef GIFDEBUG
      str = (gchar *)buf;
#endif
      sprintf ((gchar *)buf, "UNKNOWN (0x%02x)", label);
      break;
    }

#ifdef GIFDEBUG
  g_print ("GIF: got a '%s'\n", str);
#endif

  while (GetDataBlock (fd, (guchar *) buf) > 0);

  return FALSE;
}

static gint ZeroDataBlock = FALSE;

static gint
GetDataBlock (FILE   *fd,
              guchar *buf)
{
  guchar count;

  if (! ReadOK (fd, &count, 1))
    {
      g_message ("Error in getting DataBlock size");
      return -1;
    }

  ZeroDataBlock = (count == 0);

  if ((count != 0) && (! ReadOK (fd, buf, count)))
    {
      g_message ("Error in reading DataBlock");
      return -1;
    }

  return count;
}

static gint
GetCode (FILE     *fd,
         gint      code_size,
         gboolean  flag)
{
  static guchar buf[280];
  static gint   curbit, lastbit, done, last_byte;
  gint          i, j, ret, count;

  if (flag)
    {
      curbit = 0;
      lastbit = 0;
      done = FALSE;
      last_byte = 2;
      return 0;
    }

  while ((curbit + code_size) > lastbit)
    {
      if (done)
        {
          if (curbit >= lastbit)
            g_message ("Ran off the end of my bits");

          return -1;
        }

      buf[0] = buf[last_byte - 2];
      buf[1] = buf[last_byte - 1];

      count = GetDataBlock (fd, &buf[2]);
      if (count < 0)
        {
          return -1;
        }
      else if (count == 0)
        {
          done = TRUE;
        }

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

static gint
LZWReadByte (FILE *fd,
             gint  just_reset_LZW,
             gint  input_code_size)
{
  static gint fresh = FALSE;
  gint        code, incode;
  static gint code_size, set_code_size;
  static gint max_code, max_code_size;
  static gint firstcode, oldcode;
  static gint clear_code, end_code;
  static gint table[2][(1 << MAX_LZW_BITS)];
#define STACK_SIZE ((1 << (MAX_LZW_BITS)) * 2)
  static gint stack[STACK_SIZE], *sp;
  gint        i;

  if (just_reset_LZW)
    {
      if (input_code_size > MAX_LZW_BITS || input_code_size <= 1)
        {
          g_message ("Value out of range for code size (corrupted file?)");
          return -1;
        }

      set_code_size = input_code_size;
      code_size     = set_code_size + 1;
      clear_code    = 1 << set_code_size;
      end_code      = clear_code + 1;
      max_code_size = 2 * clear_code;
      max_code      = clear_code + 2;

      if (GetCode (fd, 0, TRUE) < 0)
        {
          return -1;
        }

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
          firstcode = oldcode = GetCode (fd, code_size, FALSE);
        }
      while (firstcode == clear_code);

      if (firstcode < 0)
        {
          return -1;
        }

      return firstcode & 255;
    }

  if (sp > stack)
    return (*--sp) & 255;

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

          code_size     = set_code_size + 1;
          max_code_size = 2 * clear_code;
          max_code      = clear_code + 2;
          sp            = stack;
          firstcode     = oldcode = GetCode (fd, code_size, FALSE);

          if (firstcode < 0)
            {
              return -1;
            }

          return firstcode & 255;
        }
      else if (code == end_code || code > max_code)
        {
          gint   count;
          guchar buf[260];

          if (ZeroDataBlock)
            return -2;

          while ((count = GetDataBlock (fd, buf)) > 0)
            ;

          if (count != 0)
            g_print ("GIF: missing EOD in data stream (common occurrence)");

          return -2;
        }

      incode = code;

      if (code == max_code)
        {
          if (sp < &(stack[STACK_SIZE]))
            *sp++ = firstcode;
          code = oldcode;
        }

      while (code >= clear_code && sp < &(stack[STACK_SIZE]))
        {
          *sp++ = table[1][code];
          if (code == table[0][code])
            {
              g_message ("Circular table entry.  Corrupt file.");
              gimp_quit ();
            }
          code = table[0][code];
        }

      if (sp < &(stack[STACK_SIZE]))
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
        return (*--sp) & 255;
    }

  if (code < 0)
    {
      return -1;
    }

  return code & 255;
}

static gboolean
ReadImage (FILE        *fd,
           GFile       *file,
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
           guint        screenheight,
           GimpImage  **image)
{
  static gint   frame_number = 1;

  GimpLayer    *layer;
  GeglBuffer   *buffer;
  guchar       *dest, *temp;
  guchar        c;
  gint          xpos = 0, ypos = 0, pass = 0;
  gint          cur_progress, max_progress;
  gint          v;
  gint          i, j;
  gchar        *framename;
  gchar        *framename_ptr;
  gboolean      alpha_frame = FALSE;
  static gint   previous_disposal;

  /* Guard against bogus frame size */
  if (len < 1 || height < 1)
    {
      g_message ("Bogus frame dimensions");
      *image = NULL;
      return FALSE;
    }

  /*
   **  Initialize the Compression routines
   */
  if (! ReadOK (fd, &c, 1))
    {
      g_message ("EOF / read error on image data");
      *image = NULL;
      return FALSE;
    }

  if (LZWReadByte (fd, TRUE, c) < 0)
    {
      g_message ("Error while reading");
      *image = NULL;
      return FALSE;
    }

  if (frame_number == 1)
    {
      /* Guard against bogus logical screen size values */
      if (screenwidth == 0)
        screenwidth = len;

      if (screenheight == 0)
        screenheight = height;

      *image = gimp_image_new (screenwidth, screenheight, GIMP_INDEXED);
      gimp_image_set_file (*image, file);

      for (i = 0, j = 0; i < ncols; i++)
        {
          used_cmap[0][i] = gimp_cmap[j++] = cmap[0][i];
          used_cmap[1][i] = gimp_cmap[j++] = cmap[1][i];
          used_cmap[2][i] = gimp_cmap[j++] = cmap[2][i];
        }

      gimp_image_set_colormap (*image, gimp_cmap, ncols);

      if (Gif89.delayTime < 0)
        framename = g_strdup (_("Background"));
      else
        framename = g_strdup_printf (_("Background (%d%s)"),
                                     10 * Gif89.delayTime, "ms");

      previous_disposal = Gif89.disposal;

      if (Gif89.transparent == -1)
        {
          layer = gimp_layer_new (*image, framename,
                                  len, height,
                                  GIMP_INDEXED_IMAGE,
                                  100,
                                  gimp_image_get_default_new_layer_mode (*image));
        }
      else
        {
          layer = gimp_layer_new (*image, framename,
                                  len, height,
                                  GIMP_INDEXEDA_IMAGE,
                                  100,
                                  gimp_image_get_default_new_layer_mode (*image));
          alpha_frame=TRUE;
        }

      g_free (framename);
    }
  else /* NOT FIRST FRAME */
    {
      gimp_progress_set_text_printf (_("Opening '%s' (frame %d)"),
                                     gimp_file_get_utf8_name (file),
                                     frame_number);
      gimp_progress_pulse ();

       /* If the colormap is now different, we have to promote to RGB! */
      if (! promote_to_rgb)
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
                  gimp_image_convert_rgb (*image);

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

      layer = gimp_layer_new (*image, framename,
                              len, height,
                              promote_to_rgb ?
                              GIMP_RGBA_IMAGE : GIMP_INDEXEDA_IMAGE,
                              100,
                              gimp_image_get_default_new_layer_mode (*image));
      alpha_frame = TRUE;
      g_free (framename);
    }

  frame_number++;

  gimp_image_insert_layer (*image, layer, NULL, 0);
  gimp_item_transform_translate (GIMP_ITEM (layer), (gint) leftpos, (gint) toppos);

  cur_progress = 0;
  max_progress = height;

  if (len > (G_MAXSIZE / height / (alpha_frame ? (promote_to_rgb ? 4 : 2) : 1)))
    {
      g_message ("'%s' has a larger image size than GIMP can handle.",
                 gimp_file_get_utf8_name (file));
      gimp_image_delete (*image);
      *image = NULL;
      return FALSE;
    }

  if (alpha_frame)
    dest = (guchar *) g_malloc ((gsize)len * (gsize)height * (promote_to_rgb ? 4 : 2));
  else
    dest = (guchar *) g_malloc ((gsize)len * (gsize)height);

#ifdef GIFDEBUG
    g_print ("GIF: reading %d by %d%s GIF image, ncols=%d\n",
             len, height, interlace ? " interlaced" : "", ncols);
#endif

  if (! alpha_frame && promote_to_rgb)
    {
      /* I don't see how one would easily construct a GIF in which
         this could happen, but it's a mad mad world. */
      g_message ("Ouch!  Can't handle non-alpha RGB frames.\n"
                 "Please file a bug report at "
                 "https://gitlab.gnome.org/GNOME/gimp/issues");
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

  if (v < 0)
    {
      return FALSE;
    }

 fini:
  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, len, height), 0,
                   NULL, dest, GEGL_AUTO_ROWSTRIDE);

  g_free (dest);

  g_object_unref (buffer);

  gimp_progress_update (1.0);

  if (LZWReadByte (fd, FALSE, c) >= 0)
    {
      g_print ("GIF: too much input data, ignoring extra...\n");
      return FALSE;
    }

  return TRUE;
}
