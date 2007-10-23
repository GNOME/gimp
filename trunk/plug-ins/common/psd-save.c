/*
 * PSD Save Plugin version 1.0 (BETA)
 * This GIMP plug-in is designed to save Adobe Photoshop(tm) files (.PSD)
 *
 * Monigotes
 *
 *     If this plug-in fails to save a file which you think it should,
 *     please tell me what seemed to go wrong, and anything you know
 *     about the image you tried to save.  Please don't send big PSD
 *     files to me without asking first.
 *
 *          Copyright (C) 2000 Monigotes
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Adobe and Adobe Photoshop are trademarks of Adobe Systems
 * Incorporated that may be registered in certain jurisdictions.
 */

/*
 * Revision history:
 *
 *  2000.02 / v1.0 / Monigotes
 *       First version.
 *
 *  2003-05-10  Pedro Gimeno  <pggimeno@wanadoo.es>
 *       - Cleaned up and GNUstylized.
 *       - Translated all comments and vars in Spanish to English.
 *
 *  2005-2-11 Jay Cox <jaycox@gimp.org>
 *       Rewrote all the code that deals with pixels to be stingy with
 *       memory and opperate on tile-size chunks.  Create a flattened
 *       copy of the image when necessary. Fixes file corruption bug
 *       #167139 and memory bug #121871
 *
 *  2006-03-29 Guillermo S. Romero <gsr.bugs@infernal-iceberg.com>
 *       - Added/enabled basic support for layer masks based in psd.c
 *         and whatever was already here.
 *         Layers that are not canvas sized need investigation, here
 *         or in the import plugin something seems wrong.
 *       - Cosmetic changes about the debug messages, more like psd.c.
 */

/*
 * TODO:
 *       Save preview
 */

/*
 * BUGS:
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimp/stdplugins-intl.h"


/* *** DEFINES *** */

#define SAVE_PROC      "file-psd-save"
#define PLUG_IN_BINARY "psd-save"

/* set to TRUE if you want debugging, FALSE otherwise */
#define DEBUG FALSE

/* 1: Normal debuggin, 2: Deep debuggin */
#define DEBUG_LEVEL 1

#define IFDBG if (DEBUG)
#define IF_DEEP_DBG if (DEBUG && DEBUG_LEVEL == 2)


#define PSD_UNIT_INCH 1
#define PSD_UNIT_CM   2

/* *** END OF DEFINES *** */


/* Local types etc
 */


typedef struct PsdLayerDimension
{
  gint   left;
  gint   top;
  gint32 width;
  gint32 height;
} PSD_Layer_Dimension;


typedef struct PsdImageData
{
  gboolean             compression;

  gint32               image_height;
  gint32               image_width;

  GimpImageBaseType    baseType;

  gint                 nChannels;   /* Number of user channels in the image */
  gint32              *lChannels;   /* User channels in the image */

  gint                 nLayers;     /* Number of layers in the image */
  gint32              *lLayers;     /* Identifier of each layer */
  PSD_Layer_Dimension *layersDim;   /* Dimensions of each layer */

} PSD_Image_Data;

static PSD_Image_Data PSDImageData;


/* Declare some local functions.
 */

static void   query                (void);
static void   run                  (const gchar      *name,
                                    gint              nparams,
                                    const GimpParam  *param,
                                    gint             *nreturn_vals,
                                    GimpParam       **return_vals);

static void   psd_lmode_layer      (gint32         idLayer,
                                    gchar         *psdMode);
static void   reshuffle_cmap_write (guchar        *mapGimp);
static void   save_header          (FILE          *fd,
                                    gint32         image_id);
static void   save_color_mode_data (FILE          *fd,
                                    gint32         image_id);
static void   save_resources       (FILE          *fd,
                                    gint32         image_id);
static void   save_layer_and_mask  (FILE          *fd,
                                    gint32         image_id);
static void   save_data            (FILE          *fd,
                                    gint32         image_id);
static gint   save_image           (const gchar   *filename,
                                    gint32         image_id);
static void   xfwrite              (FILE          *fd,
                                    gconstpointer  buf,
                                    glong          len,
                                    const gchar   *why);
static void   write_pascalstring   (FILE          *fd,
                                    const gchar   *val,
                                    gint           padding,
                                    const gchar   *why);
static void   write_string         (FILE          *fd,
                                    const gchar   *val,
                                    const gchar   *why);
static void   write_gchar          (FILE          *fd,
                                    guchar         val,
                                    const gchar   *why);
static void   write_gint16         (FILE          *fd,
                                    gint16         val,
                                    const gchar   *why);
static void   write_gint32         (FILE          *fd,
                                    gint32         val,
                                    const gchar   *why);
static void   write_datablock_luni (FILE          *fd,
                                    const gchar   *val,
                                    const gchar   *why);

static void   write_pixel_data     (FILE          *fd,
                                    gint32         drawableID,
                                    glong         *ChanLenPosition,
                                    gint32         rowlenOffset);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN()


static void
query (void)
{
  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" },
    { GIMP_PDB_INT32,    "compression",  "Compression type: { NONE (0), LZW (1), PACKBITS (2)" },
    { GIMP_PDB_INT32,    "fill-order",   "Fill Order: { MSB to LSB (0), LSB to MSB (1)" }
  };

  gimp_install_procedure (SAVE_PROC,
                          "saves files in the Photoshop(tm) PSD file format",
                          "This filter saves files of Adobe Photoshop(tm) native PSD format.  These files may be of any image type supported by GIMP, with or without layers, layer masks, aux channels and guides.",
                          "Monigotes",
                          "Monigotes",
                          "2000",
                          N_("Photoshop image"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/x-psd");
  gimp_register_save_handler (SAVE_PROC, "psd", "");
}


static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam values[2];
  GimpRunMode      run_mode;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;

  if (strcmp (name, SAVE_PROC) == 0)
    {
      gint32           image_id;
      gint32           drawable_id;
      GimpExportReturn export = GIMP_EXPORT_IGNORE;

      IFDBG printf ("\n---------------- %s ----------------\n",
                    param[3].data.d_string);

      image_id    = param[1].data.d_int32;
      drawable_id = param[2].data.d_int32;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);
          export = gimp_export_image (&image_id, &drawable_id, "PSD",
                                      GIMP_EXPORT_CAN_HANDLE_RGB     |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                      GIMP_EXPORT_CAN_HANDLE_LAYERS  |
                                      GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }
          break;

        default:
          break;
        }

      if (save_image (param[3].data.d_string, image_id))
        values[0].data.d_status = GIMP_PDB_SUCCESS;
      else
        values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_id);
    }
}

