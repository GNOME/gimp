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
 */

/*
 * TODO:
 *       Save preview
 *       Save layer masks
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
#define PLUG_IN_BINARY "psd_save"

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

static void   write_pixel_data     (FILE *fd, gint32 drawableID,
				    gint32 *ChanLenPosition,
				    glong rowlenOffset);


GimpPlugInInfo PLUG_IN_INFO =
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
  static GimpParamDef save_args[] =
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


static glong
pack_pb_line (guchar *start, glong length, glong stride,
	       guchar *dest_ptr)
{
  gint i,j;
  glong remaining;
  remaining = length;
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
    {
      char *chName = gimp_drawable_get_name (PSDImageData.lChannels[i]);
      write_string (fd, chName, "channel name");
      g_free (chName);
    }
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

  /* --------------- Write Guides --------------- */
  if (gimp_image_find_next_guide(image_id, 0))
    {
      gint n_guides = 0;
      gint guide_id =0;

      /* Count the guides */
      while ((guide_id = gimp_image_find_next_guide(image_id, guide_id)))
	n_guides++;

      xfwrite (fd, "8BIM", 4, "imageresources signature");
      write_gshort (fd, 0x0408, "0x0408 Id (Guides)");
      /* write_pascalstring (fd, Name, "Id name"); */
      write_gshort (fd, 0, "Id name"); /* Set to null string (two zeros) */
      write_glong (fd, 16 + 5 * n_guides, "0x0408 resource size");
      /* Save grid and guide header */
      write_glong (fd,   1, "grid/guide header version");
      write_glong (fd, 576, "grid custom spacing horizontal");/* dpi*32/4??*/
      write_glong (fd, 576, "grid custom spacing vertical");  /* dpi*32/4??*/
      write_glong (fd, n_guides, "number of guides");

      /* write the guides */
      while ((guide_id = gimp_image_find_next_guide(image_id, guide_id)))
        {
	  gchar orientation;
	  glong position;
	  orientation = gimp_image_get_guide_orientation(image_id, guide_id);
	  position    = 32 * gimp_image_get_guide_position(image_id, guide_id);
	  orientation ^= 1; /* in the psd vert =0 , horiz = 1 */
	  write_glong (fd, position, "Position of guide");
	  write_gchar (fd, orientation, "Orientation of guide");
	  n_guides--;
	}
      if ((ftell(fd) & 1))
	write_gchar(fd, 0, "pad byte");
      if (n_guides != 0)
	g_warning("Screwed up guide resource:: wrong number of guides\n");
      IFDBG printf ("      Total length of 0x0400 resource: %d\n", (int) sizeof (gshort));
    }

  /* --------------- Write resolution data ------------------- */
  {
    gdouble xres = 0, yres = 0;
    guint32 xres_fix, yres_fix;
    GimpUnit g_unit;
    gshort psd_unit;

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
    write_gshort(fd, 0x03ed, "0x03ed Id (resolution)");
    write_gshort (fd, 0, "Id name"); /* Set to null string (two zeros) */
    write_glong (fd, 16, "0x0400 resource size");
    write_glong (fd,  xres_fix, "hRes (16.16 fixed point)");
    write_gshort (fd, psd_unit, "hRes unit");
    write_gshort (fd, psd_unit, "width unit");
    write_glong (fd,  yres_fix, "vRes (16.16 fixed point)");
    write_gshort (fd, psd_unit, "vRes unit");
    write_gshort (fd, psd_unit, "height unit");
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
}



