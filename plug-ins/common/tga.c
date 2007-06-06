/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * $Id$
 * TrueVision Targa loading and saving file filter for GIMP.
 * Targa code Copyright (C) 1997 Raphael FRANCOIS and Gordon Matzigkeit
 *
 * The Targa reading and writing code was written from scratch by
 * Raphael FRANCOIS <fraph@ibm.net> and Gordon Matzigkeit
 * <gord@gnu.ai.mit.edu> based on the TrueVision TGA File Format
 * Specification, Version 2.0:
 *
 *   <URL:ftp://ftp.truevision.com/pub/TGA.File.Format.Spec/>
 *
 * It does not contain any code written for other TGA file loaders.
 * Not even the RLE handling. ;)
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
 */

/*
 * Modified August-November 2000, Nick Lamb <njl195@zepler.org.uk>
 *   - Clean-up more code, avoid structure implementation dependency,
 *   - Load more types of images reliably, reject others firmly
 *   - This is not perfect, but I think it's much better. Test please!
 *
 * Release 1.2, 1997-09-24, Gordon Matzigkeit <gord@gnu.ai.mit.edu>:
 *   - Bug fixes and source cleanups.
 *
 * Release 1.1, 1997-09-19, Gordon Matzigkeit <gord@gnu.ai.mit.edu>:
 *   - Preserve alpha channels.  For indexed images, this can only be
 *     done if there is at least one free colormap entry.
 *
 * Release 1.0, 1997-09-06, Gordon Matzigkeit <gord@gnu.ai.mit.edu>:
 *   - Handle loading all image types from the 2.0 specification.
 *   - Fix many alignment and endianness problems.
 *   - Use tiles for lower memory consumption and better speed.
 *   - Rewrite RLE code for clarity and speed.
 *   - Handle saving with RLE.
 *
 * Release 0.9, 1997-06-18, Raphael FRANCOIS <fraph@ibm.net>:
 *   - Can load 24 and 32-bit Truecolor images, with and without RLE.
 *   - Saving currently only works without RLE.
 *
 *
 * TODO:
 *   - GIMP stores the indexed alpha channel as a separate byte,
 *     one for each pixel.  The TGA file format spec requires that the
 *     alpha channel be stored as part of the colormap, not with each
 *     individual pixel.  This means that we have no good way of
 *     saving and loading INDEXEDA images that use alpha channel values
 *     other than 0 and 255.  Find a workaround.
 */

/* Set these for debugging. */
/* #define PROFILE 1 */

#include "config.h"

#ifdef PROFILE
# include <sys/times.h>
#endif

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-tga-load"
#define SAVE_PROC      "file-tga-save"
#define PLUG_IN_BINARY "tga"


typedef struct _TgaSaveVals
{
  gint rle;
  gint origin;
} TgaSaveVals;

static TgaSaveVals tsvals =
{
  1,    /* rle = ON */
  1,    /* origin = bottom left */
};


 /* TRUEVISION-XFILE magic signature string */
static guchar magic[18] =
{
  0x54, 0x52, 0x55, 0x45, 0x56, 0x49, 0x53, 0x49, 0x4f,
  0x4e, 0x2d, 0x58, 0x46, 0x49, 0x4c, 0x45, 0x2e, 0x0
};

typedef struct tga_info_struct
{
  guint8 idLength;
  guint8 colorMapType;

  guint8 imageType;
  /* Known image types. */
#define TGA_TYPE_MAPPED      1
#define TGA_TYPE_COLOR       2
#define TGA_TYPE_GRAY        3

  guint8 imageCompression;
  /* Only known compression is RLE */
#define TGA_COMP_NONE        0
#define TGA_COMP_RLE         1

  /* Color Map Specification. */
  /* We need to separately specify high and low bytes to avoid endianness
     and alignment problems. */

  guint16 colorMapIndex;
  guint16 colorMapLength;
  guint8 colorMapSize;

  /* Image Specification. */
  guint16 xOrigin;
  guint16 yOrigin;

  guint16 width;
  guint16 height;

  guint8 bpp;
  guint8 bytes;

  guint8 alphaBits;
  guint8 flipHoriz;
  guint8 flipVert;

  /* Extensions (version 2) */

/* Not all the structures described in the standard are transcribed here
   only those which seem applicable to Gimp */

  gchar   authorName[41];
  gchar   comment[324];
  guint   month, day, year, hour, minute, second;
  gchar   jobName[41];
  gchar   softwareID[41];
  guint   pixelWidth, pixelHeight;  /* write dpi? */
  gdouble gamma;
} tga_info;