static void
psd_lmode_layer (gint32  idLayer,
                 gchar  *psdMode)
{
  switch (gimp_layer_get_mode (idLayer))
    {
    case GIMP_NORMAL_MODE:
      strcpy (psdMode, "norm");
      break;
    case GIMP_DARKEN_ONLY_MODE:
      strcpy (psdMode, "dark");
      break;
    case GIMP_LIGHTEN_ONLY_MODE:
      strcpy (psdMode, "lite");
      break;
    case GIMP_HUE_MODE:
      strcpy (psdMode, "hue ");
      break;
    case GIMP_SATURATION_MODE:
      strcpy (psdMode, "sat ");
      break;
    case GIMP_COLOR_MODE:
      strcpy (psdMode, "colr");
      break;
    case GIMP_MULTIPLY_MODE:
      strcpy (psdMode, "mul ");
      break;
    case GIMP_SCREEN_MODE:
      strcpy (psdMode, "scrn");
      break;
    case GIMP_DISSOLVE_MODE:
      strcpy (psdMode, "diss");
      break;
    case GIMP_DIFFERENCE_MODE:
      strcpy (psdMode, "diff");
      break;
    case GIMP_VALUE_MODE:                  /* ? */
      strcpy (psdMode, "lum ");
      break;
    case GIMP_HARDLIGHT_MODE:
      strcpy (psdMode, "hLit");
      break;
    case GIMP_SOFTLIGHT_MODE:
      strcpy (psdMode, "sLit");
      break;
    case GIMP_OVERLAY_MODE:
      strcpy (psdMode, "over");
      break;
    default:
      {
        const gchar *nick = "?";

        gimp_enum_get_value (GIMP_TYPE_LAYER_MODE_EFFECTS,
                             gimp_layer_get_mode (idLayer),
                             NULL, &nick, NULL, NULL);

        g_message (_("Unable to save layer with mode '%s'.  Either the PSD "
                     "file format or the save plug-in does not support that, "
                     "using normal mode instead."), nick);

        IFDBG printf ("PSD: Warning - unsupported layer-blend mode: %s, "
                      "using normal mode\n", nick);

        strcpy (psdMode, "norm");
      }
      break;
    }
}


static void
write_string (FILE        *fd,
              const gchar *val,
              const gchar *why)
{
  write_gchar (fd, strlen (val), why);
  xfwrite (fd, val, strlen (val), why);
}


static void
write_pascalstring (FILE        *fd,
                    const gchar *val,
                    gint         padding,
                    const gchar *why)
{
  guchar len;
  gint   i;

  /* Calculate string length to write and limit it to 255 */

  len = (strlen (val) > 255) ? 255 : (guchar) strlen (val);

  /* Perform actual writing */

  if (len !=  0)
    {
      write_gchar (fd, len, why);
      xfwrite (fd, val, len, why);
    }
  else
    write_gint16 (fd, 0, why);

  /* If total length (length byte + content) is not a multiple of PADDING,
     add zeros to pad it.  */

  len++;           /* Add the length field */

  if ((len % padding) == 0)
    return;

  for (i = 0; i < (padding - (len % padding)); i++)
    write_gchar (fd, 0, why);
}


static void
xfwrite (FILE          *fd,
         gconstpointer  buf,
         glong          len,
         const gchar   *why)
{
  if (fwrite (buf, len, 1, fd) == 0)
    {
      IFDBG printf (" Function: xfwrite: Error while writing '%s'\n", why);
      gimp_quit ();
    }
}


static void
write_gchar (FILE        *fd,
             guchar       val,
             const gchar *why)
{
  guchar b[2];
  glong  pos;

  b[0] = val;
  b[1] = 0;

  pos = ftell (fd);
  if (fwrite (&b, 1, 2, fd) == 0)
    {
      IFDBG printf (" Function: write_gchar: Error while writing '%s'\n", why);
      gimp_quit ();
    }
  fseek (fd, pos + 1, SEEK_SET);
}


static void
write_gint16 (FILE        *fd,
              gint16       val,
              const gchar *why)
{
  guchar b[2];
  /*  b[0] = val & 255;
      b[1] = (val >> 8) & 255;*/

  b[1] = val & 255;
  b[0] = (val >> 8) & 255;

  if (fwrite (&b, 1, 2, fd) == 0)
    {
      IFDBG printf (" Function: write_gint16: Error while writing '%s'\n", why);
      gimp_quit ();
    }
}




static void
write_gint32 (FILE        *fd,
              gint32       val,
              const gchar *why)
{
  guchar b[4];

  b[3] = val & 255;
  b[2] = (val >> 8) & 255;
  b[1] = (val >> 16) & 255;
  b[0] = (val >> 24) & 255;

  if (fwrite (&b, 1, 4, fd) == 0)
    {
      IFDBG printf (" Function: write_gint32: Error while writing '%s'\n", why);
      gimp_quit ();
    }
}


static void
write_datablock_luni (FILE        *fd,
                      const gchar *val,
                      const gchar *why)
{
  if (val)
    {
      guint32    count;
      guint32    xdBlockSize;
      glong      numchars;
      gunichar2 *luniName;

      luniName = g_utf8_to_utf16 (val, -1, NULL, &numchars, NULL);

      if (luniName)
        {
          guchar len = MIN (numchars, 255);

          /* Only pad to even num of chars */
          if( len % 2 )
            xdBlockSize = len + 1;
          else
            xdBlockSize = len;

          /* 2 bytes / char + 4 bytes for pascal num chars */
          xdBlockSize = (xdBlockSize * 2) + 4;

          xfwrite (fd, "8BIMluni", 8, "luni xdb signature");
          write_gint32 (fd, xdBlockSize, "luni xdb size");
          write_gint32 (fd, len, "luni xdb pascal string");

          for (count = 0; count < len; count++)
            write_gint16 (fd, luniName[count], "luni xdb pascal string");

          /* Pad to an even number of chars */
          if (len % 2)
            write_gint16 (fd, 0x0000, "luni xdb pascal string padding");
        }
    }
}


