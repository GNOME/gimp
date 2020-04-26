/*
 * PSD Export Plugin version 1.0 (BETA)
 * This GIMP plug-in is designed to export Adobe Photoshop(tm) files (.PSD)
 *
 * Monigotes
 *
 *     If this plug-in fails to export a file which you think it should,
 *     please tell me what seemed to go wrong, and anything you know
 *     about the image you tried to export.  Please don't send big PSD
 *     files to me without asking first.
 *
 *          Copyright (C) 2000 Monigotes
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
 *       memory and operate on tile-size chunks.  Create a flattened
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
 *       Export preview
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

#include "libgimpmath/gimpmath.h"

#include "psd.h"
#include "psd-util.h"
#include "psd-save.h"

#include "libgimp/stdplugins-intl.h"


/* set to TRUE if you want debugging, FALSE otherwise */
#define DEBUG FALSE

/* 1: Normal debuggin, 2: Deep debuggin */
#define DEBUG_LEVEL 2

#undef IFDBG /* previously defined in psd.h */

#define IFDBG if (DEBUG)
#define IF_DEEP_DBG if (DEBUG && DEBUG_LEVEL == 2)

#define PSD_UNIT_INCH 1
#define PSD_UNIT_CM   2


/* Local types etc
 */

typedef enum PsdLayerType
{
  PSD_LAYER_TYPE_LAYER,
  PSD_LAYER_TYPE_GROUP_START,
  PSD_LAYER_TYPE_GROUP_END
} PSD_Layer_Type;

typedef struct PsdLayer
{
  GimpLayer      *layer;
  PSD_Layer_Type  type;
} PSD_Layer;

typedef struct PsdImageData
{
  gboolean           compression;

  gint32             image_height;
  gint32             image_width;

  GimpImageBaseType  baseType;

  GimpLayer         *merged_layer;/* Merged image,
                                     to be used for the image data section */

  gint               nChannels;   /* Number of user channels in the image */
  GList             *lChannels;   /* List of GimpChannel (User channels in the image) */

  gint               nLayers;     /* Number of layers in the image */
  GList             *lLayers;     /* List of PSD_Layer */
} PSD_Image_Data;

static PSD_Image_Data PSDImageData;

/* Declare some local functions.
 */

static const gchar * psd_lmode_layer      (GimpLayer     *layer,
                                           gboolean       section_divider);

static void          reshuffle_cmap_write (guchar        *mapGimp);

static void          save_header          (FILE          *fd,
                                           GimpImage     *image);

static void          save_color_mode_data (FILE          *fd,
                                           GimpImage     *image);

static void          save_resources       (FILE          *fd,
                                           GimpImage     *image);

static void          save_layer_and_mask  (FILE          *fd,
                                           GimpImage     *image);

static void          save_data            (FILE          *fd,
                                           GimpImage     *image);

static void          xfwrite              (FILE          *fd,
                                           gconstpointer  buf,
                                           glong          len,
                                           const gchar   *why);

static void          write_pascalstring   (FILE          *fd,
                                           const gchar   *val,
                                           gint           padding,
                                           const gchar   *why);

static void          write_string         (FILE          *fd,
                                           const gchar   *val,
                                           const gchar   *why);

static void          write_gchar          (FILE          *fd,
                                           guchar         val,
                                           const gchar   *why);

static void          write_gint16         (FILE          *fd,
                                           gint16         val,
                                           const gchar   *why);

static void          write_gint32         (FILE          *fd,
                                           gint32         val,
                                           const gchar   *why);

static void          write_datablock_luni (FILE          *fd,
                                           const gchar   *val,
                                           const gchar   *why);


static void          write_pixel_data     (FILE          *fd,
                                           GimpDrawable  *drawable,
                                           glong         *ChanLenPosition,
                                           gint32         rowlenOffset,
                                           gboolean       write_mask);

static GimpLayer   * create_merged_image  (GimpImage     *image);

static gint          get_bpc              (GimpImage     *image);
static const Babl  * get_pixel_format     (GimpDrawable  *drawable);
static const Babl  * get_channel_format   (GimpDrawable  *drawable);
static const Babl  * get_mask_format      (GimpLayerMask *mask);

static GList       * image_get_all_layers (GimpImage     *image,
                                           gint          *n_layers);

