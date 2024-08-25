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
#include "psd-export.h"

#include "libgimp/stdplugins-intl.h"


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

typedef struct PsdResourceOptions
{
  gboolean  cmyk;
  gboolean  duotone;
  gboolean  clipping_path;
  gchar    *clipping_path_name;
  gdouble   clipping_path_flatness;
} PSD_Resource_Options;

static PSD_Image_Data PSDImageData;

/* Declare some local functions.
 */

static const gchar * psd_lmode_layer      (GimpLayer      *layer,
                                           gboolean        section_divider);

static void          reshuffle_cmap_write (guchar         *mapGimp);

static void          save_header          (GOutputStream  *output,
                                           GimpImage      *image,
                                           gboolean        export_cmyk,
                                           gboolean        export_duotone);

static void          save_color_mode_data (GOutputStream  *output,
                                           GimpImage      *image,
                                           gboolean        export_duotone);

static void          save_resources       (GOutputStream  *output,
                                           GimpImage      *image,
                                           PSD_Resource_Options
                                                          *options);

static void          save_paths           (GOutputStream  *output,
                                           GimpImage      *image);
static void          save_clipping_path   (GOutputStream  *output,
                                           GimpImage      *image,
                                           const gchar    *path_name,
                                           gfloat          path_flatness);

static void          save_layer_and_mask  (GOutputStream  *output,
                                           GimpImage      *image,
                                           gboolean        export_cmyk);

static void          save_data            (GOutputStream  *output,
                                           GimpImage      *image,
                                           gboolean        export_cmyk);

static void          double_to_psd_fixed  (gdouble         value,
                                           gchar          *target);

static void          xfwrite              (GOutputStream  *output,
                                           gconstpointer   buf,
                                           gsize           len,
                                           const gchar    *why);

static void          write_pascalstring   (GOutputStream  *output,
                                           const gchar    *val,
                                           gint            padding,
                                           const gchar    *why);

static void          write_string         (GOutputStream  *output,
                                           const gchar    *val,
                                           const gchar    *why);

static void          write_gchar          (GOutputStream  *output,
                                           guchar          val,
                                           const gchar    *why);

static void          write_gint16         (GOutputStream  *output,
                                           gint16          val,
                                           const gchar    *why);

static void          write_gint32         (GOutputStream  *output,
                                           gint32          val,
                                           const gchar    *why);

static void          write_datablock_luni (GOutputStream  *output,
                                           const gchar    *val,
                                           const gchar    *why);


static void          write_pixel_data     (GOutputStream  *output,
                                           GimpImage      *image,
                                           GimpDrawable   *drawable,
                                           goffset        *ChanLenPosition,
                                           goffset         rowlenOffset,
                                           gboolean        write_mask,
                                           gboolean        export_cmyk);

static GimpLayer   * create_merged_image  (GimpImage      *image);

static gint          get_bpc              (GimpImage      *image);
static const Babl  * get_pixel_format     (GimpDrawable   *drawable);
static const Babl  * get_channel_format   (GimpDrawable   *drawable);
static const Babl  * get_mask_format      (GimpLayerMask  *mask);

static GList       * image_get_all_layers (GimpImage      *image,
                                           gint           *n_layers);

static void          update_clipping_path
                                         (GimpIntComboBox *combo,
                                          gpointer         data);

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
write_string (GOutputStream  *output,
              const gchar    *val,
              const gchar    *why)
{
  write_gchar (output, strlen (val), why);
  xfwrite (output, val, strlen (val), why);
}

static void
write_pascalstring (GOutputStream  *output,
                    const gchar    *val,
                    gint            padding,
                    const gchar    *why)
{
  guchar len;
  gint   i;

  /* Calculate string length to write and limit it to 255 */

  len = (strlen (val) > 255) ? 255 : (guchar) strlen (val);

  /* Perform actual writing */

  if (len !=  0)
    {
      write_gchar (output, len, why);
      xfwrite (output, val, len, why);
    }
  else
    {
      write_gchar (output, 0, why);
    }

  /* If total length (length byte + content) is not a multiple of PADDING,
     add zeros to pad it.  */

  len++;           /* Add the length field */

  if ((len % padding) == 0)
    return;

  for (i = 0; i < (padding - (len % padding)); i++)
    write_gchar (output, 0, why);
}

static void
xfwrite (GOutputStream  *output,
         gconstpointer   buf,
         gsize           len,
         const gchar    *why)
{
  gsize bytes_written;

  if (len == 0)
    return;

  /* FIXME Instead of NULL use error parameter and add error to all functions!
   * and then we also need to change functions from void to gboolean or something
   * and check the return values. */
  if (! g_output_stream_write_all (output, buf, len,
                                   &bytes_written, NULL, NULL))
    {
      g_printerr ("%s: Error while writing '%s'\n", G_STRFUNC, why);
      gimp_quit ();
    }
}

static void
write_gchar (GOutputStream  *output,
             guchar          val,
             const gchar    *why)
{
  guchar   b[2];
  goffset  pos;
  gsize    bytes_written;

  b[0] = val;
  b[1] = 0;

  pos = g_seekable_tell (G_SEEKABLE (output));
  /* FIXME: Use error in write and seek */
  if (! g_output_stream_write_all (output, &b, 2,
                                   &bytes_written, NULL, NULL))
    {
      g_printerr ("%s: Error while writing '%s'\n", G_STRFUNC, why);
      gimp_quit ();
    }
  g_seekable_seek (G_SEEKABLE (output),
                   pos + 1, G_SEEK_SET,
                   NULL, NULL);
}

static void
write_gint16 (GOutputStream  *output,
              gint16          val,
              const gchar    *why)
{
  guchar b[2];
  gsize  bytes_written;

  /*  b[0] = val & 255;
      b[1] = (val >> 8) & 255;*/

  b[1] = val & 255;
  b[0] = (val >> 8) & 255;

  /* FIXME: Use error */
  if (! g_output_stream_write_all (output, &b, 2,
                                   &bytes_written, NULL, NULL))
    {
      g_printerr ("%s: Error while writing '%s'\n", G_STRFUNC, why);
      gimp_quit ();
    }
}

static void
write_gint32 (GOutputStream  *output,
              gint32          val,
              const gchar    *why)
{
  guchar b[4];
  gsize  bytes_written;

  b[3] = val & 255;
  b[2] = (val >> 8) & 255;
  b[1] = (val >> 16) & 255;
  b[0] = (val >> 24) & 255;

  /* FIXME: Use error */
  if (! g_output_stream_write_all (output, &b, 4,
                                   &bytes_written, NULL, NULL))
    {
      g_printerr ("%s: Error while writing '%s'\n", G_STRFUNC, why);
      gimp_quit ();
    }
}

static void
write_datablock_luni (GOutputStream  *output,
                      const gchar    *val,
                      const gchar    *why)
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

          xfwrite (output, "8BIMluni", 8, "luni xdb signature");
          write_gint32 (output, xdBlockSize, "luni xdb size");
          write_gint32 (output, len, "luni xdb pascal string");

          for (count = 0; count < len; count++)
            write_gint16 (output, luniName[count], "luni xdb pascal string");

          /* Pad to an even number of chars */
          if (len % 2)
            write_gint16 (output, 0x0000, "luni xdb pascal string padding");
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
      IFDBG(1) g_debug ("PSD Export: gimpBaseType value is %d, "
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
save_header (GOutputStream  *output,
             GimpImage      *image,
             gboolean        export_cmyk,
             gboolean        export_duotone)
{
  gint nChannels;

  IFDBG(1) g_debug ("Function: save_header\n"
                    "\tRows: %d\n"
                    "\tColumns: %d\n"
                    "\tBase type: %d\n"
                    "\tNumber of channels: %d\n",
                    PSDImageData.image_height, PSDImageData.image_width,
                    PSDImageData.baseType,     PSDImageData.nChannels);

  if (export_cmyk)
    {
      nChannels =
        gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)) ?
        5 : 4;
    }
  else
    {
      nChannels = (PSDImageData.nChannels + nChansLayer (PSDImageData.baseType,
        gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)), 0));
    }

  xfwrite (output, "8BPS", 4, "signature");
  write_gint16 (output, 1, "version");
  write_gint32 (output, 0, "reserved 1");      /* 6 for the 'reserved' field + 4 bytes for a long */
  write_gint16 (output, 0, "reserved 1");      /* and 2 bytes for a short */
  write_gint16 (output, nChannels, "channels");
  write_gint32 (output, PSDImageData.image_height, "rows");
  write_gint32 (output, PSDImageData.image_width, "columns");
  write_gint16 (output, 8 * get_bpc (image), "depth");
  if (export_cmyk)
    write_gint16 (output, PSD_CMYK, "mode");
  else if (export_duotone)
    write_gint16 (output, PSD_DUOTONE, "mode");
  else
    write_gint16 (output, gimpBaseTypeToPsdMode (PSDImageData.baseType), "mode");
}

