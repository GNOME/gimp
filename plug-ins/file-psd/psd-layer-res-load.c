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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
  PSD_LADJ_VIBRANCE       "vibA"        -       * Adjustment layer - vibrance (PS10) *
  PSD_LADJ_COLOR_LOOKUP   "clrL"        -       * Adjustment layer - color lookup (PS13) *

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
  PSD_LPRP_VERSION        "lyvr"     Loaded     * Layer version (PS7) *

  * Vector mask *
  PSD_LMSK_VMASK          "vmsk"        -       * Vector mask setting (PS6) *

  * Parasites *
  PSD_LPAR_ANNOTATE       "Anno"        -       * Annotation (PS6) *

  * Other *
  PSD_LOTH_PATTERN        "Patt"        -       * Patterns (PS6) *
  PSD_LOTH_GRADIENT       "grdm"        -       * Gradient settings (PS6) *
  PSD_LOTH_SECTION        "lsct"     Loaded     * Section divider setting (PS6) (Layer Groups) *
  PSD_LOTH_SECTION2       "lsdk"     Loaded     * Nested section divider setting (CS5) (Layer Groups) *
  PSD_LOTH_RESTRICT       "brst"        -       * Channel blending restriction setting (PS6) *
  PSD_LOTH_FOREIGN_FX     "ffxi"        -       * Foreign effect ID (PS6) *
  PSD_LOTH_PATT_DATA      "shpa"        -       * Pattern data (PS6) *
  PSD_LOTH_META_DATA      "shmd"        -       * Meta data setting (PS6) *
  PSD_LOTH_LAYER_DATA     "layr"        -       * Layer data (PS6) *
  PSD_LOTH_CONTENT_GEN    "CgEd"        -       * Content generator extra data (PS12) *
  PSD_LOTH_TEXT_ENGINE    "Txt2"        -       * Text engine data (PS10) *
  PSD_LOTH_PATH_NAME      "pths"        -       * Unicode path name (PS13) *
  PSD_LOTH_ANIMATION_FX   "anFX"        -       * Animation effects (PS13) *
  PSD_LOTH_FILTER_MASK    "FMsk"        -       * Filter mask (PS10) *
  PSD_LOTH_VECTOR_STROKE  "vscg"        -       * Vector stroke data (PS13) *
  PSD_LOTH_ALIGN_RENDER   "sn2P"        -       * Aligned rendering flag (?) *
  PSD_LOTH_USER_MASK      "LMsk"        -       * User mask (?) *

  * Effects layer resource IDs *
  PSD_LFX_COMMON          "cmnS"        -       * Effects layer - common state (PS5) *
  PSD_LFX_DROP_SDW        "dsdw"        -       * Effects layer - drop shadow (PS5) *
  PSD_LFX_INNER_SDW       "isdw"        -       * Effects layer - inner shadow (PS5) *
  PSD_LFX_OUTER_GLW       "oglw"        -       * Effects layer - outer glow (PS5) *
  PSD_LFX_INNER_GLW       "iglw"        -       * Effects layer - inner glow (PS5) *
  PSD_LFX_BEVEL           "bevl"        -       * Effects layer - bevel (PS5) *

  * New stuff temporarily until I can get them sorted out *

  * Placed Layer *
 PSD_LPL_PLACE_LAYER      "plLd"        -       * Placed layer (?) *
 PSD_LPL_PLACE_LAYER_NEW  "SoLd"        -       * Placed layer (PS10) *

 * Linked Layer *
 PSD_LLL_LINKED_LAYER     "lnkD"        -       * Linked layer (?) *
 PSD_LLL_LINKED_LAYER_2   "lnk2"        -       * Linked layer 2nd key *
 PSD_LLL_LINKED_LAYER_3   "lnk3"        -       * Linked layer 3rd key *

 * Merged Transparency *
 PSD_LMT_MERGE_TRANS      "Mtrn"        -       * Merged transperency save flag (?) *
 PSD_LMT_MERGE_TRANS_16   "Mt16"        -       * Merged transperency save flag 2 *
 PSD_LMT_MERGE_TRANS_32   "Mt32"        -       * Merged transperency save flag 3 *

 * Filter Effects *
 PSD_LFFX_FILTER_FX       "FXid"        -       * Filter effects (?) *
 PSD_LFFX_FILTER_FX_2     "FEid"        -       * Filter effects 2 *
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