/* Declare some local functions.
 */
static void      query         (void);
static void      run           (const gchar      *name,
                                gint              nparams,
                                const GimpParam  *param,
                                gint             *nreturn_vals,
                                GimpParam       **return_vals);

static gint32    load_image    (const gchar      *filename);
static gint      save_image    (const gchar      *filename,
                                gint32            image_ID,
                                gint32            drawable_ID);

static gboolean  save_dialog   (void);

static gint32    ReadImage     (FILE             *fp,
                                tga_info         *info,
                                const gchar      *filename);


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
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save"             },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" },
    { GIMP_PDB_INT32,    "rle",          "Use RLE compression"          },
    { GIMP_PDB_INT32,    "origin",       "Image origin (0 = top-left, 1 = bottom-left)"}
  } ;

  gimp_install_procedure (LOAD_PROC,
                          "Loads files of Targa file format",
                          "FIXME: write help for tga_load",
                          "Raphael FRANCOIS, Gordon Matzigkeit",
                          "Raphael FRANCOIS, Gordon Matzigkeit",
                          "1997",
                          N_("TarGA image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-tga");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "tga,vda,icb,vst",
                                    "",
                                    "-18&,string,TRUEVISION-XFILE.,-1,byte,0");

  gimp_install_procedure (SAVE_PROC,
                          "saves files in the Targa file format",
                          "FIXME: write help for tga_save",
                          "Raphael FRANCOIS, Gordon Matzigkeit",
                          "Raphael FRANCOIS, Gordon Matzigkeit",
                          "1997",
                          N_("TarGA image"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/x-tga");
  gimp_register_save_handler (SAVE_PROC, "tga", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  gint32            drawable_ID;
  GimpExportReturn  export = GIMP_EXPORT_CANCEL;

#ifdef PROFILE
  struct tms tbuf1, tbuf2;
#endif

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
#ifdef PROFILE
      times (&tbuf1);
#endif

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
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      gimp_ui_init (PLUG_IN_BINARY, FALSE);

      image_ID     = param[1].data.d_int32;
      drawable_ID  = param[2].data.d_int32;

      /*  eventually export the image */
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          export = gimp_export_image (&image_ID, &drawable_ID, "TGA",
                                      (GIMP_EXPORT_CAN_HANDLE_RGB |
                                       GIMP_EXPORT_CAN_HANDLE_GRAY |
                                       GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                       GIMP_EXPORT_CAN_HANDLE_ALPHA ));
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
          gimp_get_data (SAVE_PROC, &tsvals);

          /*  First acquire information with a dialog  */
          if (! save_dialog ())
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 7)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              tsvals.rle = (param[5].data.d_int32) ? TRUE : FALSE;
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /*  Possibly retrieve data  */
          gimp_get_data (SAVE_PROC, &tsvals);
          break;

        default:
          break;
        }

#ifdef PROFILE
      times (&tbuf1);
#endif

      if (status == GIMP_PDB_SUCCESS)
        {
          if (save_image (param[3].data.d_string, image_ID, drawable_ID))
            {
              /*  Store psvals data  */
              gimp_set_data (SAVE_PROC, &tsvals, sizeof (tsvals));
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

  values[0].data.d_status = status;

#ifdef PROFILE
  times (&tbuf2);
  printf ("TGA: %s profile: %ld user %ld system\n", name,
          (long) tbuf2.tms_utime - tbuf1.tms_utime,
          (long) tbuf2.tms_stime - tbuf2.tms_stime);
#endif
}

static gint32
load_image (const gchar *filename)
{
  FILE     *fp;
  tga_info  info;
  guchar    header[18];
  guchar    footer[26];
  guchar    extension[495];
  long      offset;

  gint32 image_ID = -1;

  fp = g_fopen (filename, "rb");
  if (!fp)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  if (!fseek (fp, -26L, SEEK_END)) { /* Is file big enough for a footer? */
    if (fread (footer, sizeof (footer), 1, fp) != 1)
      {
        g_message (_("Cannot read footer from '%s'"),
                   gimp_filename_to_utf8 (filename));
        return -1;
      }
    else if (memcmp (footer + 8, magic, sizeof (magic)) == 0)
      {
        /* Check the signature. */

        offset= footer[0] + (footer[1] * 256) + (footer[2] * 65536)
                          + (footer[3] * 16777216);

        if (offset != 0)
          {
            if (fseek (fp, offset, SEEK_SET) ||
                fread (extension, sizeof (extension), 1, fp) != 1)
              {
                g_message (_("Cannot read extension from '%s'"),
                           gimp_filename_to_utf8 (filename));
                return -1;
              }
            /* Eventually actually handle version 2 TGA here */
          }
      }
  }

  if (fseek (fp, 0, SEEK_SET) ||
      fread (header, sizeof (header), 1, fp) != 1)
    {
      g_message ("Cannot read header from '%s'",
                 gimp_filename_to_utf8 (filename));
      return -1;
    }

  switch (header[2])
    {
    case 1:
      info.imageType = TGA_TYPE_MAPPED;
      info.imageCompression = TGA_COMP_NONE;
      break;
    case 2:
      info.imageType = TGA_TYPE_COLOR;
      info.imageCompression = TGA_COMP_NONE;
      break;
    case 3:
      info.imageType = TGA_TYPE_GRAY;
      info.imageCompression = TGA_COMP_NONE;
      break;

    case 9:
      info.imageType = TGA_TYPE_MAPPED;
      info.imageCompression = TGA_COMP_RLE;
      break;
    case 10:
      info.imageType = TGA_TYPE_COLOR;
      info.imageCompression = TGA_COMP_RLE;
      break;
    case 11:
      info.imageType = TGA_TYPE_GRAY;
      info.imageCompression = TGA_COMP_RLE;
      break;

    default:
      info.imageType = 0;
    }

  info.idLength     = header[0];
  info.colorMapType = header[1];

  info.colorMapIndex  = header[3] + header[4] * 256;
  info.colorMapLength = header[5] + header[6] * 256;
  info.colorMapSize   = header[7];

  info.xOrigin = header[8] + header[9] * 256;
  info.yOrigin = header[10] + header[11] * 256;
  info.width   = header[12] + header[13] * 256;
  info.height  = header[14] + header[15] * 256;

  info.bpp       = header[16];
  info.bytes     = (info.bpp + 7) / 8;
  info.alphaBits = header[17] & 0x0f; /* Just the low 4 bits */
  info.flipHoriz = (header[17] & 0x10) ? 1 : 0;
  info.flipVert  = (header[17] & 0x20) ? 0 : 1;

  /* hack to handle some existing files with incorrect headers, see bug #306675 */
  if (info.alphaBits == info.bpp)
    info.alphaBits = 0;

  switch (info.imageType)
    {
      case TGA_TYPE_MAPPED:
        if (info.bpp != 8)
          {
            g_message ("Unhandled sub-format in '%s'",
                       gimp_filename_to_utf8 (filename));
            return -1;
          }
        break;
      case TGA_TYPE_COLOR:
        if (info.bpp != 15 && info.bpp != 16 && info.bpp != 24
                     && info.bpp != 32)
          {
            g_message ("Unhandled sub-format in '%s'",
                       gimp_filename_to_utf8 (filename));
            return -1;
          }
        break;
      case TGA_TYPE_GRAY:
        if (info.bpp != 8 && (info.alphaBits != 8 || (info.bpp != 16 && info.bpp != 15)))
          {
            g_message ("Unhandled sub-format in '%s'",
                       gimp_filename_to_utf8 (filename));
            return -1;
          }
        break;

      default:
        g_message ("Unknown image type for '%s'",
                   gimp_filename_to_utf8 (filename));
        return -1;
    }

  /* Plausible but unhandled formats */
  if (info.bytes * 8 != info.bpp && !(info.bytes == 2 && info.bpp == 15))
    {
      g_message ("No support yet for TGA with these parameters");
      return -1;
    }

  /* Check that we have a color map only when we need it. */
  if (info.imageType == TGA_TYPE_MAPPED && info.colorMapType != 1)
    {
      g_message ("Indexed image has invalid color map type %d",
                 info.colorMapType);
      return -1;
    }
  else if (info.imageType != TGA_TYPE_MAPPED && info.colorMapType != 0)
    {
      g_message ("Non-indexed image has invalid color map type %d",
                 info.colorMapType);
      return -1;
    }

  /* Skip the image ID field. */
  if (info.idLength && fseek (fp, info.idLength, SEEK_CUR))
    {
      g_message ("File '%s' is truncated or corrupted",
                 gimp_filename_to_utf8 (filename));
      return -1;
    }

  image_ID = ReadImage (fp, &info, filename);
  fclose (fp);
  return image_ID;
}

static void
rle_write (FILE   *fp,
           guchar *buffer,
           guint   width,
           guint   bytes)
{
  gint    repeat = 0;
  gint    direct = 0;
  guchar *from   = buffer;
  guint   x;

  for (x = 1; x < width; ++x)
    {
      if (memcmp (buffer, buffer + bytes, bytes))
        {
          /* next pixel is different */
          if (repeat)
            {
              putc (128 + repeat, fp);
              fwrite (from, bytes, 1, fp);
              from = buffer+ bytes; /* point to first different pixel */
              repeat = 0;
              direct = 0;
            }
          else
            {
              direct += 1;
            }
        }
      else
        {
          /* next pixel is the same */
          if (direct)
            {
              putc (direct - 1, fp);
              fwrite (from, bytes, direct, fp);
              from = buffer; /* point to first identical pixel */
              direct = 0;
              repeat = 1;
            }
          else
            {
              repeat += 1;
            }
        }

      if (repeat == 128)
        {
          putc (255, fp);
          fwrite (from, bytes, 1, fp);
          from = buffer+ bytes;
          direct = 0;
          repeat = 0;
        }
      else if (direct == 128)
        {
          putc (127, fp);
          fwrite (from, bytes, direct, fp);
          from = buffer+ bytes;
          direct = 0;
          repeat = 0;
        }

      buffer += bytes;
    }

  if (repeat > 0)
    {
      putc (128 + repeat, fp);
      fwrite (from, bytes, 1, fp);
    }
  else
    {
      putc (direct, fp);
      fwrite (from, bytes, direct + 1, fp);
    }
}

static gint
rle_read (FILE     *fp,
          guchar   *buffer,
          tga_info *info)
{
  static gint   repeat = 0;
  static gint   direct = 0;
  static guchar sample[4];
  gint head;
  gint x, k;

  for (x = 0; x < info->width; x++)
    {
      if (repeat == 0 && direct == 0)
        {
          head = getc (fp);

          if (head == EOF)
            {
              return EOF;
            }
          else if (head >= 128)
            {
              repeat = head - 127;

              if (fread (sample, info->bytes, 1, fp) < 1)
                return EOF;
            }
          else
            {
              direct = head + 1;
            }
        }

      if (repeat > 0)
        {
          for (k = 0; k < info->bytes; ++k)
            {
              buffer[k] = sample[k];
            }

          repeat--;
        }
      else /* direct > 0 */
        {
          if (fread (buffer, info->bytes, 1, fp) < 1)
            return EOF;

          direct--;
        }

      buffer += info->bytes;
    }

  return 0;
}

static void
flip_line (guchar   *buffer,
           tga_info *info)
{
  guchar  temp;
  guchar *alt;
  gint    x, s;

  alt = buffer + (info->bytes * (info->width - 1));

  for (x = 0; x * 2 <= info->width; x++)
    {
      for (s = 0; s < info->bytes; ++s)
        {
          temp = buffer[s];
          buffer[s] = alt[s];
          alt[s] = temp;
        }

      buffer += info->bytes;
      alt -= info->bytes;
    }
}

/* Some people write 16-bit RGB TGA files. The spec would probably
   allow 27-bit RGB too, for what it's worth, but I won't fix that
   unless someone actually provides an existence proof */

static void
upsample (guchar *dest,
          guchar *src,
          guint   width,
          guint   bytes,
          guint8  alphaBits)
{
  guint x;

  for (x = 0; x < width; x++)
    {
      dest[0] =  ((src[1] << 1) & 0xf8);
      dest[0] += (dest[0] >> 5);

      dest[1] =  ((src[0] & 0xe0) >> 2) + ((src[1] & 0x03) << 6);
      dest[1] += (dest[1] >> 5);

      dest[2] =  ((src[0] << 3) & 0xf8);
      dest[2] += (dest[2] >> 5);

      switch (alphaBits)
        {
        case 1:
          dest[3] = (src[1] & 0x80)? 0: 255;
          dest += 4;
          break;
        default:
          dest += 3;
        }

      src += bytes;
    }
}

static void
bgr2rgb (guchar *dest,
         guchar *src,
         guint   width,
         guint   bytes,
         guint   alpha)
{
  guint x;

  if (alpha)
    {
      for (x = 0; x < width; x++)
        {
          *(dest++) = src[2];
          *(dest++) = src[1];
          *(dest++) = src[0];
          *(dest++) = src[3];

          src += bytes;
        }
    }
  else
    {
      for (x = 0; x < width; x++)
        {
          *(dest++) = src[2];
          *(dest++) = src[1];
          *(dest++) = src[0];

          src += bytes;
        }
    }
}

static void
read_line (FILE         *fp,
           guchar       *row,
           guchar       *buffer,
           tga_info     *info,
           GimpDrawable *drawable)
{
  if (info->imageCompression == TGA_COMP_RLE)
    {
      rle_read (fp, buffer, info);
    }
  else
    {
      fread (buffer, info->bytes, info->width, fp);
    }

  if (info->flipHoriz)
    {
      flip_line (buffer, info);
    }

  if (info->imageType == TGA_TYPE_COLOR)
    {
      if (info->bpp == 16 || info->bpp == 15)
        {
          upsample (row, buffer, info->width, info->bytes, info->alphaBits);
        }
      else
        {
          bgr2rgb (row, buffer,info->width, info->bytes,info->alphaBits);
        }
    }
  else
    {
      memcpy (row, buffer, info->width * drawable->bpp);
    }
}

static gint32
ReadImage (FILE        *fp,
           tga_info    *info,
           const gchar *filename)
{
  static gint32 image_ID;
  gint32        layer_ID;

  GimpPixelRgn       pixel_rgn;
  GimpDrawable      *drawable;
  guchar            *data, *buffer, *row;
  GimpImageType      dtype = 0;
  GimpImageBaseType  itype = 0;
  gint               i, y;

  gint max_tileheight, tileheight;

  guint  cmap_bytes;
  guchar tga_cmap[4 * 256], gimp_cmap[3 * 256];

  switch (info->imageType)
    {
    case TGA_TYPE_MAPPED:
      itype = GIMP_INDEXED;

      if (info->alphaBits)
        dtype = GIMP_INDEXEDA_IMAGE;
      else
        dtype = GIMP_INDEXED_IMAGE;
      break;

    case TGA_TYPE_GRAY:
      itype = GIMP_GRAY;

      if (info->alphaBits)
        dtype = GIMP_GRAYA_IMAGE;
      else
        dtype = GIMP_GRAY_IMAGE;
      break;

    case TGA_TYPE_COLOR:
      itype = GIMP_RGB;

      if (info->alphaBits)
        dtype = GIMP_RGBA_IMAGE;
      else
        dtype = GIMP_RGB_IMAGE;
      break;
    }

  /* Handle colormap */

  if (info->colorMapType == 1)
    {
      cmap_bytes= (info->colorMapSize + 7 ) / 8;
      if (cmap_bytes <= 4 &&
          fread (tga_cmap, info->colorMapLength * cmap_bytes, 1, fp) == 1)
        {
          if (info->colorMapSize == 32)
            bgr2rgb (gimp_cmap, tga_cmap, info->colorMapLength, cmap_bytes, 1);
          else if (info->colorMapSize == 24)
            bgr2rgb (gimp_cmap, tga_cmap, info->colorMapLength, cmap_bytes, 0);
          else if (info->colorMapSize == 16 || info->colorMapSize == 15)
            upsample (gimp_cmap, tga_cmap,
                      info->colorMapLength, cmap_bytes, info->alphaBits);

        }
      else
        {
          g_message ("File '%s' is truncated or corrupted",
                     gimp_filename_to_utf8 (filename));
          return -1;
        }
    }

  image_ID = gimp_image_new (info->width, info->height, itype);
  gimp_image_set_filename (image_ID, filename);

  if (info->colorMapType == 1)
    gimp_image_set_colormap(image_ID, gimp_cmap, info->colorMapLength);

  layer_ID = gimp_layer_new (image_ID,
                             _("Background"),
                             info->width, info->height,
                             dtype, 100,
                             GIMP_NORMAL_MODE);

  gimp_image_add_layer (image_ID, layer_ID, 0);

  drawable = gimp_drawable_get (layer_ID);

  /* Prepare the pixel region. */
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                        info->width, info->height, TRUE, FALSE);

  /* Allocate the data. */
  max_tileheight = gimp_tile_height ();
  data = g_new (guchar, info->width * max_tileheight * drawable->bpp);
  buffer = g_new (guchar, info->width * info->bytes);

  if (info->flipVert)
    {
      for (i = 0; i < info->height; i += tileheight)
        {
          tileheight = i ? max_tileheight : (info->height % max_tileheight);
          if (tileheight == 0) tileheight= max_tileheight;

          for (y= 1; y <= tileheight; ++y)
            {
              row= data + (info->width * drawable->bpp * (tileheight - y));
              read_line(fp, row, buffer, info, drawable);
            }

            gimp_progress_update ((double) (i + tileheight) /
                                  (double) info->height);
            gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0,
                                     info->height - i - tileheight,
                                     info->width, tileheight);
        }
    }
  else
    {
      for (i = 0; i < info->height; i += max_tileheight)
        {
          tileheight = MIN (max_tileheight, info->height - i);

          for (y= 0; y < tileheight; ++y)
            {
              row= data + (info->width * drawable->bpp * y);
              read_line(fp, row, buffer, info, drawable);
            }

            gimp_progress_update ((double) (i + tileheight) /
                                  (double) info->height);
            gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0, i,
                                     info->width, tileheight);
        }
    }

  g_free (data);
  g_free (buffer);

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  return image_ID;
}  /*read_image*/