static int
get_compress_channel_data (guchar  *channel_data,
			   gint32   channel_cols,
			   gint32   channel_rows,
			   gint32   stride,
			   gshort  *LengthsTable,
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

  /*  return((len + channel_rows * sizeof (gshort)) + sizeof (gshort));*/
  return len;
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

          write_glong (fd, ChanSize, "Channel Size");
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
      if (gimp_layer_get_lock_alpha (PSDImageData.lLayers[i])) flags |= 1;
      if (! gimp_drawable_get_visible (PSDImageData.lLayers[i])) flags |= 2;
      IFDBG printf ("         Flags: %u\n", flags);
      write_gchar (fd, flags, "Flags");

      /* Padding byte to make the length even */
      write_gchar (fd, 0, "Filler");

      ExtraDataPos = ftell (fd); /* Position of Extra Data size */
      write_glong (fd, 0, "Extra data size");

#ifdef SAVELAYERMASK
      mask = gimp_layer_get_mask (PSDImageData.lLayers[i]);
      if (mask  >= 0)
        {
	  write_glong  (fd, 14,                        "Layer mask size");
	  write_glong  (fd, 0,                         "Layer mask top");
	  write_glong  (fd, 0,                         "Layer mask left");
	  write_glong  (fd, gimp_drawable_height(mask),"Layer mask bottom");
	  write_glong  (fd, gimp_drawable_width(mask), "Layer mask right");
	  write_gchar  (fd, 0,                         "lmask default color");
	  flags = (1                                        | /* relative */
		   (gimp_layer_mask_is_disabled(mask) << 1) | /* disabled?*/
		   (0 << 2)                                   /* invert   */);
	  write_gchar  (fd, flags,                      "layer mask flags");
	  write_gshort (fd, 0,                          "Layer mask Padding");
	}
      else
#else
      /* NOTE Writing empty Layer mask / adjustment layer data */
      write_glong (fd, 0, "Layer mask size");
      IFDBG printf ("\n         Layer mask size: %d\n", 0);
#endif

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
      write_pixel_data(fd, PSDImageData.lLayers[i], ChannelLengthPos[i], 0);
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
write_pixel_data (FILE *fd, gint32 drawableID, gint32 *ChanLenPosition,
		  glong ltable_offset)
{
  GimpPixelRgn region;      /* Image region */
  guchar *data;             /* Temporary copy of pixel data */

  gint32 tile_height = gimp_tile_height();

  GimpDrawable *drawable = gimp_drawable_get (drawableID);

  gint32 height = drawable->height;
  gint32 width  = drawable->width;
  gint32 bytes  = drawable->bpp;
  gint32 colors = bytes;    /* fixed up down below */
  gint32 y;

  glong   len;                  /* Length of compressed data */
  gshort *LengthsTable;         /* Lengths of every compressed row */
  guchar *rledata;              /* Compressed data from a region */
  gint32 length_table_pos;      /* position in file of the length table */
  int i, j;

  if ( gimp_drawable_has_alpha  (drawableID) &&
      !gimp_drawable_is_indexed (drawableID))
    colors -= 1;
  gimp_tile_cache_ntiles (2* (drawable->width / gimp_tile_width () + 1));

  LengthsTable = g_new (gshort, height);
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
	  write_gshort (fd, 1, "Compression type (RLE)");
	  len += 2;
	}

      if (ltable_offset > 0)
        {
	  length_table_pos = ltable_offset + 2 * chan * height;
	}
      else
        {
	  length_table_pos = ftell(fd);

	  xfwrite (fd, LengthsTable, height * sizeof(gshort),
		   "Dummy RLE length");
	  len += height * 2;
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
	}

      /* Write compressed lengths table */
      fseek (fd, length_table_pos, SEEK_SET);
      for (j = 0; j < height; j++) /* write real length table */
	write_gshort (fd, LengthsTable[j], "RLE length");

      if (ChanLenPosition)    /* Update total compressed length */
	{
	  fseek (fd, ChanLenPosition[i], SEEK_SET);
	  write_glong (fd, len, "channel data length");
	}
      fseek (fd, 0, SEEK_END);
    }

  gimp_drawable_detach (drawable);

  g_free (data);
  g_free (rledata);
  g_free (LengthsTable);
}



static void
save_data (FILE *fd, gint32 image_id)
{
  gint ChanCount;
  gint i, j;
  gint nChannel;
  gint32 imageHeight;                   /* Height of image */
  glong offset;                         /* offset in file of rle lengths */
  gint chan;
  gint32 bottom_layer;

  IFDBG printf ("\n Function: save_data\n");

  ChanCount = (PSDImageData.nChannels +
               nChansLayer (PSDImageData.baseType, 0));

  i = PSDImageData.nLayers - 1;  /* Channel to be written */
  IFDBG printf ("     Processing layer %d\n", i);

  imageHeight = gimp_image_height (image_id);

  nChannel = 0;


  write_gshort (fd, 1, "RLE compression");

  /* All line lengths go before the rle pixel data */

  offset = ftell(fd); /* Offset in file of line lengths */

  for (i = 0; i < ChanCount; i++)
    for (j = 0; j < imageHeight; j++)
      write_gshort (fd, 0, "junk line lengths");

  bottom_layer = PSDImageData.lLayers[PSDImageData.nLayers - 1];

  if (PSDImageData.nLayers != 1 ||
      gimp_drawable_width  (bottom_layer) != gimp_image_width  (image_id) ||
      gimp_drawable_height (bottom_layer) != gimp_image_height (image_id))
    {
      gint32 flat_image;
      gint32 flat_drawable;

      IFDBG printf ("\n     Creating flattened image\n");
      flat_image = gimp_image_duplicate (image_id);
      gimp_image_undo_disable (flat_image);
      flat_drawable = gimp_image_flatten (flat_image);

      IFDBG printf ("\n     Writing compressed flattened image data\n");
      write_pixel_data (fd, flat_drawable, NULL, offset);

      gimp_image_delete (flat_image);
    }
  else
    {
      IFDBG printf ("\n     Writing compressed image data\n");
      write_pixel_data (fd, PSDImageData.lLayers[PSDImageData.nLayers - 1],
			NULL, offset);
    }

  chan = nChansLayer (PSDImageData.baseType, 0);

  for (i = PSDImageData.nChannels - 1; i >= 0; i--)
    {
      IFDBG printf ("\n     Writing compressed channel data for channel %d\n",
		    i);
      write_pixel_data (fd, PSDImageData.lChannels[i], NULL,
			offset + 2*imageHeight*chan);
      chan++;
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