static void
save_color_mode_data (GOutputStream  *output,
                      GimpImage      *image,
                      gboolean        export_duotone)
{
  guchar       *cmap;
  guchar       *cmap_modified;
  gint          i;
  gint32        nColors;
  GimpParasite *parasite = NULL;

  IFDBG(1) g_debug ("Function: save_color_mode_data");

  parasite = gimp_image_get_parasite (image, PSD_PARASITE_DUOTONE_DATA);
  if (export_duotone && parasite)
    {
      const guchar *parasite_data;
      guint32       parasite_size;

      IFDBG(1) g_debug ("\tImage type: DUOTONE");

      parasite_data = (const guchar *) gimp_parasite_get_data (parasite, &parasite_size);

      write_gint32 (output, parasite_size, "color data length");
      xfwrite (output, parasite_data, parasite_size, "colormap");

      gimp_parasite_free (parasite);
      return;
    }

  switch (PSDImageData.baseType)
    {
    case GIMP_INDEXED:
      IFDBG(1) g_debug ("\tImage type: INDEXED");

      cmap = gimp_image_get_colormap (image, NULL, &nColors);
      IFDBG(1) g_debug ("\t\tLength of colormap returned by gimp_image_get_colormap: %d", nColors);

      if (nColors == 0)
        {
          IFDBG(1) g_debug ("\t\tThe indexed image lacks a colormap");
          write_gint32 (output, 0, "color data length");
        }
      else if (nColors != 256)
        {
          IFDBG(1) g_debug ("\t\tThe indexed image has %d!=256 colors", nColors);
          IFDBG(1) g_debug ("\t\tPadding with zeros up to 256");
          write_gint32 (output, 768, "color data length");
            /* For this type, length is always 768 */

          cmap_modified = g_malloc (768);
          for (i = 0; i < nColors * 3; i++)
            cmap_modified[i] = cmap[i];

          for (i = nColors * 3; i < 768; i++)
            cmap_modified[i] = 0;

          reshuffle_cmap_write (cmap_modified);
          xfwrite (output, cmap_modified, 768, "colormap");  /* Write readjusted colormap */

          g_free (cmap_modified);
        }
      else         /* nColors equals 256 */
        {
          write_gint32 (output, 768, "color data length");   /* For this type, length is always 768 */
          reshuffle_cmap_write (cmap);
          xfwrite (output, cmap, 768, "colormap");  /* Write readjusted colormap */
        }
      break;

    default:
      IFDBG(1) g_debug ("\tImage type: Not INDEXED");
      write_gint32 (output, 0, "color data length");
    }
}

