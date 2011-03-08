/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP PSD Plug-in
 * Copyright 2007 by John Marshall
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* ----- Known Image Resource Block Types -----
  All image resources not otherwise handled, including unknown types
  are added as image parasites.
  The data is attached as-is from the file (i.e. in big endian order).

  PSD_PS2_IMAGE_INFO    = 1000,    Dropped    * 0x03e8 - Obsolete - ps 2.0 image info *
  PSD_MAC_PRINT_INFO    = 1001,    PS Only    * 0x03e9 - Optional - Mac print manager print info record *
  PSD_PS2_COLOR_TAB     = 1003,    Dropped    * 0x03eb - Obsolete - ps 2.0 indexed colour table *
  PSD_RESN_INFO         = 1005,    Loaded     * 0x03ed - ResolutionInfo structure *
  PSD_ALPHA_NAMES       = 1006,    Loaded     * 0x03ee - Alpha channel names *
  PSD_DISPLAY_INFO      = 1007,    Loaded     * 0x03ef - DisplayInfo structure *
  PSD_CAPTION           = 1008,    Loaded     * 0x03f0 - Optional - Caption string *
  PSD_BORDER_INFO       = 1009,               * 0x03f1 - Border info *
  PSD_BACKGROUND_COL    = 1010,               * 0x03f2 - Background colour *
  PSD_PRINT_FLAGS       = 1011,               * 0x03f3 - Print flags *
  PSD_GREY_HALFTONE     = 1012,               * 0x03f4 - Greyscale and multichannel halftoning info *
  PSD_COLOR_HALFTONE    = 1013,               * 0x03f5 - Colour halftoning info *
  PSD_DUOTONE_HALFTONE  = 1014,               * 0x03f6 - Duotone halftoning info *
  PSD_GREY_XFER         = 1015,               * 0x03f7 - Greyscale and multichannel transfer functions *
  PSD_COLOR_XFER        = 1016,               * 0x03f8 - Colour transfer functions *
  PSD_DUOTONE_XFER      = 1017,               * 0x03f9 - Duotone transfer functions *
  PSD_DUOTONE_INFO      = 1018,               * 0x03fa - Duotone image information *
  PSD_EFFECTIVE_BW      = 1019,               * 0x03fb - Effective black & white values for dot range *
  PSD_OBSOLETE_01       = 1020,    Dropped    * 0x03fc - Obsolete *
  PSD_EPS_OPT           = 1021,               * 0x03fd - EPS options *
  PSD_QUICK_MASK        = 1022,    Loaded     * 0x03fe - Quick mask info *
  PSD_OBSOLETE_02       = 1023,    Dropped    * 0x03ff - Obsolete *
  PSD_LAYER_STATE       = 1024,    Loaded     * 0x0400 - Layer state info *
  PSD_WORKING_PATH      = 1025,               * 0x0401 - Working path (not saved) *
  PSD_LAYER_GROUP       = 1026,               * 0x0402 - Layers group info *
  PSD_OBSOLETE_03       = 1027,    Dropped    * 0x0403 - Obsolete *
  PSD_IPTC_NAA_DATA     = 1028,    Loaded     * 0x0404 - IPTC-NAA record (IMV4.pdf) *
  PSD_IMAGE_MODE_RAW    = 1029,               * 0x0405 - Image mode for raw format files *
  PSD_JPEG_QUAL         = 1030,    PS Only    * 0x0406 - JPEG quality *
  PSD_GRID_GUIDE        = 1032,    Loaded     * 0x0408 - Grid & guide info *
  PSD_THUMB_RES         = 1033,    Special    * 0x0409 - Thumbnail resource *
  PSD_COPYRIGHT_FLG     = 1034,               * 0x040a - Copyright flag *
  PSD_URL               = 1035,               * 0x040b - URL string *
  PSD_THUMB_RES2        = 1036,    Special    * 0x040c - Thumbnail resource *
  PSD_GLOBAL_ANGLE      = 1037,               * 0x040d - Global angle *
  PSD_COLOR_SAMPLER     = 1038,               * 0x040e - Colour samplers resource *
  PSD_ICC_PROFILE       = 1039,    Loaded     * 0x040f - ICC Profile *
  PSD_WATERMARK         = 1040,               * 0x0410 - Watermark *
  PSD_ICC_UNTAGGED      = 1041,               * 0x0411 - Do not use ICC profile flag *
  PSD_EFFECTS_VISIBLE   = 1042,               * 0x0412 - Show   hide all effects layers *
  PSD_SPOT_HALFTONE     = 1043,               * 0x0413 - Spot halftone *
  PSD_DOC_IDS           = 1044,               * 0x0414 - Document specific IDs *
  PSD_ALPHA_NAMES_UNI   = 1045,    Loaded     * 0x0415 - Unicode alpha names *
  PSD_IDX_COL_TAB_CNT   = 1046,    Loaded     * 0x0416 - Indexed colour table count *
  PSD_IDX_TRANSPARENT   = 1047,               * 0x0417 - Index of transparent colour (if any) *
  PSD_GLOBAL_ALT        = 1049,               * 0x0419 - Global altitude *
  PSD_SLICES            = 1050,               * 0x041a - Slices *
  PSD_WORKFLOW_URL_UNI  = 1051,               * 0x041b - Workflow URL - Unicode string *
  PSD_JUMP_TO_XPEP      = 1052,               * 0x041c - Jump to XPEP (?) *
  PSD_ALPHA_ID          = 1053,    Loaded     * 0x041d - Alpha IDs *
  PSD_URL_LIST_UNI      = 1054,               * 0x041e - URL list - unicode *
  PSD_VERSION_INFO      = 1057,               * 0x0421 - Version info *
  PSD_EXIF_DATA         = 1058,    Loaded     * 0x0422 - Exif data block *
  PSD_XMP_DATA          = 1060,    Loaded     * 0x0424 - XMP data block *
  PSD_PATH_INFO_FIRST   = 2000,    Loaded     * 0x07d0 - First path info block *
  PSD_PATH_INFO_LAST    = 2998,    Loaded     * 0x0bb6 - Last path info block *
  PSD_CLIPPING_PATH     = 2999,               * 0x0bb7 - Name of clipping path *
  PSD_PRINT_FLAGS_2     = 10000               * 0x2710 - Print flags *
