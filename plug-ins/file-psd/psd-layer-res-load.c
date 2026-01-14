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
#include <json-glib/json-glib.h>
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

/*  Single layer resources */
static gint     load_resource_ladj    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_lpla    (const PSDlayerres     *res_a,
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

static gint     load_resource_cinf    (const PSDlayerres     *res_a,
                                       PSDlayer              *lyr_a,
                                       GInputStream          *input,
                                       GError               **error);

/*  Image resources */
static gint     load_resource_llnk    (const PSDlayerres     *res_a,
                                       PSDimage              *img_a,
                                       GInputStream          *input,
                                       GError               **error);

static gint     load_resource_ltxt    (const PSDlayerres     *res_a,
                                       PSDimage              *img_a,
                                       GInputStream          *input,
                                       GError               **error);

/*  Other functions */
static gint     parse_text_info       (guint32                len,
                                       gchar                 *buf,
                                       GError               **error);

static gint     parse_descriptor      (GInputStream          *input,
                                       gboolean               ibm_pc_format,
                                       JsonNode             **root_node,
                                       GError               **error);

static gint     load_descriptor       (GInputStream          *input,
                                       gboolean               ibm_pc_format,
                                       JsonNode             **base_node,
                                       GError               **error);

static gchar  * load_key              (GInputStream          *input,
                                       GError               **error);

static gint     load_type             (GInputStream          *input,
                                       gboolean               ibm_pc_format,
                                       const gchar           *class_id,
                                       const gchar           *key,
                                       const gchar           *type,
                                       JsonNode	            **node,
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
      IFDBG(4) g_debug ("PSB: Using block_len_size %d", block_len_size);
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

  else if (memcmp (res_a->key, PSD_LFX_FX, 4) == 0)
    {
      /* TODO: Remove once all legacy layer styles are
       * implemented */
      if (lyr_a)
        {
          lyr_a->unsupported_features->layer_effect = TRUE;
          lyr_a->unsupported_features->show_gui     = TRUE;
        }

      load_resource_lrfx (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LFX_FX2, 4) == 0)
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

      load_resource_lpla (res_a, lyr_a, input, error);
    }

  else if (memcmp (res_a->key, PSD_LOTH_COMPOSITOR, 4) == 0)
    load_resource_cinf (res_a, lyr_a, input, error);

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

gint
load_resource (PSDlayerres   *res_a,
               PSDimage      *img_a,
               GInputStream  *input,
               GError       **error)
{
  /* Set file position to start of layer resource data block */
  if (! psd_seek (input, res_a->data_start, G_SEEK_SET, error))
    {
      psd_set_error (error);
      return -1;
    }

  /* Process global layer resource blocks */
  if (memcmp (res_a->key, PSD_LOTH_TEXT_ENGINE, 4) == 0)
    load_resource_ltxt (res_a, img_a, input, error);

  else if (memcmp (res_a->key, PSD_LLL_LINKED_LAYER, 4) == 0   ||
      memcmp (res_a->key, PSD_LLL_LINKED_LAYER_2, 4) == 0 ||
      memcmp (res_a->key, PSD_LLL_LINKED_LAYER_3, 4) == 0 ||
      memcmp (res_a->key, PSD_LLL_LINKED_LAYER_EXT, 4) == 0)
    {
      load_resource_llnk (res_a, img_a, input, error);
    }

  /* Other resources seen here: fxrp, Patt, FMsk */

  else
    {
      load_resource_unknown (res_a, NULL, input, error);
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
load_resource_lpla (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    GInputStream       *input,
                    GError            **error)
{
  /* Load placed layer */
  static gboolean   msg_flag = FALSE;
  gchar             type[4];
  guint32           version  = 0;
  gchar            *uniqueID = NULL;
  JsonNode         *root     = NULL;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Placed layer", res_a->key);

  if (psd_read (input, &type, 4, error) < 4 ||
      psd_read (input, &version, 4, error) < 4)
    {
      psd_set_error (error);
      return -1;
    }
  version = GUINT32_FROM_BE (version);
  /* Expected
   * for plLd: type: plcL, version: 3,
   * If the below is present, there usually also is one of the above present,
   * but that can then be ignored.
   * for SoLd: type: soLD, version: 4
   * for SoLE, type: soLD, version: 4 or 5. */

   IFDBG(3) g_debug ("Placed layer type: %.4s, version: %u", type, version);

  if (version == 3)
    {
      gint32  bread, bwritten;
      guint32 page_num          = 0;
      guint32 total_pages       = 0;
      guint32 anti_alias_policy = 0;
      guint32 placed_layer_type = 0;

      /* Read pascal string */
      uniqueID = fread_pascal_string (&bread, &bwritten, 1, input, error);
      if (! uniqueID)
        {
          psd_set_error (error);
          return -1;
        }
      g_free (uniqueID);

      if (psd_read (input, &page_num,    4, error) < 4 ||
          psd_read (input, &total_pages, 4, error) < 4 ||
          psd_read (input, &anti_alias_policy, 4, error) < 4 ||
          psd_read (input, &placed_layer_type, 4, error) < 4)
        {
          g_free (uniqueID);
          psd_set_error (error);
          return -1;
        }
      page_num    = GUINT32_FROM_BE (page_num);
      total_pages = GUINT32_FROM_BE (total_pages);
      anti_alias_policy = GUINT32_FROM_BE (anti_alias_policy);
      placed_layer_type = GUINT32_FROM_BE (placed_layer_type);
      IFDBG(3) g_debug ("Page number: %u, total pages: %u, anti alias policy: %u, placed layer type: %u",
                        page_num, total_pages, anti_alias_policy, placed_layer_type);
      /* More info follows:
       * - Transformation: 8 doubles
       * - Warp: version (0), descriptor version (16), descriptor
       */
      lyr_a->smart_object.version = version;
      /* FIXME: We could think of wrapping the above data in a json object. */
    }
  else if (version == 4)
    {
      guint32 descriptor_version = 0;

      if (psd_read (input, &descriptor_version, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      descriptor_version = GUINT32_FROM_BE (descriptor_version);
      IFDBG(3) g_debug ("Descriptor version: %u",
                        descriptor_version);

      if (parse_descriptor (input, res_a->ibm_pc_format, &root, error) == 0)
        {
          if (lyr_a->smart_object.smart_object_data)
            g_warning ("Duplicate smart object data ignored, shouldn't happen!");
          else
            {
              lyr_a->smart_object.smart_object_data = root;
              lyr_a->smart_object.version = version;
            }

          /* FIXME: Temporary show json output while we work on this. */
          IFDBG(4) if (root) g_debug ("Placed Layer descriptor for layer %u:\n%s",
                                      lyr_a->id, json_to_string (root, TRUE));
        }
      else
        return -1;
    }
  else if (version == 5)
    {
      if (parse_descriptor (input, res_a->ibm_pc_format, &root, error) == 0)
        {
          if (lyr_a->smart_object.smart_object_data)
            g_warning ("Duplicate smart object data ignored, shouldn't happen!");
          else
            {
              lyr_a->smart_object.smart_object_data = root;
              lyr_a->smart_object.version = version;
            }

          /* FIXME: Temporary show json output while we work on this. */
          IFDBG(4) if (root) g_debug ("Placed Layer descriptor for layer %u:\n%s",
                                      lyr_a->id, json_to_string (root, TRUE));
        }
      else
        return -1;
    }
  else
    {
      g_debug ("FIXME: Unknown version %u placed layer resource!", version);
    }

  if (! msg_flag && CONVERSION_WARNINGS)
    {
      g_message ("Warning:\n"
                 "The image file contains placed or smart object layers. "
                 "These are not supported by the GIMP and will "
                 "be dropped.");
      msg_flag = TRUE;
    }

  return 0;
}

static gint
get_link_type (gchar *lnk_type)
{
  if (memcmp (lnk_type, "liFD", 4) == 0)
    return LINK_TYPE_FILE_DATA;
  else if (memcmp (lnk_type, "liFE", 4) == 0)
    return LINK_TYPE_EXTERNAL;
  else if (memcmp (lnk_type, "liFA", 4) == 0)
    return LINK_TYPE_ALIAS;

  return -1;
}

static gint
load_resource_llnk (const PSDlayerres     *res_a,
                    PSDimage              *img_a,
                    GInputStream          *input,
                    GError               **error)
{
  gsize          data_size = 0;
  goffset        link_data_offset, next_block_offset;
  guint16        next_block_size = 0;
  gchar          lnk_type[4]; /* 'liFD' linked file data, 'liFE' linked file external or 'liFA' linked file alias */
  guint32        ver, desc_ver;
  gchar          file_creator[4];
  gboolean       file_descriptor_present = 0;
  gint32         bread, bwritten;
  gint           li = 0;
  gint           res;
  PSDLinkedData *link_data = NULL;

  IFDBG(2) g_debug ("Process layer resource block %.4s: linked layer data.",
                    res_a->key);

  link_data_offset = PSD_TELL(input);
  if (psd_read (input, &data_size, 8, error) < 8)
    {
      psd_set_error (error);
      return -1;
    }
  data_size = GUINT64_FROM_BE (data_size);
  next_block_offset = link_data_offset + data_size + 8;

  li = 0;
  while (TRUE)
    {
      IFDBG(3) g_debug ("Linked data[%d], Offset: %" G_GOFFSET_FORMAT, li, link_data_offset);
      if (psd_read (input, (gchar *) &lnk_type, 4, error) < 4 ||
          psd_read (input, &ver, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }

      IFDBG(3) g_debug ("Next data block offset: %" G_GSIZE_FORMAT, link_data_offset + data_size + 8);

      link_data = g_new0 (PSDLinkedData, 1);
      if (img_a->linked_files.linked_data == NULL)
        {
          img_a->linked_files.linked_data = link_data;
          img_a->linked_files.last = link_data;
        }
      else
        {
          img_a->linked_files.last->next = link_data;
          img_a->linked_files.last = link_data;
        }

      link_data->link_type = get_link_type ((gchar *) &lnk_type);

      ver = GUINT32_FROM_BE (ver);
      IFDBG(3) g_debug ("Data size: %" G_GSIZE_FORMAT ", link type: %.4s, version %u", data_size, lnk_type, ver);

      link_data->link_id = fread_pascal_string (&bread, &bwritten, 1, input, error);
      if (! link_data->link_id)
        {
          return -1;
        }
      IFDBG(3) g_debug ("Unique file ID: %s", link_data->link_id);

      link_data->original_filename = fread_unicode_string (&bread, &bwritten, 1,
                                                           res_a->ibm_pc_format,
                                                           input, error);
      if (! link_data->original_filename)
        {
          return -1;
        }

      IFDBG(3) g_debug ("Original filename: %s", link_data->original_filename);

      if (psd_read (input, &link_data->file_type, 4, error) < 4 ||
          psd_read (input, &file_creator,         4, error) < 4 ||
          psd_read (input, &link_data->data_size, 8, error) < 8 ||
          psd_read (input, &file_descriptor_present, 1, error) < 1)
        {
          psd_set_error (error);
          return -1;
        }
      link_data->data_size = GUINT64_FROM_BE (link_data->data_size);
      IFDBG(3) g_debug ("File type: %.4s, creator: %.4s, data length: %" G_GSIZE_FORMAT
                        ", file descriptor present: %u",
                        link_data->file_type, file_creator, link_data->data_size,
                        (guchar) file_descriptor_present);

      if (file_descriptor_present)
        {
          IFDBG(3) g_debug ("Offset descriptor: %" G_GOFFSET_FORMAT, PSD_TELL(input));

          if (psd_read (input, &desc_ver, 4, error) < 4)
            {
              psd_set_error (error);
              return -1;
            }
          desc_ver = GUINT32_FROM_BE (desc_ver);

          IFDBG(4) g_debug ("Descriptor version: %u", desc_ver);

          /* Doesn't look like this descriptor has anything we need. */
          res = load_descriptor (input, res_a->ibm_pc_format, NULL, error);
          if (res < 0)
            return -1;
          }

      if (link_data->link_type == LINK_TYPE_EXTERNAL)
        {
          gint  external_desc_ver = 0;
          gsize filesize          = 0;

          if (psd_read (input, &external_desc_ver, 4, error) < 4)
            {
              psd_set_error (error);
              return -1;
            }
          external_desc_ver = GUINT32_FROM_BE (external_desc_ver);

          IFDBG(4) g_debug ("External link descriptor version: %u", external_desc_ver);

          /* Descriptor with 'ExternalFileLink', consisting of 5 items:
           *   + 'descVersion' (version = 1)
           *   + 'Nm  ' (filename)
           *   + 'fullPath', e.g. file://...
           *   + 'alis' (alias) unknown
           *   + 'relPath', (just the filename, is a relative path also possible?)
           */

          if (parse_descriptor (input, res_a->ibm_pc_format, &link_data->external_file, error) == 0)
            {
              /* Print json for debugging... */
              IFDBG(4) if (link_data->external_file)
                         g_debug ("External link descriptor:\n%s\n",
                                  json_to_string (link_data->external_file, TRUE));
            }
          else
            {
              return -1;
            }

          if (ver > 3)
            {
              /* Date/Time, I assume of latest update of linked file. */
              guint32 year    = 0;
              guchar  month   = 0, day = 0, hour = 0, minute = 0;
              gdouble seconds = 0.0;

              if (psd_read (input, &year, 4, error) < 4 ||
                  psd_read (input, &month, 1, error) < 1 ||
                  psd_read (input, &day, 1, error) < 1 ||
                  psd_read (input, &hour, 1, error) < 1 ||
                  psd_read (input, &minute, 1, error) < 1 ||
                  ! psd_read_double (input, &seconds, error))
                {
                  psd_set_error (error);
                  return -1;
                }
              year = GUINT32_FROM_BE (year);
              IFDBG(3) g_debug ("File date/time: %u-%u-%u %u:%u:%f", year, month, day, hour, minute, seconds);
            }
          if (psd_read (input, &filesize, 8, error) < 8)
            {
              psd_set_error (error);
              return -1;
            }
          filesize = GUINT64_FROM_BE (filesize);
          IFDBG(3) g_debug ("Filesize: %" G_GSIZE_FORMAT, filesize);

          /* Specs say actual file data follows here, but that is incorrect. */
        }
      else if (link_data->link_type == LINK_TYPE_FILE_DATA)
        {
          /* Actual image data is included here, e.g. a complete PNG including headers etc. */

          link_data->data_offset = PSD_TELL(input) + link_data->data_size;
          IFDBG(3) g_debug ("Offset of included file data: %" G_GOFFSET_FORMAT
                            ", end offset: %" G_GOFFSET_FORMAT,
                            PSD_TELL(input), link_data->data_offset);

          /* FIXME Skip actual image data for now... */
          if (! psd_seek (input, link_data->data_offset, G_SEEK_SET, error))
            {
              psd_set_error (error);
              return -1;
            }
        }
      else if (link_data->link_type == LINK_TYPE_ALIAS)
        {
          gsize dummy = 0;

          /* FIXME We don't have an example of this yet. No idea how to interpret. */
          g_debug ("Skipping alias link\n");

          /* 8 zeros according to specs */
          if (psd_read (input, &dummy, 8, error) < 8)
            {
              psd_set_error (error);
              return -1;
            }
        }

      IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));

      if (ver >= 5)
        {
          gchar *utemp = NULL;

          utemp = fread_unicode_string (&bread, &bwritten, 1,
                                        res_a->ibm_pc_format,
                                        input, error);
          if (! utemp)
            {
              return -1;
            }
          IFDBG(3) g_debug ("Child Document ID: %s", utemp);
          g_free (utemp);
        }
      if (ver >= 6)
        {
          gdouble mod_time;

          if (! psd_read_double (input, &mod_time, error))
            {
              psd_set_error (error);
              return -1;
            }
          IFDBG(3) g_debug ("Asset mod time: %f", mod_time);
        }

      if (ver >= 7)
        {
          guchar locked_state = 0;

          if (psd_read (input, &locked_state, 1, error) < 1)
            {
              psd_set_error (error);
              return -1;
            }
          IFDBG(3) g_debug ("Locked state: %u", locked_state);
        }

      if (link_data->link_type == LINK_TYPE_EXTERNAL && ver == 2)
        {
          /* FIXME Raw file data:
           * Weird: the specs don't say anything about the version earlier
           * when the raw file data was mentioned for external files.
           * We probably need an example.
           */
          IFDBG(3) g_debug ("External link version 2: raw file data included. "
                            "Never seen this yet! Please share an exmple.");
        }

      /* To make sure we arrive at the correct offset we seek to the expected
       * location. */
      next_block_offset = (next_block_offset + 3) / 4 * 4;
      if (! psd_seek (input, next_block_offset, G_SEEK_SET, error))
        {
          psd_set_error (error);
          return -1;
        }
      IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));

      link_data_offset = PSD_TELL(input);
      if (link_data->link_type == LINK_TYPE_EXTERNAL)
        {
          if (psd_read (input, &data_size, 8, error) < 8)
            {
              psd_set_error (error);
              return -1;
            }
          data_size = GUINT64_FROM_BE (data_size);
          IFDBG(3) g_debug ("Next block data size: %" G_GSIZE_FORMAT, data_size);
        }

      if (psd_read (input, &next_block_size, 2, error) < 2)
        {
          psd_set_error (error);
          return -1;
        }
      next_block_size = GUINT16_FROM_BE (next_block_size);
      if (next_block_size == 0)
        break;
      next_block_offset += next_block_size;

      IFDBG(3) g_debug ("Next block size: %u", next_block_size);
      li++;
    }

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

  if (memcmp (res_a->key, PSD_LFX_FX2, 4) == 0)
    {
      guint32    oe_version   = 0;
      guint32    desc_version = 0;
      JsonNode  *root         = NULL;

      if (psd_read (input, &oe_version, 4, error) < 4 ||
          psd_read (input, &desc_version, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      oe_version   = GUINT32_FROM_BE (oe_version);
      desc_version = GUINT32_FROM_BE (desc_version);

      IFDBG(3) g_debug ("Objects based effects layer info: object effects version: %u, descriptor version: %u", oe_version, desc_version);

      if (parse_descriptor (input, res_a->ibm_pc_format, &root, error) == 0)
        {
          lyr_a->layer_effects.effects = root;

          if (root)
            {
              /* Print json for debugging... */
              IFDBG(3) g_debug ("Layer Effects descriptor for layer %u:\n%s",
                                lyr_a->id, json_to_string (root, TRUE));
            }
        }
      else
        {
          return -1;
        }
    }

  return 0;
}

static gint
load_resource_ltyp (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    GInputStream       *input,
                    GError            **error)
{
  /* Load type tool layer */
  gint16            version           = 0;
  gint16            text_desc_vers    = 0;
  gint32            desc_version      = 0;
  gint32            warp_version      = 0;
  gint32            warp_desc_version = 0;
  gint32            dimensions[4];
  gint64            transform[6];
  gdouble           d_transform[6];
  gint              i, res;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Type tool layer", res_a->key);

  /* New style type tool layers (ps6) */
  if (memcmp (res_a->key, PSD_LTYP_TYPE2, 4) == 0)
    {
      lyr_a->text.info = NULL;

      if (psd_read (input, &version,      2, error) < 2 ||
          psd_read (input, &transform[0], 8, error) < 8 ||
          psd_read (input, &transform[1], 8, error) < 8 ||
          psd_read (input, &transform[2], 8, error) < 8 ||
          psd_read (input, &transform[3], 8, error) < 8 ||
          psd_read (input, &transform[4], 8, error) < 8 ||
          psd_read (input, &transform[5], 8, error) < 8 ||
          psd_read (input, &text_desc_vers, 2, error) < 2 ||
          psd_read (input, &desc_version,   4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }

      version = GINT16_FROM_BE (version);
      text_desc_vers = GINT16_FROM_BE (text_desc_vers);
      desc_version = GINT32_FROM_BE (desc_version);

      for (i = 0; i <= 5; i++)
        {
          gdouble *val;
          guint64  tmp;

          tmp = GUINT64_FROM_BE (transform[i]);
          val = (gpointer) &tmp;
          d_transform[i] = *val;
        }
      lyr_a->text.xx = d_transform[0];
      lyr_a->text.xy = d_transform[1];
      lyr_a->text.yx = d_transform[2];
      lyr_a->text.yy = d_transform[3];
      lyr_a->text.tx = d_transform[4];
      lyr_a->text.ty = d_transform[5];

      IFDBG(2) g_debug ("Version: %d, Text version: %d, Descriptor version: %d",
                        version, text_desc_vers, desc_version);

      IFDBG(2) g_debug ("Transform\n\txx: %f\n\txy: %f\n\tyx: %f"
                        "\n\tyy: %f\n\ttx: %f\n\tty: %f",
                        lyr_a->text.xx, lyr_a->text.xy, lyr_a->text.yx,
                        lyr_a->text.yy, lyr_a->text.tx, lyr_a->text.ty);

      res = load_descriptor (input, res_a->ibm_pc_format, NULL, error);
      /*
       * This descriptor seems to have a lot of text formatting etc related
       * data in a format that is not described in the online specs...
       * It seems that commands start with a slash followed by a command name,
       * e.g. /Editor, /Text, /EngineDict, /Properties
       */
      if (res < 0)
        return -1;

      if (psd_read (input, &warp_version, 2, error) < 2 ||
          psd_read (input, &warp_desc_version, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }

      warp_version = GINT16_FROM_BE (warp_version);
      warp_desc_version = GINT32_FROM_BE (warp_desc_version);
      IFDBG(2) g_debug ("Warp version: %d, descriptor version: %d",
                        warp_version, warp_desc_version);

      res = load_descriptor (input, res_a->ibm_pc_format, NULL, error);
      if (res < 0)
        return -1;

      IFDBG(3) g_debug ("Offset after warp descriptor: %" G_GOFFSET_FORMAT, PSD_TELL(input));

      if (psd_read (input, &dimensions[0], 4, error) < 4 ||
          psd_read (input, &dimensions[1], 4, error) < 4 ||
          psd_read (input, &dimensions[2], 4, error) < 4 ||
          psd_read (input, &dimensions[3], 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      dimensions[0] = GINT32_FROM_BE (dimensions[0]);
      dimensions[1] = GINT32_FROM_BE (dimensions[1]);
      dimensions[2] = GINT32_FROM_BE (dimensions[2]);
      dimensions[3] = GINT32_FROM_BE (dimensions[3]);

      IFDBG(2) g_debug ("Left: %d, Top: %d, Right: %d, Bottom: %d",
                        dimensions[0], dimensions[1],
                        dimensions[2], dimensions[3]);
      IFDBG(3) g_debug ("End offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));
    }

  return 0;
}

static gint load_resource_ltxt (const PSDlayerres  *res_a,
                                PSDimage           *img_a,
                                GInputStream       *input,
                                GError            **error)
{
  guint32  res_len;
  gchar   *buf;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Type tool layer", res_a->key);

  res_len = res_a->data_len;
  IFDBG(3) g_debug ("Txt2 resource length: %u at offset %" G_GOFFSET_FORMAT,
                    res_len, PSD_TELL(input));

  buf = g_malloc (res_len);
  if (psd_read (input, buf, res_len, error) < res_len)
    {
      psd_set_error (error);
      return -1;
    }
  IFDBG(3) g_debug ("Parse text info at offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));
  parse_text_info (res_len, buf, error);
  g_free(buf);
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

/* Adobe uses fixed point ints which consist of four bytes: a 16-bit number and
 * 16-bit fraction */
#define FIXED_TO_FLOAT(num,fract) (gfloat) num + fract / 65535.0f

static gint
load_resource_lrfx (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    GInputStream       *input,
                    GError            **error)
{
  PSDLayerStyles  *ls_a;
  gchar            signature[4];
  gchar            effectname[5];
  gint             i;

  ls_a = lyr_a->layer_styles;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Layer effects", res_a->key);

  if (psd_read (input, &ls_a->version, 2, error) < 2 ||
      psd_read (input, &ls_a->count,   2, error) < 2)
    {
      psd_set_error (error);
      return -1;
    }
  ls_a->version = GUINT16_TO_BE (ls_a->version);
  ls_a->count = GUINT16_TO_BE (ls_a->count);
  IFDBG(3) g_debug ("Version %u, count %u", ls_a->version, ls_a->count);

  for (i = 0; i < ls_a->count; i++)
    {
      if (psd_read (input, &signature,  4, error) < 4 ||
          psd_read (input, &effectname, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      effectname[4] = '\0';
      IFDBG(3) g_debug ("Signature: %.4s, Effect name: %s", signature, effectname);

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
              if (psd_read (input, &ls_a->cmns.size,    4, error) < 4 ||
                  psd_read (input, &ls_a->cmns.ver,     4, error) < 4 ||
                  psd_read (input, &ls_a->cmns.visible, 1, error) < 1 ||
                  psd_read (input, &ls_a->cmns.unused,  2, error) < 2)
                {
                  psd_set_error (error);
                  return -1;
                }
              IFDBG(3) g_debug ("cmnS (common state info) - size %u, version %u, visible: %u",
                                GUINT32_TO_BE (ls_a->cmns.size), GUINT32_TO_BE (ls_a->cmns.ver),
                                ls_a->cmns.visible );
            }
          else if (memcmp (effectname, "dsdw", 4) == 0 ||
                   memcmp (effectname, "isdw", 4) == 0)
            {
              PSDLayerStyleShadow *shadow;
              gchar                bim[4];
              guint16              temp[8];

              if (memcmp (effectname, "dsdw", 4) == 0)
                shadow = &ls_a->dsdw;
              else
                shadow = &ls_a->isdw;

              if (psd_read (input, &shadow->size,      4, error) < 4  ||
                  psd_read (input, &shadow->ver,       4, error) < 4  ||
                  psd_read (input, &temp,             16, error) < 16 ||
                  psd_read (input, &shadow->color[0],  2, error) < 2  ||
                  psd_read (input, &shadow->color[1],  2, error) < 2  ||
                  psd_read (input, &shadow->color[2],  2, error) < 2  ||
                  psd_read (input, &shadow->color[3],  2, error) < 2  ||
                  psd_read (input, &shadow->color[4],  2, error) < 2  ||
                  psd_read (input, &bim,               4, error) < 4  ||
                  psd_read (input, &shadow->blendsig,  4, error) < 4  ||
                  psd_read (input, &shadow->effecton,  1, error) < 1  ||
                  psd_read (input, &shadow->anglefx,   1, error) < 1  ||
                  psd_read (input, &shadow->opacity,   1, error) < 1)
                {
                  psd_set_error (error);
                  return -1;
                }
              shadow->size      = GUINT32_TO_BE (shadow->size);
              shadow->ver       = GUINT32_TO_BE (shadow->ver);
              shadow->blur      = FIXED_TO_FLOAT (GUINT16_TO_BE (temp[0]),
                                                  GUINT16_TO_BE (temp[1]));
              shadow->intensity = FIXED_TO_FLOAT (GUINT16_TO_BE (temp[2]),
                                                  GUINT16_TO_BE (temp[3]));
              shadow->angle     = FIXED_TO_FLOAT (GINT16_TO_BE (temp[4]),
                                                  GINT16_TO_BE (temp[5]));
              shadow->distance  = FIXED_TO_FLOAT (GUINT16_TO_BE (temp[6]),
                                                  GUINT16_TO_BE (temp[7]));

              IFDBG(3)
                g_debug ("%s - size %u, version %u, effect enabled: %u, "
                         "blur (pixels): %f, intensity (pct): %f, angle (degrees): %f, distance (pixels): %f, "
                         "blendsig: %.4s, effect: %.4s, reuse angle: %u, opacity: %u",
                         effectname,
                         shadow->size, shadow->ver,
                         shadow->effecton,
                         shadow->blur, shadow->intensity,
                         shadow->angle, shadow->distance,
                         bim, shadow->blendsig,
                         shadow->anglefx, shadow->opacity);

              if (shadow->ver == 2)
                {
                  if (psd_read (input, &shadow->natcolor[0], 2, error) < 2 ||
                      psd_read (input, &shadow->natcolor[1], 2, error) < 2 ||
                      psd_read (input, &shadow->natcolor[2], 2, error) < 2 ||
                      psd_read (input, &shadow->natcolor[3], 2, error) < 2 ||
                      psd_read (input, &shadow->natcolor[4], 2, error) < 2)
                    {
                      psd_set_error (error);
                      return -1;
                    }
                }
            }
          else if (memcmp (effectname, "oglw", 4) == 0)
            {
              gchar   bim[4];
              guint16 temp[4];

              if (psd_read (input, &ls_a->oglw.size,      4, error) < 4 ||
                  psd_read (input, &ls_a->oglw.ver,       4, error) < 4 ||
                  psd_read (input, &temp,                 8, error) < 8 ||
                  psd_read (input, &ls_a->oglw.color[0],  2, error) < 2 ||
                  psd_read (input, &ls_a->oglw.color[1],  2, error) < 2 ||
                  psd_read (input, &ls_a->oglw.color[2],  2, error) < 2 ||
                  psd_read (input, &ls_a->oglw.color[3],  2, error) < 2 ||
                  psd_read (input, &ls_a->oglw.color[4],  2, error) < 2 ||
                  psd_read (input, &bim,                  4, error) < 4 ||
                  psd_read (input, &ls_a->oglw.blendsig,  4, error) < 4 ||
                  psd_read (input, &ls_a->oglw.effecton,  1, error) < 1 ||
                  psd_read (input, &ls_a->oglw.opacity,   1, error) < 1)
                {
                  psd_set_error (error);
                  return -1;
                }

              ls_a->oglw.size      = GUINT32_TO_BE (ls_a->oglw.size);
              ls_a->oglw.ver       = GUINT32_TO_BE (ls_a->oglw.ver);
              ls_a->oglw.blur      = FIXED_TO_FLOAT (GUINT16_TO_BE (temp[0]),
                                                     GUINT16_TO_BE (temp[1]));
              ls_a->oglw.intensity = FIXED_TO_FLOAT (GUINT16_TO_BE (temp[2]),
                                                     GUINT16_TO_BE (temp[3]));
              IFDBG(3) g_debug ("oglw - size %u, version %u, effect enabled: %u, "
                                "blur (pixels): %u, intensity (pct): %u, "
                                "blendsig: %.4s, effect %.4s",
                                ls_a->oglw.size, GUINT32_TO_BE (ls_a->oglw.ver),
                                ls_a->oglw.effecton,
                                ls_a->oglw.blur, ls_a->oglw.intensity,
                                ls_a->oglw.blendsig, (gchar *) &ls_a->oglw.effect );

              if (ls_a->oglw.size == 42)
                {
                  if (psd_read (input, &ls_a->oglw.natcolor[0], 2, error) < 2 ||
                      psd_read (input, &ls_a->oglw.natcolor[1], 2, error) < 2 ||
                      psd_read (input, &ls_a->oglw.natcolor[2], 2, error) < 2 ||
                      psd_read (input, &ls_a->oglw.natcolor[3], 2, error) < 2 ||
                      psd_read (input, &ls_a->oglw.natcolor[4], 2, error) < 2)
                    {
                      psd_set_error (error);
                      return -1;
                    }
                }
            }
          else if (memcmp (effectname, "iglw", 4) == 0)
            {
              if (psd_read (input, &ls_a->iglw.size,        4, error) < 4 ||
                  psd_read (input, &ls_a->iglw.ver,         4, error) < 4 ||
                  psd_read (input, &ls_a->iglw.blur,        4, error) < 4 ||
                  psd_read (input, &ls_a->iglw.intensity,   4, error) < 4 ||
                  psd_read (input, &ls_a->iglw.color[0],    2, error) < 2 ||
                  psd_read (input, &ls_a->iglw.color[1],    2, error) < 2 ||
                  psd_read (input, &ls_a->iglw.color[2],    2, error) < 2 ||
                  psd_read (input, &ls_a->iglw.color[3],    2, error) < 2 ||
                  psd_read (input, &ls_a->iglw.color[4],    2, error) < 2 ||
                  psd_read (input, &ls_a->iglw.blendsig,    4, error) < 4 ||
                  psd_read (input, &ls_a->iglw.effect,      4, error) < 4 ||
                  psd_read (input, &ls_a->iglw.effecton,    1, error) < 1 ||
                  psd_read (input, &ls_a->iglw.opacity,     1, error) < 1)
                {
                  psd_set_error (error);
                  return -1;
                }

              ls_a->iglw.size = GUINT32_TO_BE (ls_a->iglw.size);
              IFDBG(3) g_debug ("iglw - size %u, version %u, effect enabled: %u, blur (pixels): %u, intensity (pct): %u, blendsig: %.4s, effect %.4s",
                                ls_a->iglw.size, GUINT32_TO_BE (ls_a->iglw.ver),
                                ls_a->iglw.effecton,
                                ls_a->iglw.blur, ls_a->iglw.intensity,
                                ls_a->iglw.blendsig, (gchar *) &ls_a->iglw.effect );
              if (ls_a->iglw.size == 43)
                {
                  if (psd_read (input, &ls_a->iglw.invert,      1, error) < 1 ||
                      psd_read (input, &ls_a->iglw.natcolor[0], 2, error) < 2 ||
                      psd_read (input, &ls_a->iglw.natcolor[1], 2, error) < 2 ||
                      psd_read (input, &ls_a->iglw.natcolor[2], 2, error) < 2 ||
                      psd_read (input, &ls_a->iglw.natcolor[3], 2, error) < 2 ||
                      psd_read (input, &ls_a->iglw.natcolor[4], 2, error) < 2)
                    {
                      psd_set_error (error);
                      return -1;
                    }
                }
            }
          else if (memcmp (effectname, "bevl", 4) == 0)
            {
              if (psd_read (input, &ls_a->bevl.size,              4, error) < 4 ||
                  psd_read (input, &ls_a->bevl.ver,               4, error) < 4 ||
                  psd_read (input, &ls_a->bevl.angle,             4, error) < 4 ||
                  psd_read (input, &ls_a->bevl.strength,          4, error) < 4 ||
                  psd_read (input, &ls_a->bevl.blur,              4, error) < 4 ||
                  psd_read (input, &ls_a->bevl.highlightsig,      4, error) < 4 ||
                  psd_read (input, &ls_a->bevl.highlighteffect,   4, error) < 4 ||
                  psd_read (input, &ls_a->bevl.shadowsig,         4, error) < 4 ||
                  psd_read (input, &ls_a->bevl.shadoweffect,      4, error) < 4 ||
                  psd_read (input, &ls_a->bevl.highlightcolor[0], 2, error) < 2 ||
                  psd_read (input, &ls_a->bevl.highlightcolor[1], 2, error) < 2 ||
                  psd_read (input, &ls_a->bevl.highlightcolor[2], 2, error) < 2 ||
                  psd_read (input, &ls_a->bevl.highlightcolor[3], 2, error) < 2 ||
                  psd_read (input, &ls_a->bevl.highlightcolor[4], 2, error) < 2 ||
                  psd_read (input, &ls_a->bevl.shadowcolor[0],    2, error) < 2 ||
                  psd_read (input, &ls_a->bevl.shadowcolor[1],    2, error) < 2 ||
                  psd_read (input, &ls_a->bevl.shadowcolor[2],    2, error) < 2 ||
                  psd_read (input, &ls_a->bevl.shadowcolor[3],    2, error) < 2 ||
                  psd_read (input, &ls_a->bevl.shadowcolor[4],    2, error) < 2 ||
                  psd_read (input, &ls_a->bevl.style,             1, error) < 1 ||
                  psd_read (input, &ls_a->bevl.highlightopacity,  1, error) < 1 ||
                  psd_read (input, &ls_a->bevl.shadowopacity,     1, error) < 1 ||
                  psd_read (input, &ls_a->bevl.enabled,           1, error) < 1 ||
                  psd_read (input, &ls_a->bevl.global,            1, error) < 1 ||
                  psd_read (input, &ls_a->bevl.direction,         1, error) < 1)
                {
                  psd_set_error (error);
                  return -1;
                }

              ls_a->bevl.size = GUINT32_TO_BE (ls_a->bevl.size);
              IFDBG(3) g_debug ("bevl - size %u, version %u, effect enabled: %u, highlightsig: %.4s, highlighteffect %.4s, shadowsig: %.4s, shadoweffect %.4s",
                                ls_a->bevl.size, GUINT32_TO_BE (ls_a->bevl.ver),
                                ls_a->bevl.enabled,
                                (gchar *) &ls_a->bevl.highlightsig, (gchar *) &ls_a->bevl.highlighteffect,
                                (gchar *) &ls_a->bevl.shadowsig, (gchar *) &ls_a->bevl.shadoweffect );
              if (ls_a->bevl.size == 78)
                {
                  if (psd_read (input, &ls_a->bevl.highlightnatcolor[0], 2, error) < 2 ||
                      psd_read (input, &ls_a->bevl.highlightnatcolor[1], 2, error) < 2 ||
                      psd_read (input, &ls_a->bevl.highlightnatcolor[2], 2, error) < 2 ||
                      psd_read (input, &ls_a->bevl.highlightnatcolor[3], 2, error) < 2 ||
                      psd_read (input, &ls_a->bevl.highlightnatcolor[4], 2, error) < 2 ||
                      psd_read (input, &ls_a->bevl.shadownatcolor[0],    2, error) < 2 ||
                      psd_read (input, &ls_a->bevl.shadownatcolor[1],    2, error) < 2 ||
                      psd_read (input, &ls_a->bevl.shadownatcolor[2],    2, error) < 2 ||
                      psd_read (input, &ls_a->bevl.shadownatcolor[3],    2, error) < 2 ||
                      psd_read (input, &ls_a->bevl.shadownatcolor[4],    2, error) < 2)
                    {
                      psd_set_error (error);
                      return -1;
                    }
                }
            }
          else if (memcmp (effectname, "sofi", 4) == 0)
            {
              /* Documentation forgets to include the 4 byte 'bim8' before
               * the blend signature */
              gchar blendsig[4];

              if (psd_read (input, &ls_a->sofi.size,        4, error) < 4 ||
                  psd_read (input, &ls_a->sofi.ver,         4, error) < 4 ||
                  psd_read (input, &blendsig,               4, error) < 4 ||
                  psd_read (input, &ls_a->sofi.blend,       4, error) < 4 ||
                  psd_read (input, &ls_a->sofi.color[0],    2, error) < 2 ||
                  psd_read (input, &ls_a->sofi.color[1],    2, error) < 2 ||
                  psd_read (input, &ls_a->sofi.color[2],    2, error) < 2 ||
                  psd_read (input, &ls_a->sofi.color[3],    2, error) < 2 ||
                  psd_read (input, &ls_a->sofi.color[4],    2, error) < 2 ||
                  psd_read (input, &ls_a->sofi.opacity,     1, error) < 1 ||
                  psd_read (input, &ls_a->sofi.enabled,     1, error) < 1 ||
                  psd_read (input, &ls_a->sofi.natcolor[0], 2, error) < 2 ||
                  psd_read (input, &ls_a->sofi.natcolor[1], 2, error) < 2 ||
                  psd_read (input, &ls_a->sofi.natcolor[2], 2, error) < 2 ||
                  psd_read (input, &ls_a->sofi.natcolor[3], 2, error) < 2 ||
                  psd_read (input, &ls_a->sofi.natcolor[4], 2, error) < 2)
                {
                  psd_set_error (error);
                  return -1;
                }
              IFDBG(3) g_debug ("sofi - size %u, version %u, enabled %u, blendsig: %.4s, blend %.4s, opacity %u",
                                GUINT32_TO_BE (ls_a->sofi.size), GUINT32_TO_BE (ls_a->sofi.ver),
                                ls_a->sofi.enabled,
                                blendsig, ls_a->sofi.blend, 100 * ls_a->sofi.opacity / 255 );
            }
          else
            {
              IFDBG(1) g_debug ("Unknown layer effect signature %.4s", effectname);
            }
        }
    }

  return 0;
}
#undef FIXED_TO_FLOAT

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

static gint
load_resource_cinf (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    GInputStream       *input,
                    GError            **error)
{
  guint32 version = 0;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Compositor Used (cinf)", res_a->key);

  if (psd_read (input, &version, 4, error) < 4)
    {
      psd_set_error (error);
      return -1;
    }
  version = GUINT32_FROM_BE (version);
  IFDBG(3) g_debug ("Descriptor version (expecting 16): %u", version);

  if (load_descriptor (input, res_a->ibm_pc_format, NULL, error) < 0)
    return -1;

  return 0;
}

static gint parse_text_info (guint32    len,
                             gchar     *buf,
                             GError   **error)
{
  gint      lvl    = 0;
  gint      ar_lvl = 0;
  guint32   bufpos = 0;
  guint32   cmdpos = 0;
  guint32   val_len = 0;
  guint32   val_pos = 0;
  guint32   value_start_pos = 0;
  /*gchar    *value;*/
  gchar    *last_cmd = NULL;
  gboolean  reading_command = FALSE;
  gboolean  reading_value   = FALSE;
  gboolean  reading_binary  = FALSE;
  /*gboolean  read_utf16      = FALSE;*/

  IFDBG(3) g_debug ("Parsing text info starts...\n");
  while (bufpos < len)
    {
      const gint COMMAND_SIZE = 128; /* No idea what the real Photoshop maximum is. */
      gchar token;
      gchar command[COMMAND_SIZE+1];
      gchar simple_value[COMMAND_SIZE+1];

      token = buf[bufpos];

      if (reading_binary)
        {
          gchar     nexttoken = ' ';
          gboolean  endbinary = FALSE;

          /* reading 16-bit values unless token == ) */
          if (token == ')' && bufpos < len)
            {
              nexttoken = buf[bufpos+1];
              //g_printerr ("bufpos: %u, token: %.2x\n", bufpos+1-value_start_pos, nexttoken);
              if (nexttoken == ' '  ||
                  nexttoken == 0x09 ||
                  nexttoken == 0x0a)
                endbinary = TRUE;
            }

          if (! endbinary)
            {
              bufpos++;
              /*if (bufpos < len)
                bufpos++;*/
              continue;
            }
          else
            {
              /* ...nothing to do here */
            }
        }

      switch (token)
        {
        case '(': /* start of binary value */
          reading_binary = TRUE;
          value_start_pos = bufpos + 1;
          IFDBG(4) g_debug ("Binary value start");
          break;
        case ')': /* end of binary value */
          reading_binary = FALSE;
          /*read_utf16    = FALSE;*/
          val_len = bufpos - value_start_pos;
          /* TODO: Copy value to buffer and then use a separate function
                   to parse that value... */
          if (strcmp (last_cmd, "Name") == 0 ||
              strcmp (last_cmd, "InternalName") == 0 ||
              strcmp (last_cmd, "Text") == 0)
            {
              gchar  *name_utf16;
              gchar  *name_utf8;
              GError *error = NULL;
              gsize   bw, br;

              /* Get the name as UTF-8 from UTF-16 data. */
              name_utf16 = g_malloc0 (val_len + 2);
              memcpy (name_utf16, &buf[value_start_pos+2], val_len-2);
              name_utf8 = g_convert (name_utf16, val_len, "utf-8", "utf-16BE", &br, &bw, &error);
              //g_printerr ("utf8 length: %d, bytes read: %d, written: %d\n", strlen(name_utf8), br, bw);
              if (error)
                {
                  g_printerr ("--> Error converting utf-16 to utf-8: %s", error->message);
                  g_clear_error (&error);
                }
              IFDBG(2) g_debug ("Text value: '%s'", name_utf8);
              g_free (name_utf16);
              g_free (name_utf8);
            }
          IFDBG(4) g_debug ("Binary value end, value length: %u bytes", val_len);
          break;
        case '/': /* start of command */
          reading_command = TRUE;
          cmdpos = 0;
          break;
        case 'a'...'z':
        case 'A'...'Z':
        case '0'...'9':
        case '-':
        case '.':
          if (reading_command)
            {
              if (token == '-' || token == '.')
                {
                  IFDBG(2) g_debug ("Unexpected token %c in command!", token);
                }
              else if (cmdpos < COMMAND_SIZE)
                {
                  command[cmdpos++] = token;
                }
              else
                {
                  g_printerr ("Warning: text command token too long!\n");
                }
            }
          else if (! reading_binary)
            {
              if (! reading_value)
                {
                  reading_value = TRUE;
                  val_pos = 0;
                }
              if (val_pos < COMMAND_SIZE)
                {
                  simple_value[val_pos++] = token;
                }
              else
                {
                  g_printerr ("Warning: text value token too long!\n");
                }
            }
          break;
        case ' ':
        case 0x09: /* TAB character */
        case 0x0a: /* newline character */
          if (reading_command)
            {
              command[cmdpos] = '\0';
              cmdpos = 0;
              reading_command = FALSE;
              g_free (last_cmd);
              last_cmd = g_strdup ((const gchar *) &command);
              IFDBG(2) g_debug ("Command: %s at level %d", command, lvl);
            }
          else if (reading_value)
            {
              simple_value[val_pos] = '\0';
              val_pos = 0;
              reading_value = FALSE;
              IFDBG(2) g_debug ("Value: %s", simple_value);
            }
          break;
        case '<': /* Increase level */
          if (buf[bufpos+1] == '<') //TODO: test buffer overrun
            {
              lvl++;
              bufpos++;
            }
          break;
        case '>': /* Decrease level */
          if (buf[bufpos+1] == '>') //TODO: test buffer overrun
            {
              lvl--;
              bufpos++;
            }
          break;
        case '[': /* begin array */
          ar_lvl++;
          IFDBG(2) g_debug ("Array start, level %d", ar_lvl);
          break;
        case ']': /* end array */
          ar_lvl--;
          IFDBG(2) g_debug ("Array end, level %d", ar_lvl);
          break;
        case (gchar) 0xfe: /* feff is UTF-16 BOM */
          if (reading_command)
            {
              if (buf[bufpos+1] == (gchar) 0xff) //TODO: test buffer overrun
                {
                  bufpos++;
                  IFDBG(2) g_debug ("Value is using UTF-16");
                }
            }
          break;
        default:
          if (reading_command)
            {
              command[cmdpos] = '\0';
              cmdpos = 0;
              g_free (last_cmd);
              last_cmd = g_strdup ((const gchar *) &command);
              reading_command = FALSE;
              IFDBG(2) g_debug ("--> Error: unexpected token! Command: %s at level %d", command, lvl);
              g_warning ("Unexpected token. Command: %s", command);
            }
          else if (reading_value)
            {
              simple_value[val_pos] = '\0';
              val_pos = 0;
              reading_value = FALSE;
              IFDBG(2) g_debug ("--> Error: unexpected token! Value: %s", simple_value);
              g_warning ("Unexpected token value.");
            }
          break;
        }
      bufpos++;
    }
  g_free (last_cmd);
  IFDBG(3) g_debug ("Parsing text info done.\n");

  return bufpos;
}

static gint
parse_descriptor (GInputStream  *input,
                  gboolean       ibm_pc_format,
                  JsonNode     **root_node,
                  GError       **error)
{
  JsonObject *obj = NULL;
  gint        res = 0;

  *root_node = json_node_new (JSON_NODE_OBJECT);
  if (! *root_node)
    {
      /* FIXME: What error class would be most suitable here? */
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error creating json node."));
      return -1;
    }
  obj = json_object_new ();
  json_node_init_object (*root_node, obj);

  /* FIXME: Should we add a version number to the root in case we
   *        need to make changes in the future?
   */

  res = load_descriptor (input, ibm_pc_format, root_node, error);
  if (res < 0)
    {
      json_node_free (*root_node);
      *root_node = NULL;
    }
  else
    {
      /* Set immutable for performance when reading the data. */
      json_node_seal (*root_node);
    }

  return res;
}

static gint
load_descriptor (GInputStream  *input,
                 gboolean       ibm_pc_format,
                 JsonNode     **base_node,
                 GError       **error)
{
  gchar      *uniqueID       = NULL;
  gchar      *classID_string = NULL;
  guint32     num_items      = 0;
  gint32      bread, bwritten;
  gint        i;
  JsonObject *obj            = NULL;
  JsonArray  *arr            = NULL;
  JsonNode   *local          = NULL;
  JsonNode   *root           = NULL;

  IFDBG(3) g_debug ("start load_descriptor - Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));
  /* Read unicode string */
  uniqueID = fread_unicode_string (&bread, &bwritten, 1, ibm_pc_format,
                                   input, error);
  /* uniqueID seems to always be empty at the root */

  if (! uniqueID)
    {
      g_warning ("uniqueID is NULL! (or an error occurred)");
      psd_set_error (error);
      return -1;
    }

  IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));
  classID_string = load_key (input, error);
  /* root class ID seems to always be 'null' */
  IFDBG(3) g_debug ("Unique ID: %s, Class ID: %s", uniqueID, classID_string);
  g_free (uniqueID);

  /* Initialize our json objects used to store the collected information. */
  if (base_node == NULL || *base_node == NULL)
    {
      local = json_node_new (JSON_NODE_OBJECT);
      obj = json_object_new ();
      json_node_init_object (local, obj);
      if (base_node)
        *base_node = local;
    }
  else
    {
      root = *base_node;
      obj = json_node_get_object (root);
    }

  json_object_set_string_member (obj, "classID", classID_string);

  IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));
  if (psd_read (input, &num_items, 4, error) < 4)
    {
      psd_set_error (error);
      g_free (classID_string);
      return -1;
    }
  num_items = GUINT32_FROM_BE (num_items);
  IFDBG(3) g_debug ("Number of items: %u", num_items);
  json_object_set_int_member (obj, "count", num_items);

  arr = json_array_new ();
  /* FIXME: Add error more checking here and elsewhere! */

  json_object_set_array_member (obj, "descriptor", arr);

  for (i = 0; i < num_items; i++)
    {
      gchar    *key;
      gchar     type[4];
      gint      res;
      JsonNode *node;

      IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));

      key = load_key (input, error);
      if (! key)
        {
          g_free (classID_string);
          return -1;
        }
      else if (psd_read (input, &type, 4, error) < 4)
        {
          psd_set_error (error);
          g_free (classID_string);
          return -1;
        }
      IFDBG(3) g_debug ("Item: %d, key: %s type: %.4s", i, key, type);

      node = NULL;
      res = load_type (input, ibm_pc_format, classID_string, key, type, &node, error);
      if (node)
        {
          json_array_add_element (arr, node);
          /* For debugging: */
          /* g_printerr ("Node Json:\n%s\n", json_to_string (node, TRUE)); */
        }
      else
        g_debug ("WARNING: Missing node for key %s!", key);
      g_free (key);
      if (res < 0)
        return res;
    }

  if (root)
    {
      /* Print json for debugging... */
      /* g_printerr ("Json:\n%s\n", json_to_string (root, TRUE)); */
    }

  g_free (classID_string);

  return 0;
}

static gchar *
load_key (GInputStream  *input,
          GError       **error)
{
  guint32  len = 0;
  gchar   *key;

  if (psd_read (input, &len, 4, error) < 4)
    {
      psd_set_error (error);
      return NULL;
    }
  len = GUINT32_FROM_BE (len);

  if (len == 0)
    {
      /* pre-defined key is always 4 bytes */
      len = 4;
    }

  /* We can't use fread_pascal_string since that expects a 1-byte length
     instead of 4-bytes. */
  key = g_malloc0 (len+1);
  if (! key)
    return NULL;

  if (psd_read (input, key, len, error) < len)
    {
      psd_set_error (error);
      g_free (key);
      return NULL;
    }
  key [len] = '\0';

  return key;
}

static JsonNode*
add_descriptor_float (const gchar  *key,
                      const gchar  *type,
                      const gchar  *float_type,
                      gfloat        value)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  /* These values are not NULL terminated */
  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);
  tmp = g_strdup_printf ("%.4s", float_type);
  json_object_set_string_member (obj, "float_name", tmp);
  g_free (tmp);

  json_object_set_double_member (obj, "value", value);

  return local;
}

static JsonNode*
add_descriptor_double (const gchar  *key,
                       const gchar  *type,
                       gdouble       value)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  /* These values are not NULL terminated */
  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  json_object_set_double_member (obj, "value", value);

  return local;
}

static JsonNode*
add_descriptor_bool (const gchar  *key,
                     const gchar  *type,
                     gboolean      value)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  json_object_set_boolean_member (obj, "value", value);

  return local;
}

static JsonNode*
add_descriptor_comp (const gchar  *key,
                     const gchar  *type,
                     gint64        value)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  json_object_set_int_member (obj, "value", value);

  return local;
}

static JsonNode*
add_descriptor_long (const gchar  *key,
                     const gchar  *type,
                     gint32        value)
{
  return add_descriptor_comp (key, type, value);
}

static JsonNode*
add_descriptor_enum (const gchar  *key,
                     const gchar  *type,
                     const gchar  *name,
                     const gchar  *value)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  json_object_set_string_member (obj, "enum_name",   name);

  json_object_set_string_member (obj, "value", value);

  return local;
}