static const gchar *
psd_lmode_layer (GimpLayer *layer,
                 gboolean   section_divider)
{
  LayerModeInfo mode_info;

  mode_info.mode            = gimp_layer_get_mode (layer);
  mode_info.blend_space     = gimp_layer_get_blend_space (layer);
  mode_info.composite_space = gimp_layer_get_composite_space (layer);
  mode_info.composite_mode  = gimp_layer_get_composite_mode (layer);

  /* pass-through groups use normal mode in their layer record; the
   * pass-through mode is specified in their section divider resource.
   */
  if (mode_info.mode == GIMP_LAYER_MODE_PASS_THROUGH && ! section_divider)
    mode_info.mode = GIMP_LAYER_MODE_NORMAL;

  return gimp_to_psd_blend_mode (&mode_info);
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
    {
      write_gchar (fd, 0, why);
    }

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
  if (len == 0)
    return;

  if (fwrite (buf, len, 1, fd) == 0)
    {
      g_printerr ("%s: Error while writing '%s'\n", G_STRFUNC, why);
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
      g_printerr ("%s: Error while writing '%s'\n", G_STRFUNC, why);
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
      g_printerr ("%s: Error while writing '%s'\n", G_STRFUNC, why);
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
      g_printerr ("%s: Error while writing '%s'\n", G_STRFUNC, why);
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
             (start[0] == start[i]))
        i++;

      if (i > 1)              /* Match found */
        {

          *dest_ptr++ = -(i - 1);
          *dest_ptr++ = *start;

          start += i;
          remaining -= i;
          length += 2;
        }
      else       /* Look for characters different from the previous */
        {
          i = 0;
          while ((i < 128)                 &&
                 (remaining - (i + 1) > 0) &&
                 (start[i] != start[i + 1] ||
                  remaining - (i + 2) <= 0 || start[i] != start[i+2]))
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
                  *dest_ptr++ = start[j];
                }
              start += i;
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
      IFDBG printf ("PSD Export: gimpBaseType value is %d, "
                    "can't convert to PSD mode", gimpBaseType);
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
save_header (FILE      *fd,
             GimpImage *image)
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
                     nChansLayer (PSDImageData.baseType,
                     gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)), 0)),
                "channels");
  write_gint32 (fd, PSDImageData.image_height, "rows");
  write_gint32 (fd, PSDImageData.image_width, "columns");
  write_gint16 (fd, 8 * get_bpc (image), "depth");
  write_gint16 (fd, gimpBaseTypeToPsdMode (PSDImageData.baseType), "mode");
}

