/*
 * DDS GIMP plugin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1301, USA.
 */

/* ImageMagick's implementation of BC7 decompression was referenced for our
 * implementation. The relevant commit:
 * https://github.com/ImageMagick/ImageMagick/pull/4126/files
 *
 * Rich Geldreich's BC7ENC-RDO BC7 compression library was referenced for our
 * implementation. The relevant commit:
 * https://github.com/richgel999/bc7enc_rdo/blob/fd057bf9fbad66733d6327330b05cee86ef61e23/bc7enc.cpp
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#include <libgimp/gimp.h>

#include "bc7.h"

#define SWAP(a, b)  do { typeof(a) t; t = a; a = b; b = t; } while(0)

static gboolean modes_initialized;

static gfloat g_mode1_rgba_midpoints[64][2];
static gfloat g_mode5_rgba_midpoints[128];
static gfloat g_mode7_rgba_midpoints[32][2];
static guchar g_mode6_reduced_quant[2048][2];

static const guchar g_bc7_color_index_bitcount[8] = { 3, 3, 2, 2, 2, 2, 4, 2 };
static const guchar g_bc7_alpha_index_bitcount[8] = { 0, 0, 0, 0, 3, 2, 4, 2 };

static endpoint_err g_bc7_mode_1_optimal_endpoints[256][2];
static endpoint_err g_bc7_mode_7_optimal_endpoints[256][2][2];

/* This table contains bitmasks indicating which "key" partitions must be best
 * ranked before this partition is worth evaluating.
 * We first rank the best/most used 14 partitions (sorted by usefulness),
 * (record the best one found as the key partition, then use that to control
 * the other partitions to evaluate. The quality loss is ~.08 dB RGB PSNR, the
 * perf gain is up to ~11% (at uber level 0). */
static const guint32 g_partition_predictors[35] =
{
  G_MAXUINT32, G_MAXUINT32, G_MAXUINT32, G_MAXUINT32, G_MAXUINT32,
  (1 << 1) | (1 << 2) | (1 << 8),
  (1 << 1) | (1 << 3) | (1 << 7),
  G_MAXUINT32, G_MAXUINT32,
  (1 << 2) | (1 << 8) | (1 << 16),
  (1 << 7) | (1 << 3) | (1 << 15),
  G_MAXUINT32,
  (1 << 8) | (1 << 14) | (1 << 16),
  (1 << 7) | (1 << 14) | (1 << 15),
  G_MAXUINT32, G_MAXUINT32, G_MAXUINT32, G_MAXUINT32,
  (1 << 14) | (1 << 15),
  (1 << 16) | (1 << 22) | (1 << 14),
  (1 << 17) | (1 << 24) | (1 << 14),
  (1 << 2) | (1 << 14) | (1 << 15) | (1 << 1),
  G_MAXUINT32,
  (1 << 1) | (1 << 3) | (1 << 14) | (1 << 16) | (1 << 22),
  G_MAXUINT32,
  (1 << 1) | (1 << 2) | (1 << 15) | (1 << 17) | (1 << 24),
  (1 << 1) | (1 << 3) | (1 << 22),
  G_MAXUINT32, G_MAXUINT32, G_MAXUINT32,
  (1 << 14) | (1 << 15) | (1 << 16) | (1 << 17),
  G_MAXUINT32, G_MAXUINT32,
  (1 << 1) | (1 << 2) | (1 << 3) | (1 << 27) | (1 << 4) | (1 << 24),
  (1 << 14) | (1 << 15) | (1 << 16) | (1 << 11) | (1 << 17) | (1 << 27)
};

typedef struct
{
  gfloat m_c[4];
} vec4F;

typedef struct
{
  guchar m_c[4];
} color_rgba;

typedef struct
{
  guint32           m_num_pixels;
  const color_rgba *m_pPixels;
  guint32           m_num_selector_weights;
  const guchar     *m_pSelector_weights;
  const vec4F      *m_pSelector_weightsx;
  guint32           m_comp_bits;
  guint32           m_weights[4];
  gboolean          m_has_alpha;
  gboolean          m_has_pbits;
  gboolean          m_endpoints_share_pbit;
  gboolean          m_perceptual;
} color_cell_compressor_params;

typedef struct
{
  guint64     m_best_overall_err;
  color_rgba  m_low_endpoint;
  color_rgba  m_high_endpoint;
  guint32     m_pbits[2];
  guchar     *m_pSelectors;
  guchar     *m_pSelectors_temp;
} color_cell_compressor_results;

typedef struct
{
  guint32    m_mode;
  guint32    m_partition;
  guchar     m_selectors[16];
  guchar     m_alpha_selectors[16];
  color_rgba m_low[3];
  color_rgba m_high[3];
  guint32    m_pbits[3][2];
  guint32    m_rotation;
  guint32    m_index_selector;
} bc7_optimization_results;

/* Decompression functions */
static guchar   get_bits               (const guchar  *block,
                                        guchar        *start_bit,
                                        guchar         length);

static void     decode_rgba_channels   (BC7_colors    *colors,
                                        guchar        *src,
                                        guint          mode,
                                        guchar        *start);

static gboolean is_pixel_anchor        (guchar         subset_index,
                                        guint          precision,
                                        guchar         pixel_index,
                                        guint          partition_id);

/* Compression functions */
static void     handle_alpha_block       (void                                *pBlock,
                                          const color_rgba                    *pPixels,
                                          const bc7_params                    *pComp_params,
                                          color_cell_compressor_params        *pParams);
static void     handle_alpha_block_mode5 (const color_rgba                    *pPixels,
                                          const bc7_params                    *pComp_params,
                                          color_cell_compressor_params        *pParams,
                                          guint32                              lo_a,
                                          guint32                              hi_a,
                                          bc7_optimization_results            *pOpt_results5,
                                          guint64                             *pMode5_err,
                                          guint64                             *pMode5_alpha_err);
static void     handle_opaque_block      (void                                *pBlock,
                                          const color_rgba                    *pPixels,
                                          const bc7_params                    *pComp_params,
                                          color_cell_compressor_params        *pParams);

static guint64  color_cell_compression   (guint32                              mode,
                                          const color_cell_compressor_params  *pParams,
                                          color_cell_compressor_results       *pResults,
                                          const bc7_params                    *pComp_params);

static guint64  pack_mode1_to_one_color  (const color_cell_compressor_params  *pParams,
                                          color_cell_compressor_results       *pResults,
                                          guint32                              r,
                                          guint32                              g,
                                          guint32                              b,
                                          guchar                              *pSelectors);
static guint64  pack_mode7_to_one_color  (const color_cell_compressor_params  *pParams,
                                          color_cell_compressor_results       *pResults,
                                          guint32                              r,
                                          guint32                              g,
                                          guint32                              b,
                                          guint32                              a,
                                          guchar                              *pSelectors,
                                          guint32                              num_pixels,
                                          const color_rgba                    *pPixels);

static void compute_least_squares_endpoints_rgb
                                        (guint32                              N,
                                         const guchar                        *pSelectors,
                                         const vec4F                          *pSelector_weights,
                                         vec4F                                *pXl,
                                         vec4F                                *pXh,
                                         const color_rgba                     *pColors);
static void compute_least_squares_endpoints_rgba
                                        (guint32                               N,
                                         const guchar                         *pSelectors,
                                         const vec4F                          *pSelector_weights,
                                         vec4F                                *pXl,
                                         vec4F                                *pXh,
                                         const color_rgba                     *pColors);
static void compute_least_squares_endpoints_a
                                        (guint32                               N,
                                         const guchar                         *pSelectors,
                                         const vec4F                          *pSelector_weights,
                                         gfloat                               *pXl,
                                         gfloat                               *pXh,
                                         const color_rgba                     *pColors);
static guint64  find_optimal_solution   (guint32                               mode,
                                         vec4F                                 xl,
                                         vec4F                                 xh,
                                         const color_cell_compressor_params   *pParams,
                                         color_cell_compressor_results        *pResults,
                                         const bc7_params                     *pComp_params);
static void     fixDegenerateEndpoints  (guint32                               mode,
                                         color_rgba                           *pTrialMinColor,
                                         color_rgba                           *pTrialMaxColor,
                                         const vec4F                          *pXl,
                                         const vec4F                          *pXh,
                                         guint32                               iscale,
                                         const bc7_params                     *pComp_params);
static guint64  evaluate_solution       (const color_rgba                     *pLow,
                                         const color_rgba                     *pHigh,
                                         const guint32                         pbits[2],
                                         const color_cell_compressor_params   *pParams,
                                         color_cell_compressor_results        *pResults,
                                         const bc7_params                     *pComp_params);

static guint32  estimate_partition      (const color_rgba                     *pPixels,
                                         const bc7_params                     *pComp_params,
                                         guint32                               pweights[4],
                                         guint32                               mode);

static guint64  color_cell_compression_est_mode1
                                        (guint32                               num_pixels,
                                         const color_rgba                     *pPixels,
                                         gboolean                              perceptual,
                                         guint32                               pweights[4],
                                         guint64                               best_err_so_far);
static guint64  color_cell_compression_est_mode7
                                        (guint32                               num_pixels,
                                         const color_rgba                     *pPixels,
                                         gboolean                              perceptual,
                                         guint32                               pweights[4],
                                         guint64                               best_err_so_far);

void            encode_bc7_block        (void                                 *pBlock,
                                         const bc7_optimization_results       *pResults);
static void     set_block_bits          (guchar                               *pBytes,
                                         guint32                               val,
                                         guint32                               num_bits,
                                         guint32                              *pCur_ofs);
static gboolean get_bc7_mode_has_seperate_alpha_selectors
                                        (gint                                  mode);

/* Inline functions */
static inline color_rgba *
color_quad_u8_set_clamped (color_rgba *pRes,
                           gint32      r,
                           gint32      g,
                           gint32      b,
                           gint32      a)
{
  pRes->m_c[0] = (guchar) CLAMP (r, 0, 255);
  pRes->m_c[1] = (guchar) CLAMP (g, 0, 255);
  pRes->m_c[2] = (guchar) CLAMP (b, 0, 255);
  pRes->m_c[3] = (guchar) CLAMP (a, 0, 255);

  return pRes;
}

static inline  color_rgba *
color_quad_u8_set (color_rgba *pRes,
                   gint32      r,
                   gint32      g,
                   gint32      b,
                   gint32      a)
{
  pRes->m_c[0] = (guchar) r;
  pRes->m_c[1] = (guchar) g;
  pRes->m_c[2] = (guchar) b;
  pRes->m_c[3] = (guchar) a;

 return pRes;
}

static inline gboolean
color_quad_u8_notequals (const color_rgba *pLHS,
                         const color_rgba *pRHS)
{
  return (pLHS->m_c[0] != pRHS->m_c[0]) ||
         (pLHS->m_c[1] != pRHS->m_c[1]) ||
         (pLHS->m_c[2] != pRHS->m_c[2]) ||
         (pLHS->m_c[3] != pRHS->m_c[3]);
}

static inline color_rgba
scale_color (const color_rgba                   *pC,
             const color_cell_compressor_params *pParams)
{
  const guint32 n = pParams->m_comp_bits + (pParams->m_has_pbits ? 1 : 0);
  color_rgba    results;

  for (gint i = 0; i < 4; i++)
    {
      guint32 v = pC->m_c[i] << (8 - n);

      v |= (v >> n);

      results.m_c[i] = (guchar) (v);
    }
  return results;
}

static inline guint64
compute_color_distance_rgb (const color_rgba *pE1,
                            const color_rgba *pE2,
                            gboolean          perceptual,
                            const guint32     weights[4])
{
  gint dr, dg, db;

  if (perceptual)
    {
      const gint l1  = pE1->m_c[0] * 109 + pE1->m_c[1] * 366 + pE1->m_c[2] * 37;
      const gint cr1 = ((gint) pE1->m_c[0] << 9) - l1;
      const gint cb1 = ((gint) pE1->m_c[2] << 9) - l1;
      const gint l2  = pE2->m_c[0] * 109 + pE2->m_c[1] * 366 + pE2->m_c[2] * 37;
      const gint cr2 = ((gint) pE2->m_c[0] << 9) - l2;
      const gint cb2 = ((gint) pE2->m_c[2] << 9) - l2;

      dr = (l1 - l2) >> 8;
      dg = (cr1 - cr2) >> 8;
      db = (cb1 - cb2) >> 8;
    }
  else
    {
      dr = (gint) pE1->m_c[0] - (gint) pE2->m_c[0];
      dg = (gint) pE1->m_c[1] - (gint) pE2->m_c[1];
      db = (gint) pE1->m_c[2] - (gint) pE2->m_c[2];
    }

  return weights[0] * (guint32) (dr * dr) +
         weights[1] * (guint32) (dg * dg) +
         weights[2] * (guint32) (db * db);
}

static inline guint64
compute_color_distance_rgba (const color_rgba *pE1,
                             const color_rgba *pE2,
                             gboolean          perceptual,
                             const guint32     weights[4])
{
  gint da = (gint) pE1->m_c[3] - (gint) pE2->m_c[3];

  return compute_color_distance_rgb (pE1, pE2, perceptual, weights) +
                                     (weights[3] * (guint64) (da * da));
}

static inline vec4F *
vec4F_set_scalar (vec4F *pV,
                  gfloat x)
{
  pV->m_c[0] = x;
  pV->m_c[1] = x;
  pV->m_c[2] = x;
  pV->m_c[3] = x;

  return pV;
}

static inline vec4F *
vec4F_set (vec4F *pV,
           gfloat x,
           gfloat y,
           gfloat z,
           gfloat w)
{
  pV->m_c[0] = x;
  pV->m_c[1] = y;
  pV->m_c[2] = z;
  pV->m_c[3] = w;

  return pV;
}

static inline vec4F *
vec4F_saturate_in_place (vec4F *pV)
{
  for (gint i = 0; i < 4; i++)
    pV->m_c[i] = CLAMP (pV->m_c[i], 0, 1.0f);

  return pV;
}

static inline vec4F
vec4F_from_color (const color_rgba *pC)
{
  vec4F res;

  vec4F_set (&res, pC->m_c[0], pC->m_c[1], pC->m_c[2], pC->m_c[3]);

  return res;
}

static inline vec4F
vec4F_saturate (const vec4F *pV)
{
  vec4F res;

  for (gint i = 0; i < 4; i++)
    res.m_c[i] = CLAMP (pV->m_c[i], 0, 1.0f);

  return res;
}

static inline vec4F
vec4F_add (const vec4F *pLHS,
           const vec4F *pRHS)
{
  vec4F res;

  vec4F_set (&res, pLHS->m_c[0] + pRHS->m_c[0], pLHS->m_c[1] + pRHS->m_c[1],
             pLHS->m_c[2] + pRHS->m_c[2], pLHS->m_c[3] + pRHS->m_c[3]);

  return res;
}

static inline vec4F
vec4F_sub (const vec4F *pLHS,
           const vec4F *pRHS)
{
  vec4F res;
  vec4F_set (&res, pLHS->m_c[0] - pRHS->m_c[0], pLHS->m_c[1] - pRHS->m_c[1],
             pLHS->m_c[2] - pRHS->m_c[2], pLHS->m_c[3] - pRHS->m_c[3]);

  return res;
}

static inline gfloat
vec4F_dot (const vec4F *pLHS,
           const vec4F *pRHS)
{
    return pLHS->m_c[0] * pRHS->m_c[0] + pLHS->m_c[1] * pRHS->m_c[1] +
           pLHS->m_c[2] * pRHS->m_c[2] + pLHS->m_c[3] * pRHS->m_c[3];
}

static inline vec4F
vec4F_mul (const vec4F *pLHS,
           gfloat       s)
{
  vec4F res;

  vec4F_set (&res, pLHS->m_c[0] * s, pLHS->m_c[1] * s, pLHS->m_c[2] * s,
             pLHS->m_c[3] * s);
  return res;
}