static gint     load_resource_lclr    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       FILE                  *f,
                                       GError               **error);

static gint     load_resource_lsct    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       FILE                  *f,
                                       GError               **error);

static gint     load_resource_lrfx    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       FILE                  *f,
                                       GError               **error);

static gint     load_resource_lyvr    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       FILE                  *f,
                                       GError               **error);

static gint     load_resource_lnsr    (const PSDlayerres     *res_a,
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

      else if (memcmp (res_a->key, PSD_LPRP_COLOR, 4) == 0)
        load_resource_lclr (res_a, lyr_a, f, error);

      else if (memcmp (res_a->key, PSD_LOTH_SECTION, 4) == 0
               || memcmp (res_a->key, PSD_LOTH_SECTION2, 4) == 0) /* bug #789981 */
        load_resource_lsct (res_a, lyr_a, f, error);

      else if (memcmp (res_a->key, PSD_LFX_FX, 4) == 0)
        load_resource_lrfx (res_a, lyr_a, f, error);

      else if (memcmp (res_a->key, PSD_LPRP_VERSION, 4) == 0)
        load_resource_lyvr (res_a, lyr_a, f, error);

      else if (memcmp (res_a->key, PSD_LPRP_SOURCE, 4) == 0)
        load_resource_lnsr (res_a, lyr_a, f, error);

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
  gint32            read_len;
  gint32            write_len;
  guint64           t_xx;
  guint64           t_xy;
  guint64           t_yx;
  guint64           t_yy;
  guint64           t_tx;
  guint64           t_ty;
  gchar            *classID;

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

      lyr_a->text.xx = t_xx >> 11;
      lyr_a->text.xy = t_xy >> 11;
      lyr_a->text.yx = t_yx >> 11;
      lyr_a->text.yy = t_yy >> 11;
      lyr_a->text.tx = t_tx >> 11;
      lyr_a->text.ty = t_ty >> 11;

      IFDBG(2) g_debug ("Version: %d, Text desc. vers.: %d, Desc. vers.: %d",
                        version, text_desc_vers, desc_version);

      IFDBG(2) g_debug ("Transform\n\txx: %f\n\txy: %f\n\tyx: %f"
                        "\n\tyy: %f\n\ttx: %f\n\tty: %f",
                        lyr_a->text.xx, lyr_a->text.xy, lyr_a->text.yx,
                        lyr_a->text.yy, lyr_a->text.tx, lyr_a->text.ty);

      classID = fread_unicode_string (&read_len, &write_len, 4, f, error);
      IFDBG(2) g_debug ("Unicode name: %s", classID);
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
load_resource_lclr (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load layer sheet color code */
  IFDBG(2) g_debug ("Process layer resource block %.4s: Sheet color",
                    res_a->key);

  if (fread (lyr_a->color_tag, 8, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  /* Photoshop only uses color_tag[0] to store a color code */
  lyr_a->color_tag[0] = GUINT16_FROM_BE(lyr_a->color_tag[0]);

  IFDBG(3) g_debug ("Layer sheet color: %i", lyr_a->color_tag[0]);

  return 0;
}

static gint
load_resource_lsct (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    FILE               *f,
                    GError            **error)
{
  /* Load layer group & type information
   * Type 0: not a group
   * Type 1: Open folder
   * Type 2: Closed folder
   * Type 3: End of most recent group */
  guint32           type;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Section divider", res_a->key);
  if (fread (&type, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  type = GUINT32_FROM_BE (type);
  IFDBG(3) g_debug ("Section divider type: %i", type);

  lyr_a->group_type = type;

  if (res_a->data_len >= 12)
    {
      gchar signature[4];
      gchar blend_mode[4];

      if (fread (signature,  4, 1, f) < 1 ||
          fread (blend_mode, 4, 1, f) < 1)
        {
          psd_set_error (feof (f), errno, error);
          return -1;
        }
      if (memcmp (signature, "8BIM", 4) == 0)
        {
          memcpy (lyr_a->blend_mode, blend_mode, 4);
          IFDBG(3) g_debug ("Section divider layer mode sig: %.4s, blend mode: %.4s",
                            signature, blend_mode);
        }
      else
        {
          IFDBG(1) g_debug ("Incorrect layer mode signature %.4s", signature);
        }
    }

  return 0;
}

static gint
load_resource_lrfx (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    FILE               *f,
                    GError            **error)
{
  gint16    version;
  gint16    count;
  gchar     signature[4];
  gchar     effectname[4];
  gint      i;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Layer effects", res_a->key);

  if (fread (&version, 2, 1, f) < 1
      || fread (&count, 2, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  for (i = 0; i < count; i++)
    {
      if (fread (&signature, 4, 1, f) < 1
          || fread(&effectname, 4, 1, f) < 1)
        {
          psd_set_error (feof (f), errno, error);
          return -1;
        }

      if (memcmp (signature, "8BIM", 4) != 0)
        {
          IFDBG(1) g_debug ("Unknown layer resource signature %.4s", signature);
        }
      else
        {
          if (memcmp (effectname, "cmnS", 4) == 0)
            {
              gint32    size;
              gint32    ver;
              gchar     visible;
              gint16    unused;

              if (fread (&size, 4, 1, f) < 1
                  || fread(&ver, 4, 1, f) < 1
                  || fread(&visible, 1, 1, f) < 1
                  || fread(&unused, 2, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return -1;
                }
            }
          else if (memcmp (effectname, "dsdw", 4) == 0
                   || memcmp (effectname, "isdw", 4) == 0)
            {
              gint32    size;
              gint32    ver;
              gint32    blur;
              gint32    intensity;
              gint32    angle;
              gint32    distance;
              gint16    color[5];
              gint32    blendsig;
              gint32    effect;
              gchar     effecton;
              gchar     anglefx;
              gchar     opacity;
              gint16    natcolor[5];

              if (fread (&size, 4, 1, f) < 1
                  || fread(&ver, 4, 1, f) < 1
                  || fread(&blur, 4, 1, f) < 1
                  || fread(&intensity, 4, 1, f) < 1
                  || fread(&angle, 4, 1, f) < 1
                  || fread(&distance, 4, 1, f) < 1
                  || fread(&color[0], 2, 1, f) < 1
                  || fread(&color[1], 2, 1, f) < 1
                  || fread(&color[2], 2, 1, f) < 1
                  || fread(&color[3], 2, 1, f) < 1
                  || fread(&color[4], 2, 1, f) < 1
                  || fread(&blendsig, 4, 1, f) < 1
                  || fread(&effect, 4, 1, f) < 1
                  || fread(&effecton, 1, 1, f) < 1
                  || fread(&anglefx, 1, 1, f) < 1
                  || fread(&opacity, 1, 1, f) < 1
                  || fread(&natcolor[0], 2, 1, f) < 1
                  || fread(&natcolor[1], 2, 1, f) < 1
                  || fread(&natcolor[2], 2, 1, f) < 1
                  || fread(&natcolor[3], 2, 1, f) < 1
                  || fread(&natcolor[4], 2, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return -1;
                }
            }
          else if (memcmp (effectname, "oglw", 4) == 0)
            {
              gint32    size;
              gint32    ver;
              gint32    blur;
              gint32    intensity;
              gint16    color[5];
              gint32    blendsig;
              gint32    effect;
              gchar     effecton;
              gchar     opacity;
              gint16    natcolor[5];

              if (fread (&size, 4, 1, f) < 1
                  || fread(&ver, 4, 1, f) < 1
                  || fread(&blur, 4, 1, f) < 1
                  || fread(&intensity, 4, 1, f) < 1
                  || fread(&color[0], 2, 1, f) < 1
                  || fread(&color[1], 2, 1, f) < 1
                  || fread(&color[2], 2, 1, f) < 1
                  || fread(&color[3], 2, 1, f) < 1
                  || fread(&color[4], 2, 1, f) < 1
                  || fread(&blendsig, 4, 1, f) < 1
                  || fread(&effect, 4, 1, f) < 1
                  || fread(&effecton, 1, 1, f) < 1
                  || fread(&opacity, 1, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return -1;
                }

              if (size == 42)
                {
                  if (fread(&natcolor[0], 2, 1, f) < 1
                      || fread(&natcolor[1], 2, 1, f) < 1
                      || fread(&natcolor[2], 2, 1, f) < 1
                      || fread(&natcolor[3], 2, 1, f) < 1
                      || fread(&natcolor[4], 2, 1, f) < 1)
                    {
                      psd_set_error (feof (f), errno, error);
                      return -1;
                    }
                }
            }
          else if (memcmp (effectname, "iglw", 4) == 0)
            {
              gint32    size;
              gint32    ver;
              gint32    blur;
              gint32    intensity;
              gint32    angle;
              gint32    distance;
              gint16    color[5];
              gint32    blendsig;
              gint32    effect;
              gchar     effecton;
              gchar     anglefx;
              gchar     opacity;
              gchar     invert;
              gint16    natcolor[5];

              if (fread (&size, 4, 1, f) < 1
                  || fread(&ver, 4, 1, f) < 1
                  || fread(&blur, 4, 1, f) < 1
                  || fread(&intensity, 4, 1, f) < 1
                  || fread(&angle, 4, 1, f) < 1
                  || fread(&distance, 4, 1, f) < 1
                  || fread(&color[0], 2, 1, f) < 1
                  || fread(&color[1], 2, 1, f) < 1
                  || fread(&color[2], 2, 1, f) < 1
                  || fread(&color[3], 2, 1, f) < 1
                  || fread(&color[4], 2, 1, f) < 1
                  || fread(&blendsig, 4, 1, f) < 1
                  || fread(&effect, 4, 1, f) < 1
                  || fread(&effecton, 1, 1, f) < 1
                  || fread(&anglefx, 1, 1, f) < 1
                  || fread(&opacity, 1, 1, f) < 1
                  || fread(&natcolor[0], 2, 1, f) < 1
                  || fread(&natcolor[1], 2, 1, f) < 1
                  || fread(&natcolor[2], 2, 1, f) < 1
                  || fread(&natcolor[3], 2, 1, f) < 1
                  || fread(&natcolor[4], 2, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return -1;
                }

              if (size == 43)
                {
                  if (fread (&invert, 1, 1, f) < 1
                      || fread(&natcolor[0], 2, 1, f) < 1
                      || fread(&natcolor[0], 2, 1, f) < 1
                      || fread(&natcolor[1], 2, 1, f) < 1
                      || fread(&natcolor[2], 2, 1, f) < 1
                      || fread(&natcolor[3], 2, 1, f) < 1
                      || fread(&natcolor[4], 2, 1, f) < 1)
                    {
                      psd_set_error (feof (f), errno, error);
                      return -1;
                    }
                }
            }
          else if (memcmp (effectname, "bevl", 4) == 0)
            {
              gint32    size;
              gint32    ver;
              gint32    angle;
              gint32    strength;
              gint32    blur;
              gint32    highlightsig;
              gint32    highlighteffect;
              gint32    shadowsig;
              gint32    shadoweffect;
              gint16    highlightcolor[5];
              gint16    shadowcolor[5];
              gchar     style;
              gchar     highlightopacity;
              gchar     shadowopacity;
              gchar     enabled;
              gchar     global;
              gchar     direction;
              gint16    highlightnatcolor[5];
              gint16    shadownatcolor[5];

              if (fread (&size, 4, 1, f) < 1
                  || fread(&ver, 4, 1, f) < 1
                  || fread(&angle, 4, 1, f) < 1
                  || fread(&strength, 4, 1, f) < 1
                  || fread(&blur, 4, 1, f) < 1
                  || fread(&highlightsig, 4, 1, f) < 1
                  || fread(&highlighteffect, 4, 1, f) < 1
                  || fread(&shadowsig, 4, 1, f) < 1
                  || fread(&highlightcolor[0], 2, 1, f) < 1
                  || fread(&shadoweffect, 4, 1, f) < 1
                  || fread(&highlightcolor[1], 2, 1, f) < 1
                  || fread(&highlightcolor[2], 2, 1, f) < 1
                  || fread(&highlightcolor[3], 2, 1, f) < 1
                  || fread(&highlightcolor[4], 2, 1, f) < 1
                  || fread(&shadowcolor[0], 2, 1, f) < 1
                  || fread(&shadowcolor[1], 2, 1, f) < 1
                  || fread(&shadowcolor[2], 2, 1, f) < 1
                  || fread(&shadowcolor[3], 2, 1, f) < 1
                  || fread(&shadowcolor[4], 2, 1, f) < 1
                  || fread(&style, 1, 1, f) < 1
                  || fread(&highlightopacity, 1, 1, f) < 1
                  || fread(&shadowopacity, 1, 1, f) < 1
                  || fread(&enabled, 1, 1, f) < 1
                  || fread(&global, 1, 1, f) < 1
                  || fread(&direction, 1, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return -1;
                }

              if (size == 78)
                {
                  if (fread(&highlightnatcolor[0], 2, 1, f) < 1
                      || fread(&highlightnatcolor[0], 2, 1, f) < 1
                      || fread(&highlightnatcolor[1], 2, 1, f) < 1
                      || fread(&highlightnatcolor[2], 2, 1, f) < 1
                      || fread(&highlightnatcolor[3], 2, 1, f) < 1
                      || fread(&highlightnatcolor[4], 2, 1, f) < 1
                      || fread(&shadownatcolor[0], 2, 1, f) < 1
                      || fread(&shadownatcolor[0], 2, 1, f) < 1
                      || fread(&shadownatcolor[1], 2, 1, f) < 1
                      || fread(&shadownatcolor[2], 2, 1, f) < 1
                      || fread(&shadownatcolor[3], 2, 1, f) < 1
                      || fread(&shadownatcolor[4], 2, 1, f) < 1)
                    {
                      psd_set_error (feof (f), errno, error);
                      return -1;
                    }
                }
            }
          else if (memcmp (effectname, "sofi", 4) == 0)
            {
              gint32    size;
              gint32    ver;
              gint32    key;
              gint16    color[5];
              gchar     opacity;
              gchar     enabled;
              gint16    natcolor[5];

              if (fread (&size, 4, 1, f) < 1
                  || fread(&ver, 4, 1, f) < 1
                  || fread(&key, 4, 1, f) < 1
                  || fread(&color[0], 2, 1, f) < 1
                  || fread(&color[1], 2, 1, f) < 1
                  || fread(&color[2], 2, 1, f) < 1
                  || fread(&color[3], 2, 1, f) < 1
                  || fread(&color[4], 2, 1, f) < 1
                  || fread(&opacity, 1, 1, f) < 1
                  || fread(&enabled, 1, 1, f) < 1
                  || fread(&natcolor[0], 2, 1, f) < 1
                  || fread(&natcolor[1], 2, 1, f) < 1
                  || fread(&natcolor[2], 2, 1, f) < 1
                  || fread(&natcolor[3], 2, 1, f) < 1
                  || fread(&natcolor[4], 2, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return -1;
                }
            }
          else
            {
              IFDBG(1) g_debug ("Unknown layer effect signature %.4s", effectname);
            }
        }
    }

  return 0;
}

static gint
load_resource_lyvr (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    FILE               *f,
                    GError            **error)
{
  gint32 version;

  IFDBG(2) g_debug ("Process layer resource block %.4s: layer version",
                    res_a->key);

  if (fread (&version, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  version = GINT32_FROM_BE(version);

  /* minimum value is 70 according to specs but there's no reason to
   * stop the loading
   */
  if (version < 70)
    {
      g_message ("Invalid version layer");
    }

  return 0;
}

static gint
load_resource_lnsr (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    FILE               *f,
                    GError            **error)
{
  gchar layername[4];

  IFDBG(2) g_debug ("Process layer resource block %.4s: layer source name",
                    res_a->key);

  if (fread (&layername, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  /* nowadays psd files, layer name are encoded in unicode, cf "luni"
   * moreover lnsr info is encoded in MacRoman, see
   * https://bugzilla.gnome.org/show_bug.cgi?id=753986#c4
   */

  return 0;
}