static void
save_resources (GOutputStream        *output,
                GimpImage            *image,
                PSD_Resource_Options *options)
{
  GList        *iter;
  gint          i;
  GList        *SelLayers;           /* The selected layers */

  goffset       eof_pos;             /* Position for End of file */
  goffset       rsc_pos;             /* Position for Lengths of Resources section */
  goffset       name_sec;            /* Position for Lengths of Channel Names */


  /* Only relevant resources in GIMP are: 0x03EE, 0x03F0 & 0x0400 */
  /* For Adobe Photoshop version 4.0 these can also be considered:
     0x0408, 0x040A & 0x040B (1006, 1008, 1024, 1032, 1034, and 1035) */

  IFDBG(1) g_debug ("Function: save_resources");


  /* Get the image title from its filename */

  IFDBG(1) g_debug ("\tImage title: %s",
                    g_file_peek_path (gimp_image_get_file (image)));

  /* Get the selected layers */

  SelLayers = gimp_image_list_selected_layers (image);

  /* Here's where actual writing starts */

  rsc_pos = g_seekable_tell (G_SEEKABLE (output));
  write_gint32 (output, 0, "image resources length");


  /* --------------- Write Channel names --------------- */
  if (! options->cmyk)
    {
      if (PSDImageData.nChannels > 0 ||
          gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)))
        {
          xfwrite (output, "8BIM", 4, "imageresources signature");
          write_gint16 (output, 0x03EE, "0x03EE Id"); /* 1006 */
          /* write_pascalstring (output, Name, "Id name"); */
          write_gint16 (output, 0, "Id name"); /* Set to null string (two zeros) */

          /* Mark current position in the file */

          name_sec = g_seekable_tell (G_SEEKABLE (output));
          write_gint32 (output, 0, "0x03EE resource size");

          /* Write all strings */

          /* if the merged_image contains transparency, write a name for it first */
          if (gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)))
            write_string (output, "Transparency", "channel name");

          for (iter = PSDImageData.lChannels; iter; iter = g_list_next (iter))
            {
              char *chName = gimp_item_get_name (iter->data);
              write_string (output, chName, "channel name");
              g_free (chName);
            }
          /* Calculate and write actual resource's length */

          eof_pos = g_seekable_tell (G_SEEKABLE (output));

          g_seekable_seek (G_SEEKABLE (output),
                           name_sec, G_SEEK_SET,
                           NULL, NULL /*FIXME: error*/);
          write_gint32 (output, eof_pos - name_sec - sizeof (gint32), "0x03EE resource size");
          IFDBG(1) g_debug ("\tTotal length of 0x03EE resource: %d",
                            (int) (eof_pos - name_sec - sizeof (gint32)));

          /* Return to EOF to continue writing */

          g_seekable_seek (G_SEEKABLE (output),
                           eof_pos, G_SEEK_SET,
                           NULL, NULL /*FIXME: error*/);

          /* Pad if length is odd */

          if ((eof_pos - name_sec - sizeof (gint32)) & 1)
            write_gchar (output, 0, "pad byte");
        }

      /* --------------- Write Channel properties --------------- */

      if (PSDImageData.nChannels > 0 ||
          gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)))
        {
          xfwrite (output, "8BIM", 4, "imageresources signature");
          write_gint16 (output, 0x0435, "0x0435 Id"); /* 1077 */
          /* write_pascalstring (output, Name, "Id name"); */
          write_gint16 (output, 0, "Id name"); /* Set to null string (two zeros) */
          write_gint32 (output,
                        4  +
                        13 * (gimp_drawable_has_alpha (
                                GIMP_DRAWABLE (PSDImageData.merged_layer)) +
                              PSDImageData.nChannels),
                        "0x0435 resource size");

          /* The function of the first 4 bytes is unclear. As per
           * load_resource_1077() in psd-image-res-load.c, it seems to be a version
           * number that is always one.
           */
          write_gint32 (output, 1, "0x0435 version");

          /* Write all channel properties */

          #define DOUBLE_TO_INT16(x) ROUND (SAFE_CLAMP (x, 0.0, 1.0) * 0xffff)

          /* if the merged_image contains transparency, write its properties first */
          if (gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)))
            {
              write_gint16 (output, PSD_CS_RGB, "channel color space");
              write_gint16 (output, (gint16) DOUBLE_TO_INT16 (1.0), "channel color r");
              write_gint16 (output, (gint16) DOUBLE_TO_INT16 (0.0), "channel color g");
              write_gint16 (output, (gint16) DOUBLE_TO_INT16 (0.0), "channel color b");
              write_gint16 (output, 0,                     "channel color padding");
              write_gint16 (output, 100,                   "channel opacity");
              write_gchar  (output, 1,                     "channel mode");
            }

          for (iter = PSDImageData.lChannels; iter; iter = g_list_next (iter))
            {
              GimpChannel *channel = iter->data;
              GeglColor   *color;
              gdouble      rgb[3];
              gdouble      opacity;

              color = gimp_channel_get_color (channel);
              opacity = gimp_channel_get_opacity (channel);

              gegl_color_get_pixel (color, babl_format ("R'G'B' double"), rgb);

              write_gint16 (output, PSD_CS_RGB,               "channel color space");
              write_gint16 (output, DOUBLE_TO_INT16 (rgb[0]), "channel color r");
              write_gint16 (output, DOUBLE_TO_INT16 (rgb[1]), "channel color g");
              write_gint16 (output, DOUBLE_TO_INT16 (rgb[2]), "channel color b");
              write_gint16 (output, 0,                        "channel color padding");
              write_gint16 (output, ROUND (opacity),          "channel opacity");
              write_gchar  (output, 1,                        "channel mode");

              g_object_unref (color);
            }

          #undef DOUBLE_TO_INT16

          /* Pad if length is odd */

          if (g_seekable_tell (G_SEEKABLE (output)) & 1)
            write_gchar (output, 0, "pad byte");
        }
    }
  /* --------------- Write Guides --------------- */
  if (gimp_image_find_next_guide (image, 0))
    {
      gint n_guides = 0;
      gint guide_id = 0;

      /* Count the guides */
      while ((guide_id = gimp_image_find_next_guide(image, guide_id)))
        n_guides++;

      xfwrite (output, "8BIM", 4, "imageresources signature");
      write_gint16 (output, 0x0408, "0x0408 Id (Guides)"); /* 1032 */
      /* write_pascalstring (output, Name, "Id name"); */
      write_gint16 (output, 0, "Id name"); /* Set to null string (two zeros) */
      write_gint32 (output, 16 + 5 * n_guides, "0x0408 resource size");
      /* Save grid and guide header */
      write_gint32 (output,   1, "grid/guide header version");
      write_gint32 (output, 576, "grid custom spacing horizontal");/* dpi*32/4??*/
      write_gint32 (output, 576, "grid custom spacing vertical");  /* dpi*32/4??*/
      write_gint32 (output, n_guides, "number of guides");

      /* write the guides */
      while ((guide_id = gimp_image_find_next_guide (image, guide_id)))
        {
          gchar orientation;
          gint32 position;
          orientation = gimp_image_get_guide_orientation (image, guide_id);
          position    = 32 * gimp_image_get_guide_position (image, guide_id);
          orientation ^= 1; /* in the psd vert =0 , horiz = 1 */
          write_gint32 (output, position, "Position of guide");
          write_gchar (output, orientation, "Orientation of guide");
          n_guides--;
        }
      if ((g_seekable_tell (G_SEEKABLE (output)) & 1))
        write_gchar(output, 0, "pad byte");
      if (n_guides != 0)
        g_warning("Screwed up guide resource:: wrong number of guides\n");
      IFDBG(1) g_debug ("\tTotal length of 0x0400 resource: %d",
                        (int) sizeof (gint16));
    }

  /* --------------- Write paths ------------------- */
  save_paths (output, image);

  if (options->clipping_path)
    {
      GimpParasite *parasite;

      save_clipping_path (output, image,
                          options->clipping_path_name,
                          options->clipping_path_flatness);

      /* Update parasites */
      parasite = gimp_parasite_new (PSD_PARASITE_CLIPPING_PATH, 0,
                                    strlen (options->clipping_path_name) + 1,
                                    options->clipping_path_name);
      gimp_image_attach_parasite (image, parasite);
      gimp_parasite_free (parasite);

      parasite = gimp_parasite_new (PSD_PARASITE_PATH_FLATNESS, 0,
                                    sizeof (gfloat),
                                    (gpointer) &options->clipping_path_flatness);
      gimp_image_attach_parasite (image, parasite);
      gimp_parasite_free (parasite);
    }

  /* --------------- Write resolution data ------------------- */
  {
    gdouble   xres = 0, yres = 0;
    guint32   xres_fix, yres_fix;
    GimpUnit *g_unit;
    gint16    psd_unit;

    g_unit = gimp_image_get_unit (image);
    gimp_image_get_resolution (image, &xres, &yres);

    if (g_unit == gimp_unit_mm ())
      {
        psd_unit = PSD_UNIT_CM;
      }
    else
      {
        psd_unit = PSD_UNIT_INCH;
      }

    /* Don't convert resolution based on g_unit which is a display unit.
     * PSD resolution is always in pixels/inch. */
    xres_fix = xres * 65536.0 + .5; /* Convert to 16.16 fixed point */
    yres_fix = yres * 65536.0 + .5; /* Convert to 16.16 fixed point */

    xfwrite (output, "8BIM", 4, "imageresources signature (for resolution)");
    write_gint16(output, 0x03ed, "0x03ed Id (resolution)"); /* 1005 */
    write_gint16 (output, 0, "Id name"); /* Set to null string (two zeros) */
    write_gint32 (output, 16, "0x0400 resource size");
    write_gint32 (output, xres_fix, "hRes (16.16 fixed point)");
    write_gint16 (output, psd_unit, "hRes unit");
    write_gint16 (output, psd_unit, "width unit");
    write_gint32 (output, yres_fix, "vRes (16.16 fixed point)");
    write_gint16 (output, psd_unit, "vRes unit");
    write_gint16 (output, psd_unit, "height unit");
  }

  /* --------------- Write Selected Layers --------------- */

  if (SelLayers)
    {
      if (g_list_length (SelLayers) == 1)
        {
          /* Write the Layer State Information (0x0400) if and only if
           * there is exactly one selected layer.
           * Unless mistaken, this block does not seem used for multiple
           * layer selected. It seems anyway redundant with the Layer
           * Selection ID(s) block (0x042D) which is more recent
           * (Photoshop CS2) but it's probably a good idea to store both
           * information.
           */
          for (iter = PSDImageData.lLayers, i = 0;
               iter;
               iter = g_list_next (iter), i++)
            {
              if (SelLayers->data == ((PSD_Layer *) iter->data)->layer)
                {
                  xfwrite (output, "8BIM", 4, "imageresources signature");
                  write_gint16 (output, 0x0400, "0x0400 Id"); /* 1024 */
                  /* write_pascalstring (output, Name, "Id name"); */
                  write_gint16 (output, 0, "Id name"); /* Set to null string (two zeros) */
                  write_gint32 (output, sizeof (gint16), "0x0400 resource size");

                  /* Layer State Information uses the layer index. */
                  write_gint16 (output, PSDImageData.nLayers - i - 1, "active layer");

                  IFDBG(1) g_debug ("\tTotal length of 0x0400 resource: %d",
                                    (int) sizeof (gint16));
                  break;
                }
            }
        }

      /* Write the Layer Selection ID(s) block when there is at least
       * one selected layer or more.
       */
      xfwrite (output, "8BIM", 4, "imageresources signature");
      write_gint16 (output, 0x042D, "0x042D Id"); /* 1069 */
      write_gint16 (output, 0, "Id name"); /* Set to null string (two zeros) */
      write_gint32 (output, sizeof (gint16) + sizeof (gint32) * g_list_length (SelLayers), "0x0400 resource size");
      write_gint16 (output, g_list_length (SelLayers), "2 bytes count");
      for (iter = SelLayers; iter; iter = iter->next)
        write_gint32 (output, GPOINTER_TO_INT (gimp_item_get_tattoo (iter->data)), "4 bytes layer ID");
    }
  g_list_free (SelLayers);

  /* --------------- Write ICC profile data ------------------- */
  {
    GimpColorProfile *profile = NULL;

    if (options->cmyk)
      {
        profile = gimp_image_get_simulation_profile (image);

        if (profile && ! gimp_color_profile_is_cmyk (profile))
          g_clear_object (&profile);
      }
    else if (! options->duotone)
      {
        profile = gimp_image_get_effective_color_profile (image);
      }

    if (profile)
      {
        const guint8 *icc_data;
        gsize         icc_length;

        icc_data = gimp_color_profile_get_icc_profile (profile, &icc_length);

        xfwrite (output, "8BIM", 4, "imageresources signature");
        write_gint16 (output, 0x040f, "0x040f Id");
        write_gint16 (output, 0, "Id name"); /* Set to null string (two zeros) */
        write_gint32 (output, icc_length, "0x040f resource size");
        xfwrite (output, icc_data, icc_length, "ICC profile");

        g_object_unref (profile);
      }
  }

  /* --------------- Write Total Section Length --------------- */

  eof_pos = g_seekable_tell (G_SEEKABLE (output));

  g_seekable_seek (G_SEEKABLE (output),
                   rsc_pos, G_SEEK_SET,
                   NULL, NULL /*FIXME: error*/);
  write_gint32 (output, eof_pos - rsc_pos - sizeof (gint32), "image resources length");
  IFDBG(1) g_debug ("\tResource section total length: %d",
                    (int) (eof_pos - rsc_pos - sizeof (gint32)));

  /* Return to EOF to continue writing */

  g_seekable_seek (G_SEEKABLE (output),
                   eof_pos, G_SEEK_SET,
                   NULL, NULL /*FIXME: error*/);
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