static inline vec4F *
vec4F_normalize_in_place (vec4F *pV)
{
  gfloat s = pV->m_c[0] * pV->m_c[0] + pV->m_c[1] * pV->m_c[1] +
             pV->m_c[2] * pV->m_c[2] + pV->m_c[3] * pV->m_c[3];

  if (s != 0.0f)
    {
       s = 1.0f / sqrtf(s);

       pV->m_c[0] *= s;
       pV->m_c[1] *= s;
       pV->m_c[2] *= s;
       pV->m_c[3] *= s;
    }

  return pV;
}

static inline gint32
iabs32 (gint32 v)
{
  guint32 msk = v >> 31;

  return (v ^ msk) - msk;
}

static inline void
swapu (guint32 *a,
       guint32 *b)
{
  guint32 t = *a;

  *a = *b;
  *b = t;
}

static gint
get_bc7_color_index_size (gint mode,
                          gint index_selection_bit)
{
  return g_bc7_color_index_bitcount[mode] + index_selection_bit;
}

static gint
get_bc7_alpha_index_size (gint mode,
                          gint index_selection_bit)
{
  return g_bc7_alpha_index_bitcount[mode] - index_selection_bit;
}

/* Public functions */
void
bc7_initialize (bc7_params *params,
                gboolean    is_perceptual)
{
  if (modes_initialized)
    return;

  /* Initialize parameters */
  params->m_mode_mask                              = G_MAXUINT32;
  params->m_max_partitions                         = 64;
  params->m_try_least_squares                      = TRUE;
  params->m_force_alpha                            = FALSE;
  params->m_mode17_partition_estimation_filterbank = FALSE;
  params->m_uber_level                             = 4;
  params->m_force_selectors                        = FALSE;
  params->m_quant_mode6_endpoints                  = TRUE;
  params->m_bias_mode1_pbits                       = TRUE;
  params->m_pbit1_weight                           = 1.0f;
  params->m_mode1_error_weight                     = 1.0f;
  params->m_mode5_error_weight                     = 1.0f;
  params->m_mode6_error_weight                     = 1.0f;
  params->m_mode7_error_weight                     = 1.0f;
  params->m_low_frequency_partition_weight         = 1.0f;
  params->m_perceptual                             = is_perceptual;
  if (is_perceptual)
    {
      params->m_weights[0] = 128;
      params->m_weights[1] = 64;
      params->m_weights[2] = 16;
      params->m_weights[3] = 32;
    }
  else
    {
      params->m_weights[0] = 1;
      params->m_weights[1] = 1;
      params->m_weights[2] = 1;
      params->m_weights[3] = 1;
    }

  /*  Mode 7 endpoint midpoints */
  for (guint32 p = 0; p < 2; p++)
    {
      for (guint32 i = 0; i < 32; i++)
        {
          guint32 vl = ((i << 1) | p) << 2;
          gfloat  lo;
          guint32 vh;
          gfloat  hi;

          vl |= (vl >> 6);
          lo = vl / 255.0f;

          vh = ((MIN (31, (i + 1)) << 1) | p) << 2;
          vh |= (vh >> 6);
          hi = vh / 255.0f;

          if (i == 31)
            g_mode7_rgba_midpoints[i][p] = 1.0f;
          else
            g_mode7_rgba_midpoints[i][p] = (lo + hi) / 2.0f;
        }
    }

  /* Mode 1 endpoint midpoints */
  for (guint32 p = 0; p < 2; p++)
    {
      for (guint32 i = 0; i < 64; i++)
        {
          guint32 vl = ((i << 1) | p) << 1;
          gfloat  lo;
          guint32 vh;
          gfloat  hi;

          vl |= (vl >> 7);
          lo = vl / 255.0f;

          vh = ((MIN (63, (i + 1)) << 1) | p) << 1;
          vh |= (vh >> 7);
          hi = vh / 255.0f;

          if (i == 63)
            g_mode1_rgba_midpoints[i][p] = 1.0f;
          else
            g_mode1_rgba_midpoints[i][p] = (lo + hi) / 2.0f;
        }
    }

  /* Mode 5 endpoint midpoints */
  for (guint32 i = 0; i < 128; i++)
    {
      guint32 vl = (i << 1);
      gfloat  lo;
      guint32 vh;
      gfloat  hi;

      vl |= (vl >> 7);
      lo = vl / 255.0f;

      vh = MIN (127, i + 1) << 1;
      vh |= (vh >> 7);
      hi = vh / 255.0f;

      if (i == 127)
        g_mode5_rgba_midpoints[i] = 1.0f;
      else
        g_mode5_rgba_midpoints[i] = (lo + hi) / 2.0f;
    }

  /* Mode 6 */
  for (guint32 p = 0; p < 2; p++)
    {
      for (guint32 i = 0; i < 2048; i++)
        {
          gfloat f          = i / 2047.0f;
          gfloat best_err   = 1e+9f;
          gint   best_index = 0;

          for (guint j = 0; j < 64; j++)
            {
              gint   ik = (j * 127 + 31) / 63;
              gfloat k  = ((ik << 1) + p) / 255.0f;
              gfloat e  = fabsf (k - f);

              if (e < best_err)
                {
                  best_err   = e;
                  best_index = ik;
                }
            }
          g_mode6_reduced_quant[i][p] = (guchar) best_index;
        }
    }

  /* Mode 1 */
  for (guint32 c = 0; c < 256; c++)
    {
      for (guint32 lp = 0; lp < 2; lp++)
        {
          endpoint_err best;

          best.m_error = (gushort) G_MAXUINT16;
          for (guint32 l = 0; l < 64; l++)
            {
              guint32 low = ((l << 1) | lp) << 1;

              low |= (low >> 7);
              for (guint h = 0; h < 64; h++)
                {
                  guint32 high = ((h << 1) | lp) << 1;
                  gint    k;
                  gint    err;

                  high |= (high >> 7);
                  k   = (low * (64 - weight_3[2]) + high * weight_3[2] + 32) >> 6;
                  err = (k - c) * (k - c);
                  if (err < best.m_error)
                    {
                      best.m_error = (gushort) err;
                      best.m_lo    = (guchar) l;
                      best.m_hi    = (guchar) h;
                    }
                }
            }
          g_bc7_mode_1_optimal_endpoints[c][lp] = best;
        }
    }

  /* Mode 7: 555.1 2-bit indices */
  for (guint32 c = 0; c < 256; c++)
    {
      for (guint32 hp = 0; hp < 2; hp++)
        {
          for (gint lp = 0; lp < 2; lp++)
            {
              endpoint_err best;

              best.m_error = (gushort) G_MAXUINT16;
              best.m_lo = 0;
              best.m_hi = 0;

              for (guint32 l = 0; l < 32; l++)
                {
                  guint32 low = ((l << 1) | lp) << 2;

                  low |= (low >> 6);
                  for (guint h = 0; h < 32; h++)
                    {
                      guint32 high = ((h << 1) | hp) << 2;
                      gint    k;
                      gint    err;

                      high |= (high >> 6);

                      k   = (low * (64 - weight_2[1]) + high * weight_2[1] + 32) >> 6;
                      err = (k - c) * (k - c);
                      if (err < best.m_error)
                        {
                          best.m_error = (gushort) err;
                          best.m_lo    = (guchar) l;
                          best.m_hi    = (guchar) h;
                        }
                    }
                }
              g_bc7_mode_7_optimal_endpoints[c][hp][lp] = best;
            }
        }
    }
  modes_initialized = TRUE;
}

gint
bc7_decompress (guchar *src,
                guint   size,
                guchar *block)
{
  /* BC7 blocks are always 16 bytes */
  guchar     s[16];
  BC7_colors colors;
  guchar     rgb_indexes[16];
  guchar     alpha_indexes[16];
  guchar     subset_indexes[16];
  guchar     rgba[4];
  guint      mode             = 0;
  guint      subset_count     = 0;
  guint      partition_id     = 0;
  guint      swap             = 0;
  guint      selector         = 0;
  guchar     current_bit      = 0;
  guint      i_precision      = 0;
  guint      no_sel_precision = 0;

  for (gint i = 0; i < 16; i++)
    s[i] = src[i];

  /* BC7 blocks are read from the last bit of the last byte, backwards.
   * The mode is determined by the first 1 bit encountered. For instance,
   * 1 is mode 0, 10 is mode 1, 100 is mode 2, and so on. */
  while (current_bit <= 8 && ! get_bits (s, &current_bit, 1))
    {
      continue;
    }
  mode = current_bit - 1;

  if (mode > 7)
    return 0;

  /* For modes that support partitions, we start counting
   * from left of the mode bit, and get the partition ID.
   * The size of the partition ID is defined by the mode. */
  subset_count = mode_info[mode].subset_count;
  if (subset_count > 1)
    {
      partition_id = get_bits (s, &current_bit,
                               mode_info[mode].partition_bits);

      if (partition_id > 63)
        return 0;
    }

  /* Mode 4 and 5 might swap channels for better compression. 2 bits after the mode bit
   * indicate which were swapped (if any) */
  if (mode == 4 || mode == 5)
    swap = get_bits (s, &current_bit, 2);

  /* Mode 4 also has a single selector bit, to the left of its swap bits.
   * This bit determines where the alpha channel indexes are located. */
  if (mode == 4)
    selector = get_bits (s, &current_bit, 1);

  /* Channel values are stored in RnGnBn(An) format, where the mode determines
   * how many bits of precision and how many values are stored. */
  decode_rgba_channels (&colors, s, mode, &current_bit);

  /* Next, we get the indexes to assemble pixels from the channel values. */
  i_precision      = mode_info[mode].index_precision;
  no_sel_precision = mode_info[mode].no_sel_precision;

  if (mode == 4 && selector)
    {
      i_precision      = 3;
      alpha_indexes[0] = get_bits (s, &current_bit, 1);

      for (gint i = 1; i < 16; i++)
        alpha_indexes[i] = get_bits (s, &current_bit, 2);
    }

  /* First, calculate the RGB channel indexes based on the partition ID
   * and the number of subsets.  */
  for (gint i = 0; i < 16; i++)
    {
      guint precision = i_precision;

      if (subset_count == 2)
        subset_indexes[i] = partition_table[0][partition_id][i];
      else if (subset_count == 3)
        subset_indexes[i] = partition_table[1][partition_id][i];
      else
        subset_indexes[i] = 0;

      if (is_pixel_anchor (subset_indexes[i], subset_count, i, partition_id))
        precision--;

      rgb_indexes[i]= get_bits (s, &current_bit, precision);
    }

  if (mode == 5 || (mode == 4 && ! selector))
    {
      alpha_indexes[0] = get_bits (s, &current_bit, no_sel_precision - 1);

      for (gint i = 1; i < 16; i++)
        alpha_indexes[i] = get_bits (s, &current_bit, no_sel_precision);
    }

  /* Create pixels from subset indexes */
  for (gint i = 0; i < 16; i++)
    {
      guint  weight;
      guint  c0 = 2 * subset_indexes[i];
      guint  c1 = (2 * subset_indexes[i]) + 1;

      if (i_precision == 2)
        weight = weight_2[rgb_indexes[i]];
      else if (i_precision == 3)
        weight = weight_3[rgb_indexes[i]];
      else
        weight = weight_4[rgb_indexes[i]];

      rgba[0] = ((64 - weight) * colors.r[c0] + weight * colors.r[c1] + 32) >> 6;
      rgba[1] = ((64 - weight) * colors.g[c0] + weight * colors.g[c1] + 32) >> 6;
      rgba[2] = ((64 - weight) * colors.b[c0] + weight * colors.b[c1] + 32) >> 6;

      if (mode == 4 || mode == 5)
        {
          weight = weight_2[alpha_indexes[i]];

          if (mode == 4 && ! selector)
            weight = weight_3[alpha_indexes[i]];
        }
      rgba[3] = ((64 - weight) * colors.a[c0] + weight * colors.a[c1] + 32) >> 6;

      switch (swap)
        {
          case 1:
            SWAP (rgba[3], rgba[0]);
            break;
          case 2:
            SWAP (rgba[3], rgba[1]);
            break;
          case 3:
            SWAP (rgba[3], rgba[2]);
            break;
          default:
            break;
         }

      for (gint j = 0; j < 4; j++)
        block[(i * 4) + j] = rgba[j];
    }

  return 1;
}

void
bc7_compress (guchar       *dst,
              const guchar *src,
              bc7_params   *params)
{
  color_cell_compressor_params  color_params;
  const color_rgba             *pPixels = (const color_rgba *) (src);

  if (params->m_perceptual)
    {
      const gfloat pr_weight = (0.5f / (1.0f - .2126f)) * (0.5f / (1.0f - .2126f));
      const gfloat pb_weight = (0.5f / (1.0f - .0722f)) * (0.5f / (1.0f - .0722f));

      color_params.m_weights[0] = (gint) (params->m_weights[0] * 4.0f);
      color_params.m_weights[1] = (gint) (params->m_weights[1] * 4.0f * pr_weight);
      color_params.m_weights[2] = (gint) (params->m_weights[2] * 4.0f * pb_weight);
      color_params.m_weights[3] = params->m_weights[3] * 4;
    }
  else
    {
      memcpy (color_params.m_weights, params->m_weights,
              sizeof (color_params.m_weights));
    }

  if (params->m_force_alpha)
    {
      handle_alpha_block (dst, pPixels, params, &color_params);
      return;
    }

  /* Check if any pixels in the block have transparency;
   * otherwise, run opaque code */
  for (gint i = 0; i < 16; i++)
    {
      if (pPixels[i].m_c[3] < 255)
        {
          handle_alpha_block (dst, pPixels, params, &color_params);
          return;
        }
     }
  handle_opaque_block (dst, pPixels, params, &color_params);
}

/* Private Functions */

static guchar
get_bits (const guchar *block,
          guchar       *start_bit,
          guchar        length)
{
  guchar bits;
  guint  index;
  guint  base;
  guint  first_bits;
  guint  next_bits;

  index = (*start_bit) >> 3;
  base  = (*start_bit) - (index << 3);

  if (index > 15)
    return 0;

  if (base + length > 8)
    {
      first_bits = 8 - base;
      next_bits  = length - first_bits;

      bits = block[index] >> base;
      bits |= (block[index + 1] & ((1 << next_bits) - 1)) << first_bits;
    }
  else
    {
      bits = (block[index] >> base) & ((1 << length) - 1);
    }
  (*start_bit) += length;

  return bits;
}

