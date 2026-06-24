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

typedef  struct {
    gint        lvl;
    gint        ar_lvl;
    guint32     bufpos;
    guint32     cmdpos;
    guint32     val_len;
    guint32     val_pos;
    guint32     value_start_pos;
    gchar      *last_cmd;
    gboolean    reading_command;
    gboolean    reading_value;
    gboolean    reading_binary;
  } ParserData;

/*  Local function prototypes  */
static gint       load_resource_unknown (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

/*  Single layer resources */
static gint       load_resource_ladj    (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_lpla    (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_lfil    (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_lfx     (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_ltyp    (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_luni    (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_lyid    (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_lclr    (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_lsct    (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_lrfx    (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_lyvr    (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_lnsr    (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_cinf    (const PSDlayerres     *res_a,
                                         PSDlayer              *lyr_a,
                                         GInputStream          *input,
                                         GError               **error);

/*  Image resources */
static gint       load_resource_llnk    (const PSDlayerres     *res_a,
                                         PSDimage              *img_a,
                                         GInputStream          *input,
                                         GError               **error);

static gint       load_resource_ltxt    (const PSDlayerres     *res_a,
                                         PSDimage              *img_a,
                                         GInputStream          *input,
                                         GError               **error);

/*  Other functions */
static JsonNode * add_descriptor_float  (const gchar           *key,
                                         const gchar           *type,
                                         const gchar           *float_type,
                                         gfloat                 value);

static JsonNode * add_descriptor_double (const gchar           *key,
                                         const gchar           *type,
                                         gdouble                value);

static JsonNode * add_descriptor_bool   (const gchar           *key,
                                         const gchar           *type,
                                         gboolean               value);

static JsonNode * add_descriptor_comp   (const gchar           *key,
                                         const gchar           *type,
                                         gint64                 value);

static JsonNode * add_descriptor_long   (const gchar           *key,
                                         const gchar           *type,
                                         gint32                 value);

static JsonNode * add_descriptor_enum   (const gchar           *key,
                                         const gchar           *type,
                                         const gchar           *name,
                                         const gchar           *value);

static JsonNode * add_descriptor_text   (const gchar           *key,
                                         const gchar           *type,
                                         const gchar           *name,
                                         const gchar           *value);

static JsonNode * add_descriptor_objc   (const gchar           *key,
                                         const gchar           *type);

static JsonNode * add_descriptor_vlls   (const gchar           *key,
                                         const gchar           *type);

static JsonNode * add_descriptor_float_list
                                        (const gchar           *key,
                                         const gchar           *type,
                                         const gchar           *float_type);

static gint     parse_tdta              (guint32                len,
                                         gchar                 *buf,
                                         JsonNode             **tdta_root,
                                         GError               **error);

static gint     parse_text_info         (guint32                len,
                                         gchar                 *buf,
                                         JsonNode             **tdta_root,
                                         GError               **error);

static gint     parse_text_loop         (guint32                len,
                                         gchar                 *buf,
                                         JsonNode             **node,
                                         ParserData            *data,
                                         GError               **error);

static gint     parse_descriptor        (GInputStream          *input,
                                         gboolean               ibm_pc_format,
                                         JsonNode             **root_node,
                                         GError               **error);

static gint     load_descriptor         (GInputStream          *input,
                                         gboolean               ibm_pc_format,
                                         JsonNode             **base_node,
                                         GError               **error);

static gchar  * load_key                (GInputStream          *input,
                                         GError               **error);

static gint     load_type               (GInputStream          *input,
                                         gboolean               ibm_pc_format,
                                         const gchar           *class_id,
                                         const gchar           *key,
                                         const gchar           *type,
                                         JsonNode	      **node,
                                         GError               **error);

static gint     get_link_type           (gchar                 *lnk_type);


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
  if (memcmp (res_a->key, PSD_LADJ_CURVE, 4) == 0       ||
      memcmp (res_a->key, PSD_LADJ_BLACK_WHITE, 4) == 0 ||
      memcmp (res_a->key, PSD_LADJ_SELECTIVE, 4) == 0   ||
      memcmp (res_a->key, PSD_LADJ_GRAD_MAP, 4) == 0    ||
      memcmp (res_a->key, PSD_LADJ_PHOTO_FILT, 4) == 0  ||
      memcmp (res_a->key, PSD_LADJ_EXPOSURE, 4) == 0)
    {
      if (lyr_a)
        {
          lyr_a->unsupported_features->adjustment_layer = TRUE;
          lyr_a->unsupported_features->show_gui         = TRUE;
        }

      load_resource_ladj (res_a, lyr_a, input, error);
    }
  /* TODO: Implement all adjustment layers */
  else if (memcmp (res_a->key, PSD_LADJ_LEVEL, 4) == 0      ||
           memcmp (res_a->key, PSD_LADJ_BRIGHTNESS, 4) == 0 ||
           memcmp (res_a->key, PSD_LADJ_BALANCE, 4) == 0    ||
           memcmp (res_a->key, PSD_LADJ_HUE, 4) == 0        ||
           memcmp (res_a->key, PSD_LADJ_HUE2, 4) == 0       ||
           memcmp (res_a->key, PSD_LADJ_MIXER, 4) == 0      ||
           memcmp (res_a->key, PSD_LADJ_INVERT, 4) == 0     ||
           memcmp (res_a->key, PSD_LADJ_POSTERIZE, 4) == 0  ||
           memcmp (res_a->key, PSD_LADJ_THRESHOLD, 4) == 0)
    {
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

  /* Level Adjustment Layer */
  if (memcmp (res_a->key, PSD_LADJ_LEVEL, 4) == 0)
    {
      guchar  in_out[4][8];
      guint16 gamma[4];
      guchar  remainder[10 * 25];

      memcpy (lyr_a->adjustment_layer->type, PSD_LADJ_LEVEL, 4);

      if (! psd_read_int16 (input, &lyr_a->adjustment_layer->version,
                            res_a->ibm_pc_format, error)                    ||
          psd_read (input, in_out[0], 8, error) < 8                         ||
          ! psd_read_uint16 (input, &gamma[0], res_a->ibm_pc_format, error) ||
          psd_read (input, in_out[1], 8, error) < 8                         ||
          ! psd_read_uint16 (input, &gamma[1], res_a->ibm_pc_format, error) ||
          psd_read (input, in_out[2], 8, error) < 8                         ||
          ! psd_read_uint16 (input, &gamma[2], res_a->ibm_pc_format, error) ||
          psd_read (input, in_out[3], 8, error) < 8                         ||
          ! psd_read_uint16 (input, &gamma[3], res_a->ibm_pc_format, error) ||
          psd_read (input, remainder, 250, error) < 250)
        {
          psd_set_error (error);
          return -1;
        }

      for (gint i = 0; i < 4; i++)
        {
          for (gint j = 0; j < 8; j += 2)
            lyr_a->adjustment_layer->in_out_gamma[i][j / 2] =
              in_out[i][j + 1];

          lyr_a->adjustment_layer->in_out_gamma[i][4] = gamma[i];
        }
    }
  else if (memcmp (res_a->key, PSD_LADJ_BRIGHTNESS, 4) == 0)
    {
      guint16 brightness = 0;
      guint16 contrast   = 0;
      gint32  unused     = 0;

      memcpy (lyr_a->adjustment_layer->type, PSD_LADJ_BRIGHTNESS, 4);

      if (! psd_read_int16 (input, &lyr_a->adjustment_layer->version,
                            res_a->ibm_pc_format, error)                      ||
          ! psd_read_uint16 (input, &brightness, res_a->ibm_pc_format, error) ||
          ! psd_read_uint16 (input, &contrast,   res_a->ibm_pc_format, error) ||
          ! psd_read_int32  (input, &unused,     res_a->ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }

      lyr_a->adjustment_layer->brightness = (guchar) brightness;
      lyr_a->adjustment_layer->contrast   = (guchar) contrast;
    }
  else if (memcmp (res_a->key, PSD_LADJ_BALANCE, 4) == 0)
    {
      gint16  data[9];
      guint16 preserve_luminosity = 0;

      memcpy (lyr_a->adjustment_layer->type, PSD_LADJ_BALANCE, 4);

      if (psd_read (input, data, 18, error) < 18 ||
          ! psd_read_uint16 (input, &preserve_luminosity, res_a->ibm_pc_format,
                             error))
        {
          psd_set_error (error);
          return -1;
        }

      for (gint i = 0; i < 3; i++)
        {
          lyr_a->adjustment_layer->shadows[i] =
            (gchar) (res_a->ibm_pc_format ?
                     GINT16_FROM_LE (data[i]) :
                     GINT16_FROM_BE (data[i]));

          lyr_a->adjustment_layer->midtones[i] =
            (gchar) (res_a->ibm_pc_format ?
                     GINT16_FROM_LE (data[i + 3]) :
                     GINT16_FROM_BE (data[i + 3]));

          lyr_a->adjustment_layer->highlights[i] =
            (gchar) (res_a->ibm_pc_format ?
                     GINT16_FROM_LE (data[i + 6]) :
                     GINT16_FROM_BE (data[i + 6]));
        }

      lyr_a->adjustment_layer->preserve_luminosity = (preserve_luminosity != 0);
    }
  else if (memcmp (res_a->key, PSD_LADJ_HUE, 4) == 0 ||
           memcmp (res_a->key, PSD_LADJ_HUE2, 4) == 0)
    {
      guchar padding = 0;
      gint16 hsl[3];
      gint16 data[45];

      memcpy (lyr_a->adjustment_layer->type, res_a->key, 4);

      if (! psd_read_int16 (input, &lyr_a->adjustment_layer->version,
                            res_a->ibm_pc_format, error)                        ||
          psd_read (input, &lyr_a->adjustment_layer->is_colorize, 1, error) < 1 ||
          psd_read (input, &padding, 1, error) < 1                              ||
          psd_read (input, hsl, 6, error) < 6                                   ||
          psd_read (input, data, 90, error) < 90)
        {
          psd_set_error (error);
          return -1;
        }

      for (gint i = 0; i < 3; i++)
        {
          lyr_a->adjustment_layer->colorization[i] =
            (gfloat) (res_a->ibm_pc_format ?
                      GINT16_FROM_LE (hsl[i]) :
                      GINT16_FROM_BE (hsl[i]));

          lyr_a->adjustment_layer->hsl[0][i] =
            (gfloat) (res_a->ibm_pc_format ?
                      GINT16_FROM_LE (data[i]) :
                      GINT16_FROM_BE (data[i]));
        }

      for (gint i = 1; i < 7; i++)
        {
          /* Each value is stored as 4 range values (for CMYK), followed by
           * 3 LAB value settings. Since we just use HSL, we need to offset
           * by 4 each time to get the correct next values */
          gint base = ((i - 1) * 3) + ((i - 1) * 4) + 7;

          for (gint j = 0; j < 3; j++)
            lyr_a->adjustment_layer->hsl[i][j] =
              (gchar) (res_a->ibm_pc_format ?
                       GINT16_FROM_LE (data[base + j]) :
                       GINT16_FROM_BE (data[base + j]));
        }

      /* The difference between old and new Hue-Saturation settings
       * is the denominator of the HSL values */
      if (memcmp (res_a->key, PSD_LADJ_HUE, 4) == 0)
        {
          for (gint i = 0; i < 3; i++)
            {
              lyr_a->adjustment_layer->colorization[i] /= 100;
            }
          lyr_a->adjustment_layer->colorization[2] =
            (lyr_a->adjustment_layer->colorization[2] + 1) / 2;
        }
      else
        {
          /* GIMP's Colorize hue & saturation ranges from 0.0 to 1.0, so we
           * need to offset the imported values from PS */
          lyr_a->adjustment_layer->colorization[0] /= 180.0;
          lyr_a->adjustment_layer->colorization[1] /= 100.0;
          lyr_a->adjustment_layer->colorization[2] /= 100.0;
          for (gint i = 0; i < 2; i++)
            {
              lyr_a->adjustment_layer->colorization[i] += 1.0;
              lyr_a->adjustment_layer->colorization[i] /= 2.0;
            }
        }

      for (gint i = 0; i < 7; i++)
        {
          lyr_a->adjustment_layer->hsl[i][0] /= 180.0;
          lyr_a->adjustment_layer->hsl[i][1] /= 100.0;
          lyr_a->adjustment_layer->hsl[i][2] /= 100.0;
        }
    }
  else if (memcmp (res_a->key, PSD_LADJ_MIXER, 4) == 0)
    {
      guint16 mono = 0;

      memcpy (lyr_a->adjustment_layer->type, PSD_LADJ_MIXER, 4);

      if (! psd_read_int16 (input, &lyr_a->adjustment_layer->version,
                            res_a->ibm_pc_format, error)                      ||
          ! psd_read_uint16 (input, &mono, res_a->ibm_pc_format, error)       ||
          psd_read (input, &lyr_a->adjustment_layer->red, 10, error) < 10     ||
          psd_read (input, &lyr_a->adjustment_layer->green, 10, error) < 10   ||
          psd_read (input, &lyr_a->adjustment_layer->blue, 10, error) < 10    ||
          psd_read (input, &lyr_a->adjustment_layer->total, 10, error) < 10)
        {
          psd_set_error (error);
          return -1;
        }

      lyr_a->adjustment_layer->is_mono = (mono != 0);

      for (gint i = 0; i < 5; i++)
        {
          lyr_a->adjustment_layer->red[i] =
            res_a->ibm_pc_format ?
              GINT16_FROM_LE (lyr_a->adjustment_layer->red[i]) :
              GINT16_FROM_BE (lyr_a->adjustment_layer->red[i]);

          lyr_a->adjustment_layer->green[i] =
            res_a->ibm_pc_format ?
              GINT16_FROM_LE (lyr_a->adjustment_layer->green[i]) :
              GINT16_FROM_BE (lyr_a->adjustment_layer->green[i]);

          lyr_a->adjustment_layer->blue[i] =
            res_a->ibm_pc_format ?
              GINT16_FROM_LE (lyr_a->adjustment_layer->blue[i]) :
              GINT16_FROM_BE (lyr_a->adjustment_layer->blue[i]);

          lyr_a->adjustment_layer->total[i] =
            res_a->ibm_pc_format ?
              GINT16_FROM_LE (lyr_a->adjustment_layer->total[i]) :
              GINT16_FROM_BE (lyr_a->adjustment_layer->total[i]);
        }

    }
  else if (memcmp (res_a->key, PSD_LADJ_INVERT, 4) == 0)
    {
      memcpy (lyr_a->adjustment_layer->type, PSD_LADJ_INVERT, 4);
    }
  else if (memcmp (res_a->key, PSD_LADJ_POSTERIZE, 4) == 0)
    {
      gint16 levels = 0;
      gint16 spacer = 0;

      memcpy (lyr_a->adjustment_layer->type, PSD_LADJ_POSTERIZE, 4);

      if (! psd_read_int16 (input, &levels, res_a->ibm_pc_format, error) ||
          ! psd_read_int16 (input, &spacer, res_a->ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }
      lyr_a->adjustment_layer->level = levels;
    }
  else if (memcmp (res_a->key, PSD_LADJ_THRESHOLD, 4) == 0)
    {
      gint16 levels = 0;
      gint16 spacer = 0;

      memcpy (lyr_a->adjustment_layer->type, PSD_LADJ_THRESHOLD, 4);

      if (! psd_read_int16 (input, &levels, res_a->ibm_pc_format, error) ||
          ! psd_read_int16 (input, &spacer, res_a->ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }
      lyr_a->adjustment_layer->level = levels;
    }

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
      ! psd_read_uint32 (input, &version, res_a->ibm_pc_format, error))
    {
      psd_set_error (error);
      return -1;
    }

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

      if (! psd_read_uint32 (input, &page_num,          res_a->ibm_pc_format, error) ||
          ! psd_read_uint32 (input, &total_pages,       res_a->ibm_pc_format, error) ||
          ! psd_read_uint32 (input, &anti_alias_policy, res_a->ibm_pc_format, error) ||
          ! psd_read_uint32 (input, &placed_layer_type, res_a->ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }

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

      if (! psd_read_uint32 (input, &descriptor_version, res_a->ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }

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
load_resource_llnk (const PSDlayerres     *res_a,
                    PSDimage              *img_a,
                    GInputStream          *input,
                    GError               **error)
{
  guint64        data_size = 0;
  goffset        link_data_offset, next_block_offset;
  guint16        next_block_size = 0;
  gchar          lnk_type[4]; /* 'liFD' linked file data, 'liFE' linked file external or 'liFA' linked file alias */
  guint32        ver = 0, desc_ver = 0;
  gchar          file_creator[4];
  guchar         file_descriptor_present = 0;
  gint32         bread, bwritten;
  gint           li = 0;
  gint           res;
  PSDLinkedData *link_data = NULL;

  IFDBG(2) g_debug ("Process layer resource block %.4s: linked layer data.",
                    res_a->key);

  link_data_offset = PSD_TELL (input);
  if (! psd_read_uint64 (input, &data_size, res_a->ibm_pc_format, error))
    {
      psd_set_error (error);
      return -1;
    }

  next_block_offset = link_data_offset + (goffset) data_size + 8;

  li = 0;
  while (TRUE)
    {
      IFDBG(3) g_debug ("Linked data[%d], Offset: %" G_GOFFSET_FORMAT, li, link_data_offset);

      if (! psd_read_chars (input, lnk_type, 4, res_a->ibm_pc_format, error) ||
          ! psd_read_uint32 (input, &ver, res_a->ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }

      IFDBG(3) g_debug ("Next data block offset: %" G_GUINT64_FORMAT,
                        (guint64) (link_data_offset + (goffset) data_size + 8));

      link_data = g_new0 (PSDLinkedData, 1);
      if (img_a->linked_files.linked_data == NULL)
        {
          img_a->linked_files.linked_data = link_data;
          img_a->linked_files.last        = link_data;
        }
      else
        {
          img_a->linked_files.last->next = link_data;
          img_a->linked_files.last       = link_data;
        }

      link_data->link_type = get_link_type (lnk_type);

      IFDBG(3) g_debug ("Data size: %" G_GUINT64_FORMAT ", link type: %.4s, version %u",
                        (guint64) data_size, lnk_type, ver);

      link_data->link_id = fread_pascal_string (&bread, &bwritten, 1, input, error);
      if (! link_data->link_id)
        return -1;

      IFDBG(3) g_debug ("Unique file ID: %s", link_data->link_id);

      link_data->original_filename = fread_unicode_string (&bread, &bwritten, 1,
                                                           res_a->ibm_pc_format,
                                                           input, error);
      if (! link_data->original_filename)
        return -1;

      IFDBG(3) g_debug ("Original filename: %s", link_data->original_filename);

      if (! psd_read_chars  (input, link_data->file_type, 4,
                             res_a->ibm_pc_format, error) ||
          ! psd_read_chars  (input, file_creator, 4, res_a->ibm_pc_format,
                             error) ||
          ! psd_read_uint64 (input, (guint64 *) &link_data->data_size,
                             res_a->ibm_pc_format, error) ||
          psd_read (input, &file_descriptor_present, 1, error) < 1)
        {
          psd_set_error (error);
          return -1;
        }

      IFDBG(3) g_debug ("File type: %.4s, creator: %.4s, data length: %" G_GUINT64_FORMAT
                        ", file descriptor present: %u",
                        link_data->file_type, file_creator,
                        (guint64) link_data->data_size,
                        (guchar) file_descriptor_present);

      if (file_descriptor_present)
        {
          IFDBG(3) g_debug ("Offset descriptor: %" G_GOFFSET_FORMAT, PSD_TELL (input));

          if (! psd_read_uint32 (input, &desc_ver, res_a->ibm_pc_format, error))
            {
              psd_set_error (error);
              return -1;
            }

          IFDBG(4) g_debug ("Descriptor version: %u", desc_ver);

          /* Doesn't look like this descriptor has anything we need. */
          res = load_descriptor (input, res_a->ibm_pc_format, NULL, error);
          if (res < 0)
            return -1;
        }

      if (link_data->link_type == LINK_TYPE_EXTERNAL)
        {
          gint32  external_desc_ver = 0;
          guint64 filesize          = 0;

          if (! psd_read_int32 (input, &external_desc_ver, res_a->ibm_pc_format, error))
            {
              psd_set_error (error);
              return -1;
            }

          IFDBG(4) g_debug ("External link descriptor version: %d", external_desc_ver);

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

              if (! psd_read_uint32 (input, &year, res_a->ibm_pc_format, error) ||
                  psd_read (input, &month, 1, error) < 1 ||
                  psd_read (input, &day, 1, error) < 1 ||
                  psd_read (input, &hour, 1, error) < 1 ||
                  psd_read (input, &minute, 1, error) < 1 ||
                  ! psd_read_double (input, &seconds, res_a->ibm_pc_format, error))
                {
                  psd_set_error (error);
                  return -1;
                }

              IFDBG(3) g_debug ("File date/time: %u-%u-%u %u:%u:%f",
                                year, month, day, hour, minute, seconds);
            }

          if (! psd_read_uint64 (input, &filesize, res_a->ibm_pc_format, error))
            {
              psd_set_error (error);
              return -1;
            }

          IFDBG(3) g_debug ("Filesize: %" G_GUINT64_FORMAT, (guint64) filesize);

          /* Specs say actual file data follows here, but that is incorrect. */
        }
      else if (link_data->link_type == LINK_TYPE_FILE_DATA)
        {
          /* Actual image data is included here, e.g. a complete PNG including headers etc. */

          link_data->data_offset = PSD_TELL (input) + link_data->data_size;
          IFDBG(3) g_debug ("Offset of included file data: %" G_GOFFSET_FORMAT
                            ", end offset: %" G_GOFFSET_FORMAT,
                            PSD_TELL (input), link_data->data_offset);

          /* FIXME Skip actual image data for now... */
          if (! psd_seek (input, link_data->data_offset, G_SEEK_SET, error))
            {
              psd_set_error (error);
              return -1;
            }
        }
      else if (link_data->link_type == LINK_TYPE_ALIAS)
        {
          guint64 dummy = 0;

          /* FIXME We don't have an example of this yet. No idea how to interpret. */
          IFDBG(3) g_debug ("Skipping alias link\n");

          /* 8 zeros according to specs */
          if (! psd_read_uint64 (input, &dummy, res_a->ibm_pc_format, error))
            {
              psd_set_error (error);
              return -1;
            }
        }

      IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL (input));

      if (ver >= 5)
        {
          gchar *utemp = NULL;

          utemp = fread_unicode_string (&bread, &bwritten, 1,
                                        res_a->ibm_pc_format,
                                        input, error);
          if (! utemp)
            return -1;

          IFDBG(3) g_debug ("Child Document ID: %s", utemp);
          g_free (utemp);
        }

      if (ver >= 6)
        {
          gdouble mod_time;

          if (! psd_read_double (input, &mod_time, res_a->ibm_pc_format, error))
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
      IFDBG(3) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL (input));

      link_data_offset = PSD_TELL (input);
      if (link_data->link_type == LINK_TYPE_EXTERNAL)
        {
          if (! psd_read_uint64 (input, &data_size, res_a->ibm_pc_format, error))
            {
              psd_set_error (error);
              return -1;
            }

          IFDBG(3) g_debug ("Next block data size: %" G_GUINT64_FORMAT, (guint64) data_size);
        }

      if (! psd_read_uint16 (input, &next_block_size, res_a->ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }

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

      if (! psd_read_uint32 (input, &oe_version,   res_a->ibm_pc_format, error) ||
          ! psd_read_uint32 (input, &desc_version, res_a->ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }

      IFDBG(3) g_debug ("Objects based effects layer info: object effects version: %u, descriptor version: %u",
                        oe_version, desc_version);

      if (parse_descriptor (input, res_a->ibm_pc_format, &root, error) == 0)
        {
          lyr_a->layer_effects.effects = root;

          IFDBG(4)
            if (root)
              g_debug ("Layer Effects descriptor for layer %u:\n%s",
                       lyr_a->id, json_to_string (root, TRUE));
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
  gint16            warp_version      = 0;
  gint32            warp_desc_version = 0;
  gint32            dimensions[4];
  gdouble           d_transform[6];
  gint              res;
  JsonNode         *descriptor        = NULL;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Type tool layer", res_a->key);

  /* New style type tool layers (ps6) */
  if (memcmp (res_a->key, PSD_LTYP_TYPE2, 4) == 0)
    {
      lyr_a->text.info = NULL;

      if (! psd_read_int16   (input, &version,        res_a->ibm_pc_format, error) ||
          ! psd_read_double  (input, &d_transform[0], res_a->ibm_pc_format, error) ||
          ! psd_read_double  (input, &d_transform[1], res_a->ibm_pc_format, error) ||
          ! psd_read_double  (input, &d_transform[2], res_a->ibm_pc_format, error) ||
          ! psd_read_double  (input, &d_transform[3], res_a->ibm_pc_format, error) ||
          ! psd_read_double  (input, &d_transform[4], res_a->ibm_pc_format, error) ||
          ! psd_read_double  (input, &d_transform[5], res_a->ibm_pc_format, error) ||
          ! psd_read_int16   (input, &text_desc_vers, res_a->ibm_pc_format, error) ||
          ! psd_read_int32   (input, &desc_version,   res_a->ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
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

      res = load_descriptor (input, res_a->ibm_pc_format, &descriptor, error);
      if (res < 0)
        return -1;
      else
        lyr_a->text.textdata = descriptor;

      if (! psd_read_int16 (input, &warp_version, res_a->ibm_pc_format, error) ||
          ! psd_read_int32 (input, &warp_desc_version, res_a->ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }

      IFDBG(2) g_debug ("Warp version: %d, descriptor version: %d",
                        warp_version, warp_desc_version);

      descriptor = NULL;
      res = load_descriptor (input, res_a->ibm_pc_format, &descriptor, error);
      if (res < 0)
        return -1;
      else
        lyr_a->text.warpdata = descriptor;

      IFDBG(3) g_debug ("Offset after warp descriptor: %" G_GOFFSET_FORMAT, PSD_TELL(input));

      if (! psd_read_int32 (input, &dimensions[0], res_a->ibm_pc_format, error) ||
          ! psd_read_int32 (input, &dimensions[1], res_a->ibm_pc_format, error) ||
          ! psd_read_int32 (input, &dimensions[2], res_a->ibm_pc_format, error) ||
          ! psd_read_int32 (input, &dimensions[3], res_a->ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }

      IFDBG(2) g_debug ("Left: %d, Top: %d, Right: %d, Bottom: %d",
                        dimensions[0], dimensions[1],
                        dimensions[2], dimensions[3]);
      IFDBG(3) g_debug ("End offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));
    }

  return 0;
}

static gint
load_resource_ltxt (const PSDlayerres  *res_a,
                    PSDimage           *img_a,
                    GInputStream       *input,
                    GError            **error)
{
  guint32   res_len;
  gchar    *buf;
  gint      res  = 0;
  JsonNode *root = NULL;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Type tool layer", res_a->key);

  res_len = (guint32) res_a->data_len;

  IFDBG(3) g_debug ("Txt2 resource length: %u at offset %" G_GOFFSET_FORMAT,
                    res_len, PSD_TELL (input));

  buf = g_malloc (res_len);
  if (psd_read (input, buf, res_len, error) < res_len)
    {
      psd_set_error (error);
      return -1;
    }

  IFDBG(3) g_debug ("Parse text info at offset: %" G_GOFFSET_FORMAT, PSD_TELL (input));

  /* TXT2 payload is a byte stream (not a sequence of fixed-width integers),
   * so we read it raw; parse_tdta() interprets the contents. */
  res = parse_tdta (res_len, buf, &root, error);
  if (res < 0)
    g_warning ("Error reading TXT2 data.");
  else
    img_a->global_text_data = root;

  g_free (buf);

  return 0;
}

static gint
load_resource_luni (const PSDlayerres  *res_a,
                    PSDlayer           *lyr_a,
                    GInputStream       *input,
                    GError            **error)
{
  /* Load layer name in unicode (length padded to multiple of 4 bytes) */
  gint32 read_len;
  gint32 write_len;

  IFDBG(2) g_debug ("Process layer resource block luni: Unicode Name");

  if (lyr_a->name)
    g_free (lyr_a->name);

  lyr_a->name = fread_unicode_string (&read_len, &write_len, 4,
                                      res_a->ibm_pc_format, input, error);

  if (error && *error)
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

  if (! psd_read_uint32 (input, &lyr_a->id, res_a->ibm_pc_format, error))
    {
      psd_set_error (error);
      return -1;
    }

  IFDBG(3) g_debug ("Layer id: %u", lyr_a->id);

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

  if (! psd_read_uint16 (input, &lyr_a->color_tag[0], res_a->ibm_pc_format, error) ||
      ! psd_read_uint16 (input, &lyr_a->color_tag[1], res_a->ibm_pc_format, error) ||
      ! psd_read_uint16 (input, &lyr_a->color_tag[2], res_a->ibm_pc_format, error) ||
      ! psd_read_uint16 (input, &lyr_a->color_tag[3], res_a->ibm_pc_format, error))
    {
      psd_set_error (error);
      return -1;
    }

  /* Photoshop only uses color_tag[0] to store a color code */
  IFDBG(3) g_debug ("Layer sheet color: %u", lyr_a->color_tag[0]);

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

  if (! psd_read_uint32 (input, &type, res_a->ibm_pc_format, error))
    {
      psd_set_error (error);
      return -1;
    }

  IFDBG(3) g_debug ("Section divider type: %u", type);

  lyr_a->group_type = type;

  if (res_a->data_len >= 12)
    {
      gchar signature[4];
      gchar blend_mode[4];

      if (! psd_read_chars (input, signature, 4, res_a->ibm_pc_format, error) ||
          ! psd_read_chars (input, blend_mode, 4, res_a->ibm_pc_format, error))
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
  PSDLayerStyles  *ls_a;
  gchar            signature[4];
  gchar            effectname[5];
  gint             i;

  ls_a = lyr_a->layer_styles;

  IFDBG(2) g_debug ("Process layer resource block %.4s: Layer effects", res_a->key);

  if (! psd_read_uint16 (input, &ls_a->version, res_a->ibm_pc_format, error) ||
      ! psd_read_uint16 (input, &ls_a->count,   res_a->ibm_pc_format, error))
    {
      psd_set_error (error);
      return -1;
    }

  IFDBG(3) g_debug ("Version %u, count %u", ls_a->version, ls_a->count);

  for (i = 0; i < ls_a->count; i++)
    {
      if (! psd_read_chars (input, signature,  4, res_a->ibm_pc_format, error) ||
          ! psd_read_chars (input, effectname, 4, res_a->ibm_pc_format, error))
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
              if (! psd_read_uint32 (input, &ls_a->cmns.size,    res_a->ibm_pc_format, error) ||
                  ! psd_read_uint32 (input, &ls_a->cmns.ver,     res_a->ibm_pc_format, error) ||
                  psd_read (input, &ls_a->cmns.visible, 1, error) < 1                         ||
                  ! psd_read_uint16 (input, &ls_a->cmns.unused,  res_a->ibm_pc_format, error))
                {
                  psd_set_error (error);
                  return -1;
                }

              IFDBG(3) g_debug ("cmnS (common state info) - size %u, version %u, visible: %u",
                                ls_a->cmns.size, ls_a->cmns.ver, ls_a->cmns.visible);
            }
          else if (memcmp (effectname, "dsdw", 4) == 0 ||
                   memcmp (effectname, "isdw", 4) == 0)
            {
              PSDLayerStyleShadow *shadow;
              gchar                bim[4];

              if (memcmp (effectname, "dsdw", 4) == 0)
                shadow = &ls_a->dsdw;
              else
                shadow = &ls_a->isdw;

              if (! psd_read_uint32       (input, &shadow->size,      res_a->ibm_pc_format, error)  ||
                  ! psd_read_uint32       (input, &shadow->ver,       res_a->ibm_pc_format, error)  ||
                  ! psd_read_fixed_float  (input, &shadow->blur,      res_a->ibm_pc_format, error)  ||
                  ! psd_read_fixed_float  (input, &shadow->intensity, res_a->ibm_pc_format, error)  ||
                  ! psd_read_fixed_float  (input, &shadow->angle,     res_a->ibm_pc_format, error)  ||
                  ! psd_read_fixed_float  (input, &shadow->distance,  res_a->ibm_pc_format, error)  ||
                  ! psd_read_uint16       (input, &shadow->color[0],  res_a->ibm_pc_format, error)  ||
                  ! psd_read_uint16       (input, &shadow->color[1],  res_a->ibm_pc_format, error)  ||
                  ! psd_read_uint16       (input, &shadow->color[2],  res_a->ibm_pc_format, error)  ||
                  ! psd_read_uint16       (input, &shadow->color[3],  res_a->ibm_pc_format, error)  ||
                  ! psd_read_uint16       (input, &shadow->color[4],  res_a->ibm_pc_format, error)  ||
                  ! psd_read_chars        (input, bim,              4, res_a->ibm_pc_format, error) ||
                  ! psd_read_chars        (input, shadow->blendsig, 4, res_a->ibm_pc_format, error) ||
                  psd_read (input, &shadow->effecton, 1, error) < 1 ||
                  psd_read (input, &shadow->anglefx,  1, error) < 1 ||
                  psd_read (input, &shadow->opacity,  1, error) < 1)
                {
                  psd_set_error (error);
                  return -1;
                }

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
                  if (! psd_read_uint16 (input, &shadow->natcolor[0], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &shadow->natcolor[1], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &shadow->natcolor[2], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &shadow->natcolor[3], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &shadow->natcolor[4], res_a->ibm_pc_format, error))
                    {
                      psd_set_error (error);
                      return -1;
                    }
                }
            }
          else if (memcmp (effectname, "oglw", 4) == 0)
            {
              gchar bim[4];

              if (! psd_read_uint32      (input, &ls_a->oglw.size,      res_a->ibm_pc_format, error) ||
                  ! psd_read_uint32      (input, &ls_a->oglw.ver,       res_a->ibm_pc_format, error) ||
                  ! psd_read_fixed_float (input, &ls_a->oglw.blur,      res_a->ibm_pc_format, error) ||
                  ! psd_read_fixed_float (input, &ls_a->oglw.intensity, res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16      (input, &ls_a->oglw.color[0],  res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16      (input, &ls_a->oglw.color[1],  res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16      (input, &ls_a->oglw.color[2],  res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16      (input, &ls_a->oglw.color[3],  res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16      (input, &ls_a->oglw.color[4],  res_a->ibm_pc_format, error) ||
                  ! psd_read_chars       (input, bim,                  4, res_a->ibm_pc_format, error) ||
                  ! psd_read_chars       (input, ls_a->oglw.blendsig,   4, res_a->ibm_pc_format, error) ||
                  psd_read (input, &ls_a->oglw.effecton, 1, error) < 1 ||
                  psd_read (input, &ls_a->oglw.opacity,  1, error) < 1)
                {
                  psd_set_error (error);
                  return -1;
                }

              IFDBG(3) g_debug ("oglw - size %u, version %u, effect enabled: %u, "
                                "blur (pixels): %f, intensity (pct): %f, "
                                "blendsig: %.4s, effect %.4s",
                                ls_a->oglw.size, ls_a->oglw.ver,
                                ls_a->oglw.effecton,
                                ls_a->oglw.blur, ls_a->oglw.intensity,
                                ls_a->oglw.blendsig, (gchar *) &ls_a->oglw.effect);

              if (ls_a->oglw.size == 42)
                {
                  if (! psd_read_uint16 (input, &ls_a->oglw.natcolor[0], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->oglw.natcolor[1], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->oglw.natcolor[2], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->oglw.natcolor[3], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->oglw.natcolor[4], res_a->ibm_pc_format, error))
                    {
                      psd_set_error (error);
                      return -1;
                    }
                }
            }
          else if (memcmp (effectname, "iglw", 4) == 0)
            {
              if (! psd_read_uint32      (input, &ls_a->iglw.size,    res_a->ibm_pc_format, error) ||
                  ! psd_read_uint32      (input, &ls_a->iglw.ver,     res_a->ibm_pc_format, error) ||
                  ! psd_read_fixed_float (input, &ls_a->iglw.blur,    res_a->ibm_pc_format, error) ||
                  ! psd_read_fixed_float (input, &ls_a->iglw.intensity, res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16      (input, &ls_a->iglw.color[0], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16      (input, &ls_a->iglw.color[1], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16      (input, &ls_a->iglw.color[2], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16      (input, &ls_a->iglw.color[3], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16      (input, &ls_a->iglw.color[4], res_a->ibm_pc_format, error) ||
                  ! psd_read_chars       (input, ls_a->iglw.blendsig,  4, res_a->ibm_pc_format, error) ||
                  ! psd_read_chars       (input, (gchar *) &ls_a->iglw.effect, 4, res_a->ibm_pc_format, error) ||
                  psd_read (input, &ls_a->iglw.effecton, 1, error) < 1 ||
                  psd_read (input, &ls_a->iglw.opacity,  1, error) < 1)
                {
                  psd_set_error (error);
                  return -1;
                }

              IFDBG(3) g_debug ("iglw - size %u, version %u, effect enabled: %u, blur (pixels): %f, intensity (pct): %f, blendsig: %.4s, effect %.4s",
                                ls_a->iglw.size, ls_a->iglw.ver,
                                ls_a->iglw.effecton,
                                ls_a->iglw.blur, ls_a->iglw.intensity,
                                ls_a->iglw.blendsig, (gchar *) &ls_a->iglw.effect);

              if (ls_a->iglw.size == 43)
                {
                  if (psd_read (input, &ls_a->iglw.invert, 1, error) < 1 ||
                      ! psd_read_uint16 (input, &ls_a->iglw.natcolor[0], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->iglw.natcolor[1], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->iglw.natcolor[2], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->iglw.natcolor[3], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->iglw.natcolor[4], res_a->ibm_pc_format, error))
                    {
                      psd_set_error (error);
                      return -1;
                    }
                }
            }
          else if (memcmp (effectname, "bevl", 4) == 0)
            {
              if (! psd_read_uint32 (input, &ls_a->bevl.size, res_a->ibm_pc_format, error) ||
                  ! psd_read_uint32 (input, &ls_a->bevl.ver, res_a->ibm_pc_format, error) ||
                  ! psd_read_int32  (input, &ls_a->bevl.angle, res_a->ibm_pc_format, error) ||
                  ! psd_read_uint32 (input, &ls_a->bevl.strength, res_a->ibm_pc_format, error) ||
                  ! psd_read_uint32 (input, &ls_a->bevl.blur, res_a->ibm_pc_format, error) ||
                  ! psd_read_chars  (input, (gchar *) &ls_a->bevl.highlightsig, 4, res_a->ibm_pc_format, error) ||
                  ! psd_read_chars  (input, (gchar *) &ls_a->bevl.highlighteffect, 4, res_a->ibm_pc_format, error) ||
                  ! psd_read_chars  (input, (gchar *) &ls_a->bevl.shadowsig, 4, res_a->ibm_pc_format, error) ||
                  ! psd_read_chars  (input, (gchar *) &ls_a->bevl.shadoweffect, 4, res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->bevl.highlightcolor[0], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->bevl.highlightcolor[1], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->bevl.highlightcolor[2], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->bevl.highlightcolor[3], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->bevl.highlightcolor[4], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->bevl.shadowcolor[0], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->bevl.shadowcolor[1], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->bevl.shadowcolor[2], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->bevl.shadowcolor[3], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->bevl.shadowcolor[4], res_a->ibm_pc_format, error) ||
                  psd_read (input, &ls_a->bevl.style, 1, error) < 1 ||
                  psd_read (input, &ls_a->bevl.highlightopacity, 1, error) < 1 ||
                  psd_read (input, &ls_a->bevl.shadowopacity, 1, error) < 1 ||
                  psd_read (input, &ls_a->bevl.enabled, 1, error) < 1 ||
                  psd_read (input, &ls_a->bevl.global, 1, error) < 1 ||
                  psd_read (input, &ls_a->bevl.direction, 1, error) < 1)
                {
                  psd_set_error (error);
                  return -1;
                }

              IFDBG(3) g_debug ("bevl - size %u, version %u, effect enabled: %u, highlightsig: %.4s, highlighteffect %.4s, shadowsig: %.4s, shadoweffect %.4s",
                                ls_a->bevl.size, ls_a->bevl.ver,
                                ls_a->bevl.enabled,
                                (gchar *) &ls_a->bevl.highlightsig, (gchar *) &ls_a->bevl.highlighteffect,
                                (gchar *) &ls_a->bevl.shadowsig, (gchar *) &ls_a->bevl.shadoweffect);

              if (ls_a->bevl.size == 78)
                {
                  if (! psd_read_uint16 (input, &ls_a->bevl.highlightnatcolor[0], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->bevl.highlightnatcolor[1], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->bevl.highlightnatcolor[2], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->bevl.highlightnatcolor[3], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->bevl.highlightnatcolor[4], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->bevl.shadownatcolor[0], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->bevl.shadownatcolor[1], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->bevl.shadownatcolor[2], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->bevl.shadownatcolor[3], res_a->ibm_pc_format, error) ||
                      ! psd_read_uint16 (input, &ls_a->bevl.shadownatcolor[4], res_a->ibm_pc_format, error))
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

              if (! psd_read_uint32 (input, &ls_a->sofi.size, res_a->ibm_pc_format, error) ||
                  ! psd_read_uint32 (input, &ls_a->sofi.ver,  res_a->ibm_pc_format, error) ||
                  ! psd_read_chars  (input, blendsig,         4, res_a->ibm_pc_format, error) ||
                  ! psd_read_chars  (input, ls_a->sofi.blend, 4, res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->sofi.color[0], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->sofi.color[1], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->sofi.color[2], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->sofi.color[3], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->sofi.color[4], res_a->ibm_pc_format, error) ||
                  psd_read (input, &ls_a->sofi.opacity, 1, error) < 1 ||
                  psd_read (input, &ls_a->sofi.enabled, 1, error) < 1 ||
                  ! psd_read_uint16 (input, &ls_a->sofi.natcolor[0], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->sofi.natcolor[1], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->sofi.natcolor[2], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->sofi.natcolor[3], res_a->ibm_pc_format, error) ||
                  ! psd_read_uint16 (input, &ls_a->sofi.natcolor[4], res_a->ibm_pc_format, error))
                {
                  psd_set_error (error);
                  return -1;
                }

              IFDBG(3) g_debug ("sofi - size %u, version %u, enabled %u, blendsig: %.4s, blend %.4s, opacity %u",
                                ls_a->sofi.size, ls_a->sofi.ver,
                                ls_a->sofi.enabled,
                                blendsig, ls_a->sofi.blend,
                                100 * ls_a->sofi.opacity / 255);
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

  if (! psd_read_int32 (input, &version, res_a->ibm_pc_format, error))
    {
      psd_set_error (error);
      return -1;
    }

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

  if (! psd_read_chars (input, layername, 4, res_a->ibm_pc_format, error))
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

  if (! psd_read_uint32 (input, &version, res_a->ibm_pc_format, error))
    {
      psd_set_error (error);
      return -1;
    }

  IFDBG(3) g_debug ("Descriptor version (expecting 16): %u", version);

  if (load_descriptor (input, res_a->ibm_pc_format, NULL, error) < 0)
    return -1;

  return 0;
}

static gint
parse_tdta (guint32    len,
            gchar     *buf,
            JsonNode **tdta_root,
            GError   **error)
{
  JsonObject *obj      = NULL;
  JsonObject *desc_obj = NULL;
  JsonNode   *root     = NULL;
  JsonNode   *desc     = NULL;
  gint        res;

  /* Initialize our json objects used to store the collected information. */
  if (tdta_root == NULL || *tdta_root == NULL)
    {
      JsonNode *local = json_node_new (JSON_NODE_OBJECT);

      obj = json_object_new ();
      json_node_init_object (local, obj);
      if (tdta_root)
        *tdta_root = local;
    }
  else
    {
      root = *tdta_root;
      obj = json_node_get_object (root);
    }

  desc_obj = json_object_new ();
  desc     = json_node_new (JSON_NODE_OBJECT);
  json_node_init_object (desc, desc_obj);
  json_object_set_object_member (obj, "descriptor", desc_obj);

  res = parse_text_info (len, buf, &desc, error);
  if (res < 0)
    json_node_free (desc);

  return res;
}

static gint
parse_text_info (guint32    len,
                 gchar     *buf,
                 JsonNode **tdta_root,
                 GError   **error)
{
  ParserData  data     = {};
  JsonObject *obj      = NULL;
  JsonObject *desc_obj = NULL;
  JsonNode   *root     = NULL;
  JsonNode   *desc     = NULL;

  if (tdta_root == NULL || *tdta_root == NULL)
    {
      g_critical (_("Invalid root node!"));
      return -1;
    }
  root = *tdta_root;

  obj  = json_node_get_object (root);
  json_object_set_string_member (obj, "classID", "textdata");

  desc_obj = json_object_new ();
  desc     = json_node_new (JSON_NODE_OBJECT);
  json_node_init_object (desc, desc_obj);
  json_object_set_object_member (obj, "descriptor", desc_obj);

  IFDBG(3) g_debug ("Parsing text info...");

  parse_text_loop (len, buf, &desc, &data, error);

  g_free (data.last_cmd);

  IFDBG(4)
    if (root)
      g_debug ("Text data Json:\n%s\n", json_to_string (root, TRUE));

  return data.bufpos;
}

static gint
parse_text_loop (guint32      len,
                 gchar       *buf,
                 JsonNode   **node,
                 ParserData  *data,
                 GError     **error)
{
  JsonObject *desc_obj  = NULL;
  JsonArray  *arr       = NULL;
  JsonNode   *arr_node  = NULL;

  JsonNode   *member_node = NULL;
  JsonObject *member_obj  = NULL;

  gboolean    in_array    = FALSE;

  if (! JSON_NODE_HOLDS_OBJECT (*node))
    {
      g_critical ("Object node expected!");
      return -1;
    }

  desc_obj  = json_node_get_object (*node);

#define COMMAND_SIZE 128 /* No idea what the real Photoshop maximum is. */

  while (data->bufpos < len)
    {
      gchar token;
      gchar command[COMMAND_SIZE+1];
      gchar simple_value[COMMAND_SIZE+1];

      token = buf[data->bufpos];

      if (data->reading_binary)
        {
          gchar     nexttoken = ' ';
          gboolean  endbinary = FALSE;

          /* reading 16-bit values unless token == ) */
          if (token == ')' && data->bufpos + 1 < len)
            {
              nexttoken = buf[data->bufpos+1];
              if (nexttoken == ' '  ||
                  nexttoken == 0x09 ||
                  nexttoken == 0x0a)
                endbinary = TRUE;
            }

          if (! endbinary)
            {
              data->bufpos++;
              continue;
            }
        }

      switch (token)
        {
        case '(': /* start of binary value */
          data->reading_binary = TRUE;
          data->value_start_pos = data->bufpos + 1;
          IFDBG(4) g_debug ("Binary value start");
          break;

        case ')': /* end of binary value */
          {
            gchar  *name_utf16;
            gchar  *name_utf8;
            GError *error = NULL;
            gsize   bw, br;

            data->reading_binary = FALSE;
            data->val_len = data->bufpos - data->value_start_pos;

            /* Get the name as UTF-8 from UTF-16 data. */
            name_utf16 = g_malloc0 (data->val_len + 2);
            memcpy (name_utf16, &buf[data->value_start_pos+2], data->val_len-2);
            name_utf8 = g_convert (name_utf16, data->val_len, "utf-8", "utf-16BE", &br, &bw, &error);

            if (error)
              {
                g_warning ("Error converting utf-16 to utf-8: %s", error->message);
                g_clear_error (&error);
              }
            IFDBG(3) g_debug ("Text value: '%s'", name_utf8);

            json_object_set_string_member (member_obj, "type", "string");
            json_object_set_string_member (member_obj, "value", name_utf8);

            g_free (name_utf16);
            g_free (name_utf8);

            IFDBG(4) g_debug ("Binary value end, value length: %u bytes", data->val_len);
          }
          break;

        case '/': /* start of command */
          data->reading_command = TRUE;
          data->cmdpos = 0;
          break;

        case 'a'...'z':
        case 'A'...'Z':
        case '0'...'9':
        case '-':
        case '.':
          if (data->reading_command)
            {
              if (token == '-' || token == '.')
                {
                  IFDBG(3) g_debug ("Unexpected token %c in command!", token);
                }
              else if (data->cmdpos < COMMAND_SIZE)
                {
                  command[data->cmdpos++] = token;
                }
              else
                {
                  g_critical ("Text command token too long!");
                }
            }
          else if (! data->reading_binary)
            {
              if (! data->reading_value)
                {
                  data->reading_value = TRUE;
                  data->val_pos = 0;
                }
              if (data->val_pos < COMMAND_SIZE)
                {
                  simple_value[data->val_pos++] = token;
                }
              else
                {
                  g_critical ("Text value token too long!");
                }
            }
          break;

        case ' ':
        case 0x09: /* TAB character */
        case 0x0a: /* newline character */
          if (data->reading_command)
            {
              command[data->cmdpos] = '\0';
              data->cmdpos = 0;
              data->reading_command = FALSE;
              g_free (data->last_cmd);
              data->last_cmd = g_strdup ((const gchar *) &command);

              member_node = json_node_new (JSON_NODE_OBJECT);
              member_obj  = json_object_new ();
              json_node_init_object (member_node, member_obj);
              json_object_set_object_member (desc_obj, command, member_obj);

              IFDBG(3) g_debug ("Command: %s at level %d", command, data->lvl);

              json_object_set_string_member (member_obj, "key", command);
              IFDBG(4)
                json_object_set_int_member (member_obj, "level", data->lvl);
            }
          else if (data->reading_value)
            {
              simple_value[data->val_pos] = '\0';
              data->val_pos = 0;
              data->reading_value = FALSE;
              IFDBG(3) g_debug ("Value: %s", simple_value);

              if (! in_array)
                {
                  if (member_obj == NULL)
                    g_critical ("Cannot add value: member object is NULL!");

                  json_object_set_string_member (member_obj, "type", "string");
                  json_object_set_string_member (member_obj, "value", simple_value);
                }
              else
                {
                  json_array_add_string_element (arr, simple_value);
                }
            }
          break;

        case '<': /* Increase level */
          if (data->bufpos + 1 < len && buf[data->bufpos+1] == '<')
            {
              JsonNode   *new_node   = NULL;
              JsonObject *new_obj    = NULL;
              JsonObject *parent_obj = NULL;

              if (member_obj)
                {
                  parent_obj = desc_obj;
                  desc_obj   = member_obj;
                  member_obj = NULL;
                }

              if (desc_obj == NULL)
                g_critical ("Increasing level: descriptor object is NULL!");

              json_object_set_string_member (desc_obj, "type", "Objc");

              new_obj  = json_object_new ();
              new_node = json_node_new (JSON_NODE_OBJECT);
              json_node_init_object (new_node, new_obj);

              if (! in_array)
                json_object_set_object_member (desc_obj, "descriptor", new_obj);
              else
                json_array_add_object_element (arr, new_obj);

              data->lvl++;
              data->bufpos++;
              if (data->bufpos < len)
                {
                  data->bufpos++;

                  parse_text_loop (len, buf, &new_node, data, error);
                }
              if (parent_obj)
                desc_obj = parent_obj;
            }
          break;

        case '>': /* Decrease level */
          if (data->bufpos + 1 < len && buf[data->bufpos+1] == '>')
            {
              data->lvl--;
              data->bufpos++;

              return data->bufpos;
            }
          break;

        case '[': /* begin array */
          data->ar_lvl++;
          IFDBG(3) g_debug ("Array start, level %d", data->ar_lvl);
          json_object_set_string_member (member_obj, "type", "array");

          arr      = json_array_new ();
          arr_node = json_node_new (JSON_NODE_ARRAY);
          json_node_init_array (arr_node, arr);
          json_object_set_array_member (member_obj, "values", arr);
          in_array = TRUE;
          break;

        case ']': /* end array */
          data->ar_lvl--;
          IFDBG(3) g_debug ("Array end, level %d", data->ar_lvl);
          in_array = FALSE;
          break;

        case (gchar) 0xfe: /* feff is UTF-16 BOM */
          if (data->reading_command)
            {
              if (data->bufpos + 1 < len && buf[data->bufpos+1] == (gchar) 0xff)
                {
                  data->bufpos++;
                  IFDBG(3) g_debug ("Value is using UTF-16");
                }
            }
          break;

        default:
          if (data->reading_command)
            {
              command[data->cmdpos] = '\0';
              data->cmdpos = 0;
              g_free (data->last_cmd);
              data->last_cmd = g_strdup ((const gchar *) &command);
              data->reading_command = FALSE;
              IFDBG(2) g_debug ("--> Error: unexpected token! Command: %s at level %d", command, data->lvl);
              g_warning ("Unexpected token. Command: %s", command);
            }
          else if (data->reading_value)
            {
              simple_value[data->val_pos] = '\0';
              data->val_pos = 0;
              data->reading_value = FALSE;
              IFDBG(2) g_debug ("--> Error: unexpected token! Value: %s", simple_value);
              g_warning ("Unexpected token value.");
            }
          break;
        }
      data->bufpos++;
    }

#undef COMMAND_SIZE

  /* Check here for conditions that could indicate a broken image or
   * an issue with our parser. */
  if (in_array)
    g_warning ("Unclosed array: loading may be incorrect!");
  if (data->reading_command)
    g_warning ("Incomplete command: loading may be incorrect!");
  if (data->reading_value)
    g_warning ("Incomplete value: loading may be incorrect!");

  return data->bufpos;
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

  IFDBG(4) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));
  classID_string = load_key (input, error);
  /* root class ID seems to always be 'null' */
  IFDBG(3) g_debug ("Unique ID: %s, Class ID: %s", uniqueID, classID_string);
  g_free (uniqueID);

  /* Initialize our json objects used to store the collected information. */
  if (base_node == NULL || *base_node == NULL)
    {
      JsonNode *local = NULL;

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

  IFDBG(4) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));
  if (psd_read (input, &num_items, 4, error) < 4)
    {
      psd_set_error (error);
      g_free (classID_string);
      return -1;
    }
  num_items = (ibm_pc_format ? GUINT32_FROM_LE (num_items) : GUINT32_FROM_BE (num_items));
  IFDBG(3) g_debug ("Number of items: %u", num_items);
  json_object_set_int_member (obj, "count", num_items);

  arr = json_array_new ();

  json_object_set_array_member (obj, "descriptor", arr);

  for (i = 0; i < num_items; i++)
    {
      gchar    *key;
      gchar     type[4];
      gint      res;
      JsonNode *node;

      IFDBG(4) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));

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
        json_array_add_element (arr, node);
      else
        g_warning ("Missing node for key %s!", key);

      g_free (key);
      if (res < 0)
        return res;
    }

  IFDBG(4)
    if (root)
      g_debug ("Text data Json:\n%s\n", json_to_string (root, TRUE));

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
  IFDBG(4) g_debug ("Loading type %.4s for key %s", type, key);

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
      list_items = (ibm_pc_format ? GUINT32_FROM_LE (list_items) : GUINT32_FROM_BE (list_items));
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
            g_debug ("Unable to add array element!");
        }
    }
  else if (memcmp (type, "doub", 4) == 0)
    {
      gdouble data;

      if (! psd_read_double (input, &data, ibm_pc_format, error))
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
        if (! psd_read_double (input, &data, ibm_pc_format, error))
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
      count = (ibm_pc_format ? GUINT32_FROM_LE (count) : GUINT32_FROM_BE (count));
      IFDBG(3) g_debug ("Array[%u] of float of type: %.4s", count, floatkey);

      *node = add_descriptor_float_list (key, type, floatkey);
      arr = json_array_new ();
      json_object_set_array_member (json_node_get_object (*node), "list", arr);

      for (i = 0; i < count; i++)
        {
          gdouble   data;
          JsonNode *list_node;

          if (! psd_read_double (input, &data, ibm_pc_format, error))
            {
              psd_set_error (error);
              return -1;
            }
          IFDBG(3) g_debug ("[%i] - value: %f", i, data);

          list_node = add_descriptor_double (key, "doub", data);
          if (list_node)
            json_array_add_element (arr, list_node);
          else
            g_debug ("Unable to add array element!");
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
      psdlong = (ibm_pc_format ? GUINT32_FROM_LE (psdlong) : GUINT32_FROM_BE (psdlong));
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
      psdlarge = (ibm_pc_format ? GUINT32_FROM_LE (psdlarge) : GUINT32_FROM_BE (psdlarge));
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
      size = (ibm_pc_format ? GUINT32_FROM_LE (size) : GUINT32_FROM_BE (size));
      ofs  = PSD_TELL(input) + size;

      IFDBG(4) g_debug ("Size of alis: %u, ending offset: %" G_GOFFSET_FORMAT, size, ofs);

      if (! psd_seek (input, ofs, G_SEEK_SET, error))
        {
          psd_set_error (error);
          return -1;
        }
    }
  else if (memcmp (type, "tdta", 4) == 0)
    {
      /* Raw Data */
      guint32   size      = 0;
      goffset   ofs;
      gint      res;
      gchar    *buf       = NULL;
      JsonNode *tdta_node = NULL;

      if (psd_read (input, &size, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      size = (ibm_pc_format ? GUINT32_FROM_LE (size) : GUINT32_FROM_BE (size));
      ofs  = PSD_TELL(input) + size;

      IFDBG(3) g_debug ("Size of tdta: %u, ending offset: %" G_GOFFSET_FORMAT, size, ofs);
      buf = g_malloc (size);
      if (psd_read (input, buf, size, error) < size)
        {
          g_warning (_("Could not read enough bytes!"));
          psd_set_error (error);
          return -1;
        }

      IFDBG(3) g_debug ("tdta begin");
      tdta_node = add_descriptor_objc (key, type);

      res = parse_tdta (size, buf, &tdta_node, error);
      if (res < 0)
        return -1;
      else
        *node = tdta_node;
      IFDBG(3) g_debug ("tdta end");

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

      if (! psd_read_uint32 (input, &desc_ver, ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }


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

static gint
get_link_type (gchar *lnk_type)
{
  if (memcmp (lnk_type, "liFD", 4) == 0)
    return LINK_TYPE_FILE_DATA;
  if (memcmp (lnk_type, "liFE", 4) == 0)
    return LINK_TYPE_EXTERNAL;
  if (memcmp (lnk_type, "liFA", 4) == 0)
    return LINK_TYPE_ALIAS;

  return -1;
}