/* Ported /from plug-ins/file-tiff/file-tiff-export.c */
static void
double_to_psd_fixed (gdouble  value,
                     gchar   *target)
{
  gdouble in, frac;
  gint    i, f;

  frac = modf (value, &in);
  if (frac < 0)
    {
      in -= 1;
      frac += 1;
    }

  i = (gint) CLAMP (in, -16, 15);
  f = CLAMP ((gint) (frac * 0xFFFFFF), 0, 0xFFFFFF);

  target[0] = i & 0xFF;
  target[1] = (f >> 16) & 0xFF;
  target[2] = (f >>  8) & 0xFF;
  target[3] = f & 0xFF;
}

/* Ported from /plug-ins/file-tiff/file-tiff-export.c */
static void
save_paths (GOutputStream  *output,
            GimpImage      *image)
{
  gshort  id     = 0x07D0; /* Photoshop paths have IDs >= 2000 */
  gdouble width  = gimp_image_get_width (image);
  gdouble height = gimp_image_get_height (image);
  GList  *paths;
  GList  *iter;
  gint    v;
  gint    num_strokes;
  gint   *strokes;
  gint    s;

  paths = gimp_image_list_paths (image);

  if (! paths)
    return;

  /* Only up to 997 paths supported */
  for (iter = paths, v = 0;
       iter && v <= 997;
       iter = g_list_next (iter), v++)
    {
      GString *data;
      gchar   *name, *nameend;
      gsize    len;
      gint     lenpos;
      gchar    pointrecord[26] = { 0, };
      gchar   *tmpname;
      GError  *err = NULL;

      data = g_string_new ("8BIM");
      g_string_append_c (data, id / 256);
      g_string_append_c (data, id % 256);

      /*
       * - use iso8859-1 if possible
       * - otherwise use UTF-8, prepended with \xef\xbb\xbf (Byte-Order-Mark)
       */
      name = gimp_item_get_name (iter->data);
      tmpname = g_convert (name, -1, "iso8859-1", "utf-8", NULL, &len, &err);

      if (tmpname && err == NULL)
        {
          g_string_append_c (data, (gchar) MIN (len, 255));
          g_string_append_len (data, tmpname, MIN (len, 255));
          g_free (tmpname);
        }
      else
        {
          /* conversion failed, we fall back to UTF-8 */
          len = g_utf8_strlen (name, 255 - 3);  /* need three marker-bytes */

          nameend = g_utf8_offset_to_pointer (name, len);
          len = nameend - name; /* in bytes */
          g_assert (len + 3 <= 255);

          g_string_append_c (data, len + 3);
          g_string_append_len (data, "\xEF\xBB\xBF", 3); /* Unicode 0xfeff */
          g_string_append_len (data, name, len);

          if (tmpname)
            g_free (tmpname);
        }

      if (data->len % 2)  /* padding to even size */
        g_string_append_c (data, 0);
      g_free (name);

      lenpos = data->len;
      g_string_append_len (data, "\0\0\0\0", 4); /* will be filled in later */
      len = data->len; /* to calculate the data size later */

      pointrecord[1] = 6;  /* fill rule record */
      g_string_append_len (data, pointrecord, 26);

      strokes = gimp_path_get_strokes (iter->data, &num_strokes);

      for (s = 0; s < num_strokes; s++)
        {
          GimpPathStrokeType  type;
          gdouble            *points;
          gint                num_points;
          gboolean            closed;
          gint                p = 0;

          type = gimp_path_stroke_get_points (iter->data, strokes[s],
                                              &num_points, &points, &closed);

          if (type != GIMP_PATH_STROKE_TYPE_BEZIER ||
              num_points > 65535                   ||
              num_points % 6)
            {
              g_printerr ("psd-export: unsupported stroke type: "
                          "%d (%d points)\n", type, num_points);
              continue;
            }

          memset (pointrecord, 0, 26);
          pointrecord[1] = closed ? 0 : 3;
          pointrecord[2] = (num_points / 6) / 256;
          pointrecord[3] = (num_points / 6) % 256;
          g_string_append_len (data, pointrecord, 26);

          for (p = 0; p < num_points; p += 6)
            {
              pointrecord[1] = closed ? 2 : 5;

              double_to_psd_fixed (points[p+1] / height, pointrecord + 2);
              double_to_psd_fixed (points[p+0] / width,  pointrecord + 6);
              double_to_psd_fixed (points[p+3] / height, pointrecord + 10);
              double_to_psd_fixed (points[p+2] / width,  pointrecord + 14);
              double_to_psd_fixed (points[p+5] / height, pointrecord + 18);
              double_to_psd_fixed (points[p+4] / width,  pointrecord + 22);

              g_string_append_len (data, pointrecord, 26);
            }
        }

      g_free (strokes);

      /* fix up the length */

      len = data->len - len;
      data->str[lenpos + 0] = (len & 0xFF000000) >> 24;
      data->str[lenpos + 1] = (len & 0x00FF0000) >> 16;
      data->str[lenpos + 2] = (len & 0x0000FF00) >>  8;
      data->str[lenpos + 3] = (len & 0x000000FF) >>  0;

      xfwrite (output, data->str, data->len, "path resources data");
      g_string_free (data, TRUE);
      id += 0x01;
    }

  g_list_free (paths);
}

static void
save_clipping_path (GOutputStream  *output,
                    GimpImage      *image,
                    const gchar    *path_name,
                    gfloat          path_flatness)
{
  gshort   id  = 0x0BB7;
  gsize    len;
  GString *data;
  gchar   *tmpname;
  gchar    flatness[4];
  GList   *paths;
  GError  *err = NULL;

  paths = gimp_image_list_paths (image);

  if (! paths)
    return;

  data = g_string_new ("8BIM");
  g_string_append_c (data, id / 256);
  g_string_append_c (data, id % 256);

  tmpname = g_convert (path_name, -1, "iso8859-1", "utf-8", NULL, &len, &err);

  g_string_append_len (data, "\x00\x00\x00\x00", 4);
  if ((len + 6 + 1) <= 255)
    g_string_append_len (data, "\x00", 1);
  g_string_append_c (data, len + 6 + 1);

  g_string_append_c (data, (gchar) MIN (len, 255));
  g_string_append_len (data, tmpname, MIN (len, 255));
  g_free (tmpname);

  if (data->len % 2)  /* padding to even size */
    g_string_append_c (data, 0);

  double_to_psd_fixed (path_flatness, flatness);
  g_string_append_len (data, flatness, 4);

  /* Adobe specifications state they ignore the fill rule,
   * but we'll write it anyway.
   */
  g_string_append_len (data, "\x00\x01", 2);

  xfwrite (output, data->str, data->len, "clipping path resources data");
  g_string_free (data, TRUE);
}