static void
decode_rgba_channels (BC7_colors *colors,
                      guchar     *src,
                      guint       mode,
                      guchar     *start)
{
  guint channel_count   = mode_info[mode].subset_count * 2;
  guint rgb_precision   = mode_info[mode].rgb_precision;
  guint alpha_precision = mode_info[mode].alpha_precision;

  /* Get RGB channel values */
  for (gint i = 0; i < channel_count; i++)
    colors->r[i] = get_bits (src, start, rgb_precision);

  for (gint i = 0; i < channel_count; i++)
    colors->g[i] = get_bits (src, start, rgb_precision);

  for (gint i = 0; i < channel_count; i++)
    colors->b[i] = get_bits (src, start, rgb_precision);

  /* Modes 4 - 7 also have alpha values */
  if (alpha_precision)
    {
      for (gint i = 0; i < channel_count; i++)
        colors->a[i] = get_bits (src, start, alpha_precision);
    }
  else
    {
      for (gint i = 0; i < channel_count; i++)
        colors->a[i] = 255;
    }

  /* Modes 0, 1, 3, 6, and 7 have P-bits, which increase the precision
   * by 1. */
  if (mode_info[mode].p_bit_count)
    {
      rgb_precision++;
      if (alpha_precision)
        alpha_precision++;

      /* P-bits are added at the least significant bit, so
       * we have to shift existing channel values over by 1 */
      for (gint i = 0; i < channel_count; i++)
        {
          colors->r[i] <<= 1;
          colors->g[i] <<= 1;
          colors->b[i] <<= 1;
          if (alpha_precision)
            colors->a[i] <<= 1;
        }

      if (mode == 1)
        {
          guint p_bit_1 = get_bits (src, start, 1);
          guint p_bit_2 = get_bits (src, start, 1);

          for (gint i = 0; i < 2; i++)
            {
              colors->r[i] |= p_bit_1;
              colors->g[i] |= p_bit_1;
              colors->b[i] |= p_bit_1;

              colors->r[i + 2] |= p_bit_2;
              colors->g[i + 2] |= p_bit_2;
              colors->b[i + 2] |= p_bit_2;
            }
        }
      else
        {
          for (gint i = 0; i < channel_count; i++)
            {
              guint p_bit = get_bits (src, start, 1);

              colors->r[i] |= p_bit;
              colors->g[i] |= p_bit;
              colors->b[i] |= p_bit;
              if (alpha_precision)
                colors->a[i] |= p_bit;
            }
        }
    }

  /* Normalize all channel values to 8 bits */
  for (gint i = 0; i < channel_count; i++)
    {
      colors->r[i] <<= (8 - rgb_precision);
      colors->g[i] <<= (8 - rgb_precision);
      colors->b[i] <<= (8 - rgb_precision);

      colors->r[i] = colors->r[i] | (colors->r[i] >> rgb_precision);
      colors->g[i] = colors->g[i] | (colors->g[i] >> rgb_precision);
      colors->b[i] = colors->b[i] | (colors->b[i] >> rgb_precision);

      if (alpha_precision)
        {
          colors->a[i] <<= (8 - alpha_precision);
          colors->a[i] = colors->a[i] | (colors->a[i] >> alpha_precision);
        }
    }
}

static gboolean
is_pixel_anchor (guchar subset_index,
                 guint  precision,
                 guchar pixel_index,
                 guint  partition_id)
{
  guint table_index;

  if (subset_index == 0)
    table_index = 0;
  else if (subset_index == 1 && precision == 2)
    table_index = 1;
  else if (subset_index == 1 && precision == 3)
    table_index = 2;
  else
    table_index = 3;

  if (anchor_index_table[table_index][partition_id] == pixel_index)
    return TRUE;

  return FALSE;
}

/* Compression functions */
static void
handle_alpha_block (void                         *pBlock,
                    const color_rgba             *pPixels,
                    const bc7_params             *pComp_params,
                    color_cell_compressor_params *pParams)
{
  bc7_optimization_results      opt_results5;
  bc7_optimization_results      opt_results6;
  bc7_optimization_results      opt_results7;
  color_cell_compressor_results results6;

  guint64 best_err  = G_MAXUINT64;
  guint32 best_mode = 0;
  guchar  selectors_temp[16];

  pParams->m_pSelector_weights    = weight_4;
  pParams->m_pSelector_weightsx   = (const vec4F *) g_bc7_weights4x;
  pParams->m_num_selector_weights = 16;
  pParams->m_comp_bits            = 7;
  pParams->m_has_pbits            = TRUE;
  pParams->m_endpoints_share_pbit = FALSE;
  pParams->m_has_alpha            = TRUE;
  pParams->m_perceptual           = pComp_params->m_perceptual;
  pParams->m_num_pixels           = 16;
  pParams->m_pPixels              = pPixels;

  memset (&results6, 0, sizeof (results6));

  if (pComp_params->m_mode_mask & (1 << 6))
    {
      results6.m_pSelectors      = opt_results6.m_selectors;
      results6.m_pSelectors_temp = selectors_temp;

      best_err = (guint64) (color_cell_compression (6, pParams, &results6, pComp_params) *
                            pComp_params->m_mode6_error_weight + .5f);
      best_mode = 6;
    }

  if ((best_err > 0) && (pComp_params->m_mode_mask & (1 << 5)))
    {
      guint32 lo_a = 255, hi_a = 0;
      guint64 mode5_err, mode5_alpha_err;

      for (gint i = 0; i < 16; i++)
        {
          guint32 a = pPixels[i].m_c[3];

          lo_a = MIN (lo_a, a);
          hi_a = MAX (hi_a, a);
        }

      handle_alpha_block_mode5 (pPixels, pComp_params, pParams, lo_a, hi_a,
                                &opt_results5, &mode5_err, &mode5_alpha_err);

      mode5_err =
        (guint64) (mode5_err * pComp_params->m_mode5_error_weight + 0.5f);

      if (mode5_err < best_err)
        {
          best_err  = mode5_err;
          best_mode = 5;
        }
    }

  if ((best_err > 0) && (pComp_params->m_mode_mask & (1 << 7)))
    {
      guint32                        trial_partition;
      guint32                        subset_total_colors7[2] = { 0, 0 };
      guchar                         subset_pixel_index7[2][16];
      guchar                         subset_selectors7[2][16];
      color_cell_compressor_results  subset_results7[2];
      guint64                        trial_err = 0;
      color_rgba                     subset_colors[2][16];
      guint64                        mode7_trial_err;

      trial_partition = estimate_partition (pPixels, pComp_params,
                                            pParams->m_weights, 7);

      pParams->m_pSelector_weights    = weight_2;
      pParams->m_pSelector_weightsx   = (const vec4F *) g_bc7_weights2x;
      pParams->m_num_selector_weights = 4;
      pParams->m_comp_bits            = 5;
      pParams->m_has_pbits            = TRUE;
      pParams->m_endpoints_share_pbit = FALSE;
      pParams->m_has_alpha            = TRUE;

      for (gint idx = 0; idx < 16; idx++)
        {
          const guint32 p = partition_table[0][trial_partition][idx];

          subset_colors[p][subset_total_colors7[p]]       = pPixels[idx];
          subset_pixel_index7[p][subset_total_colors7[p]] = (guchar) idx;
          subset_total_colors7[p]++;
        }

      for (gint subset = 0; subset < 2; subset++)
        {
          guint64                         err;
          color_cell_compressor_results * pResults = &subset_results7[subset];

          pParams->m_num_pixels = subset_total_colors7[subset];
          pParams->m_pPixels    = &subset_colors[subset][0];

          pResults->m_pSelectors      = &subset_selectors7[subset][0];
          pResults->m_pSelectors_temp = selectors_temp;

          err = color_cell_compression (7, pParams, pResults, pComp_params);

          trial_err += err;
          if ((guint64) (trial_err * pComp_params->m_mode7_error_weight + .5f) > best_err)
            break;
        }

      mode7_trial_err =
        (guint64) (trial_err * pComp_params->m_mode7_error_weight + 0.5f);

      if (mode7_trial_err < best_err)
        {
          best_err                      = mode7_trial_err;
          best_mode                     = 7;
          opt_results7.m_mode           = 7;
          opt_results7.m_partition      = trial_partition;
          opt_results7.m_index_selector = 0;
          opt_results7.m_rotation       = 0;

          for (gint subset = 0; subset < 2; subset++)
            {
              for (gint i = 0; i < subset_total_colors7[subset]; i++)
                opt_results7.m_selectors[subset_pixel_index7[subset][i]] =
                  subset_selectors7[subset][i];

              opt_results7.m_low[subset] =
                subset_results7[subset].m_low_endpoint;
              opt_results7.m_high[subset] =
                subset_results7[subset].m_high_endpoint;
              opt_results7.m_pbits[subset][0] =
                subset_results7[subset].m_pbits[0];
              opt_results7.m_pbits[subset][1] =
                subset_results7[subset].m_pbits[1];
            }
        }
    }

  if (best_mode == 7)
    {
      encode_bc7_block (pBlock, &opt_results7);
    }
  else if (best_mode == 5)
    {
      opt_results5.m_mode           = 5;
      opt_results5.m_partition      = 0;
      opt_results5.m_rotation       = 0;
      opt_results5.m_index_selector = 0;

      encode_bc7_block (pBlock, &opt_results5);
    }
  else if (best_mode == 6)
    {
      opt_results6.m_mode           = 6;
      opt_results6.m_partition      = 0;
      opt_results6.m_low[0]         = results6.m_low_endpoint;
      opt_results6.m_high[0]        = results6.m_high_endpoint;
      opt_results6.m_pbits[0][0]    = results6.m_pbits[0];
      opt_results6.m_pbits[0][1]    = results6.m_pbits[1];
      opt_results6.m_rotation       = 0;
      opt_results6.m_index_selector = 0;

      encode_bc7_block (pBlock, &opt_results6);
    }
}

static void
handle_alpha_block_mode5 (const color_rgba             *pPixels,
                          const bc7_params             *pComp_params,
                          color_cell_compressor_params *pParams,
                          guint32                       lo_a,
                          guint32                       hi_a,
                          bc7_optimization_results     *pOpt_results5,
                          guint64                      *pMode5_err,
                          guint64                      *pMode5_alpha_err)
{
  guchar                        selectors_temp[16];
  color_cell_compressor_results results5;

  pParams->m_pSelector_weights    = weight_2;
  pParams->m_pSelector_weightsx   = (const vec4F *) g_bc7_weights2x;
  pParams->m_num_selector_weights = 4;

  pParams->m_comp_bits            = 7;
  pParams->m_has_pbits            = FALSE;
  pParams->m_endpoints_share_pbit = FALSE;
  pParams->m_has_alpha            = FALSE;

  pParams->m_perceptual      = pComp_params->m_perceptual;
  pParams->m_num_pixels      = 16;
  pParams->m_pPixels         = pPixels;
  results5.m_pSelectors      = pOpt_results5->m_selectors;
  results5.m_pSelectors_temp = selectors_temp;

  *pMode5_err = color_cell_compression (5, pParams, &results5, pComp_params);

  pOpt_results5->m_low[0]  = results5.m_low_endpoint;
  pOpt_results5->m_high[0] = results5.m_high_endpoint;

  if (lo_a == hi_a)
    {
      *pMode5_alpha_err               = 0;
      pOpt_results5->m_low[0].m_c[3]  = (guchar) lo_a;
      pOpt_results5->m_high[0].m_c[3] = (guchar) hi_a;

      memset (pOpt_results5->m_alpha_selectors, 0,
              sizeof(pOpt_results5->m_alpha_selectors));
    }
  else
    {
      const guint32 total_passes = (pComp_params->m_uber_level >= 1) ? 3 : 2;

      *pMode5_alpha_err = G_MAXUINT64;
      for (gint pass = 0; pass < total_passes; pass++)
        {
          gint32       vals[4];
          guchar       trial_alpha_selectors[16];
          guint32      trial_alpha_err = 0;
          const gint32 w_s1 = 21, w_s2 = 43;

          vals[0] = lo_a;
          vals[3] = hi_a;

          vals[1] = (vals[0] * (64 - w_s1) + vals[3] * w_s1 + 32) >> 6;
          vals[2] = (vals[0] * (64 - w_s2) + vals[3] * w_s2 + 32) >> 6;

          for (gint i = 0; i < 16; i++)
            {
              const gint32 a  = pParams->m_pPixels[i].m_c[3];
              gint         s  = 0;
              gint32       be = iabs32 (a - vals[0]);
              gint         e  = iabs32 (a - vals[1]);
              guint32      a_err;

              if (e < be)
                {
                  be = e;
                  s  = 1;
                }

              e = iabs32 (a - vals[2]);
              if (e < be)
                {
                  be = e;
                  s  = 2;
                }
              e = iabs32 (a - vals[3]);
              if (e < be)
                {
                  be = e;
                  s  = 3;
                }

              trial_alpha_selectors[i] = (guchar) s;

              a_err = (guint32) (be * be) * pParams->m_weights[3];

              trial_alpha_err += a_err;
            }

          if (trial_alpha_err < *pMode5_alpha_err)
            {
              *pMode5_alpha_err               = trial_alpha_err;
              pOpt_results5->m_low[0].m_c[3]  = (guchar) lo_a;
              pOpt_results5->m_high[0].m_c[3] = (guchar) hi_a;

              memcpy (pOpt_results5->m_alpha_selectors, trial_alpha_selectors,
                      sizeof (pOpt_results5->m_alpha_selectors));
            }

          if (pass != (total_passes - 1U))
            {
              gfloat  xl, xh;
              guint32 new_lo_a;
              guint32 new_hi_a;

              compute_least_squares_endpoints_a (16, trial_alpha_selectors,
                                                 (const vec4F *) g_bc7_weights2x,
                                                 &xl, &xh, pParams->m_pPixels);

              new_lo_a = CLAMP ((gint) floor (xl + 0.5f), 0, 255);
              new_hi_a = CLAMP ((gint) floor (xh + 0.5f), 0, 255);
              if (new_lo_a > new_hi_a)
                swapu (&new_lo_a, &new_hi_a);

              if ((new_lo_a == lo_a) && (new_hi_a == hi_a))
                break;

              lo_a = new_lo_a;
              hi_a = new_hi_a;
            }
        }

      *pMode5_err += *pMode5_alpha_err;
    }
}

static void
handle_opaque_block (void                         *pBlock,
                     const color_rgba             *pPixels,
                     const bc7_params             *pComp_params,
                     color_cell_compressor_params *pParams)
{
  guchar                   selectors_temp[16];
  bc7_optimization_results opt_results;
  guint64                  best_err = G_MAXUINT64;

  pParams->m_perceptual        = pComp_params->m_perceptual;
  pParams->m_num_pixels        = 16;
  pParams->m_pPixels           = pPixels;
  pParams->m_has_alpha         = FALSE;
  opt_results.m_partition      = 0;
  opt_results.m_index_selector = 0;
  opt_results.m_rotation       = 0;

  /* Mode 6 */
  if (pComp_params->m_mode_mask & (1 << 6))
    {
      color_cell_compressor_results results6;

      pParams->m_pSelector_weights    = weight_4;
      pParams->m_pSelector_weightsx   = (const vec4F *) g_bc7_weights4x;
      pParams->m_num_selector_weights = 16;
      pParams->m_comp_bits            = 7;
      pParams->m_has_pbits            = TRUE;
      pParams->m_endpoints_share_pbit = FALSE;

      results6.m_pSelectors           = opt_results.m_selectors;
      results6.m_pSelectors_temp      = selectors_temp;

      best_err =
        (guint64) (color_cell_compression (6, pParams, &results6, pComp_params) *
                   pComp_params->m_mode6_error_weight + 0.5f);

      opt_results.m_mode        = 6;
      opt_results.m_low[0]      = results6.m_low_endpoint;
      opt_results.m_high[0]     = results6.m_high_endpoint;
      opt_results.m_pbits[0][0] = results6.m_pbits[0];
      opt_results.m_pbits[0][1] = results6.m_pbits[1];
    }

  /* Mode 1 */
  if ((best_err > 0)                       &&
      (pComp_params->m_max_partitions > 0) &&
      (pComp_params->m_mode_mask & (1 << 1)))
    {
      guint32                       trial_partition;
      guint64                       mode1_trial_err;
      guint64                       trial_err = 0;
      const guchar                 *pPartition;
      color_rgba                    subset_colors[2][16];
      guint32                       subset_total_colors1[2] = { 0, 0 };
      guchar                        subset_pixel_index1[2][16];
      guchar                        subset_selectors1[2][16];
      color_cell_compressor_results subset_results1[2];

      trial_partition = estimate_partition (pPixels, pComp_params,
                                            pParams->m_weights, 1);

      pParams->m_pSelector_weights    = weight_3;
      pParams->m_pSelector_weightsx   = (const vec4F *) g_bc7_weights3x;
      pParams->m_num_selector_weights = 8;
      pParams->m_comp_bits            = 6;
      pParams->m_has_pbits            = TRUE;
      pParams->m_endpoints_share_pbit = TRUE;

      pPartition = &partition_table[0][trial_partition][0];

      for (guint32 idx = 0; idx < 16; idx++)
        {
          const guint32 p = pPartition[idx];

          subset_colors[p][subset_total_colors1[p]]       = pPixels[idx];
          subset_pixel_index1[p][subset_total_colors1[p]] = (guchar) idx;
          subset_total_colors1[p]++;
        }

      for (guint32 subset = 0; subset < 2; subset++)
        {
          guint64                        err;
          color_cell_compressor_results *pResults = &subset_results1[subset];

          pParams->m_num_pixels = subset_total_colors1[subset];
          pParams->m_pPixels    = &subset_colors[subset][0];

          pResults->m_pSelectors      = &subset_selectors1[subset][0];
          pResults->m_pSelectors_temp = selectors_temp;

          err = color_cell_compression (1, pParams, pResults, pComp_params);

          trial_err += err;
          if ((guint64) (trial_err * pComp_params->m_mode1_error_weight + 0.5f) > best_err)
            break;

        }

      mode1_trial_err =
        (guint64) (trial_err * pComp_params->m_mode1_error_weight + 0.5f);

      if (mode1_trial_err < best_err)
        {
          best_err                = mode1_trial_err;
          opt_results.m_mode      = 1;
          opt_results.m_partition = trial_partition;

          for (guint32 subset = 0; subset < 2; subset++)
            {
              for (guint32 i = 0; i < subset_total_colors1[subset]; i++)
                opt_results.m_selectors[subset_pixel_index1[subset][i]] =
                  subset_selectors1[subset][i];

              opt_results.m_low[subset]      = subset_results1[subset].m_low_endpoint;
              opt_results.m_high[subset]     = subset_results1[subset].m_high_endpoint;
              opt_results.m_pbits[subset][0] = subset_results1[subset].m_pbits[0];
            }
        }
    }

  encode_bc7_block (pBlock, &opt_results);
}