static void
save_color_mode_data (FILE      *fd,
                      GimpImage *image)
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

      cmap = gimp_image_get_colormap (image, &nColors);
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
save_resources (FILE      *fd,
                GimpImage *image)
{
  GList        *iter;
  gint          i;
  gchar        *fileName;            /* Image file name */
  GimpLayer    *ActLayer;            /* The active layer */
  guint         nActiveLayer = 0;    /* Number of the active layer */
  gboolean      ActiveLayerPresent;  /* TRUE if there's an active layer */

  glong         eof_pos;             /* Position for End of file */
  glong         rsc_pos;             /* Position for Lengths of Resources section */
  glong         name_sec;            /* Position for Lengths of Channel Names */


  /* Only relevant resources in GIMP are: 0x03EE, 0x03F0 & 0x0400 */
  /* For Adobe Photoshop version 4.0 these can also be considered:
     0x0408, 0x040A & 0x040B (1006, 1008, 1024, 1032, 1034, and 1035) */

  IFDBG printf (" Function: save_resources\n");


  /* Get the image title from its filename */

  fileName = g_file_get_path (gimp_image_get_file (image));
  IFDBG printf ("\tImage title: %s\n", fileName);

  /* Get the active layer number id */

  ActLayer = gimp_image_get_active_layer (image);
  IFDBG printf ("\tCurrent layer id: %d\n",
                gimp_item_get_id (GIMP_ITEM (ActLayer)));

  ActiveLayerPresent = FALSE;
  for (iter = PSDImageData.lLayers, i = 0;
       iter;
       iter = g_list_next (iter), i++)
    {
      if (ActLayer == ((PSD_Layer *) iter->data)->layer)
        {
          nActiveLayer = PSDImageData.nLayers - i - 1;
          ActiveLayerPresent = TRUE;
          break;
        }
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

  if (PSDImageData.nChannels > 0 ||
      gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)))
    {
      xfwrite (fd, "8BIM", 4, "imageresources signature");
      write_gint16 (fd, 0x03EE, "0x03EE Id"); /* 1006 */
      /* write_pascalstring (fd, Name, "Id name"); */
      write_gint16 (fd, 0, "Id name"); /* Set to null string (two zeros) */

      /* Mark current position in the file */

      name_sec = ftell (fd);
      write_gint32 (fd, 0, "0x03EE resource size");

      /* Write all strings */

      /* if the merged_image contains transparency, write a name for it first */
      if (gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)))
        write_string (fd, "Transparency", "channel name");

      for (iter = PSDImageData.lChannels; iter; iter = g_list_next (iter))
        {
          char *chName = gimp_item_get_name (iter->data);
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

  /* --------------- Write Channel properties --------------- */

  if (PSDImageData.nChannels > 0 ||
      gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)))
    {
      xfwrite (fd, "8BIM", 4, "imageresources signature");
      write_gint16 (fd, 0x0435, "0x0435 Id"); /* 1077 */
      /* write_pascalstring (fd, Name, "Id name"); */
      write_gint16 (fd, 0, "Id name"); /* Set to null string (two zeros) */
      write_gint32 (fd,
                    4  +
                    13 * (gimp_drawable_has_alpha (
                            GIMP_DRAWABLE (PSDImageData.merged_layer)) +
                          PSDImageData.nChannels),
                    "0x0435 resource size");

      /* The function of the first 4 bytes is unclear. As per
       * load_resource_1077() in psd-image-res-load.c, it seems to be a version
       * number that is always one.
       */
      write_gint32 (fd, 1, "0x0435 version");

      /* Write all channel properties */

      #define DOUBLE_TO_INT16(x) ROUND (SAFE_CLAMP (x, 0.0, 1.0) * 0xffff)

      /* if the merged_image contains transparency, write its properties first */
      if (gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)))
        {
          write_gint16 (fd, PSD_CS_RGB, "channel color space");
          write_gint16 (fd, DOUBLE_TO_INT16 (1.0), "channel color r");
          write_gint16 (fd, DOUBLE_TO_INT16 (0.0), "channel color g");
          write_gint16 (fd, DOUBLE_TO_INT16 (0.0), "channel color b");
          write_gint16 (fd, 0,                     "channel color padding");
          write_gint16 (fd, 100,                   "channel opacity");
          write_gchar  (fd, 1,                     "channel mode");
        }

      for (iter = PSDImageData.lChannels; iter; iter = g_list_next (iter))
        {
          GimpChannel *channel = iter->data;
          GimpRGB      color;
          gdouble      opacity;

          gimp_channel_get_color (channel, &color);
          opacity = gimp_channel_get_opacity (channel);

          write_gint16 (fd, PSD_CS_RGB,                "channel color space");
          write_gint16 (fd, DOUBLE_TO_INT16 (color.r), "channel color r");
          write_gint16 (fd, DOUBLE_TO_INT16 (color.g), "channel color g");
          write_gint16 (fd, DOUBLE_TO_INT16 (color.b), "channel color b");
          write_gint16 (fd, 0,                         "channel color padding");
          write_gint16 (fd, ROUND (opacity),           "channel opacity");
          write_gchar  (fd, 1,                         "channel mode");
        }

      #undef DOUBLE_TO_INT16

      /* Pad if length is odd */

      if (ftell (fd) & 1)
        write_gchar (fd, 0, "pad byte");
    }

  /* --------------- Write Guides --------------- */
  if (gimp_image_find_next_guide (image, 0))
    {
      gint n_guides = 0;
      gint guide_id =0;

      /* Count the guides */
      while ((guide_id = gimp_image_find_next_guide(image, guide_id)))
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
      while ((guide_id = gimp_image_find_next_guide (image, guide_id)))
        {
          gchar orientation;
          gint32 position;
          orientation = gimp_image_get_guide_orientation (image, guide_id);
          position    = 32 * gimp_image_get_guide_position (image, guide_id);
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

    g_unit = gimp_image_get_unit (image);
    gimp_image_get_resolution (image, &xres, &yres);

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

  /* --------------- Write ICC profile data ------------------- */
  {
    GimpColorProfile *profile;

    profile = gimp_image_get_effective_color_profile (image);

    if (profile)
      {
        const guint8 *icc_data;
        gsize         icc_length;

        icc_data = gimp_color_profile_get_icc_profile (profile, &icc_length);

        xfwrite (fd, "8BIM", 4, "imageresources signature");
        write_gint16 (fd, 0x040f, "0x040f Id");
        write_gint16 (fd, 0, "Id name"); /* Set to null string (two zeros) */
        write_gint32 (fd, icc_length, "0x040f resource size");
        xfwrite (fd, icc_data, icc_length, "ICC profile");

        g_object_unref (profile);
      }
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
                           gint32   bpc,
                           gint16  *LengthsTable,
                           guchar  *remdata)
{
  gint    i;
  gint32  len;                 /* Length of compressed data */
  guchar *start;               /* Starting position of a row in channel_data */

  stride /= bpc;

  /* Pack channel data, and perform byte-order conversion */
  switch (bpc)
    {
    case 1:
      {
        if (stride > 1)
          {
            const guint8 *src  = (const guint8 *) channel_data;
            guint8       *dest = (guint8       *) channel_data;

            for (i = 0; i < channel_rows * channel_cols; i++)
              {
                *dest = *src;

                dest++;
                src += stride;
              }
          }
      }
      break;

    case 2:
      {
        const guint16 *src  = (const guint16 *) channel_data;
        guint16       *dest = (guint16       *) channel_data;

        for (i = 0; i < channel_rows * channel_cols; i++)
          {
            *dest = GUINT16_TO_BE (*src);

            dest++;
            src += stride;
          }
      }
      break;

    case 4:
      {
        const guint32 *src  = (const guint32 *) channel_data;
        guint32       *dest = (guint32       *) channel_data;

        for (i = 0; i < channel_rows * channel_cols; i++)
          {
            *dest = GUINT32_TO_BE (*src);

            dest++;
            src += stride;
          }
      }
      break;

    default:
      g_return_val_if_reached (0);
    }

  /* For every row in the channel */

  len = 0;
  for (i = 0; i < channel_rows; i++)
    {
      start = channel_data + i * channel_cols * bpc;

      /* Create packed data for this row */
      LengthsTable[i] = pack_pb_line (start, channel_cols * bpc,
                                      &remdata[len]);
      len += LengthsTable[i];
    }

  /*  return((len + channel_rows * sizeof (gint16)) + sizeof (gint16));*/
  return len;
}

static void
save_layer_and_mask (FILE      *fd,
                     GimpImage *image)
{
  gint           i,j;
  gint           idChannel;
  gint           offset_x;               /* X offset for each layer */
  gint           offset_y;               /* Y offset for each layer */
  gint32         layerWidth;             /* Width of each layer */
  gint32         layerHeight;            /* Height of each layer */
  const gchar   *blendMode;              /* Blending mode of the layer */
  guchar         layerOpacity;           /* Opacity of the layer */
  guchar         flags;                  /* Layer flags */
  gint           nChannelsLayer;         /* Number of channels of a layer */
  gint32         ChanSize;               /* Data length for a channel */
  gchar         *layerName;              /* Layer name */
  GimpLayerMask *mask;                   /* Layer mask */
  gint           depth;                  /* Layer group nesting depth */
  gint          bpc;                     /* Image BPC */

  glong          eof_pos;                /* Position: End of file */
  glong          ExtraDataPos;           /* Position: Extra data length */
  glong          LayerMaskPos;           /* Position: Layer & Mask section length */
  glong          LayerInfoPos;           /* Position: Layer info section length*/
  glong        **ChannelLengthPos;       /* Position: Channel length */
  GList         *iter;


  IFDBG printf (" Function: save_layer_and_mask\n");

  /* Create first array dimension (layers, channels) */

  ChannelLengthPos = g_newa (glong *, PSDImageData.nLayers);

  /* Layer and mask information section */

  LayerMaskPos = ftell (fd);
  write_gint32 (fd, 0, "layers & mask information length");

  /* Layer info section */

  LayerInfoPos = ftell (fd);
  write_gint32 (fd, 0, "layers info section length");

  /* Layer structure section */

  if (gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)))
    write_gint16 (fd, -PSDImageData.nLayers, "Layer structure count");
  else
    write_gint16 (fd, PSDImageData.nLayers, "Layer structure count");

  depth = 0;

  bpc = get_bpc (image);

  /* Layer records section */
  /* GIMP layers must be written in reverse order */

  for (iter = g_list_last (PSDImageData.lLayers), i = PSDImageData.nLayers;
       iter;
       iter = g_list_previous (iter), i--)
    {
      PSD_Layer *psd_layer = (PSD_Layer *) iter->data;
      gint       hasMask = 0;

      if (psd_layer->type == PSD_LAYER_TYPE_LAYER)
        {
          gimp_drawable_offsets (GIMP_DRAWABLE (psd_layer->layer),
                                 &offset_x, &offset_y);
          layerWidth = gimp_drawable_width (GIMP_DRAWABLE (psd_layer->layer));
          layerHeight = gimp_drawable_height (GIMP_DRAWABLE (psd_layer->layer));
        }
      else
        {
          /* groups don't specify their dimensions, and have empty channel
           * data
           */
          offset_x    = 0;
          offset_y    = 0;
          layerWidth  = 0;
          layerHeight = 0;
        }

      IFDBG printf ("\tLayer number: %d\n", i);
      IFDBG
        {
          const gchar *type;

          switch (psd_layer->type)
            {
            case PSD_LAYER_TYPE_LAYER:       type = "normal layer"; break;
            case PSD_LAYER_TYPE_GROUP_START: type = "group start marker"; break;
            case PSD_LAYER_TYPE_GROUP_END:   type = "group end marker"; break;
            }

          printf ("\t\tType: %s\n", type);
        }
      IFDBG printf ("\t\tX offset: %d\n", offset_x);
      IFDBG printf ("\t\tY offset: %d\n", offset_y);
      IFDBG printf ("\t\tWidth: %d\n", layerWidth);
      IFDBG printf ("\t\tHeight: %d\n", layerHeight);

      write_gint32 (fd, offset_y,               "Layer top");
      write_gint32 (fd, offset_x,               "Layer left");
      write_gint32 (fd, offset_y + layerHeight, "Layer bottom");
      write_gint32 (fd, offset_x + layerWidth,  "Layer right");

      hasMask = (psd_layer->type != PSD_LAYER_TYPE_GROUP_END &&
                 gimp_layer_get_mask (psd_layer->layer) != NULL);
      nChannelsLayer = nChansLayer (PSDImageData.baseType,
                                    gimp_drawable_has_alpha (GIMP_DRAWABLE (psd_layer->layer)),
                                    hasMask);


      write_gint16 (fd, nChannelsLayer, "Number channels in the layer");
      IFDBG printf ("\t\tNumber of channels: %d\n", nChannelsLayer);

      /* Create second array dimension (layers, channels) */

      ChannelLengthPos[i] = g_new (glong, nChannelsLayer);

      /* Try with gimp_drawable_bpp() */

      for (j = 0; j < nChannelsLayer; j++)
        {
          if (gimp_drawable_has_alpha (GIMP_DRAWABLE (psd_layer->layer)))
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
          ChanSize = sizeof (gint16) + (layerWidth * layerHeight * bpc);

          write_gint32 (fd, ChanSize, "Channel Size");
          IFDBG printf ("\t\t\tLength: %d\n", ChanSize);
        }

      xfwrite (fd, "8BIM", 4, "blend mode signature");

      blendMode = psd_lmode_layer (psd_layer->layer, FALSE);
      IFDBG printf ("\t\tBlend mode: %s\n", blendMode);
      xfwrite (fd, blendMode, 4, "blend mode key");

      layerOpacity = RINT ((gimp_layer_get_opacity (psd_layer->layer) * 255.0) / 100.0);
      IFDBG printf ("\t\tOpacity: %u\n", layerOpacity);
      write_gchar (fd, layerOpacity, "Opacity");

      /* Apparently this field is not used in GIMP */
      write_gchar (fd, 0, "Clipping");

      flags = 0;
      if (gimp_layer_get_lock_alpha (psd_layer->layer)) flags |= 1;
      if (! gimp_item_get_visible (GIMP_ITEM (psd_layer->layer))) flags |= 2;
      if (psd_layer->type != PSD_LAYER_TYPE_LAYER) flags |= 0x18;
      IFDBG printf ("\t\tFlags: %u\n", flags);
      write_gchar (fd, flags, "Flags");

      /* Padding byte to make the length even */
      write_gchar (fd, 0, "Filler");

      ExtraDataPos = ftell (fd); /* Position of Extra Data size */
      write_gint32 (fd, 0, "Extra data size");

      if (hasMask)
        {
          gint     maskOffset_x;
          gint     maskOffset_y;
          gint     maskWidth;
          gint     maskHeight;
          gboolean apply;

          mask = gimp_layer_get_mask (psd_layer->layer);

          gimp_drawable_offsets (GIMP_DRAWABLE (mask), &maskOffset_x, &maskOffset_y);

          maskWidth  = gimp_drawable_width  (GIMP_DRAWABLE (mask));
          maskHeight = gimp_drawable_height (GIMP_DRAWABLE (mask));
          apply      = gimp_layer_get_apply_mask (psd_layer->layer);

          IFDBG printf ("\t\tLayer mask size: %d\n", 20);
          write_gint32 (fd, 20,                        "Layer mask size");
          write_gint32 (fd, maskOffset_y,              "Layer mask top");
          write_gint32 (fd, maskOffset_x,              "Layer mask left");
          write_gint32 (fd, maskOffset_y + maskHeight, "Layer mask bottom");
          write_gint32 (fd, maskOffset_x + maskWidth,  "Layer mask right");
          write_gchar  (fd, 0,                         "Layer mask default color");
          flags = (0                    |  /* position relative to layer */
                   (apply ? 0 : 1) << 1 |  /* layer mask disabled        */
                   0 << 2);                /* invert layer mask          */
          write_gchar  (fd, flags,                     "Layer mask flags");
          write_gint16 (fd, 0,                         "Layer mask Padding");
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

      if (psd_layer->type != PSD_LAYER_TYPE_GROUP_END)
        layerName = gimp_item_get_name (GIMP_ITEM (psd_layer->layer));
      else
        layerName = g_strdup ("</Layer group>");
      write_pascalstring (fd, layerName, 4, "layer name");
      IFDBG printf ("\t\tLayer name: %s\n", layerName);

      /* Additional layer information blocks */
      /* Unicode layer name */
      write_datablock_luni (fd, layerName, "luni extra data block");

      g_free (layerName);

      /* Layer color tag */
      xfwrite (fd, "8BIMlclr", 8, "sheet color signature");
      write_gint32 (fd, 8, "sheet color size");
      write_gint16 (fd,
                    gimp_to_psd_layer_color_tag (gimp_item_get_color_tag (GIMP_ITEM (psd_layer->layer))),
                    "sheet color code");
      write_gint16 (fd, 0, "sheet color unused value");
      write_gint16 (fd, 0, "sheet color unused value");
      write_gint16 (fd, 0, "sheet color unused value");

      /* Group layer section divider */
      if (psd_layer->type != PSD_LAYER_TYPE_LAYER)
        {
          gint32 size;
          gint32 type;

          size = 12;

          if (psd_layer->type == PSD_LAYER_TYPE_GROUP_START)
            {
              type = gimp_item_get_expanded (GIMP_ITEM (psd_layer->layer)) ? 1 : 2;

              depth--;
            }
          else
            {
              type = 3;

              depth++;
            }

          blendMode = psd_lmode_layer (psd_layer->layer, TRUE);

          if (type < 3 || depth <= 5)
            {
              xfwrite (fd, "8BIMlsct", 8, "section divider");
            }
          else
            {
              /* layer groups whose nesting depth is above 5 are only supported
               * by Photoshop CS5 and up, and their end markers use the
               * (undocumented) "lsdk" key, instead of "lsct".
               */
              xfwrite (fd, "8BIMlsdk", 8, "nested section divider");
            }
          write_gint32 (fd, size, "section divider size");
          write_gint32 (fd, type, "section divider type");
          xfwrite (fd, "8BIM", 4, "section divider blend mode signature");
          xfwrite (fd, blendMode, 4, "section divider blend mode key");
        }

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

  for (iter = g_list_last (PSDImageData.lLayers), i = PSDImageData.nLayers;
       iter;
       iter = g_list_previous (iter), i--)
    {
      PSD_Layer *psd_layer = (PSD_Layer *) iter->data;

      gimp_progress_update ((PSDImageData.nLayers - i - 1.0) / (PSDImageData.nLayers + 1.0));

      IFDBG printf ("\t\tWriting pixel data for layer slot %d\n", i);
      write_pixel_data (fd, GIMP_DRAWABLE (psd_layer->layer), ChannelLengthPos[i], 0,
                        psd_layer->type != PSD_LAYER_TYPE_GROUP_END);
      g_free (ChannelLengthPos[i]);
    }

  gimp_progress_update (PSDImageData.nLayers / (PSDImageData.nLayers + 1.0));
  eof_pos = ftell (fd);

  /* Write actual size of Layer info section */

  fseek (fd, LayerInfoPos, SEEK_SET);
  write_gint32 (fd, eof_pos - LayerInfoPos - sizeof (gint32), "layers info section length");
  IFDBG printf ("\t\tTotal layers info section length: %d\n",
                (int) (eof_pos - LayerInfoPos - sizeof (gint32)));

  /* Write actual size of Layer and mask information section */

  fseek (fd, LayerMaskPos, SEEK_SET);
  write_gint32 (fd, eof_pos - LayerMaskPos - sizeof (gint32), "layers & mask information length");
  IFDBG printf ("\t\tTotal layers & mask information length: %d\n",
                (int) (eof_pos - LayerMaskPos - sizeof (gint32)));

  /* Return to EOF to continue writing */

  fseek (fd, eof_pos, SEEK_SET);
}

static void
write_pixel_data (FILE         *fd,
                  GimpDrawable *drawable,
                  glong        *ChanLenPosition,
                  gint32        ltable_offset,
                  gboolean      write_mask)
{
  GeglBuffer    *buffer = gimp_drawable_get_buffer (drawable);
  const Babl    *format;
  GimpLayerMask *mask;
  gint32         tile_height = gimp_tile_height ();
  gint32         height = gegl_buffer_get_height (buffer);
  gint32         width  = gegl_buffer_get_width (buffer);
  gint32         bytes;
  gint32         components;
  gint32         bpc;
  gint32         colors;
  gint32         y;
  gint32         len;                  /* Length of compressed data */
  gint16        *LengthsTable;         /* Lengths of every compressed row */
  guchar        *rledata;              /* Compressed data from a region */
  guchar        *data;                 /* Temporary copy of pixel data */
  glong          length_table_pos;     /* position in file of the length table */
  int            i, j;

  IFDBG printf (" Function: write_pixel_data, drw %d, lto %d\n",
                gimp_item_get_id (GIMP_ITEM (drawable)), ltable_offset);

  if (write_mask)
    mask = gimp_layer_get_mask (GIMP_LAYER (drawable));
  else
    mask = NULL;

  /* groups have empty channel data, but may have a mask */
  if (gimp_item_is_group (GIMP_ITEM (drawable)) && mask == NULL)
    {
      width  = 0;
      height = 0;
    }

  if (gimp_item_is_channel (GIMP_ITEM (drawable)))
    format = get_channel_format (drawable);
  else
    format = get_pixel_format (drawable);

  bytes      = babl_format_get_bytes_per_pixel (format);
  components = babl_format_get_n_components    (format);
  bpc        = bytes / components;

  colors = components;

  if (gimp_drawable_has_alpha  (drawable) &&
      ! gimp_drawable_is_indexed (drawable))
    colors -= 1;

  LengthsTable = g_new (gint16, height);
  rledata = g_new (guchar, (MIN (height, tile_height) *
                            (width + 10 + (width / 100))) * bpc);

  data = g_new (guchar, MIN (height, tile_height) * width * bytes);

  /* groups have empty channel data */
  if (gimp_item_is_group (GIMP_ITEM (drawable)))
    {
      width  = 0;
      height = 0;
    }

  for (i = 0; i < components; i++)
    {
      gint chan;

      len = 0;

      if (components != colors && ltable_offset == 0) /* Need to write alpha channel first, except in image data section */
        {
          if (i == 0)
            {
              chan = components - 1;
            }
          else
            {
              chan = i - 1;
            }
        }
      else
        {
          chan = i;
        }

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
          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (0, y,
                                           width,
                                           MIN (height - y, tile_height)),
                           1.0, format, data,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
          tlen = get_compress_channel_data (&data[chan * bpc],
                                            width,
                                            MIN(height - y, tile_height),
                                            bytes, bpc,
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
  if (mask != NULL)
    {
      GeglBuffer *mbuffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));
      const Babl *mformat = get_mask_format (mask);

      width  = gegl_buffer_get_width (buffer);
      height = gegl_buffer_get_height (buffer);

      len = 0;

      if (ChanLenPosition)
        {
          write_gint16 (fd, 1, "Compression type (RLE)");
          len += 2;
          IF_DEEP_DBG printf ("\t\t\t\t. ChanLenPos, len %d\n", len);
        }

      if (ltable_offset > 0)
        {
          length_table_pos = ltable_offset + 2 * (components+1) * height;
          IF_DEEP_DBG printf ("\t\t\t\t. ltable, pos %ld\n",
                              length_table_pos);
        }
      else
        {
          length_table_pos = ftell(fd);

          xfwrite (fd, LengthsTable, height * sizeof(gint16),
                   "Dummy RLE length");
          len += height * sizeof(gint16);
          IF_DEEP_DBG printf ("\t\t\t\t. ltable, pos %ld len %d\n",
                              length_table_pos, len);
        }

      for (y = 0; y < height; y += tile_height)
        {
          int tlen;
          gegl_buffer_get (mbuffer,
                           GEGL_RECTANGLE (0, y,
                                           width,
                                           MIN (height - y, tile_height)),
                           1.0, mformat, data,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
          tlen = get_compress_channel_data (&data[0],
                                            width,
                                            MIN(height - y, tile_height),
                                            bpc, bpc,
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
          IF_DEEP_DBG printf ("\t\t\t\t. Updating RLE len %d\n",
                              LengthsTable[j]);
        }

      if (ChanLenPosition)    /* Update total compressed length */
        {
          fseek (fd, ChanLenPosition[bytes], SEEK_SET); /*+bytes OR SOMETHING*/
          write_gint32 (fd, len, "channel data length");
          IFDBG printf ("\t\tUpdating data len to %d, at %ld\n", len, ftell(fd));
        }
      fseek (fd, 0, SEEK_END);
      IF_DEEP_DBG printf ("\t\t\t\t. Cur pos %ld\n", ftell(fd));

      g_object_unref (mbuffer);
    }

  g_object_unref (buffer);

  g_free (data);
  g_free (rledata);
  g_free (LengthsTable);
}

static void
save_data (FILE      *fd,
           GimpImage *image)
{
  GList  *iter;
  gint    ChanCount;
  gint    i, j;
  gint32  imageHeight;                   /* Height of image */
  glong   offset;                         /* offset in file of rle lengths */
  gint    chan;

  IFDBG printf (" Function: save_data\n");

  ChanCount = (PSDImageData.nChannels +
               nChansLayer (PSDImageData.baseType,
                            gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)),
                            0));

  imageHeight = gimp_image_height (image);

  write_gint16 (fd, 1, "RLE compression");

  /* All line lengths go before the rle pixel data */

  offset = ftell(fd); /* Offset in file of line lengths */

  for (i = 0; i < ChanCount; i++)
    for (j = 0; j < imageHeight; j++)
      write_gint16 (fd, 0, "junk line lengths");

  IFDBG printf ("\t\tWriting compressed image data\n");
  write_pixel_data (fd, GIMP_DRAWABLE (PSDImageData.merged_layer),
                    NULL, offset, FALSE);

  chan = nChansLayer (PSDImageData.baseType,
                      gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)), 0);

  for (iter = PSDImageData.lChannels; iter; iter = g_list_next (iter))
    {
      IFDBG printf ("\t\tWriting compressed channel data for channel %d\n",
                    i);
      write_pixel_data (fd, iter->data, NULL,
                        offset + 2*imageHeight*chan, FALSE); //check how imgs are channels here
      chan++;
    }
}

