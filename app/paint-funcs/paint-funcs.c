/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "paint-funcs-types.h"

#include "base/pixel-processor.h"
#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "composite/gimp-composite.h"

#include "paint-funcs.h"
#include "paint-funcs-generic.h"


#define RANDOM_SEED   314159265
#define EPSILON       0.0001

#define LOG_1_255     -5.541263545    /*  log (1.0 / 255.0)  */


/*  Layer modes information  */
typedef struct _LayerMode LayerMode;
struct _LayerMode
{
  const guint   affect_alpha     : 1; /*  does the layer mode affect the
                                          alpha channel  */
  const guint   increase_opacity : 1; /*  layer mode can increase opacity */
  const guint   decrease_opacity : 1; /*  layer mode can decrease opacity */
};

static const LayerMode layer_modes[] =
  /* This must be in the same order as the
   * corresponding values in GimpLayerModeEffects.
   */
{
  { TRUE,  TRUE,  FALSE, },  /*  GIMP_NORMAL_MODE        */
  { TRUE,  TRUE,  FALSE, },  /*  GIMP_DISSOLVE_MODE      */
  { TRUE,  TRUE,  FALSE, },  /*  GIMP_BEHIND_MODE        */
  { FALSE, FALSE, FALSE, },  /*  GIMP_MULTIPLY_MODE      */
  { FALSE, FALSE, FALSE, },  /*  GIMP_SCREEN_MODE        */
  { FALSE, FALSE, FALSE, },  /*  GIMP_OVERLAY_MODE       */
  { FALSE, FALSE, FALSE, },  /*  GIMP_DIFFERENCE_MODE    */
  { FALSE, FALSE, FALSE, },  /*  GIMP_ADDITION_MODE      */
  { FALSE, FALSE, FALSE, },  /*  GIMP_SUBTRACT_MODE      */
  { FALSE, FALSE, FALSE, },  /*  GIMP_DARKEN_ONLY_MODE   */
  { FALSE, FALSE, FALSE, },  /*  GIMP_LIGHTEN_ONLY_MODE  */
  { FALSE, FALSE, FALSE, },  /*  GIMP_HUE_MODE           */
  { FALSE, FALSE, FALSE, },  /*  GIMP_SATURATION_MODE    */
  { FALSE, FALSE, FALSE, },  /*  GIMP_COLOR_MODE         */
  { FALSE, FALSE, FALSE, },  /*  GIMP_VALUE_MODE         */
  { FALSE, FALSE, FALSE, },  /*  GIMP_DIVIDE_MODE        */
  { FALSE, FALSE, FALSE, },  /*  GIMP_DODGE_MODE         */
  { FALSE, FALSE, FALSE, },  /*  GIMP_BURN_MODE          */
  { FALSE, FALSE, FALSE, },  /*  GIMP_HARDLIGHT_MODE     */
  { FALSE, FALSE, FALSE, },  /*  GIMP_SOFTLIGHT_MODE     */
  { FALSE, FALSE, FALSE, },  /*  GIMP_GRAIN_EXTRACT_MODE */
  { FALSE, FALSE, FALSE, },  /*  GIMP_GRAIN_MERGE_MODE   */
  { TRUE,  FALSE, TRUE,  },  /*  GIMP_COLOR_ERASE_MODE   */
  { TRUE,  FALSE, TRUE,  },  /*  GIMP_ERASE_MODE         */
  { TRUE,  TRUE,  TRUE,  },  /*  GIMP_REPLACE_MODE       */
  { TRUE,  TRUE,  FALSE, }   /*  GIMP_ANTI_ERASE_MODE    */
};


typedef void (* LayerModeFunc) (struct apply_layer_mode_struct *);

static const LayerModeFunc layer_mode_funcs[] =
{
  layer_normal_mode,
  layer_dissolve_mode,
  layer_behind_mode,
  layer_multiply_mode,
  layer_screen_mode,
  layer_overlay_mode,
  layer_difference_mode,
  layer_addition_mode,
  layer_subtract_mode,
  layer_darken_only_mode,
  layer_lighten_only_mode,
  layer_hue_mode,
  layer_saturation_mode,
  layer_color_mode,
  layer_value_mode,
  layer_divide_mode,
  layer_dodge_mode,
  layer_burn_mode,
  layer_hardlight_mode,
  layer_softlight_mode,
  layer_grain_extract_mode,
  layer_grain_merge_mode,
  layer_color_erase_mode,
  layer_erase_mode,
  layer_replace_mode,
  layer_anti_erase_mode
};


static const guchar  no_mask = OPAQUE_OPACITY;


/*  Local function prototypes  */

static gint *   make_curve               (gdouble         sigma_square,
                                          gint           *length);
static gdouble  cubic                    (gdouble         dx,
                                          gint            jm1,
                                          gint            j,
                                          gint            jp1,
                                          gint            jp2);
static void     apply_layer_mode_replace (const guchar   *src1,
                                          const guchar   *src2,
                                          guchar         *dest,
                                          const guchar   *mask,
                                          gint            x,
                                          gint            y,
                                          guint           opacity,
                                          guint           length,
                                          guint           bytes1,
                                          guint           bytes2,
                                          const gboolean *affect);

static inline void rotate_pointers       (guchar        **p,
                                          guint32         n);

static void
update_tile_rowhints (Tile *tile,
                      gint  ymin,
                      gint  ymax)
{
  const guchar *ptr;
  gint          bpp, ewidth;
  gint          x, y;

#ifdef HINTS_SANITY
  g_assert (tile != NULL);
#endif

  tile_allocate_rowhints (tile);

  bpp = tile_bpp (tile);
  ewidth = tile_ewidth (tile);

  switch (bpp)
    {
    case 1:
    case 3:
      for (y = ymin; y <= ymax; y++)
        tile_set_rowhint (tile, y, TILEROWHINT_OPAQUE);
      break;

    case 4:
#ifdef HINTS_SANITY
      g_assert (tile != NULL);
#endif

      ptr = tile_data_pointer (tile, 0, ymin);

#ifdef HINTS_SANITY
      g_assert (ptr != NULL);
#endif

      for (y = ymin; y <= ymax; y++)
        {
          TileRowHint hint = tile_get_rowhint (tile, y);

#ifdef HINTS_SANITY
          if (hint == TILEROWHINT_BROKEN)
            g_error ("BROKEN y=%d", y);
          if (hint == TILEROWHINT_OUTOFRANGE)
            g_error ("OOR y=%d", y);
          if (hint == TILEROWHINT_UNDEFINED)
            g_error ("UNDEFINED y=%d - bpp=%d ew=%d eh=%d",
                     y, bpp, ewidth, eheight);
#endif

#ifdef HINTS_SANITY
          if (hint == TILEROWHINT_TRANSPARENT ||
              hint == TILEROWHINT_MIXED ||
              hint == TILEROWHINT_OPAQUE)
            {
              goto next_row4;
            }

          if (hint != TILEROWHINT_UNKNOWN)
            {
              g_error ("MEGABOGUS y=%d - bpp=%d ew=%d eh=%d",
                       y, bpp, ewidth, eheight);
            }
#endif

          if (hint == TILEROWHINT_UNKNOWN)
            {
              const guchar alpha = ptr[3];

              /* row is all-opaque or all-transparent? */
              if (alpha == 0 || alpha == 255)
                {
                  if (ewidth > 1)
                    {
                      for (x = 1; x < ewidth; x++)
                        {
                          if (ptr[x * 4 + 3] != alpha)
                            {
                              tile_set_rowhint (tile, y, TILEROWHINT_MIXED);
                              goto next_row4;
                            }
                        }
                    }

                  tile_set_rowhint (tile, y,
                                    (alpha == 0) ?
                                    TILEROWHINT_TRANSPARENT :
                                    TILEROWHINT_OPAQUE);
                }
              else
                {
                  tile_set_rowhint (tile, y, TILEROWHINT_MIXED);
                }
            }

        next_row4:
          ptr += 4 * ewidth;
        }
      break;

    case 2:
#ifdef HINTS_SANITY
      g_assert (tile != NULL);
#endif

      ptr = tile_data_pointer (tile, 0, ymin);

#ifdef HINTS_SANITY
      g_assert (ptr != NULL);
#endif

      for (y = ymin; y <= ymax; y++)
        {
          TileRowHint hint = tile_get_rowhint (tile, y);

#ifdef HINTS_SANITY
          if (hint == TILEROWHINT_BROKEN)
            g_error ("BROKEN y=%d",y);
          if (hint == TILEROWHINT_OUTOFRANGE)
            g_error ("OOR y=%d",y);
          if (hint == TILEROWHINT_UNDEFINED)
            g_error ("UNDEFINED y=%d - bpp=%d ew=%d eh=%d",
                     y, bpp, ewidth, eheight);
#endif

#ifdef HINTS_SANITY
          if (hint == TILEROWHINT_TRANSPARENT ||
              hint == TILEROWHINT_MIXED ||
              hint == TILEROWHINT_OPAQUE)
            {
              goto next_row2;
            }

          if (hint != TILEROWHINT_UNKNOWN)
            {
              g_error ("MEGABOGUS y=%d - bpp=%d ew=%d eh=%d",
                       y, bpp, ewidth, eheight);
            }
#endif

          if (hint == TILEROWHINT_UNKNOWN)
            {
              const guchar alpha = ptr[1];

              /* row is all-opaque or all-transparent? */
              if (alpha == 0 || alpha == 255)
                {
                  if (ewidth > 1)
                    {
                      for (x = 1; x < ewidth; x++)
                        {
                          if (ptr[x * 2 + 1] != alpha)
                            {
                              tile_set_rowhint (tile, y, TILEROWHINT_MIXED);
                              goto next_row2;
                            }
                        }
                    }
                  tile_set_rowhint (tile, y,
                                    (alpha == 0) ?
                                    TILEROWHINT_TRANSPARENT :
                                    TILEROWHINT_OPAQUE);
                }
              else
                {
                  tile_set_rowhint (tile, y, TILEROWHINT_MIXED);
                }
            }

        next_row2:
          ptr += 2 * ewidth;
        }
      break;

    default:
      g_return_if_reached ();
      break;
    }
}


/*
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y^2)
 */

static gint *
make_curve (gdouble  sigma_square,
            gint    *length)
{
  const gdouble sigma2 = 2 * sigma_square;
  const gdouble l      = sqrt (-sigma2 * LOG_1_255);

  gint *curve;
  gint  i, n;

  n = ceil (l) * 2;
  if ((n % 2) == 0)
    n += 1;

  curve = g_new (gint, n);

  *length = n / 2;
  curve += *length;
  curve[0] = 255;

  for (i = 1; i <= *length; i++)
    {
      gint temp = (gint) (exp (- SQR (i) / sigma2) * 255);

      curve[-i] = temp;
      curve[i] = temp;
    }

  return curve;
}


static inline void
run_length_encode (const guchar *src,
                   guint        *dest,
                   guint         w,
                   guint         bytes)
{
  guint  start;
  guint  i;
  guint  j;
  guchar last;

  last = *src;
  src += bytes;
  start = 0;

  for (i = 1; i < w; i++)
    {
      if (*src != last)
        {
          for (j = start; j < i; j++)
            {
              *dest++ = (i - j);
              *dest++ = last;
            }

          start = i;
          last = *src;
        }

      src += bytes;
    }

  for (j = start; j < i; j++)
    {
      *dest++ = (i - j);
      *dest++ = last;
    }
}

/* Note: cubic function no longer clips result */
static inline gdouble
cubic (gdouble dx,
       gint    jm1,
       gint    j,
       gint    jp1,
       gint    jp2)
{
  /* Catmull-Rom - not bad */
  return (gdouble) ((( ( - jm1 + 3 * j - 3 * jp1 + jp2 ) * dx +
                       ( 2 * jm1 - 5 * j + 4 * jp1 - jp2 ) ) * dx +
                     ( - jm1 + jp1 ) ) * dx + (j + j) ) / 2.0;
}


/*********************/
/*  FUNCTIONS        */
/*********************/

void
paint_funcs_setup (void)
{
  GRand *gr;
  gint   i;

  /*  generate a table of random seeds  */
  gr = g_rand_new_with_seed (RANDOM_SEED);

  for (i = 0; i < RANDOM_TABLE_SIZE; i++)
    random_table[i] = g_rand_int (gr);

  for (i = 0; i < 256; i++)
    add_lut[i] = i;

  for (i = 256; i <= 510; i++)
    add_lut[i] = 255;

  g_rand_free (gr);
}

void
paint_funcs_free (void)
{
}

void
combine_indexed_and_indexed_pixels (const guchar   *src1,
                                    const guchar   *src2,
                                    guchar         *dest,
                                    const guchar   *mask,
                                    guint           opacity,
                                    const gboolean *affect,
                                    guint           length,
                                    guint           bytes)
{
  gint  b;
  gint  tmp;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          guchar  new_alpha = INT_MULT (*m , opacity, tmp);

          for (b = 0; b < bytes; b++)
            dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

          m++;

          src1 += bytes;
          src2 += bytes;
          dest += bytes;
        }
    }
  else
    {
      while (length --)
        {
          guchar  new_alpha = opacity;

          for (b = 0; b < bytes; b++)
            dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

          src1 += bytes;
          src2 += bytes;
          dest += bytes;
        }
    }
}


void
combine_indexed_and_indexed_a_pixels (const guchar   *src1,
                                      const guchar   *src2,
                                      guchar         *dest,
                                      const guchar   *mask,
                                      guint           opacity,
                                      const gboolean *affect,
                                      guint           length,
                                      guint           bytes)
{
  const gint alpha      = 1;
  const gint src2_bytes = 2;
  glong      tmp;
  gint       b;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          guchar new_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);

          for (b = 0; b < bytes; b++)
            dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

          m++;

          src1 += bytes;
          src2 += src2_bytes;
          dest += bytes;
        }
    }
  else
    {
      while (length --)
        {
          guchar new_alpha = INT_MULT (src2[alpha], opacity, tmp);

          for (b = 0; b < bytes; b++)
            dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

          src1 += bytes;
          src2 += src2_bytes;
          dest += bytes;
        }
    }
}


void
combine_indexed_a_and_indexed_a_pixels (const guchar   *src1,
                                        const guchar   *src2,
                                        guchar         *dest,
                                        const guchar   *mask,
                                        guint           opacity,
                                        const gboolean *affect,
                                        guint           length,
                                        guint           bytes)
{
  const gint alpha = 1;
  gint       b;
  glong      tmp;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          guchar new_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);

          for (b = 0; b < alpha; b++)
            dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

          dest[alpha] = (affect[alpha] && new_alpha > 127) ?
            OPAQUE_OPACITY : src1[alpha];

          m++;

          src1 += bytes;
          src2 += bytes;
          dest += bytes;
        }
    }
  else
    {
      while (length --)
        {
          guchar new_alpha = INT_MULT (src2[alpha], opacity, tmp);

          for (b = 0; b < alpha; b++)
            dest[b] = (affect[b] && new_alpha > 127) ? src2[b] : src1[b];

          dest[alpha] = (affect[alpha] && new_alpha > 127) ?
            OPAQUE_OPACITY : src1[alpha];

          src1 += bytes;
          src2 += bytes;
          dest += bytes;
        }
    }
}