static gint32
pack_pb_line (guchar *start,
              gint32  length,
              gint32  stride,
              guchar *dest_ptr)
{
  gint32  remaining = length;
  gint    i, j;

  length = 0;
  while (remaining > 0)
    {
      /* Look for characters matching the first */

      i = 0;
      while ((i < 128) &&
             (remaining - i > 0) &&
             (start[0] == start[i*stride]))
        i++;

      if (i > 1)              /* Match found */
        {

          *dest_ptr++ = -(i - 1);
          *dest_ptr++ = *start;

          start += i*stride;
          remaining -= i;
          length += 2;
        }
      else       /* Look for characters different from the previous */
        {
          i = 0;
          while ((i < 128)             &&
                 (remaining - (i + 1) > 0) &&
                 (start[i*stride] != start[(i + 1)*stride] ||
                  remaining - (i + 2) <= 0  || start[i*stride] != start[(i+2)*stride]))
            i++;

          /* If there's only 1 remaining, the previous WHILE stmt doesn't
             catch it */

          if (remaining == 1)
            {
              i = 1;
            }

          if (i > 0)               /* Some distinct ones found */
            {
              *dest_ptr++ = i - 1;
              for (j = 0; j < i; j++)
                {
                  *dest_ptr++ = start[j*stride];
                }
              start += i*stride;
              remaining -= i;
              length += i + 1;
            }

        }
    }
  return length;
}

static gint
gimpBaseTypeToPsdMode (GimpImageBaseType gimpBaseType)
{
  switch (gimpBaseType)
    {
    case GIMP_RGB:
      return 3;                                         /* RGB */
    case GIMP_GRAY:
      return 1;                                         /* Grayscale */
    case GIMP_INDEXED:
      return 2;                                         /* Indexed */
    default:
      g_message (_("Error: Can't convert GIMP base imagetype to PSD mode"));
      IFDBG printf ("PSD Save: gimpBaseType value is %d, can't convert to PSD mode", gimpBaseType);
      gimp_quit ();
      return 3;                            /* Return RGB by default */
    }
}


static gint
nChansLayer (gint gimpBaseType,
             gint hasAlpha,
             gint hasMask)
{
  gint incAlpha = 0;
  gint incMask = 0;

  incAlpha = (hasAlpha == 0) ? 0 : 1;
  incMask = (hasMask == 0) ? 0 : 1;

  switch (gimpBaseType)
    {
    case GIMP_RGB:
      return 3 + incAlpha + incMask;     /* R,G,B & Alpha & Mask (if any) */
    case GIMP_GRAY:
      return 1 + incAlpha + incMask;     /* G & Alpha & Mask (if any) */
    case GIMP_INDEXED:
      return 1 + incAlpha + incMask;     /* I & Alpha & Mask (if any) */
    default:
      return 0;                          /* Return 0 channels by default */
    }
}


static void
reshuffle_cmap_write (guchar *mapGimp)
{
  guchar *mapPSD;
  gint    i;

  mapPSD = g_malloc (768);

  for (i = 0; i < 256; i++)
    {
      mapPSD[i] = mapGimp[i * 3];
      mapPSD[i + 256] = mapGimp[i * 3 + 1];
      mapPSD[i + 512] = mapGimp[i * 3 + 2];
    }

  for (i = 0; i < 768; i++)
    {
      mapGimp[i] = mapPSD[i];
    }

  g_free (mapPSD);
}


static void
save_header (FILE   *fd,
             gint32  image_id)
{
  IFDBG printf (" Function: save_header\n");
  IFDBG printf ("\tRows: %d\n", PSDImageData.image_height);
  IFDBG printf ("\tColumns: %d\n", PSDImageData.image_width);
  IFDBG printf ("\tBase type: %d\n", PSDImageData.baseType);
  IFDBG printf ("\tNumber of channels: %d\n", PSDImageData.nChannels);

  xfwrite (fd, "8BPS", 4, "signature");
  write_gint16 (fd, 1, "version");
  write_gint32 (fd, 0, "reserved 1");      /* 6 for the 'reserved' field + 4 bytes for a long */
  write_gint16 (fd, 0, "reserved 1");      /* and 2 bytes for a short */
  write_gint16 (fd, (PSDImageData.nChannels +
                     nChansLayer (PSDImageData.baseType, 0, 0)),
                "channels");
  write_gint32 (fd, PSDImageData.image_height, "rows");
  write_gint32 (fd, PSDImageData.image_width, "columns");
  write_gint16 (fd, 8, "depth");  /* Apparently GIMP only supports 8 bit deep
                                     PSD images.  */
  write_gint16 (fd, gimpBaseTypeToPsdMode (PSDImageData.baseType), "mode");
}



static void
save_color_mode_data (FILE   *fd,
                      gint32  image_id)
{
  guchar *cmap;
  guchar *cmap_modified;
  gint    i;
  gint32  nColors;

  IFDBG printf (" Function: save_color_mode_data\n");

  switch (PSDImageData.baseType)
    {
    case GIMP_INDEXED:
      IFDBG printf ("\tImage type: INDEXED\n");

      cmap = gimp_image_get_colormap (image_id, &nColors);
      IFDBG printf ("\t\tLength of colormap returned by gimp_image_get_colormap: %d\n", nColors);

      if (nColors == 0)
        {
          IFDBG printf ("\t\tThe indexed image lacks a colormap\n");
          write_gint32 (fd, 0, "color data length");
        }
      else if (nColors != 256)
        {
          IFDBG printf ("\t\tThe indexed image has %d!=256 colors\n", nColors);
          IFDBG printf ("\t\tPadding with zeros up to 256\n");
          write_gint32 (fd, 768, "color data length");
            /* For this type, length is always 768 */

          cmap_modified = g_malloc (768);
          for (i = 0; i < nColors * 3; i++)
            cmap_modified[i] = cmap[i];

          for (i = nColors * 3; i < 768; i++)
            cmap_modified[i] = 0;

          reshuffle_cmap_write (cmap_modified);
          xfwrite (fd, cmap_modified, 768, "colormap");  /* Write readjusted colormap */

          g_free (cmap_modified);
        }
      else         /* nColors equals 256 */
        {
          write_gint32 (fd, 768, "color data length");   /* For this type, length is always 768 */
          reshuffle_cmap_write (cmap);
          xfwrite (fd, cmap, 768, "colormap");  /* Write readjusted colormap */
        }
      break;

    default:
      IFDBG printf ("\tImage type: Not INDEXED\n");
      write_gint32 (fd, 0, "color data length");
    }
}



