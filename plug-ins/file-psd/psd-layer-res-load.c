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
  will show up empty with a warning.

  * Adjustment layer IDs *
  PSD_LADJ_LEVEL          "levl"    Empty Layer  * Adjustment layer - levels (PS4) *
  PSD_LADJ_CURVE          "curv"    Empty Layer  * Adjustment layer - curves (PS4) *
  PSD_LADJ_BRIGHTNESS     "brit"    Empty Layer  * Adjustment layer - brightness contrast (PS4) *
  PSD_LADJ_BALANCE        "blnc"    Empty Layer  * Adjustment layer - color balance (PS4) *
  PSD_LADJ_BLACK_WHITE    "blwh"    Empty Layer  * Adjustment layer - black & white (PS10) *
  PSD_LADJ_HUE            "hue "    Empty Layer  * Adjustment layer - old hue saturation (PS4) *
  PSD_LADJ_HUE2           "hue2"    Empty Layer  * Adjustment layer - hue saturation (PS5) *
  PSD_LADJ_SELECTIVE      "selc"    Empty Layer  * Adjustment layer - selective color (PS4) *
  PSD_LADJ_MIXER          "mixr"    Empty Layer  * Adjustment layer - channel mixer (PS9) *
  PSD_LADJ_GRAD_MAP       "grdm"    Empty Layer  * Adjustment layer - gradient map (PS9) *
  PSD_LADJ_PHOTO_FILT     "phfl"    Empty Layer  * Adjustment layer - photo filter (PS9) *
  PSD_LADJ_EXPOSURE       "expA"    Empty Layer  * Adjustment layer - exposure (PS10) *
  PSD_LADJ_INVERT         "nvrt"    Empty Layer  * Adjustment layer - invert (PS4) *
  PSD_LADJ_THRESHOLD      "thrs"    Empty Layer  * Adjustment layer - threshold (PS4) *
  PSD_LADJ_POSTERIZE      "post"    Empty Layer  * Adjustment layer - posterize (PS4) *
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
 PSD_LPL_PLACE_LAYER      "PlLd"        -       * Placed layer (?) (based on PSD files, not specification) *
 PSD_LPL_PLACE_LAYER_NEW  "SoLd"        -       * Placed layer (PS10) *
 PSD_SMART_OBJECT_LAYER   "SoLE"        -       * Smart Object Layer (CC2015) *

 * Linked Layer *
 PSD_LLL_LINKED_LAYER     "lnkD"        -       * Linked layer (?) *
 PSD_LLL_LINKED_LAYER_2   "lnk2"        -       * Linked layer 2nd key *
 PSD_LLL_LINKED_LAYER_3   "lnk3"        -       * Linked layer 3rd key *
 PSD_LLL_LINKED_LAYER_EXT "lnkE"        -       * Linked layer external *

 * Merged Transparency *
 PSD_LMT_MERGE_TRANS      "Mtrn"        -       * Merged transparency save flag (?) *
 PSD_LMT_MERGE_TRANS_16   "Mt16"        -       * Merged transparency save flag 2 *
 PSD_LMT_MERGE_TRANS_32   "Mt32"        -       * Merged transparency save flag 3 *

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
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_ladj    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_lfil    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_lfx     (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_ltyp    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_luni    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_lyid    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_lclr    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_lsct    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_lrfx    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_lyvr    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_lnsr    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

/* Public Functions */

/* Returns < 0 for errors, else returns the size of the resource header
 * which should be either 4 or 8. */
gint
get_layer_resource_header (PSDlayerres   *res_a,
                           guint16        psd_version,
                           GInputStream  *input,
                           GError       **error)
{
  gint block_len_size = 4;

  res_a->ibm_pc_format = FALSE;

  if (psd_read (input, res_a->sig, 4, error) < 4 ||
      psd_read (input, res_a->key, 4, error) < 4)
    {
      psd_set_error (error);
      return -1;
    }
  else if (memcmp (res_a->sig, "8BIM", 4) != 0 &&
           memcmp (res_a->sig, "MIB8", 4) != 0 &&
           memcmp (res_a->sig, "8B64", 4) != 0)
    {
      IFDBG(1) g_debug ("Unknown layer resource signature %.4s", res_a->sig);
    }

  if (memcmp (res_a->sig, "MIB8", 4) == 0)
    {
      gchar ibm_sig[4];
      gchar ibm_key[4];

      res_a->ibm_pc_format = TRUE;

      for (gint i = 0; i < 4; i++)
        {
          ibm_sig[i] = res_a->sig[3 - i];
          ibm_key[i] = res_a->key[3 - i];
        }
      for (gint i = 0; i < 4; i++)
        {
          res_a->sig[i] = ibm_sig[i];
          res_a->key[i] = ibm_key[i];
        }
    }

  if (psd_version == 1)
    block_len_size = 4;
  else
    {
      /* For PSB only certain block resources have a double sized length
        * so we need to check which resource it is first before we can
        * read the block length.
        * According to the docs: LMsk, Lr16, Lr32, Layr, Mt16, Mt32, Mtrn,
        * Alph, FMsk, lnk2, FEid, FXid, PxSD have an 8 byte length. */
      if (memcmp (res_a->key, "LMsk", 4) == 0 ||
          memcmp (res_a->key, "Lr16", 4) == 0 ||
          memcmp (res_a->key, "Lr32", 4) == 0 ||
          memcmp (res_a->key, "Layr", 4) == 0 ||
          memcmp (res_a->key, "Mt16", 4) == 0 ||
          memcmp (res_a->key, "Mt32", 4) == 0 ||
          memcmp (res_a->key, "Mtrn", 4) == 0 ||
          memcmp (res_a->key, "Alph", 4) == 0 ||
          memcmp (res_a->key, "FMsk", 4) == 0 ||
          memcmp (res_a->key, "lnk2", 4) == 0 ||
          memcmp (res_a->key, "FEid", 4) == 0 ||
          memcmp (res_a->key, "FXid", 4) == 0 ||
          memcmp (res_a->key, "PxSD", 4) == 0 ||
          /* Apparently also using 8 bytes in size but not mentioned in specs: */
          memcmp (res_a->key, "lnkE", 4) == 0 ||
          memcmp (res_a->key, "pths", 4) == 0
          )
        block_len_size = 8;
      else
        block_len_size = 4;
      IFDBG(3) g_debug ("PSB: Using block_len_size %d", block_len_size);
    }

  if (psd_read (input, &res_a->data_len, block_len_size, error) < block_len_size)
    {
      psd_set_error (error);
      return -1;
    }

  if (! res_a->ibm_pc_format)
    {
      if (block_len_size == 4)
        res_a->data_len = GUINT32_FROM_BE (res_a->data_len);
      else
        res_a->data_len = GUINT64_FROM_BE (res_a->data_len);
    }
  else
    {
      if (block_len_size == 4)
        res_a->data_len = GUINT32_FROM_LE (res_a->data_len);
      else
        res_a->data_len = GUINT64_FROM_LE (res_a->data_len);
    }

  res_a->data_start = PSD_TELL (input);

  IFDBG(2) g_debug ("Sig: %.4s, key: %.4s, start: %" G_GOFFSET_FORMAT ", len: %" G_GOFFSET_FORMAT,
                     res_a->sig, res_a->key, res_a->data_start, res_a->data_len);

  return block_len_size + 8;
}

gint
load_layer_resource (PSDlayerres   *res_a,
                     PSDlayer      *lyr_a,
                     GInputStream  *input,
                     GError       **error)
{
  /* Set file position to start of layer resource data block */
  if (! psd_seek (input, res_a->data_start, G_SEEK_SET, error))
    {
      psd_set_error (error);
      return -1;
    }

  /* Process layer resource blocks */
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
    {
      if (lyr_a)
        {
          lyr_a->unsupported_features->adjustment_layer = TRUE;
          lyr_a->unsupported_features->show_gui         = TRUE;
        }

      load_resource_ladj (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LFIL_SOLID, 4) == 0
           || memcmp (res_a->key, PSD_LFIL_PATTERN, 4) == 0
           || memcmp (res_a->key, PSD_LFIL_GRADIENT, 4) == 0)
    {
      if (lyr_a)
        {
          lyr_a->unsupported_features->fill_layer = TRUE;
          lyr_a->unsupported_features->show_gui   = TRUE;
        }

      load_resource_lfil (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LFX_FX, 4) == 0
           || memcmp (res_a->key, PSD_LFX_FX2, 4) == 0)
    {
      if (lyr_a)
        {
          lyr_a->unsupported_features->layer_effect = TRUE;
          lyr_a->unsupported_features->show_gui     = TRUE;
        }

      load_resource_lfx (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LTYP_TYPE, 4) == 0
           || memcmp (res_a->key, PSD_LTYP_TYPE2, 4) == 0)
    {
      if (lyr_a)
        {
          lyr_a->unsupported_features->text_layer = TRUE;
          lyr_a->unsupported_features->show_gui   = TRUE;
        }

      load_resource_ltyp (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LPRP_UNICODE, 4) == 0)
    {
      load_resource_luni (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LPRP_ID, 4) == 0)
    {
      load_resource_lyid (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LPRP_COLOR, 4) == 0)
    {
      load_resource_lclr (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LOTH_SECTION, 4) == 0
           || memcmp (res_a->key, PSD_LOTH_SECTION2, 4) == 0) /* bug #789981 */
    {
      load_resource_lsct (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LFX_FX, 4) == 0)
    {
      load_resource_lrfx (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LPRP_VERSION, 4) == 0)
    {
      load_resource_lyvr (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LPRP_SOURCE, 4) == 0)
    {
      load_resource_lnsr (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LOTH_VECTOR_STROKE, 4) == 0)
    {
      if (lyr_a)
        {
          lyr_a->unsupported_features->stroke   = TRUE;
          lyr_a->unsupported_features->show_gui = TRUE;
        }

      load_resource_unknown (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LMSK_VMASK, 4) == 0)
    {
      if (lyr_a)
        {
          lyr_a->unsupported_features->vector_mask = TRUE;
          lyr_a->unsupported_features->show_gui    = TRUE;
        }

      load_resource_unknown (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_SMART_OBJECT_LAYER, 4) == 0
           || memcmp (res_a->key, PSD_LPL_PLACE_LAYER, 4) == 0
           || memcmp (res_a->key, PSD_LPL_PLACE_LAYER_NEW, 4) == 0)
    {
      if (lyr_a)
        {
          lyr_a->unsupported_features->smart_object = TRUE;
          lyr_a->unsupported_features->show_gui     = TRUE;
        }

      load_resource_unknown (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LLL_LINKED_LAYER, 4) == 0
           || memcmp (res_a->key, PSD_LLL_LINKED_LAYER_2, 4) == 0
           || memcmp (res_a->key, PSD_LLL_LINKED_LAYER_3, 4) == 0
           || memcmp (res_a->key, PSD_LLL_LINKED_LAYER_EXT, 4) == 0)
    {
      if (lyr_a)
        {
          lyr_a->unsupported_features->linked_layer = TRUE;
          lyr_a->unsupported_features->show_gui     = TRUE;
        }

      load_resource_unknown (res_a, lyr_a, input, error);
    }

  else
    {
      load_resource_unknown (res_a, lyr_a, input, error);
    }

  if (error && *error)
    return -1;

  /* Set file position to end of layer resource block */
  if (! psd_seek (input, res_a->data_start + res_a->data_len, G_SEEK_SET, error))
    {
      psd_set_error (error);
      return -1;
    }

  return 0;
}

/* Private Functions */

static gint
load_resource_unknown (const PSDlayerres  *res_a,
                       PSDlayer           *lyr_a,
                       GInputStream       *input,
                       GError            **error)
{
  IFDBG(2) g_debug ("Process unknown layer resource block: %.4s", res_a->key);

  return 0;
}

static gint
load_resource_ladj (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    GInputStream       *input,
                    GError            **error)
{
  /* Load adjustment layer */

  IFDBG(2) g_debug ("Process layer resource block %.4s: Adjustment layer", res_a->key);

  return 0;
}

static gint
load_resource_lfil (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    GInputStream       *input,
                    GError            **error)
{
  /* Load fill layer */

  IFDBG(2) g_debug ("Process layer resource block %.4s: Fill layer", res_a->key);

  return 0;
}

static gint
load_resource_lfx (const PSDlayerres  *res_a,
                   PSDlayer           *lyr_a,
                   GInputStream       *input,
                   GError            **error)
{
  /* Load layer effects */

  IFDBG(2) g_debug ("Process layer resource block %.4s: Layer effects", res_a->key);

  return 0;
}

static gint
load_resource_ltyp (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    GInputStream       *input,
                    GError            **error)
{
  /* Load type tool layer */
  gint16            version        = 0;
  gint16            text_desc_vers = 0;
  gint32            desc_version   = 0;
  gint32            read_len;
  gint32            write_len;
  guint64           t_xx = 0;
  guint64           t_xy = 0;
  guint64           t_yx = 0;
  guint64           t_yy = 0;
  guint64           t_tx = 0;
  guint64           t_ty = 0;
  gchar            *classID;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Type tool layer", res_a->key);

  /* New style type tool layers (ps6) */
  if (memcmp (res_a->key, PSD_LTYP_TYPE2, 4) == 0)
    {
      if (psd_read (input, &version, 2, error) < 2 ||
          psd_read (input, &t_xx,    8, error) < 8 ||
          psd_read (input, &t_xy,    8, error) < 8 ||
          psd_read (input, &t_yx,    8, error) < 8 ||
          psd_read (input, &t_yy,    8, error) < 8 ||
          psd_read (input, &t_tx,    8, error) < 8 ||
          psd_read (input, &t_ty,    8, error) < 8 ||
          psd_read (input, &text_desc_vers, 2, error) < 2 ||
          psd_read (input, &desc_version,   4, error) < 4)
        {
          psd_set_error (error);
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

      classID = fread_unicode_string (&read_len, &write_len, 4,
                                      res_a->ibm_pc_format, input, error);
      IFDBG(2) g_debug ("Unicode name: %s", classID);
    }

  return 0;
}

static gint
load_resource_luni (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    GInputStream       *input,
                    GError            **error)
{
  /* Load layer name in unicode (length padded to multiple of 4 bytes) */
  gint32        read_len;
  gint32        write_len;

  IFDBG(2) g_debug ("Process layer resource block luni: Unicode Name");
  if (lyr_a->name)
    g_free (lyr_a->name);

  lyr_a->name = fread_unicode_string (&read_len, &write_len, 4,
                                      res_a->ibm_pc_format, input, error);

  if (*error)
    return -1;
  IFDBG(3) g_debug ("Unicode name: %s", lyr_a->name);

  return 0;
}

static gint
load_resource_lyid (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    GInputStream       *input,
                    GError            **error)
{
  /* Load layer id (tattoo) */

  IFDBG(2) g_debug ("Process layer resource block lyid: Layer ID");
  if (psd_read (input, &lyr_a->id, 4, error) < 4)
    {
      psd_set_error (error);
      return -1;
    }
  lyr_a->id = GUINT32_FROM_BE (lyr_a->id);
  IFDBG(3) g_debug ("Layer id: %i", lyr_a->id);

  return 0;
}

static gint
load_resource_lclr (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    GInputStream       *input,
                    GError            **error)
{
  /* Load layer sheet color code */
  IFDBG(2) g_debug ("Process layer resource block %.4s: Sheet color",
                    res_a->key);

  if (psd_read (input, lyr_a->color_tag, 8, error) < 8)
    {
      psd_set_error (error);
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
                    GInputStream       *input,
                    GError            **error)
{
  /* Load layer group & type information
   * Type 0: not a group
   * Type 1: Open folder
   * Type 2: Closed folder
   * Type 3: End of most recent group */
  guint32 type = 0;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Section divider", res_a->key);
  if (psd_read (input, &type, 4, error) < 4)
    {
      psd_set_error (error);
      return -1;
    }
  type = GUINT32_FROM_BE (type);
  IFDBG(3) g_debug ("Section divider type: %i", type);

  lyr_a->group_type = type;

  if (res_a->data_len >= 12)
    {
      gchar signature[4];
      gchar blend_mode[4];

      if (psd_read (input, signature,  4, error) < 4 ||
          psd_read (input, blend_mode, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      /* Not sure if 8B64 is possible here but it won't hurt to check. */
      if (memcmp (signature, "8BIM", 4) == 0 ||
          memcmp (signature, "8B64", 4) == 0)
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
                    GInputStream       *input,
                    GError            **error)
{
  gint16 version = 0;
  gint16 count   = 0;
  gchar  signature[4];
  gchar  effectname[4];
  gint   i;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Layer effects", res_a->key);

  if (psd_read (input, &version, 2, error) < 2 ||
      psd_read (input, &count,   2, error) < 2)
    {
      psd_set_error (error);
      return -1;
    }

  for (i = 0; i < count; i++)
    {
      if (psd_read (input, &signature,  4, error) < 4 ||
          psd_read (input, &effectname, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }

      /* Not sure if 8B64 is possible here but it won't hurt to check. */
      if (memcmp (signature, "8BIM", 4) != 0 &&
          memcmp (signature, "8B64", 4) != 0)
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

              if (psd_read (input, &size,    4, error) < 4 ||
                  psd_read (input, &ver,     4, error) < 4 ||
                  psd_read (input, &visible, 1, error) < 1 ||
                  psd_read (input, &unused,  2, error) < 2)
                {
                  psd_set_error (error);
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

              if (psd_read (input, &size,        4, error) < 4 ||
                  psd_read (input, &ver,         4, error) < 4 ||
                  psd_read (input, &blur,        4, error) < 4 ||
                  psd_read (input, &intensity,   4, error) < 4 ||
                  psd_read (input, &angle,       4, error) < 4 ||
                  psd_read (input, &distance,    4, error) < 4 ||
                  psd_read (input, &color[0],    2, error) < 2 ||
                  psd_read (input, &color[1],    2, error) < 2 ||
                  psd_read (input, &color[2],    2, error) < 2 ||
                  psd_read (input, &color[3],    2, error) < 2 ||
                  psd_read (input, &color[4],    2, error) < 2 ||
                  psd_read (input, &blendsig,    4, error) < 4 ||
                  psd_read (input, &effect,      4, error) < 4 ||
                  psd_read (input, &effecton,    1, error) < 1 ||
                  psd_read (input, &anglefx,     1, error) < 1 ||
                  psd_read (input, &opacity,     1, error) < 1 ||
                  psd_read (input, &natcolor[0], 2, error) < 2 ||
                  psd_read (input, &natcolor[1], 2, error) < 2 ||
                  psd_read (input, &natcolor[2], 2, error) < 2 ||
                  psd_read (input, &natcolor[3], 2, error) < 2 ||
                  psd_read (input, &natcolor[4], 2, error) < 2)
                {
                  psd_set_error (error);
                  return -1;
                }
            }
          else if (memcmp (effectname, "oglw", 4) == 0)
            {
              gint32    size = 0;
              gint32    ver;
              gint32    blur;
              gint32    intensity;
              gint16    color[5];
              gint32    blendsig;
              gint32    effect;
              gchar     effecton;
              gchar     opacity;
              gint16    natcolor[5];

              if (psd_read (input, &size,      4, error) < 4 ||
                  psd_read (input, &ver,       4, error) < 4 ||
                  psd_read (input, &blur,      4, error) < 4 ||
                  psd_read (input, &intensity, 4, error) < 4 ||
                  psd_read (input, &color[0],  2, error) < 2 ||
                  psd_read (input, &color[1],  2, error) < 2 ||
                  psd_read (input, &color[2],  2, error) < 2 ||
                  psd_read (input, &color[3],  2, error) < 2 ||
                  psd_read (input, &color[4],  2, error) < 2 ||
                  psd_read (input, &blendsig,  4, error) < 4 ||
                  psd_read (input, &effect,    4, error) < 4 ||
                  psd_read (input, &effecton,  1, error) < 1 ||
                  psd_read (input, &opacity,   1, error) < 1)
                {
                  psd_set_error (error);
                  return -1;
                }

              if (size == 42)
                {
                  if (psd_read (input, &natcolor[0], 2, error) < 2 ||
                      psd_read (input, &natcolor[1], 2, error) < 2 ||
                      psd_read (input, &natcolor[2], 2, error) < 2 ||
                      psd_read (input, &natcolor[3], 2, error) < 2 ||
                      psd_read (input, &natcolor[4], 2, error) < 2)
                    {
                      psd_set_error (error);
                      return -1;
                    }
                }
            }
          else if (memcmp (effectname, "iglw", 4) == 0)
            {
              gint32 size = 0;
              gint32 ver;
              gint32 blur;
              gint32 intensity;
              gint32 angle;
              gint32 distance;
              gint16 color[5];
              gint32 blendsig;
              gint32 effect;
              gchar  effecton;
              gchar  anglefx;
              gchar  opacity;
              gchar  invert;
              gint16 natcolor[5];

              if (psd_read (input, &size,        4, error) < 4 ||
                  psd_read (input, &ver,         4, error) < 4 ||
                  psd_read (input, &blur,        4, error) < 4 ||
                  psd_read (input, &intensity,   4, error) < 4 ||
                  psd_read (input, &angle,       4, error) < 4 ||
                  psd_read (input, &distance,    4, error) < 4 ||
                  psd_read (input, &color[0],    2, error) < 2 ||
                  psd_read (input, &color[1],    2, error) < 2 ||
                  psd_read (input, &color[2],    2, error) < 2 ||
                  psd_read (input, &color[3],    2, error) < 2 ||
                  psd_read (input, &color[4],    2, error) < 2 ||
                  psd_read (input, &blendsig,    4, error) < 4 ||
                  psd_read (input, &effect,      4, error) < 4 ||
                  psd_read (input, &effecton,    1, error) < 1 ||
                  psd_read (input, &anglefx,     1, error) < 1 ||
                  psd_read (input, &opacity,     1, error) < 1 ||
                  psd_read (input, &natcolor[0], 2, error) < 2 ||
                  psd_read (input, &natcolor[1], 2, error) < 2 ||
                  psd_read (input, &natcolor[2], 2, error) < 2 ||
                  psd_read (input, &natcolor[3], 2, error) < 2 ||
                  psd_read (input, &natcolor[4], 2, error) < 2)
                {
                  psd_set_error (error);
                  return -1;
                }

              if (size == 43)
                {
                  if (psd_read (input, &invert,      1, error) < 1 ||
                      psd_read (input, &natcolor[0], 2, error) < 2 ||
                      psd_read (input, &natcolor[0], 2, error) < 2 ||
                      psd_read (input, &natcolor[1], 2, error) < 2 ||
                      psd_read (input, &natcolor[2], 2, error) < 2 ||
                      psd_read (input, &natcolor[3], 2, error) < 2 ||
                      psd_read (input, &natcolor[4], 2, error) < 2)
                    {
                      psd_set_error (error);
                      return -1;
                    }
                }
            }
          else if (memcmp (effectname, "bevl", 4) == 0)
            {
              gint32    size = 0;
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

              if (psd_read (input, &size,              4, error) < 4 ||
                  psd_read (input, &ver,               4, error) < 4 ||
                  psd_read (input, &angle,             4, error) < 4 ||
                  psd_read (input, &strength,          4, error) < 4 ||
                  psd_read (input, &blur,              4, error) < 4 ||
                  psd_read (input, &highlightsig,      4, error) < 4 ||
                  psd_read (input, &highlighteffect,   4, error) < 4 ||
                  psd_read (input, &shadowsig,         4, error) < 4 ||
                  psd_read (input, &highlightcolor[0], 2, error) < 2 ||
                  psd_read (input, &shadoweffect,      4, error) < 4 ||
                  psd_read (input, &highlightcolor[1], 2, error) < 2 ||
                  psd_read (input, &highlightcolor[2], 2, error) < 2 ||
                  psd_read (input, &highlightcolor[3], 2, error) < 2 ||
                  psd_read (input, &highlightcolor[4], 2, error) < 2 ||
                  psd_read (input, &shadowcolor[0],    2, error) < 2 ||
                  psd_read (input, &shadowcolor[1],    2, error) < 2 ||
                  psd_read (input, &shadowcolor[2],    2, error) < 2 ||
                  psd_read (input, &shadowcolor[3],    2, error) < 2 ||
                  psd_read (input, &shadowcolor[4],    2, error) < 2 ||
                  psd_read (input, &style,             1, error) < 1 ||
                  psd_read (input, &highlightopacity,  1, error) < 1 ||
                  psd_read (input, &shadowopacity,     1, error) < 1 ||
                  psd_read (input, &enabled,           1, error) < 1 ||
                  psd_read (input, &global,            1, error) < 1 ||
                  psd_read (input, &direction,         1, error) < 1)
                {
                  psd_set_error (error);
                  return -1;
                }

              if (size == 78)
                {
                  if (psd_read (input, &highlightnatcolor[0], 2, error) < 2 ||
                      psd_read (input, &highlightnatcolor[0], 2, error) < 2 ||
                      psd_read (input, &highlightnatcolor[1], 2, error) < 2 ||
                      psd_read (input, &highlightnatcolor[2], 2, error) < 2 ||
                      psd_read (input, &highlightnatcolor[3], 2, error) < 2 ||
                      psd_read (input, &highlightnatcolor[4], 2, error) < 2 ||
                      psd_read (input, &shadownatcolor[0],    2, error) < 2 ||
                      psd_read (input, &shadownatcolor[0],    2, error) < 2 ||
                      psd_read (input, &shadownatcolor[1],    2, error) < 2 ||
                      psd_read (input, &shadownatcolor[2],    2, error) < 2 ||
                      psd_read (input, &shadownatcolor[3],    2, error) < 2 ||
                      psd_read (input, &shadownatcolor[4],    2, error) < 2)
                    {
                      psd_set_error (error);
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

              if (psd_read (input, &size,        4, error) < 4 ||
                  psd_read (input, &ver,         4, error) < 4 ||
                  psd_read (input, &key,         4, error) < 4 ||
                  psd_read (input, &color[0],    2, error) < 2 ||
                  psd_read (input, &color[1],    2, error) < 2 ||
                  psd_read (input, &color[2],    2, error) < 2 ||
                  psd_read (input, &color[3],    2, error) < 2 ||
                  psd_read (input, &color[4],    2, error) < 2 ||
                  psd_read (input, &opacity,     1, error) < 1 ||
                  psd_read (input, &enabled,     1, error) < 1 ||
                  psd_read (input, &natcolor[0], 2, error) < 2 ||
                  psd_read (input, &natcolor[1], 2, error) < 2 ||
                  psd_read (input, &natcolor[2], 2, error) < 2 ||
                  psd_read (input, &natcolor[3], 2, error) < 2 ||
                  psd_read (input, &natcolor[4], 2, error) < 2)
                {
                  psd_set_error (error);
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
                    GInputStream       *input,
                    GError            **error)
{
  gint32 version = 0;

  IFDBG(2) g_debug ("Process layer resource block %.4s: layer version",
                    res_a->key);

  if (psd_read (input, &version, 4, error) < 4)
    {
      psd_set_error (error);
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
                    GInputStream       *input,
                    GError            **error)
{
  gchar layername[4];

  IFDBG(2) g_debug ("Process layer resource block %.4s: layer source name",
                    res_a->key);

  if (psd_read (input, &layername, 4, error) < 4)
    {
      psd_set_error (error);
      return -1;
    }

  /* nowadays psd files, layer name are encoded in unicode, cf "luni"
   * moreover lnsr info is encoded in MacRoman, see
   * https://bugzilla.gnome.org/show_bug.cgi?id=753986#c4
   */

  return 0;
}