static guint64
color_cell_compression (guint32                             mode,
                        const color_cell_compressor_params *pParams,
                        color_cell_compressor_results      *pResults,
                        const bc7_params                   *pComp_params)
{
  vec4F  meanColor, axis;
  vec4F  meanColorScaled;
  gfloat l = 1e+9f;
  gfloat h = -1e+9f;
  vec4F  b0;
  vec4F  b1;
  vec4F  c0;
  vec4F  c1;
  vec4F  minColor;
  vec4F  maxColor;
  vec4F  whiteVec;

  pResults->m_best_overall_err = G_MAXUINT64;

  /* If the partition's colors are all the same in mode 1, then just pack them
   * as a single color. */
  if (mode == 1)
    {
      const guint32 cr      = pParams->m_pPixels[0].m_c[0];
      const guint32 cg      = pParams->m_pPixels[0].m_c[1];
      const guint32 cb      = pParams->m_pPixels[0].m_c[2];
      gboolean      allSame = TRUE;

      for (guint i = 1; i < pParams->m_num_pixels; i++)
        {
          if ((cr != pParams->m_pPixels[i].m_c[0]) ||
              (cg != pParams->m_pPixels[i].m_c[1]) ||
              (cb != pParams->m_pPixels[i].m_c[2]))
            {
              allSame = FALSE;
              break;
            }
        }

      if (allSame)
        return pack_mode1_to_one_color (pParams, pResults, cr, cg, cb,
                                        pResults->m_pSelectors);
    }
  else if (mode == 7)
    {
      const guint32 cr      = pParams->m_pPixels[0].m_c[0];
      const guint32 cg      = pParams->m_pPixels[0].m_c[1];
      const guint32 cb      = pParams->m_pPixels[0].m_c[2];
      const guint32 ca      = pParams->m_pPixels[0].m_c[3];
      gboolean      allSame = TRUE;

      for (guint32 i = 1; i < pParams->m_num_pixels; i++)
        {
          if ((cr != pParams->m_pPixels[i].m_c[0]) ||
              (cg != pParams->m_pPixels[i].m_c[1]) ||
              (cb != pParams->m_pPixels[i].m_c[2]) ||
              (ca != pParams->m_pPixels[i].m_c[3]))
          {
            allSame = FALSE;
            break;
          }
        }

      if (allSame)
        return pack_mode7_to_one_color (pParams, pResults, cr, cg, cb, ca,
                                        pResults->m_pSelectors,
                                        pParams->m_num_pixels,
                                        pParams->m_pPixels);
    }

  /* Compute partition's mean color and principle axis. */
  vec4F_set_scalar (&meanColor, 0.0f);
  for (gint i = 0; i < pParams->m_num_pixels; i++)
    {
      vec4F color = vec4F_from_color (&pParams->m_pPixels[i]);

      meanColor = vec4F_add (&meanColor, &color);
    }

  meanColorScaled = vec4F_mul (&meanColor,
                               1.0f / (gfloat) (pParams->m_num_pixels));

  meanColor = vec4F_mul (&meanColor,
                         1.0f / (gfloat) (pParams->m_num_pixels * 255.0f));
  vec4F_saturate_in_place (&meanColor);

  if (pParams->m_has_alpha)
    {
      /* Use incremental PCA for RGBA PCA, because it's simple. */
      vec4F_set_scalar (&axis, 0.0f);
      for (gint i = 0; i < pParams->m_num_pixels; i++)
        {
          vec4F color = vec4F_from_color (&pParams->m_pPixels[i]);
          vec4F a;
          vec4F b;
          vec4F c;
          vec4F d;
          vec4F n;

          color = vec4F_sub (&color, &meanColorScaled);
          a     = vec4F_mul (&color, color.m_c[0]);
          b     = vec4F_mul (&color, color.m_c[1]);
          c     = vec4F_mul (&color, color.m_c[2]);
          d     = vec4F_mul (&color, color.m_c[3]);
          n     = i ? axis : color;

          vec4F_normalize_in_place (&n);
          axis.m_c[0] += vec4F_dot (&a, &n);
          axis.m_c[1] += vec4F_dot (&b, &n);
          axis.m_c[2] += vec4F_dot (&c, &n);
          axis.m_c[3] += vec4F_dot (&d, &n);
        }
      vec4F_normalize_in_place (&axis);
    }
  else
    {
      /* Use covar technique for RGB PCA, because it doesn't require per-pixel
       * normalization. */
      gfloat cov[6] = { 0, 0, 0, 0, 0, 0 };
      gfloat vfr    = 0.9f;
      gfloat vfg    = 1.0f;
      gfloat vfb    = 0.7f;
      gfloat len;

      for (gint i = 0; i < pParams->m_num_pixels; i++)
        {
          gfloat r = pParams->m_pPixels[i].m_c[0] - meanColorScaled.m_c[0];
          gfloat g = pParams->m_pPixels[i].m_c[1] - meanColorScaled.m_c[1];
          gfloat b = pParams->m_pPixels[i].m_c[2] - meanColorScaled.m_c[2];

          cov[0] += r * r;
          cov[1] += r * g;
          cov[2] += r * b;
          cov[3] += g * g;
          cov[4] += g * b;
          cov[5] += b * b;
        }

      for (gint iter = 0; iter < 3; iter++)
        {
          gfloat r = vfr * cov[0] + vfg * cov[1] + vfb * cov[2];
          gfloat g = vfr * cov[1] + vfg * cov[3] + vfb * cov[4];
          gfloat b = vfr * cov[2] + vfg * cov[4] + vfb * cov[5];
          gfloat m = MAX (MAX (fabsf (r), fabsf (g)), fabsf (b));

          if (m > 1e-10f)
            {
              m = 1.0f / m;
              r *= m;
              g *= m;
              b *= m;
            }
          vfr = r;
          vfg = g;
          vfb = b;
        }

      len = (vfr * vfr) + (vfg * vfg) + (vfb * vfb);
      if (len < 1e-10f)
        {
          vec4F_set_scalar (&axis, 0.0f);
        }
      else
        {
          len = 1.0f / sqrtf (len);
          vfr *= len;
          vfg *= len;
          vfb *= len;

          vec4F_set (&axis, vfr, vfg, vfb, 0);
        }
    }

  if (vec4F_dot (&axis, &axis) < 0.5f)
    {
      if (pParams->m_perceptual)
        vec4F_set (&axis, 0.213f, 0.715f, 0.072f,
                   pParams->m_has_alpha ? 0.715f : 0);
      else
        vec4F_set (&axis, 1.0f, 1.0f, 1.0f,
                   pParams->m_has_alpha ? 1.0f : 0);
      vec4F_normalize_in_place (&axis);
    }

  for (guint i = 0; i < pParams->m_num_pixels; i++)
    {
      vec4F color = vec4F_from_color (&pParams->m_pPixels[i]);
      vec4F  q    = vec4F_sub (&color, &meanColorScaled);
      gfloat d    = vec4F_dot (&q, &axis);

      l = MIN (l, d);
      h = MAX (h, d);
    }

  l *= (1.0f / 255.0f);
  h *= (1.0f / 255.0f);

  b0       = vec4F_mul (&axis, l);
  b1       = vec4F_mul (&axis, h);
  c0       = vec4F_add (&meanColor, &b0);
  c1       = vec4F_add (&meanColor, &b1);
  minColor = vec4F_saturate (&c0);
  maxColor = vec4F_saturate (&c1);

  vec4F_set_scalar (&whiteVec, 1.0f);
  if (vec4F_dot (&minColor, &whiteVec) > vec4F_dot(&maxColor, &whiteVec))
    {
      gfloat a = minColor.m_c[0];
      gfloat b = minColor.m_c[1];
      gfloat c = minColor.m_c[2];
      gfloat d = minColor.m_c[3];

      minColor.m_c[0] = maxColor.m_c[0];
      minColor.m_c[1] = maxColor.m_c[1];
      minColor.m_c[2] = maxColor.m_c[2];
      minColor.m_c[3] = maxColor.m_c[3];
      maxColor.m_c[0] = a;
      maxColor.m_c[1] = b;
      maxColor.m_c[2] = c;
      maxColor.m_c[3] = d;
    }

  /* First find a solution using the block's PCA. */
  if (! find_optimal_solution (mode, minColor, maxColor, pParams, pResults,
                               pComp_params))
    return 0;

  if (pComp_params->m_try_least_squares)
    {
      /* Now try to refine the solution using least squares by computing the
       * optimal endpoints from the current selectors. */
      vec4F xl, xh;

      vec4F_set_scalar (&xl, 0.0f);
      vec4F_set_scalar (&xh, 0.0f);
      if (pParams->m_has_alpha)
        compute_least_squares_endpoints_rgba (pParams->m_num_pixels,
                                              pResults->m_pSelectors,
                                              pParams->m_pSelector_weightsx,
                                              &xl, &xh, pParams->m_pPixels);
      else
        compute_least_squares_endpoints_rgb (pParams->m_num_pixels,
                                             pResults->m_pSelectors,
                                             pParams->m_pSelector_weightsx,
                                             &xl, &xh, pParams->m_pPixels);

      xl = vec4F_mul (&xl, (1.0f / 255.0f));
      xh = vec4F_mul (&xh, (1.0f / 255.0f));

      if (! find_optimal_solution (mode, xl, xh, pParams, pResults,
                                   pComp_params))
        return 0;
    }

  if (pComp_params->m_uber_level > 0)
    {
      /* In uber level 1, try varying the selectors a little, somewhat like
       * cluster fit would. First try incrementing the minimum selectors,
       * then try decrementing the selectors, then try both. */
      guchar        selectors_temp[16], selectors_temp1[16];
      vec4F         xl, xh;
      guint32       min_sel = 16;
      guint32       max_sel = 0;
      const gint    max_selector    = pParams->m_num_selector_weights - 1;
      const guint32 uber_err_thresh = (pParams->m_num_pixels * 56) >> 4;

      memcpy (selectors_temp, pResults->m_pSelectors, pParams->m_num_pixels);

      for (gint i = 0; i < pParams->m_num_pixels; i++)
        {
          guint32 sel = selectors_temp[i];

          min_sel = MIN (min_sel, sel);
          max_sel = MAX (max_sel, sel);
        }

      for (gint i = 0; i < pParams->m_num_pixels; i++)
        {
          guint32 sel = selectors_temp[i];

          if ((sel == min_sel) && (sel < (pParams->m_num_selector_weights - 1)))
            sel++;
          selectors_temp1[i] = (guchar) sel;
        }

      vec4F_set_scalar (&xl, 0.0f);
      vec4F_set_scalar (&xh, 0.0f);
      if (pParams->m_has_alpha)
        compute_least_squares_endpoints_rgba (pParams->m_num_pixels,
                                              selectors_temp1,
                                              pParams->m_pSelector_weightsx,
                                              &xl, &xh, pParams->m_pPixels);
      else
        compute_least_squares_endpoints_rgb (pParams->m_num_pixels,
                                             selectors_temp1,
                                             pParams->m_pSelector_weightsx,
                                             &xl, &xh, pParams->m_pPixels);

      xl = vec4F_mul(&xl, (1.0f / 255.0f));
      xh = vec4F_mul(&xh, (1.0f / 255.0f));

      if (! find_optimal_solution(mode, xl, xh, pParams, pResults, pComp_params))
        return 0;

      for (gint i = 0; i < pParams->m_num_pixels; i++)
      {
        guint32 sel = selectors_temp[i];

        if ((sel == max_sel) && (sel > 0))
          sel--;
        selectors_temp1[i] = (guchar) sel;
      }

      if (pParams->m_has_alpha)
        compute_least_squares_endpoints_rgba (pParams->m_num_pixels,
                                              selectors_temp1,
                                              pParams->m_pSelector_weightsx,
                                              &xl, &xh, pParams->m_pPixels);
      else
        compute_least_squares_endpoints_rgb (pParams->m_num_pixels,
                                             selectors_temp1,
                                             pParams->m_pSelector_weightsx,
                                             &xl, &xh, pParams->m_pPixels);

      xl = vec4F_mul(&xl, (1.0f / 255.0f));
      xh = vec4F_mul(&xh, (1.0f / 255.0f));

      if (! find_optimal_solution(mode, xl, xh, pParams, pResults, pComp_params))
        return 0;

      for (gint i = 0; i < pParams->m_num_pixels; i++)
        {
          guint32 sel = selectors_temp[i];

          if ((sel == min_sel) && (sel < (pParams->m_num_selector_weights - 1)))
            sel++;
          else if ((sel == max_sel) && (sel > 0))
            sel--;
          selectors_temp1[i] = (guchar) sel;
        }

      if (pParams->m_has_alpha)
        compute_least_squares_endpoints_rgba (pParams->m_num_pixels,
                                              selectors_temp1,
                                              pParams->m_pSelector_weightsx,
                                              &xl, &xh, pParams->m_pPixels);
      else
        compute_least_squares_endpoints_rgb (pParams->m_num_pixels,
                                             selectors_temp1,
                                             pParams->m_pSelector_weightsx,
                                             &xl, &xh, pParams->m_pPixels);

      xl = vec4F_mul(&xl, (1.0f / 255.0f));
      xh = vec4F_mul(&xh, (1.0f / 255.0f));

      if (! find_optimal_solution (mode, xl, xh, pParams, pResults, pComp_params))
        return 0;

      /* In uber levels 2+, try taking more advantage of endpoint extrapolation
       * by scaling the selectors in one direction or another. */
      if ((pComp_params->m_uber_level >= 2) && (pResults->m_best_overall_err > uber_err_thresh))
        {
          const gint Q = (pComp_params->m_uber_level >= 4) ?
                          (pComp_params->m_uber_level - 2) : 1;

          for (gint ly = -Q; ly <= 1; ly++)
            {
              for (gint hy = max_selector - 1; hy <= (max_selector + Q); hy++)
                {
                  if ((ly == 0) && (hy == max_selector))
                    continue;

                  for (gint i = 0; i < pParams->m_num_pixels; i++)
                    selectors_temp1[i] = (guchar) CLAMP (floorf ((gfloat) max_selector             *
                                                        ((gfloat) selectors_temp[i] - (gfloat) ly) /
                                                        ((gfloat) hy - (gfloat) ly) + .5f),
                                                        0, (gfloat) max_selector);

                  vec4F_set_scalar (&xl, 0.0f);
                  vec4F_set_scalar (&xh, 0.0f);
                  if (pParams->m_has_alpha)
                    compute_least_squares_endpoints_rgba (pParams->m_num_pixels,
                                                          selectors_temp1,
                                                          pParams->m_pSelector_weightsx,
                                                          &xl, &xh, pParams->m_pPixels);
                  else
                    compute_least_squares_endpoints_rgb (pParams->m_num_pixels,
                                                         selectors_temp1,
                                                         pParams->m_pSelector_weightsx,
                                                         &xl, &xh, pParams->m_pPixels);

                  xl = vec4F_mul(&xl, (1.0f / 255.0f));
                  xh = vec4F_mul(&xh, (1.0f / 255.0f));

                  if (! find_optimal_solution (mode, xl, xh, pParams, pResults, pComp_params))
                    return 0;
                }
            }
        }
    }

  if (mode == 1)
    {
      /* Try encoding the partition as a single color by using the optimal
       * single colors tables to encode the block to its mean. */
      color_cell_compressor_results avg_results = *pResults;
      const guint32 r       = (gint) (0.5f + meanColor.m_c[0] * 255.0f);
      const guint32 g       = (gint) (0.5f + meanColor.m_c[1] * 255.0f);
      const guint32 b       = (gint) (0.5f + meanColor.m_c[2] * 255.0f);
      guint64       avg_err = pack_mode1_to_one_color (pParams, &avg_results,
                                                       r, g, b,
                                                       pResults->m_pSelectors_temp);

      if (avg_err < pResults->m_best_overall_err)
        {
          *pResults = avg_results;

          memcpy (pResults->m_pSelectors, pResults->m_pSelectors_temp,
                  sizeof (pResults->m_pSelectors[0]) * pParams->m_num_pixels);
          pResults->m_best_overall_err = avg_err;
        }
    }
  else if (mode == 7)
    {
      color_cell_compressor_results avg_results = *pResults;
      const guint32 r = (gint)(.5f + meanColor.m_c[0] * 255.0f);
      const guint32 g = (gint)(.5f + meanColor.m_c[1] * 255.0f);
      const guint32 b = (gint)(.5f + meanColor.m_c[2] * 255.0f);
      const guint32 a = (gint)(.5f + meanColor.m_c[3] * 255.0f);
      guint64 avg_err = pack_mode7_to_one_color (pParams, &avg_results,
                                                 r, g, b, a,
                                                 pResults->m_pSelectors_temp,
                                                 pParams->m_num_pixels,
                                                 pParams->m_pPixels);

      if (avg_err < pResults->m_best_overall_err)
        {
          *pResults = avg_results;

          memcpy (pResults->m_pSelectors, pResults->m_pSelectors_temp,
                  sizeof (pResults->m_pSelectors[0]) * pParams->m_num_pixels);
          pResults->m_best_overall_err = avg_err;
        }
    }

  return pResults->m_best_overall_err;
}