static void
save_resources (FILE   *fd,
                gint32  image_id)
{
  gint       i;
  gchar     *fileName;            /* Image file name */
  gint32     idActLayer;          /* Id of the active layer */
  guint      nActiveLayer = 0;    /* Number of the active layer */
  gboolean   ActiveLayerPresent;  /* TRUE if there's an active layer */

  glong      eof_pos;             /* Position for End of file */
  glong      rsc_pos;             /* Position for Lengths of Resources section */
  glong      name_sec;            /* Position for Lengths of Channel Names */


  /* Only relevant resources in GIMP are: 0x03EE, 0x03F0 & 0x0400 */
  /* For Adobe Photoshop version 4.0 these can also be considered:
     0x0408, 0x040A & 0x040B (1006, 1008, 1024, 1032, 1034, and 1035) */

  IFDBG printf (" Function: save_resources\n");


  /* Get the image title from its filename */

  fileName = gimp_image_get_filename (image_id);
  IFDBG printf ("\tImage title: %s\n", fileName);

  /* Get the active layer number id */

  idActLayer = gimp_image_get_active_layer (image_id);
  IFDBG printf ("\tCurrent layer id: %d\n", idActLayer);

  ActiveLayerPresent = FALSE;
  for (i = 0; i < PSDImageData.nLayers; i++)
    if (idActLayer == PSDImageData.lLayers[i])
      {
        nActiveLayer = i;
        ActiveLayerPresent = TRUE;
      }

  if (ActiveLayerPresent)
    {
      IFDBG printf ("\t\tActive layer is number %d\n", nActiveLayer);
    }
  else
    {
      IFDBG printf ("\t\tNo active layer\n");
    }


  /* Here's where actual writing starts */

  rsc_pos = ftell (fd);
  write_gint32 (fd, 0, "image resources length");


  /* --------------- Write Channel names --------------- */

  if (PSDImageData.nChannels > 0)
    {
      xfwrite (fd, "8BIM", 4, "imageresources signature");
      write_gint16 (fd, 0x03EE, "0x03EE Id"); /* 1006 */
      /* write_pascalstring (fd, Name, "Id name"); */
      write_gint16 (fd, 0, "Id name"); /* Set to null string (two zeros) */

    /* Mark current position in the file */

    name_sec = ftell (fd);
    write_gint32 (fd, 0, "0x03EE resource size");

    /* Write all strings */

    for (i = PSDImageData.nChannels - 1; i >= 0; i--)
    {
      char *chName = gimp_drawable_get_name (PSDImageData.lChannels[i]);
      write_string (fd, chName, "channel name");
      g_free (chName);
    }
    /* Calculate and write actual resource's length */

    eof_pos = ftell (fd);

    fseek (fd, name_sec, SEEK_SET);
    write_gint32 (fd, eof_pos - name_sec - sizeof (gint32), "0x03EE resource size");
    IFDBG printf ("\tTotal length of 0x03EE resource: %d\n",
                  (int) (eof_pos - name_sec - sizeof (gint32)));

    /* Return to EOF to continue writing */

    fseek (fd, eof_pos, SEEK_SET);

    /* Pad if length is odd */

    if ((eof_pos - name_sec - sizeof (gint32)) & 1)
      write_gchar (fd, 0, "pad byte");
  }

  /* --------------- Write Guides --------------- */
  if (gimp_image_find_next_guide(image_id, 0))
    {
      gint n_guides = 0;
      gint guide_id =0;

      /* Count the guides */
      while ((guide_id = gimp_image_find_next_guide(image_id, guide_id)))
        n_guides++;

      xfwrite (fd, "8BIM", 4, "imageresources signature");
      write_gint16 (fd, 0x0408, "0x0408 Id (Guides)"); /* 1032 */
      /* write_pascalstring (fd, Name, "Id name"); */
      write_gint16 (fd, 0, "Id name"); /* Set to null string (two zeros) */
      write_gint32 (fd, 16 + 5 * n_guides, "0x0408 resource size");
      /* Save grid and guide header */
      write_gint32 (fd,   1, "grid/guide header version");
      write_gint32 (fd, 576, "grid custom spacing horizontal");/* dpi*32/4??*/
      write_gint32 (fd, 576, "grid custom spacing vertical");  /* dpi*32/4??*/
      write_gint32 (fd, n_guides, "number of guides");

      /* write the guides */
      while ((guide_id = gimp_image_find_next_guide(image_id, guide_id)))
        {
          gchar orientation;
          gint32 position;
          orientation = gimp_image_get_guide_orientation(image_id, guide_id);
          position    = 32 * gimp_image_get_guide_position(image_id, guide_id);
          orientation ^= 1; /* in the psd vert =0 , horiz = 1 */
          write_gint32 (fd, position, "Position of guide");
          write_gchar (fd, orientation, "Orientation of guide");
          n_guides--;
        }
      if ((ftell(fd) & 1))
        write_gchar(fd, 0, "pad byte");
      if (n_guides != 0)
        g_warning("Screwed up guide resource:: wrong number of guides\n");
      IFDBG printf ("\tTotal length of 0x0400 resource: %d\n", (int) sizeof (gint16));
    }

  /* --------------- Write resolution data ------------------- */
  {
    gdouble xres = 0, yres = 0;
    guint32 xres_fix, yres_fix;
    GimpUnit g_unit;
    gint16 psd_unit;

    g_unit = gimp_image_get_unit (image_id);
    gimp_image_get_resolution (image_id, &xres, &yres);

    if (g_unit == GIMP_UNIT_MM)
      {
        gdouble factor = gimp_unit_get_factor (g_unit) / 10.0;

        xres /= factor;
        yres /= factor;

        psd_unit = PSD_UNIT_CM;
      }
    else
      {
        psd_unit = PSD_UNIT_INCH;
      }

    xres_fix = xres * 65536.0 + .5; /* Convert to 16.16 fixed point */
    yres_fix = yres * 65536.0 + .5; /* Convert to 16.16 fixed point */

    xfwrite (fd, "8BIM", 4, "imageresources signature (for resolution)");
    write_gint16(fd, 0x03ed, "0x03ed Id (resolution)"); /* 1005 */
    write_gint16 (fd, 0, "Id name"); /* Set to null string (two zeros) */
    write_gint32 (fd, 16, "0x0400 resource size");
    write_gint32 (fd,  xres_fix, "hRes (16.16 fixed point)");
    write_gint16 (fd, psd_unit, "hRes unit");
    write_gint16 (fd, psd_unit, "width unit");
    write_gint32 (fd,  yres_fix, "vRes (16.16 fixed point)");
    write_gint16 (fd, psd_unit, "vRes unit");
    write_gint16 (fd, psd_unit, "height unit");
  }
  /* --------------- Write Active Layer Number --------------- */

  if (ActiveLayerPresent)
    {
      xfwrite (fd, "8BIM", 4, "imageresources signature");
      write_gint16 (fd, 0x0400, "0x0400 Id"); /* 1024 */
      /* write_pascalstring (fd, Name, "Id name"); */
      write_gint16 (fd, 0, "Id name"); /* Set to null string (two zeros) */
      write_gint32 (fd, sizeof (gint16), "0x0400 resource size");

      /* Save title as gint16 (length always even) */

      write_gint16 (fd, nActiveLayer, "active layer");

      IFDBG printf ("\tTotal length of 0x0400 resource: %d\n", (int) sizeof (gint16));
    }


  /* --------------- Write Total Section Length --------------- */

  eof_pos = ftell (fd);

  fseek (fd, rsc_pos, SEEK_SET);
  write_gint32 (fd, eof_pos - rsc_pos - sizeof (gint32), "image resources length");
  IFDBG printf ("\tResource section total length: %d\n",
                (int) (eof_pos - rsc_pos - sizeof (gint32)));

  /* Return to EOF to continue writing */

  fseek (fd, eof_pos, SEEK_SET);
}