static gint
save_image (const gchar *filename,
            gint32       image_ID,
            gint32       drawable_ID)
{
  GimpPixelRgn   pixel_rgn;
  GimpDrawable  *drawable;
  GimpImageType  dtype;
  gint           width;
  gint           height;

  FILE     *fp;
  gint      out_bpp = 0;
  gboolean  status  = TRUE;
  gint      i, row;

  guchar  header[18];
  guchar  footer[26];
  guchar *pixels;
  guchar *data;

  gint    num_colors;
  guchar *gimp_cmap = NULL;

  drawable = gimp_drawable_get (drawable_ID);
  dtype    = gimp_drawable_type (drawable_ID);

  width  = drawable->width;
  height = drawable->height;

  if ((fp = g_fopen (filename, "wb")) == NULL)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  gimp_progress_init_printf (_("Saving '%s'"),
                             gimp_filename_to_utf8 (filename));

  header[0] = 0; /* No image identifier / description */

  if (dtype == GIMP_INDEXED_IMAGE || dtype == GIMP_INDEXEDA_IMAGE)
    {
      gimp_cmap = gimp_image_get_colormap (image_ID, &num_colors);

      header[1] = 1; /* cmap type */
      header[2] = (tsvals.rle) ? 9 : 1;
      header[3] = header[4]= 0; /* no offset */
      header[5] = num_colors % 256;
      header[6] = num_colors / 256;
      header[7] = 24; /* cmap size / bits */
    }
  else
    {
      header[1]= 0;

      if (dtype == GIMP_RGB_IMAGE || dtype == GIMP_RGBA_IMAGE)
        {
          header[2]= (tsvals.rle) ? 10 : 2;
        }
      else
        {
          header[2]= (tsvals.rle) ? 11 : 3;
        }

      header[3] = header[4] = header[5] = header[6] = header[7] = 0;
    }

  header[8]  = header[9]  = 0; /* xorigin */
  header[10] = header[11] = 0; /* yorigin */

  header[12] = width % 256;
  header[13] = width / 256;

  header[14] = height % 256;
  header[15] = height / 256;

  switch (dtype)
    {
    case GIMP_INDEXED_IMAGE:
    case GIMP_GRAY_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      out_bpp = 1;
      header[16] = 8; /* bpp */
      header[17] = (tsvals.origin) ? 0 :  0x20; /* alpha + orientation */
      break;

    case GIMP_GRAYA_IMAGE:
      out_bpp = 2;
      header[16] = 16; /* bpp */
      header[17] = (tsvals.origin) ? 8 : 0x28; /* alpha + orientation */
      break;

    case GIMP_RGB_IMAGE:
      out_bpp = 3;
      header[16] = 24; /* bpp */
      header[17] = (tsvals.origin) ? 0 : 0x20; /* alpha + orientation */
      break;

    case GIMP_RGBA_IMAGE:
      out_bpp = 4;
      header[16] = 32; /* bpp */
      header[17] = (tsvals.origin) ? 8 : 0x28; /* alpha + orientation */
      break;
    }

  /* write header to front of file */
  fwrite (header, sizeof (header), 1, fp);

  if (dtype == GIMP_INDEXED_IMAGE || dtype == GIMP_INDEXEDA_IMAGE)
    {
      /* write out palette */
      for (i= 0; i < num_colors; ++i)
        {
          fputc (gimp_cmap[(i * 3) + 2], fp);
          fputc (gimp_cmap[(i * 3) + 1], fp);
          fputc (gimp_cmap[(i * 3) + 0], fp);
        }
    }

  gimp_tile_cache_ntiles ((width / gimp_tile_width ()) + 1);

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  pixels = g_new (guchar, width * drawable->bpp);
  data   = g_new (guchar, width * out_bpp);

  for (row = 0; row < height; ++row)
    {
      if (tsvals.origin)
        {
          gimp_pixel_rgn_get_row (&pixel_rgn, pixels, 0, height-(row+1), width);
        }
      else
        {
          gimp_pixel_rgn_get_row (&pixel_rgn, pixels, 0, row, width);
        }

      if (dtype == GIMP_RGB_IMAGE)
        {
          bgr2rgb (data, pixels, width, drawable->bpp, 0);
        }
      else if (dtype == GIMP_RGBA_IMAGE)
        {
          bgr2rgb (data, pixels, width, drawable->bpp, 1);
        }
      else if (dtype == GIMP_INDEXEDA_IMAGE)
        {
          for (i = 0; i < width; ++i)
            data[i]= pixels[i*2];
        }
      else
        {
          memcpy (data, pixels, width * drawable->bpp);
        }

      if (tsvals.rle)
        {
          rle_write (fp, data, width, out_bpp);
        }
      else
        {
          fwrite (data, width * out_bpp, 1, fp);
        }

      gimp_progress_update ((gdouble) row / (gdouble) height);
    }

  gimp_drawable_detach (drawable);
  g_free (data);

  /* footer must be the last thing written to file */
  memset (footer, 0, 8); /* No extensions, no developer directory */
  memcpy (footer + 8, magic, sizeof (magic)); /* magic signature */
  fwrite (footer, sizeof (footer), 1, fp);

  fclose (fp);

  return status;
}

static gboolean
save_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *toggle;
  GtkWidget *origin;
  GtkWidget *vbox;
  gboolean   run;

  dialog = gimp_dialog_new (_("Save as TGA"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, SAVE_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /*  rle  */
  toggle = gtk_check_button_new_with_mnemonic (_("_RLE compression"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), tsvals.rle);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &tsvals.rle);

  /*  origin  */
  origin = gtk_check_button_new_with_mnemonic (_("Or_igin at bottom left"));
  gtk_box_pack_start (GTK_BOX (vbox), origin, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (origin), tsvals.origin);
  gtk_widget_show (origin);

  g_signal_connect (origin, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &tsvals.origin);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