static void
save_layer_and_mask (GOutputStream  *output,
                     GimpImage      *image,
                     gboolean        export_cmyk)
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
  gint           bpc;                    /* Image BPC */

  goffset        eof_pos;                /* Position: End of file */
  goffset        ExtraDataPos;           /* Position: Extra data length */
  goffset        LayerMaskPos;           /* Position: Layer & Mask section length */
  goffset        LayerInfoPos;           /* Position: Layer info section length*/
  goffset      **ChannelLengthPos;       /* Position: Channel length */
  GList         *iter;


  IFDBG(1) g_debug ("Function: save_layer_and_mask");

  /* Create first array dimension (layers, channels) */

  ChannelLengthPos = g_newa (goffset *, PSDImageData.nLayers);

  /* Layer and mask information section */

  LayerMaskPos = g_seekable_tell (G_SEEKABLE (output));
  write_gint32 (output, 0, "layers & mask information length");

  /* Layer info section */

  LayerInfoPos = g_seekable_tell (G_SEEKABLE (output));
  write_gint32 (output, 0, "layers info section length");

  /* Layer structure section */

  if (gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)))
    write_gint16 (output, -PSDImageData.nLayers, "Layer structure count");
  else
    write_gint16 (output, PSDImageData.nLayers, "Layer structure count");

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
          gimp_drawable_get_offsets (GIMP_DRAWABLE (psd_layer->layer),
                                     &offset_x, &offset_y);
          layerWidth = gimp_drawable_get_width (GIMP_DRAWABLE (psd_layer->layer));
          layerHeight = gimp_drawable_get_height (GIMP_DRAWABLE (psd_layer->layer));
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

      IFDBG(1)
        {
          const gchar *type;

          switch (psd_layer->type)
            {
            case PSD_LAYER_TYPE_LAYER:       type = "normal layer"; break;
            case PSD_LAYER_TYPE_GROUP_START: type = "group start marker"; break;
            case PSD_LAYER_TYPE_GROUP_END:   type = "group end marker"; break;
            default:                         type = "unknown"; break;
            }

          g_debug ("\n\tLayer number: %d\n"
                   "\t\tType: %s\n"
                   "\t\tX offset: %d\n"
                   "\t\tY offset: %d\n"
                   "\t\tWidth: %d\n"
                   "\t\tHeight: %d\n",
                   i-1, type, offset_x, offset_y,
                   layerWidth, layerHeight);
        }

      write_gint32 (output, offset_y,               "Layer top");
      write_gint32 (output, offset_x,               "Layer left");
      write_gint32 (output, offset_y + layerHeight, "Layer bottom");
      write_gint32 (output, offset_x + layerWidth,  "Layer right");

      hasMask = (psd_layer->type != PSD_LAYER_TYPE_GROUP_END &&
                 gimp_layer_get_mask (psd_layer->layer) != NULL);
      nChannelsLayer = nChansLayer (PSDImageData.baseType,
                                    gimp_drawable_has_alpha (GIMP_DRAWABLE (psd_layer->layer)),
                                    hasMask);

      /* Manually set channels to 4 or 5 when export as CMYK;
       * Can be removed once CMYK channels are accessible in GIMP
       */
      if (export_cmyk)
        {
          nChannelsLayer =
            gimp_drawable_has_alpha (GIMP_DRAWABLE (psd_layer->layer)) ?
            5 : 4;
        }

      write_gint16 (output, nChannelsLayer, "Number channels in the layer");
      IFDBG(1) g_debug ("\t\tNumber of channels: %d", nChannelsLayer);

      /* Create second array dimension (layers, channels) */

      ChannelLengthPos[i-1] = g_new (goffset, nChannelsLayer);

      /* Try with gimp_drawable_get_bpp() */

      for (j = 0; j < nChannelsLayer; j++)
        {
          if (gimp_drawable_has_alpha (GIMP_DRAWABLE (psd_layer->layer)))
            idChannel = j - 1;
          else
            idChannel = j;
          if (hasMask && (j+1 == nChannelsLayer)) /* Last channel ... */
            idChannel = -2; /* ... will be layer mask */

          write_gint16 (output, idChannel, "Channel ID");
          IFDBG(1) g_debug ("\t\t\tChannel Identifier: %d", idChannel);

          /* Write the length assuming no compression.  In case there is,
             will modify it later when writing data.  */

          ChannelLengthPos[i-1][j] = g_seekable_tell (G_SEEKABLE (output));
          ChanSize = sizeof (gint16) + (layerWidth * layerHeight * bpc);

          write_gint32 (output, ChanSize, "Channel Size");
          IFDBG(1) g_debug ("\t\t\tLength: %d", ChanSize);
        }

      xfwrite (output, "8BIM", 4, "blend mode signature");

      blendMode = psd_lmode_layer (psd_layer->layer, FALSE);
      IFDBG(1) g_debug ("\t\tBlend mode: %s", blendMode);
      xfwrite (output, blendMode, 4, "blend mode key");

      layerOpacity = RINT ((gimp_layer_get_opacity (psd_layer->layer) * 255.0) / 100.0);
      IFDBG(1) g_debug ("\t\tOpacity: %u", layerOpacity);
      write_gchar (output, layerOpacity, "Opacity");

      if (gimp_layer_get_composite_mode (psd_layer->layer) == GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP)
        write_gchar (output, 1, "Clipping");
      else
        write_gchar (output, 0, "Clipping");

      flags = 0;
      if (gimp_layer_get_lock_alpha (psd_layer->layer)) flags |= 1;
      if (! gimp_item_get_visible (GIMP_ITEM (psd_layer->layer))) flags |= 2;
      if (psd_layer->type != PSD_LAYER_TYPE_LAYER) flags |= 0x18;
      IFDBG(1) g_debug ("\t\tFlags: %u", flags);
      write_gchar (output, flags, "Flags");

      /* Padding byte to make the length even */
      write_gchar (output, 0, "Filler");

      ExtraDataPos = g_seekable_tell (G_SEEKABLE (output)); /* Position of Extra Data size */
      write_gint32 (output, 0, "Extra data size");

      if (hasMask)
        {
          gint     maskOffset_x;
          gint     maskOffset_y;
          gint     maskWidth;
          gint     maskHeight;
          gboolean apply;

          mask = gimp_layer_get_mask (psd_layer->layer);

          gimp_drawable_get_offsets (GIMP_DRAWABLE (mask), &maskOffset_x, &maskOffset_y);

          maskWidth  = gimp_drawable_get_width  (GIMP_DRAWABLE (mask));
          maskHeight = gimp_drawable_get_height (GIMP_DRAWABLE (mask));
          apply      = gimp_layer_get_apply_mask (psd_layer->layer);

          IFDBG(1) g_debug ("\t\tLayer mask size: %d", 20);
          write_gint32 (output, 20,                        "Layer mask size");
          write_gint32 (output, maskOffset_y,              "Layer mask top");
          write_gint32 (output, maskOffset_x,              "Layer mask left");
          write_gint32 (output, maskOffset_y + maskHeight, "Layer mask bottom");
          write_gint32 (output, maskOffset_x + maskWidth,  "Layer mask right");
          write_gchar  (output, 0,                         "Layer mask default color");
          flags = (0                    |  /* position relative to layer */
                   (apply ? 0 : 1) << 1 |  /* layer mask disabled        */
                   0 << 2);                /* invert layer mask          */
          write_gchar  (output, flags,                     "Layer mask flags");
          write_gint16 (output, 0,                         "Layer mask Padding");
        }
      else
        {
          /* NOTE Writing empty Layer mask / adjustment layer data */
          write_gint32 (output, 0, "Layer mask size");
          IFDBG(1) g_debug ("\t\tLayer mask size: %d", 0);
        }

      /* NOTE Writing empty Layer blending ranges data */
      write_gint32 (output, 0, "Layer blending size");
      IFDBG(1) g_debug ("\t\tLayer blending size: %d", 0);

      if (psd_layer->type != PSD_LAYER_TYPE_GROUP_END)
        layerName = gimp_item_get_name (GIMP_ITEM (psd_layer->layer));
      else
        layerName = g_strdup ("</Layer group>");
      write_pascalstring (output, layerName, 4, "layer name");
      IFDBG(1) g_debug ("\t\tLayer name: %s", layerName);

      /* Additional layer information blocks */
      /* Unicode layer name */
      write_datablock_luni (output, layerName, "luni extra data block");

      g_free (layerName);

      /* Layer ID */
      xfwrite (output, "8BIMlyid", 8, "lyid signature");
      write_gint32 (output, 4, "lyid size");
      write_gint32 (output, gimp_item_get_tattoo (GIMP_ITEM (psd_layer->layer)), "Layer ID");

      /* Layer color tag */
      xfwrite (output, "8BIMlclr", 8, "sheet color signature");
      write_gint32 (output, 8, "sheet color size");
      write_gint16 (output,
                    gimp_to_psd_layer_color_tag (gimp_item_get_color_tag (GIMP_ITEM (psd_layer->layer))),
                    "sheet color code");
      write_gint16 (output, 0, "sheet color unused value");
      write_gint16 (output, 0, "sheet color unused value");
      write_gint16 (output, 0, "sheet color unused value");

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
              xfwrite (output, "8BIMlsct", 8, "section divider");
            }
          else
            {
              /* layer groups whose nesting depth is above 5 are only supported
               * by Photoshop CS5 and up, and their end markers use the
               * (undocumented) "lsdk" key, instead of "lsct".
               */
              xfwrite (output, "8BIMlsdk", 8, "nested section divider");
            }
          write_gint32 (output, size, "section divider size");
          write_gint32 (output, type, "section divider type");
          xfwrite (output, "8BIM", 4, "section divider blend mode signature");
          xfwrite (output, blendMode, 4, "section divider blend mode key");
        }

      /* Write real length for: Extra data */

      eof_pos = g_seekable_tell (G_SEEKABLE (output));

      g_seekable_seek (G_SEEKABLE (output),
                       ExtraDataPos, G_SEEK_SET,
                       NULL, NULL /*FIXME: error*/);
      write_gint32 (output, eof_pos - ExtraDataPos - sizeof (gint32), "Extra data size");
      IFDBG(1) g_debug ("\t\tExtraData size: %d",
                        (int) (eof_pos - ExtraDataPos - sizeof (gint32)));

      /* Return to EOF to continue writing */

      g_seekable_seek (G_SEEKABLE (output),
                       eof_pos, G_SEEK_SET,
                       NULL, NULL /*FIXME: error*/);
    }


  /* Channel image data section */
  /* Gimp layers must be written in reverse order */

  for (iter = g_list_last (PSDImageData.lLayers), i = PSDImageData.nLayers;
       iter;
       iter = g_list_previous (iter), i--)
    {
      PSD_Layer *psd_layer = (PSD_Layer *) iter->data;

      gimp_progress_update ((PSDImageData.nLayers - i - 1.0) / (PSDImageData.nLayers + 1.0));

      IFDBG(1) g_debug ("\t\tWriting pixel data for layer slot %d", i-1);
      write_pixel_data (output, image, GIMP_DRAWABLE (psd_layer->layer), ChannelLengthPos[i-1], 0,
                        psd_layer->type != PSD_LAYER_TYPE_GROUP_END, export_cmyk);
      g_free (ChannelLengthPos[i-1]);
    }

  gimp_progress_update (PSDImageData.nLayers / (PSDImageData.nLayers + 1.0));
  eof_pos = g_seekable_tell (G_SEEKABLE (output));

  /* Write actual size of Layer info section */

  g_seekable_seek (G_SEEKABLE (output),
                   LayerInfoPos, G_SEEK_SET,
                   NULL, NULL /*FIXME: error*/);
  write_gint32 (output, eof_pos - LayerInfoPos - sizeof (gint32), "layers info section length");
  IFDBG(1) g_debug ("\t\tTotal layers info section length: %d",
                    (int) (eof_pos - LayerInfoPos - sizeof (gint32)));

  /* Write actual size of Layer and mask information section */

  g_seekable_seek (G_SEEKABLE (output),
                   LayerMaskPos, G_SEEK_SET,
                   NULL, NULL /*FIXME: error*/);
  write_gint32 (output, eof_pos - LayerMaskPos - sizeof (gint32), "layers & mask information length");
  IFDBG(1) g_debug ("\t\tTotal layers & mask information length: %d",
                    (int) (eof_pos - LayerMaskPos - sizeof (gint32)));

  /* Return to EOF to continue writing */

  g_seekable_seek (G_SEEKABLE (output),
                   eof_pos, G_SEEK_SET,
                   NULL, NULL /*FIXME: error*/);
}