static GimpLayer *
create_merged_image (GimpImage *image)
{
  GimpLayer *projection;

  projection = gimp_layer_new_from_visible (image, image, "psd-save");

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (projection)))
    return projection;

  if (gimp_image_base_type (image) != GIMP_INDEXED)
    {
      GeglBuffer         *buffer             = gimp_drawable_get_buffer (GIMP_DRAWABLE (projection));
      const Babl         *format             = get_pixel_format (GIMP_DRAWABLE (projection));
      gboolean            transparency_found = FALSE;
      gint                bpp                = babl_format_get_bytes_per_pixel (format);
      GeglBufferIterator *iter;

      iter = gegl_buffer_iterator_new (buffer, NULL, 0, format,
                                       GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);

      while (gegl_buffer_iterator_next (iter))
        {
          guchar *d = iter->items[0].data;
          gint    i;

          for (i = 0; i < iter->length; i++)
            {
              gint32 alpha = d[bpp - 1];

              if (alpha < 255)
                {
                  gint c;

                  transparency_found = TRUE;

                  /* blend against white, photoshop does this. */
                  for (c = 0; c < bpp - 1; c++)
                    d[c] = ((guint32) d[c] * alpha + 128) / 255 + 255 - alpha;
                }

              d += bpp;
            }
        }

      g_object_unref (buffer);

      if (! transparency_found)
        gimp_layer_flatten (projection);
    }
  else
    {
      gimp_layer_flatten (projection);  /* PSDs don't support transparency information in indexed images*/
    }

  return projection;
}

