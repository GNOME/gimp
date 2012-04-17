/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core/core-types.h" /* eek, but this file will die anyway */

#include "core/gimptempbuf.h"

#include "base/pixel-processor.h"
#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile-rowhints.h"
#include "base/tile.h"

#include "composite/gimp-composite.h"

#include "paint-funcs.h"
#include "paint-funcs-utils.h"
#include "paint-funcs-generic.h"


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


static const guchar  no_mask = OPAQUE_OPACITY;


/*  Local function prototypes  */

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
combine_indexed_and_indexed_pixels (const guchar   *src1,
                                    const guchar   *src2,
                                    guchar         *dest,
                                    const guchar   *mask,
                                    const guint     opacity,
                                    const gboolean *affect,
                                    guint           length,
                                    const guint     bytes)
{
  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          register gulong tmp;
          const guchar    new_alpha = INT_MULT (*m , opacity, tmp);
          guint           b;

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
          guint b;

          for (b = 0; b < bytes; b++)
            dest[b] = (affect[b] && opacity > 127) ? src2[b] : src1[b];

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
                                      const guint     opacity,
                                      const gboolean *affect,
                                      guint           length,
                                      const guint     bytes)
{
  const gint alpha      = 1;
  const gint src2_bytes = 2;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          register gulong tmp;
          const guchar    new_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);
          guint           b;

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
          register gulong tmp;
          const guchar    new_alpha = INT_MULT (src2[alpha], opacity, tmp);
          guint           b;

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
                                        const guint     opacity,
                                        const gboolean *affect,
                                        guint           length,
                                        const guint     bytes)
{
  const gint alpha = 1;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          register gulong tmp;
          const guchar    new_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);
          guint           b;

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
          register gulong tmp;
          const guchar    new_alpha = INT_MULT (src2[alpha], opacity, tmp);
          guint           b;

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
                                    const guint   opacity,
                                    guint         length,
                                    const guint   bytes)
{
  const gint src2_bytes = 1;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          register gulong tmp;
          const guint     index     = src2[0] * 3;
          const guchar    new_alpha = INT_MULT3 (255, *m, opacity, tmp);
          guint           b;

          for (b = 0; b < bytes - 1; b++)
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
          register gulong tmp;
          const guint     index     = src2[0] * 3;
          const guchar    new_alpha = INT_MULT (255, opacity, tmp);
          guint           b;

          for (b = 0; b < bytes - 1; b++)
            dest[b] = (new_alpha > 127) ? cmap[index + b] : src1[b];

          dest[b] = (new_alpha > 127) ? OPAQUE_OPACITY : src1[b];
          /*  alpha channel is opaque  */

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
                                      const guint   opacity,
                                      guint         length,
                                      const guint   bytes)
{
  const gint alpha      = 1;
  const gint src2_bytes = 2;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          register gulong tmp;
          guint           index     = src2[0] * 3;
          const guchar    new_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);
          guint           b;

          for (b = 0; b < bytes - 1; b++)
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
          register gulong tmp;
          guint           index     = src2[0] * 3;
          const guchar    new_alpha = INT_MULT (src2[alpha], opacity, tmp);
          guint           b;

          for (b = 0; b < bytes - 1; b++)
            dest[b] = (new_alpha > 127) ? cmap[index + b] : src1[b];

          dest[b] = (new_alpha > 127) ? OPAQUE_OPACITY : src1[b];
          /*  alpha channel is opaque  */

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
                                const guint     opacity,
                                const gboolean *affect,
                                guint           length,
                                const guint     bytes)
{
  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          register gulong tmp;
          const guchar    new_alpha = INT_MULT (*m, opacity, tmp);
          guint           b;

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
          register gulong tmp;
          guint           b;

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
                                  const guint     opacity,
                                  const gboolean *affect,
                                  guint           length,
                                  const guint     bytes)
{
  const gint alpha      = bytes;
  const gint src2_bytes = bytes + 1;

  if (mask)
    {
      const guchar *m = mask;

      while (length --)
        {
          register glong  t1;
          const guchar    new_alpha = INT_MULT3 (src2[alpha], *m, opacity, t1);
          guint           b;

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
              register glong  t1;
              const guchar    new_alpha = INT_MULT (src2[alpha], opacity, t1);

              dest[0] = INT_BLEND (src2[0], src1[0], new_alpha, t1);
              dest[1] = INT_BLEND (src2[1], src1[1], new_alpha, t1);
              dest[2] = INT_BLEND (src2[2], src1[2], new_alpha, t1);

              src1 += bytes;
              src2 += src2_bytes;
              dest += bytes;
            }
        }
      else
        {
          while (length --)
            {
              register glong  t1;
              const guchar    new_alpha = INT_MULT (src2[alpha], opacity, t1);
              guint           b;

              for (b = 0; b < bytes; b++)
                dest[b] = (affect[b] ?
                           INT_BLEND (src2[b], src1[b], new_alpha, t1) :
                           src1[b]);

              src1 += bytes;
              src2 += src2_bytes;
              dest += bytes;
            }
        }
    }
}

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