static void
write_pixel_data (GOutputStream  *output,
                  GimpImage      *image,
                  GimpDrawable   *drawable,
                  goffset        *ChanLenPosition,
                  goffset         ltable_offset,
                  gboolean        write_mask,
                  gboolean        export_cmyk)
{
  GeglBuffer       *buffer = gimp_drawable_get_buffer (drawable);
  const Babl       *format;
  const Babl       *space  = NULL;
  const Babl       *type;
  GimpColorProfile *profile;
  GimpLayerMask    *mask;
  gint32            tile_height = gimp_tile_height ();
  gint32            height = gegl_buffer_get_height (buffer);
  gint32            width  = gegl_buffer_get_width (buffer);
  gint32            bytes;
  gint32            components;
  gint32            bpc;
  gint32            colors;
  gint32            y;
  gsize             len;                  /* Length of compressed data */
  gint16           *LengthsTable;         /* Lengths of every compressed row */
  guchar           *rledata;              /* Compressed data from a region */
  guchar           *data;                 /* Temporary copy of pixel data */
  goffset           length_table_pos;     /* position in file of the length table */
  int               i, j;

  IFDBG(1) g_debug ("Function: write_pixel_data, drw %d, lto %" G_GOFFSET_FORMAT,
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

  if (export_cmyk && ! gimp_item_is_channel (GIMP_ITEM (drawable)))
    {
      profile = gimp_image_get_simulation_profile (image);
      if (profile && gimp_color_profile_is_cmyk (profile))
        space = gimp_color_profile_get_space (profile,
                                              gimp_image_get_simulation_intent (image),
                                              NULL);
      if (profile)
        g_object_unref (profile);

      if (get_bpc (image) == 1)
        type = babl_type ("u8");
      else
        type = babl_type ("u16");

      if (gimp_drawable_has_alpha (drawable))
        format = babl_format_new (babl_model ("cmykA"),
                                  type,
                                  babl_component ("cyan"),
                                  babl_component ("magenta"),
                                  babl_component ("yellow"),
                                  babl_component ("key"),
                                  babl_component ("A"),
                                  NULL);
      else
        format = babl_format_new (babl_model ("cmyk"),
                                  type,
                                  babl_component ("cyan"),
                                  babl_component ("magenta"),
                                  babl_component ("yellow"),
                                  babl_component ("key"),
                                  NULL);

      format = babl_format_with_space (babl_format_get_encoding (format),
                                       space);
    }

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
          write_gint16 (output, 1, "Compression type (RLE)");
          len += 2;
        }

      if (ltable_offset > 0)
        {
          length_table_pos = ltable_offset + 2 * chan * height;
        }
      else
        {
          length_table_pos = g_seekable_tell (G_SEEKABLE (output));

          xfwrite (output, LengthsTable, height * sizeof(gint16),
                   "Dummy RLE length");
          len += height * sizeof(gint16);
          IFDBG(3) g_debug ("\t\t\t\t. ltable, pos %" G_GOFFSET_FORMAT
                            " len %" G_GSIZE_FORMAT,
                            length_table_pos, len);
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
          xfwrite (output, rledata, tlen, "Compressed pixel data");
          IFDBG(3) g_debug ("\t\t\t\t. Writing compressed pixels, stream of %d", tlen);
        }

      /* Write compressed lengths table */
      g_seekable_seek (G_SEEKABLE (output),
                       length_table_pos, G_SEEK_SET,
                       NULL, NULL /*FIXME: error*/);
      for (j = 0; j < height; j++) /* write real length table */
        write_gint16 (output, LengthsTable[j], "RLE length");

      if (ChanLenPosition)    /* Update total compressed length */
        {
          g_seekable_seek (G_SEEKABLE (output),
                           ChanLenPosition[i], G_SEEK_SET,
                           NULL, NULL /*FIXME: error*/);
          write_gint32 (output, len, "channel data length");
          IFDBG(1) g_debug ("\t\tUpdating data len to %" G_GSIZE_FORMAT, len);
        }
      g_seekable_seek (G_SEEKABLE (output),
                       0, G_SEEK_END,
                       NULL, NULL /*FIXME: error*/);
      IFDBG(3) g_debug ("\t\t\t\t. Cur pos %" G_GOFFSET_FORMAT, g_seekable_tell (G_SEEKABLE (output)));
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
          write_gint16 (output, 1, "Compression type (RLE)");
          len += 2;
          IFDBG(3) g_debug ("\t\t\t\t. ChanLenPos, len %" G_GSIZE_FORMAT, len);
        }

      if (ltable_offset > 0)
        {
          length_table_pos = ltable_offset + 2 * (components+1) * height;
          IFDBG(3) g_debug ("\t\t\t\t. ltable, pos %" G_GOFFSET_FORMAT,
                            length_table_pos);
        }
      else
        {
          length_table_pos = g_seekable_tell (G_SEEKABLE (output));

          xfwrite (output, LengthsTable, height * sizeof(gint16),
                   "Dummy RLE length");
          len += height * sizeof(gint16);
          IFDBG(3) g_debug ("\t\t\t\t. ltable, pos %" G_GOFFSET_FORMAT
                            " len %" G_GSIZE_FORMAT,
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
          xfwrite (output, rledata, tlen, "Compressed mask data");
          IFDBG(3) g_debug ("\t\t\t\t. Writing compressed mask, stream of %d", tlen);
        }

      /* Write compressed lengths table */
      g_seekable_seek (G_SEEKABLE (output),
                       length_table_pos, G_SEEK_SET,
                       NULL, NULL /*FIXME: error*/);
      for (j = 0; j < height; j++) /* write real length table */
        {
          write_gint16 (output, LengthsTable[j], "RLE length");
          IFDBG(3) g_debug ("\t\t\t\t. Updating RLE len %d",
                            LengthsTable[j]);
        }

      if (ChanLenPosition)    /* Update total compressed length */
        {
          /* Mask follows other components so use that as offset. */
          g_seekable_seek (G_SEEKABLE (output),
                           ChanLenPosition[components], G_SEEK_SET,
                           NULL, NULL /*FIXME: error*/);

          write_gint32 (output, len, "channel data length");
          IFDBG(1) g_debug ("\t\tUpdating data len to %" G_GSIZE_FORMAT
                            ", at %" G_GOFFSET_FORMAT,
                            len, g_seekable_tell (G_SEEKABLE (output)));
        }
      g_seekable_seek (G_SEEKABLE (output),
                       0, G_SEEK_END,
                       NULL, NULL /*FIXME: error*/);
      IFDBG(3) g_debug ("\t\t\t\t. Cur pos %" G_GOFFSET_FORMAT,
                        g_seekable_tell (G_SEEKABLE (output)));

      g_object_unref (mbuffer);
    }

  g_object_unref (buffer);

  g_free (data);
  g_free (rledata);
  g_free (LengthsTable);
}