static void
get_image_data (GimpImage *image)
{
  IFDBG printf (" Function: get_image_data\n");

  PSDImageData.compression = FALSE;

  PSDImageData.image_height = gimp_image_height (image);
  IFDBG printf ("\tGot number of rows: %d\n", PSDImageData.image_height);

  PSDImageData.image_width = gimp_image_width (image);
  IFDBG printf ("\tGot number of cols: %d\n", PSDImageData.image_width);

  PSDImageData.baseType = gimp_image_base_type (image);
  IFDBG printf ("\tGot base type: %d\n", PSDImageData.baseType);

  PSDImageData.merged_layer = create_merged_image (image);

  PSDImageData.lChannels = gimp_image_list_channels (image);
  PSDImageData.nChannels = g_list_length (PSDImageData.lChannels);
  IFDBG printf ("\tGot number of channels: %d\n", PSDImageData.nChannels);

  PSDImageData.lLayers = image_get_all_layers (image,
                                               &PSDImageData.nLayers);
  IFDBG printf ("\tGot number of layers: %d\n", PSDImageData.nLayers);
}

static void
clear_image_data (void)
{
  IFDBG printf (" Function: clear_image_data\n");

  g_list_free (PSDImageData.lChannels);
  PSDImageData.lChannels = NULL;

  g_list_free (PSDImageData.lLayers);
  PSDImageData.lLayers = NULL;
}