void
combine_inten_a_and_indexed_pixels (const guchar *src1,
                                    const guchar *src2,
                                    guchar       *dest,
                                    const guchar *mask,
                                    const guchar *cmap,
                                    guint         opacity,
                                    guint         length,
                                    guint         bytes)
{
  const gint src2_bytes = 1;
  gint       b;
  glong      tmp;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          gint   index     = src2[0] * 3;
          guchar new_alpha = INT_MULT3 (255, *m, opacity, tmp);

          for (b = 0; b < bytes-1; b++)
            dest[b] = (new_alpha > 127) ? cmap[index + b] : src1[b];

          dest[b] = (new_alpha > 127) ? OPAQUE_OPACITY : src1[b];
          /*  alpha channel is opaque  */

          m++;

          src1 += bytes;
          src2 += src2_bytes;
          dest += bytes;
        }
    }
  else
    {
      while (length --)
        {
          gint   index     = src2[0] * 3;
          guchar new_alpha = INT_MULT (255, opacity, tmp);

          for (b = 0; b < bytes-1; b++)
            dest[b] = (new_alpha > 127) ? cmap[index + b] : src1[b];

          dest[b] = (new_alpha > 127) ? OPAQUE_OPACITY : src1[b];
          /*  alpha channel is opaque  */

          /* m++; /Per */

          src1 += bytes;
          src2 += src2_bytes;
          dest += bytes;
        }
    }
}


void
combine_inten_a_and_indexed_a_pixels (const guchar *src1,
                                      const guchar *src2,
                                      guchar       *dest,
                                      const guchar *mask,
                                      const guchar *cmap,
                                      guint         opacity,
                                      guint         length,
                                      guint         bytes)
{
  const gint alpha = 1;
  const gint src2_bytes = 2;
  gint       b;
  glong      tmp;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          gint   index     = src2[0] * 3;
          guchar new_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);

          for (b = 0; b < bytes-1; b++)
            dest[b] = (new_alpha > 127) ? cmap[index + b] : src1[b];

          dest[b] = (new_alpha > 127) ? OPAQUE_OPACITY : src1[b];
          /*  alpha channel is opaque  */

          m++;

          src1 += bytes;
          src2 += src2_bytes;
          dest += bytes;
        }
    }
  else
    {
      while (length --)
        {
          gint   index     = src2[0] * 3;
          guchar new_alpha = INT_MULT (src2[alpha], opacity, tmp);

          for (b = 0; b < bytes-1; b++)
            dest[b] = (new_alpha > 127) ? cmap[index + b] : src1[b];

          dest[b] = (new_alpha > 127) ? OPAQUE_OPACITY : src1[b];
          /*  alpha channel is opaque  */

          /* m++; /Per */

          src1 += bytes;
          src2 += src2_bytes;
          dest += bytes;
        }
    }
}


void
combine_inten_and_inten_pixels (const guchar   *src1,
                                const guchar   *src2,
                                guchar         *dest,
                                const guchar   *mask,
                                guint           opacity,
                                const gboolean *affect,
                                guint           length,
                                guint           bytes)
{
  gint  b;
  gint  tmp;

  if (mask)
    {
      const guchar * m = mask;

      while (length --)
        {
          guchar new_alpha = INT_MULT (*m, opacity, tmp);

          for (b = 0; b < bytes; b++)
            dest[b] = (affect[b] ?
                       INT_BLEND (src2[b], src1[b], new_alpha, tmp) : src1[b]);

          m++;

          src1 += bytes;
          src2 += bytes;
          dest += bytes;
        }
    }
  else
    {
      while (length --)
        {

          for (b = 0; b < bytes; b++)
            dest[b] = (affect[b] ?
                       INT_BLEND (src2[b], src1[b], opacity, tmp) : src1[b]);

          src1 += bytes;
          src2 += bytes;
          dest += bytes;
        }
    }
}


void
combine_inten_and_inten_a_pixels (const guchar   *src1,
                                  const guchar   *src2,
                                  guchar         *dest,
                                  const guchar   *mask,
                                  guint           opacity,
                                  const gboolean *affect,
                                  guint           length,
                                  guint           bytes)
{
  const gint      alpha      = bytes;
  const gint      src2_bytes = bytes + 1;
  gint            b;
  register glong  t1;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          guchar new_alpha = INT_MULT3 (src2[alpha], *m, opacity, t1);

          for (b = 0; b < bytes; b++)
            dest[b] = (affect[b] ?
                       INT_BLEND (src2[b], src1[b], new_alpha, t1) : src1[b]);

          m++;
          src1 += bytes;
          src2 += src2_bytes;
          dest += bytes;
        }
    }
  else
    {
      if (bytes == 3 && affect[0] && affect[1] && affect[2])
        {
          while (length --)
            {
              guchar new_alpha = INT_MULT (src2[alpha],opacity,t1);

              dest[0] = INT_BLEND (src2[0] , src1[0] , new_alpha, t1);
              dest[1] = INT_BLEND (src2[1] , src1[1] , new_alpha, t1);
              dest[2] = INT_BLEND (src2[2] , src1[2] , new_alpha, t1);

              src1 += bytes;
              src2 += src2_bytes;
              dest += bytes;
            }
        }
      else
        {
          while (length --)
            {
              guchar new_alpha = INT_MULT (src2[alpha],opacity,t1);

              for (b = 0; b < bytes; b++)
                dest[b] = (affect[b] ?
                           INT_BLEND (src2[b] , src1[b] , new_alpha, t1) :
                           src1[b]);

              src1 += bytes;
              src2 += src2_bytes;
              dest += bytes;
            }
        }
    }
}

/*orig #define alphify(src2_alpha,new_alpha) \
        if (new_alpha == 0 || src2_alpha == 0)                                                        \
          {                                                                                        \
            for (b = 0; b < alpha; b++)                                                                \
              dest[b] = src1 [b];                                                                \
          }                                                                                        \
        else if (src2_alpha == new_alpha){                                                        \
          for (b = 0; b < alpha; b++)                                                                \
            dest [b] = affect [b] ? src2 [b] : src1 [b];                                        \
        } else {                                                                                \
          ratio = (float) src2_alpha / new_alpha;                                                \
          compl_ratio = 1.0 - ratio;                                                                \
                                                                                                  \
          for (b = 0; b < alpha; b++)                                                                \
            dest[b] = affect[b] ?                                                                \
              (guchar) (src2[b] * ratio + src1[b] * compl_ratio + EPSILON) : src1[b];        \
        }*/

/*shortened #define alphify(src2_alpha,new_alpha) \
        if (src2_alpha != 0 && new_alpha != 0)                                                        \
          {                                                                                        \
            if (src2_alpha == new_alpha){                                                        \
              for (b = 0; b < alpha; b++)                                                        \
              dest [b] = affect [b] ? src2 [b] : src1 [b];                                        \
            } else {                                                                                \
              ratio = (float) src2_alpha / new_alpha;                                                \
              compl_ratio = 1.0 - ratio;                                                        \
                                                                                                  \
              for (b = 0; b < alpha; b++)                                                        \
                dest[b] = affect[b] ?                                                                \
                  (guchar) (src2[b] * ratio + src1[b] * compl_ratio + EPSILON) : src1[b];\
            }                                                                                   \
          }*/

#define alphify(src2_alpha,new_alpha) \
        if (src2_alpha != 0 && new_alpha != 0)                                                        \
          {                                                                                        \
            b = alpha; \
            if (src2_alpha == new_alpha){                                                        \
              do { \
              b--; dest [b] = affect [b] ? src2 [b] : src1 [b];} while (b);        \
            } else {                                                                                \
              ratio = (float) src2_alpha / new_alpha;                                                \
              compl_ratio = 1.0 - ratio;                                                        \
                                                                                                  \
              do { b--; \
                dest[b] = affect[b] ?                                                                \
                  (guchar) (src2[b] * ratio + src1[b] * compl_ratio + EPSILON) : src1[b];\
                   } while (b); \
            }    \
          }

/*special #define alphify4(src2_alpha,new_alpha) \
        if (src2_alpha != 0 && new_alpha != 0)                                                        \
          {                                                                                        \
            if (src2_alpha == new_alpha){                                                        \
              dest [0] = affect [0] ? src2 [0] : src1 [0];                                        \
              dest [1] = affect [1] ? src2 [1] : src1 [1];                                        \
              dest [2] = affect [2] ? src2 [2] : src1 [2];                                        \
            } else {                                                                                \
              ratio = (float) src2_alpha / new_alpha;                                                \
              compl_ratio = 1.0 - ratio;                                                        \
                                                                                                  \
              dest[0] = affect[0] ?                                                                \
                (guchar) (src2[0] * ratio + src1[0] * compl_ratio + EPSILON) : src1[0];  \
              dest[1] = affect[1] ?                                                                \
                (guchar) (src2[1] * ratio + src1[1] * compl_ratio + EPSILON) : src1[1];  \
              dest[2] = affect[2] ?                                                                \
                (guchar) (src2[2] * ratio + src1[2] * compl_ratio + EPSILON) : src1[2];  \
            }                                                                                   \
          }*/

void
combine_inten_a_and_inten_pixels (const guchar   *src1,
                                  const guchar   *src2,
                                  guchar         *dest,
                                  const guchar   *mask,
                                  guint           opacity,
                                  const gboolean *affect,
                                  gboolean        mode_affect,  /*  how does the combination mode affect alpha?  */
                                  guint           length,
                                  guint           bytes)        /*  4 or 2 depending on RGBA or GRAYA  */
{
  const gint    src2_bytes = bytes - 1;
  const gint    alpha      = bytes - 1;
  gint          b;
  gfloat        ratio;
  gfloat        compl_ratio;
  glong         tmp;

  if (mask)
    {
      const guchar *m = mask;

      if (opacity == OPAQUE_OPACITY) /* HAS MASK, FULL OPACITY */
        {
          while (length--)
            {
              guchar src2_alpha = *m;
              guchar new_alpha  =
                src1[alpha] + INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

              alphify (src2_alpha, new_alpha);

              if (mode_affect)
                {
                  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
                }
              else
                {
                  dest[alpha] = (src1[alpha]) ? src1[alpha] :
                    (affect[alpha] ? new_alpha : src1[alpha]);
                }

              m++;
              src1 += bytes;
              src2 += src2_bytes;
              dest += bytes;
            }
        }
      else /* HAS MASK, SEMI-OPACITY */
        {
          while (length--)
            {
              guchar src2_alpha = INT_MULT (*m, opacity, tmp);
              guchar new_alpha  =
                src1[alpha] + INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

              alphify (src2_alpha, new_alpha);

              if (mode_affect)
                {
                  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
                }
              else
                {
                  dest[alpha] = (src1[alpha]) ? src1[alpha] :
                    (affect[alpha] ? new_alpha : src1[alpha]);
                }

              m++;
              src1 += bytes;
              src2 += src2_bytes;
              dest += bytes;
            }
        }
    }
  else /* NO MASK */
    {
      while (length --)
        {
          guchar src2_alpha = opacity;
          guchar new_alpha  =
            src1[alpha] + INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

          alphify (src2_alpha, new_alpha);

          if (mode_affect)
            dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
          else
            dest[alpha] = (src1[alpha]) ? src1[alpha] : (affect[alpha] ? new_alpha : src1[alpha]);

            src1 += bytes;
            src2 += src2_bytes;
            dest += bytes;
        }
    }
}


void
combine_inten_a_and_inten_a_pixels (const guchar   *src1,
                                    const guchar   *src2,
                                    guchar         *dest,
                                    const guchar   *mask,
                                    guint           opacity,
                                    const gboolean *affect,
                                    gboolean        mode_affect,  /*  how does the combination mode affect alpha?  */
                                    guint           length,
                                    guint           bytes)  /*  4 or 2 depending on RGBA or GRAYA  */
{
  const guint alpha = bytes - 1;
  guint       b;
  gfloat      ratio;
  gfloat      compl_ratio;
  glong       tmp;

  if (mask)
    {
      const guchar *m = mask;

      if (opacity == OPAQUE_OPACITY) /* HAS MASK, FULL OPACITY */
        {
          const gint *mask_ip;
          gint        i, j;

          if (length >= sizeof (gint))
            {
              /* HEAD */
              i =  (GPOINTER_TO_INT(m) & (sizeof (gint) - 1));

              if (i != 0)
                {
                  i = sizeof (gint) - i;
                  length -= i;

                  while (i--)
                    {
                      /* GUTS */
                      guchar src2_alpha = INT_MULT (src2[alpha], *m, tmp);
                      guchar new_alpha  =
                        src1[alpha] +
                        INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

                      alphify (src2_alpha, new_alpha);

                      if (mode_affect)
                        {
                          dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
                        }
                      else
                        {
                          dest[alpha] = (src1[alpha]) ? src1[alpha] :
                            (affect[alpha] ? new_alpha : src1[alpha]);
                        }

                      m++;
                      src1 += bytes;
                      src2 += bytes;
                      dest += bytes;
                      /* GUTS END */
                    }
                }

              /* BODY */
              mask_ip = (const gint *) m;
              i = length / sizeof (gint);
              length %= sizeof (gint);

              while (i--)
                {
                  if (*mask_ip)
                    {
                      m = (const guchar *) mask_ip;
                      j = sizeof (gint);

                      while (j--)
                        {
                          /* GUTS */
                          guchar src2_alpha = INT_MULT (src2[alpha], *m, tmp);
                          guchar new_alpha  =
                            src1[alpha] +
                            INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

                          alphify (src2_alpha, new_alpha);

                          if (mode_affect)
                            {
                              dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
                            }
                          else
                            {
                              dest[alpha] = (src1[alpha]) ? src1[alpha] :
                                (affect[alpha] ? new_alpha : src1[alpha]);
                            }

                          m++;
                          src1 += bytes;
                          src2 += bytes;
                          dest += bytes;
                          /* GUTS END */
                        }
                    }
                  else
                    {
                      j = bytes * sizeof (gint);
                      src2 += j;
                      while (j--)
                        {
                          *(dest++) = *(src1++);
                        }
                    }
                  mask_ip++;
                }

              m = (const guchar *) mask_ip;
            }

          /* TAIL */
          while (length--)
            {
              /* GUTS */
              guchar src2_alpha = INT_MULT (src2[alpha], *m, tmp);
              guchar new_alpha  =
                src1[alpha] + INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

              alphify (src2_alpha, new_alpha);

              if (mode_affect)
                {
                  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
                }
              else
                {
                  dest[alpha] = (src1[alpha]) ? src1[alpha] :
                    (affect[alpha] ? new_alpha : src1[alpha]);
                }

              m++;
              src1 += bytes;
              src2 += bytes;
              dest += bytes;
              /* GUTS END */
            }
        }
      else /* HAS MASK, SEMI-OPACITY */
        {
          const gint *mask_ip;
          gint        i,j;

          if (length >= sizeof (gint))
            {
              /* HEAD */
              i = (GPOINTER_TO_INT(m) & (sizeof (gint) - 1));
              if (i != 0)
                {
                  i = sizeof (gint) - i;
                  length -= i;

                  while (i--)
                    {
                      /* GUTS */
                      guchar src2_alpha = INT_MULT3 (src2[alpha], *m, opacity,
                                                     tmp);
                      guchar new_alpha  =
                        src1[alpha] +
                        INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

                      alphify (src2_alpha, new_alpha);

                      if (mode_affect)
                        {
                          dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
                        }
                      else
                        {
                          dest[alpha] = (src1[alpha]) ? src1[alpha] :
                            (affect[alpha] ? new_alpha : src1[alpha]);
                        }

                      m++;
                      src1 += bytes;
                      src2 += bytes;
                      dest += bytes;
                      /* GUTS END */
                    }
                }

              /* BODY */
              mask_ip = (const gint *) m;
              i = length / sizeof (gint);
              length %= sizeof(gint);

              while (i--)
                {
                  if (*mask_ip)
                    {
                      m = (const guchar *) mask_ip;
                      j = sizeof (gint);

                     while (j--)
                        {
                          /* GUTS */
                          guchar src2_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);
                          guchar new_alpha  =
                            src1[alpha] +
                            INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

                          alphify (src2_alpha, new_alpha);

                          if (mode_affect)
                            {
                              dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
                            }
                          else
                            {
                              dest[alpha] = (src1[alpha]) ? src1[alpha] :
                                (affect[alpha] ? new_alpha : src1[alpha]);
                            }

                          m++;
                          src1 += bytes;
                          src2 += bytes;
                          dest += bytes;
                          /* GUTS END */
                        }
                    }
                  else
                    {
                      j = bytes * sizeof (gint);
                      src2 += j;

                      while (j--)
                        {
                          *(dest++) = *(src1++);
                        }
                    }
                  mask_ip++;
                }

              m = (const guchar *) mask_ip;
            }

          /* TAIL */
          while (length--)
            {
              /* GUTS */
              guchar src2_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);
              guchar new_alpha  =
                src1[alpha] +
                INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

              alphify (src2_alpha, new_alpha);

              if (mode_affect)
                {
                  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
                }
              else
                {
                  dest[alpha] = (src1[alpha]) ? src1[alpha] :
                    (affect[alpha] ? new_alpha : src1[alpha]);
                }

              m++;
              src1 += bytes;
              src2 += bytes;
              dest += bytes;
              /* GUTS END */
            }
        }
    }
  else
    {
      if (opacity == OPAQUE_OPACITY) /* NO MASK, FULL OPACITY */
        {
          while (length --)
            {
              guchar src2_alpha = src2[alpha];
              guchar new_alpha  =
                src1[alpha] +
                INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

              alphify (src2_alpha, new_alpha);

              if (mode_affect)
                {
                  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
                }
              else
                {
                  dest[alpha] = (src1[alpha]) ? src1[alpha] :
                    (affect[alpha] ? new_alpha : src1[alpha]);
                }

              src1 += bytes;
              src2 += bytes;
              dest += bytes;
            }
        }
      else /* NO MASK, SEMI OPACITY */
        {
          while (length --)
            {
              guchar src2_alpha = INT_MULT (src2[alpha], opacity, tmp);
              guchar new_alpha  =
                src1[alpha] + INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

              alphify (src2_alpha, new_alpha);

              if (mode_affect)
                {
                  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
                }
              else
                {
                  dest[alpha] = (src1[alpha]) ? src1[alpha] :
                    (affect[alpha] ? new_alpha : src1[alpha]);
                }

              src1 += bytes;
              src2 += bytes;
              dest += bytes;
            }
        }
    }
}
#undef alphify