static int
get_compress_channel_data (guchar  *channel_data,
                           gint32   channel_cols,
                           gint32   channel_rows,
                           gint32   stride,
                           gint16  *LengthsTable,
                           guchar  *remdata)
{
  gint    i;
  gint32  len;                 /* Length of compressed data */
  guchar *start;               /* Starting position of a row in channel_data */
  gint32  channel_length;      /* Total channel's length */

  channel_length = channel_cols * channel_rows;

  /* For every row in the channel */

  len = 0;
  for (i = 0; i < channel_rows; i++)
    {
      start = channel_data + (i * channel_cols * stride);

      /* Create packed data for this row */
      LengthsTable[i] = pack_pb_line (start, channel_cols, stride,
                                       &remdata[len]);
      len += LengthsTable[i];
    }

  /*  return((len + channel_rows * sizeof (gint16)) + sizeof (gint16));*/
  return len;
}


static void
save_layer_and_mask (FILE   *fd,
                     gint32  image_id)
{
  gint     i,j;
  gint     idChannel;
  gint     offset_x;                    /* X offset for each layer */
  gint     offset_y;                    /* Y offset for each layer */
  gint32   layerWidth;                  /* Width of each layer */
  gint32   layerHeight;                 /* Height of each layer */
  gchar    blendMode[5];                /* Blending mode of the layer */
  guchar   layerOpacity;                /* Opacity of the layer */
  guchar   flags;                       /* Layer flags */
  gint     nChannelsLayer;              /* Number of channels of a layer */
  gint32   ChanSize;                    /* Data length for a channel */
  gchar   *layerName;                   /* Layer name */
  gint     mask;                        /* Layer mask */

  glong    eof_pos;                     /* Position: End of file */
  glong    ExtraDataPos;                /* Position: Extra data length */
  glong    LayerMaskPos;                /* Position: Layer & Mask section length */
  glong    LayerInfoPos;                /* Position: Layer info section length*/
  glong  **ChannelLengthPos;            /* Position: Channel length */


  IFDBG printf (" Function: save_layer_and_mask\n");

  /* Create first array dimension (layers, channels) */

  ChannelLengthPos = g_new (glong *, PSDImageData.nLayers);

  /* Layer and mask information section */

  LayerMaskPos = ftell (fd);
  write_gint32 (fd, 0, "layers & mask information length");

  /* Layer info section */

  LayerInfoPos = ftell (fd);
  write_gint32 (fd, 0, "layers info section length");

  /* Layer structure section */

  write_gint16 (fd, PSDImageData.nLayers, "Layer structure count");

  /* Layer records section */
  /* GIMP layers must be written in reverse order */

  for (i = PSDImageData.nLayers - 1; i >= 0; i--)
    {
      gint hasMask = 0;

      gimp_drawable_offsets (PSDImageData.lLayers[i], &offset_x, &offset_y);
      layerWidth = gimp_drawable_width (PSDImageData.lLayers[i]);
      layerHeight = gimp_drawable_height (PSDImageData.lLayers[i]);

      PSDImageData.layersDim[i].left = offset_x;
      PSDImageData.layersDim[i].top = offset_y;
      PSDImageData.layersDim[i].width = layerWidth;
      PSDImageData.layersDim[i].height = layerHeight;

      IFDBG printf ("\tLayer number: %d\n", i);
      IFDBG printf ("\t\tX offset: %d\n", PSDImageData.layersDim[i].left);
      IFDBG printf ("\t\tY offset: %d\n", PSDImageData.layersDim[i].top);
      IFDBG printf ("\t\tWidth: %d\n", PSDImageData.layersDim[i].width);
      IFDBG printf ("\t\tHeight: %d\n", PSDImageData.layersDim[i].height);

      write_gint32 (fd, PSDImageData.layersDim[i].top, "Layer top");
      write_gint32 (fd, PSDImageData.layersDim[i].left, "Layer left");
      write_gint32 (fd, (PSDImageData.layersDim[i].height +
                        PSDImageData.layersDim[i].top), "Layer bottom");
      write_gint32 (fd, (PSDImageData.layersDim[i].width +
                        PSDImageData.layersDim[i].left), "Layer right");

      hasMask = (gimp_layer_get_mask(PSDImageData.lLayers[i]) == -1 ) ? 0 : 1;
      nChannelsLayer = nChansLayer (PSDImageData.baseType,
                                    gimp_drawable_has_alpha (PSDImageData.lLayers[i]),
                                    hasMask);


      write_gint16 (fd, nChannelsLayer, "Number channels in the layer");
      IFDBG printf ("\t\tNumber of channels: %d\n", nChannelsLayer);

      /* Create second array dimension (layers, channels) */

      ChannelLengthPos[i] = g_new (glong, nChannelsLayer);

      /* Try with gimp_drawable_bpp() */

      for (j = 0; j < nChannelsLayer; j++)
        {
          if (gimp_drawable_has_alpha (PSDImageData.lLayers[i]))
            idChannel = j - 1;
          else
            idChannel = j;
          if (hasMask && (j+1 == nChannelsLayer)) /* Last channel ... */
            idChannel = -2; /* ... will be layer mask */

          write_gint16 (fd, idChannel, "Channel ID");
          IFDBG printf ("\t\t\tChannel Identifier: %d\n", idChannel);

          /* Write the length assuming no compression.  In case there is,
             will modify it later when writing data.  */

          ChannelLengthPos[i][j] = ftell (fd);
          ChanSize = sizeof (gint16) + (PSDImageData.layersDim[i].width *
                                        PSDImageData.layersDim[i].height);

          write_gint32 (fd, ChanSize, "Channel Size");
          IFDBG printf ("\t\t\tLength: %d\n", ChanSize);
        }

      xfwrite (fd, "8BIM", 4, "blend mode signature");

      psd_lmode_layer (PSDImageData.lLayers[i], blendMode);
      IFDBG printf ("\t\tBlend mode: %s\n", blendMode);
      xfwrite (fd, blendMode, 4, "blend mode key");

      layerOpacity = (gimp_layer_get_opacity (PSDImageData.lLayers[i]) * 255.0) / 100.0;
      IFDBG printf ("\t\tOpacity: %u\n", layerOpacity);
      write_gchar (fd, layerOpacity, "Opacity");

      /* Apparently this field is not used in GIMP */
      write_gchar (fd, 0, "Clipping");

      flags = 0;
      if (gimp_layer_get_lock_alpha (PSDImageData.lLayers[i])) flags |= 1;
      if (! gimp_drawable_get_visible (PSDImageData.lLayers[i])) flags |= 2;
      IFDBG printf ("\t\tFlags: %u\n", flags);
      write_gchar (fd, flags, "Flags");

      /* Padding byte to make the length even */
      write_gchar (fd, 0, "Filler");

      ExtraDataPos = ftell (fd); /* Position of Extra Data size */
      write_gint32 (fd, 0, "Extra data size");

      mask = gimp_layer_get_mask (PSDImageData.lLayers[i]);
      if (mask  >= 0)
        {
          IFDBG printf ("\t\tLayer mask size: %d\n", 20);
          write_gint32  (fd, 20,                        "Layer mask size");
          write_gint32  (fd, 0,                         "Layer mask top");
          write_gint32  (fd, 0,                         "Layer mask left");
          write_gint32  (fd, gimp_drawable_height(mask),"Layer mask bottom");
          write_gint32  (fd, gimp_drawable_width(mask), "Layer mask right");
          write_gchar  (fd, 0,                         "lmask default color");
          flags = (1                                        | /* relative */
                   (gimp_layer_get_apply_mask(PSDImageData.lLayers[i]) << 1) | /* disabled?*/
                   (0 << 2)                                   /* invert   */);
          write_gchar  (fd, flags,                      "layer mask flags");
          write_gint16 (fd, 0,                          "Layer mask Padding");
        }
      else
        {
          /* NOTE Writing empty Layer mask / adjustment layer data */
          write_gint32 (fd, 0, "Layer mask size");
          IFDBG printf ("\t\tLayer mask size: %d\n", 0);
        }

      /* NOTE Writing empty Layer blending ranges data */
      write_gint32 (fd, 0, "Layer blending size");
      IFDBG printf ("\t\tLayer blending size: %d\n", 0);

      layerName = gimp_drawable_get_name (PSDImageData.lLayers[i]);
      write_pascalstring (fd, layerName, 4, "layer name");
      IFDBG printf ("\t\tLayer name: %s\n", layerName);

      write_datablock_luni(fd, layerName, "luni extra data block");

      /* Write real length for: Extra data */

      eof_pos = ftell (fd);

      fseek (fd, ExtraDataPos, SEEK_SET);
      write_gint32 (fd, eof_pos - ExtraDataPos - sizeof (gint32), "Extra data size");
      IFDBG printf ("\t\tExtraData size: %d\n",
                    (int) (eof_pos - ExtraDataPos - sizeof (gint32)));

      /* Return to EOF to continue writing */

      fseek (fd, eof_pos, SEEK_SET);
    }


  /* Channel image data section */
  /* Gimp layers must be written in reverse order */

  for (i = PSDImageData.nLayers - 1; i >= 0; i--)
    {
      IFDBG printf ("\t\tWriting pixel data for layer slot %d\n", i);
      write_pixel_data(fd, PSDImageData.lLayers[i], ChannelLengthPos[i], 0);
    }

  eof_pos = ftell (fd);

  /* Write actual size of Layer info section */

  fseek (fd, LayerInfoPos, SEEK_SET);
  write_gint32 (fd, eof_pos - LayerInfoPos - sizeof (gint32), "layers info section length");
  IFDBG printf ("\t\tTotal layers info section length: %d\n",
                (int) (eof_pos - LayerInfoPos - sizeof (gint32)));

  /* Write actual size of Layer and mask information secton */

  fseek (fd, LayerMaskPos, SEEK_SET);
  write_gint32 (fd, eof_pos - LayerMaskPos - sizeof (gint32), "layers & mask information length");
  IFDBG printf ("\t\tTotal layers & mask information length: %d\n",
                (int) (eof_pos - LayerMaskPos - sizeof (gint32)));

  /* Return to EOF to continue writing */

  fseek (fd, eof_pos, SEEK_SET);
}