gboolean
save_image (GFile      *file,
            GimpImage  *image,
            GError    **error)
{
  gchar      *filename;
  FILE       *fd;
  GeglBuffer *buffer;
  GList      *iter;

  IFDBG printf (" Function: save_image\n");

  if (gimp_image_width (image) > 30000 ||
      gimp_image_height (image) > 30000)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unable to export '%s'.  The PSD file format does not "
                     "support images that are more than 30,000 pixels wide "
                     "or tall."),
                   gimp_file_get_utf8_name (file));
      return FALSE;
    }

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  get_image_data (image);

  /* Need to check each of the layers size individually also */
  for (iter = PSDImageData.lLayers; iter; iter = iter->next)
    {
      PSD_Layer *layer = iter->data;

      if (layer->type == PSD_LAYER_TYPE_LAYER)
        {
          buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer->layer));

          if (gegl_buffer_get_width (buffer)  > 30000 ||
              gegl_buffer_get_height (buffer) > 30000)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Unable to export '%s'.  The PSD file format does not "
                             "support images with layers that are more than 30,000 "
                             "pixels wide or tall."),
                           gimp_file_get_utf8_name (file));
              clear_image_data ();
              return FALSE;
            }

          g_object_unref (buffer);
        }
    }

  filename = g_file_get_path (file);
  fd = g_fopen (filename, "wb");
  g_free (filename);

  if (! fd)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      clear_image_data ();
      return FALSE;
    }

  IFDBG g_print ("\tFile '%s' has been opened\n",
                 gimp_file_get_utf8_name (file));

  save_header (fd, image);
  save_color_mode_data (fd, image);
  save_resources (fd, image);

  /* PSD format does not support layers in indexed images */

  if (PSDImageData.baseType == GIMP_INDEXED)
    write_gint32 (fd, 0, "layers info section length");
  else
    save_layer_and_mask (fd, image);

  /* If this is an indexed image, write now channel and layer info */

  save_data (fd, image);

  /* Delete merged image now */

  gimp_item_delete (GIMP_ITEM (PSDImageData.merged_layer));

  clear_image_data ();

  IFDBG printf ("----- Closing PSD file, done -----\n\n");

  fclose (fd);

  gimp_progress_update (1.0);

  return TRUE;
}