void
combine_inten_a_and_channel_mask_pixels (const guchar *src,
                                         const guchar *channel,
                                         guchar       *dest,
                                         const guchar *col,
                                         guint         opacity,
                                         guint         length,
                                         guint         bytes)
{
  const gint alpha = bytes - 1;
  gint       b;
  gint       t, s;

  while (length --)
    {
      guchar channel_alpha = INT_MULT (255 - *channel, opacity, t);

      if (channel_alpha)
        {
          guchar compl_alpha;
          guchar new_alpha =
            src[alpha] + INT_MULT ((255 - src[alpha]), channel_alpha, t);

          if (new_alpha != 255)
            channel_alpha = (channel_alpha * 255) / new_alpha;

          compl_alpha = 255 - channel_alpha;

          for (b = 0; b < alpha; b++)
            dest[b] = INT_MULT (col[b], channel_alpha, t) +
              INT_MULT (src[b], compl_alpha, s);

          dest[b] = new_alpha;
        }
      else
        {
          memcpy(dest, src, bytes);
        }

      /*  advance pointers  */
      src+=bytes;
      dest+=bytes;
      channel++;
    }
}


void
combine_inten_a_and_channel_selection_pixels (const guchar *src,
                                              const guchar *channel,
                                              guchar       *dest,
                                              const guchar *col,
                                              guint         opacity,
                                              guint         length,
                                              guint         bytes)
{
  const gint alpha = bytes - 1;
  gint       b;
  gint       t, s;

  while (length --)
    {
      guchar channel_alpha = INT_MULT (*channel, opacity, t);

      if (channel_alpha)
        {
          guchar compl_alpha;
          guchar new_alpha =
            src[alpha] + INT_MULT ((255 - src[alpha]), channel_alpha, t);

          if (new_alpha != 255)
            channel_alpha = (channel_alpha * 255) / new_alpha;

          compl_alpha = 255 - channel_alpha;

          for (b = 0; b < alpha; b++)
            dest[b] = INT_MULT (col[b], channel_alpha, t) +
              INT_MULT (src[b], compl_alpha, s);

          dest[b] = new_alpha;
        }
      else
        memcpy(dest, src, bytes);

      /*  advance pointers  */
      src+=bytes;
      dest+=bytes;
      channel++;
    }
}


/*  paint "behind" the existing pixel row.
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */

static inline void
behind_inten_pixels (const guchar   *src1,
                     const guchar   *src2,
                     guchar         *dest,
                     const guchar   *mask,
                     guint           opacity,
                     const gboolean *affect,
                     guint           length,
                     guint           bytes1,
                     guint           bytes2)
{
  const guint   alpha = bytes1 - 1;
  const guchar *m     = mask ? mask : &no_mask;
  guint         b;
  gfloat        ratio;
  gfloat        compl_ratio;
  glong         tmp;

  while (length --)
    {
      guchar src1_alpha = src1[alpha];
      guchar src2_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);
      guchar new_alpha  =
        src2_alpha + INT_MULT ((255 - src2_alpha), src1_alpha, tmp);

      if (new_alpha)
        ratio = (float) src1_alpha / new_alpha;
      else
        ratio = 0.0;

      compl_ratio = 1.0 - ratio;

      for (b = 0; b < alpha; b++)
        dest[b] = (affect[b]) ?
          (guchar) (src1[b] * ratio + src2[b] * compl_ratio + EPSILON) :
        src1[b];

      dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];

      if (mask)
        m++;

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes1;
    }
}


/*  paint "behind" the existing pixel row (for indexed images).
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */

static inline void
behind_indexed_pixels (const guchar   *src1,
                       const guchar   *src2,
                       guchar         *dest,
                       const guchar   *mask,
                       guint           opacity,
                       const gboolean *affect,
                       guint           length,
                       guint           bytes1,
                       guint           bytes2)
{
  const guint   alpha = bytes1 - 1;
  const guchar *m     = mask ? mask : &no_mask;
  guint         b;
  glong         tmp;

  /*  the alpha channel  */

  while (length --)
    {
      guchar src1_alpha = src1[alpha];
      guchar src2_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);
      guchar new_alpha  =
        (src2_alpha > 127) ? OPAQUE_OPACITY : TRANSPARENT_OPACITY;

      for (b = 0; b < bytes1; b++)
        dest[b] =
          (affect[b] && new_alpha == OPAQUE_OPACITY && (src1_alpha > 127)) ?
          src2[b] : src1[b];

      if (mask)
        m++;

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes1;
    }
}


/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */

static inline void
replace_inten_pixels (const guchar   *src1,
                      const guchar   *src2,
                      guchar         *dest,
                      const guchar   *mask,
                      guint           opacity,
                      const gboolean *affect,
                      guint           length,
                      guint           bytes1,
                      guint           bytes2)
{
  const guint has_alpha1 = HAS_ALPHA (bytes1);
  const guint has_alpha2 = HAS_ALPHA (bytes2);
  const guint bytes      = MIN (bytes1, bytes2);
  guint b;
  gint  tmp;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          guchar mask_alpha = INT_MULT (*m, opacity, tmp);

          for (b = 0; b < bytes; b++)
            dest[b] = (affect[b]) ?
              INT_BLEND (src2[b], src1[b], mask_alpha, tmp) :
              src1[b];

          if (has_alpha1 && !has_alpha2)
            dest[b] = src1[b];

          m++;

          src1 += bytes1;
          src2 += bytes2;
          dest += bytes1;
        }
    }
  else
    {
      const guchar mask_alpha = OPAQUE_OPACITY;

      while (length --)
        {
          for (b = 0; b < bytes; b++)
            dest[b] = (affect[b]) ?
              INT_BLEND (src2[b], src1[b], mask_alpha, tmp) :
              src1[b];

          if (has_alpha1 && !has_alpha2)
            dest[b] = src1[b];

          src1 += bytes1;
          src2 += bytes2;
          dest += bytes1;
        }
    }
}

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */

static inline void
replace_indexed_pixels (const guchar   *src1,
                        const guchar   *src2,
                        guchar         *dest,
                        const guchar   *mask,
                        guint           opacity,
                        const gboolean *affect,
                        guint           length,
                        guint           bytes1,
                        guint           bytes2)
{
  const guint   has_alpha1 = HAS_ALPHA (bytes1);
  const guint   has_alpha2 = HAS_ALPHA (bytes2);
  const guint   bytes      = MIN (bytes1, bytes2);
  const guchar *m          = mask ? mask : &no_mask;
  guint         b;
  gint          tmp;

  while (length --)
    {
      guchar mask_alpha = INT_MULT (*m, opacity, tmp);

      for (b = 0; b < bytes; b++)
        dest[b] = (affect[b] && mask_alpha) ? src2[b] : src1[b];

      if (has_alpha1 && !has_alpha2)
        dest[b] = src1[b];

      if (mask)
        m++;

      src1 += bytes1;
      src2 += bytes2;
      dest += bytes1;
    }
}

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for intensity)
 */

static inline void
erase_inten_pixels (const guchar   *src1,
                    const guchar   *src2,
                    guchar         *dest,
                    const guchar   *mask,
                    guint           opacity,
                    const gboolean *affect,
                    guint           length,
                    guint           bytes)
{
  const guint alpha = bytes - 1;
  guint       b;
  guchar      src2_alpha;
  glong       tmp;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          for (b = 0; b < alpha; b++)
            dest[b] = src1[b];

          src2_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);
          dest[alpha] = src1[alpha] - INT_MULT (src1[alpha], src2_alpha, tmp);

          m++;

          src1 += bytes;
          src2 += bytes;
          dest += bytes;
        }
    }
  else
    {
      const guchar *m = &no_mask;

      while (length --)
        {
          for (b = 0; b < alpha; b++)
            dest[b] = src1[b];

          src2_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);
          dest[alpha] = src1[alpha] - INT_MULT (src1[alpha], src2_alpha, tmp);

          src1 += bytes;
          src2 += bytes;
          dest += bytes;
        }
    }
}


/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for indexed)
 */

static inline void
erase_indexed_pixels (const guchar   *src1,
                      const guchar   *src2,
                      guchar         *dest,
                      const guchar   *mask,
                      guint           opacity,
                      const gboolean *affect,
                      guint           length,
                      guint           bytes)
{
  const guint   alpha = bytes - 1;
  const guchar *m     = mask ? mask : &no_mask;
  guchar        src2_alpha;
  guint         b;
  glong         tmp;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
        dest[b] = src1[b];

      src2_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);
      dest[alpha] = (src2_alpha > 127) ? TRANSPARENT_OPACITY : src1[alpha];

      if (mask)
        m++;

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
}

static inline void
anti_erase_inten_pixels (const guchar   *src1,
                         const guchar   *src2,
                         guchar         *dest,
                         const guchar   *mask,
                         guint           opacity,
                         const gboolean *affect,
                         guint           length,
                         guint           bytes)
{
  const gint    alpha = bytes - 1;
  const guchar *m     = mask ? mask : &no_mask;
  gint          b;
  guchar        src2_alpha;
  glong         tmp;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
        dest[b] = src1[b];

      src2_alpha  = INT_MULT3 (src2[alpha], *m, opacity, tmp);
      dest[alpha] =
        src1[alpha] + INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

      if (mask)
        m++;

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
}


static inline void
anti_erase_indexed_pixels (const guchar   *src1,
                           const guchar   *src2,
                           guchar         *dest,
                           const guchar   *mask,
                           guint           opacity,
                           const gboolean *affect,
                           guint           length,
                           guint           bytes)
{
  const guint   alpha = bytes - 1;
  const guchar *m     = mask ? mask : &no_mask;
  gint          b;
  guchar        src2_alpha;
  glong         tmp;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
        dest[b] = src1[b];

      src2_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);
      dest[alpha] = (src2_alpha > 127) ? OPAQUE_OPACITY : src1[alpha];

      if (mask)
        m++;

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
}


static inline void
color_erase_helper (GimpRGB       *src,
                    const GimpRGB *color)
{
  GimpRGB alpha;

  alpha.a = src->a;

  if (color->r < 0.0001)
    alpha.r = src->r;
  else if ( src->r > color->r )
    alpha.r = (src->r - color->r) / (1.0 - color->r);
  else if (src->r < color->r)
    alpha.r = (color->r - src->r) / color->r;
  else alpha.r = 0.0;

  if (color->g < 0.0001)
    alpha.g = src->g;
  else if ( src->g > color->g )
    alpha.g = (src->g - color->g) / (1.0 - color->g);
  else if ( src->g < color->g )
    alpha.g = (color->g - src->g) / (color->g);
  else alpha.g = 0.0;

  if (color->b < 0.0001)
    alpha.b = src->b;
  else if ( src->b > color->b )
    alpha.b = (src->b - color->b) / (1.0 - color->b);
  else if ( src->b < color->b )
    alpha.b = (color->b - src->b) / (color->b);
  else alpha.b = 0.0;

  if ( alpha.r > alpha.g )
    {
      if ( alpha.r > alpha.b )
        {
          src->a = alpha.r;
        }
      else
        {
          src->a = alpha.b;
        }
    }
  else if ( alpha.g > alpha.b )
    {
      src->a = alpha.g;
    }
  else
    {
      src->a = alpha.b;
    }

  src->a = (1.0 - color->a) + (src->a * color->a);

  if (src->a < 0.0001)
    return;

  src->r = (src->r - color->r) / src->a + color->r;
  src->g = (src->g - color->g) / src->a + color->g;
  src->b = (src->b - color->b) / src->a + color->b;

  src->a *= alpha.a;
}