static void
write_pixel_data (FILE   *fd,
                  gint32  drawableID,
                  glong  *ChanLenPosition,
                  gint32  ltable_offset)
{
  GimpPixelRgn region;      /* Image region */
  guchar *data;             /* Temporary copy of pixel data */

  gint32 tile_height = gimp_tile_height();

  GimpDrawable *drawable = gimp_drawable_get (drawableID);
  gint32 maskID = gimp_layer_get_mask(drawableID);

  gint32 height = drawable->height;
  gint32 width  = drawable->width;
  gint32 bytes  = drawable->bpp;
  gint32 colors = bytes;    /* fixed up down below */
  gint32 y;

  gint32   len;                  /* Length of compressed data */
  gint16 *LengthsTable;         /* Lengths of every compressed row */
  guchar *rledata;              /* Compressed data from a region */
  glong  length_table_pos;      /* position in file of the length table */
  int i, j;

  IFDBG printf (" Function: write_pixel_data, drw %d, lto %d\n",
                drawableID, ltable_offset);

  if ( gimp_drawable_has_alpha  (drawableID) &&
      !gimp_drawable_is_indexed (drawableID))
    colors -= 1;
  gimp_tile_cache_ntiles (2* (drawable->width / gimp_tile_width () + 1));

  LengthsTable = g_new (gint16, height);
  rledata = g_new (guchar, (MIN (height, tile_height) *
                            (width + 10 + (width / 100))));

  data = g_new (guchar, MIN(height, tile_height) * width * bytes);

  gimp_pixel_rgn_init (&region, drawable, 0, 0,
                       width, height, FALSE, FALSE);

  for (i = 0; i < bytes; i++)
    {
      int chan;
      len = 0;
      if (bytes != colors) /* Need to write alpha channel first */
        {
          if (i == 0)
            {
              if (ltable_offset > 0)
                continue;
              chan = bytes - 1;
            }
          else
            chan = i - 1;
        }
      else
        chan = i;
      if (ChanLenPosition)
        {
          write_gint16 (fd, 1, "Compression type (RLE)");
          len += 2;
        }

      if (ltable_offset > 0)
        {
          length_table_pos = ltable_offset + 2 * chan * height;
        }
      else
        {
          length_table_pos = ftell(fd);

          xfwrite (fd, LengthsTable, height * sizeof(gint16),
                   "Dummy RLE length");
          len += height * sizeof(gint16);
          IF_DEEP_DBG printf ("\t\t\t\t. ltable, pos %ld len %d\n", length_table_pos, len);
        }

      for (y = 0; y < height; y += tile_height)
        {
          int tlen;
            gimp_pixel_rgn_get_rect (&region, data, 0, y,
                                   width, MIN(height - y, tile_height));
          tlen = get_compress_channel_data (&data[chan],
                                             width,
                                             MIN(height - y, tile_height),
                                             bytes,
                                             &LengthsTable[y],
                                             rledata);
          len += tlen;
          xfwrite (fd, rledata, tlen, "Compressed pixel data");
          IF_DEEP_DBG printf ("\t\t\t\t. Writing compressed pixels, stream of %d\n", tlen);
        }

      /* Write compressed lengths table */
      fseek (fd, length_table_pos, SEEK_SET);
      for (j = 0; j < height; j++) /* write real length table */
        write_gint16 (fd, LengthsTable[j], "RLE length");

      if (ChanLenPosition)    /* Update total compressed length */
        {
          fseek (fd, ChanLenPosition[i], SEEK_SET);
          write_gint32 (fd, len, "channel data length");
          IFDBG printf ("\t\tUpdating data len to %d\n", len);
        }
      fseek (fd, 0, SEEK_END);
      IF_DEEP_DBG printf ("\t\t\t\t. Cur pos %ld\n", ftell(fd));
    }

  /* Write layer mask, as last channel, id -2 */
  if (maskID != -1) {
    GimpDrawable *mdrawable = gimp_drawable_get(maskID);
    len = 0;

    gimp_pixel_rgn_init (&region, mdrawable, 0, 0,
                         width, height, FALSE, FALSE);

    if (ChanLenPosition)
      {
        write_gint16 (fd, 1, "Compression type (RLE)");
        len += 2;
        IF_DEEP_DBG printf ("\t\t\t\t. ChanLenPos, len %d\n", len);
      }

    if (ltable_offset > 0)
      {
        length_table_pos = ltable_offset + 2 * (bytes+1) * height;
        IF_DEEP_DBG printf ("\t\t\t\t. ltable, pos %ld\n", length_table_pos);
      }
    else
      {
        length_table_pos = ftell(fd);

        xfwrite (fd, LengthsTable, height * sizeof(gint16),
                 "Dummy RLE length");
        len += height * sizeof(gint16);
        IF_DEEP_DBG printf ("\t\t\t\t. ltable, pos %ld len %d\n", length_table_pos, len);
      }

    for (y = 0; y < height; y += tile_height)
      {
        int tlen;
        gimp_pixel_rgn_get_rect (&region, data, 0, y,
                                 width, MIN(height - y, tile_height));
        tlen = get_compress_channel_data (&data[0],
                                          width,
                                          MIN(height - y, tile_height),
                                          1,
                                          &LengthsTable[y],
                                          rledata);
        len += tlen;
        xfwrite (fd, rledata, tlen, "Compressed mask data");
        IF_DEEP_DBG printf ("\t\t\t\t. Writing compressed mask, stream of %d\n", tlen);
      }

    /* Write compressed lengths table */
    fseek (fd, length_table_pos, SEEK_SET); /*POS WHERE???*/
    for (j = 0; j < height; j++) /* write real length table */
      {
        write_gint16 (fd, LengthsTable[j], "RLE length");
        IF_DEEP_DBG printf ("\t\t\t\t. Updating RLE len %d\n", LengthsTable[j]);
      }

    if (ChanLenPosition)    /* Update total compressed length */
      {
        fseek (fd, ChanLenPosition[bytes], SEEK_SET); /*+bytes OR SOMETHING*/
        write_gint32 (fd, len, "channel data length");
        IFDBG printf ("\t\tUpdating data len to %d, at %ld\n", len, ftell(fd));
      }
    fseek (fd, 0, SEEK_END);
    IF_DEEP_DBG printf ("\t\t\t\t. Cur pos %ld\n", ftell(fd));

    gimp_drawable_detach (mdrawable);
  }

  gimp_drawable_detach (drawable);

  g_free (data);
  g_free (rledata);
  g_free (LengthsTable);
}