static JsonNode*
add_descriptor_text (const gchar  *key,
                     const gchar  *type,
                     const gchar  *name,
                     const gchar  *value)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  json_object_set_string_member (obj, "text_name", name);

  json_object_set_string_member (obj, "value", value);

  return local;
}

static JsonNode*
add_descriptor_objc (const gchar  *key,
                     const gchar  *type)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  return local;
}

static JsonNode*
add_descriptor_vlls (const gchar  *key,
                     const gchar  *type)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  return local;
}

static JsonNode*
add_descriptor_float_list (const gchar  *key,
                           const gchar  *type,
                           const gchar  *float_type)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  return local;
}

static gint
load_type (GInputStream  *input,
           gboolean       ibm_pc_format,
           const gchar   *class_id,
           const gchar   *key,
           const gchar   *type,
           JsonNode	    **node,
           GError       **error)
{
  /* g_printerr ("Loading type %.4s for key %s\n", type, key); */

  if (memcmp (type, "obj ", 4) == 0)
    {
      /* Reference structure*/
      IFDBG(3) g_debug ("ob 'reference structure'");
      g_message ("FIXME Type: %.4s", type);
    }
  else if (memcmp (type, "Objc", 4) == 0)
    {
      /* Descriptor structure*/
      gint      res;
      JsonNode *desc_node = NULL;

      IFDBG(3) g_debug ("Objc Descriptor begin");
      desc_node = add_descriptor_objc (key, type);
      res = load_descriptor (input, ibm_pc_format, &desc_node, error);
      *node = desc_node;
      IFDBG(3) g_debug ("Objc Descriptor end");
      if (res < 0)
        return res;
    }
  else if (memcmp (type, "VlLs", 4) == 0)
    {
      /* List */
      gint32       list_items = 0;
      gint32       li;
      JsonArray   *arr;

      if (psd_read (input, &list_items, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      list_items = GUINT32_FROM_BE (list_items);
      IFDBG(3) g_debug ("Number of items in list: %i", list_items);

      *node = add_descriptor_vlls (key, type);
      arr = json_array_new ();
      json_object_set_array_member (json_node_get_object (*node), "list", arr);

      for (li = 0; li < list_items; li++)
        {
          gchar     type[4];
          gint      res;
          JsonNode *list_node;

          if (psd_read (input, &type, 4, error) < 4)
            {
              psd_set_error (error);
              return -1;
            }
          IFDBG(3) g_debug ("Item: %d, type: %.4s", li, type);

          list_node = NULL;
          res = load_type (input, ibm_pc_format, class_id, key, type, &list_node, error);
          if (res < 0)
            return res;

          if (list_node)
            json_array_add_element (arr, list_node);
          else
            g_printerr ("Failed to add array element!\n");
        }
    }
  else if (memcmp (type, "doub", 4) == 0)
    {
      gdouble data;

      if (! psd_read_double (input, &data, error))
        {
          psd_set_error (error);
          return -1;
        }

      IFDBG(3) g_debug ("double value: %f", data);
      *node = add_descriptor_double (key, type, data);
    }
  else if (memcmp (type, "UntF", 4) == 0)
    {
      /* Unit float */
      gchar   floatkey[4] = "";
      gdouble data;

      if (psd_read (input, &floatkey, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
        if (! psd_read_double (input, &data, error))
          {
            psd_set_error (error);
            return -1;
          }

          IFDBG(3) g_debug ("Float: %.4s, value: %f", floatkey, data);
      *node = add_descriptor_float (key, type, floatkey, data);
    }
  else if (memcmp (type, "UnFl ", 4) == 0)
    {
      /* Undocumented: Unit Floats */
      gchar      floatkey[4] = "";
      guint32    count       = 0;
      gint       i;
      JsonArray *arr;

      if (psd_read (input, &floatkey, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      if (psd_read (input, &count, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      count = GUINT32_FROM_BE (count);
      IFDBG(3) g_debug ("Array[%u] of float of type: %.4s", count, floatkey);

      *node = add_descriptor_float_list (key, type, floatkey);
      arr = json_array_new ();
      json_object_set_array_member (json_node_get_object (*node), "list", arr);

      for (i = 0; i < count; i++)
        {
          gdouble   data;
          JsonNode *list_node;

          if (! psd_read_double (input, &data, error))
            {
              psd_set_error (error);
              return -1;
            }
          IFDBG(3) g_debug ("[%i] - value: %f", i, data);

          list_node = add_descriptor_double (key, "doub", data);
          if (list_node)
            json_array_add_element (arr, list_node);
          else
            g_printerr ("Failed to add array element!\n");
        }
    }
  else if (memcmp (type, "TEXT", 4) == 0)
    {
      /* String */
      gchar  *str;
      gint32  bread, bwritten;

      str = fread_unicode_string (&bread, &bwritten, 1, ibm_pc_format,
                                  input, error);
      if (! str)
        {
          psd_set_error (error);
          return -1;
        }
      IFDBG(3) g_debug ("string value: %s (class id: '%s', key: '%s')", str, class_id, key);
      *node = add_descriptor_text (key, type, class_id, str);

      g_free (str);
    }
  else if (memcmp (type, "enum", 4) == 0)
    {
      /* Enumerated descriptor */
      gchar  *nameID, *valueID;

      nameID  = load_key (input, error);
      valueID = load_key (input, error);
      /* FIXME: Error handling */

      IFDBG(3) g_debug ("Enum name: %s, value: %s",
                        nameID, valueID);
      *node = add_descriptor_enum (key, type, nameID, valueID);

      g_free (nameID);
      g_free (valueID);
    }
  else if (memcmp (type, "long", 4) == 0)
    {
      /* Integer */
      gint32 psdlong = 0;

      if (psd_read (input, &psdlong, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      psdlong = GUINT32_FROM_BE (psdlong);
      IFDBG(3) g_debug ("long value: %d", psdlong);
      *node = add_descriptor_long (key, type, psdlong);
    }
  else if (memcmp (type, "comp", 4) == 0)
    {
      /* Large Integer */
      gint64 psdlarge = 0;

      if (psd_read (input, &psdlarge, 8, error) < 8)
        {
          psd_set_error (error);
          return -1;
        }
      psdlarge = GUINT32_FROM_BE (psdlarge);
      IFDBG(3) g_debug ("large value: %" G_GINT64_FORMAT, psdlarge);
      *node = add_descriptor_comp (key, type, psdlarge);
    }
  else if (memcmp (type, "bool", 4) == 0)
    {
      /* Boolean */
      gboolean val = FALSE;

      if (psd_read (input, &val, 1, error) < 1)
        {
          psd_set_error (error);
          return -1;
        }
      IFDBG(3) g_debug ("Boolean value: %u", (gboolean) val);
      *node = add_descriptor_bool (key, type, val);
    }
  else if (memcmp (type, "GlbO", 4) == 0)
    {
      /* GlobalObject same as Descriptor */
      IFDBG(3) g_debug ("GlbO GlobalObject same as Descriptor");
      g_message ("FIXME Type: %.4s", type);
    }
  else if (memcmp (type, "type", 4) == 0)
    {
      /* Class */
      IFDBG(3) g_debug ("type (Class)");
      g_message ("FIXME Type: %.4s", type);
    }
  else if (memcmp (type, "GlbC", 4) == 0)
    {
      /* Class */
      IFDBG(3) g_debug ("GlbC (global? Class)");
      g_message ("FIXME Type: %.4s", type);
    }
  else if (memcmp (type, "alis", 4) == 0)
    {
      /* Alias */
      guint32  size = 0;
      goffset  ofs;

      /* Used in placedLayer.psd in 'lnkE' resource */
      /* According to the specs:
       * FSSpec data structure for Macintosh, or a handle to a string to the
       * full path on Windows.
       * At this time it doesn't look like we will need this data.
       */
      IFDBG(3) g_debug ("alis (Alias)");

      if (psd_read (input, &size, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      size = GUINT32_FROM_BE (size);
      ofs  = PSD_TELL(input) + size;

      IFDBG(3) g_debug ("Size of alis: %u, ending offset: %" G_GOFFSET_FORMAT, size, ofs);

      if (! psd_seek (input, ofs, G_SEEK_SET, error))
        {
          psd_set_error (error);
          return -1;
        }
    }
  else if (memcmp (type, "tdta", 4) == 0)
    {
      /* Raw Data */
      guint32  size = 0;
      goffset  ofs;
      gchar   *buf = NULL;

      g_message ("FIXME Type: %.4s - missing json conversion", type);

      if (psd_read (input, &size, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      size = GUINT32_FROM_BE (size);
      ofs  = PSD_TELL(input) + size;

      IFDBG(3) g_debug ("Size of tdta: %u, ending offset: %" G_GOFFSET_FORMAT, size, ofs);
      buf = g_malloc (size);
      if (psd_read (input, buf, size, error) < size)
        {
          g_printerr ("Didn't read enough bytes!\n");
          psd_set_error (error);
          return -1;
        }
      parse_text_info (size, buf, error);
      g_free (buf);
      if (! psd_seek (input, ofs, G_SEEK_SET, error))
        {
          psd_set_error (error);
          return -1;
        }
    }
  else if (memcmp (type, "ObAr", 4) == 0)
    {
      /* Undocumented: Object Array */
      gint32    res;
      guint32   desc_ver  = 0;
      JsonNode *desc_node = NULL;

      if (psd_read (input, &desc_ver, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      desc_ver = GUINT32_FROM_BE (desc_ver);

      IFDBG(4) g_debug ("Descriptor version: %u", desc_ver);

      /* Not sure what the difference is with Objc, except the extra
       * descriptor version. It doesn't seem like there is an array of
       * descriptors.
       */
      IFDBG(3) g_debug ("Descriptor array begin");
      desc_node = add_descriptor_objc (key, type);
      res = load_descriptor (input, ibm_pc_format, &desc_node, error);
      *node = desc_node;
      IFDBG(3) g_debug ("Descriptor array end");
      if (res < 0)
        return res;
    }
  else if (memcmp (type, "Pth ", 4) == 0)
    {
      /* Undocumented: Path */
      IFDBG(3) g_debug ("Pth (Path, undocumented)");
      g_message ("FIXME Type: %.4s", type);
    }
  else
    {
      /* Unknown/undocumented Photoshop descriptor type */
      IFDBG(3) g_debug ("Unknown or undocumented type %s", type);
      g_warning ("Unknown or undocumented type %s", type);
      g_message ("FIXME Type: %.4s", type);
      return -2;
    }

  return 0;
}