static inline void
color_erase_inten_pixels (const guchar   *src1,
                          const guchar   *src2,
                          guchar         *dest,
                          const guchar   *mask,
                          guint           opacity,
                          const gboolean *affect,
                          guint           length,
                          guint           bytes)
{
  const guchar *m     = mask ? mask : &no_mask;
  guchar        src2_alpha;
  glong         tmp;
  GimpRGB       bgcolor, color;

  while (length --)
    {
      switch (bytes)
        {
        case 2:
          src2_alpha = INT_MULT3 (src2[1], *m, opacity, tmp);

          gimp_rgba_set_uchar (&color,
                               src1[0], src1[0], src1[0], src1[1]);

          gimp_rgba_set_uchar (&bgcolor,
                               src2[0], src2[0], src2[0], src2_alpha);

          color_erase_helper (&color, &bgcolor);

          gimp_rgba_get_uchar (&color, dest, NULL, NULL, dest + 1);
          break;

        case 4:
          src2_alpha = INT_MULT3 (src2[3], *m, opacity, tmp);

          gimp_rgba_set_uchar (&color,
                               src1[0], src1[1], src1[2], src1[3]);

          gimp_rgba_set_uchar (&bgcolor,
                               src2[0], src2[1], src2[2], src2_alpha);

          color_erase_helper (&color, &bgcolor);

          gimp_rgba_get_uchar (&color, dest, dest + 1, dest + 2, dest + 3);
          break;
        }

      if (mask)
        m++;

      src1 += bytes;
      src2 += bytes;
      dest += bytes;
    }
}


void
extract_from_inten_pixels (guchar       *src,
                           guchar       *dest,
                           const guchar *mask,
                           const guchar *bg,
                           gboolean      cut,
                           guint         length,
                           guint         src_bytes,
                           guint         dest_bytes)
{
  const gint    alpha = HAS_ALPHA (src_bytes) ? src_bytes - 1 : src_bytes;
  const guchar *m     = mask ? mask : &no_mask;
  gint          b;
  gint          tmp;

  while (length --)
    {
      for (b = 0; b < alpha; b++)
        dest[b] = src[b];

      if (HAS_ALPHA (src_bytes))
        {
          dest[alpha] = INT_MULT (*m, src[alpha], tmp);
          if (cut)
            src[alpha] = INT_MULT ((255 - *m), src[alpha], tmp);
        }
      else
        {
          if (HAS_ALPHA (dest_bytes))
            dest[alpha] = *m;

          if (cut)
            for (b = 0; b < src_bytes; b++)
              src[b] = INT_BLEND (bg[b], src[b], *m, tmp);
        }

      if (mask)
        m++;

      src += src_bytes;
      dest += dest_bytes;
    }
}


void
extract_from_indexed_pixels (guchar       *src,
                             guchar       *dest,
                             const guchar *mask,
                             const guchar *cmap,
                             const guchar *bg,
                             gboolean      cut,
                             guint         length,
                             guint         src_bytes,
                             guint         dest_bytes)
{
  const guchar *m = mask ? mask : &no_mask;
  gint          b;
  gint          t;

  while (length --)
    {
      gint index = src[0] * 3;

      for (b = 0; b < 3; b++)
        dest[b] = cmap[index + b];

      if (HAS_ALPHA (src_bytes))
        {
          dest[3] = INT_MULT (*m, src[1], t);
          if (cut)
            src[1] = INT_MULT ((255 - *m), src[1], t);
        }
      else
        {
          if (HAS_ALPHA (dest_bytes))
            dest[3] = *m;

          if (cut)
            src[0] = (*m > 127) ? bg[0] : src[0];
        }

      if (mask)
        m++;

      src += src_bytes;
      dest += dest_bytes;
    }
}


/**************************************************/
/*    REGION FUNCTIONS                            */
/**************************************************/