*/

#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>
#include <libgimp/gimp.h>

#include <jpeglib.h>
#include <jerror.h>

#ifdef HAVE_EXIF
#include <libexif/exif-data.h>
#endif /* HAVE_EXIF */
#ifdef HAVE_IPTCDATA
#include <libiptcdata/iptc-data.h>
#endif /* HAVE_IPTCDATA */

#include "psd.h"
#include "psd-util.h"
#include "psd-image-res-load.h"

#include "libgimp/stdplugins-intl.h"

#define EXIF_HEADER_SIZE 8

/*  Local function prototypes  */
static gint     load_resource_unknown  (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_ps_only  (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1005     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1006     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        PSDimage              *img_a,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1007     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        PSDimage              *img_a,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1008     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1022     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        PSDimage              *img_a,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1024     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        PSDimage              *img_a,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1028     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1032     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1033     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1039     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1045     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        PSDimage              *img_a,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1046     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1053     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        PSDimage              *img_a,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1058     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_1060     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        FILE                  *f,
                                        GError               **error);

static gint     load_resource_2000     (const PSDimageres     *res_a,
                                        const gint32           image_id,
                                        FILE                  *f,
                                        GError               **error);

/* Public Functions */
gint
get_image_resource_header (PSDimageres  *res_a,
                           FILE         *f,
                           GError      **error)
{
  gint32        read_len;
  gint32        write_len;
  gchar        *name;

  if (fread (&res_a->type, 4, 1, f) < 1
      || fread (&res_a->id, 2, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  res_a->id = GUINT16_FROM_BE (res_a->id);
  name = fread_pascal_string (&read_len, &write_len, 2, f, error);
  if (*error)
    return -1;
  if (name != NULL)
    g_strlcpy (res_a->name, name, write_len + 1);
  else
    res_a->name[0] = 0x0;
  g_free (name);
  if (fread (&res_a->data_len, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  res_a->data_len = GUINT32_FROM_BE (res_a->data_len);
  res_a->data_start = ftell (f);

  IFDBG(2) g_debug ("Type: %.4s, id: %d, start: %d, len: %d",
                    res_a->type, res_a->id, res_a->data_start, res_a->data_len);

  return 0;
}

gint
load_image_resource (PSDimageres   *res_a,
                     const gint32   image_id,
                     PSDimage      *img_a,
                     FILE          *f,
                     GError       **error)
{
  gint  pad;

  /* Set file position to start of image resource data block */
  if (fseek (f, res_a->data_start, SEEK_SET) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

   /* Process image resource blocks */
  if (memcmp (res_a->type, "8BIM", 4) != 0 &&
      memcmp (res_a->type, "MeSa", 4) !=0)
    {
      IFDBG(1) g_debug ("Unknown image resource type signature %.4s",
                        res_a->type);
    }
  else
    {
      switch (res_a->id)
        {
          case PSD_PS2_IMAGE_INFO:
          case PSD_PS2_COLOR_TAB:
          case PSD_OBSOLETE_01:
          case PSD_OBSOLETE_02:
          case PSD_OBSOLETE_03:
            /* Drop obsolete image resource blocks */
            IFDBG(2) g_debug ("Obsolete image resource block: %d",
                               res_a->id);
            break;

          case PSD_THUMB_RES:
          case PSD_THUMB_RES2:
            /* Drop thumbnails from standard file load */
            IFDBG(2) g_debug ("Thumbnail resource block: %d",
                               res_a->id);
            break;

          case PSD_MAC_PRINT_INFO:
          case PSD_JPEG_QUAL:
            /* Save photoshop resources with no meaning for GIMP
              as image parasites */
            load_resource_ps_only (res_a, image_id, f, error);
            break;

          case PSD_RESN_INFO:
            load_resource_1005 (res_a, image_id, f, error);
            break;

          case PSD_ALPHA_NAMES:
            load_resource_1006 (res_a, image_id, img_a, f, error);
            break;

          case PSD_DISPLAY_INFO:
            load_resource_1007 (res_a, image_id, img_a, f, error);
            break;

          case PSD_CAPTION:
            load_resource_1008 (res_a, image_id, f, error);
            break;

          case PSD_QUICK_MASK:
            load_resource_1022 (res_a, image_id, img_a, f, error);
            break;

          case PSD_LAYER_STATE:
            load_resource_1024 (res_a, image_id, img_a, f, error);
            break;

          case PSD_IPTC_NAA_DATA:
            load_resource_1028 (res_a, image_id, f, error);
            break;

          case PSD_GRID_GUIDE:
            load_resource_1032 (res_a, image_id, f, error);
            break;

          case PSD_ICC_PROFILE:
            load_resource_1039 (res_a, image_id, f, error);
            break;

          case PSD_ALPHA_NAMES_UNI:
            load_resource_1045 (res_a, image_id, img_a, f, error);
            break;

          case PSD_IDX_COL_TAB_CNT:
            load_resource_1046 (res_a, image_id, f, error);
            break;

          case PSD_ALPHA_ID:
            load_resource_1053 (res_a, image_id, img_a, f, error);
            break;

          case PSD_EXIF_DATA:
            load_resource_1058 (res_a, image_id, f, error);
            break;

          case PSD_XMP_DATA:
            load_resource_1060 (res_a, image_id, f, error);
            break;

          default:
            if (res_a->id >= 2000 &&
                res_a->id <  2999)
              load_resource_2000 (res_a, image_id, f, error);
            else
              load_resource_unknown (res_a, image_id, f, error);
        }
    }

  /* Image blocks are null padded to even length */
  if (res_a->data_len % 2 == 0)
    pad = 0;
  else
    pad = 1;

  /* Set file position to end of image resource block */
  if (fseek (f, res_a->data_start + res_a->data_len + pad, SEEK_SET) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  return 0;
}

gint
load_thumbnail_resource (PSDimageres   *res_a,
                         const gint32   image_id,
                         FILE          *f,
                         GError       **error)
{
  gint  rtn = 0;
  gint  pad;

  /* Set file position to start of image resource data block */
  if (fseek (f, res_a->data_start, SEEK_SET) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

   /* Process image resource blocks */
 if (res_a->id == PSD_THUMB_RES
     || res_a->id == PSD_THUMB_RES2)
   {
        /* Load thumbnails from standard file load */
        load_resource_1033 (res_a, image_id, f, error);
        rtn = 1;
   }

  /* Image blocks are null padded to even length */
  if (res_a->data_len % 2 == 0)
    pad = 0;
  else
    pad = 1;

  /* Set file position to end of image resource block */
  if (fseek (f, res_a->data_start + res_a->data_len + pad, SEEK_SET) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  return rtn;
}

/* Private Functions */

static gint
load_resource_unknown (const PSDimageres  *res_a,
                       const gint32        image_id,
                       FILE               *f,
                       GError            **error)
{
  /* Unknown image resources attached as parasites to re-save later */
  GimpParasite  *parasite;
  gchar         *data;
  gchar         *name;

  IFDBG(2) g_debug ("Process unknown image resource block: %d", res_a->id);

  data = g_malloc (res_a->data_len);
  if (fread (data, res_a->data_len, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      g_free (data);
      return -1;
    }

  name = g_strdup_printf ("psd-image-resource-%.4s-%.4x",
                            res_a->type, res_a->id);
  IFDBG(2) g_debug ("Parasite name: %s", name);

  parasite = gimp_parasite_new (name, 0, res_a->data_len, data);
  gimp_image_attach_parasite (image_id, parasite);
  gimp_parasite_free (parasite);
  g_free (data);
  g_free (name);

  return 0;
}

static gint
load_resource_ps_only (const PSDimageres  *res_a,
                       const gint32        image_id,
                       FILE               *f,
                       GError            **error)
{
  /* Save photoshop resources with no meaning for GIMP as image parasites
     to re-save later */
  GimpParasite  *parasite;
  gchar         *data;
  gchar         *name;

  IFDBG(3) g_debug ("Process image resource block: %d", res_a->id);

  data = g_malloc (res_a->data_len);
  if (fread (data, res_a->data_len, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      g_free (data);
      return -1;
    }

  name = g_strdup_printf ("psd-image-resource-%.4s-%.4x",
                            res_a->type, res_a->id);
  IFDBG(2) g_debug ("Parasite name: %s", name);

  parasite = gimp_parasite_new (name, 0, res_a->data_len, data);
  gimp_image_attach_parasite (image_id, parasite);
  gimp_parasite_free (parasite);
  g_free (data);
  g_free (name);

  return 0;
}

static gint
load_resource_1005 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    FILE               *f,
                    GError            **error)
{
  /* Load image resolution and unit of measure */

  /* FIXME  width unit and height unit unused at present */

  ResolutionInfo        res_info;
  GimpUnit              image_unit;

  IFDBG(2) g_debug ("Process image resource block 1005: Resolution Info");

  if (fread (&res_info.hRes, 4, 1, f) < 1
      || fread (&res_info.hResUnit, 2, 1, f) < 1
      || fread (&res_info.widthUnit, 2, 1, f) < 1
      || fread (&res_info.vRes, 4, 1, f) < 1
      || fread (&res_info.vResUnit, 2, 1, f) < 1
      || fread (&res_info.heightUnit, 2, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  res_info.hRes = GINT32_FROM_BE (res_info.hRes);
  res_info.hResUnit = GINT16_FROM_BE (res_info.hResUnit);
  res_info.widthUnit = GINT16_FROM_BE (res_info.widthUnit);
  res_info.vRes = GINT32_FROM_BE (res_info.vRes);
  res_info.vResUnit = GINT16_FROM_BE (res_info.vResUnit);
  res_info.heightUnit = GINT16_FROM_BE (res_info.heightUnit);

  IFDBG(3) g_debug ("Resolution: %d, %d, %d, %d, %d, %d",
                      res_info.hRes,
                      res_info.hResUnit,
                      res_info.widthUnit,
                      res_info.vRes,
                      res_info.vResUnit,
                      res_info.heightUnit);

  /* Resolution always recorded as pixels / inch in a fixed point implied
     decimal int32 with 16 bits before point and 16 after (i.e. cast as
     double and divide resolution by 2^16 */
  gimp_image_set_resolution (image_id,
                             res_info.hRes / 65536.0, res_info.vRes / 65536.0);

  /* GIMP only has one display unit so use ps horizontal resolution unit */
  switch (res_info.hResUnit)
    {
      case PSD_RES_INCH:
        image_unit = GIMP_UNIT_INCH;
        break;
      case PSD_RES_CM:
        image_unit = GIMP_UNIT_MM;
        break;
      default:
        image_unit = GIMP_UNIT_INCH;
    }

  gimp_image_set_unit (image_id, image_unit);

  return 0;
}

static gint
load_resource_1006 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    PSDimage           *img_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load alpha channel names stored as a series of pascal strings
     unpadded between strings */

  gchar        *str;
  gint32        block_rem;
  gint32        read_len;
  gint32        write_len;

  IFDBG(2) g_debug ("Process image resource block 1006: Alpha Channel Names");

  if (img_a->alpha_names)
    {
      IFDBG(3) g_debug ("Alpha names loaded from unicode resource block");
      return 0;
    }

  img_a->alpha_names = g_ptr_array_new ();

  block_rem = res_a->data_len;
  while (block_rem > 1)
    {
      str = fread_pascal_string (&read_len, &write_len, 1, f, error);
      if (*error)
        return -1;
      IFDBG(3) g_debug ("String: %s, %d, %d", str, read_len, write_len);
      if (write_len >= 0)
        {
          g_ptr_array_add (img_a->alpha_names, (gpointer) str);
        }
      block_rem -= read_len;
    }

  return 0;
}

static gint
load_resource_1007 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    PSDimage           *img_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load alpha channel display info */

  DisplayInfo       dsp_info;
  CMColor           ps_color;
  GimpRGB           gimp_rgb;
  GimpHSV           gimp_hsv;
  GimpCMYK          gimp_cmyk;
  gint16            tot_rec;
  gint              cidx;

  IFDBG(2) g_debug ("Process image resource block 1007: Display Info");
  tot_rec = res_a->data_len / 14;
  if (tot_rec == 0)
    return 0;

  img_a->alpha_display_info = g_new (PSDchanneldata *, tot_rec);
  img_a->alpha_display_count = tot_rec;
  for (cidx = 0; cidx < tot_rec; ++cidx)
    {
      if (fread (&dsp_info.colorSpace, 2, 1, f) < 1
          || fread (&dsp_info.color, 8, 1, f) < 1
          || fread (&dsp_info.opacity, 2, 1, f) < 1
          || fread (&dsp_info.kind, 1, 1, f) < 1
          || fread (&dsp_info.padding, 1, 1, f) < 1)
        {
          psd_set_error (feof (f), errno, error);
          return -1;
        }
      dsp_info.colorSpace = GINT16_FROM_BE (dsp_info.colorSpace);
      ps_color.cmyk.cyan = GUINT16_FROM_BE (dsp_info.color[0]);
      ps_color.cmyk.magenta = GUINT16_FROM_BE (dsp_info.color[1]);
      ps_color.cmyk.yellow = GUINT16_FROM_BE (dsp_info.color[2]);
      ps_color.cmyk.black = GUINT16_FROM_BE (dsp_info.color[3]);
      dsp_info.opacity = GINT16_FROM_BE (dsp_info.opacity);

      switch (dsp_info.colorSpace)
        {
          case PSD_CS_RGB:
            gimp_rgb_set (&gimp_rgb, ps_color.rgb.red / 65535.0,
                          ps_color.rgb.green / 65535.0,
                          ps_color.rgb.blue / 65535.0);
            break;

          case PSD_CS_HSB:
            gimp_hsv_set (&gimp_hsv, ps_color.hsv.hue / 65535.0,
                          ps_color.hsv.saturation / 65535.0,
                          ps_color.hsv.value / 65535.0);
            gimp_hsv_to_rgb (&gimp_hsv, &gimp_rgb);
            break;

          case PSD_CS_CMYK:
            gimp_cmyk_set (&gimp_cmyk, 1.0 - ps_color.cmyk.cyan / 65535.0,
                           1.0 - ps_color.cmyk.magenta / 65535.0,
                           1.0 - ps_color.cmyk.yellow / 65535.0,
                           1.0 - ps_color.cmyk.black / 65535.0);
            gimp_cmyk_to_rgb (&gimp_cmyk, &gimp_rgb);
            break;

          case PSD_CS_GRAYSCALE:
            gimp_rgb_set (&gimp_rgb, ps_color.gray.gray / 10000.0,
                          ps_color.gray.gray / 10000.0,
                          ps_color.gray.gray / 10000.0);
            break;

          case PSD_CS_FOCOLTONE:
          case PSD_CS_TRUMATCH:
          case PSD_CS_HKS:
          case PSD_CS_LAB:
          case PSD_CS_PANTONE:
          case PSD_CS_TOYO:
          case PSD_CS_DIC:
          case PSD_CS_ANPA:
          default:
            if (CONVERSION_WARNINGS)
              g_message ("Unsupported color space: %d",
                         dsp_info.colorSpace);
            gimp_rgb_set (&gimp_rgb, 1.0, 0.0, 0.0);
        }

      gimp_rgb_set_alpha (&gimp_rgb, 1.0);

      IFDBG(2) g_debug ("PS cSpace: %d, col: %d %d %d %d, opacity: %d, kind: %d",
             dsp_info.colorSpace, ps_color.cmyk.cyan, ps_color.cmyk.magenta,
             ps_color.cmyk.yellow, ps_color.cmyk.black, dsp_info.opacity,
             dsp_info.kind);

      IFDBG(2) g_debug ("cSpace: %d, col: %g %g %g, opacity: %d, kind: %d",
             dsp_info.colorSpace, gimp_rgb.r * 255 , gimp_rgb.g * 255,
             gimp_rgb.b * 255, dsp_info.opacity, dsp_info.kind);

      img_a->alpha_display_info[cidx] = g_malloc (sizeof (PSDchanneldata));
      img_a->alpha_display_info[cidx]->gimp_color = gimp_rgb;
      img_a->alpha_display_info[cidx]->opacity = dsp_info.opacity;
      img_a->alpha_display_info[cidx]->ps_kind = dsp_info.kind;
      img_a->alpha_display_info[cidx]->ps_cspace = dsp_info.colorSpace;
      img_a->alpha_display_info[cidx]->ps_color = ps_color;
    }

  return 0;
}

static gint
load_resource_1008 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    FILE               *f,
                    GError            **error)
{
  /* Load image caption */
  GimpParasite  *parasite;
  gchar         *caption;
  gint32         read_len;
  gint32         write_len;

  IFDBG(2) g_debug ("Process image resource block: 1008: Caption");
  caption = fread_pascal_string (&read_len, &write_len, 1, f, error);
  if (*error)
    return -1;

  IFDBG(3) g_debug ("Caption: %s", caption);
  parasite = gimp_parasite_new (GIMP_PARASITE_COMMENT, GIMP_PARASITE_PERSISTENT,
                                write_len, caption);
  gimp_image_attach_parasite (image_id, parasite);
  gimp_parasite_free (parasite);
  g_free (caption);

  return 0;
}

static gint
load_resource_1022 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    PSDimage           *img_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load quick mask info */
  gboolean              quick_mask_empty;       /* Quick mask initially empty */

  IFDBG(2) g_debug ("Process image resource block: 1022: Quick Mask");

  if (fread (&img_a->quick_mask_id, 2, 1, f) < 1
      || fread (&quick_mask_empty, 1, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  img_a->quick_mask_id = GUINT16_FROM_BE (img_a->quick_mask_id);

  IFDBG(3) g_debug ("Quick mask channel: %d, empty: %d",
                      img_a->quick_mask_id,
                      quick_mask_empty);

  return 0;
}

static gint
load_resource_1024 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    PSDimage           *img_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load image layer state - current active layer counting from bottom up */
  IFDBG(2) g_debug ("Process image resource block: 1024: Layer State");

  if (fread (&img_a->layer_state, 2, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  img_a->layer_state = GUINT16_FROM_BE (img_a->layer_state);

  return 0;
}

static gint
load_resource_1028 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    FILE               *f,
                    GError            **error)
{
  /* Load IPTC data block */

#ifdef HAVE_IPTCDATA
  IptcData     *iptc_data;
  guchar       *iptc_buf;
  guint         iptc_buf_len;
#else
  gchar        *name;
#endif /* HAVE_IPTCDATA */

  GimpParasite *parasite;
  gchar        *res_data;

  IFDBG(2) g_debug ("Process image resource block: 1028: IPTC data");

  res_data = g_malloc (res_a->data_len);
  if (fread (res_data, res_a->data_len, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      g_free (res_data);
      return -1;
    }

#ifdef HAVE_IPTCDATA
  /* Load IPTC data structure */
  iptc_data = iptc_data_new_from_data (res_data, res_a->data_len);
  IFDBG (3) iptc_data_dump (iptc_data, 0);

  /* Store resource data as a GIMP IPTC parasite */
  IFDBG (2) g_debug ("Processing IPTC data as GIMP IPTC parasite");
  /* Serialize IPTC data */
  iptc_data_save (iptc_data, &iptc_buf, &iptc_buf_len);
  if (iptc_buf_len > 0)
    {
      parasite = gimp_parasite_new (GIMP_PARASITE_IPTC,
                                    GIMP_PARASITE_PERSISTENT,
                                    iptc_buf_len, iptc_buf);
      gimp_image_attach_parasite (image_id, parasite);
      gimp_parasite_free (parasite);
    }

  iptc_data_unref (iptc_data);
  g_free (iptc_buf);

#else
  /* Store resource data as a standard psd parasite */
  IFDBG (2) g_debug ("Processing IPTC data as psd parasite");
  name = g_strdup_printf ("psd-image-resource-%.4s-%.4x",
                           res_a->type, res_a->id);
  IFDBG(3) g_debug ("Parasite name: %s", name);

  parasite = gimp_parasite_new (name, 0, res_a->data_len, res_data);
  gimp_image_attach_parasite (image_id, parasite);
  gimp_parasite_free (parasite);
  g_free (name);

#endif /* HAVE_IPTCDATA */

  g_free (res_data);
  return 0;
}

static gint
load_resource_1032 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    FILE               *f,
                    GError            **error)
{
  /* Load grid and guides */

  /* Grid info is not used (CS2 or earlier) */

  GuideHeader           hdr;
  GuideResource         guide;
  gint                  i;

  IFDBG(2) g_debug ("Process image resource block 1032: Grid and Guide Info");

  if (fread (&hdr.fVersion, 4, 1, f) < 1
      || fread (&hdr.fGridCycleV, 4, 1, f) < 1
      || fread (&hdr.fGridCycleH, 4, 1, f) < 1
      || fread (&hdr.fGuideCount, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  hdr.fVersion = GUINT32_FROM_BE (hdr.fVersion);
  hdr.fGridCycleV = GUINT32_FROM_BE (hdr.fGridCycleV);
  hdr.fGridCycleH = GUINT32_FROM_BE (hdr.fGridCycleH);
  hdr.fGuideCount = GUINT32_FROM_BE (hdr.fGuideCount);

  IFDBG(3) g_debug ("Grids & Guides: %d, %d, %d, %d",
                     hdr.fVersion,
                     hdr.fGridCycleV,
                     hdr.fGridCycleH,
                     hdr.fGuideCount);

  for (i = 0; i < hdr.fGuideCount; ++i)
    {
      if (fread (&guide.fLocation, 4, 1, f) < 1
          || fread (&guide.fDirection, 1, 1, f) < 1)
        {
          psd_set_error (feof (f), errno, error);
          return -1;
        }
      guide.fLocation = GUINT32_FROM_BE (guide.fLocation);
      guide.fLocation /= 32;

      IFDBG(3) g_debug ("Guide: %d px, %d",
                         guide.fLocation,
                         guide.fDirection);

      if (guide.fDirection == PSD_VERTICAL)
        gimp_image_add_vguide (image_id, guide.fLocation);
      else
        gimp_image_add_hguide (image_id, guide.fLocation);
    }

  return 0;
}

static gint
load_resource_1033 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    FILE               *f,
                    GError            **error)
{
  /* Load thumbnail image */

  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr         jerr;

  ThumbnailInfo         thumb_info;
  GimpDrawable         *drawable;
  GimpPixelRgn          pixel_rgn;
  gint32                layer_id;
  guchar               *buf;
  guchar               *rgb_buf;
  guchar              **rowbuf;
  gint                  i;

  IFDBG(2) g_debug ("Process image resource block %d: Thumbnail Image", res_a->id);

  /* Read thumbnail resource header info */
  if (fread (&thumb_info.format, 4, 1, f) < 1
      || fread (&thumb_info.width, 4, 1, f) < 1
      || fread (&thumb_info.height, 4, 1, f) < 1
      || fread (&thumb_info.widthbytes, 4, 1, f) < 1
      || fread (&thumb_info.size, 4, 1, f) < 1
      || fread (&thumb_info.compressedsize, 4, 1, f) < 1
      || fread (&thumb_info.bitspixel, 2, 1, f) < 1
      || fread (&thumb_info.planes, 2, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  thumb_info.format = GINT32_FROM_BE (thumb_info.format);
  thumb_info.width = GINT32_FROM_BE (thumb_info.width);
  thumb_info.height = GINT32_FROM_BE (thumb_info.height);
  thumb_info.widthbytes = GINT32_FROM_BE (thumb_info.widthbytes);
  thumb_info.size = GINT32_FROM_BE (thumb_info.size);
  thumb_info.compressedsize = GINT32_FROM_BE (thumb_info.compressedsize);
  thumb_info.bitspixel = GINT16_FROM_BE (thumb_info.bitspixel);
  thumb_info.planes = GINT16_FROM_BE (thumb_info.planes);

  IFDBG(2) g_debug ("\nThumbnail:\n"
                    "\tFormat: %d\n"
                    "\tDimensions: %d x %d\n",
                      thumb_info.format,
                      thumb_info.width,
                      thumb_info.height);

  if (thumb_info.format != 1)
    {
      IFDBG(1) g_debug ("Unknown thumbnail format %d", thumb_info.format);
      return -1;
    }

  /* Load Jpeg RGB thumbnail info */

  /* Step 1: Allocate and initialize JPEG decompression object */
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_decompress (&cinfo);

  /* Step 2: specify data source (eg, a file) */
  jpeg_stdio_src(&cinfo, f);

  /* Step 3: read file parameters with jpeg_read_header() */
  jpeg_read_header (&cinfo, TRUE);

  /* Step 4: set parameters for decompression */


  /* Step 5: Start decompressor */
  jpeg_start_decompress (&cinfo);

  /* temporary buffers */
  buf = g_new (guchar, cinfo.output_height * cinfo.output_width
               * cinfo.output_components);
  if (res_a->id == PSD_THUMB_RES)
    rgb_buf = g_new (guchar, cinfo.output_height * cinfo.output_width
                     * cinfo.output_components);
  else
    rgb_buf = NULL;
  rowbuf = g_new (guchar *, cinfo.output_height);

  for (i = 0; i < cinfo.output_height; ++i)
    rowbuf[i] = buf + cinfo.output_width * cinfo.output_components * i;

  /* Create image layer */
  gimp_image_resize (image_id, cinfo.output_width, cinfo.output_height, 0, 0);
  layer_id = gimp_layer_new (image_id, _("Background"),
                             cinfo.output_width,
                             cinfo.output_height,
                             GIMP_RGB_IMAGE, 100, GIMP_NORMAL_MODE);
  drawable = gimp_drawable_get (layer_id);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                       drawable->width, drawable->height, TRUE, FALSE);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */
  while (cinfo.output_scanline < cinfo.output_height)
    {
      jpeg_read_scanlines (&cinfo,
                           (JSAMPARRAY) &rowbuf[cinfo.output_scanline], 1);

      if (res_a->id == PSD_THUMB_RES)   /* Order is BGR for resource 1033 */
        {
          guchar *dst = rgb_buf;
          guchar *src = buf;

          for (i = 0; i < drawable->width * drawable->height; ++i)
            {
              guchar r, g, b;

              r = *(src++);
              g = *(src++);
              b = *(src++);
              *(dst++) = b;
              *(dst++) = g;
              *(dst++) = r;
            }
        }
      gimp_pixel_rgn_set_rect (&pixel_rgn, rgb_buf ? rgb_buf : buf,
                               0, 0, drawable->width, drawable->height);
    }

  /* Step 7: Finish decompression */
  jpeg_finish_decompress (&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */
  jpeg_destroy_decompress (&cinfo);

  /* free up the temporary buffers */
  g_free (rowbuf);
  g_free (buf);
  g_free (rgb_buf);

  /* At this point you may want to check to see whether any
   * corrupt-data warnings occurred (test whether
   * jerr.num_warnings is nonzero).
   */
  gimp_image_insert_layer (image_id, layer_id, -1, 0);
  gimp_drawable_detach (drawable);

  return 0;
}

static gint
load_resource_1039 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    FILE               *f,
                    GError            **error)
{
  /* Load ICC profile */
  GimpParasite  *parasite;
  gchar         *icc_profile;

  IFDBG(2) g_debug ("Process image resource block: 1039: ICC Profile");

  icc_profile = g_malloc (res_a->data_len);
  if (fread (icc_profile, res_a->data_len, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      g_free (icc_profile);
      return -1;
    }

  parasite = gimp_parasite_new (GIMP_PARASITE_ICC_PROFILE,
                                GIMP_PARASITE_PERSISTENT,
                                res_a->data_len, icc_profile);
  gimp_image_attach_parasite (image_id, parasite);
  gimp_parasite_free (parasite);
  g_free (icc_profile);

  return 0;
}

static gint
load_resource_1045 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    PSDimage           *img_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load alpha channel names stored as a series of unicode strings
     in a GPtrArray */

  gchar        *str;
  gint32        block_rem;
  gint32        read_len;
  gint32        write_len;

  IFDBG(2) g_debug ("Process image resource block 1045: Unicode Alpha Channel Names");

  if (img_a->alpha_names)
    {
      gint      i;
      IFDBG(3) g_debug ("Deleting localised alpha channel names");
      for (i = 0; i < img_a->alpha_names->len; ++i)
        {
          str = g_ptr_array_index (img_a->alpha_names, i);
          g_free (str);
        }
      g_ptr_array_free (img_a->alpha_names, TRUE);
    }

  img_a->alpha_names = g_ptr_array_new ();

  block_rem = res_a->data_len;
  while (block_rem > 1)
    {
      str = fread_unicode_string (&read_len, &write_len, 1, f, error);
      if (*error)
        return -1;

      IFDBG(3) g_debug ("String: %s, %d, %d", str, read_len, write_len);
      if (write_len >= 0)
        {
          g_ptr_array_add (img_a->alpha_names, (gpointer) str);
        }
      block_rem -= read_len;
    }

  return 0;
}

static gint
load_resource_1046 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    FILE               *f,
                    GError            **error)
{
  /* Load indexed color table count */
  guchar       *cmap;
  gint32        cmap_count = 0;
  gint16        index_count = 0;

  IFDBG(2) g_debug ("Process image resource block: 1046: Indexed Color Table Count");

  if (fread (&index_count, 2, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  index_count = GINT16_FROM_BE (index_count);

  IFDBG(3) g_debug ("Indexed color table count: %d", index_count);
  /* FIXME - check that we have indexed image */
  if (index_count && index_count < 256)
    {
      cmap = gimp_image_get_colormap (image_id, &cmap_count);
      if (cmap && index_count < cmap_count)
        gimp_image_set_colormap (image_id, cmap, index_count);
      g_free (cmap);
    }
  return 0;
}

static gint
load_resource_1053 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    PSDimage           *img_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load image alpha channel ids (tattoos) */
  gint16        tot_rec;
  gint16        cidx;

  IFDBG(2) g_debug ("Process image resource block: 1053: Channel ID");

  tot_rec = res_a->data_len / 4;
  if (tot_rec ==0)
    return 0;

  img_a->alpha_id = g_malloc (sizeof (img_a->alpha_id) * tot_rec);
  img_a->alpha_id_count = tot_rec;
  for (cidx = 0; cidx < tot_rec; ++cidx)
    {
      if (fread (&img_a->alpha_id[cidx], 4, 1, f) < 1)
        {
          psd_set_error (feof (f), errno, error);
          return -1;
        }
      img_a->alpha_id[cidx] = GUINT32_FROM_BE (img_a->alpha_id[cidx]);

      IFDBG(3) g_debug ("Channel id: %d", img_a->alpha_id[cidx]);
    }

  return 0;
}

static gint
load_resource_1058 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    FILE               *f,
                    GError            **error)
{
  /* Load EXIF data block */

#ifdef HAVE_EXIF
  ExifData     *exif_data;
  ExifEntry    *exif_entry;
  guchar       *exif_buf;
  guchar       *tmp_data;
  guint         exif_buf_len;
  gint16        jpeg_len;
  gint16        jpeg_fill = 0;
  GimpParam    *return_vals;
  gint          nreturn_vals;
#else
  gchar        *name;
#endif /* HAVE_EXIF */

  GimpParasite *parasite;
  gchar        *res_data;

  IFDBG(2) g_debug ("Process image resource block: 1058: EXIF data");

  res_data = g_malloc (res_a->data_len);
  if (fread (res_data, res_a->data_len, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      g_free (res_data);
      return -1;
    }

#ifdef HAVE_EXIF
  /* Add JPEG header & trailer to the TIFF Exif data held in PSD
     resource to allow us to use libexif to serialize the data
     in the same manner as the JPEG load.
  */
  jpeg_len = res_a->data_len + 8;
  tmp_data = g_malloc (res_a->data_len + 12);
  /* SOI & APP1 markers */
  memcpy (tmp_data, "\xFF\xD8\xFF\xE1", 4);
  /* APP1 block len */
  memcpy (tmp_data + 4, &jpeg_len, 2);
  /* Exif marker */
  memcpy (tmp_data + 6, "Exif", 4);
  /* Filler */
  memcpy (tmp_data + 10, &jpeg_fill, 2);
  /* Exif data */
  memcpy (tmp_data + 12, res_data, res_a->data_len);

  /* Create Exif data structure */
  exif_data = exif_data_new_from_data (tmp_data, res_a->data_len + 12);
  g_free (tmp_data);
  IFDBG (3) exif_data_dump (exif_data);

  /* Check for XMP data block in Exif data - PS7 */
  if ((exif_entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_0],
                                            EXIF_TAG_XML_PACKET)))
    {
      IFDBG(3) g_debug ("Processing Exif XMP data block");
      /*Create NULL terminated EXIF data block */
      tmp_data = g_malloc (exif_entry->size + 1);
      memcpy (tmp_data, exif_entry->data, exif_entry->size);
      tmp_data[exif_entry->size] = 0;
      /* Merge with existing XMP data block */
      return_vals = gimp_run_procedure (DECODE_XMP_PROC,
                                        &nreturn_vals,
                                        GIMP_PDB_IMAGE,  image_id,
                                        GIMP_PDB_STRING, tmp_data,
                                        GIMP_PDB_END);
      g_free (tmp_data);
      gimp_destroy_params (return_vals, nreturn_vals);
      IFDBG(3) g_debug ("Deleting XMP block from Exif data");
      /* Delete XMP data from Exif block */
      exif_content_remove_entry (exif_data->ifd[EXIF_IFD_0],
                                 exif_entry);
    }

  /* Check for Photoshp Image Resource data block in Exif data - PS7 */
  if ((exif_entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_0],
                                            EXIF_TAG_IMAGE_RESOURCES)))
    {
      IFDBG(3) g_debug ("Deleting PS Image Resource block from Exif data");
      /* Delete PS Image Resource data from Exif block */
      exif_content_remove_entry (exif_data->ifd[EXIF_IFD_0],
                                 exif_entry);
    }

  IFDBG (3) exif_data_dump (exif_data);
  /* Store resource data as a GIMP Exif parasite */
  IFDBG (2) g_debug ("Processing exif data as GIMP Exif parasite");
  /* Serialize exif data */
  exif_data_save_data (exif_data, &exif_buf, &exif_buf_len);
  if (exif_buf_len > EXIF_HEADER_SIZE)
    {
      parasite = gimp_parasite_new (GIMP_PARASITE_EXIF,
                                    GIMP_PARASITE_PERSISTENT,
                                    exif_buf_len, exif_buf);
      gimp_image_attach_parasite (image_id, parasite);
      gimp_parasite_free (parasite);
    }
  exif_data_unref (exif_data);
  g_free (exif_buf);

#else
  /* Store resource data as a standard psd parasite */
  IFDBG (2) g_debug ("Processing exif data as psd parasite");
  name = g_strdup_printf ("psd-image-resource-%.4s-%.4x",
                           res_a->type, res_a->id);
  IFDBG(3) g_debug ("Parasite name: %s", name);

  parasite = gimp_parasite_new (name, 0, res_a->data_len, res_data);
  gimp_image_attach_parasite (image_id, parasite);
  gimp_parasite_free (parasite);
  g_free (name);

#endif /* HAVE_EXIF */

  g_free (res_data);
  return 0;
}

static gint
load_resource_1060 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    FILE               *f,
                    GError            **error)
{
  /* Load XMP Metadata block */
  GimpParam    *return_vals;
  gint          nreturn_vals;
  gchar        *res_data;

  IFDBG(2) g_debug ("Process image resource block: 1060: XMP Data");

  res_data = g_malloc (res_a->data_len + 1);
  if (fread (res_data, res_a->data_len, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      g_free (res_data);
      return -1;
    }
  /* Null terminate metadata block for decode procedure */
  res_data[res_a->data_len] = 0;

  return_vals = gimp_run_procedure (DECODE_XMP_PROC,
                                    &nreturn_vals,
                                    GIMP_PDB_IMAGE,  image_id,
                                    GIMP_PDB_STRING, res_data,
                                    GIMP_PDB_END);
  g_free (res_data);
  gimp_destroy_params (return_vals, nreturn_vals);
  return 0;
}

static gint
load_resource_2000 (const PSDimageres  *res_a,
                    const gint32        image_id,
                    FILE               *f,
                    GError            **error)
{
  gdouble      *controlpoints;
  gint32        x[3];
  gint32        y[3];
  gint32        vector_id = -1;
  gint16        type;
  gint16        init_fill;
  gint16        num_rec;
  gint16        path_rec;
  gint16        cntr;
  gint          image_width;
  gint          image_height;
  gint          i;
  gboolean      closed;
  gboolean      fill;

  /* Load path data from image resources 2000-2998 */

  IFDBG(2) g_debug ("Process image resource block: %d :Path data", res_a->id);
  path_rec = res_a->data_len / 26;
  if (path_rec ==0)
    return 0;

  if (fread (&type, 2, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  type = GINT16_FROM_BE (type);
  if (type != PSD_PATH_FILL_RULE)
    {
      IFDBG(1) g_debug ("Unexpected path record type: %d", type);
      return -1;
    }
  else
    fill = FALSE;

  if (fseek (f, 24, SEEK_CUR) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  path_rec--;
  if (path_rec ==0)
    return 0;

  image_width = gimp_image_width (image_id);
  image_height = gimp_image_height (image_id);

  /* Create path */
  vector_id = gimp_vectors_new (image_id, res_a->name);
  gimp_image_insert_vectors (image_id, vector_id, -1, -1);

  while (path_rec > 0)
    {
      if (fread (&type, 2, 1, f) < 1)
        {
          psd_set_error (feof (f), errno, error);
          return -1;
        }
      type = GINT16_FROM_BE (type);
      IFDBG(3) g_debug ("Path record type %d", type);

      if (type == PSD_PATH_FILL_RULE)
        {
          fill = FALSE;
          if (fseek (f, 24, SEEK_CUR) < 0)
            {
              psd_set_error (feof (f), errno, error);
              return -1;
            }
        }

      else if (type == PSD_PATH_FILL_INIT)
        {
          if (fread (&init_fill, 2, 1, f) < 1)
            {
              psd_set_error (feof (f), errno, error);
              return -1;
            }
          if (init_fill != 0)
            fill = TRUE;

          if (fseek (f, 22, SEEK_CUR) < 0)
            {
              psd_set_error (feof (f), errno, error);
              return -1;
            }
        }

      else if (type == PSD_PATH_CL_LEN
               || type == PSD_PATH_OP_LEN)
        {
          if (fread (&num_rec, 2, 1, f) < 1)
            {
              psd_set_error (feof (f), errno, error);
              return -1;
            }
          num_rec = GINT16_FROM_BE (num_rec);
          if (num_rec > path_rec)
            {
              psd_set_error (feof (f), errno, error);
              return - 1;
            }
          IFDBG(3) g_debug ("Num path records %d", num_rec);

          if (type == PSD_PATH_CL_LEN)
            closed = TRUE;
          else
            closed = FALSE;
          cntr = 0;
          controlpoints = g_malloc (sizeof (gdouble) * num_rec * 6);
          if (fseek (f, 22, SEEK_CUR) < 0)
            {
              psd_set_error (feof (f), errno, error);
              g_free (controlpoints);
              return -1;
            }

          while (num_rec > 0)
            {
              if (fread (&type, 2, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return -1;
                }
              type = GINT16_FROM_BE (type);
              IFDBG(3) g_debug ("Path record type %d", type);

              if (type == PSD_PATH_CL_LNK
                  || type == PSD_PATH_CL_UNLNK
                  || type == PSD_PATH_OP_LNK
                  || type == PSD_PATH_OP_UNLNK)
                {
                  if (fread (&y[0], 4, 1, f) < 1
                    || fread (&x[0], 4, 1, f) < 1
                    || fread (&y[1], 4, 1, f) < 1
                    || fread (&x[1], 4, 1, f) < 1
                    || fread (&y[2], 4, 1, f) < 1
                    || fread (&x[2], 4, 1, f) < 1)
                    {
                      psd_set_error (feof (f), errno, error);
                      return -1;
                    }
                  for (i = 0; i < 3; ++i)
                    {
                      x[i] = GINT32_FROM_BE (x[i]);
                      controlpoints[cntr] = x[i] / 16777216.0 * image_width;
                      cntr++;
                      y[i] = GINT32_FROM_BE (y[i]);
                      controlpoints[cntr] = y[i] / 16777216.0 * image_height;
                      cntr++;
                    }
                  IFDBG(3) g_debug ("Path points (%d,%d), (%d,%d), (%d,%d)",
                                    x[0], y[0], x[1], y[1], x[2], y[2]);
                }
              else
                {
                  IFDBG(1) g_debug ("Unexpected path type record %d", type);
                  if (fseek (f, 24, SEEK_CUR) < 0)
                    {
                      psd_set_error (feof (f), errno, error);
                      return -1;
                    }
                }
              path_rec--;
              num_rec--;
            }
          /* Add sub-path */
          gimp_vectors_stroke_new_from_points (vector_id,
                                               GIMP_VECTORS_STROKE_TYPE_BEZIER,
                                               cntr, controlpoints, closed);
          g_free (controlpoints);
        }

      else
        {
          if (fseek (f, 24, SEEK_CUR) < 0)
            {
              psd_set_error (feof (f), errno, error);
              return -1;
            }
        }

      path_rec--;
    }

 return 0;
}