void
combine_inten_a_and_inten_pixels (const guchar   *src1,
                                  const guchar   *src2,
                                  guchar         *dest,
                                  const guchar   *mask,
                                  const guint     opacity,
                                  const gboolean *affect,
                                  const gboolean  mode_affect,  /*  how does the combination mode affect alpha?  */
                                  guint           length,
                                  const guint     bytes)        /*  4 or 2 depending on RGBA or GRAYA  */
{
  const gint src2_bytes = bytes - 1;
  const gint alpha      = bytes - 1;
  gint       b;
  gfloat     ratio;
  gfloat     compl_ratio;

  if (mask)
    {
      const guchar *m = mask;

      if (opacity == OPAQUE_OPACITY) /* HAS MASK, FULL OPACITY */
        {
          while (length--)
            {
              register gulong tmp;
              guchar          src2_alpha = *m;
              guchar          new_alpha  =
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
              register gulong tmp;
              guchar          src2_alpha = INT_MULT (*m, opacity, tmp);
              guchar          new_alpha  =
                src1[alpha] + INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

              alphify (src2_alpha, new_alpha);

              if (mode_affect)
                {
                  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
                }
              else
                {
                  dest[alpha] = (src1[alpha] ?
                                 src1[alpha] : (affect[alpha] ?
                                                new_alpha : src1[alpha]));
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
          register gulong tmp;
          guchar          src2_alpha = opacity;
          guchar          new_alpha  =
            src1[alpha] + INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

          alphify (src2_alpha, new_alpha);

          if (mode_affect)
            dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
          else
            dest[alpha] = (src1[alpha] ?
                           src1[alpha] : (affect[alpha] ?
                                          new_alpha : src1[alpha]));

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
                                    const guint     opacity,
                                    const gboolean *affect,
                                    const gboolean  mode_affect,  /*  how does the combination mode affect alpha?  */
                                    guint           length,
                                    const guint     bytes)  /*  4 or 2 depending on RGBA or GRAYA  */
{
  const guint alpha = bytes - 1;
  guint       b;
  gfloat      ratio;
  gfloat      compl_ratio;

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
                      register gulong  tmp;
                      guchar src2_alpha = INT_MULT (src2[alpha], *m, tmp);
                      guchar new_alpha  =
                        src1[alpha] +
                        INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

                      alphify (src2_alpha, new_alpha);

                      if (mode_affect)
                        {
                          dest[alpha] = (affect[alpha] ?
                                         new_alpha : src1[alpha]);
                        }
                      else
                        {
                          dest[alpha] = (src1[alpha] ?
                                         src1[alpha] : (affect[alpha] ?
                                                        new_alpha : src1[alpha]));
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
                          register gulong  tmp;
                          guchar src2_alpha = INT_MULT (src2[alpha], *m, tmp);
                          guchar new_alpha  =
                            src1[alpha] +
                            INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

                          alphify (src2_alpha, new_alpha);

                          if (mode_affect)
                            {
                              dest[alpha] = (affect[alpha] ?
                                             new_alpha : src1[alpha]);
                            }
                          else
                            {
                              dest[alpha] = (src1[alpha] ?
                                             src1[alpha] : (affect[alpha] ?
                                                            new_alpha : src1[alpha]));
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
              register gulong  tmp;
              guchar src2_alpha = INT_MULT (src2[alpha], *m, tmp);
              guchar new_alpha  =
                src1[alpha] + INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

              alphify (src2_alpha, new_alpha);

              if (mode_affect)
                {
                  dest[alpha] = affect[alpha] ? new_alpha : src1[alpha];
                }
              else
                {
                  dest[alpha] = (src1[alpha] ?
                                 src1[alpha] : (affect[alpha] ?
                                                new_alpha : src1[alpha]));
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
                      register gulong  tmp;
                      guchar src2_alpha = INT_MULT3 (src2[alpha], *m, opacity,
                                                     tmp);
                      guchar new_alpha  =
                        src1[alpha] +
                        INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

                      alphify (src2_alpha, new_alpha);

                      if (mode_affect)
                        {
                          dest[alpha] = (affect[alpha] ?
                                         new_alpha : src1[alpha]);
                        }
                      else
                        {
                          dest[alpha] = (src1[alpha] ?
                                         src1[alpha] : (affect[alpha] ?
                                                        new_alpha : src1[alpha]));
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
                          register gulong  tmp;
                          guchar src2_alpha = INT_MULT3 (src2[alpha],
                                                         *m, opacity, tmp);
                          guchar new_alpha  =
                            src1[alpha] +
                            INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

                          alphify (src2_alpha, new_alpha);

                          if (mode_affect)
                            {
                              dest[alpha] = (affect[alpha] ?
                                             new_alpha : src1[alpha]);
                            }
                          else
                            {
                              dest[alpha] = (src1[alpha] ?
                                             src1[alpha] : (affect[alpha] ?
                                                            new_alpha : src1[alpha]));
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
              register gulong  tmp;
              guchar src2_alpha = INT_MULT3 (src2[alpha], *m, opacity, tmp);
              guchar new_alpha  =
                src1[alpha] +
                INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

              alphify (src2_alpha, new_alpha);

              if (mode_affect)
                {
                  dest[alpha] = affect[alpha] ? new_alpha : src1[alpha];
                }
              else
                {
                  dest[alpha] = (src1[alpha] ?
                                 src1[alpha] : (affect[alpha] ?
                                                new_alpha : src1[alpha]));
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
              register gulong  tmp;
              guchar src2_alpha = src2[alpha];
              guchar new_alpha  =
                src1[alpha] +
                INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

              alphify (src2_alpha, new_alpha);

              if (mode_affect)
                {
                  dest[alpha] = affect[alpha] ? new_alpha : src1[alpha];
                }
              else
                {
                  dest[alpha] = (src1[alpha] ?
                                 src1[alpha] : (affect[alpha] ?
                                                new_alpha : src1[alpha]));
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
              register gulong  tmp;
              guchar src2_alpha = INT_MULT (src2[alpha], opacity, tmp);
              guchar new_alpha  =
                src1[alpha] + INT_MULT ((255 - src1[alpha]), src2_alpha, tmp);

              alphify (src2_alpha, new_alpha);

              if (mode_affect)
                {
                  dest[alpha] = affect[alpha] ? new_alpha : src1[alpha];
                }
              else
                {
                  dest[alpha] = (src1[alpha] ?
                                 src1[alpha] : (affect[alpha] ?
                                                new_alpha : src1[alpha]));
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
                                         const guint   opacity,
                                         guint         length,
                                         const guint   bytes)
{
  const gint alpha = bytes - 1;

  while (length --)
    {
      register gulong t;
      guchar channel_alpha = INT_MULT (255 - *channel, opacity, t);

      if (channel_alpha)
        {
          register gulong s;
          const guchar new_alpha =
            src[alpha] + INT_MULT ((255 - src[alpha]), channel_alpha, t);
          guchar       compl_alpha;
          guint        b;

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
      src += bytes;
      dest += bytes;
      channel++;
    }
}


void
combine_inten_a_and_channel_selection_pixels (const guchar *src,
                                              const guchar *channel,
                                              guchar       *dest,
                                              const guchar *col,
                                              const guint   opacity,
                                              guint         length,
                                              const guint   bytes)
{
  const gint alpha = bytes - 1;

  while (length --)
    {
      register gulong t;
      guchar          channel_alpha = INT_MULT (*channel, opacity, t);

      if (channel_alpha)
        {
          register gulong s;
          const guchar new_alpha =
            src[alpha] + INT_MULT ((255 - src[alpha]), channel_alpha, t);
          guchar       compl_alpha;
          guint        b;

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
          memcpy (dest, src, bytes);
        }

      /*  advance pointers  */
      src += bytes;
      dest += bytes;
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

#define INT_DIV(a, b) ((a)/(b) + (((a) % (b)) > ((b) / 2)))

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
  const guint   has_alpha1 = HAS_ALPHA (bytes1);
  const guint   has_alpha2 = HAS_ALPHA (bytes2);
  const guint   alpha      = bytes1 - has_alpha1;
  const guint   alpha2     = bytes2 - has_alpha2;
  const guchar *m          = mask ? mask : &no_mask;
  guint         b;
  gint          tmp;

  while (length --)
    {
      guchar src1_alpha  = has_alpha1 ? src1[alpha]  : 255;
      guchar src2_alpha  = has_alpha2 ? src2[alpha2] : 255;
      guchar new_alpha   = INT_BLEND (src2_alpha, src1_alpha,
                                      INT_MULT (*m, opacity, tmp), tmp);

      if (new_alpha)
        {
          guint ratio = *m * opacity;
          ratio = ratio / 255 * src2_alpha;

          ratio = INT_DIV (ratio, new_alpha);

          for (b = 0; b < alpha; b++)
            {
              if (! affect[b])
                {
                  dest[b] = src1[b];
                }
              else if (src2[b] > src1[b])
                {
                  guint t = (src2[b] - src1[b]) * ratio;
                  dest[b] = src1[b] + INT_DIV (t, 255);
                }
              else
                {
                  guint t = (src1[b] - src2[b]) * ratio;
                  dest[b] = src1[b] - INT_DIV (t, 255);
                }
            }
        }
      else
        {
          for (b = 0; b < alpha; b++)
            dest[b] = src1[b];
        }

      if (has_alpha1)
        dest[alpha] = affect[alpha] ? new_alpha : src1[alpha];

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


void
paint_funcs_color_erase_helper (GimpRGB       *src,
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

          paint_funcs_color_erase_helper (&color, &bgcolor);

          gimp_rgba_get_uchar (&color, dest, NULL, NULL, dest + 1);
          break;

        case 4:
          src2_alpha = INT_MULT3 (src2[3], *m, opacity, tmp);

          gimp_rgba_set_uchar (&color,
                               src1[0], src1[1], src1[2], src1[3]);

          gimp_rgba_set_uchar (&bgcolor,
                               src2[0], src2[1], src2[2], src2_alpha);

          paint_funcs_color_erase_helper (&color, &bgcolor);

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


/**************************************************/
/*    REGION FUNCTIONS                            */
/**************************************************/

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
      tile_update_rowhints (src2->curtile, src2->offy, src1->h);
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
  gboolean has_alpha1;
  guint i;
  struct combine_regions_struct st;

  /*  Determine which sources have alpha channels  */
  switch (type)
    {
    case COMBINE_INTEN_INTEN:
    case COMBINE_INDEXED_INDEXED:
      has_alpha1 = FALSE;
      break;
    case COMBINE_INTEN_A_INTEN:
    case COMBINE_INTEN_A_INDEXED:
      has_alpha1 = TRUE;
      break;
    case COMBINE_INTEN_INTEN_A:
    case COMBINE_INDEXED_INDEXED_A:
      has_alpha1 = FALSE;
      break;
    case COMBINE_INTEN_A_INTEN_A:
    case COMBINE_INDEXED_A_INDEXED_A:
      has_alpha1 = TRUE;
      break;
    default:
      has_alpha1 = FALSE;
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
