/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP PSD Plug-in
 * Copyright 2007 by John Marshall
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

/* ----- Known Layer Resource Block Types -----
  All layer resources not otherwise handled, including unknown types
  are dropped with a warning.

 * Adjustment layer IDs *
  PSD_LADJ_LEVEL          "levl"    Drop Layer  * Adjustment layer - levels (PS4) *
  PSD_LADJ_CURVE          "curv"    Drop Layer  * Adjustment layer - curves (PS4) *
  PSD_LADJ_BRIGHTNESS     "brit"    Drop Layer  * Adjustment layer - brightness contrast (PS4) *
  PSD_LADJ_BALANCE        "blnc"    Drop Layer  * Adjustment layer - color balance (PS4) *
  PSD_LADJ_BLACK_WHITE    "blwh"    Drop Layer  * Adjustment layer - black & white (PS10) *
  PSD_LADJ_HUE            "hue "    Drop Layer  * Adjustment layer - old hue saturation (PS4) *
  PSD_LADJ_HUE2           "hue2"    Drop Layer  * Adjustment layer - hue saturation (PS5) *
  PSD_LADJ_SELECTIVE      "selc"    Drop Layer  * Adjustment layer - selective color (PS4) *
  PSD_LADJ_MIXER          "mixr"    Drop Layer  * Adjustment layer - channel mixer (PS9) *
  PSD_LADJ_GRAD_MAP       "grdm"    Drop Layer  * Adjustment layer - gradient map (PS9) *
  PSD_LADJ_PHOTO_FILT     "phfl"    Drop Layer  * Adjustment layer - photo filter (PS9) *
  PSD_LADJ_EXPOSURE       "expA"    Drop Layer  * Adjustment layer - exposure (PS10) *
  PSD_LADJ_INVERT         "nvrt"    Drop Layer  * Adjustment layer - invert (PS4) *
  PSD_LADJ_THRESHOLD      "thrs"    Drop Layer  * Adjustment layer - threshold (PS4) *
  PSD_LADJ_POSTERIZE      "post"    Drop Layer  * Adjustment layer - posterize (PS4) *

 * Fill Layer IDs *
  PSD_LFIL_SOLID          "SoCo"        -       * Solid color sheet setting (PS6) *
  PSD_LFIL_PATTERN        "PtFl"        -       * Pattern fill setting (PS6) *
  PSD_LFIL_GRADIENT       "GdFl"        -       * Gradient fill setting (PS6) *

 * Effects Layer IDs *
  PSD_LFX_FX              "lrFX"        -       * Effects layer info (PS5) *
  PSD_LFX_FX2             "lfx2"        -       * Object based effects layer info (PS6) *

 * Type Tool Layers *
  PSD_LTYP_TYPE           "tySh"        -       * Type tool layer (PS5) *
  PSD_LTYP_TYPE2          "TySh"        -       * Type tool object setting (PS6) *

 * Layer Properties *
  PSD_LPRP_UNICODE        "luni"     Loaded     * Unicode layer name (PS5) *
  PSD_LPRP_SOURCE         "lnsr"     Loaded     * Layer name source setting (PS6) *
  PSD_LPRP_ID             "lyid"     Loaded     * Layer ID (PS5) *
  PSD_LPRP_BLEND_CLIP     "clbl"        -       * Blend clipping elements (PS6) *
  PSD_LPRP_BLEND_INT      "infx"        -       * Blend interior elements (PS6) *
  PSD_LPRP_KNOCKOUT       "knko"        -       * Knockout setting (PS6) *
  PSD_LPRP_PROTECT        "lspf"        -       * Protected setting (PS6) *
  PSD_LPRP_COLOR          "lclr"        -       * Sheet color setting (PS6) *
  PSD_LPRP_REF_POINT      "fxrp"        -       * Reference point (PS6) *

 * Vector mask *
  PSD_LMSK_VMASK          "vmsk"        -       * Vector mask setting (PS6) *

 * Parasites *
  PSD_LPAR_ANNOTATE       "Anno"        -       * Annotation (PS6) *

 * Other *
  PSD_LOTH_PATTERN        "Patt"        -       * Patterns (PS6) *
  PSD_LOTH_GRADIENT       "grdm"        -       * Gradient settings (PS6) *
  PSD_LOTH_SECTION        "lsct"    Drop Layer  * Section divider setting (PS6) (Layer Groups) *
  PSD_LOTH_RESTRICT       "brst"        -       * Channel blending restirction setting (PS6) *
  PSD_LOTH_FOREIGN_FX     "ffxi"        -       * Foreign effect ID (PS6) *
  PSD_LOTH_PATT_DATA      "shpa"        -       * Pattern data (PS6) *
  PSD_LOTH_META_DATA      "shmd"        -       * Meta data setting (PS6) *
  PSD_LOTH_LAYER_DATA     "layr"        -       * Layer data (PS6) *

 * Effects layer resource IDs *
  PSD_LFX_COMMON          "cmnS"        -       * Effects layer - common state (PS5) *
  PSD_LFX_DROP_SDW        "dsdw"        -       * Effects layer - drop shadow (PS5) *
  PSD_LFX_INNER_SDW       "isdw"        -       * Effects layer - inner shadow (PS5) *
  PSD_LFX_OUTER_GLW       "oglw"        -       * Effects layer - outer glow (PS5) *
  PSD_LFX_INNER_GLW       "iglw"        -       * Effects layer - inner glow (PS5) *
  PSD_LFX_BEVEL           "bevl"        -       * Effects layer - bevel (PS5) *
*/