static gint
get_bpc (GimpImage *image)
{
  switch (gimp_image_get_precision (image))
    {
    case GIMP_PRECISION_U8_LINEAR:
    case GIMP_PRECISION_U8_NON_LINEAR:
    case GIMP_PRECISION_U8_PERCEPTUAL:
      return 1;

    case GIMP_PRECISION_U16_LINEAR:
    case GIMP_PRECISION_U16_NON_LINEAR:
    case GIMP_PRECISION_U16_PERCEPTUAL:
    case GIMP_PRECISION_HALF_LINEAR:
    case GIMP_PRECISION_HALF_NON_LINEAR:
    case GIMP_PRECISION_HALF_PERCEPTUAL:
      return 2;

    case GIMP_PRECISION_U32_LINEAR:
    case GIMP_PRECISION_U32_NON_LINEAR:
    case GIMP_PRECISION_U32_PERCEPTUAL:
    case GIMP_PRECISION_FLOAT_LINEAR:
    case GIMP_PRECISION_FLOAT_NON_LINEAR:
    case GIMP_PRECISION_FLOAT_PERCEPTUAL:
    default:
      /* FIXME: we *should* encode the image as u32 in this case, but simply
       * using the same code as for the other cases produces invalid psd files
       * (they're rejected by photoshop, although they can be read by the
       * corresponding psd-load.c code, which in turn can't actually read
       * photoshop-generated u32 files.)
       *
       * simply encode the image as u16 for now.
       */
      /* return 4; */
      return 2;
    }
}

