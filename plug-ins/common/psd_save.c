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
 */

/*
 * TODO:
 */

/*
 * BUGS:
 */

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>

#include <gtk/gtk.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimp/stdplugins-intl.h"


/* *** DEFINES *** */

/* set to TRUE if you want debugging, FALSE otherwise */
#define DEBUG FALSE

/* 1: Normal debuggin, 2: Deep debuggin */
#define DEBUG_LEVEL 1

#define IFDBG if (DEBUG)
#define IF_DEEP_DBG if (DEBUG && DEBUG_LEVEL == 2)

/* *** END OF DEFINES *** */


/* Local types etc
 */


typedef struct PsdLayerDimension
{
  gint left;
  gint top;
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

static void * xmalloc              (size_t n);
static void   psd_lmode_layer      (gint32 idLayer, gchar* psdMode);
static void   reshuffle_cmap_write (guchar *mapGimp);
static void   save_header          (FILE *fd, gint32 image_id);
static void   save_color_mode_data (FILE *fd, gint32 image_id);
static void   save_resources       (FILE *fd, gint32 image_id);
static void   save_layer_and_mask  (FILE *fd, gint32 image_id);
static void   save_data            (FILE *fd, gint32 image_id);
static gint   save_image           (const gchar *filename, gint32 image_id);
static void   xfwrite              (FILE *fd, void *buf, long len, gchar *why);
static void   write_pascalstring   (FILE *fd, char *val, gint padding,
                                    gchar *why);
static void   write_string         (FILE *fd, char *val, gchar *why);
static void   write_gchar          (FILE *fd, unsigned char val, gchar *why);
static void   write_gshort         (FILE *fd, gshort val, gchar *why);
static void   write_glong          (FILE *fd, glong val, gchar *why);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static const gchar *prog_name = "PSD";

MAIN()


static void
query (void)
{
  static GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Drawable to save" },
    { GIMP_PDB_STRING, "filename", "The name of the file to save the image in" },
    { GIMP_PDB_STRING, "raw_filename", "The name of the file to save the image in" },
    { GIMP_PDB_INT32, "compression", "Compression type: { NONE (0), LZW (1), PACKBITS (2)" },
    { GIMP_PDB_INT32, "fillorder", "Fill Order: { MSB to LSB (0), LSB to MSB (1)" }
  };

  gimp_install_procedure ("file_psd_save",
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

  gimp_register_file_handler_mime ("file_psd_save", "image/x-psd");
  gimp_register_save_handler ("file_psd_save", "psd", "");
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

  if (strcmp (name, "file_psd_save") == 0)
    {
      gint32           image_id;
      gint32           drawable_id;
      GimpExportReturn export = GIMP_EXPORT_IGNORE;

      IFDBG printf ("\n---------------- %s ----------------\n", param[3].data.d_string);

      image_id    = param[1].data.d_int32;
      drawable_id = param[2].data.d_int32;

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init ("psd_save", FALSE);
          export = gimp_export_image (&image_id, &drawable_id, "PSD",
                                      GIMP_EXPORT_CAN_HANDLE_RGB     |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                      GIMP_EXPORT_CAN_HANDLE_LAYERS);

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



static void *
xmalloc (size_t n)
{
  void *p;

  if (n == 0)
    {
      IFDBG printf ("PSD: WARNING: %s: xmalloc asked for zero-sized chunk\n", prog_name);

      return (NULL);
    }

  if ((p = g_malloc (n)) != NULL)
    return p;

  IFDBG printf ("%s: out of memory\n", prog_name);
  gimp_quit ();
  return NULL;
}

static void
psd_lmode_layer (gint32 idLayer, gchar *psdMode)
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
    case GIMP_OVERLAY_MODE:                /* ? */
      strcpy (psdMode, "over");
      break;
/*    case GIMP_BEHIND_MODE:                 These are from GIMP 1.1.14*/
/*    case GIMP_DIVIDE_MODE:                 These are from GIMP 1.1.14*/
    case GIMP_ADDITION_MODE:
    case GIMP_SUBTRACT_MODE:
      IFDBG printf ("PSD: Warning - unsupported layer-blend mode: %c, using 'norm' mode\n",
                    gimp_layer_get_mode (idLayer));
      strcpy (psdMode, "norm");
      break;
    default:
      IFDBG printf ("PSD: Warning - UNKNOWN layer-blend mode, reverting to 'norm'\n");
      strcpy (psdMode, "norm");
      break;
    }
}


static void
write_string (FILE *fd, char *val, gchar *why)
{
  write_gchar (fd, strlen (val), why);
  xfwrite (fd, val, strlen (val), why);
}


static void
write_pascalstring (FILE *fd, char *val, gint padding, gchar *why)
{
  unsigned char len;
  gint i;

  /* Calculate string length to write and limit it to 255 */

  len = (strlen (val) > 255) ? 255 : (unsigned char) strlen (val);

  /* Perform actual writing */

  if (len !=  0)
    {
      write_gchar (fd, len, why);
      xfwrite (fd, val, len, why);
    }
  else
    write_gshort (fd, 0, why);

  /* If total length (length byte + content) is not a multiple of PADDING,
     add zeros to pad it.  */

  len++;           /* Add the length field */

  if ((len % padding) == 0)
    return;

  for (i = 0; i < (padding - (len % padding)); i++)
    write_gchar (fd, 0, why);
}


static void
xfwrite (FILE *fd, void *buf, long len, gchar *why)
{
  if (fwrite (buf, len, 1, fd) == 0)
    {
      IFDBG printf (" Function: xfwrite: Error while writing '%s'\n", why);
      gimp_quit ();
    }
}




static void
write_gchar (FILE *fd, unsigned char val, gchar *why)
{
  unsigned char b[2];
  gint32 pos;

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
write_gshort (FILE *fd, gshort val, gchar *why)
{
  unsigned char b[2];
  /*  b[0] = val & 255;
      b[1] = (val >> 8) & 255;*/

  b[1] = val & 255;
  b[0] = (val >> 8) & 255;

  if (fwrite (&b, 1, 2, fd) == 0)
    {
      IFDBG printf (" Function: write_gshort: Error while writing '%s'\n", why);
      gimp_quit ();
    }
}




static void
write_glong (FILE *fd, glong val, gchar *why)
{
  unsigned char b[4];
  /*  b[0] = val & 255;
      b[1] = (val >> 8) & 255;
      b[2] = (val >> 16) & 255;
      b[3] = (val >> 24) & 255;*/

  b[3] = val & 255;
  b[2] = (val >> 8) & 255;
  b[1] = (val >> 16) & 255;
  b[0] = (val >> 24) & 255;

  if (fwrite (&b, 1, 4, fd) == 0)
    {
      IFDBG printf (" Function: write_glong: Error while writing '%s'\n", why);
      gimp_quit ();
    }
}


static void
pack_pb_line (guchar *start, guchar *end,
              guchar *dest_ptr, gshort *length)
{
  gint i,j;
  gint32 remaining;

  remaining = end - start;
  *length = 0;

  while (remaining > 0)
    {
      /* Look for characters matching the first */

      i = 0;
      while ((i < 128) &&
             (start + i < end) &&
             (start[0] == start[i]))
        i++;

      if (i > 1)              /* Match found */
        {
          IF_DEEP_DBG printf ("Repetition: '%d', %d times ------------> ", *start, i);
          IF_DEEP_DBG printf ("Writing: '%d' %d\n", -(i - 1), *start);

          *dest_ptr++ = -(i - 1);
          *dest_ptr++ = *start;

          start += i;
          remaining -= i;
          *length += 2;
        }
      else       /* Look for characters different from the previous */
        {
          i = 0;
          while ((i < 128) &&
                 (start + i + 1 <= end) &&
                 (start[i] != start[i + 1] || 
		  start + i + 2 >= end || start[i] != start[i+2]))
            i++;

          /* If there's only 1 remaining, the previous WHILE stmt doesn't
             catch it */

          if (remaining == 1)
            {
              IF_DEEP_DBG printf ("1 Remaining:\t");
              i = 1;
            }

          if (i > 0)               /* Some distinct ones found */
            {
              IF_DEEP_DBG printf ("%d distinct              ------------> Writing: '%d' ", i, i - 1);

              *dest_ptr++ = i - 1;
              for (j = 0; j < i; j++)
                {
                  IF_DEEP_DBG printf ("%d ", start[j]);
                  *dest_ptr++ = start[j];
                }

              IF_DEEP_DBG printf ("\n");

              start += i;
              remaining -= i;
              *length += i + 1;
            }

        }

      IF_DEEP_DBG printf ("Remaining: %d\n", remaining);
    }

  IF_DEEP_DBG printf ("\nTotal length: %d\n", *length);

/*  if (*length & 1)        // length is odd, NOP added (128)
    {
      *length += 1;
      *dest_ptr++ = 128;

      IF_DEEP_DBG printf ("Total modified length: %d\n", *length);
    } */
}


static void
GRAYA_to_chans (guchar *greyA, gint numpix, guchar **grey, guchar **alpha)
{
  gint i;
  gint nPix;

  if (greyA == NULL)
    {
      IFDBG printf ("greyA is a null channel");
      *grey = NULL;
      *alpha = NULL;
      return;
    }

  nPix = numpix / 2;
  *grey = xmalloc (nPix);
  *alpha = xmalloc (nPix);

  for (i = 0; i < nPix; i++)
    {
      (*grey)[i] = greyA[i * 2];
      (*alpha)[i] = greyA[i * 2 + 1];
    }
}


static void
RGB_to_chans (guchar *rgb, gint numpix,
              guchar **red, guchar **green, guchar **blue)
{
  gint i;
  gint nPix;

  if (rgb == NULL)
    {
      IFDBG printf ("rgb is a null channel");
      *red = NULL;
      *green = NULL;
      *blue = NULL;
      return;
    }

  nPix = numpix / 3;
  *red = xmalloc (nPix);
  *green = xmalloc (nPix);
  *blue = xmalloc (nPix);

  for (i = 0; i < nPix; i++)
    {
      (*red)[i] = rgb[i * 3];
      (*green)[i] = rgb[i * 3 + 1];
      (*blue)[i] = rgb[i * 3 + 2];
    }
}


static void
RGBA_to_chans (guchar *rgbA, gint numpix,
               guchar **red, guchar **green, guchar **blue,
               guchar **alpha)
{
  gint i;
  gint nPix;

  if (rgbA == NULL)
    {
      IFDBG printf ("rgb is a null channel");
      *red = NULL;
      *green = NULL;
      *blue = NULL;
      *alpha = NULL;
      return;
    }

  nPix = numpix / 4;
  *red = xmalloc (nPix);
  *green = xmalloc (nPix);
  *blue = xmalloc (nPix);
  *alpha = xmalloc (nPix);

  for (i = 0; i < nPix; i++)
    {
      (*red)[i] = rgbA[i * 4];
      (*green)[i] = rgbA[i * 4 + 1];
      (*blue)[i] = rgbA[i * 4 + 2];
      (*alpha)[i] = rgbA[i * 4 + 3];
    }
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
      g_message ("Error: Can't convert GIMP base imagetype to PSD mode");
      IFDBG printf ("PSD Save: gimpBaseType value is %d, can't convert to PSD mode", gimpBaseType);
      gimp_quit ();
      return 3;                            /* Return RGB by default */
    }
}


static gint
nChansLayer (gint gimpBaseType, gint hasAlpha)
{
  gint incAlpha = 0;

  incAlpha = (hasAlpha == 0) ? 0 : 1;

  switch (gimpBaseType)
    {
    case GIMP_RGB:
      return 3 + incAlpha;               /* R,G,B & Alpha (if any) */
    case GIMP_GRAY:
      return 1 + incAlpha;               /* G & Alpha (if any) */
    case GIMP_INDEXED:
      return 1 + incAlpha;               /* I & Alpha (if any) */
    default:
      return 0;                          /* Return 0 channels by default */
    }
}


static void
reshuffle_cmap_write (guchar *mapGimp)
{
  guchar *mapPSD;
  gint i;

  mapPSD = xmalloc (768);

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
save_header (FILE *fd, gint32 image_id)
{
  IFDBG printf (" Function: save_header\n");
  IFDBG printf ("      Rows: %d\n", PSDImageData.image_height);
  IFDBG printf ("      Columns: %d\n", PSDImageData.image_width);
  IFDBG printf ("      Base type: %d\n", PSDImageData.baseType);
  IFDBG printf ("      Number of channels: %d\n", PSDImageData.nChannels);

  xfwrite (fd, "8BPS", 4, "signature");
  write_gshort (fd, 1, "version");
  write_glong (fd, 0, "reserved 1");       /* 6 for the 'reserved' field + 4 bytes for a long */
  write_gshort (fd, 0, "reserved 1");      /* and 2 bytes for a short */
  write_gshort (fd, (PSDImageData.nChannels +
                     nChansLayer (PSDImageData.baseType, 0)),
                "channels");
  write_glong (fd, PSDImageData.image_height, "rows");
  write_glong (fd, PSDImageData.image_width, "columns");
  write_gshort (fd, 8, "depth");  /* Apparently GIMP only supports 8 bit deep
                                     PSD images.  */
  write_gshort (fd, gimpBaseTypeToPsdMode (PSDImageData.baseType), "mode");
}



static void
save_color_mode_data (FILE *fd, gint32 image_id)
{
  guchar *cmap;
  guchar *cmap_modified;
  gint     i;
  gint32  nColors;

  IFDBG printf (" Function: save_color_mode_data\n");

  switch (PSDImageData.baseType)
    {
    case GIMP_INDEXED:
      IFDBG printf ("      Image type: INDEXED\n");

      cmap = gimp_image_get_colormap (image_id, &nColors);
      IFDBG printf ("      Length of colormap returned by gimp_image_get_colormap: %d\n", nColors);

      if (nColors == 0)
        {
          IFDBG printf ("      The indexed image lacks a colormap\n");
          write_glong (fd, 0, "color data length");
        }
      else if (nColors != 256)
        {
          IFDBG printf ("      The indexed image has %d!=256 colors\n", nColors);
          IFDBG printf ("      Padding with zeros up to 256\n");
          write_glong (fd, 768, "color data length");
            /* For this type, length is always 768 */

          cmap_modified = xmalloc (768);
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
          write_glong (fd, 768, "color data length");   /* For this type, length is always 768 */
          reshuffle_cmap_write (cmap);
          xfwrite (fd, cmap, 768, "colormap");  /* Write readjusted colormap */
        }
      break;

    default:
      IFDBG printf ("      Image type: Not INDEXED\n");
      write_glong (fd, 0, "color data length");
    }
}



static void
save_resources (FILE *fd, gint32 image_id)
{
  gint i;
  gchar    **chName = NULL;       /* Channel names */
  gchar     *fileName;            /* Image file name */
  gint32     idActLayer;          /* Id of the active layer */
  guint      nActiveLayer = 0;    /* Number of the active layer */
  gboolean   ActiveLayerPresent;  /* TRUE if there's an active layer */

  gint32     eof_pos;             /* Position for End of file */
  gint32     rsc_pos;             /* Position for Lengths of Resources section */
  gint32     name_sec;            /* Position for Lengths of Channel Names */


  /* Only relevant resources in GIMP are: 0x03EE, 0x03F0 & 0x0400 */
  /* For Adobe Photoshop version 4.0 these can also be considered:
     0x0408, 0x040A & 0x040B */

  IFDBG printf (" Function: save_resources\n");

  /* Get channel names */

  if (PSDImageData.nChannels > 0)
    chName = (char **) xmalloc(sizeof (char *) * PSDImageData.nChannels);

  for (i = 0; i < PSDImageData.nChannels; i++)
    {
      chName[i] = gimp_drawable_get_name (PSDImageData.lChannels[i]);
      IFDBG printf ("      Channel %d name: %s\n", i, chName[i]);
    }

  /* Get the image title from its filename */

  fileName = gimp_image_get_filename (image_id);
  IFDBG printf ("      Image title: %s\n", fileName);

  /* Get the active layer number id */

  idActLayer = gimp_image_get_active_layer (image_id);
  IFDBG printf ("      Current layer id: %d\n", idActLayer);

  ActiveLayerPresent = FALSE;
  for (i = 0; i < PSDImageData.nLayers; i++)
    if (idActLayer == PSDImageData.lLayers[i])
      {
        nActiveLayer = i;
        ActiveLayerPresent = TRUE;
      }

  if (ActiveLayerPresent)
    {
      IFDBG printf ("      Active layer number is: %d\n", nActiveLayer);
    }
  else
    {
      IFDBG printf ("      No active layer\n");
    }


  /* Here's where actual writing starts */

  rsc_pos = ftell (fd);
  write_glong (fd, 0, "image resources length");


  /* --------------- Write Channel names --------------- */

  if (PSDImageData.nChannels > 0)
    {
      xfwrite (fd, "8BIM", 4, "imageresources signature");
      write_gshort (fd, 0x03EE, "0x03EE Id");
      /* write_pascalstring (fd, Name, "Id name"); */
      write_gshort (fd, 0, "Id name"); /* Set to null string (two zeros) */

    /* Mark current position in the file */

    name_sec = ftell (fd);
    write_glong (fd, 0, "0x03EE resource size");

    /* Write all strings */

    for (i = PSDImageData.nChannels - 1; i >= 0; i--)
      /*write_pascalstring (fd, chName[i], 2, "chanel name"); */
      write_string (fd, chName[i], "channel name");

    /* Calculate and write actual resource's length */

    eof_pos = ftell (fd);

    fseek (fd, name_sec, SEEK_SET);
    write_glong (fd, eof_pos - name_sec - sizeof (glong), "0x03EE resource size");
    IFDBG printf ("\n      Total length of 0x03EE resource: %d\n",
                  (int) (eof_pos - name_sec - sizeof (glong)));

    /* Return to EOF to continue writing */

    fseek (fd, eof_pos, SEEK_SET);

    /* Pad if length is odd */

    if ((eof_pos - name_sec - sizeof (glong)) & 1)
      write_gchar (fd, 0, "pad byte");
  }


  /* --------------- Write Active Layer Number --------------- */

  if (ActiveLayerPresent)
    {
      xfwrite (fd, "8BIM", 4, "imageresources signature");
      write_gshort (fd, 0x0400, "0x0400 Id");
      /* write_pascalstring (fd, Name, "Id name"); */
      write_gshort (fd, 0, "Id name"); /* Set to null string (two zeros) */
      write_glong (fd, sizeof (gshort), "0x0400 resource size");

      /* Save title as gshort (length always even) */

      write_gshort (fd, nActiveLayer, "active layer");

      IFDBG printf ("      Total length of 0x0400 resource: %d\n", (int) sizeof (gshort));
    }


  /* --------------- Write Total Section Length --------------- */

  eof_pos = ftell (fd);

  fseek (fd, rsc_pos, SEEK_SET);
  write_glong (fd, eof_pos - rsc_pos - sizeof (glong), "image resources length");
  IFDBG printf ("      Resource section total length: %d\n",
                (int) (eof_pos - rsc_pos - sizeof (glong)));

  /* Return to EOF to continue writing */

  fseek (fd, eof_pos, SEEK_SET);

  g_free (chName);
}


static void
get_compress_channel_data (guchar  *channel_data,
                           gint32   channel_cols,
                           gint32   channel_rows,
                           gshort **LengthsTable,
                           guchar **remdata,
                           glong   *TotalCompressedLen)
{
  gint    i;
  gint32  len;                  /* Length of compressed data */
  gshort  rowlen;               /* Length of current row */
  guchar *start;                /* Starting position of a row in channel_data */
  guchar *end;                  /* End position of a row in channel_data */
  gint32  channel_length;       /* Total channel's length */


  channel_length = channel_cols * channel_rows;
  *remdata = g_new (guchar, channel_length * 2);
  *LengthsTable = g_new (gshort, channel_rows);

  /* For every row in the channel */

  len = 0;
  for (i = 0; i < channel_rows; i++)
    {
      start = channel_data + (i * channel_cols);
      end = start + channel_cols;

      /* Create packed data for this row */
      pack_pb_line (start, end, (*remdata) + len, &rowlen);
      (*LengthsTable)[i] = rowlen;
      len += rowlen;
    }

  *TotalCompressedLen = ((len + channel_rows * sizeof (gshort))
                         + sizeof (gshort));
}


static void
save_channel_data (FILE *fd,
                   guchar *channel_data,
                   gint32 channel_cols,
                   gint32 channel_rows,
                   gint32 posLong,
                   gchar *why)
{
  gint    i;
  gint32  len;                  /* Length of compressed data */
  glong   TotalRawLen;          /* Total length of raw data */
  glong   TotalCompressedLen;   /* Total length of compressed data */
  gshort *LengthsTable;         /* Lengths of every compressed row */
  gshort  rowlen;               /* Length of row currently being handled */
  guchar *remdata;              /* Compressed data from a row */
  guchar *start;                /* Start position of a row in channel_data */
  guchar *end;                  /* End position of a row in channel_data */
  gint32  channel_length;       /* Total channel's length */


  channel_length = channel_cols * channel_rows;
  remdata = g_new (guchar, channel_length * 2);
  LengthsTable = g_new (gshort, channel_rows);

  /* For every row in the channel */

  len = 0;
  for (i = 0; i < channel_rows; i++)
    {
      start = channel_data + (i * channel_cols);
      end = start + channel_cols;

      /* Create compressed data for this row */
      pack_pb_line (start, end, remdata + len, &rowlen);
      LengthsTable[i] = rowlen;
      len += rowlen;
    }

  /* Calculate total lengths of both kinds */

  TotalRawLen = (channel_rows * channel_cols) + sizeof (gshort);
  TotalCompressedLen = ((len + channel_rows * sizeof (gshort))
                        + sizeof (gshort));

/*  IFDBG printf ("\nCompressed length: %ld\n", TotalCompressedLen);
  IFDBG printf ("\nRaw length: %ld\n", TotalRawLen); */

  if (TotalCompressedLen < TotalRawLen)
    {
      IFDBG printf ("        Saving data (RLE): %ld\n", TotalCompressedLen);

      write_gshort (fd, 1, "Compression"); /* Write compression type */

      /* Write compressed lengths table */

      for (i = 0; i < channel_rows; i++)
        write_gshort (fd, LengthsTable[i], "RLE length");

      xfwrite (fd, remdata, len, why); /* Write compressed data */

      /* Update total compressed length */

      fseek (fd, posLong, SEEK_SET);
      write_glong (fd, TotalCompressedLen, "channel data length");
      fseek (fd, 0, SEEK_END);
    }
  else
    {
      IFDBG printf ("        Write raw data: %ld\n", TotalRawLen);

      write_gshort (fd, 0, "Compression"); /* Save compression type */

      xfwrite (fd, channel_data, channel_length, why); /* Save raw data */
    }
  g_free (remdata);
  g_free (LengthsTable);
}


static void
save_layer_and_mask (FILE *fd, gint32 image_id)
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

  gint32   eof_pos;                     /* Position: End of file */
  gint32   ExtraDataPos;                /* Position: Extra data length */
  gint32   LayerMaskPos;                /* Position: Layer & Mask section length */
  gint32   LayerInfoPos;                /* Position: Layer info section length*/
  gint32 **ChannelLengthPos;            /* Position: Channel length */


  IFDBG printf (" Function: save_layer&mask\n");

  /* Create first array dimension (layers, channels) */

  ChannelLengthPos = g_new (gint32 *, PSDImageData.nLayers);

  /* Layer and mask information section */

  LayerMaskPos = ftell (fd);
  write_glong (fd, 0, "layers & mask information length");

  /* Layer info section */

  LayerInfoPos = ftell (fd);
  write_glong (fd, 0, "layers info section length");

  /* Layer structure section */

  write_gshort (fd, PSDImageData.nLayers, "Layer structure count");

  /* Layer records section */
  /* GIMP layers must be written in reverse order */

  for (i = PSDImageData.nLayers - 1; i >= 0; i--)
    {
      gimp_drawable_offsets (PSDImageData.lLayers[i], &offset_x, &offset_y);
      layerWidth = gimp_drawable_width (PSDImageData.lLayers[i]);
      layerHeight = gimp_drawable_height (PSDImageData.lLayers[i]);

      PSDImageData.layersDim[i].left = offset_x;
      PSDImageData.layersDim[i].top = offset_y;
      PSDImageData.layersDim[i].width = layerWidth;
      PSDImageData.layersDim[i].height = layerHeight;

      IFDBG printf ("      Layer number: %d\n", i);
      IFDBG printf ("         Offset x: %d\n", PSDImageData.layersDim[i].left);
      IFDBG printf ("         Offset y: %d\n", PSDImageData.layersDim[i].top);
      IFDBG printf ("         Width: %d\n", PSDImageData.layersDim[i].width);
      IFDBG printf ("         Height: %d\n", PSDImageData.layersDim[i].height);

      write_glong (fd, PSDImageData.layersDim[i].top, "Layer top");
      write_glong (fd, PSDImageData.layersDim[i].left, "Layer left");
      write_glong (fd, (PSDImageData.layersDim[i].height +
                        PSDImageData.layersDim[i].top), "Layer bottom");
      write_glong (fd, (PSDImageData.layersDim[i].width +
                        PSDImageData.layersDim[i].left), "Layer right");

      nChannelsLayer = nChansLayer (PSDImageData.baseType,
                                    gimp_drawable_has_alpha (PSDImageData.lLayers[i]));


      write_gshort (fd, nChannelsLayer, "Number channels in the layer");
      IFDBG printf ("         Number of channels: %d\n", nChannelsLayer);

      /* Create second array dimension (layers, channels) */

      ChannelLengthPos[i] = g_new (gint32, nChannelsLayer);

      /* Try with gimp_drawable_bpp() */

      for (j = 0; j < nChannelsLayer; j++)
        {
          if (gimp_drawable_has_alpha (PSDImageData.lLayers[i]))
            idChannel = j - 1;
          else
            idChannel = j;

          write_gshort (fd, idChannel, "Channel ID");
          IFDBG printf ("           - Identifier: %d\n", idChannel);

          /* Write the length assuming no compression.  In case there is,
             will modify it later when writing data.  */

          ChannelLengthPos[i][j] = ftell (fd);
          ChanSize = sizeof (gshort) + (PSDImageData.layersDim[i].width *
                                        PSDImageData.layersDim[i].height);

          write_glong (fd, ChanSize, "Channel ID");
          IFDBG printf ("             Length: %d\n", ChanSize);
        }

      xfwrite (fd, "8BIM", 4, "blend mode signature");

      psd_lmode_layer (PSDImageData.lLayers[i], blendMode);
      IFDBG printf ("         Blend mode: %s\n", blendMode);
      xfwrite (fd, blendMode, 4, "blend mode key");

      layerOpacity = (gimp_layer_get_opacity (PSDImageData.lLayers[i]) * 255.0) / 100.0;
      IFDBG printf ("         Opacity: %u\n", layerOpacity);
      write_gchar (fd, layerOpacity, "Opacity");

      /* Apparently this field is not used in GIMP */
      write_gchar (fd, 0, "Clipping");

      flags = 0;
      if (gimp_layer_get_preserve_trans (PSDImageData.lLayers[i])) flags |= 1;
      if (! gimp_drawable_get_visible (PSDImageData.lLayers[i])) flags |= 2;
      IFDBG printf ("         Flags: %u\n", flags);
      write_gchar (fd, flags, "Flags");

      /* Padding byte to make the length even */
      write_gchar (fd, 0, "Filler");

      ExtraDataPos = ftell (fd); /* Position of Extra Data size */
      write_glong (fd, 0, "Extra data size");

      /* NOTE Writing empty Layer mask / adjustment layer data */
      write_glong (fd, 0, "Layer mask size");
      IFDBG printf ("\n         Layer mask size: %d\n", 0);

      /* NOTE Writing empty Layer blending ranges data */
      write_glong (fd, 0, "Layer blending size");
      IFDBG printf ("\n         Layer blending size: %d\n", 0);

      layerName = gimp_drawable_get_name (PSDImageData.lLayers[i]);
      write_pascalstring (fd, layerName, 4, "layer name");
      IFDBG printf ("\n         Layer name: %s\n", layerName);

      /* Write real length for: Extra data */

      eof_pos = ftell (fd);

      fseek (fd, ExtraDataPos, SEEK_SET);
      write_glong (fd, eof_pos - ExtraDataPos - sizeof (glong), "Extra data size");
      IFDBG printf ("      ExtraData size: %d\n",
                    (int) (eof_pos - ExtraDataPos - sizeof (glong)));

      /* Return to EOF to continue writing */

      fseek (fd, eof_pos, SEEK_SET);
    }


  /* Channel image data section */
  /* Gimp layers must be written in reverse order */

  for (i = PSDImageData.nLayers - 1; i >= 0; i--)
    {
      gint nChannel;
      GimpDrawable *drawable;
      GimpPixelRgn region;      /* Image region */
      guchar *data;             /* Full layer data including all channels */
      guchar *red;              /* R channel data */
      guchar *green;            /* G channel data */
      guchar *blue;             /* B channel data */
      guchar *gray;             /* Gray channel data */
      guchar *alpha;            /* Alpha channel data */
      gint32 ChanSize;          /* Length of channel data */

      IFDBG printf ("\n     Channels image data. Layer: %d\n", i);

      ChanSize = (PSDImageData.layersDim[i].width *
                  PSDImageData.layersDim[i].height);
      nChannelsLayer = nChansLayer (PSDImageData.baseType,
                                    gimp_drawable_has_alpha (PSDImageData.lLayers[i]));
      data = g_new (guchar, ChanSize * nChannelsLayer);

      drawable = gimp_drawable_get (PSDImageData.lLayers[i]);

      gimp_pixel_rgn_init (&region, drawable, 0, 0,
                           PSDImageData.layersDim[i].width,
                           PSDImageData.layersDim[i].height, FALSE, FALSE);

      gimp_pixel_rgn_get_rect (&region, data, 0, 0,
                               PSDImageData.layersDim[i].width,
                               PSDImageData.layersDim[i].height);

      IFDBG printf ("        Channeel size: %d\n", ChanSize);

      nChannel = 0;
      switch (PSDImageData.baseType)
        {
        case GIMP_RGB:

          if (gimp_drawable_has_alpha (PSDImageData.lLayers[i]))
            {
              RGBA_to_chans (data, ChanSize * nChannelsLayer, &red, &green, &blue, &alpha);
              IFDBG printf ("        Writing alpha channel...\n");

              save_channel_data (fd, alpha, PSDImageData.layersDim[i].width,
                                 PSDImageData.layersDim[i].height,
                                 ChannelLengthPos[i][nChannel++],
                                 "alpha channel");
            }
          else
            RGB_to_chans (data, ChanSize * nChannelsLayer, &red, &green, &blue);

          IFDBG printf ("        Writing red channel...\n");
          save_channel_data (fd, red,PSDImageData.layersDim[i].width,
                             PSDImageData.layersDim[i].height,
                             ChannelLengthPos[i][nChannel++],
                             "red channel");

          IFDBG printf ("        Writing green channel...\n");
          save_channel_data (fd, green, PSDImageData.layersDim[i].width,
                             PSDImageData.layersDim[i].height,
                             ChannelLengthPos[i][nChannel++],
                             "green channel");

          IFDBG printf ("        Writing blue channel...\n");
          save_channel_data (fd, blue, PSDImageData.layersDim[i].width,
                             PSDImageData.layersDim[i].height,
                             ChannelLengthPos[i][nChannel++],
                             "blue channel");
          break;

        case GIMP_GRAY:

          if (gimp_drawable_has_alpha (PSDImageData.lLayers[i]))
            {
              GRAYA_to_chans (data, ChanSize * nChannelsLayer, &gray, &alpha);

              IFDBG printf ("        Writing alpha channel...\n");
              save_channel_data (fd, alpha, PSDImageData.layersDim[i].width,
                                 PSDImageData.layersDim[i].height,
                                 ChannelLengthPos[i][nChannel++],
                                 "alpha channel");

              IFDBG printf ("        Writing gray channel...\n");
              save_channel_data (fd, gray, PSDImageData.layersDim[i].width,
                                 PSDImageData.layersDim[i].height,
                                 ChannelLengthPos[i][nChannel++],
                                 "gray channel");
            }
          else
            {
              IFDBG printf ("        Writing gray channel...\n");
              save_channel_data (fd, data, PSDImageData.layersDim[i].width,
                                 PSDImageData.layersDim[i].height,
                                 ChannelLengthPos[i][nChannel++],
                                 "gray channel");
            }

          break;

        case GIMP_INDEXED:
          IFDBG printf ("        Writing indexed channel...\n");
          save_channel_data (fd, data, PSDImageData.layersDim[i].width,
                             PSDImageData.layersDim[i].height,
                             ChannelLengthPos[i][nChannel++],
                             "indexed channel");
          break;
        }


    }


  eof_pos = ftell (fd);

  /* Write actual size of Layer info section */

  fseek (fd, LayerInfoPos, SEEK_SET);
  write_glong (fd, eof_pos - LayerInfoPos - sizeof (glong), "layers info section length");
  IFDBG printf ("\n      Total layers info section length: %d\n",
                (int) (eof_pos - LayerInfoPos - sizeof (glong)));

  /* Write actual size of Layer and mask information secton */

  fseek (fd, LayerMaskPos, SEEK_SET);
  write_glong (fd, eof_pos - LayerMaskPos - sizeof (glong), "layers & mask information length");
  IFDBG printf ("      Total layers & mask information length: %d\n",
                (int) (eof_pos - LayerMaskPos - sizeof (glong)));

  /* Return to EOF to continue writing */

  fseek (fd, eof_pos, SEEK_SET);
}



static void
save_data (FILE *fd, gint32 image_id)
{
  gint ChanCount;
  gint i, j;
  gint nChannel;
  gint offset_x;                        /* X offset for each layer */
  gint offset_y;                        /* Y offset for each layer */
  gint32 layerWidth;                    /* Width of each layer */
  gint32 layerHeight;                   /* Height of each layer */
  GimpDrawable *drawable;
  GimpPixelRgn region;                  /* Image region */
  guchar *data;                         /* Full layer data including all channels */
  guchar *red;                          /* R channel data */
  guchar *green;                        /* G channel data */
  guchar *blue;                         /* B channel data */
  guchar *gray_indexed = NULL;          /* Gray/indexed channel data */
  gint32 ChanSize;                      /* Length of channel data */
  gint nChannelsLayer;                  /* Number of channels in a layer */

  gshort **TLdataCompress;
  guchar **dataCompress;
  glong   *CompressDataSize;
  glong    TotalSize;


  IFDBG printf ("\n Function: save_data\n");

  ChanCount = (PSDImageData.nChannels +
               nChansLayer (PSDImageData.baseType, 0));
  TLdataCompress = g_new (gshort *, ChanCount);
  dataCompress = g_new (guchar *, ChanCount);
  CompressDataSize = g_new (glong, ChanCount);

  i = PSDImageData.nLayers - 1;  /* Channel to be written */
  IFDBG printf ("     Processing layer %d\n", i);

  gimp_drawable_offsets (PSDImageData.lLayers[i], &offset_x, &offset_y);
  layerHeight = gimp_drawable_height (PSDImageData.lLayers[i]);
  layerWidth = gimp_drawable_width (PSDImageData.lLayers[i]);

  ChanSize = layerWidth * layerHeight;
  nChannelsLayer = nChansLayer (PSDImageData.baseType,
                                gimp_drawable_has_alpha (PSDImageData.lLayers[i]));
  data = g_new (guchar, ChanSize * nChannelsLayer);

  drawable = gimp_drawable_get (PSDImageData.lLayers[i]);
  gimp_pixel_rgn_init (&region, drawable, 0, 0, layerWidth, layerHeight, FALSE, FALSE);
  gimp_pixel_rgn_get_rect (&region, data, 0, 0, layerWidth, layerHeight);

  nChannel = 0;
  switch (PSDImageData.baseType)
    {
    case GIMP_RGB:
      RGB_to_chans (data, ChanSize * nChannelsLayer, &red, &green, &blue);

      get_compress_channel_data (red, layerWidth, layerHeight,
                                 &(TLdataCompress[nChannel]), &(dataCompress[nChannel]),
                                 &(CompressDataSize[nChannel]));
      IFDBG printf ("        Compressed length of red channel: %ld\n",
                    CompressDataSize[nChannel]);
      nChannel++;

      get_compress_channel_data (green, layerWidth, layerHeight,
                                 &(TLdataCompress[nChannel]), &(dataCompress[nChannel]),
                                 &(CompressDataSize[nChannel]));
      IFDBG printf ("        Compressed length of green channel: %ld\n",
                    CompressDataSize[nChannel]);
      nChannel++;

      get_compress_channel_data (blue, layerWidth, layerHeight,
                                 &(TLdataCompress[nChannel]), &(dataCompress[nChannel]),
                                 &(CompressDataSize[nChannel]));
      IFDBG printf ("        Compressed length of blue channel: %ld\n",
                    CompressDataSize[nChannel]);
      nChannel++;
      break;

    case GIMP_GRAY:
    case GIMP_INDEXED:
      gray_indexed = data;
      get_compress_channel_data (gray_indexed, layerWidth, layerHeight,
                                 &(TLdataCompress[nChannel]), &(dataCompress[nChannel]),
                                 &(CompressDataSize[nChannel]));
      IFDBG printf ("        Compressed length for gray or indexed channel: %ld\n",
                    CompressDataSize[nChannel]);
      nChannel++;
      break;
    }


  for (i = PSDImageData.nChannels - 1; i >= 0; i--)
    {
      ChanSize = (PSDImageData.image_width *
                  PSDImageData.image_height);

      drawable = gimp_drawable_get (PSDImageData.lChannels[i]);

      gimp_pixel_rgn_init (&region, drawable, 0, 0,
                           PSDImageData.image_width,
                           PSDImageData.image_height, FALSE, FALSE);

      gimp_pixel_rgn_get_rect (&region, data, 0, 0,
                               PSDImageData.image_width,
                               PSDImageData.image_height);

      get_compress_channel_data (data, layerWidth, layerHeight,
                                 &(TLdataCompress[nChannel]), &(dataCompress[nChannel]),
                                 &(CompressDataSize[nChannel]));
      IFDBG printf ("     Compressed length of user-defined channel: %ld\n",
                    CompressDataSize[nChannel]);
      nChannel++;
    }

  /* Calculate length of all compressed channels */

  TotalSize = 0;
  for (i = 0; i < ChanCount; i++)
    TotalSize += CompressDataSize[i];

  IFDBG printf ("\n     Total size of compressed data: %ld\n", TotalSize);
  IFDBG printf ("     Total size of raw data: %d\n", ChanCount * ChanSize);

  /* Decide whether to write compressed or raw data */

  if (TotalSize < ChanCount * ChanSize)  /* Write compressed data */
    {
      IFDBG printf ("\n     Writing compressed data\n");
      write_gshort (fd, 1, "RLE compression");

      /* Write all line lengths first */

      for (i = 0; i < ChanCount; i++)
        for (j = 0; j < layerHeight; j++)
          write_gshort (fd, TLdataCompress[i][j], "line lengths");

      /* Now compressed data */

      for (i = 0; i < ChanCount; i++)
        {
          TotalSize = 0;
          for (j = 0; j < layerHeight; j++)
            TotalSize += TLdataCompress[i][j];

          xfwrite (fd, dataCompress[i], TotalSize, "channel data");
        }
    }
  else   /* Write raw */
    {
      IFDBG printf ("\n     Writing raw data\n");
      write_gshort (fd, 0, "RLE compression");

      switch (PSDImageData.baseType)
        {
        case GIMP_RGB:
          xfwrite (fd, red, ChanSize, "red channel data");
          xfwrite (fd, green, ChanSize, "green channel data");
          xfwrite (fd, blue, ChanSize, "blue channel data");
          break;

        case GIMP_GRAY:
        case GIMP_INDEXED:
          xfwrite (fd, gray_indexed, ChanSize, "gray or indexed channel data");
          break;
        }

      /* Now for user channels */

      for (i = PSDImageData.nChannels - 1; i >= 0; i--)
        {
          drawable = gimp_drawable_get (PSDImageData.lChannels[i]);

          gimp_pixel_rgn_init (&region, drawable, 0, 0,
                               PSDImageData.image_width,
                               PSDImageData.image_height, FALSE, FALSE);

          gimp_pixel_rgn_get_rect (&region, data, 0, 0,
                                   PSDImageData.image_width,
                                   PSDImageData.image_height);

          xfwrite (fd, data, ChanSize, "channel data");
        }
    }
}



static void
get_image_data (FILE *fd, gint32 image_id)
{
  IFDBG printf (" Function: get_image_data\n");

  PSDImageData.compression = FALSE;

  PSDImageData.image_height = gimp_image_height (image_id);
  IFDBG printf ("      Got number of rows: %d\n", PSDImageData.image_height);

  PSDImageData.image_width = gimp_image_width (image_id);
  IFDBG printf ("      Got number of cols: %d\n", PSDImageData.image_width);

  PSDImageData.baseType = gimp_image_base_type (image_id);
  IFDBG printf ("      Got base type: %d\n", PSDImageData.baseType);

  /* PSD format does not support indexed layered images */

  if (PSDImageData.baseType == GIMP_INDEXED)
    {
      IFDBG printf ("      Flattening indexed image\n");
      gimp_image_flatten (image_id);
    }

  PSDImageData.lChannels = gimp_image_get_channels (image_id, &PSDImageData.nChannels);
  IFDBG printf ("      Got number of channels: %d\n", PSDImageData.nChannels);

  PSDImageData.lLayers = gimp_image_get_layers (image_id, &PSDImageData.nLayers);
  IFDBG printf ("      Got number of layers: %d\n", PSDImageData.nLayers);

  PSDImageData.layersDim = g_new (PSD_Layer_Dimension, PSDImageData.nLayers);
}



gint
save_image (const gchar *filename,
            gint32       image_id)
{
  FILE  *fd;
  gchar *name_buf;
  gint32 *layers;
  int nlayers;
  int i;
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


  fd = fopen (filename, "wb");
  if (fd == NULL)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  name_buf = g_strdup_printf (_("Saving '%s'..."),
                              gimp_filename_to_utf8 (filename));
  gimp_progress_init (name_buf);
  g_free (name_buf);

  IFDBG g_print ("      File \"%s\" has been opened\n",
                 gimp_filename_to_utf8 (filename));

  get_image_data (fd, image_id);
  save_header (fd, image_id);
  save_color_mode_data (fd, image_id);
  save_resources (fd, image_id);

  /* PSD format does not support layers in indexed images */

  if (PSDImageData.baseType == GIMP_INDEXED)
    write_glong (fd, 0, "layers info section length");
  else
    save_layer_and_mask (fd, image_id);

  /* If this is an indexed image, write now channel and layer info */

  save_data (fd, image_id);

  IFDBG printf ("\n\n");

  fclose (fd);
  return TRUE;
}