void
color_region (PixelRegion  *dest,
              const guchar *col)
{
  gpointer pr;

  for (pr = pixel_regions_register (1, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      guchar *s = dest->data;
      gint    h = dest->h;

      if (dest->w * dest->bytes == dest->rowstride)
        {
          /* do it all in one function call if we can
           * this hasn't been tested to see if it is a
           * signifigant speed gain yet
           */
          color_pixels (s, col, dest->w * h, dest->bytes);
        }
      else
        {
          while (h--)
            {
              color_pixels (s, col, dest->w, dest->bytes);

              s += dest->rowstride;
            }
        }
    }
}

void
color_region_mask (PixelRegion  *dest,
                   PixelRegion  *mask,
                   const guchar *col)
{
  gpointer pr;

  for (pr = pixel_regions_register (2, dest, mask);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      guchar       *d = dest->data;
      const guchar *m = mask->data;
      gint          h = dest->h;

      if (dest->w * dest->bytes == dest->rowstride &&
          mask->w * mask->bytes == mask->rowstride)
        {
          /* do it all in one function call if we can
           * this hasn't been tested to see if it is a
           * signifigant speed gain yet
           */
          color_pixels_mask (d, m, col, dest->w * h, dest->bytes);
        }
      else
        {
          while (h--)
            {
              color_pixels_mask (d, m, col, dest->w, dest->bytes);

              d += dest->rowstride;
              m += mask->rowstride;
            }
        }
    }
}

void
pattern_region (PixelRegion  *dest,
                PixelRegion  *mask,
                TempBuf      *pattern,
                gint          off_x,
                gint          off_y)
{
  gpointer pr;

  for (pr = pixel_regions_register (2, dest, mask);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      guchar       *d = dest->data;
      const guchar *m = mask ? mask->data : NULL;
      gint          y;

      for (y = 0; y < dest->h; y++)
        {
          pattern_pixels_mask (d, m, pattern, dest->w, dest->bytes,
                               off_x + dest->x,
                               off_y + dest->y + y);

          d += dest->rowstride;

          if (mask)
            m += mask->rowstride;
        }
    }
}

void
blend_region (PixelRegion *src1,
              PixelRegion *src2,
              PixelRegion *dest,
              guchar       blend)
{
  gpointer pr;

  for (pr = pixel_regions_register (3, src1, src2, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *s1 = src1->data;
      const guchar *s2 = src2->data;
      guchar       *d  = dest->data;
      gint          h  = src1->h;

      while (h --)
        {
          blend_pixels (s1, s2, d, blend, src1->w, src1->bytes);

          s1 += src1->rowstride;
          s2 += src2->rowstride;
          d += dest->rowstride;
        }
    }
}


void
shade_region (PixelRegion *src,
              PixelRegion *dest,
              guchar      *color,
              guchar       blend)
{
  const guchar *s = src->data;
  guchar       *d = dest->data;
  gint          h = src->h;

  while (h --)
    {
      blend_pixels (s, d, color, blend, src->w, src->bytes);

      s += src->rowstride;
      d += dest->rowstride;
    }
}


void
copy_region (PixelRegion *src,
             PixelRegion *dest)
{
  gpointer pr;

#ifdef COWSHOW
  fputc ('[',stderr);
#endif
  for (pr = pixel_regions_register (2, src, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      if (src->tiles && dest->tiles &&
          src->curtile && dest->curtile &&
          src->offx == 0 && dest->offx == 0 &&
          src->offy == 0 && dest->offy == 0 &&
          src->w  == tile_ewidth (src->curtile)  &&
          dest->w == tile_ewidth (dest->curtile) &&
          src->h  == tile_eheight (src->curtile) &&
          dest->h == tile_eheight (dest->curtile))
        {
#ifdef COWSHOW
          fputc('!',stderr);
#endif
          tile_manager_map_over_tile (dest->tiles,
                                      dest->curtile, src->curtile);
        }
      else
        {
          const guchar *s      = src->data;
          guchar       *d      = dest->data;
          gint          h      = src->h;
          gint          pixels = src->w * src->bytes;

#ifdef COWSHOW
          fputc ('.',stderr);
#endif

          while (h --)
            {
              memcpy (d, s, pixels);

              s += src->rowstride;
              d += dest->rowstride;
            }
        }
    }

#ifdef COWSHOW
  fputc (']',stderr);
  fputc ('\n',stderr);
#endif
}

void
copy_region_nocow (PixelRegion *src,
                   PixelRegion *dest)
{
  gpointer pr;

  for (pr = pixel_regions_register (2, src, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *s      = src->data;
      guchar       *d      = dest->data;
      gint          pixels = src->w * src->bytes;
      gint          h      = src->h;

      while (h --)
        {
          memcpy (d, s, pixels);

          s += src->rowstride;
          d += dest->rowstride;
        }
    }
}


void
add_alpha_region (PixelRegion *src,
                  PixelRegion *dest)
{
  gpointer pr;

  for (pr = pixel_regions_register (2, src, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *s = src->data;
      guchar       *d = dest->data;
      gint          h = src->h;

      while (h --)
        {
          add_alpha_pixels (s, d, src->w, src->bytes);

          s += src->rowstride;
          d += dest->rowstride;
        }
    }
}


void
flatten_region (PixelRegion *src,
                PixelRegion *dest,
                guchar      *bg)
{
  gpointer pr;

  for (pr = pixel_regions_register (2, src, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *s = src->data;
      guchar       *d = dest->data;
      gint          h = src->h;

      while (h --)
        {
          flatten_pixels (s, d, bg, src->w, src->bytes);
          s += src->rowstride;
          d += dest->rowstride;
        }
    }
}


void
extract_alpha_region (PixelRegion *src,
                      PixelRegion *mask,
                      PixelRegion *dest)
{
  gpointer pr;

  for (pr = pixel_regions_register (3, src, mask, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *m = mask ? mask->data : NULL;
      const guchar *s = src->data;
      guchar       *d = dest->data;
      gint          h = src->h;

      while (h --)
        {
          extract_alpha_pixels (s, m, d, src->w, src->bytes);

          s += src->rowstride;
          d += dest->rowstride;
          if (mask)
            m += mask->rowstride;
        }
    }
}


void
extract_from_region (PixelRegion       *src,
                     PixelRegion       *dest,
                     PixelRegion       *mask,
                     const guchar      *cmap,
                     const guchar      *bg,
                     GimpImageBaseType  type,
                     gboolean           cut)
{
  gpointer pr;

  for (pr = pixel_regions_register (3, src, dest, mask);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *m = mask ? mask->data : NULL;
      guchar       *s = src->data;
      guchar       *d = dest->data;
      gint          h = src->h;

      while (h --)
        {
          switch (type)
            {
            case GIMP_RGB:
            case GIMP_GRAY:
              extract_from_inten_pixels (s, d, m, bg, cut, src->w,
                                         src->bytes, dest->bytes);
              break;

            case GIMP_INDEXED:
              extract_from_indexed_pixels (s, d, m, cmap, bg, cut, src->w,
                                           src->bytes, dest->bytes);
              break;
            }

          s += src->rowstride;
          d += dest->rowstride;
          if (mask)
            m += mask->rowstride;
        }
    }
}


void
convolve_region (PixelRegion         *srcR,
                 PixelRegion         *destR,
                 const gfloat        *matrix,
                 gint                 size,
                 gdouble              divisor,
                 GimpConvolutionType  mode,
                 gboolean             alpha_weighting)
{
  /*  Convolve the src image using the convolution matrix, writing to dest  */
  /*  Convolve is not tile-enabled--use accordingly  */
  const guchar *src       = srcR->data;
  guchar       *dest      = destR->data;
  const gint    bytes     = srcR->bytes;
  const gint    a_byte    = bytes - 1;
  const gint    rowstride = srcR->rowstride;
  const gint    margin    = size / 2;
  const gint    x1        = srcR->x;
  const gint    y1        = srcR->y;
  const gint    x2        = srcR->x + srcR->w - 1;
  const gint    y2        = srcR->y + srcR->h - 1;
  gint          x, y;
  gint          offset;

  /*  If the mode is NEGATIVE_CONVOL, the offset should be 128  */
  if (mode == GIMP_NEGATIVE_CONVOL)
    {
      offset = 128;
      mode = GIMP_NORMAL_CONVOL;
    }
  else
    {
      offset = 0;
    }

  for (y = 0; y < destR->h; y++)
    {
      guchar *d = dest;

      if (alpha_weighting)
        {
          for (x = 0; x < destR->w; x++)
            {
              const gfloat *m                = matrix;
              gdouble       total[4]         = { 0.0, 0.0, 0.0, 0.0 };
              gdouble       weighted_divisor = 0.0;
              gint          i, j, b;

              for (j = y - margin; j <= y + margin; j++)
                {
                  for (i = x - margin; i <= x + margin; i++, m++)
                    {
                      gint          xx = CLAMP (i, x1, x2);
                      gint          yy = CLAMP (j, y1, y2);
                      const guchar *s  = src + yy * rowstride + xx * bytes;
                      const guchar  a  = s[a_byte];

                      if (a)
                        {
                          gdouble mult_alpha = *m * a;

                          weighted_divisor += mult_alpha;

                          for (b = 0; b < a_byte; b++)
                            total[b] += mult_alpha * s[b];

                          total[a_byte] += mult_alpha;
                        }
                    }
                }

              if (weighted_divisor == 0.0)
                weighted_divisor = divisor;

              for (b = 0; b < a_byte; b++)
                total[b] /= weighted_divisor;

              total[a_byte] /= divisor;

              for (b = 0; b < bytes; b++)
                {
                  total[b] += offset;

                  if (mode != GIMP_NORMAL_CONVOL && total[b] < 0.0)
                    total[b] = - total[b];

                  if (total[b] < 0.0)
                    *d++ = 0;
                  else
                    *d++ = (total[b] > 255.0) ? 255 : (guchar) ROUND (total[b]);
                }
            }
        }
      else
        {
          for (x = 0; x < destR->w; x++)
            {
              const gfloat *m        = matrix;
              gdouble       total[4] = { 0.0, 0.0, 0.0, 0.0 };
              gint          i, j, b;

              for (j = y - margin; j <= y + margin; j++)
                {
                  for (i = x - margin; i <= x + margin; i++, m++)
                    {
                      gint          xx = CLAMP (i, x1, x2);
                      gint          yy = CLAMP (j, y1, y2);
                      const guchar *s  = src + yy * rowstride + xx * bytes;

                      for (b = 0; b < bytes; b++)
                        total[b] += *m * s[b];
                    }
                }

              for (b = 0; b < bytes; b++)
                {
                  total[b] = total[b] / divisor + offset;

                  if (mode != GIMP_NORMAL_CONVOL && total[b] < 0.0)
                    total[b] = - total[b];

                  if (total[b] < 0.0)
                    *d++ = 0.0;
                  else
                    *d++ = (total[b] > 255.0) ? 255 : (guchar) ROUND (total[b]);
                }
            }
        }

      dest += destR->rowstride;
    }
}

/* Convert from separated alpha to premultiplied alpha. Only works on
   non-tiled regions! */
void
multiply_alpha_region (PixelRegion *srcR)
{
  guchar  *src, *s;
  gint     x, y;
  gint     width, height;
  gint     b, bytes;
  gdouble  alpha_val;

  width = srcR->w;
  height = srcR->h;
  bytes = srcR->bytes;

  src = srcR->data;

  for (y = 0; y < height; y++)
    {
      s = src;

      for (x = 0; x < width; x++)
        {
          alpha_val = s[bytes - 1] * (1.0 / 255.0);

          for (b = 0; b < bytes - 1; b++)
            s[b] = 0.5 + s[b] * alpha_val;

          s += bytes;
        }

      src += srcR->rowstride;
    }
}

/* Convert from premultiplied alpha to separated alpha. Only works on
   non-tiled regions! */
void
separate_alpha_region (PixelRegion *srcR)
{
  guchar  *src, *s;
  gint     x, y;
  gint     width, height;
  gint     b, bytes;
  gdouble  alpha_recip;
  gint     new_val;

  width = srcR->w;
  height = srcR->h;
  bytes = srcR->bytes;

  src = srcR->data;

  for (y = 0; y < height; y++)
    {
      s = src;

      for (x = 0; x < width; x++)
        {
          /* predicate is equivalent to:
             (((s[bytes - 1] - 1) & 255) + 2) & 256
             */
          if (s[bytes - 1] != 0 && s[bytes - 1] != 255)
            {
              alpha_recip = 255.0 / s[bytes - 1];

              for (b = 0; b < bytes - 1; b++)
                {
                  new_val = 0.5 + s[b] * alpha_recip;
                  new_val = MIN (new_val, 255);
                  s[b] = new_val;
                }
            }

          s += bytes;
        }

      src += srcR->rowstride;
    }
}

void
gaussian_blur_region (PixelRegion *srcR,
                      gdouble      radius_x,
                      gdouble      radius_y)
{
  glong   width, height;
  guint   bytes;
  guchar *src, *sp;
  guchar *dest, *dp;
  guchar *data;
  guint  *buf, *b;
  gint    pixels;
  gint    total;
  gint    i, row, col;
  gint    start, end;
  gint   *curve;
  gint   *sum;
  gint    val;
  gint    length;
  gint    alpha;
  gint    initial_p;
  gint    initial_m;

  if (radius_x == 0.0 && radius_y == 0.0)
    return;

  /*  allocate the result buffer  */
  length = MAX (srcR->w, srcR->h) * srcR->bytes;
  data = g_new (guchar, length * 2);
  src = data;
  dest = data + length;

  width = srcR->w;
  height = srcR->h;
  bytes = srcR->bytes;
  alpha = bytes - 1;

  buf = g_new (guint, MAX (width, height) * 2);

  if (radius_y != 0.0)
    {
      curve = make_curve (- SQR (radius_y) / (2 * LOG_1_255), &length);

      sum = g_new (gint, 2 * length + 1);
      sum[0] = 0;

      for (i = 1; i <= length * 2; i++)
        sum[i] = curve[i - length - 1] + sum[i - 1];

      sum += length;

      total = sum[length] - sum[-length];

      for (col = 0; col < width; col++)
        {
          pixel_region_get_col (srcR, col + srcR->x, srcR->y, height, src, 1);
          sp = src + alpha;

          initial_p = sp[0];
          initial_m = sp[(height - 1) * bytes];

          /*  Determine a run-length encoded version of the column  */
          run_length_encode (sp, buf, height, bytes);

          for (row = 0; row < height; row++)
            {
              start = (row < length) ? -row : -length;
              end = (height <= (row + length)) ? (height - row - 1) : length;

              val = total / 2;
              i = start;
              b = buf + (row + i) * 2;

              if (start != -length)
                val += initial_p * (sum[start] - sum[-length]);

              while (i < end)
                {
                  pixels = b[0];
                  i += pixels;
                  if (i > end)
                    i = end;
                  val += b[1] * (sum[i] - sum[start]);
                  b += (pixels * 2);
                  start = i;
                }

              if (end != length)
                val += initial_m * (sum[length] - sum[end]);

              sp[row * bytes] = val / total;
            }

          pixel_region_set_col (srcR, col + srcR->x, srcR->y, height, src);
        }

      g_free (sum - length);
      g_free (curve - length);
    }

  if (radius_x != 0.0)
    {
      curve = make_curve (- SQR (radius_x) / (2 * LOG_1_255), &length);

      sum = g_new (gint, 2 * length + 1);
      sum[0] = 0;

      for (i = 1; i <= length * 2; i++)
        sum[i] = curve[i - length - 1] + sum[i - 1];

      sum += length;

      total = sum[length] - sum[-length];

      for (row = 0; row < height; row++)
        {
          pixel_region_get_row (srcR, srcR->x, row + srcR->y, width, src, 1);
          sp = src + alpha;
          dp = dest + alpha;

          initial_p = sp[0];
          initial_m = sp[(width - 1) * bytes];

          /*  Determine a run-length encoded version of the row  */
          run_length_encode (sp, buf, width, bytes);

          for (col = 0; col < width; col++)
            {
              start = (col < length) ? -col : -length;
              end = (width <= (col + length)) ? (width - col - 1) : length;

              val = total / 2;
              i = start;
              b = buf + (col + i) * 2;

              if (start != -length)
                val += initial_p * (sum[start] - sum[-length]);

              while (i < end)
                {
                  pixels = b[0];

                  i += pixels;

                  if (i > end)
                    i = end;

                  val += b[1] * (sum[i] - sum[start]);
                  b += (pixels * 2);
                  start = i;
                }

              if (end != length)
                val += initial_m * (sum[length] - sum[end]);

              val = val / total;

              dp[col * bytes] = val;
            }

          pixel_region_set_row (srcR, srcR->x, row + srcR->y, width, dest);
        }

      g_free (sum - length);
      g_free (curve - length);
    }

  g_free (data);
  g_free (buf);
}

static inline void
rotate_pointers (guchar  **p,
                 guint32   n)
{
  guint32  i;
  guchar  *tmp;

  tmp = p[0];

  for (i = 0; i < n - 1; i++)
    p[i] = p[i + 1];

  p[i] = tmp;
}


gfloat
shapeburst_region (PixelRegion      *srcPR,
                   PixelRegion      *distPR,
                   GimpProgressFunc  progress_callback,
                   gpointer          progress_data)
{
  Tile   *tile;
  guchar *tile_data;
  gfloat  max_iterations = 0.0;
  gfloat *distp_cur;
  gfloat *distp_prev;
  gfloat *memory;
  gfloat *tmp;
  gfloat  min_prev;
  gfloat  float_tmp;
  gint    min;
  gint    min_left;
  gint    length;
  gint    i, j, k;
  gint    fraction;
  gint    prev_frac;
  gint    x, y;
  gint    end;
  gint    boundary;
  gint    inc;
  gint    src          = 0;
  gint    max_progress = srcPR->w * srcPR->h;
  gint    progress     = 0;

  length = distPR->w + 1;
  memory = g_new (gfloat, length * 2);

  distp_prev = memory;
  for (i = 0; i < length; i++)
    distp_prev[i] = 0.0;

  distp_prev += 1;
  distp_cur = distp_prev + length;

  for (i = 0; i < srcPR->h; i++)
    {
      /*  set the current dist row to 0's  */
      memset (distp_cur - 1, 0, sizeof (gfloat) * (length - 1));

      for (j = 0; j < srcPR->w; j++)
        {
          min_prev = MIN (distp_cur[j-1], distp_prev[j]);
          min_left = MIN ((srcPR->w - j - 1), (srcPR->h - i - 1));
          min = (gint) MIN (min_left, min_prev);
          fraction = 255;

          /*  This might need to be changed to 0
              instead of k = (min) ? (min - 1) : 0  */

          for (k = (min) ? (min - 1) : 0; k <= min; k++)
            {
              x = j;
              y = i + k;
              end = y - k;

              while (y >= end)
                {
                  gint width;

                  tile = tile_manager_get_tile (srcPR->tiles,
                                                x, y, TRUE, FALSE);

                  tile_data = tile_data_pointer (tile, x, y);
                  width = tile_ewidth (tile);

                  boundary = MIN (y % TILE_HEIGHT,
                                  width - (x % TILE_WIDTH) - 1);
                  boundary = MIN (boundary, y - end) + 1;

                  inc = 1 - width;

                  while (boundary--)
                    {
                      src = *tile_data;

                      if (src == 0)
                        {
                          min = k;
                          y = -1;
                          break;
                        }

                      if (src < fraction)
                        fraction = src;

                      x++;
                      y--;
                      tile_data += inc;
                    }

                  tile_release (tile, FALSE);
                }
            }

          if (src != 0)
            {
              /*  If min_left != min_prev use the previous fraction
               *   if it is less than the one found
               */
              if (min_left != min)
                {
                  prev_frac = (int) (255 * (min_prev - min));

                  if (prev_frac == 255)
                    prev_frac = 0;

                  fraction = MIN (fraction, prev_frac);
                }

              min++;
            }

          float_tmp = distp_cur[j] = min + fraction / 256.0;

          if (float_tmp > max_iterations)
            max_iterations = float_tmp;
        }

      /*  set the dist row  */
      pixel_region_set_row (distPR,
                            distPR->x, distPR->y + i, distPR->w,
                            (guchar *) distp_cur);

      /*  swap pointers around  */
      tmp = distp_prev;
      distp_prev = distp_cur;
      distp_cur = tmp;

      if (progress_callback)
        {
          progress += srcPR->h;
          (* progress_callback) (0, max_progress, progress, progress_data);
        }
    }

  g_free (memory);

  return max_iterations;
}

static void
compute_border (gint16  *circ,
                guint16  xradius,
                guint16  yradius)
{
  gint32  i;
  gint32  diameter = xradius * 2 + 1;
  gdouble tmp;

  for (i = 0; i < diameter; i++)
  {
    if (i > xradius)
      tmp = (i - xradius) - 0.5;
    else if (i < xradius)
      tmp = (xradius - i) - 0.5;
    else
      tmp = 0.0;

    circ[i] = RINT (yradius /
                    (gdouble) xradius * sqrt (SQR (xradius) - SQR (tmp)));
  }
}

void
fatten_region (PixelRegion *region,
               gint16       xradius,
               gint16       yradius)
{
  /*
     Any bugs in this fuction are probably also in thin_region
     Blame all bugs in this function on jaycox@gimp.org
  */
  register gint32 i, j, x, y;

  guchar  **buf;  /* caches the region's pixel data */
  guchar   *out;  /* holds the new scan line we are computing */
  guchar  **max;  /* caches the largest values for each column */
  gint16   *circ; /* holds the y coords of the filter's mask */
  gint16    last_max, last_index;

  guchar   *buffer;

  if (xradius <= 0 || yradius <= 0)
    return;

  max = g_new (guchar *, region->w + 2 * xradius);
  buf = g_new (guchar *, yradius + 1);

  for (i = 0; i < yradius + 1; i++)
    buf[i] = g_new (guchar, region->w);

  buffer = g_new (guchar, (region->w + 2 * xradius) * (yradius + 1));

  for (i = 0; i < region->w + 2 * xradius; i++)
    {
      if (i < xradius)
        max[i] = buffer;
      else if (i < region->w + xradius)
        max[i] = &buffer[(yradius + 1) * (i - xradius)];
      else
        max[i] = &buffer[(yradius + 1) * (region->w + xradius - 1)];

      for (j = 0; j < xradius + 1; j++)
        max[i][j] = 0;
    }

  /* offset the max pointer by xradius so the range of the array
     is [-xradius] to [region->w + xradius] */
  max += xradius;

  out =  g_new (guchar, region->w);

  circ = g_new (gint16, 2 * xradius + 1);
  compute_border (circ, xradius, yradius);

  /* offset the circ pointer by xradius so the range of the array
     is [-xradius] to [xradius] */
  circ += xradius;

  memset (buf[0], 0, region->w);

  for (i = 0; i < yradius && i < region->h; i++) /* load top of image */
    pixel_region_get_row (region,
                          region->x, region->y + i, region->w, buf[i + 1], 1);

  for (x = 0; x < region->w; x++) /* set up max for top of image */
    {
      max[x][0] = 0;         /* buf[0][x] is always 0 */
      max[x][1] = buf[1][x]; /* MAX (buf[1][x], max[x][0]) always = buf[1][x]*/

      for (j = 2; j < yradius + 1; j++)
        max[x][j] = MAX(buf[j][x], max[x][j-1]);
    }

  for (y = 0; y < region->h; y++)
    {
      rotate_pointers (buf, yradius + 1);

      if (y < region->h - (yradius))
        pixel_region_get_row (region,
                              region->x, region->y + y + yradius, region->w,
                              buf[yradius], 1);
      else
        memset (buf[yradius], 0, region->w);

      for (x = 0; x < region->w; x++) /* update max array */
        {
          for (i = yradius; i > 0; i--)
            max[x][i] = MAX (MAX (max[x][i - 1], buf[i - 1][x]), buf[i][x]);

          max[x][0] = buf[0][x];
        }

      last_max = max[0][circ[-1]];
      last_index = 1;

      for (x = 0; x < region->w; x++) /* render scan line */
        {
          last_index--;

          if (last_index >= 0)
            {
              if (last_max == 255)
                {
                  out[x] = 255;
                }
              else
                {
                  last_max = 0;

                  for (i = xradius; i >= 0; i--)
                    if (last_max < max[x + i][circ[i]])
                      {
                        last_max = max[x + i][circ[i]];
                        last_index = i;
                      }

                  out[x] = last_max;
                }
            }
          else
            {
              last_index = xradius;
              last_max = max[x + xradius][circ[xradius]];

              for (i = xradius - 1; i >= -xradius; i--)
                if (last_max < max[x + i][circ[i]])
                  {
                    last_max = max[x + i][circ[i]];
                    last_index = i;
                  }

              out[x] = last_max;
            }
        }

      pixel_region_set_row (region, region->x, region->y + y, region->w, out);
    }

  /* undo the offsets to the pointers so we can free the malloced memmory */
  circ -= xradius;
  max -= xradius;

  g_free (circ);
  g_free (buffer);
  g_free (max);

  for (i = 0; i < yradius + 1; i++)
    g_free (buf[i]);

  g_free (buf);
  g_free (out);
}

void
thin_region (PixelRegion *region,
             gint16       xradius,
             gint16       yradius,
             gboolean     edge_lock)
{
  /*
     pretty much the same as fatten_region only different
     blame all bugs in this function on jaycox@gimp.org
  */
  /* If edge_lock is true  we assume that pixels outside the region
     we are passed are identical to the edge pixels.
     If edge_lock is false, we assume that pixels outside the region are 0
  */
  register gint32 i, j, x, y;
  guchar  **buf;  /* caches the the region's pixels */
  guchar   *out;  /* holds the new scan line we are computing */
  guchar  **max;  /* caches the smallest values for each column */
  gint16   *circ; /* holds the y coords of the filter's mask */
  gint16    last_max, last_index;

  guchar   *buffer;
  gint      buffer_size;

  if (xradius <= 0 || yradius <= 0)
    return;

  max = g_new (guchar *, region->w + 2 * xradius);
  buf = g_new (guchar *, yradius + 1);

  for (i = 0; i < yradius + 1; i++)
    buf[i] = g_new (guchar, region->w);

  buffer_size = (region->w + 2 * xradius + 1) * (yradius + 1);
  buffer = g_new (guchar, buffer_size);

  if (edge_lock)
    memset(buffer, 255, buffer_size);
  else
    memset(buffer, 0, buffer_size);

  for (i = 0; i < region->w + 2 * xradius; i++)
    {
      if (i < xradius)
        {
          if (edge_lock)
            max[i] = buffer;
          else
            max[i] = &buffer[(yradius + 1) * (region->w + xradius)];
        }
      else if (i < region->w + xradius)
        {
          max[i] = &buffer[(yradius + 1) * (i - xradius)];
        }
      else
        {
          if (edge_lock)
            max[i] = &buffer[(yradius + 1) * (region->w + xradius - 1)];
          else
            max[i] = &buffer[(yradius + 1) * (region->w + xradius)];
        }
    }

  if (! edge_lock)
    for (j = 0 ; j < xradius + 1; j++)
      max[0][j] = 0;

  /* offset the max pointer by xradius so the range of the array
     is [-xradius] to [region->w + xradius] */
  max += xradius;

  out = g_new (guchar, region->w);

  circ = g_new (gint16, 2 * xradius + 1);
  compute_border (circ, xradius, yradius);

 /* offset the circ pointer by xradius so the range of the array
    is [-xradius] to [xradius] */
  circ += xradius;

  for (i = 0; i < yradius && i < region->h; i++) /* load top of image */
    pixel_region_get_row (region,
                          region->x, region->y + i, region->w, buf[i + 1], 1);
  if (edge_lock)
    memcpy (buf[0], buf[1], region->w);
  else
    memset (buf[0], 0, region->w);


  for (x = 0; x < region->w; x++) /* set up max for top of image */
    {
      max[x][0] = buf[0][x];

      for (j = 1; j < yradius + 1; j++)
        max[x][j] = MIN(buf[j][x], max[x][j-1]);
    }

  for (y = 0; y < region->h; y++)
    {
      rotate_pointers (buf, yradius + 1);

      if (y < region->h - yradius)
        pixel_region_get_row (region,
                              region->x, region->y + y + yradius, region->w,
                              buf[yradius], 1);
      else if (edge_lock)
        memcpy (buf[yradius], buf[yradius - 1], region->w);
      else
        memset (buf[yradius], 0, region->w);

      for (x = 0 ; x < region->w; x++) /* update max array */
        {
          for (i = yradius; i > 0; i--)
            max[x][i] = MIN (MIN (max[x][i - 1], buf[i - 1][x]), buf[i][x]);

          max[x][0] = buf[0][x];
        }

      last_max =  max[0][circ[-1]];
      last_index = 0;

      for (x = 0 ; x < region->w; x++) /* render scan line */
        {
          last_index--;

          if (last_index >= 0)
            {
              if (last_max == 0)
                {
                  out[x] = 0;
                }
              else
                {
                  last_max = 255;

                  for (i = xradius; i >= 0; i--)
                    if (last_max > max[x + i][circ[i]])
                      {
                        last_max = max[x + i][circ[i]];
                        last_index = i;
                      }

                  out[x] = last_max;
                }
            }
          else
            {
              last_index = xradius;
              last_max = max[x + xradius][circ[xradius]];

              for (i = xradius - 1; i >= -xradius; i--)
                if (last_max > max[x + i][circ[i]])
                  {
                    last_max = max[x + i][circ[i]];
                    last_index = i;
                  }

              out[x] = last_max;
            }
        }

      pixel_region_set_row (region, region->x, region->y + y, region->w, out);
    }

  /* undo the offsets to the pointers so we can free the malloced memmory */
  circ -= xradius;
  max -= xradius;

  /* free the memmory */
  g_free (circ);
  g_free (buffer);
  g_free (max);

  for (i = 0; i < yradius + 1; i++)
    g_free (buf[i]);

  g_free (buf);
  g_free (out);
}

/*  Simple convolution filter to smooth a mask (1bpp).  */
void
smooth_region (PixelRegion *region)
{
  gint      x, y;
  gint      width;
  gint      i;
  guchar   *buf[3];
  guchar   *out;

  width = region->w;

  for (i = 0; i < 3; i++)
    buf[i] = g_new (guchar, width + 2);

  out = g_new (guchar, width);

  /* load top of image */
  pixel_region_get_row (region, region->x, region->y, width, buf[0] + 1, 1);

  buf[0][0]         = buf[0][1];
  buf[0][width + 1] = buf[0][width];

  memcpy (buf[1], buf[0], width + 2);

  for (y = 0; y < region->h; y++)
    {
      if (y + 1 < region->h)
        {
          pixel_region_get_row (region, region->x, region->y + y + 1, width,
                                buf[2] + 1, 1);

          buf[2][0]         = buf[2][1];
          buf[2][width + 1] = buf[2][width];
        }
      else
        {
          memcpy (buf[2], buf[1], width + 2);
        }

      for (x = 0 ; x < width; x++)
        {
          gint value = (buf[0][x] + buf[0][x+1] + buf[0][x+2] +
                        buf[1][x] + buf[2][x+1] + buf[1][x+2] +
                        buf[2][x] + buf[1][x+1] + buf[2][x+2]);

          out[x] = value / 9;
        }

      pixel_region_set_row (region, region->x, region->y + y, width, out);

      rotate_pointers (buf, 3);
    }

  for (i = 0; i < 3; i++)
    g_free (buf[i]);

  g_free (out);
}

/*  Erode (radius 1 pixel) a mask (1bpp).  */
void
erode_region (PixelRegion *region)
{
  gint      x, y;
  gint      width;
  gint      i;
  guchar   *buf[3];
  guchar   *out;

  width = region->w;

  for (i = 0; i < 3; i++)
    buf[i] = g_new (guchar, width + 2);

  out = g_new (guchar, width);

  /* load top of image */
  pixel_region_get_row (region, region->x, region->y, width, buf[0] + 1, 1);

  buf[0][0]         = buf[0][1];
  buf[0][width + 1] = buf[0][width];

  memcpy (buf[1], buf[0], width + 2);

  for (y = 0; y < region->h; y++)
    {
      if (y + 1 < region->h)
        {
          pixel_region_get_row (region, region->x, region->y + y + 1, width,
                                buf[2] + 1, 1);

          buf[2][0]         = buf[2][1];
          buf[2][width + 1] = buf[2][width];
        }
      else
        {
          memcpy (buf[2], buf[1], width + 2);
        }

      for (x = 0 ; x < width; x++)
        {
          gint min = 255;

          if (buf[0][x+1] < min) min = buf[0][x+1];
          if (buf[1][x]   < min) min = buf[1][x];
          if (buf[1][x+1] < min) min = buf[1][x+1];
          if (buf[1][x+2] < min) min = buf[1][x+2];
          if (buf[2][x+1] < min) min = buf[2][x+1];

          out[x] = min;
        }

      pixel_region_set_row (region, region->x, region->y + y, width, out);

      rotate_pointers (buf, 3);
    }

  for (i = 0; i < 3; i++)
    g_free (buf[i]);

  g_free (out);
}

/*  Dilate (radius 1 pixel) a mask (1bpp).  */
void
dilate_region (PixelRegion *region)
{
  gint      x, y;
  gint      width;
  gint      i;
  guchar   *buf[3];
  guchar   *out;

  width = region->w;

  for (i = 0; i < 3; i++)
    buf[i] = g_new (guchar, width + 2);

  out = g_new (guchar, width);

  /* load top of image */
  pixel_region_get_row (region, region->x, region->y, width, buf[0] + 1, 1);

  buf[0][0]         = buf[0][1];
  buf[0][width + 1] = buf[0][width];

  memcpy (buf[1], buf[0], width + 2);

  for (y = 0; y < region->h; y++)
    {
      if (y + 1 < region->h)
        {
          pixel_region_get_row (region, region->x, region->y + y + 1, width,
                                buf[2] + 1, 1);

          buf[2][0]         = buf[2][1];
          buf[2][width + 1] = buf[2][width];
        }
      else
        {
          memcpy (buf[2], buf[1], width + 2);
        }

      for (x = 0 ; x < width; x++)
        {
          gint max = 0;

          if (buf[0][x+1] > max) max = buf[0][x+1];
          if (buf[1][x]   > max) max = buf[1][x];
          if (buf[1][x+1] > max) max = buf[1][x+1];
          if (buf[1][x+2] > max) max = buf[1][x+2];
          if (buf[2][x+1] > max) max = buf[2][x+1];

          out[x] = max;
        }

      pixel_region_set_row (region, region->x, region->y + y, width, out);

      rotate_pointers (buf, 3);
    }

  for (i = 0; i < 3; i++)
    g_free (buf[i]);

  g_free (out);
}

/* Computes whether pixels in `buf[1]', if they are selected, have neighbouring
   pixels that are unselected. Put result in `transition'. */
static void
compute_transition (guchar    *transition,
                    guchar   **buf,
                    gint32     width,
                    gboolean   edge_lock)
{
  register gint32 x = 0;

  if (width == 1)
    {
      if (buf[1][0] > 127 && (buf[0][0] < 128 || buf[2][0] < 128))
        transition[0] = 255;
      else
        transition[0] = 0;
      return;
    }

  if (buf[1][0] > 127 && edge_lock)
    {
      /* The pixel to the left (outside of the canvas) is considered selected,
         so we check if there are any unselected pixels in neighbouring pixels
         _on_ the canvas. */
      if (buf[0][x] < 128 || buf[0][x + 1] < 128 ||
                             buf[1][x + 1] < 128 ||
          buf[2][x] < 128 || buf[2][x + 1] < 128 )
        {
          transition[x] = 255;
        }
      else
        {
          transition[x] = 0;
        }
    }
  else if (buf[1][0] > 127 && !edge_lock)
    {
      /* We must not care about neighbouring pixels on the image canvas since
         there always are unselected pixels to the left (which is outside of
         the image canvas). */
      transition[x] = 255;
    }
  else
    {
      transition[x] = 0;
    }

  for (x = 1; x < width - 1; x++)
    {
      if (buf[1][x] >= 128)
        {
          if (buf[0][x - 1] < 128 || buf[0][x] < 128 || buf[0][x + 1] < 128 ||
              buf[1][x - 1] < 128 ||                    buf[1][x + 1] < 128 ||
              buf[2][x - 1] < 128 || buf[2][x] < 128 || buf[2][x + 1] < 128)
            transition[x] = 255;
          else
            transition[x] = 0;
        }
      else
        {
          transition[x] = 0;
        }
    }

  if (buf[1][width - 1] >= 128 && edge_lock)
    {
      /* The pixel to the right (outside of the canvas) is considered selected,
         so we check if there are any unselected pixels in neighbouring pixels
         _on_ the canvas. */
      if ( buf[0][x - 1] < 128 || buf[0][x] < 128 ||
           buf[1][x - 1] < 128 ||
           buf[2][x - 1] < 128 || buf[2][x] < 128)
        {
          transition[width - 1] = 255;
        }
      else
        {
          transition[width - 1] = 0;
        }
    }
  else if (buf[1][width - 1] >= 128 && !edge_lock)
    {
      /* We must not care about neighbouring pixels on the image canvas since
         there always are unselected pixels to the right (which is outside of
         the image canvas). */
      transition[width - 1] = 255;
    }
  else
    {
      transition[width - 1] = 0;
    }
}

void
border_region (PixelRegion *src,
               gint16       xradius,
               gint16       yradius,
               gboolean     feather,
               gboolean     edge_lock)
{
  /*
     This function has no bugs, but if you imagine some you can
     blame them on jaycox@gimp.org
  */

  register gint32 i, j, x, y;

  /* A cache used in the algorithm as it works its way down. `buf[1]' is the
     current row. Thus, at algorithm initialization, `buf[0]' represents the
     row 'above' the first row of the region. */
  guchar  *buf[3];

  /* The resulting selection is calculated row by row, and this buffer holds the
     output for each individual row, on each iteration. */
  guchar  *out;

  /* Keeps track of transitional pixels (pixels that are selected and have
     unselected neighbouring pixels). */
  guchar **transition;

  /* TODO: Figure out role clearly in algorithm. */
  gint16  *max;

  /* TODO: Figure out role clearly in algorithm. */
  guchar **density;

  guchar   last_max;
  gint16   last_index;

  if (xradius < 0 || yradius < 0)
    {
      g_warning ("border_region: negative radius specified.");
      return;
    }

  /* A border without a width is no border at all; return an empty region. */
  if (xradius == 0 || yradius == 0)
    {
      guchar color[] = "\0\0\0\0";

      color_region (src, color);
      return;
    }

  /* optimize this case specifically */
  if (xradius == 1 && yradius == 1)
    {
      guchar *transition;
      guchar *source[3];

      for (i = 0; i < 3; i++)
        source[i] = g_new (guchar, src->w);

      transition = g_new (guchar, src->w);

      /* With `edge_lock', initialize row above image as selected, otherwise,
         initialize as unselected. */
      memset (source[0], edge_lock ? 255 : 0, src->w);

      pixel_region_get_row (src, src->x, src->y + 0, src->w, source[1], 1);

      if (src->h > 1)
        pixel_region_get_row (src, src->x, src->y + 1, src->w, source[2], 1);
      else
        memcpy (source[2], source[1], src->w);

      compute_transition (transition, source, src->w, edge_lock);
      pixel_region_set_row (src, src->x, src->y , src->w, transition);

      for (y = 1; y < src->h; y++)
        {
          rotate_pointers (source, 3);

          if (y + 1 < src->h)
            {
              pixel_region_get_row (src, src->x, src->y + y + 1, src->w,
                                    source[2], 1);
            }
          else
            {
              /* Depending on `edge_lock', set the row below the image as either
                 selected or non-selected. */
              memset(source[2], edge_lock ? 255 : 0, src->w);
            }

          compute_transition (transition, source, src->w, edge_lock);
          pixel_region_set_row (src, src->x, src->y + y, src->w, transition);
        }

      for (i = 0; i < 3; i++)
        g_free (source[i]);

      g_free (transition);

      /* Finnished handling the radius = 1 special case, return here. */
      return;
    }

  max = g_new (gint16, src->w + 2 * xradius);

  for (i = 0; i < (src->w + 2 * xradius); i++)
    max[i] = yradius + 2;

  max += xradius;

  for (i = 0; i < 3; i++)
    buf[i] = g_new (guchar, src->w);

  transition = g_new (guchar *, yradius + 1);

  for (i = 0; i < yradius + 1; i++)
    {
      transition[i] = g_new (guchar, src->w + 2 * xradius);
      memset(transition[i], 0, src->w + 2 * xradius);
      transition[i] += xradius;
    }

  out = g_new (guchar, src->w);

  density = g_new (guchar *, 2 * xradius + 1);
  density += xradius;

   /* allocate density[][] */
  for (x = 0; x < (xradius + 1); x++)
    {
      density[ x]  = g_new (guchar, 2 * yradius + 1);
      density[ x] += yradius;
      density[-x]  = density[x];
    }

  /* compute density[][] */
  for (x = 0; x < (xradius + 1); x++)
    {
      register gdouble tmpx, tmpy, dist;
      guchar a;

      if (x > 0)
        tmpx = x - 0.5;
      else if (x < 0)
        tmpx = x + 0.5;
      else
        tmpx = 0.0;

      for (y = 0; y < (yradius + 1); y++)
        {
          if (y > 0)
            tmpy = y - 0.5;
          else if (y < 0)
            tmpy = y + 0.5;
          else
            tmpy = 0.0;

          dist = ((tmpy * tmpy) / (yradius * yradius) +
                  (tmpx * tmpx) / (xradius * xradius));

          if (dist < 1.0)
            {
              if (feather)
                a = 255 * (1.0 - sqrt (dist));
              else
                a = 255;
            }
          else
            {
              a = 0;
            }

          density[ x][ y] = a;
          density[ x][-y] = a;
          density[-x][ y] = a;
          density[-x][-y] = a;
        }
    }

  /* Since the algorithm considerers `buf[0]' to be 'over' the row currently
     calculated, we must start with `buf[0]' as non-selected if there is no
     `edge_lock. If there is an 'edge_lock', initialize the first row to
     'selected'. Refer to bug #350009. */
  memset (buf[0], edge_lock ? 255 : 0, src->w);
  pixel_region_get_row (src, src->x, src->y + 0, src->w, buf[1], 1);

  if (src->h > 1)
    pixel_region_get_row (src, src->x, src->y + 1, src->w, buf[2], 1);
  else
    memcpy (buf[2], buf[1], src->w);

  compute_transition (transition[1], buf, src->w, edge_lock);

   /* set up top of image */
  for (y = 1; y < yradius && y + 1 < src->h; y++)
    {
      rotate_pointers (buf, 3);
      pixel_region_get_row (src, src->x, src->y + y + 1, src->w, buf[2], 1);
      compute_transition (transition[y + 1], buf, src->w, edge_lock);
    }

  /* set up max[] for top of image */
  for (x = 0; x < src->w; x++)
    {
      max[x] = -(yradius + 7);

      for (j = 1; j < yradius + 1; j++)
        if (transition[j][x])
          {
            max[x] = j;
            break;
          }
    }

  /* main calculation loop */
  for (y = 0; y < src->h; y++)
    {
      rotate_pointers (buf, 3);
      rotate_pointers (transition, yradius + 1);

      if (y < src->h - (yradius + 1))
        {
          pixel_region_get_row (src, src->x, src->y + y + yradius + 1, src->w,
                                buf[2], 1);
          compute_transition (transition[yradius], buf, src->w, edge_lock);
        }
      else
        {
          if (edge_lock)
            {
              memcpy (transition[yradius], transition[yradius - 1], src->w);
            }
          else
            {
              /* No edge lock, set everything 'below canvas' as seen from the
                 algorithm as unselected. */
              memset (buf[2], 0, src->w);
              compute_transition (transition[yradius], buf, src->w, edge_lock);
            }
        }

       /* update max array */
      for (x = 0; x < src->w; x++)
        {
          if (max[x] < 1)
            {
              if (max[x] <= -yradius)
                {
                  if (transition[yradius][x])
                    max[x] = yradius;
                  else
                    max[x]--;
                }
              else
                {
                  if (transition[-max[x]][x])
                    max[x] = -max[x];
                  else if (transition[-max[x] + 1][x])
                    max[x] = -max[x] + 1;
                  else
                    max[x]--;
                }
            }
          else
            {
              max[x]--;
            }

          if (max[x] < -yradius - 1)
            max[x] = -yradius - 1;
        }

      last_max =  max[0][density[-1]];
      last_index = 1;

       /* render scan line */
      for (x = 0 ; x < src->w; x++)
        {
          last_index--;

          if (last_index >= 0)
            {
              last_max = 0;

              for (i = xradius; i >= 0; i--)
                if (max[x + i] <= yradius && max[x + i] >= -yradius &&
                    density[i][max[x+i]] > last_max)
                  {
                    last_max = density[i][max[x + i]];
                    last_index = i;
                  }

              out[x] = last_max;
            }
          else
            {
              last_max = 0;

              for (i = xradius; i >= -xradius; i--)
                if (max[x + i] <= yradius && max[x + i] >= -yradius &&
                    density[i][max[x + i]] > last_max)
                  {
                    last_max = density[i][max[x + i]];
                    last_index = i;
                  }

              out[x] = last_max;
            }

          if (last_max == 0)
            {
              for (i = x + 1; i < src->w; i++)
                {
                  if (max[i] >= -yradius)
                    break;
                }

              if (i - x > xradius)
                {
                  for (; x < i - xradius; x++)
                    out[x] = 0;

                  x--;
                }

              last_index = xradius;
            }
        }

      pixel_region_set_row (src, src->x, src->y + y, src->w, out);
    }

  g_free (out);

  for (i = 0; i < 3; i++)
    g_free (buf[i]);

  max -= xradius;
  g_free (max);

  for (i = 0; i < yradius + 1; i++)
    {
      transition[i] -= xradius;
      g_free (transition[i]);
    }

  g_free (transition);

  for (i = 0; i < xradius + 1 ; i++)
    {
      density[i] -= yradius;
      g_free (density[i]);
    }

  density -= xradius;
  g_free (density);
}

void
swap_region (PixelRegion *src,
             PixelRegion *dest)
{
  gpointer pr;

  for (pr = pixel_regions_register (2, src, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      guchar *s      = src->data;
      guchar *d      = dest->data;
      gint    pixels = src->w * src->bytes;
      gint    h      = src->h;

      while (h--)
        {
          swap_pixels (s, d, pixels);

          s += src->rowstride;
          d += dest->rowstride;
        }
    }
}


/* Computes whether pixels in `buf[1]' have neighbouring pixels that are
   unselected. Put result in `transition'. */
static void
apply_mask_to_sub_region (gint        *opacityp,
                          PixelRegion *src,
                          PixelRegion *mask)
{
  guchar       *s       = src->data;
  const guchar *m       = mask->data;
  gint          h       = src->h;
  guint         opacity = *opacityp;

  while (h--)
    {
      apply_mask_to_alpha_channel (s, m, opacity, src->w, src->bytes);
      s += src->rowstride;
      m += mask->rowstride;
    }
}

void
apply_mask_to_region (PixelRegion *src,
                      PixelRegion *mask,
                      guint        opacity)
{
  pixel_regions_process_parallel ((PixelProcessorFunc)
                                  apply_mask_to_sub_region,
                                  &opacity, 2, src, mask);
}


static void
combine_mask_and_sub_region_stipple (gint        *opacityp,
                                     PixelRegion *src,
                                     PixelRegion *mask)
{
  guchar       *s       = src->data;
  const guchar *m       = mask->data;
  gint          h       = src->h;
  guint         opacity = *opacityp;

  while (h--)
    {
      combine_mask_and_alpha_channel_stipple (s, m, opacity,
                                              src->w, src->bytes);
      s += src->rowstride;
      m += mask->rowstride;
    }
}


static void
combine_mask_and_sub_region_stroke (gint        *opacityp,
                                    PixelRegion *src,
                                    PixelRegion *mask)
{
  guchar       *s       = src->data;
  const guchar *m       = mask->data;
  gint          h       = src->h;
  guint         opacity = *opacityp;

  while (h--)
    {
      combine_mask_and_alpha_channel_stroke (s, m, opacity, src->w, src->bytes);
      s += src->rowstride;
      m += mask->rowstride;
    }
}


void
combine_mask_and_region (PixelRegion *src,
                         PixelRegion *mask,
                         guint        opacity,
                         gboolean     stipple)
{
  if (stipple)
    pixel_regions_process_parallel ((PixelProcessorFunc)
                                    combine_mask_and_sub_region_stipple,
                                    &opacity, 2, src, mask);
  else
    pixel_regions_process_parallel ((PixelProcessorFunc)
                                    combine_mask_and_sub_region_stroke,
                                    &opacity, 2, src, mask);
}


void
copy_gray_to_region (PixelRegion *src,
                     PixelRegion *dest)
{
  gpointer pr;

  for (pr = pixel_regions_register (2, src, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *s = src->data;
      guchar       *d = dest->data;
      gint          h = src->h;

      while (h--)
        {
          copy_gray_to_inten_a_pixels (s, d, src->w, dest->bytes);

          s += src->rowstride;
          d += dest->rowstride;
        }
    }
}

void
copy_component (PixelRegion *src,
                PixelRegion *dest,
                guint        pixel)
{
  gpointer pr;

  for (pr = pixel_regions_register (2, src, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *s = src->data;
      guchar       *d = dest->data;
      gint          h = src->h;

      while (h--)
        {
          copy_component_pixels (s, d, src->w, src->bytes, pixel);
          s += src->rowstride;
          d += dest->rowstride;
        }
    }
}

void
copy_color (PixelRegion *src,
            PixelRegion *dest)
{
  gpointer pr;

  for (pr = pixel_regions_register (2, src, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *s = src->data;
      guchar       *d = dest->data;
      gint          h = src->h;

      while (h--)
        {
          copy_color_pixels (s, d, src->w, src->bytes);
          s += src->rowstride;
          d += dest->rowstride;
        }
    }
}

struct initial_regions_struct
{
  guint                 opacity;
  GimpLayerModeEffects  mode;
  const gboolean       *affect;
  InitialMode           type;
  const guchar         *data;
};

static void
initial_sub_region (struct initial_regions_struct *st,
                    PixelRegion                   *src,
                    PixelRegion                   *dest,
                    PixelRegion                   *mask)
{
  gint                  h;
  guchar               *s, *d, *m;
  guchar               *buf;
  const guchar         *data;
  guint                 opacity;
  GimpLayerModeEffects  mode;
  const gboolean       *affect;
  InitialMode           type;

  /* use src->bytes + 1 since DISSOLVE always needs a buffer with alpha */
  buf = g_alloca (MAX (src->w * (src->bytes + 1),
                       dest->w * dest->bytes));
  data    = st->data;
  opacity = st->opacity;
  mode    = st->mode;
  affect  = st->affect;
  type    = st->type;

  s = src->data;
  d = dest->data;
  m = mask ? mask->data : NULL;

  for (h = 0; h < src->h; h++)
    {
      /*  based on the type of the initial image...  */
      switch (type)
        {
        case INITIAL_CHANNEL_MASK:
        case INITIAL_CHANNEL_SELECTION:
          initial_channel_pixels (s, d, src->w, dest->bytes);
          break;

        case INITIAL_INDEXED:
          initial_indexed_pixels (s, d, data, src->w);
          break;

        case INITIAL_INDEXED_ALPHA:
          initial_indexed_a_pixels (s, d, m, &no_mask, data, opacity, src->w);
          break;

        case INITIAL_INTENSITY:
          if (mode == GIMP_DISSOLVE_MODE)
            {
              if (gimp_composite_options.bits & GIMP_COMPOSITE_OPTION_USE)
                {
                  GimpCompositeContext ctx;

                  ctx.A = NULL;
                  ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;

                  ctx.B = s;
                  ctx.pixelformat_B = (src->bytes   == 1 ? GIMP_PIXELFORMAT_V8
                                       : src->bytes == 2 ? GIMP_PIXELFORMAT_VA8
                                       : src->bytes == 3 ? GIMP_PIXELFORMAT_RGB8
                                       : src->bytes == 4 ? GIMP_PIXELFORMAT_RGBA8
                                       : GIMP_PIXELFORMAT_ANY);
                  ctx.D = buf;
                  ctx.pixelformat_D = ctx.pixelformat_B;

                  ctx.M = m;

                  ctx.n_pixels = src->w;
                  ctx.op = GIMP_COMPOSITE_DISSOLVE;
                  ctx.dissolve.x = src->x;
                  ctx.dissolve.y = src->y + h;
                  ctx.dissolve.opacity = opacity;
                  gimp_composite_dispatch (&ctx);
                }
              else
                {
                  dissolve_pixels (s, m, buf, src->x, src->y + h,
                                   opacity, src->w,
                                   src->bytes, src->bytes + 1,
                                   FALSE);
                }

              initial_inten_a_pixels (buf, d, NULL, OPAQUE_OPACITY, affect,
                                      src->w, src->bytes + 1);
            }
          else
            {
              initial_inten_pixels (s, d, m, &no_mask, opacity, affect,
                                    src->w, src->bytes);
            }
          break;

        case INITIAL_INTENSITY_ALPHA:
          if (mode == GIMP_DISSOLVE_MODE)
            {
              if (gimp_composite_options.bits & GIMP_COMPOSITE_OPTION_USE)
                {
                  GimpCompositeContext ctx;

                  ctx.A = NULL;
                  ctx.pixelformat_A = GIMP_PIXELFORMAT_RGBA8;

                  ctx.B = s;
                  ctx.pixelformat_B = (src->bytes   == 1 ? GIMP_PIXELFORMAT_V8
                                       : src->bytes == 2 ? GIMP_PIXELFORMAT_VA8
                                       : src->bytes == 3 ? GIMP_PIXELFORMAT_RGB8
                                       : src->bytes == 4 ? GIMP_PIXELFORMAT_RGBA8
                                       : GIMP_PIXELFORMAT_ANY);
                  ctx.D = buf;
                  ctx.pixelformat_D = ctx.pixelformat_B;

                  ctx.M = m;

                  ctx.n_pixels = src->w;
                  ctx.op = GIMP_COMPOSITE_DISSOLVE;
                  ctx.dissolve.x = src->x;
                  ctx.dissolve.y = src->y + h;
                  ctx.dissolve.opacity = opacity;
                  gimp_composite_dispatch (&ctx);
                }
              else
                {
                  dissolve_pixels (s, m, buf, src->x, src->y + h,
                                   opacity, src->w,
                                   src->bytes, src->bytes,
                                   TRUE);
                }

              initial_inten_a_pixels (buf, d, NULL, OPAQUE_OPACITY, affect,
                                      src->w, src->bytes);
            }
          else
            {
              initial_inten_a_pixels (s, d, m,
                                      opacity, affect, src->w, src->bytes);
            }
          break;
        }

      s += src->rowstride;
      d += dest->rowstride;
      if (mask)
        m += mask->rowstride;
    }
}

void
initial_region (PixelRegion          *src,
                PixelRegion          *dest,
                PixelRegion          *mask,
                const guchar         *data,
                guint                 opacity,
                GimpLayerModeEffects  mode,
                const gboolean       *affect,
                InitialMode           type)
{
  struct initial_regions_struct st;

  st.opacity = opacity;
  st.mode    = mode;
  st.affect  = affect;
  st.type    = type;
  st.data    = data;

  pixel_regions_process_parallel ((PixelProcessorFunc) initial_sub_region,
                                  &st, 3, src, dest, mask);
}

struct combine_regions_struct
{
  guint                 opacity;
  GimpLayerModeEffects  mode;
  const gboolean       *affect;
  CombinationMode       type;
  const guchar         *data;
  gboolean              opacity_quickskip_possible;
  gboolean              transparency_quickskip_possible;
};

static inline CombinationMode
apply_indexed_layer_mode (guchar                *src1,
                          guchar                *src2,
                          guchar               **dest,
                          GimpLayerModeEffects   mode,
                          CombinationMode        cmode)
{
  /*  assumes we're applying src2 TO src1  */
  switch (mode)
    {
    case GIMP_REPLACE_MODE:
      *dest = src2;
      cmode = REPLACE_INDEXED;
      break;

    case GIMP_BEHIND_MODE:
      *dest = src2;
      if (cmode == COMBINE_INDEXED_A_INDEXED_A)
        cmode = BEHIND_INDEXED;
      else
        cmode = NO_COMBINATION;
      break;

    case GIMP_ERASE_MODE:
      *dest = src2;
      /*  If both sources have alpha channels, call erase function.
       *  Otherwise, just combine in the normal manner
       */
      cmode = (cmode == COMBINE_INDEXED_A_INDEXED_A) ? ERASE_INDEXED : cmode;
      break;

    default:
      break;
    }

  return cmode;
}

static void
combine_sub_region (struct combine_regions_struct *st,
                    PixelRegion                   *src1,
                    PixelRegion                   *src2,
                    PixelRegion                   *dest,
                    PixelRegion                   *mask)
{
  const guchar         *data;
  guint                 opacity;
  guint                 layer_mode_opacity;
  const guchar         *layer_mode_mask;
  GimpLayerModeEffects  mode;
  const gboolean       *affect;
  guint                 h;
  CombinationMode       combine = NO_COMBINATION;
  CombinationMode       type;
  gboolean              mode_affect = FALSE;
  guchar               *s, *s1, *s2;
  guchar               *d;
  const guchar         *m;
  guchar               *buf;
  gboolean              opacity_quickskip_possible;
  gboolean              transparency_quickskip_possible;
  TileRowHint           hint;

  /* use src2->bytes + 1 since DISSOLVE always needs a buffer with alpha */
  buf = g_alloca (MAX (MAX (src1->w * src1->bytes,
                            src2->w * (src2->bytes + 1)),
                       dest->w * dest->bytes));

  opacity    = st->opacity;
  mode       = st->mode;
  affect     = st->affect;
  type       = st->type;
  data       = st->data;

  opacity_quickskip_possible = (st->opacity_quickskip_possible &&
                                src2->tiles);
  transparency_quickskip_possible = (st->transparency_quickskip_possible &&
                                     src2->tiles);

  s1 = src1->data;
  s2 = src2->data;
  d = dest->data;
  m = mask ? mask->data : NULL;

  if (transparency_quickskip_possible || opacity_quickskip_possible)
    {
#ifdef HINTS_SANITY
      if (src1->h != src2->h)
        g_error("HEIGHTS SUCK!!");
      if (src1->offy != dest->offy)
        g_error("SRC1 OFFSET != DEST OFFSET");
#endif
      update_tile_rowhints (src2->curtile,
                            src2->offy, src2->offy + (src1->h - 1));
    }
  /* else it's probably a brush-composite */

  /*  use separate variables for the combining opacity and the opacity
   *  the layer mode is applied with since DISSLOVE_MODE "consumes"
   *  all opacity and wants to be applied OPAQUE
   */
  layer_mode_opacity = opacity;
  layer_mode_mask    = m;

  if (mode == GIMP_DISSOLVE_MODE)
    {
      opacity = OPAQUE_OPACITY;
      m       = NULL;
    }

  for (h = 0; h < src1->h; h++)
    {
      hint = TILEROWHINT_UNDEFINED;

      if (transparency_quickskip_possible)
        {
          hint = tile_get_rowhint (src2->curtile, (src2->offy + h));

          if (hint == TILEROWHINT_TRANSPARENT)
            {
              goto next_row;
            }
        }
      else
        {
          if (opacity_quickskip_possible)
            {
              hint = tile_get_rowhint (src2->curtile, (src2->offy + h));
            }
        }

      s = buf;

      /*  apply the paint mode based on the combination type & mode  */
      switch (type)
        {
        case COMBINE_INTEN_A_INDEXED:
        case COMBINE_INTEN_A_INDEXED_A:
        case COMBINE_INTEN_A_CHANNEL_MASK:
        case COMBINE_INTEN_A_CHANNEL_SELECTION:
          combine = type;
          break;

        case COMBINE_INDEXED_INDEXED:
        case COMBINE_INDEXED_INDEXED_A:
        case COMBINE_INDEXED_A_INDEXED_A:
          /*  Now, apply the paint mode--for indexed images  */
          combine = apply_indexed_layer_mode (s1, s2, &s, mode, type);
          break;

        case COMBINE_INTEN_INTEN_A:
        case COMBINE_INTEN_A_INTEN:
        case COMBINE_INTEN_INTEN:
        case COMBINE_INTEN_A_INTEN_A:
          {
            /*  Now, apply the paint mode  */

            if (gimp_composite_options.bits & GIMP_COMPOSITE_OPTION_USE)
              {
                GimpCompositeContext ctx;

                ctx.A             = s1;
                ctx.pixelformat_A = (src1->bytes == 1 ? GIMP_PIXELFORMAT_V8    :
                                     src1->bytes == 2 ? GIMP_PIXELFORMAT_VA8   :
                                     src1->bytes == 3 ? GIMP_PIXELFORMAT_RGB8  :
                                     src1->bytes == 4 ? GIMP_PIXELFORMAT_RGBA8 :
                                     GIMP_PIXELFORMAT_ANY);

                ctx.B             = s2;
                ctx.pixelformat_B = (src2->bytes == 1 ? GIMP_PIXELFORMAT_V8    :
                                     src2->bytes == 2 ? GIMP_PIXELFORMAT_VA8   :
                                     src2->bytes == 3 ? GIMP_PIXELFORMAT_RGB8  :
                                     src2->bytes == 4 ? GIMP_PIXELFORMAT_RGBA8 :
                                     GIMP_PIXELFORMAT_ANY);

                ctx.D             = s;
                ctx.pixelformat_D = ctx.pixelformat_A;

                ctx.M             = layer_mode_mask;
                ctx.pixelformat_M = GIMP_PIXELFORMAT_ANY;

                ctx.n_pixels      = src1->w;
                ctx.combine       = combine;
                ctx.op            = mode;

                ctx.dissolve.x       = src1->x;
                ctx.dissolve.y       = src1->y + h;
                ctx.dissolve.opacity = layer_mode_opacity;

                mode_affect =
                  gimp_composite_operation_effects[mode].affect_opacity;

                gimp_composite_dispatch (&ctx);

                s = ctx.D;
                combine = (ctx.combine == NO_COMBINATION) ? type : ctx.combine;
              }
            else
              {
                struct apply_layer_mode_struct alms;

                alms.src1    = s1;
                alms.src2    = s2;
                alms.mask    = layer_mode_mask;
                alms.dest    = &s;
                alms.x       = src1->x;
                alms.y       = src1->y + h;
                alms.opacity = layer_mode_opacity;
                alms.combine = combine;
                alms.length  = src1->w;
                alms.bytes1  = src1->bytes;
                alms.bytes2  = src2->bytes;

                /*  Determine whether the alpha channel of the destination
                 *  can be affected by the specified mode. -- This keeps
                 *  consistency with varying opacities.
                 */
                mode_affect = layer_modes[mode].affect_alpha;

                layer_mode_funcs[mode] (&alms);

                combine = (alms.combine == NO_COMBINATION ?
                           type : alms.combine);
              }
          }
          break;

        default:
          g_warning ("combine_sub_region: unhandled combine-type.");
          break;
        }

      /*  based on the type of the initial image...  */
      switch (combine)
        {
        case COMBINE_INDEXED_INDEXED:
          combine_indexed_and_indexed_pixels (s1, s2, d, m, opacity,
                                              affect, src1->w,
                                              src1->bytes);
          break;

        case COMBINE_INDEXED_INDEXED_A:
          combine_indexed_and_indexed_a_pixels (s1, s2, d, m, opacity,
                                                affect, src1->w,
                                                src1->bytes);
          break;

        case COMBINE_INDEXED_A_INDEXED_A:
          combine_indexed_a_and_indexed_a_pixels (s1, s2, d, m, opacity,
                                                  affect, src1->w,
                                                  src1->bytes);
          break;

        case COMBINE_INTEN_A_INDEXED:
          /*  assume the data passed to this procedure is the
           *  indexed layer's colormap
           */
          combine_inten_a_and_indexed_pixels (s1, s2, d, m, data, opacity,
                                              src1->w, dest->bytes);
          break;

        case COMBINE_INTEN_A_INDEXED_A:
          /*  assume the data passed to this procedure is the
           *  indexed layer's colormap
           */
          combine_inten_a_and_indexed_a_pixels (s1, s2, d, m, data, opacity,
                                                src1->w, dest->bytes);
          break;

        case COMBINE_INTEN_A_CHANNEL_MASK:
          /*  assume the data passed to this procedure is the
           *  indexed layer's colormap
           */
          combine_inten_a_and_channel_mask_pixels (s1, s2, d, data, opacity,
                                                   src1->w, dest->bytes);
          break;

        case COMBINE_INTEN_A_CHANNEL_SELECTION:
          combine_inten_a_and_channel_selection_pixels (s1, s2, d, data,
                                                        opacity,
                                                        src1->w,
                                                        src1->bytes);
          break;

        case COMBINE_INTEN_INTEN:
          if ((hint == TILEROWHINT_OPAQUE) &&
              opacity_quickskip_possible)
            {
              memcpy (d, s, dest->w * dest->bytes);
            }
          else
            combine_inten_and_inten_pixels (s1, s, d, m, opacity,
                                            affect, src1->w, src1->bytes);
          break;

        case COMBINE_INTEN_INTEN_A:
          combine_inten_and_inten_a_pixels (s1, s, d, m, opacity,
                                            affect, src1->w, src1->bytes);
          break;

        case COMBINE_INTEN_A_INTEN:
          combine_inten_a_and_inten_pixels (s1, s, d, m, opacity,
                                            affect, mode_affect, src1->w,
                                            src1->bytes);
          break;

        case COMBINE_INTEN_A_INTEN_A:
          if ((hint == TILEROWHINT_OPAQUE) &&
              opacity_quickskip_possible)
            {
              memcpy (d, s, dest->w * dest->bytes);
            }
          else
            combine_inten_a_and_inten_a_pixels (s1, s, d, m, opacity,
                                                affect, mode_affect,
                                                src1->w, src1->bytes);
          break;

        case BEHIND_INTEN:
          behind_inten_pixels (s1, s, d, m, opacity,
                               affect, src1->w, src1->bytes,
                               src2->bytes);
          break;

        case BEHIND_INDEXED:
          behind_indexed_pixels (s1, s, d, m, opacity,
                                 affect, src1->w, src1->bytes,
                                 src2->bytes);
          break;

        case REPLACE_INTEN:
          replace_inten_pixels (s1, s, d, m, opacity,
                                affect, src1->w, src1->bytes,
                                src2->bytes);
          break;

        case REPLACE_INDEXED:
          replace_indexed_pixels (s1, s, d, m, opacity,
                                  affect, src1->w, src1->bytes,
                                  src2->bytes);
          break;

        case ERASE_INTEN:
          erase_inten_pixels (s1, s, d, m, opacity,
                              affect, src1->w, src1->bytes);
          break;

        case ERASE_INDEXED:
          erase_indexed_pixels (s1, s, d, m, opacity,
                                affect, src1->w, src1->bytes);
          break;

        case ANTI_ERASE_INTEN:
          anti_erase_inten_pixels (s1, s, d, m, opacity,
                                   affect, src1->w, src1->bytes);
          break;

        case ANTI_ERASE_INDEXED:
          anti_erase_indexed_pixels (s1, s, d, m, opacity,
                                     affect, src1->w, src1->bytes);
          break;

        case COLOR_ERASE_INTEN:
          color_erase_inten_pixels (s1, s, d, m, opacity,
                                    affect, src1->w, src1->bytes);
          break;

        case NO_COMBINATION:
          g_warning("NO_COMBINATION");
          break;

        default:
          g_warning("UNKNOWN COMBINATION: %d", combine);
          break;
        }

    next_row:
      s1 += src1->rowstride;
      s2 += src2->rowstride;
      d += dest->rowstride;
      if (mask)
        {
          layer_mode_mask += mask->rowstride;

          if (m)
            m += mask->rowstride;
        }
    }
}


void
combine_regions (PixelRegion          *src1,
                 PixelRegion          *src2,
                 PixelRegion          *dest,
                 PixelRegion          *mask,
                 const guchar         *data,
                 guint                 opacity,
                 GimpLayerModeEffects  mode,
                 const gboolean       *affect,
                 CombinationMode       type)
{
  gboolean has_alpha1, has_alpha2;
  guint i;
  struct combine_regions_struct st;

  /*  Determine which sources have alpha channels  */
  switch (type)
    {
    case COMBINE_INTEN_INTEN:
    case COMBINE_INDEXED_INDEXED:
      has_alpha1 = has_alpha2 = FALSE;
      break;
    case COMBINE_INTEN_A_INTEN:
    case COMBINE_INTEN_A_INDEXED:
      has_alpha1 = TRUE;
      has_alpha2 = FALSE;
      break;
    case COMBINE_INTEN_INTEN_A:
    case COMBINE_INDEXED_INDEXED_A:
      has_alpha1 = FALSE;
      has_alpha2 = TRUE;
      break;
    case COMBINE_INTEN_A_INTEN_A:
    case COMBINE_INDEXED_A_INDEXED_A:
      has_alpha1 = has_alpha2 = TRUE;
      break;
    default:
      has_alpha1 = has_alpha2 = FALSE;
    }

  st.opacity    = opacity;
  st.mode       = mode;
  st.affect     = affect;
  st.type       = type;
  st.data       = data;

  /* cheap and easy when the row of src2 is completely opaque/transparent
     and the wind is otherwise blowing in the right direction.
  */

  /* First check - we can't do an opacity quickskip if the drawable
     has a mask, or non-full opacity, or the layer mode dictates
     that we might gain transparency.
  */
  st.opacity_quickskip_possible = ((!mask)                               &&
                                   (opacity == 255)                      &&
                                   (!layer_modes[mode].decrease_opacity) &&
                                   (layer_modes[mode].affect_alpha       &&
                                    has_alpha1                           &&
                                    affect[src1->bytes - 1]));

  /* Second check - if any single colour channel can't be affected,
     we can't use the opacity quickskip.
  */
  if (st.opacity_quickskip_possible)
    {
      for (i = 0; i < src1->bytes - 1; i++)
        {
          if (!affect[i])
            {
              st.opacity_quickskip_possible = FALSE;
              break;
            }
        }
    }

  /* transparency quickskip is only possible if the layer mode
     dictates that we cannot possibly gain opacity, or the 'overall'
     opacity of the layer is set to zero anyway.
   */
  st.transparency_quickskip_possible = ((!layer_modes[mode].increase_opacity)
                                        || (opacity==0));

  /* Start the actual processing.
   */
  pixel_regions_process_parallel ((PixelProcessorFunc) combine_sub_region,
                                  &st, 4, src1, src2, dest, mask);
}

void
combine_regions_replace (PixelRegion     *src1,
                         PixelRegion     *src2,
                         PixelRegion     *dest,
                         PixelRegion     *mask,
                         const guchar    *data,
                         guint            opacity,
                         const gboolean  *affect,
                         CombinationMode  type)
{
  gpointer pr;

  for (pr = pixel_regions_register (4, src1, src2, dest, mask);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar  *s1 = src1->data;
      const guchar  *s2 = src2->data;
      guchar        *d  = dest->data;
      const guchar  *m  = mask->data;
      guint          h;

      for (h = 0; h < src1->h; h++)
        {
          /*  Now, apply the paint mode  */
          apply_layer_mode_replace (s1, s2, d, m, src1->x, src1->y + h,
                                    opacity, src1->w,
                                    src1->bytes, src2->bytes, affect);

          s1 += src1->rowstride;
          s2 += src2->rowstride;
          d += dest->rowstride;
          m += mask->rowstride;
        }
    }
}

static void
apply_layer_mode_replace (const guchar   *src1,
                          const guchar   *src2,
                          guchar         *dest,
                          const guchar   *mask,
                          gint            x,
                          gint            y,
                          guint           opacity,
                          guint           length,
                          guint           bytes1,
                          guint           bytes2,
                          const gboolean *affect)
{
  replace_pixels (src1, src2, dest, mask, length,
                  opacity, affect, bytes1, bytes2);
}