static guint64
pack_mode1_to_one_color (const color_cell_compressor_params *pParams,
                         color_cell_compressor_results      *pResults,
                         guint32                             r,
                         guint32                             g,
                         guint32                             b,
                         guchar                             *pSelectors)
{
  guint32             best_err  = G_MAXUINT32;
  guint32             best_p    = 0;
  guint64             total_err = 0;
  const endpoint_err *pEr;
  const endpoint_err *pEg;
  const endpoint_err *pEb;
  color_rgba          p;

  for (gint i = 0; i < 2; i++)
    {
      guint32 err = g_bc7_mode_1_optimal_endpoints[r][i].m_error +
                    g_bc7_mode_1_optimal_endpoints[g][i].m_error +
                    g_bc7_mode_1_optimal_endpoints[b][i].m_error;

      if (err < best_err)
        {
          best_err = err;
          best_p   = i;
          if (! best_err)
            break;
        }
    }

  pEr = &g_bc7_mode_1_optimal_endpoints[r][best_p];
  pEg = &g_bc7_mode_1_optimal_endpoints[g][best_p];
  pEb = &g_bc7_mode_1_optimal_endpoints[b][best_p];

  color_quad_u8_set (&pResults->m_low_endpoint, pEr->m_lo, pEg->m_lo,
                     pEb->m_lo, 0);
  color_quad_u8_set (&pResults->m_high_endpoint, pEr->m_hi, pEg->m_hi,
                     pEb->m_hi, 0);
  pResults->m_pbits[0] = best_p;
  pResults->m_pbits[1] = 0;

  memset (pSelectors, 2, pParams->m_num_pixels);

  for (gint i = 0; i < 3; i++)
    {
      guint32 low  = ((pResults->m_low_endpoint.m_c[i] << 1) |
                      pResults->m_pbits[0]) << 1;
      guint32 high = ((pResults->m_high_endpoint.m_c[i] << 1) |
                      pResults->m_pbits[0]) << 1;

      low  |= (low >> 7);
      high |= (high >> 7);

      p.m_c[i] = (guchar) ((low * (64 - weight_3[2]) +
                            high * weight_3[2] + 32) >> 6);
    }
  p.m_c[3] = 255;

  for (gint i = 0; i < pParams->m_num_pixels; i++)
    total_err += compute_color_distance_rgb (&p, &pParams->m_pPixels[i],
                                             pParams->m_perceptual,
                                             pParams->m_weights);

  pResults->m_best_overall_err = total_err;

  return total_err;
}

static guint64
pack_mode7_to_one_color (const color_cell_compressor_params *pParams,
                         color_cell_compressor_results      *pResults,
                         guint32                             r,
                         guint32                             g,
                         guint32                             b,
                         guint32                             a,
                         guchar                             *pSelectors,
                         guint32                             num_pixels,
                         const color_rgba                   *pPixels)
{
  guint32     best_err = G_MAXUINT;
  guint32     best_p   = 0;
  guint32     best_hi_p;
  guint32     best_lo_p;
  color_rgba  p;
  guint64     total_err;

  const endpoint_err *pEr;
  const endpoint_err *pEg;
  const endpoint_err *pEb;
  const endpoint_err *pEa;

  for (guint i = 0; i < 4; i++)
    {
      guint32 hi_p = i >> 1;
      guint32 lo_p = i & 1;
      guint32 err  = g_bc7_mode_7_optimal_endpoints[r][hi_p][lo_p].m_error +
                     g_bc7_mode_7_optimal_endpoints[g][hi_p][lo_p].m_error +
                     g_bc7_mode_7_optimal_endpoints[b][hi_p][lo_p].m_error +
                     g_bc7_mode_7_optimal_endpoints[a][hi_p][lo_p].m_error;

      if (err < best_err)
        {
          best_err = err;
          best_p = i;
          if (! best_err)
            break;
        }
    }

  best_hi_p = best_p >> 1;
  best_lo_p = best_p & 1;

  pEr = &g_bc7_mode_7_optimal_endpoints[r][best_hi_p][best_lo_p];
  pEg = &g_bc7_mode_7_optimal_endpoints[g][best_hi_p][best_lo_p];
  pEb = &g_bc7_mode_7_optimal_endpoints[b][best_hi_p][best_lo_p];
  pEa = &g_bc7_mode_7_optimal_endpoints[a][best_hi_p][best_lo_p];

  color_quad_u8_set (&pResults->m_low_endpoint, pEr->m_lo, pEg->m_lo,
                     pEb->m_lo, pEa->m_lo);
  color_quad_u8_set (&pResults->m_high_endpoint, pEr->m_hi, pEg->m_hi,
                     pEb->m_hi, pEa->m_hi);
  pResults->m_pbits[0] = best_lo_p;
  pResults->m_pbits[1] = best_hi_p;

  for (gint i = 0; i < num_pixels; i++)
    pSelectors[i] = (guchar) 1;

  for (gint i = 0; i < 4; i++)
    {
      guint32 low  = (pResults->m_low_endpoint.m_c[i] << 1) | pResults->m_pbits[0];
      guint32 high = (pResults->m_high_endpoint.m_c[i] << 1) | pResults->m_pbits[1];

      low = (low << 2) | (low >> 6);
      high = (high << 2) | (high >> 6);

      p.m_c[i] =
        (guchar) ((low * (64 - weight_2[1]) + high * weight_2[1] + 32) >> 6);
    }

  total_err = 0;
  for (gint i = 0; i < num_pixels; i++)
    total_err += compute_color_distance_rgba (&p, &pPixels[i],
                                              pParams->m_perceptual,
                                              pParams->m_weights);

  pResults->m_best_overall_err = total_err;

  return total_err;
}

static void
compute_least_squares_endpoints_rgb (guint32            N,
                                     const guchar      *pSelectors,
                                     const vec4F       *pSelector_weights,
                                     vec4F             *pXl,
                                     vec4F             *pXh,
                                     const color_rgba  *pColors)
{
  gfloat z00 = 0.0f, z01 = 0.0f, z10 = 0.0f, z11 = 0.0f;
  gfloat q00_r = 0.0f, q10_r = 0.0f, t_r = 0.0f;
  gfloat q00_g = 0.0f, q10_g = 0.0f, t_g = 0.0f;
  gfloat q00_b = 0.0f, q10_b = 0.0f, t_b = 0.0f;
  gfloat iz00, iz01, iz10, iz11;
  gfloat det;

  for (gint i = 0; i < N; i++)
    {
      const guint32 sel = pSelectors[i];
      gfloat        w   = pSelector_weights[sel].m_c[3];

      z00 += pSelector_weights[sel].m_c[0];
      z10 += pSelector_weights[sel].m_c[1];
      z11 += pSelector_weights[sel].m_c[2];

      q00_r += w * pColors[i].m_c[0];
      t_r   += pColors[i].m_c[0];
      q00_g += w * pColors[i].m_c[1];
      t_g   += pColors[i].m_c[1];
      q00_b += w * pColors[i].m_c[2];
      t_b   += pColors[i].m_c[2];
    }

  q10_r = t_r - q00_r;
  q10_g = t_g - q00_g;
  q10_b = t_b - q00_b;

  z01 = z10;

  det = z00 * z11 - z01 * z10;
  if (det != 0.0f)
    det = 1.0f / det;

  iz00 = z11 * det;
  iz01 = -z01 * det;
  iz10 = -z10 * det;
  iz11 = z00 * det;

  pXl->m_c[0] = (gfloat) (iz00 * q00_r + iz01 * q10_r);
  pXh->m_c[0] = (gfloat) (iz10 * q00_r + iz11 * q10_r);
  pXl->m_c[1] = (gfloat) (iz00 * q00_g + iz01 * q10_g);
  pXh->m_c[1] = (gfloat) (iz10 * q00_g + iz11 * q10_g);
  pXl->m_c[2] = (gfloat) (iz00 * q00_b + iz01 * q10_b);
  pXh->m_c[2] = (gfloat) (iz10 * q00_b + iz11 * q10_b);
  pXl->m_c[3] = 255.0f;
  pXh->m_c[3] = 255.0f;

  for (gint c = 0; c < 3; c++)
    {
      if ((pXl->m_c[c] < 0.0f) || (pXh->m_c[c] > 255.0f))
        {
          guint32 lo_v = G_MAXUINT32, hi_v = 0;

          for (gint i = 0; i < N; i++)
            {
              lo_v = MIN (lo_v, pColors[i].m_c[c]);
              hi_v = MAX (hi_v, pColors[i].m_c[c]);
            }

          if (lo_v == hi_v)
            {
              pXl->m_c[c] = (gfloat) lo_v;
              pXh->m_c[c] = (gfloat) hi_v;
            }
        }
    }
}

static void
compute_least_squares_endpoints_rgba (guint32            N,
                                      const guchar      *pSelectors,
                                      const vec4F       *pSelector_weights,
                                      vec4F             *pXl,
                                      vec4F             *pXh,
                                      const color_rgba  *pColors)
{
  /* Least squares using normal equations:
   * http://www.cs.cornell.edu/~bindel/class/cs3220-s12/notes/lec10.pdf
   * I did this in matrix form first, expanded out all the ops,
   * then optimized it a bit. */
  gfloat z00 = 0.0f, z01 = 0.0f, z10 = 0.0f, z11 = 0.0f;
  gfloat q00_r = 0.0f, q10_r = 0.0f, t_r = 0.0f;
  gfloat q00_g = 0.0f, q10_g = 0.0f, t_g = 0.0f;
  gfloat q00_b = 0.0f, q10_b = 0.0f, t_b = 0.0f;
  gfloat q00_a = 0.0f, q10_a = 0.0f, t_a = 0.0f;
  gfloat iz00, iz01, iz10, iz11;
  gfloat det;

  for (gint i = 0; i < N; i++)
    {
      const guint32 sel = pSelectors[i];
      gfloat        w   = pSelector_weights[sel].m_c[3];

      z00 += pSelector_weights[sel].m_c[0];
      z10 += pSelector_weights[sel].m_c[1];
      z11 += pSelector_weights[sel].m_c[2];

      q00_r += w * pColors[i].m_c[0];
      t_r   += pColors[i].m_c[0];
      q00_g += w * pColors[i].m_c[1];
      t_g   += pColors[i].m_c[1];
      q00_b += w * pColors[i].m_c[2];
      t_b   += pColors[i].m_c[2];
      q00_a += w * pColors[i].m_c[3];
      t_a   += pColors[i].m_c[3];
    }

  q10_r = t_r - q00_r;
  q10_g = t_g - q00_g;
  q10_b = t_b - q00_b;
  q10_a = t_a - q00_a;

  z01 = z10;

  det = z00 * z11 - z01 * z10;
  if (det != 0.0f)
    det = 1.0f / det;

  iz00 = z11 * det;
  iz01 = -z01 * det;
  iz10 = -z10 * det;
  iz11 = z00 * det;

  pXl->m_c[0] = (gfloat) (iz00 * q00_r + iz01 * q10_r);
  pXh->m_c[0] = (gfloat) (iz10 * q00_r + iz11 * q10_r);
  pXl->m_c[1] = (gfloat) (iz00 * q00_g + iz01 * q10_g);
  pXh->m_c[1] = (gfloat) (iz10 * q00_g + iz11 * q10_g);
  pXl->m_c[2] = (gfloat) (iz00 * q00_b + iz01 * q10_b);
  pXh->m_c[2] = (gfloat) (iz10 * q00_b + iz11 * q10_b);
  pXl->m_c[3] = (gfloat) (iz00 * q00_a + iz01 * q10_a);
  pXh->m_c[3] = (gfloat) (iz10 * q00_a + iz11 * q10_a);

  for (gint c = 0; c < 4; c++)
    {
      if ((pXl->m_c[c] < 0.0f) || (pXh->m_c[c] > 255.0f))
        {
          guint32 lo_v = G_MAXUINT32, hi_v = 0;

          for (gint i = 0; i < N; i++)
            {
              lo_v = MIN (lo_v, pColors[i].m_c[c]);
              hi_v = MAX (hi_v, pColors[i].m_c[c]);
            }

          if (lo_v == hi_v)
            {
              pXl->m_c[c] = (gfloat) lo_v;
              pXh->m_c[c] = (gfloat) hi_v;
            }
        }
    }
}