static void
save_data (GOutputStream  *output,
           GimpImage      *image,
           gboolean        export_cmyk)
{
  GList  *iter;
  gint    ChanCount;
  gint    i, j;
  gint32  imageHeight;                   /* Height of image */
  goffset offset;                        /* offset in file of rle lengths */
  gint    chan;

  IFDBG(1) g_debug ("Function: save_data");

  ChanCount = (PSDImageData.nChannels +
               nChansLayer (PSDImageData.baseType,
                            gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)),
                            0));

  imageHeight = gimp_image_get_height (image);

  write_gint16 (output, 1, "RLE compression");

  /* All line lengths go before the rle pixel data */

  offset = g_seekable_tell (G_SEEKABLE (output)); /* Offset in file of line lengths */

  for (i = 0; i < ChanCount; i++)
    for (j = 0; j < imageHeight; j++)
      write_gint16 (output, 0, "junk line lengths");

  IFDBG(1) g_debug ("\t\tWriting compressed image data");
  write_pixel_data (output, image, GIMP_DRAWABLE (PSDImageData.merged_layer),
                    NULL, offset, FALSE, export_cmyk);

  chan = nChansLayer (PSDImageData.baseType,
                      gimp_drawable_has_alpha (GIMP_DRAWABLE (PSDImageData.merged_layer)), 0);

  for (iter = PSDImageData.lChannels; iter; iter = g_list_next (iter))
    {
      IFDBG(1) g_debug ("\t\tWriting compressed channel data for channel %d", i);
      write_pixel_data (output, image, iter->data, NULL,
                        offset + 2*imageHeight*chan, FALSE,
                        export_cmyk); //check how imgs are channels here
      chan++;
    }
}

static GimpLayer *
create_merged_image (GimpImage *image)
{
  GimpLayer *projection;

  projection = gimp_layer_new_from_visible (image, image, "psd-export");

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (projection)))
    return projection;

  if (gimp_image_get_base_type (image) != GIMP_INDEXED)
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
  IFDBG(1) g_debug ("Function: get_image_data");

  PSDImageData.compression = FALSE;

  PSDImageData.image_height = gimp_image_get_height (image);
  IFDBG(1) g_debug ("\tGot number of rows: %d", PSDImageData.image_height);

  PSDImageData.image_width = gimp_image_get_width (image);
  IFDBG(1) g_debug ("\tGot number of cols: %d", PSDImageData.image_width);

  PSDImageData.baseType = gimp_image_get_base_type (image);
  IFDBG(1) g_debug ("\tGot base type: %d", PSDImageData.baseType);

  PSDImageData.merged_layer = create_merged_image (image);

  PSDImageData.lChannels = gimp_image_list_channels (image);
  PSDImageData.nChannels = g_list_length (PSDImageData.lChannels);
  IFDBG(1) g_debug ("\tGot number of channels: %d", PSDImageData.nChannels);

  PSDImageData.lLayers = image_get_all_layers (image,
                                               &PSDImageData.nLayers);
  IFDBG(1) g_debug ("\tGot number of layers: %d", PSDImageData.nLayers);
}

static void
clear_image_data (void)
{
  IFDBG(1) g_debug ("Function: clear_image_data");

  g_list_free (PSDImageData.lChannels);
  PSDImageData.lChannels = NULL;

  g_list_free (PSDImageData.lLayers);
  PSDImageData.lLayers = NULL;
}

gboolean
export_image (GFile      *file,
              GimpImage  *image,
              GObject    *config,
              GError    **error)
{
  GOutputStream        *output;
  GeglBuffer           *buffer;
  GList                *iter;
  GError               *local_error = NULL;
  GimpParasite         *parasite    = NULL;
  PSD_Resource_Options  resource_options;

  g_object_get (config,
                "cmyk",                 &resource_options.cmyk,
                "duotone",              &resource_options.duotone,
                "clippingpath",         &resource_options.clipping_path,
                "clippingpathname",     &resource_options.clipping_path_name,
                "clippingpathflatness", &resource_options.clipping_path_flatness,
                NULL);

  IFDBG(1) g_debug ("Function: export_image");

  if (resource_options.cmyk)
    resource_options.duotone = FALSE;

  if (gimp_image_get_width (image) > 30000 ||
      gimp_image_get_height (image) > 30000)
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

  /* If image is not grayscale or lacks Duotone color space parasite,
   * turn off "Export as Duotone".
   */
  parasite = gimp_image_get_parasite (image, PSD_PARASITE_DUOTONE_DATA);
  if (parasite)
    {
      if (gimp_image_get_base_type (image) != GIMP_GRAY)
        resource_options.duotone = FALSE;

      gimp_parasite_free (parasite);
    }
  else
    {
      resource_options.duotone = FALSE;
    }

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

  output = G_OUTPUT_STREAM (g_file_replace (file, NULL, FALSE,
                                            G_FILE_CREATE_NONE, NULL,
                                            &local_error));

  if (! output)
    {
      if (! local_error)
        g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                     _("Could not open '%s' for writing: %s"),
                       gimp_file_get_utf8_name (file), g_strerror (errno));
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Could not open '%s' for reading: %s"),
                       gimp_file_get_utf8_name (file), local_error->message);
          g_error_free (local_error);
        }
      clear_image_data ();
      return FALSE;
    }

  IFDBG(1) g_debug ("\tFile '%s' has been opened",
                    gimp_file_get_utf8_name (file));

  save_header (output, image, resource_options.cmyk, resource_options.duotone);
  save_color_mode_data (output, image, resource_options.duotone);
  save_resources (output, image, &resource_options);

  /* PSD format does not support layers in indexed images */

  if (PSDImageData.baseType == GIMP_INDEXED)
    write_gint32 (output, 0, "layers info section length");
  else
    save_layer_and_mask (output, image, resource_options.cmyk);

  /* If this is an indexed image, write now channel and layer info */

  save_data (output, image, resource_options.cmyk);

  /* Delete merged image now */

  gimp_item_delete (GIMP_ITEM (PSDImageData.merged_layer));

  clear_image_data ();

  IFDBG(1) g_debug ("----- Closing PSD file, done -----\n");

  g_object_unref (output);
  g_free (resource_options.clipping_path_name);

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