static const Babl *
get_pixel_format (GimpDrawable *drawable)
{
  GimpImage   *image = gimp_item_get_image (GIMP_ITEM (drawable));
  const gchar *model;
  gint         bpc;
  gchar        format[32];

  switch (gimp_drawable_type (drawable))
    {
    case GIMP_GRAY_IMAGE:
      model = "Y'";
      break;

    case GIMP_GRAYA_IMAGE:
      model = "Y'A";
      break;

    case GIMP_RGB_IMAGE:
      model = "R'G'B'";
      break;

    case GIMP_RGBA_IMAGE:
      model = "R'G'B'A";
      break;

    case GIMP_INDEXED_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      return gimp_drawable_get_format (drawable);

    default:
      g_return_val_if_reached (NULL);
    }

  bpc = get_bpc (image);

  sprintf (format, "%s u%d", model, 8 * bpc);

  return babl_format (format);
}

static const Babl *
get_channel_format (GimpDrawable *drawable)
{
  GimpImage *image = gimp_item_get_image (GIMP_ITEM (drawable));
  gint       bpc;
  gchar      format[32];

  /* see gimp_image_get_channel_format() */
  if (gimp_image_get_precision (image) == GIMP_PRECISION_U8_NON_LINEAR)
    return babl_format ("Y' u8");

  bpc = get_bpc (image);

  sprintf (format, "Y u%d", 8 * bpc);

  return babl_format (format);
}

static const Babl *
get_mask_format (GimpLayerMask *mask)
{
  GimpImage *image = gimp_item_get_image (GIMP_ITEM (mask));
  gint       bpc;
  gchar      format[32];

  bpc = get_bpc (image);

  sprintf (format, "Y u%d", 8 * bpc);

  return babl_format (format);
}

static GList *
append_layers (GList *layers)
{
  GList *psd_layers = NULL;
  GList *iter;

  for (iter = layers; iter; iter = iter->next)
    {
      PSD_Layer *layer = g_new0 (PSD_Layer, 1);
      gboolean   is_group;

      layer->layer = iter->data;

      is_group = gimp_item_is_group (iter->data);

      if (! is_group)
        layer->type = PSD_LAYER_TYPE_LAYER;
      else
        layer->type = PSD_LAYER_TYPE_GROUP_START;

      psd_layers = g_list_append (psd_layers, layer);

      if (is_group)
        {
          PSD_Layer *end_layer = g_new0 (PSD_Layer, 1);
          GList     *group_layers;

          group_layers = gimp_item_list_children (iter->data);
          psd_layers = g_list_concat (psd_layers,
                                      append_layers (group_layers));
          g_list_free (group_layers);

          end_layer->layer = iter->data;
          end_layer->type = PSD_LAYER_TYPE_GROUP_END;
          psd_layers = g_list_append (psd_layers, end_layer);
        }
    }

  return psd_layers;
}

static GList *
image_get_all_layers (GimpImage *image,
                      gint      *n_layers)
{
  GList *psd_layers = NULL;
  GList *layers;

  layers = gimp_image_list_layers (image);
  psd_layers = append_layers (layers);
  g_list_free (layers);

  *n_layers = g_list_length (psd_layers);

  return psd_layers;
}