static void
compute_least_squares_endpoints_a (guint32           N,
                                   const guchar     *pSelectors,
                                   const vec4F      *pSelector_weights,
                                   gfloat           *pXl,
                                   gfloat           *pXh,
                                   const color_rgba *pColors)
{
  gfloat z00 = 0.0f, z01 = 0.0f, z10 = 0.0f, z11 = 0.0f;
  gfloat q00_a = 0.0f, q10_a = 0.0f, t_a = 0.0f;
  gfloat iz00, iz01, iz10, iz11;
  gfloat det;

  for (gint i = 0; i < N; i++)
    {
      const guint32 sel = pSelectors[i];
      gfloat        w   = pSelector_weights[sel].m_c[3];

      z00 += pSelector_weights[sel].m_c[0];
      z10 += pSelector_weights[sel].m_c[1];
      z11 += pSelector_weights[sel].m_c[2];

      q00_a += w * pColors[i].m_c[3];
      t_a   += pColors[i].m_c[3];
    }

  q10_a = t_a - q00_a;

  z01 = z10;

  det = z00 * z11 - z01 * z10;
  if (det != 0.0f)
    det = 1.0f / det;

  iz00 = z11 * det;
  iz01 = -z01 * det;
  iz10 = -z10 * det;
  iz11 = z00 * det;

  *pXl = (gfloat) (iz00 * q00_a + iz01 * q10_a);
  *pXh = (gfloat)(iz10 * q00_a + iz11 * q10_a);

  if ((*pXl < 0.0f) || (*pXh > 255.0f))
  {
    guint32 lo_v = G_MAXUINT32, hi_v = 0;

    for (gint i = 0; i < N; i++)
      {
        lo_v = MIN (lo_v, pColors[i].m_c[3]);
        hi_v = MAX (hi_v, pColors[i].m_c[3]);
      }

    if (lo_v == hi_v)
      {
        *pXl = (gfloat) lo_v;
        *pXh = (gfloat) hi_v;
      }
  }
}

static guint64
find_optimal_solution (guint32                             mode,
                       vec4F                               xl,
                       vec4F                               xh,
                       const color_cell_compressor_params *pParams,
                       color_cell_compressor_results      *pResults,
                       const bc7_params                   *pComp_params)
{
  vec4F_saturate_in_place (&xl);
  vec4F_saturate_in_place (&xh);

  if (pParams->m_has_pbits)
    {
      const gint    iscalep    = (1 << (pParams->m_comp_bits + 1)) - 1;
      const gfloat  scalep     = (gfloat) iscalep;
      const guint32 totalComps = pParams->m_has_alpha ? 4 : 3;

      guint32    best_pbits[2];
      color_rgba bestMinColor;
      color_rgba bestMaxColor;

      if (! pParams->m_endpoints_share_pbit)
      {
        if ((pParams->m_comp_bits == 7) && (pComp_params->m_quant_mode6_endpoints))
          {
            best_pbits[0] = 0;
            bestMinColor.m_c[0] =
              g_mode6_reduced_quant[(gint)((xl.m_c[0] * 2047.0f) + .5f)][0];
            bestMinColor.m_c[1] =
              g_mode6_reduced_quant[(gint)((xl.m_c[1] * 2047.0f) + .5f)][0];
            bestMinColor.m_c[2] =
              g_mode6_reduced_quant[(gint)((xl.m_c[2] * 2047.0f) + .5f)][0];
            bestMinColor.m_c[3] =
              g_mode6_reduced_quant[(gint)((xl.m_c[3] * 2047.0f) + .5f)][0];

            best_pbits[1] = 1;
            bestMaxColor.m_c[0] =
              g_mode6_reduced_quant[(gint)((xh.m_c[0] * 2047.0f) + .5f)][1];
            bestMaxColor.m_c[1] =
              g_mode6_reduced_quant[(gint)((xh.m_c[1] * 2047.0f) + .5f)][1];
            bestMaxColor.m_c[2] =
              g_mode6_reduced_quant[(gint)((xh.m_c[2] * 2047.0f) + .5f)][1];
            bestMaxColor.m_c[3] =
              g_mode6_reduced_quant[(gint)((xh.m_c[3] * 2047.0f) + .5f)][1];
          }
        else
          {
            gfloat best_err0 = 1e+9;
            gfloat best_err1 = 1e+9;

            for (int p = 0; p < 2; p++)
              {
                color_rgba xMinColor;
                color_rgba xMaxColor;
                color_rgba scaledLow;
                color_rgba scaledHigh;
                gfloat err0 = 0;
                gfloat err1 = 0;

                /* Notes: The pbit controls which quantization intervals are
                 * selected.
                 * total_levels=2^(comp_bits+1), where comp_bits=4 for mode 0.
                 * pbit 0: v=(b*2)/(total_levels-1),
                 * pbit 1: v=(b*2+1)/(total_levels-1) where b is the component
                 * bin from [0,total_levels/2-1] and v is the [0,1] component
                 * value rearranging you get for pbit 0:
                 * b=floor(v*(total_levels-1)/2+.5)
                 * rearranging you get for pbit 1:
                 * b=floor((v*(total_levels-1)-1)/2+.5) */
                if (pParams->m_comp_bits == 5)
                  {
                    for (gint c = 0; c < 4; c++)
                      {
                        gint vl = (gint) (xl.m_c[c] * 31.0f);
                        gint vh = (gint) (xh.m_c[c] * 31.0f);

                        vl += (xl.m_c[c] > g_mode7_rgba_midpoints[vl][p]);
                        xMinColor.m_c[c] = (guchar) CLAMP (vl * 2 + p, p, 63 - 1 + p);

                        vh += (xh.m_c[c] > g_mode7_rgba_midpoints[vh][p]);
                        xMaxColor.m_c[c] = (guchar) CLAMP (vh * 2 + p, p, 63 - 1 + p);
                      }
                  }
                else
                  {
                    for (gint c = 0; c < 4; c++)
                      {
                        xMinColor.m_c[c] =
                          (guchar) (CLAMP (((gint) ((xl.m_c[c] * scalep - p) / 2.0f + .5f)) * 2 + p,
                                           p, iscalep - 1 + p));
                        xMaxColor.m_c[c] =
                          (guchar) (CLAMP (((gint) ((xh.m_c[c] * scalep - p) / 2.0f + .5f)) * 2 + p,
                                           p, iscalep - 1 + p));
                      }
                  }

                scaledLow  = scale_color (&xMinColor, pParams);
                scaledHigh = scale_color (&xMaxColor, pParams);

                for (gint i = 0; i < totalComps; i++)
                  {
                    err0 += sqrtf (scaledLow.m_c[i] - xl.m_c[i] * 255.0f);
                    err1 += sqrtf (scaledHigh.m_c[i] - xh.m_c[i] * 255.0f);
                  }

                if (p == 1)
                  {
                    err0 *= pComp_params->m_pbit1_weight;
                    err1 *= pComp_params->m_pbit1_weight;
                  }

                if (err0 < best_err0)
                  {
                    best_err0 = err0;
                    best_pbits[0] = p;

                    bestMinColor.m_c[0] = xMinColor.m_c[0] >> 1;
                    bestMinColor.m_c[1] = xMinColor.m_c[1] >> 1;
                    bestMinColor.m_c[2] = xMinColor.m_c[2] >> 1;
                    bestMinColor.m_c[3] = xMinColor.m_c[3] >> 1;
                  }

                if (err1 < best_err1)
                  {
                    best_err1 = err1;
                    best_pbits[1] = p;

                    bestMaxColor.m_c[0] = xMaxColor.m_c[0] >> 1;
                    bestMaxColor.m_c[1] = xMaxColor.m_c[1] >> 1;
                    bestMaxColor.m_c[2] = xMaxColor.m_c[2] >> 1;
                    bestMaxColor.m_c[3] = xMaxColor.m_c[3] >> 1;
                  }
              }
          }
      }
      else
        {
          if ((mode == 1) && (pComp_params->m_bias_mode1_pbits))
            {
              color_rgba xMinColor;
              color_rgba xMaxColor;
              gfloat     x = 0.0f;
              gint       p = 0;

              for (gint c = 0; c < 3; c++)
                x = MAX (MAX (x, xl.m_c[c]), xh.m_c[c]);

              if (x > (253.0f / 255.0f))
                p = 1;

              for (gint c = 0; c < 4; c++)
                {
                  gint vl = (gint) (xl.m_c[c] * 63.0f);
                  gint vh = (gint) (xh.m_c[c] * 63.0f);

                  vl += (xl.m_c[c] > g_mode1_rgba_midpoints[vl][p]);
                  xMinColor.m_c[c] = (guchar) CLAMP (vl * 2 + p, p, 127 - 1 + p);

                  vh += (xh.m_c[c] > g_mode1_rgba_midpoints[vh][p]);
                  xMaxColor.m_c[c] = (guchar) CLAMP (vh * 2 + p, p, 127 - 1 + p);
                }

              best_pbits[0] = p;
              best_pbits[1] = p;
              for (gint j = 0; j < 4; j++)
                {
                  bestMinColor.m_c[j] = xMinColor.m_c[j] >> 1;
                  bestMaxColor.m_c[j] = xMaxColor.m_c[j] >> 1;
                }
            }
          else
            {
              /* Endpoints share pbits */
              gfloat best_err = 1e+9;

              for (int p = 0; p < 2; p++)
                {
                  color_rgba xMinColor;
                  color_rgba xMaxColor;
                  color_rgba scaledLow;
                  color_rgba scaledHigh;
                  gfloat     err = 0;

                  if (pParams->m_comp_bits == 6)
                    {
                      for (gint c = 0; c < 4; c++)
                        {
                          gint vl = (gint) (xl.m_c[c] * 63.0f);
                          gint vh = (gint) (xh.m_c[c] * 63.0f);

                          vl += (xl.m_c[c] > g_mode1_rgba_midpoints[vl][p]);
                          xMinColor.m_c[c] = (guchar) CLAMP (vl * 2 + p, p, 127 - 1 + p);

                          vh += (xh.m_c[c] > g_mode1_rgba_midpoints[vh][p]);
                          xMaxColor.m_c[c] = (guchar) CLAMP (vh * 2 + p, p, 127 - 1 + p);
                        }
                    }
                  else
                    {
                      for (gint c = 0; c < 4; c++)
                        {
                          xMinColor.m_c[c] =
                            (guchar) (CLAMP (((gint)((xl.m_c[c] * scalep - p) / 2.0f + .5f)) * 2 + p,
                                             p, iscalep - 1 + p));
                          xMaxColor.m_c[c] =
                            (guchar) (CLAMP (((gint)((xh.m_c[c] * scalep - p) / 2.0f + .5f)) * 2 + p,
                                             p, iscalep - 1 + p));
                        }
                    }

                  scaledLow  = scale_color (&xMinColor, pParams);
                  scaledHigh = scale_color (&xMaxColor, pParams);

                  for (gint i = 0; i < totalComps; i++)
                    err += sqrtf ((scaledLow.m_c[i] / 255.0f) - xl.m_c[i]) +
                           sqrtf ((scaledHigh.m_c[i] / 255.0f) - xh.m_c[i]);

                  if (p == 1)
                    err *= pComp_params->m_pbit1_weight;

                  if (err < best_err)
                    {
                      best_err = err;
                      best_pbits[0] = p;
                      best_pbits[1] = p;
                      for (gint j = 0; j < 4; j++)
                        {
                          bestMinColor.m_c[j] = xMinColor.m_c[j] >> 1;
                          bestMaxColor.m_c[j] = xMaxColor.m_c[j] >> 1;
                        }
                    }
                }
            }
        }

      fixDegenerateEndpoints (mode, &bestMinColor, &bestMaxColor, &xl, &xh,
                              iscalep >> 1, pComp_params);

      if ((pResults->m_best_overall_err == G_MAXUINT64)                       ||
          color_quad_u8_notequals (&bestMinColor, &pResults->m_low_endpoint)  ||
          color_quad_u8_notequals (&bestMaxColor, &pResults->m_high_endpoint) ||
          (best_pbits[0] != pResults->m_pbits[0])                             ||
          (best_pbits[1] != pResults->m_pbits[1]))
        evaluate_solution (&bestMinColor, &bestMaxColor, best_pbits, pParams,
                           pResults, pComp_params);
    }
  else
    {
      const gint   iscale  = (1 << pParams->m_comp_bits) - 1;
      const gfloat scale   = (gfloat) iscale;
      color_rgba   trialMinColor;
      color_rgba   trialMaxColor;

      if (pParams->m_comp_bits == 7)
        {
          for (gint c = 0; c < 4; c++)
            {
              gint vl = (gint) (xl.m_c[c] * 127.0f);
              gint vh = (gint)( xh.m_c[c] * 127.0f);

              vl += (xl.m_c[c] > g_mode5_rgba_midpoints[vl]);
              trialMinColor.m_c[c] = (guchar) CLAMP (vl, 0, 127);

              vh += (xh.m_c[c] > g_mode5_rgba_midpoints[vh]);
              trialMaxColor.m_c[c] = (guchar) CLAMP (vh, 0, 127);
            }
        }
      else
        {
          color_quad_u8_set_clamped (&trialMinColor,
                                     (gint) (xl.m_c[0] * scale + 0.5f),
                                     (gint) (xl.m_c[1] * scale + 0.5f),
                                     (gint) (xl.m_c[2] * scale + 0.5f),
                                     (gint) (xl.m_c[3] * scale + 0.5f));
          color_quad_u8_set_clamped (&trialMaxColor,
                                     (gint) (xh.m_c[0] * scale + 0.5f),
                                     (gint) (xh.m_c[1] * scale + 0.5f),
                                     (gint) (xh.m_c[2] * scale + 0.5f),
                                     (gint) (xh.m_c[3] * scale + 0.5f));
        }

      fixDegenerateEndpoints (mode, &trialMinColor, &trialMaxColor, &xl, &xh,
                              iscale, pComp_params);

      if ((pResults->m_best_overall_err == G_MAXUINT64)                       ||
          color_quad_u8_notequals (&trialMinColor, &pResults->m_low_endpoint) ||
          color_quad_u8_notequals (&trialMaxColor, &pResults->m_high_endpoint))
        evaluate_solution (&trialMinColor, &trialMaxColor, pResults->m_pbits,
                           pParams, pResults, pComp_params);
    }

  return pResults->m_best_overall_err;
}

static void
fixDegenerateEndpoints (guint32           mode,
                        color_rgba       *pTrialMinColor,
                        color_rgba       *pTrialMaxColor,
                        const vec4F      *pXl,
                        const vec4F      *pXh,
                        guint32           iscale,
                        const bc7_params *pComp_params)
{
  if ((mode == 1) ||
      ((mode == 6) && (pComp_params->m_quant_mode6_endpoints)))
    {
      for (gint i = 0; i < 3; i++)
        {
          if (pTrialMinColor->m_c[i] == pTrialMaxColor->m_c[i])
            {
              if (fabs (pXl->m_c[i] - pXh->m_c[i]) > 0.0f)
                {
                  if (pTrialMinColor->m_c[i] > (iscale >> 1))
                    {
                      if (pTrialMinColor->m_c[i] > 0)
                        pTrialMinColor->m_c[i]--;
                      else if (pTrialMaxColor->m_c[i] < iscale)
                          pTrialMaxColor->m_c[i]++;
                    }
                  else
                    {
                      if (pTrialMaxColor->m_c[i] < iscale)
                        pTrialMaxColor->m_c[i]++;
                      else if (pTrialMinColor->m_c[i] > 0)
                        pTrialMinColor->m_c[i]--;
                    }
                }
            }
        }
    }
}