static void
save_data (FILE   *fd,
           gint32  image_id)
{
  gint ChanCount;
  gint i, j;
  gint nChannel;
  gint32 imageHeight;                   /* Height of image */
  glong offset;                         /* offset in file of rle lengths */
  gint chan;
  gint32 bottom_layer;

  IFDBG printf (" Function: save_data\n");

  ChanCount = (PSDImageData.nChannels +
               nChansLayer (PSDImageData.baseType, 0, 0));

  i = PSDImageData.nLayers - 1;  /* Layers to be written */
  IFDBG printf ("\tProcessing %d layers\n", i);

  imageHeight = gimp_image_height (image_id);

  nChannel = 0;


  write_gint16 (fd, 1, "RLE compression");

  /* All line lengths go before the rle pixel data */

  offset = ftell(fd); /* Offset in file of line lengths */

  for (i = 0; i < ChanCount; i++)
    for (j = 0; j < imageHeight; j++)
      write_gint16 (fd, 0, "junk line lengths");

  bottom_layer = PSDImageData.lLayers[PSDImageData.nLayers - 1];

  if (PSDImageData.nLayers != 1 ||
      gimp_drawable_width  (bottom_layer) != gimp_image_width  (image_id) ||
      gimp_drawable_height (bottom_layer) != gimp_image_height (image_id))
    {
      gint32 flat_image;
      gint32 flat_drawable;

      IFDBG printf ("\t\tCreating flattened image\n");
      flat_image = gimp_image_duplicate (image_id);
      gimp_image_undo_disable (flat_image);
      flat_drawable = gimp_image_flatten (flat_image);

      /* gimp_image_flatten() may fail if there are no visible layers */
      if (flat_drawable != -1)
        {
          IFDBG printf ("\t\tWriting compressed flattened image data\n");
          write_pixel_data (fd, flat_drawable, NULL, offset);
        }

      gimp_image_delete (flat_image);
    }
  else
    {
      IFDBG printf ("\t\tWriting compressed image data\n");
      write_pixel_data (fd, PSDImageData.lLayers[PSDImageData.nLayers - 1],
                        NULL, offset);
    }

  chan = nChansLayer (PSDImageData.baseType, 0, 0);

  for (i = PSDImageData.nChannels - 1; i >= 0; i--)
    {
      IFDBG printf ("\t\tWriting compressed channel data for channel %d\n",
                    i);
      write_pixel_data (fd, PSDImageData.lChannels[i], NULL,
                        offset + 2*imageHeight*chan);
      chan++;
    }
}