gboolean
save_dialog (GimpImage     *image,
             GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget        *dialog;
  GtkWidget        *duotone_notice;
  GtkWidget        *label;
  gchar            *text;
  GtkWidget        *profile_label;
  GimpColorProfile *cmyk_profile;
  GimpParasite     *parasite         = NULL;
  gboolean          has_duotone_data = FALSE;
  GList            *paths;
  gboolean          run;

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Export Image as PSD"));

  /* CMYK profile label */
  profile_label = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                                   "profile-label", _("No soft-proofing profile"),
                                                   FALSE, FALSE);
  gtk_label_set_xalign (GTK_LABEL (profile_label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (profile_label), PANGO_ELLIPSIZE_END);
  gimp_label_set_attributes (GTK_LABEL (profile_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gimp_help_set_help_data (profile_label,
                           _("Name of the color profile used for CMYK export."), NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "cmyk-frame", "cmyk", FALSE,
                                    "profile-label");
  cmyk_profile = gimp_image_get_simulation_profile (image);
  if (cmyk_profile)
    {
      if (gimp_color_profile_is_cmyk (cmyk_profile))
        {
          gchar *label_text;

          label_text = g_strdup_printf (_("Profile: %s"),
                                        gimp_color_profile_get_label (cmyk_profile));
          gtk_label_set_text (GTK_LABEL (profile_label), label_text);
          gimp_label_set_attributes (GTK_LABEL (profile_label),
                                     PANGO_ATTR_STYLE, PANGO_STYLE_NORMAL,
                                     -1);
          g_free (label_text);
        }
      g_object_unref (cmyk_profile);
    }

  /* Only show dialog if image is grayscale and duotone color space
   * information was attached from the original image imported
   */
   parasite = gimp_image_get_parasite (image, PSD_PARASITE_DUOTONE_DATA);
   if (parasite)
     {
       if (gimp_image_get_base_type (image) == GIMP_GRAY)
         {
           has_duotone_data = TRUE;

           /* Duotone Option label */
           duotone_notice = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                                             "duotone-notice",
                                                             _("Duotone color space information "
                                                             "from the original\nimported image "
                                                             "will be used."),
                                                             FALSE, FALSE);
           gtk_label_set_xalign (GTK_LABEL (duotone_notice), 0.0);
           gtk_label_set_ellipsize (GTK_LABEL (duotone_notice), PANGO_ELLIPSIZE_END);
           gimp_label_set_attributes (GTK_LABEL (duotone_notice),
                                      PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                      -1);

           gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                             "duotone-frame", "duotone", FALSE,
                                             "duotone-notice");

           /* Prevent you from setting both Duotone and CMYK exports */
           gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                                "duotone",
                                                TRUE, config, "cmyk", TRUE);
           gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                                "cmyk",
                                                TRUE, config, "duotone", TRUE);

         }

       gimp_parasite_free (parasite);
     }

  /* Clipping Path */
  paths = gimp_image_list_paths (image);
  if (paths)
    {
      GtkWidget    *vbox;
      GtkWidget    *frame;
      GtkWidget    *entry;
      GtkWidget    *combo;
      GtkWidget    *label;
      GList        *list;
      GtkTreeModel *model;
      GtkTreeIter   iter;
      gint          v;
      gint          saved_id  = -1;
      gchar        *path_name = NULL;

      parasite = gimp_image_get_parasite (image, PSD_PARASITE_CLIPPING_PATH);
      if (parasite)
        {
          guint32  parasite_size;

          path_name = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
          path_name = g_strndup (path_name, parasite_size);

          gimp_parasite_free (parasite);
        }
      else
        {
          /* Uncheck clipping path if no parasite data saved */
          g_object_set (config,
                        "clippingpath", FALSE,
                        NULL);
        }

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      combo = gimp_path_combo_box_new (NULL, NULL, NULL);

      /* Alert user if they have more than 998 paths */
      if (g_list_length (paths) > 998)
        {
          label = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                                   "path-warning",
                                                   _("PSD files can store up to "
                                                     "998 paths. \nThe rest "
                                                     "will be discarded."),
                                                   FALSE, FALSE);
           gtk_label_set_xalign (GTK_LABEL (label), 0.0);
           gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
           gimp_label_set_attributes (GTK_LABEL (label),
                                      PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                      -1);
          gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                      "path-warning",
                                      NULL);
          gtk_widget_show (label);
        }

      /* Fixing labels */
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_list_store_clear (GTK_LIST_STORE (model));
      for (list = paths, v = 0;
           list && v <= 997;
           list = g_list_next (list), v++)
        {
          gtk_list_store_append (GTK_LIST_STORE (model), &iter);
          gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                              GIMP_INT_STORE_VALUE,     gimp_item_get_id (list->data),
                              GIMP_INT_STORE_LABEL,     gimp_item_get_name (list->data),
                              GIMP_INT_STORE_PIXBUF,    NULL,
                              GIMP_INT_STORE_USER_DATA, gimp_item_get_name (list->data),
                              -1);

          if (! g_strcmp0 (gimp_item_get_name (list->data),
                           path_name))
            saved_id = gimp_item_get_id (list->data);
        }

      if (saved_id != -1)
        gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                       saved_id);

      gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                  0,
                                  G_CALLBACK (update_clipping_path),
                                  config, NULL);
      gtk_widget_show (combo);

      entry = gimp_procedure_dialog_get_spin_scale (GIMP_PROCEDURE_DIALOG (dialog),
                                                    "clippingpathflatness", 1.0);

      parasite = gimp_image_get_parasite (image, PSD_PARASITE_PATH_FLATNESS);
      if (parasite)
        {
          gfloat  *path_flatness = NULL;
          guint32  parasite_size;

          path_flatness = (gfloat *) gimp_parasite_get_data (parasite, &parasite_size);
          if (path_flatness && *path_flatness > 0)
            {
              gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), *path_flatness);
              g_object_set (config,
                            "clippingpathflatness", *path_flatness,
                            NULL);
            }

          gimp_parasite_free (parasite);
        }

      gtk_widget_show (entry);

      gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                        "clipping-path-frame", "clippingpath", FALSE,
                                        NULL);
      frame = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                                "clipping-path-subframe", NULL, FALSE,
                                                NULL);

      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
      gtk_widget_show (vbox);

      gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                  "clipping-path-frame",
                                  "clipping-path-subframe",
                                  NULL);

      gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                           "clipping-path-subframe",
                                           TRUE, config, "clippingpath", FALSE);
    }

  if (has_duotone_data)
    gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                "cmyk-frame",
                                "duotone-frame",
                                NULL);
  else
    gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                "cmyk-frame",
                                NULL);

/* Multi-layer Indexed Image Warning */
  if (gimp_image_get_base_type (image) == GIMP_INDEXED)
    {
      gint32 n_layers = 0;

      g_free (gimp_image_get_layers (image, &n_layers));

      if (n_layers > 1)
        {
            text = g_strdup_printf ("\n<b>%s</b>: %s",
                          _("Indexed Image Warning"),
                          _("Photoshop does not support indexed images "
                            "that have more than one layer. Layers will "
                            "be merged on export."));

            label = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                                     "indexed-notice", "Indexed Image Warning",
                                                     FALSE, FALSE);
            gtk_label_set_markup (GTK_LABEL (label), text);
            gtk_label_set_xalign (GTK_LABEL (label), 0.0);
            gtk_label_set_max_width_chars (GTK_LABEL (label), 50);
            gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

            gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                        "indexed-notice", NULL);
            g_free (text);
        }
    }

  /* Compatibility Notice */
  text = g_strdup_printf ("\n<b>%s</b>: %s",
                          _("Compatibility Notice"),
                          _("Legacy layer modes have better compatibility "
                            "with Photoshop, but do not support Clip to "
                            "Backdrop, which is needed for using Photoshop's "
                            "'Blend Clipped Layers as Group'. "
                            "If you encounter display issues, consider "
                            "switching to those layer modes."));

  label = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                           "compat-notice", "Compatibility Notice",
                                           FALSE, FALSE);
  gtk_label_set_markup (GTK_LABEL (label), text);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_max_width_chars (GTK_LABEL (label), 50);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "compat-notice", NULL);
  g_free (text);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void update_clipping_path (GimpIntComboBox *combo,
                                  gpointer         data)
{
  gpointer value;

  if (gimp_int_combo_box_get_active_user_data (GIMP_INT_COMBO_BOX (combo),
                                               &value))
    {
      GObject *config = G_OBJECT (data);
      g_object_set (config,
                    "clippingpathname", (gchar *) value,
                    NULL);
    }
}