static guint64
evaluate_solution (const color_rgba                   *pLow,
                   const color_rgba                   *pHigh,
                   const guint32                       pbits[2],
                   const color_cell_compressor_params *pParams,
                   color_cell_compressor_results      *pResults,
                   const bc7_params                   *pComp_params)
{
  const guint32 N  = pParams->m_num_selector_weights;
  const guint32 nc = pParams->m_has_alpha ? 4 : 3;
  color_rgba    quantMinColor;
  color_rgba    quantMaxColor;
  color_rgba    actualMinColor;
  color_rgba    actualMaxColor;
  color_rgba    weightedColors[16];
  gint          lr;
  gint          lg;
  gint          lb;
  gint          dr;
  gint          dg;
  gint          db;
  guint64       total_err = 0;

  quantMinColor = *pLow;
  quantMaxColor = *pHigh;

  if (pParams->m_has_pbits)
    {
      guint32 minPBit, maxPBit;

      if (pParams->m_endpoints_share_pbit)
        {
          maxPBit = minPBit = pbits[0];
        }
      else
        {
          minPBit = pbits[0];
          maxPBit = pbits[1];
        }

      quantMinColor.m_c[0] = (guchar) ((pLow->m_c[0] << 1) | minPBit);
      quantMinColor.m_c[1] = (guchar) ((pLow->m_c[1] << 1) | minPBit);
      quantMinColor.m_c[2] = (guchar) ((pLow->m_c[2] << 1) | minPBit);
      quantMinColor.m_c[3] = (guchar) ((pLow->m_c[3] << 1) | minPBit);

      quantMaxColor.m_c[0] = (guchar) ((pHigh->m_c[0] << 1) | maxPBit);
      quantMaxColor.m_c[1] = (guchar) ((pHigh->m_c[1] << 1) | maxPBit);
      quantMaxColor.m_c[2] = (guchar) ((pHigh->m_c[2] << 1) | maxPBit);
      quantMaxColor.m_c[3] = (guchar) ((pHigh->m_c[3] << 1) | maxPBit);
    }

  actualMinColor = scale_color (&quantMinColor, pParams);
  actualMaxColor = scale_color (&quantMaxColor, pParams);

  weightedColors[0]     = actualMinColor;
  weightedColors[N - 1] = actualMaxColor;

  for (guint32 i = 1; i < (N - 1); i++)
    for (guint32 j = 0; j < nc; j++)
      weightedColors[i].m_c[j] =
        (guchar) ((actualMinColor.m_c[j] * (64 - pParams->m_pSelector_weights[i]) +
                   actualMaxColor.m_c[j] * pParams->m_pSelector_weights[i] + 32) >> 6);

  lr = actualMinColor.m_c[0];
  lg = actualMinColor.m_c[1];
  lb = actualMinColor.m_c[2];
  dr = actualMaxColor.m_c[0] - lr;
  dg = actualMaxColor.m_c[1] - lg;
  db = actualMaxColor.m_c[2] - lb;

  if (pComp_params->m_force_selectors)
    {
      for (guint i = 0; i < pParams->m_num_pixels; i++)
        {
          const guint32 best_sel = pComp_params->m_selectors[i];
          guint64       best_err;

          if (pParams->m_has_alpha)
            best_err = compute_color_distance_rgba (&weightedColors[best_sel],
                                                    &pParams->m_pPixels[i],
                                                    pParams->m_perceptual,
                                                    pParams->m_weights);
          else
            best_err = compute_color_distance_rgb (&weightedColors[best_sel],
                                                   &pParams->m_pPixels[i],
                                                   pParams->m_perceptual,
                                                   pParams->m_weights);

          total_err += best_err;

          pResults->m_pSelectors_temp[i] = (guchar) best_sel;
        }
    }
  else if (! pParams->m_perceptual)
    {
      if (pParams->m_has_alpha)
        {
          const gint la = actualMinColor.m_c[3];
          const gint da = actualMaxColor.m_c[3] - la;

          const gfloat f =
            N / (gfloat) (pow (dr, 2) + pow (dg, 2) + pow (db, 2) + pow (da, 2) +
                          0.00000125f);

          for (gint i = 0; i < pParams->m_num_pixels; i++)
            {
              const color_rgba *pC = &pParams->m_pPixels[i];
              gint              r  = pC->m_c[0];
              gint              g  = pC->m_c[1];
              gint              b  = pC->m_c[2];
              gint              a  = pC->m_c[3];
              guint64           err0;
              guint64           err1;
              gint              best_sel;

              best_sel = (gint) ((gfloat) ((r - lr) * dr + (g - lg) * dg +
                                           (b - lb) * db + (a - la) * da) *
                                           f + .5f);
              best_sel = CLAMP (best_sel, 1, N - 1);

              err0 = compute_color_distance_rgba (&weightedColors[best_sel - 1],
                                                  pC, FALSE, pParams->m_weights);
              err1 = compute_color_distance_rgba (&weightedColors[best_sel],
                                                  pC, FALSE, pParams->m_weights);

              if (err1 > err0)
                {
                  err1 = err0;
                  --best_sel;
                }
              total_err += err1;

              pResults->m_pSelectors_temp[i] = (guchar) best_sel;
            }
        }
      else
        {
          const gfloat f =
            N / (gfloat) (pow (dr, 2) + pow (dg, 2) + pow (db, 2) + 0.00000125f);

          for (guint32 i = 0; i < pParams->m_num_pixels; i++)
            {
              const color_rgba *pC = &pParams->m_pPixels[i];
              gint              r  = pC->m_c[0];
              gint              g  = pC->m_c[1];
              gint              b  = pC->m_c[2];
              guint64           err0;
              guint64           err1;
              gint              sel;
              gint              best_sel;
              guint64           best_err;

              sel = (gint) ((gfloat) ((r - lr) * dr + (g - lg) * dg +
                                      (b - lb) * db) * f + 0.5f);
              sel = CLAMP (sel, 1, N - 1);

              err0 = compute_color_distance_rgb (&weightedColors[sel - 1],
                                                 pC, FALSE, pParams->m_weights);
              err1 = compute_color_distance_rgb (&weightedColors[sel], pC,
                                                 FALSE, pParams->m_weights);

              best_sel = sel;
              best_err = err1;
              if (err0 < best_err)
                {
                  best_err = err0;
                  best_sel = sel - 1;
                }

              total_err += best_err;

              pResults->m_pSelectors_temp[i] = (guchar) best_sel;
            }
        }
    }
  else
    {
      for (guint32 i = 0; i < pParams->m_num_pixels; i++)
        {
          guint64 best_err = G_MAXUINT64;
          guint32 best_sel = 0;

          if (pParams->m_has_alpha)
            {
              for (guint32 j = 0; j < N; j++)
                {
                  guint64 err =
                    compute_color_distance_rgba (&weightedColors[j],
                                                 &pParams->m_pPixels[i],
                                                 TRUE, pParams->m_weights);

                  if (err < best_err)
                    {
                      best_err = err;
                      best_sel = j;
                    }
                }
            }
          else
            {
              for (guint32 j = 0; j < N; j++)
                {
                  guint64 err =
                    compute_color_distance_rgb (&weightedColors[j],
                                                &pParams->m_pPixels[i],
                                                TRUE, pParams->m_weights);

                  if (err < best_err)
                    {
                      best_err = err;
                      best_sel = j;
                    }
                }
            }
          total_err += best_err;

          pResults->m_pSelectors_temp[i] = (guchar)best_sel;
        }
    }

  if (total_err < pResults->m_best_overall_err)
    {
      pResults->m_best_overall_err = total_err;

      pResults->m_low_endpoint  = *pLow;
      pResults->m_high_endpoint = *pHigh;

      pResults->m_pbits[0] = pbits[0];
      pResults->m_pbits[1] = pbits[1];

      memcpy (pResults->m_pSelectors, pResults->m_pSelectors_temp,
              sizeof (pResults->m_pSelectors[0]) * pParams->m_num_pixels);
    }

  return total_err;
}

/* Estimate the partition used by modes 1/7. This scans through each partition
 * and computes an approximate error for each. */
static guint32
estimate_partition (const color_rgba *pPixels,
                    const bc7_params *pComp_params,
                    guint32           pweights[4],
                    guint32           mode)
{
  const guint32 total_partitions   = MIN (pComp_params->m_max_partitions, 64);
  guint64       best_err           = G_MAXUINT64;
  guint32       best_partition     = 0;
  gint          best_key_partition = 0;

  /* Partition order sorted by usage frequency across a large test corpus.
   * Pattern 34 (checkerboard) must appear in slot 34.
   * Using a sorted order allows the user to decrease the # of partitions to
   * scan with minimal loss in quality. */
  static const guchar s_sorted_partition_order[64] =
    {
      1 - 1, 14 - 1, 2 - 1, 3 - 1, 16 - 1, 15 - 1, 11 - 1, 17 - 1,
      4 - 1, 24 - 1, 27 - 1, 7 - 1, 8 - 1, 22 - 1, 20 - 1, 30 - 1,
      9 - 1, 5 - 1, 10 - 1, 21 - 1, 6 - 1, 32 - 1, 23 - 1, 18 - 1,
      19 - 1, 12 - 1, 13 - 1, 31 - 1, 25 - 1, 26 - 1, 29 - 1, 28 - 1,
      33 - 1, 34 - 1, 35 - 1, 46 - 1, 47 - 1, 52 - 1, 50 - 1, 51 - 1,
      49 - 1, 39 - 1, 40 - 1, 38 - 1, 54 - 1, 53 - 1, 55 - 1, 37 - 1,
      58 - 1, 59 - 1, 56 - 1, 42 - 1, 41 - 1, 43 - 1, 44 - 1, 60 - 1,
      45 - 1, 57 - 1, 48 - 1, 36 - 1, 61 - 1, 64 - 1, 63 - 1, 62 - 1
    };

  if (total_partitions <= 1)
    return 0;

  for (guint32 partition_iter = 0;
       (partition_iter < total_partitions) && (best_err > 0);
       partition_iter++)
    {
      const guint32  partition = s_sorted_partition_order[partition_iter];
      guint64        total_subset_err       = 0;
      guint32        subset_total_colors[2] = { 0, 0 };
      color_rgba     subset_colors[2][16];
      const guchar  *pPartition;

      /* Check to see if we should bother evaluating this partition at all,
       * depending on the best partition found from the first 14. */
      if (pComp_params->m_mode17_partition_estimation_filterbank)
        {
          if ((partition_iter >= 14) && (partition_iter <= 34))
            {
              const guint32 best_key_partition_bitmask =
                1 << (best_key_partition + 1);

              if ((g_partition_predictors[partition] &
                   best_key_partition_bitmask) == 0)
                {
                  if (partition_iter == 34)
                    break;

                  continue;
                }
            }
        }

      pPartition = &partition_table[0][partition][0];
      for (gint index = 0; index < 16; index++)
        subset_colors[pPartition[index]][subset_total_colors[pPartition[index]]++] =
          pPixels[index];

      for (guint32 subset = 0;
           (subset < 2) && (total_subset_err < best_err);
           subset++)
      {
        if (mode == 7)
          total_subset_err +=
            color_cell_compression_est_mode7 (subset_total_colors[subset],
                                              &subset_colors[subset][0],
                                              pComp_params->m_perceptual,
                                              pweights,
                                              best_err);
        else
          total_subset_err +=
            color_cell_compression_est_mode1 (subset_total_colors[subset],
                                              &subset_colors[subset][0],
                                              pComp_params->m_perceptual,
                                              pweights,
                                              best_err);
      }

      if (partition < 16)
        total_subset_err =
          (guint64) ((gdouble) total_subset_err *
                     pComp_params->m_low_frequency_partition_weight + .5f);

      if (total_subset_err < best_err)
        {
          best_err = total_subset_err;
          best_partition = partition;
        }

      /* If the checkerboard pattern doesn't get the highest ranking vs.
       * the previous (lower frequency) patterns, then just stop now because
       * statistically the subsequent patterns won't do well either. */
      if ((partition == 34) && (best_partition != 34))
        break;

      if (partition_iter == 13)
        best_key_partition = best_partition;

    }

  return best_partition;
}

static guint64
color_cell_compression_est_mode1 (guint32           num_pixels,
                                  const color_rgba *pPixels,
                                  gboolean          perceptual,
                                  guint32           pweights[4],
                                  guint64           best_err_so_far)
{
  /* Find RGB bounds as an approximation of the block's principle axis */
  guint32 lr = 255, lg = 255, lb = 255;
  guint32 hr = 0, hg = 0, hb = 0;
  color_rgba  lowColor;
  color_rgba  highColor;
  const       guint32 N = 8;
  color_rgba  weightedColors[N];
  gint        ar;
  gint        ag;
  gint        ab;
  gint        dots[N];
  gint        thresh[N - 1];
  guint64     total_err = 0;

  for (guint32 i = 0; i < num_pixels; i++)
    {
      const color_rgba *pC = &pPixels[i];

      if (pC->m_c[0] < lr)
        lr = pC->m_c[0];
      if (pC->m_c[1] < lg)
        lg = pC->m_c[1];
      if (pC->m_c[2] < lb)
        lb = pC->m_c[2];

      if (pC->m_c[0] > hr)
        hr = pC->m_c[0];
      if (pC->m_c[1] > hg)
        hg = pC->m_c[1];
      if (pC->m_c[2] > hb)
        hb = pC->m_c[2];
    }

  color_quad_u8_set (&lowColor, lr, lg, lb, 0);
  color_quad_u8_set (&highColor, hr, hg, hb, 0);

  /* Place endpoints at bbox diagonals and compute interpolated colors */
  weightedColors[0]     = lowColor;
  weightedColors[N - 1] = highColor;

  for (guint32 i = 1; i < (N - 1); i++)
    {
      weightedColors[i].m_c[0] =
        (guchar) ((lowColor.m_c[0] * (64 - weight_3[i]) +
                   highColor.m_c[0] * weight_3[i] + 32) >> 6);
      weightedColors[i].m_c[1]  =
        (guchar) ((lowColor.m_c[1] * (64 - weight_3[i]) +
                   highColor.m_c[1] * weight_3[i] + 32) >> 6);
      weightedColors[i].m_c[2]  =
        (guchar) ((lowColor.m_c[2] * (64 - weight_3[i]) +
                   highColor.m_c[2] * weight_3[i] + 32) >> 6);
    }

  /* Compute dots and thresholds */
  ar = highColor.m_c[0] - lowColor.m_c[0];
  ag = highColor.m_c[1] - lowColor.m_c[1];
  ab = highColor.m_c[2] - lowColor.m_c[2];

  for (gint i = 0; i < N; i++)
    dots[i] = weightedColors[i].m_c[0] * ar + weightedColors[i].m_c[1] * ag +
              weightedColors[i].m_c[2] * ab;

  for (gint i = 0; i < (N - 1); i++)
    thresh[i] = (dots[i] + dots[i + 1] + 1) >> 1;

  if (perceptual)
    {
      /* Transform block's interpolated colors to YCbCr */
      gint l1[8], cr1[8], cb1[8];

      for (gint j = 0; j < N; j++)
        {
          const color_rgba *pE1 = &weightedColors[j];

          l1[j]  = pE1->m_c[0] * 109 + pE1->m_c[1] * 366 + pE1->m_c[2] * 37;
          cr1[j] = ((gint) pE1->m_c[0] << 9) - l1[j];
          cb1[j] = ((gint) pE1->m_c[2] << 9) - l1[j];
        }

      for (guint i = 0; i < num_pixels; i++)
        {
          const color_rgba *pC = &pPixels[i];
          gint              l2;
          gint              cr2;
          gint              cb2;
          gint              dl;
          gint              dcr;
          gint              dcb;
          gint              ie;
          guint32           s = 0;
          gint              d;

          d = ar * pC->m_c[0] + ag * pC->m_c[1] + ab * pC->m_c[2];

          /* Find approximate selector */
          if (d >= thresh[6])
            s = 7;
          else if (d >= thresh[5])
            s = 6;
          else if (d >= thresh[4])
            s = 5;
          else if (d >= thresh[3])
            s = 4;
          else if (d >= thresh[2])
            s = 3;
          else if (d >= thresh[1])
            s = 2;
          else if (d >= thresh[0])
            s = 1;

          /* Compute error */
          l2  = pC->m_c[0] * 109 + pC->m_c[1] * 366 + pC->m_c[2] * 37;
          cr2 = ((gint) pC->m_c[0] << 9) - l2;
          cb2 = ((gint) pC->m_c[2] << 9) - l2;

          dl  = (l1[s] - l2) >> 8;
          dcr = (cr1[s] - cr2) >> 8;
          dcb = (cb1[s] - cb2) >> 8;

          ie = (pweights[0] * dl * dl) + (pweights[1] * dcr * dcr) +
               (pweights[2] * dcb * dcb);

          total_err += ie;
          if (total_err > best_err_so_far)
            break;
        }
    }
  else
    {
      for (guint32 i = 0; i < num_pixels; i++)
        {
          const color_rgba *pC = &pPixels[i];
          guint32           s  = 0;
          const color_rgba *pE1;
          gint              dr;
          gint              dg;
          gint              db;
          gint              d;

          d = ar * pC->m_c[0] + ag * pC->m_c[1] + ab * pC->m_c[2];

          /* Find approximate selector */
          if (d >= thresh[6])
            s = 7;
          else if (d >= thresh[5])
            s = 6;
          else if (d >= thresh[4])
            s = 5;
          else if (d >= thresh[3])
            s = 4;
          else if (d >= thresh[2])
            s = 3;
          else if (d >= thresh[1])
            s = 2;
          else if (d >= thresh[0])
            s = 1;

          /* Compute error */
          pE1 = &weightedColors[s];

          dr = (gint) pE1->m_c[0] - (gint) pC->m_c[0];
          dg = (gint) pE1->m_c[1] - (gint) pC->m_c[1];
          db = (gint) pE1->m_c[2] - (gint) pC->m_c[2];

          total_err += pweights[0] * (dr * dr) + pweights[1] * (dg * dg) +
                       pweights[2] * (db * db);
          if (total_err > best_err_so_far)
            break;
        }
    }

  return total_err;
}