static void
get_image_data (FILE   *fd,
                gint32  image_id)
{
  IFDBG printf (" Function: get_image_data\n");

  PSDImageData.compression = FALSE;

  PSDImageData.image_height = gimp_image_height (image_id);
  IFDBG printf ("\tGot number of rows: %d\n", PSDImageData.image_height);

  PSDImageData.image_width = gimp_image_width (image_id);
  IFDBG printf ("\tGot number of cols: %d\n", PSDImageData.image_width);

  PSDImageData.baseType = gimp_image_base_type (image_id);
  IFDBG printf ("\tGot base type: %d\n", PSDImageData.baseType);

  /* PSD format does not support indexed layered images */

  if (PSDImageData.baseType == GIMP_INDEXED)
    {
      IFDBG printf ("\tFlattening indexed image\n");
      gimp_image_flatten (image_id);
    }

  PSDImageData.lChannels = gimp_image_get_channels (image_id, &PSDImageData.nChannels);
  IFDBG printf ("\tGot number of channels: %d\n", PSDImageData.nChannels);

  PSDImageData.lLayers = gimp_image_get_layers (image_id, &PSDImageData.nLayers);
  IFDBG printf ("\tGot number of layers: %d\n", PSDImageData.nLayers);

  PSDImageData.layersDim = g_new (PSD_Layer_Dimension, PSDImageData.nLayers);
}



gint
save_image (const gchar *filename,
            gint32       image_id)
{
  FILE   *fd;
  gint32 *layers;
  gint    nlayers;
  gint    i;
  GimpDrawable *drawable;

  IFDBG printf (" Function: save_image\n");

  if (gimp_image_width (image_id) > 30000 ||
      gimp_image_height(image_id) > 30000)
  {
      g_message (_("Unable to save '%s'.  The psd file format does not support images that are more than 30000 pixels wide or tall."),
                 gimp_filename_to_utf8 (filename));
      return FALSE;
    }

 /* Need to check each of the layers size individually also */
  layers = gimp_image_get_layers (image_id, &nlayers);
  for (i = 0; i < nlayers; i++)
    {
      drawable = gimp_drawable_get (layers[i]);
      if (drawable->width > 30000 || drawable->height > 30000)
        {
          g_message (_("Unable to save '%s'.  The psd file format does not support images with layers that are more than 30000 pixels wide or tall."),
                     gimp_filename_to_utf8 (filename));
          g_free (layers);
          return FALSE;
        }
    }
  g_free (layers);


  fd = g_fopen (filename, "wb");
  if (fd == NULL)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  gimp_progress_init_printf (_("Saving '%s'"),
                             gimp_filename_to_utf8 (filename));

  IFDBG g_print ("\tFile '%s' has been opened\n",
                 gimp_filename_to_utf8 (filename));

  get_image_data (fd, image_id);
  save_header (fd, image_id);
  save_color_mode_data (fd, image_id);
  save_resources (fd, image_id);

  /* PSD format does not support layers in indexed images */

  if (PSDImageData.baseType == GIMP_INDEXED)
    write_gint32 (fd, 0, "layers info section length");
  else
    save_layer_and_mask (fd, image_id);

  /* If this is an indexed image, write now channel and layer info */

  save_data (fd, image_id);

  IFDBG printf ("----- Closing PSD file, done -----\n\n");

  fclose (fd);
  return TRUE;
}