#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>
#include <libgimp/gimp.h>

#include "psd.h"
#include "psd-util.h"
#include "psd-layer-res-load.h"

#include "libgimp/stdplugins-intl.h"

/*  Local function prototypes  */
static gint     load_resource_unknown (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       FILE                  *f,
                                       GError               **error);

static gint     load_resource_ladj    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       FILE                  *f,
                                       GError               **error);

static gint     load_resource_lfil    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       FILE                  *f,
                                       GError               **error);

static gint     load_resource_lfx     (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       FILE                  *f,
                                       GError               **error);

static gint     load_resource_ltyp    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       FILE                  *f,
                                       GError               **error);

static gint     load_resource_luni    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       FILE                  *f,
                                       GError               **error);

static gint     load_resource_lyid    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       FILE                  *f,
                                       GError               **error);

static gint     load_resource_lsct    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       FILE                  *f,
                                       GError               **error);


/* Public Functions */
gint
get_layer_resource_header (PSDlayerres  *res_a,
                           FILE         *f,
                           GError      **error)
{
  if (fread (res_a->sig, 4, 1, f) < 1
      || fread (res_a->key, 4, 1, f) < 1
      || fread (&res_a->data_len, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  res_a->data_len = GUINT32_FROM_BE (res_a->data_len);
  res_a->data_start = ftell (f);

  IFDBG(2) g_debug ("Sig: %.4s, key: %.4s, start: %d, len: %d",
                     res_a->sig, res_a->key, res_a->data_start, res_a->data_len);

  return 0;
}

gint
load_layer_resource (PSDlayerres  *res_a,
                     PSDlayer     *lyr_a,
                     FILE         *f,
                     GError      **error)
{
  /* Set file position to start of layer resource data block */
  if (fseek (f, res_a->data_start, SEEK_SET) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

   /* Process layer resource blocks */
  if (memcmp (res_a->sig, "8BIM", 4) != 0)
    {
      IFDBG(1) g_debug ("Unknown layer resource signature %.4s", res_a->sig);
    }
  else
    {
      if (memcmp (res_a->key, PSD_LADJ_LEVEL, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_CURVE, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_BRIGHTNESS, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_BALANCE, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_BLACK_WHITE, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_HUE, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_HUE2, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_SELECTIVE, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_MIXER, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_GRAD_MAP, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_PHOTO_FILT, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_EXPOSURE, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_THRESHOLD, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_INVERT, 4) == 0
          || memcmp (res_a->key, PSD_LADJ_POSTERIZE, 4) == 0)
            load_resource_ladj (res_a, lyr_a, f, error);

      else if (memcmp (res_a->key, PSD_LFIL_SOLID, 4) == 0
          || memcmp (res_a->key, PSD_LFIL_PATTERN, 4) == 0
          || memcmp (res_a->key, PSD_LFIL_GRADIENT, 4) == 0)
            load_resource_lfil (res_a, lyr_a, f, error);

      else if (memcmp (res_a->key, PSD_LFX_FX, 4) == 0
          || memcmp (res_a->key, PSD_LFX_FX2, 4) == 0)
            load_resource_lfx (res_a, lyr_a, f, error);

      else if (memcmp (res_a->key, PSD_LTYP_TYPE, 4) == 0
          || memcmp (res_a->key, PSD_LTYP_TYPE2, 4) == 0)
            load_resource_ltyp (res_a, lyr_a, f, error);

      else if (memcmp (res_a->key, PSD_LPRP_UNICODE, 4) == 0)
            load_resource_luni (res_a, lyr_a, f, error);

      else if (memcmp (res_a->key, PSD_LPRP_ID, 4) == 0)
            load_resource_lyid (res_a, lyr_a, f, error);

      else if (memcmp (res_a->key, PSD_LOTH_SECTION, 4) == 0)
            load_resource_lsct (res_a, lyr_a, f, error);

      else
        load_resource_unknown (res_a, lyr_a, f, error);
    }

  /* Set file position to end of layer resource block */
  if (fseek (f, res_a->data_start + res_a->data_len, SEEK_SET) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  return 0;
}

/* Private Functions */

static gint
load_resource_unknown (const PSDlayerres  *res_a,
                       PSDlayer           *lyr_a,
                       FILE               *f,
                       GError            **error)
{
  IFDBG(2) g_debug ("Process unknown layer resource block: %.4s", res_a->key);

  return 0;
}

static gint
load_resource_ladj (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load adjustment layer */
  static gboolean   msg_flag = FALSE;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Adjustment layer", res_a->key);
  lyr_a->drop = TRUE;
  if (! msg_flag && CONVERSION_WARNINGS)
    {
      g_message ("Warning:\n"
                 "The image file contains adjustment layers. "
                 "These are not supported by the GIMP and will "
                 "be dropped.");
      msg_flag = TRUE;
    }

  return 0;
}

static gint
load_resource_lfil (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load fill layer */
  static gboolean   msg_flag = FALSE;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Fill layer", res_a->key);
  if (! msg_flag && CONVERSION_WARNINGS)
    {
      g_message ("Warning:\n"
                 "The image file contains fill layers. "
                 "These are not supported by the GIMP and will "
                 "be rasterized.");
      msg_flag = TRUE;
    }

  return 0;
}

static gint
load_resource_lfx (const PSDlayerres  *res_a,
                   PSDlayer           *lyr_a,
                   FILE               *f,
                   GError            **error)
{
  /* Load layer effects */
  static gboolean   msg_flag = FALSE;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Layer effects", res_a->key);
  if (! msg_flag && CONVERSION_WARNINGS)
    {
      g_message ("Warning:\n"
                 "The image file contains layer effects. "
                 "These are not supported by the GIMP and will "
                 "be dropped.");
      msg_flag = TRUE;
    }

  return 0;
}

static gint
load_resource_ltyp (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load type tool layer */
  gint16            version;
  gint16            text_desc_vers;
  gint32            desc_version;
  guint64           t_xx;
  guint64           t_xy;
  guint64           t_yx;
  guint64           t_yy;
  guint64           t_tx;
  guint64           t_ty;
  gdouble           transform_xx;
  gdouble           transform_xy;
  gdouble           transform_yx;
  gdouble           transform_yy;
  gdouble           transform_tx;
  gdouble           transform_ty;

  static gboolean   msg_flag = FALSE;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Type tool layer", res_a->key);
  if (! msg_flag && CONVERSION_WARNINGS)
    {
      g_message ("Warning:\n"
                 "The image file contains type tool layers. "
                 "These are not supported by the GIMP and will "
                 "be dropped.");
      msg_flag = TRUE;
    }

  /* New style type tool layers (ps6) */
  if (memcmp (res_a->key, PSD_LTYP_TYPE2, 4) == 0)
    {
      if (fread (&version, 2, 1, f) < 1
          || fread (&t_xx, 8, 1, f) < 1
          || fread (&t_xy, 8, 1, f) < 1
          || fread (&t_yx, 8, 1, f) < 1
          || fread (&t_yy, 8, 1, f) < 1
          || fread (&t_tx, 8, 1, f) < 1
          || fread (&t_ty, 8, 1, f) < 1
          || fread (&text_desc_vers, 2, 1, f) < 1
          || fread (&desc_version, 4, 1, f) < 1)
        {
          psd_set_error (feof (f), errno, error);
          return -1;
        }

      version = GINT16_FROM_BE (version);
      text_desc_vers = GINT16_FROM_BE (text_desc_vers);
      desc_version = GINT32_FROM_BE (desc_version);
//      t_xx = GUINT64_FROM_BE (t_xx);
//      t_xy = GUINT64_FROM_BE (t_xy);
//      t_yx = GUINT64_FROM_BE (t_yx);
//      t_yy = GUINT64_FROM_BE (t_yy);
//      t_tx = GUINT64_FROM_BE (t_tx);
//      t_ty = GUINT64_FROM_BE (t_ty);

      transform_xx = t_xx >> 11;
      transform_xy = t_xy >> 11;
      transform_yx = t_yx >> 11;
      transform_yy = t_yy >> 11;
      transform_tx = t_tx >> 11;
      transform_ty = t_ty >> 11;

      IFDBG(2) g_debug ("Version: %d, Text desc. vers.: %d, Desc. vers.: %d",
                        version, text_desc_vers, desc_version);

      IFDBG(2) g_debug ("Transform\n\txx: %f\n\txy: %f\n\tyx: %f"
                        "\n\tyy: %f\n\ttx: %f\n\tty: %f",
                        transform_xx, transform_xy, transform_yx,
                        transform_yy, transform_tx, transform_ty);

//      classID = fread_unicode_string (&read_len, &write_len, 4, f);
//      IFDBG(2) g_debug ("Unicode name: %s", classID);

    }

  return 0;
}

static gint
load_resource_luni (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load layer name in unicode (length padded to multiple of 4 bytes) */
  gint32        read_len;
  gint32        write_len;

  IFDBG(2) g_debug ("Process layer resource block luni: Unicode Name");
  if (lyr_a->name)
    g_free (lyr_a->name);

  lyr_a->name = fread_unicode_string (&read_len, &write_len, 4, f, error);
  if (*error)
    return -1;
  IFDBG(3) g_debug ("Unicode name: %s", lyr_a->name);

  return 0;
}

static gint
load_resource_lyid (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load layer id (tattoo) */

  IFDBG(2) g_debug ("Process layer resource block lyid: Layer ID");
  if (fread (&lyr_a->id, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  lyr_a->id = GUINT32_FROM_BE (lyr_a->id);
  IFDBG(3) g_debug ("Layer id: %i", lyr_a->id);

  return 0;
}

static gint
load_resource_lsct (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load adjustment layer */
  static gboolean   msg_flag = FALSE;
  guint32           type;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Section divider", res_a->key);
  if (fread (&type, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  type = GUINT32_FROM_BE (type);
  IFDBG(3) g_debug ("Section divider type: %i", type);

  if (type == 1 ||      /* Layer group start - open folder */
      type == 2)        /* Layer group start - closed folder */
    {
      lyr_a->drop = TRUE;
      if (! msg_flag && CONVERSION_WARNINGS)
        {
          g_message ("Warning:\n"
                     "The image file contains layer groups. "
                     "These are not supported by the GIMP and will "
                     "be dropped.");
          msg_flag = TRUE;
        }
    }

  if (type == 3)        /* End of layer group - hidden in UI */
      lyr_a->drop = TRUE;


  return 0;
}