static guint64
color_cell_compression_est_mode7 (guint32           num_pixels,
                                  const color_rgba *pPixels,
                                  gboolean          perceptual,
                                  guint32           pweights[4],
                                  guint64           best_err_so_far)
{
  /* Find RGB bounds as an approximation of the block's principle axis */
  guint32       lr = 255, lg = 255, lb = 255, la = 255;
  guint32       hr = 0, hg = 0, hb = 0, ha = 0;
  color_rgba    lowColor;
  color_rgba    highColor;
  const guint32 N = 4;
  color_rgba    weightedColors[N];
  gint          ar;
  gint          ag;
  gint          ab;
  gint          aa;
  gint          dots[N];
  gint          thresh[N - 1];
  guint64       total_err = 0;

  for (guint32 i = 0; i < num_pixels; i++)
    {
      const color_rgba *pC = &pPixels[i];

      if (pC->m_c[0] < lr)
        lr = pC->m_c[0];
      if (pC->m_c[1] < lg)
        lg = pC->m_c[1];
      if (pC->m_c[2] < lb)
        lb = pC->m_c[2];
      if (pC->m_c[3] < la)
        lb = pC->m_c[3];

      if (pC->m_c[0] > hr)
        hr = pC->m_c[0];
      if (pC->m_c[1] > hg)
        hg = pC->m_c[1];
      if (pC->m_c[2] > hb)
        hb = pC->m_c[2];
      if (pC->m_c[3] > ha)
        ha = pC->m_c[3];
    }
  color_quad_u8_set (&lowColor, lr, lg, lb, la);
  color_quad_u8_set (&highColor, hr, hg, hb, ha);

  /* Place endpoints at bbox diagonals and compute interpolated colors */
  weightedColors[0]     = lowColor;
  weightedColors[N - 1] = highColor;

  for (gint i = 1; i < (N - 1); i++)
    {
      weightedColors[i].m_c[0] =
        (guchar) ((lowColor.m_c[0] * (64 - weight_2[i]) +
                   highColor.m_c[0] * weight_2[i] + 32) >> 6);
      weightedColors[i].m_c[1] =
        (guchar) ((lowColor.m_c[1] * (64 - weight_2[i]) +
                   highColor.m_c[1] * weight_2[i] + 32) >> 6);
      weightedColors[i].m_c[2] =
        (guchar) ((lowColor.m_c[2] * (64 - weight_2[i]) +
                   highColor.m_c[2] * weight_2[i] + 32) >> 6);
      weightedColors[i].m_c[3] =
        (guchar) ((lowColor.m_c[3] * (64 - weight_2[i]) +
                   highColor.m_c[3] * weight_2[i] + 32) >> 6);
    }

  /* Compute dots and thresholds */
  ar = highColor.m_c[0] - lowColor.m_c[0];
  ag = highColor.m_c[1] - lowColor.m_c[1];
  ab = highColor.m_c[2] - lowColor.m_c[2];
  aa = highColor.m_c[3] - lowColor.m_c[3];

  for (gint i = 0; i < N; i++)
    dots[i] = weightedColors[i].m_c[0] * ar + weightedColors[i].m_c[1] * ag +
              weightedColors[i].m_c[2] * ab + weightedColors[i].m_c[3] * aa;

  for (gint i = 0; i < (N - 1); i++)
    thresh[i] = (dots[i] + dots[i + 1] + 1) >> 1;

  if (perceptual)
    {
      /* Transform block's interpolated colors to YCbCr */
      gint l1[N], cr1[N], cb1[N];

      for (gint j = 0; j < N; j++)
        {
          const color_rgba* pE1 = &weightedColors[j];

          l1[j]  = pE1->m_c[0] * 109 + pE1->m_c[1] * 366 + pE1->m_c[2] * 37;
          cr1[j] = ((gint) pE1->m_c[0] << 9) - l1[j];
          cb1[j] = ((gint) pE1->m_c[2] << 9) - l1[j];
        }

      for (guint32 i = 0; i < num_pixels; i++)
        {
          const color_rgba *pC = &pPixels[i];

          gint    d = ar * pPixels->m_c[0] + ag * pPixels->m_c[1] +
                      ab * pPixels->m_c[2] + aa * pPixels->m_c[3];
          /* Find approximate selector */
          guint32 s = 0;
          gint    l2;
          gint    cr2;
          gint    cb2;
          gint    dl;
          gint    dcr;
          gint    dcb;
          gint    dca;
          gint    ie;

          if (d >= thresh[2])
            s = 3;
          else if (d >= thresh[1])
            s = 2;
          else if (d >= thresh[0])
            s = 1;

          /* Compute error */
          l2  = pC->m_c[0] * 109 + pC->m_c[1] * 366 + pC->m_c[2] * 37;
          cr2 = ((gint) pC->m_c[0] << 9) - l2;
          cb2 = ((gint) pC->m_c[2] << 9) - l2;

          dl = (l1[s] - l2) >> 8;
          dcr = (cr1[s] - cr2) >> 8;
          dcb = (cb1[s] - cb2) >> 8;

          dca = (gint) pC->m_c[3] - (gint) weightedColors[s].m_c[3];

          ie = (pweights[0] * dl * dl) + (pweights[1] * dcr * dcr) +
               (pweights[2] * dcb * dcb) + (pweights[3] * dca * dca);

          total_err += ie;
          if (total_err > best_err_so_far)
            break;
        }
    }
  else
    {
      for (guint32 i = 0; i < num_pixels; i++)
        {
          const color_rgba *pC = &pPixels[i];
          const color_rgba *pE1;

          gint    d = ar * pPixels->m_c[0] + ag * pPixels->m_c[1] +
                      ab * pPixels->m_c[2] + aa * pPixels->m_c[3];
          /* Find approximate selector */
          guint32 s = 0;
          gint    dr;
          gint    dg;
          gint    db;
          gint    da;

          if (d >= thresh[2])
            s = 3;
          else if (d >= thresh[1])
            s = 2;
          else if (d >= thresh[0])
            s = 1;

          pE1 = &weightedColors[s];

          /* Compute error */
          dr = (gint) pE1->m_c[0] - (gint) pC->m_c[0];
          dg = (gint) pE1->m_c[1] - (gint) pC->m_c[1];
          db = (gint) pE1->m_c[2] - (gint) pC->m_c[2];
          da = (gint) pE1->m_c[3] - (gint) pC->m_c[3];

          total_err += pweights[0] * (dr * dr) + pweights[1] * (dg * dg) +
                       pweights[2] * (db * db) + pweights[3] * (da * da);
          if (total_err > best_err_so_far)
            break;
        }
    }

  return total_err;
}

void
encode_bc7_block (void                           *pBlock,
                  const bc7_optimization_results *pResults)
{
  const guint32  best_mode   = pResults->m_mode;
  const guint32  total_comps = (best_mode >= 4) ? 4 : 3;

  const guint32  total_subsets    = g_bc7_num_subsets[best_mode];
  const guint32  total_partitions = 1 << g_bc7_partition_bits[best_mode];
  guint32        cur_bit_ofs      = 0;
  guchar         color_selectors[16];
  guchar         alpha_selectors[16];
  guint32        pbits[3][2];
  gint           anchor[3]    = { -1, -1, -1 };
  guchar        *pBlock_bytes = (guchar *) (pBlock);
  color_rgba     low[3];
  color_rgba     high[3];
  const guchar  *pPartition;

  if (total_subsets == 1)
    pPartition = &g_bc7_partition1[0];
  else if (total_subsets == 2)
    pPartition = &partition_table[0][pResults->m_partition][0];
  else
    pPartition = &partition_table[1][pResults->m_partition][0];

  memcpy (color_selectors, pResults->m_selectors, 16);
  memcpy (alpha_selectors, pResults->m_alpha_selectors, 16);
  memcpy (low, pResults->m_low, sizeof (low));
  memcpy (high, pResults->m_high, sizeof (high));
  memcpy (pbits, pResults->m_pbits, sizeof (pbits));

  for (guint32 k = 0; k < total_subsets; k++)
    {
      guint32 anchor_index = 0;
      guint32 color_index_bits;
      guint32 num_color_indices;

      if (k)
        {
          if ((total_subsets == 3) && (k == 1))
            anchor_index = g_bc7_table_anchor_index_third_subset_1[pResults->m_partition];
          else if ((total_subsets == 3) && (k == 2))
            anchor_index = g_bc7_table_anchor_index_third_subset_2[pResults->m_partition];
          else
            anchor_index = g_bc7_table_anchor_index_second_subset[pResults->m_partition];
        }

      anchor[k] = anchor_index;

      color_index_bits =
        get_bc7_color_index_size (best_mode, pResults->m_index_selector);
      num_color_indices = 1 << color_index_bits;

      if (color_selectors[anchor_index] & (num_color_indices >> 1))
        {
          for (guint32 i = 0; i < 16; i++)
            if (pPartition[i] == k)
              color_selectors[i] = (guchar) ((num_color_indices - 1) -
                                             color_selectors[i]);

          if (get_bc7_mode_has_seperate_alpha_selectors (best_mode))
            {
              for (guint32 q = 0; q < 3; q++)
                {
                  guchar t = low[k].m_c[q];

                  low[k].m_c[q]  = high[k].m_c[q];
                  high[k].m_c[q] = t;
                }
            }
          else
            {
              color_rgba tmp;

              for (gint i = 0; i < 4; i++)
                {
                  tmp.m_c[i]     = low[k].m_c[i];
                  low[k].m_c[i]  = high[k].m_c[i];
                  high[k].m_c[i] = tmp.m_c[i];
                }
            }

          if (! g_bc7_mode_has_shared_p_bits[best_mode])
            {
              guint32 t = pbits[k][0];

              pbits[k][0] = pbits[k][1];
              pbits[k][1] = t;
            }
        }

      if (get_bc7_mode_has_seperate_alpha_selectors (best_mode))
        {
          const guint32 alpha_index_bits =
            get_bc7_alpha_index_size (best_mode, pResults->m_index_selector);
          const guint32 num_alpha_indices = 1 << alpha_index_bits;

          if (alpha_selectors[anchor_index] & (num_alpha_indices >> 1))
            {
              guchar t;

              for (guint32 i = 0; i < 16; i++)
                if (pPartition[i] == k)
                  alpha_selectors[i] = (guchar) ((num_alpha_indices - 1) -
                                                 alpha_selectors[i]);

              t = low[k].m_c[3];

              low[k].m_c[3]  = high[k].m_c[3];
              high[k].m_c[3] = t;
            }
        }
    }
  memset (pBlock_bytes, 0, 16);

  set_block_bits (pBlock_bytes, 1 << best_mode, best_mode + 1, &cur_bit_ofs);

  if ((best_mode == 4) || (best_mode == 5))
    set_block_bits (pBlock_bytes, pResults->m_rotation, 2, &cur_bit_ofs);

  if (best_mode == 4)
    set_block_bits (pBlock_bytes, pResults->m_index_selector, 1, &cur_bit_ofs);

  if (total_partitions > 1)
    set_block_bits (pBlock_bytes, pResults->m_partition,
                    (total_partitions == 64) ? 6 : 4, &cur_bit_ofs);

  for (guint32 comp = 0; comp < total_comps; comp++)
    {
      for (guint32 subset = 0; subset < total_subsets; subset++)
        {
          set_block_bits (pBlock_bytes, low[subset].m_c[comp],
                          (comp == 3) ? g_bc7_alpha_precision_table[best_mode] :
                                        g_bc7_color_precision_table[best_mode],
                          &cur_bit_ofs);
          set_block_bits (pBlock_bytes, high[subset].m_c[comp],
                          (comp == 3) ? g_bc7_alpha_precision_table[best_mode] :
                                        g_bc7_color_precision_table[best_mode],
                          &cur_bit_ofs);
        }
    }

  if (g_bc7_mode_has_p_bits[best_mode])
    {
      for (guint32 subset = 0; subset < total_subsets; subset++)
        {
          set_block_bits (pBlock_bytes, pbits[subset][0], 1, &cur_bit_ofs);

          if (!g_bc7_mode_has_shared_p_bits[best_mode])
            set_block_bits (pBlock_bytes, pbits[subset][1], 1, &cur_bit_ofs);
        }
    }

  for (guint32 y = 0; y < 4; y++)
    {
      for (guint32 x = 0; x < 4; x++)
        {
          gint idx = x + y * 4;

          guint32 n = pResults->m_index_selector ?
                      get_bc7_alpha_index_size (best_mode, pResults->m_index_selector) :
                      get_bc7_color_index_size (best_mode, pResults->m_index_selector);

          if ((idx == anchor[0]) || (idx == anchor[1]) || (idx == anchor[2]))
            n--;

          set_block_bits (pBlock_bytes, pResults->m_index_selector ?
                                        alpha_selectors[idx] :
                                        color_selectors[idx], n, &cur_bit_ofs);
        }
    }

  if (get_bc7_mode_has_seperate_alpha_selectors (best_mode))
    {
      for (guint32 y = 0; y < 4; y++)
        {
          for (guint32 x = 0; x < 4; x++)
            {
              gint idx = x + y * 4;

              guint32 n = pResults->m_index_selector ?
                          get_bc7_color_index_size(best_mode, pResults->m_index_selector) :
                          get_bc7_alpha_index_size(best_mode, pResults->m_index_selector);

              if ((idx == anchor[0]) || (idx == anchor[1]) || (idx == anchor[2]))
                n--;

              set_block_bits (pBlock_bytes, pResults->m_index_selector ?
                                            color_selectors[idx] :
                                            alpha_selectors[idx], n, &cur_bit_ofs);
            }
        }
    }
}

static void
set_block_bits (guchar  *pBytes,
                guint32  val,
                guint32  num_bits,
                guint32 *pCur_ofs)
{
  while (num_bits)
    {
      const guint32 n = MIN (8 - (*pCur_ofs & 7), num_bits);

      pBytes[*pCur_ofs >> 3] |= (guchar) (val << (*pCur_ofs & 7));
      val >>= n;
      num_bits -= n;
      *pCur_ofs += n;
    }
}

static gboolean
get_bc7_mode_has_seperate_alpha_selectors (gint mode)
{
  return (mode == 4) || (mode == 5);
}
