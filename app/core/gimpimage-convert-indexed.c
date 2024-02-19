/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimage-convert-indexed.c
 * Copyright (C) 1997-2004 Adam D. Moss <adam@gimp.org>
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
 * 2024-02-20 - Do error diffusion in linear RGB, color qunatization
 *   still in CIE Lab. Most prominent when dithering to 1bit black/white
 *   the resulting image when averaged in linear RGB / displayed had
 *   the wrong gamma when doing dithering in CIE Lab, dithering is
 *   now done in 16bit linear RGB - and the use of 8bpc optimization
 *   tables is gone.
 *
 * 2020-05-01 - snap black and white to black and white [pippin]
 *
 * 2005-09-04 - Switch 'positional' dither matrix to a 32x32 Bayer,
 *  which generates results that compress somewhat better (and may look
 *  worse or better depending on what you enjoy...).  [adam@gimp.org]
 *
 * 2004-12-12 - Use a slower but much nicer technique for finding the
 *  two best colors to dither between when using fixed/positional
 *  dither methods.  Makes positional dither much less lame.  [adam@gimp.org]
 *
 * 2002-02-10 - Quantizer version 3.0 (the rest of the commit started
 *  a year ago -- whoops).  Divide colors within CIE L*a*b* space using
 *  CPercep module (cpercep.[ch]), color-match and dither likewise,
 *  change the underlying box selection criteria and division point
 *  logic, bump luminance precision upwards, etc.etc.  Generally
 *  chooses a much richer color set, especially for low numbers of
 *  colors.  n.b.: Less luminance-sloppy in straight remapping which is
 *  good for color but a bit worse for high-frequency detail (that's
 *  partly what fs-dithering is for -- use it).  [adam@gimp.org]
 *
 * 2001-03-25 - Define accessor function/macro for histogram reads and
 *  writes.  This slows us down a little because we avoid some of the
 *  dirty tricks we used when we knew that the histogram was a straight
 *  3d array, so I've recovered some of the speed loss by implementing
 *  a 5d accessor function with good locality of reference.  This change
 *  is the first step towards quantizing in a more interesting colorspace
 *  than frumpy old RGB.  [Adam]
 *
 * 2000/01/30 - Use palette_selector instead of option_menu for custom
 *  palette. Use libgimp callback functions.  [Sven]
 *
 * 99/09/01 - Created a low-bleed FS-dither option.  [Adam]
 *
 * 99/08/29 - Deterministic color dithering to arbitrary palettes.
 *  Ideal for animations that are going to be delta-optimized or simply
 *  don't want to look 'busy' in static areas.  Also a bunch of bugfixes
 *  and tweaks.  [Adam]
 *
 * 99/08/28 - Deterministic alpha dithering over layers, reduced bleeding
 *  of transparent values into opaque values, added optional stage to
 *  remove duplicate or unused color entries from final colormap. [Adam]
 *
 * 99/02/24 - Many revisions to the box-cut quantizer used in RGB->INDEXED
 *  conversion.  Box to be cut is chosen on the basis of possessing an axis
 *  with the largest sum of weighted perceptible error, rather than based on
 *  volume or population.  The box is split along this axis rather than its
 *  longest axis, at the point of error mean rather than simply at its centre.
 *  Error-limiting in the F-S dither has been disabled - it may become optional
 *  again later.  If you're convinced that you have an image where the old
 *  dither looks better, let me know.  [Adam]
 *
 * 99/01/10 - Hourglass... [Adam]
 *
 * 98/07/25 - Convert-to-indexed now remembers the last invocation's
 *  settings.  Also, GRAY->INDEXED is more flexible.  [Adam]
 *
 * 98/07/05 - Sucked the warning about quantizing to too many colors into
 *  a text widget embedded in the dialog, improved intelligence of dialog
 *  to default 'custom palette' selection to 'Web' if available, and
 *  in this case not bother to present the native WWW-palette radio
 *  button.  [Adam]
 *
 * 98/04/13 - avoid a division by zero when converting an empty gray-scale
 *  image (who would like to do such a thing anyway??)  [Sven ]
 *
 * 98/03/23 - fixed a longstanding fencepost - hopefully the *right*
 *  way, *again*.  [Adam]
 *
 * 97/11/14 - added a proper pdb interface and support for dithering
 *  to custom palettes (based on a patch by Eric Hernes) [Yosh]
 *
 * 97/11/04 - fixed the accidental use of the color-counting case
 *  when palette_type is WEB or MONO. [Adam]
 *
 * 97/10/25 - color-counting implemented (could use some hashing, but
 *  performance actually seems okay) - now RGB->INDEXED conversion isn't
 *  destructive if it doesn't have to be. [Adam]
 *
 * 97/10/14 - fixed divide-by-zero when converting a completely transparent
 *  RGB image to indexed. [Adam]
 *
 * 97/07/01 - started todo/revision log.  Put code back in to
 *  eliminate full-alpha pixels from RGB histogram.
 *  [Adam D. Moss - adam@gimp.org]
 */

  /* TODO for Convert:
   *
   * . Tweak, tweak, tweak.  Old RGB code was tuned muchly.
   *
   * . Re-enable Heckbert locality for matching, benchmark it
   *
   * . Try faster fixed-point sRGB<->L*a*b* pixel conversion (see cpercep.c)
   *
   * . Use palette of another open INDEXED image?
   *
   * . Do error-splitting trick for GREY->INDEXED (hardly worth it)
   */

  /* CODE READABILITY BUGS:
   *
   * . Most uses of variants of the R,G,B variable naming convention
   *   are referring to L*a*b* coordinates, not RGB coordinates!
   *
   * . Each said variable is usually one of the following, but it is
   *   rarely clear which one:
   *     - (assumed sRGB) raw non-linear 8-bit RGB coordinates
   *     - 'full'-precision (unshifted) 8-bit L*a*b* coordinates
   *     - box-space (reduced-precision shifted L*a*b*) coordinates
   */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpdrawable.h"
#include "gimperror.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpimage-colormap.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimpobjectqueue.h"
#include "gimppalette.h"
#include "gimpprogress.h"

#include "text/gimptextlayer.h"

#include "gimpimage-convert-data.h"
#include "gimpimage-convert-indexed.h"

#include "gimp-intl.h"


/* basic memory/quality tradeoff */
#define PRECISION_R 8
#define PRECISION_G 8
#define PRECISION_B 8

#define HIST_R_ELEMS (1<<PRECISION_R)
#define HIST_G_ELEMS (1<<PRECISION_G)
#define HIST_B_ELEMS (1<<PRECISION_B)

#define BITS_IN_SAMPLE 8

#define R_SHIFT  (BITS_IN_SAMPLE-PRECISION_R)
#define G_SHIFT  (BITS_IN_SAMPLE-PRECISION_G)
#define B_SHIFT  (BITS_IN_SAMPLE-PRECISION_B)

/* we've stretched our non-cubic L*a*b* volume to touch the
 * faces of the logical cube we've allocated for it, so re-scale
 * again in inverse proportion to get back to linear proportions.
 */
#define R_SCALE 13              /*  scale R (L*) distances by this much  */
#define G_SCALE 24              /*  scale G (a*) distances by this much  */
#define B_SCALE 26              /*  and B (b*) by this much              */


typedef struct _Color Color;
typedef struct _QuantizeObj QuantizeObj;

typedef void (* Pass1Func)     (QuantizeObj *quantize_obj);
typedef void (* Pass2InitFunc) (QuantizeObj *quantize_obj);
typedef void (* Pass2Func)     (QuantizeObj *quantize_obj,
                                GimpLayer   *layer,
                                GeglBuffer  *new_buffer);
typedef void (* CleanupFunc)   (QuantizeObj *quantize_obj);

typedef guint64 ColorFreq;
typedef ColorFreq * CFHistogram;

typedef enum { AXIS_UNDEF, AXIS_RED, AXIS_BLUE, AXIS_GREEN } AxisType;

typedef double etype;


/*
  We provide two different histogram access interfaces.  HIST_LIN()
  accesses the histogram in histogram-native space, taking absolute
  histogram coordinates.  HIST_RGB() accesses the histogram in RGB
  space.  This latter takes unsigned 8-bit coordinates, internally
  converts those coordinates to histogram-native space and returns
  the access pointer to the corresponding histogram cell.

  Using these two interfaces we can import RGB data into a more
  interesting space and efficiently work in the latter space until
  it is time to output the quantized values in RGB again.  For
  this final conversion we implement the function lin_to_rgb().

  We effectively pull our three-dimensional space into five dimensions
  such that the most-entropic bits lay in the lowest bits of the resulting
  array index.  This gives significantly better locality of reference
  and hence a small speedup despite the extra work involved in calculating
  the index.

  Why not six dimensions?  The expansion of dimensionality is good for random
  access such as histogram population and the query pattern typical of
  dithering but we have some code which iterates in a scanning manner, for
  which the expansion is suboptimal.  The compromise is to leave the B
  dimension unmolested in the lower-order bits of the index, since this is
  the dimension most commonly iterated through in the inner loop of the
  scans.

  --adam

  RhGhRlGlB
*/
#define VOL_GBITS  (PRECISION_G)
#define VOL_BBITS  (PRECISION_B)
#define VOL_RBITS  (PRECISION_R)
#define VOL_GBITSh (VOL_GBITS - 3)
#define VOL_GBITSl (VOL_GBITS - VOL_GBITSh)
#define VOL_BBITSh (VOL_BBITS - 4)
#define VOL_BBITSl (VOL_BBITS - VOL_BBITSh)
#define VOL_RBITSh (VOL_RBITS - 3)
#define VOL_RBITSl (VOL_RBITS - VOL_RBITSh)
#define VOL_GMASKh (((1<<VOL_GBITSh)-1) << VOL_GBITSl)
#define VOL_GMASKl ((1<<VOL_GBITSl)-1)
#define VOL_BMASKh (((1<<VOL_BBITSh)-1) << VOL_BBITSl)
#define VOL_BMASKl ((1<<VOL_BBITSl)-1)
#define VOL_RMASKh (((1<<VOL_RBITSh)-1) << VOL_RBITSl)
#define VOL_RMASKl ((1<<VOL_RBITSl)-1)
/* The 5d compromise thing. */
/*
#define REF_FUNC(r,g,b) \
( \
 (((r) & VOL_RMASKh) << (VOL_BBITS + VOL_GBITS)) | \
 (((r) & VOL_RMASKl) << (VOL_GBITSl + VOL_BBITS)) | \
 (((g) & VOL_GMASKh) << (VOL_RBITSl + VOL_BBITS)) | \
 (((g) & VOL_GMASKl) << (VOL_BBITS)) | \
 (b) \
)
*/
/* The full-on 6d thing. */
/*
#define REF_FUNC(r,g,b) \
( \
 (((r) & VOL_RMASKh) << (VOL_BBITS + VOL_GBITS)) | \
 (((r) & VOL_RMASKl) << (VOL_GBITSl + VOL_BBITSl)) | \
 (((g) & VOL_GMASKh) << (VOL_RBITSl + VOL_BBITS)) | \
 (((g) & VOL_GMASKl) << (VOL_BBITSl)) | \
 (((b) & VOL_BMASKh) << (VOL_RBITSl + VOL_GBITSl)) | \
 ((b) & VOL_BMASKl) \
)
*/
/* The boring old 3d thing. */

#define REF_FUNC(r,g,b) (((r)<<(PRECISION_G+PRECISION_B)) | ((g)<<(PRECISION_B)) | (b))


/* You even get to choose whether you want the accessor function
   implemented as a macro or an inline function.  Don't say I never
   give you anything. */
/*
#define HIST_LIN(hist_ptr,r,g,b) (&(hist_ptr)[REF_FUNC((r),(g),(b))])
*/
static inline ColorFreq *
HIST_LIN (ColorFreq  *hist_ptr,
          const gint  r,
          const gint  g,
          const gint  b)
{
  return (&(hist_ptr) [REF_FUNC (r, g, b)]);
}


#define LOWA   (-86.181F)
#define LOWB  (-107.858F)
#define HIGHA   (98.237F)
#define HIGHB   (94.480F)

#if 1
#define LRAT (2.55F)
#define ARAT (255.0F / (HIGHA - LOWA))
#define BRAT (255.0F / (HIGHB - LOWB))
#else
#define LRAT (1.0F)
#define ARAT (1.0F)
#define BRAT (1.0F)
#endif

static const Babl *rgb_to_lab_fish = NULL;
static const Babl *rgb_to_linear_fish = NULL;
static const Babl *gray_to_linear_fish = NULL;
static const Babl *linear_to_rgb_float_fish = NULL;
static const Babl *linear_to_gray_float_fish = NULL;
static const Babl *lab_to_rgb_fish = NULL;

static inline void
rgb_to_unshifted_lin (const guchar  r,
                      const guchar  g,
                      const guchar  b,
                      gint         *hr,
                      gint         *hg,
                      gint         *hb)
{
  gint   or, og, ob;
  gfloat rgb[3] = { r / 255.0, g / 255.0, b / 255.0 };
  gfloat lab[3];

  babl_process (rgb_to_lab_fish, rgb, lab, 1);

  /* fprintf(stderr, " %d-%d-%d -> %0.3f,%0.3f,%0.3f ", r, g, b, sL, sa, sb);*/

  or = RINT(lab[0] * LRAT);
  og = RINT((lab[1] - LOWA) * ARAT);
  ob = RINT((lab[2] - LOWB) * BRAT);

  *hr = CLAMP(or, 0, 255);
  *hg = CLAMP(og, 0, 255);
  *hb = CLAMP(ob, 0, 255);

  /*  fprintf(stderr, " %d:%d:%d ", *hr, *hg, *hb); */
}

static inline int
gray_to_linear (const guchar i)
{
  gfloat fi = i / 255.0f;
  guint16 o16;
  babl_process (gray_to_linear_fish, &fi, &o16, 1);
  return o16;
}

static inline int
linear_to_u8 (const gint i)
{
  guint16 i16 = i;
  float of;
  int ret;
  babl_process (linear_to_gray_float_fish, &i16, &of, 1);
  ret = of*255;
  if (ret <0) return 0;
  if (ret >255) return 255;
  return ret;
}

static inline void
rgb_to_linear (const guchar  r,
               const guchar  g,
               const guchar  b,
               gint         *hr,
               gint         *hg,
               gint         *hb)
{
  gfloat rgb[3] = { r / 255.0f, g / 255.0f, b / 255.0f };
  guint16 out[3];

  babl_process (rgb_to_linear_fish, rgb, out, 1);

  *hr = out[0];
  *hg = out[1];
  *hb = out[2];
}



static inline void
rgb_to_lin (const guchar  r,
            const guchar  g,
            const guchar  b,
            gint         *hr,
            gint         *hg,
            gint         *hb)
{
  gint or, og, ob;

  /*
  double sL, sa, sb;
  {
    double low_l = 999.0, low_a = 999.9, low_b = 999.0;
    double high_l = -999.0, high_a = -999.0, high_b = -999.0;

    int r,g,b;

    for (r=0; r<256; r++)
      for (g=0; g<256; g++)
        for (b=0; b<256; b++)
          {
            cpercep_rgb_to_space(r,g,b, &sL, &sa, &sb);

            if (sL > high_l)
              high_l = sL;
            if (sL < low_l)
              low_l = sL;
            if (sa > high_a)
              high_a = sa;
            if (sa < low_a)
              low_a = sa;
            if (sb > high_b)
              high_b = sb;
            if (sb < low_b)
              low_b = sb;
          }

    fprintf(stderr, " [L: %0.3f -> %0.3f / a: %0.3f -> %0.3f / b: %0.3f -> %0.3f]\t", low_l, high_l, low_a, high_a, low_b, high_b);

    exit(-1);
  }
  */

  rgb_to_unshifted_lin (r, g, b, &or, &og, &ob);

#if 0
#define RSDF(r) ((r) >= ((HIST_R_ELEMS-1) << R_SHIFT) ? HIST_R_ELEMS-1 : \
                 ((r) + ((1<<R_SHIFT)>>1) ) >> R_SHIFT)
#define GSDF(g) ((g) >= ((HIST_G_ELEMS-1) << G_SHIFT) ? HIST_G_ELEMS-1 : \
                 ((g) + ((1<<G_SHIFT)>>1) ) >> G_SHIFT)
#define BSDF(b) ((b) >= ((HIST_B_ELEMS-1) << B_SHIFT) ? HIST_B_ELEMS-1 : \
                 ((b) + ((1<<B_SHIFT)>>1) ) >> B_SHIFT)
#else
#define RSDF(r) ((r) >> R_SHIFT)
#define GSDF(g) ((g) >> G_SHIFT)
#define BSDF(b) ((b) >> B_SHIFT)
#endif

  or = RSDF (or);
  og = GSDF (og);
  ob = BSDF (ob);

  *hr = or;
  *hg = og;
  *hb = ob;
}


static inline ColorFreq *
HIST_RGB (ColorFreq  *hist_ptr,
          const gint  r,
          const gint  g,
          const gint  b)
{
  gint hr, hg, hb;

  rgb_to_lin (r, g, b, &hr, &hg, &hb);

  return HIST_LIN (hist_ptr, hr, hg, hb);
}


static inline void
lin_to_rgb (const gdouble  hr,
            const gdouble  hg,
            const gdouble  hb,
            guchar        *r,
            guchar        *g,
            guchar        *b)
{
  gfloat  rgb[3];
  gfloat  lab[3];
  gdouble ir, ig, ib;

  ir = ((gdouble) (hr)) * 255.0F / (gdouble) (HIST_R_ELEMS - 1);
  ig = ((gdouble)( hg)) * 255.0F / (gdouble) (HIST_G_ELEMS - 1);
  ib = ((gdouble)( hb)) * 255.0F / (gdouble) (HIST_B_ELEMS - 1);

  ir = ir / LRAT;
  ig = (ig / ARAT) + LOWA;
  ib = (ib / BRAT) + LOWB;

  lab[0] = ir;
  lab[1] = ig;
  lab[2] = ib;

  babl_process (lab_to_rgb_fish, lab, rgb, 1);

  *r = RINT (CLAMP (rgb[0] * 255, 0.0F, 255.0F));
  *g = RINT (CLAMP (rgb[1] * 255, 0.0F, 255.0F));
  *b = RINT (CLAMP (rgb[2] * 255, 0.0F, 255.0F));
}



struct _Color
{
  gint red;
  gint green;
  gint blue;
};

struct _QuantizeObj
{
  Pass1Func     first_pass;       /* first pass over image data creates colormap  */
  Pass2InitFunc second_pass_init; /* Initialize data which persists over invocations */
  Pass2Func     second_pass;      /* second pass maps from image data to colormap */
  CleanupFunc   delete_func;      /* function to clean up data associated with private */

  GimpPalette  *custom_palette;           /* The custom palette, if any        */

  gint          desired_number_of_colors; /* Number of colors we will allow    */
  gint          actual_number_of_colors;  /* Number of colors actually needed  */
  Color         cmap[256];                /* colormap created by quantization  */
  Color         clab[256];                /* .. converted to LAB space */
  Color         clin[256];                /* .. converted to linear space */
  guint64       index_used_count[256];    /* how many times an index was used  */
  CFHistogram   histogram;                /* holds the histogram               */

  gboolean      want_dither_alpha;
  gint          error_freedom;            /* 0=much bleed, 1=controlled bleed */

  GimpProgress *progress;

  const Babl   *space;
};

typedef struct
{
  /*  The bounds of the box (inclusive); expressed as histogram indexes  */
  gint    Rmin, Rmax;
  gint    Rhalferror;
  gint    Gmin, Gmax;
  gint    Ghalferror;
  gint    Bmin, Bmax;
  gint    Bhalferror;

  /*  The volume (actually 2-norm) of the box  */
  gint    volume;

  /*  The number of nonzero histogram cells within this box  */
  gint64  colorcount;

  /* The sum of the weighted error within this box */
  guint64 error;
  /* The sum of the unweighted error within this box */
  guint64 rerror;
  guint64 gerror;
  guint64 berror;

} box, *boxptr;


static void          zero_histogram_gray     (CFHistogram   histogram);
static void          zero_histogram_rgb      (CFHistogram   histogram);
static void          generate_histogram_gray (CFHistogram   hostogram,
                                              GimpLayer    *layer,
                                              gboolean      dither_alpha);
static void          generate_histogram_rgb  (CFHistogram   histogram,
                                              GimpLayer    *layer,
                                              gint          col_limit,
                                              gboolean      dither_alpha,
                                              GimpProgress *progress);

static QuantizeObj * initialize_median_cut   (GimpImageBaseType      old_type,
                                              gint                   max_colors,
                                              GimpConvertDitherType  dither_type,
                                              GimpConvertPaletteType palette_type,
                                              GimpPalette           *custom_palette,
                                              gboolean               dither_alpha,
                                              GimpProgress          *progress);

static void          compute_color_lin8      (QuantizeObj           *quantobj,
                                              CFHistogram            histogram,
                                              boxptr                 boxp,
                                              const int              icolor);


static guchar    found_cols[MAXNUMCOLORS][3];
static gint      num_found_cols;
static gboolean  needs_quantize;
static gboolean  had_white;
static gboolean  had_black;


/**********************************************************/
typedef struct
{
  gint64 used_count;
  guchar initial_index;
} PalEntry;

static int
mapping_compare (const void *a,
                 const void *b)
{
  PalEntry *m1 = (PalEntry *) a;
  PalEntry *m2 = (PalEntry *) b;

  return (m2->used_count - m1->used_count);
}

/* FWIW, the make_remap_table() and mapping_compare() function source
 * and PalEntry may be re-used under the XFree86-style license.
 * <adam@gimp.org>
 */
static void
make_remap_table (const guchar  old_palette[],
                  guchar        new_palette[],
                  const guint64 index_used_count[],
                  guchar        remap_table[],
                  gint         *num_entries)
{
  gint      i, j, k;
  guchar    temppal[256 * 3];
  guint64   tempuse[256];
  guint64   transmap[256];
  PalEntry *palentries;
  gint      used = 0;

  memset (temppal, 0, 256 * 3);
  memset (tempuse, 0, 256 * sizeof (guint64));
  memset (transmap, 255, 256 * sizeof (guint64));

  /* First pass - only collect entries which are marked as being used
   * at all in index_used_count.
   */
  for (i = 0; i < *num_entries; i++)
    {
      if (index_used_count[i])
        {
          temppal[used*3 + 0] = old_palette[i*3 + 0];
          temppal[used*3 + 1] = old_palette[i*3 + 1];
          temppal[used*3 + 2] = old_palette[i*3 + 2];

          tempuse[used] = index_used_count[i];
          transmap[i] = used;

          used++;
        }
    }

  /* Second pass - remove duplicates. (O(n^3), could do better!) */
  for (i = 0; i < used; i++)
    {
      for (j = 0; j < i; j++)
        {
          if ((temppal[i*3 + 1] == temppal[j*3 + 1]) &&
              (temppal[i*3 + 0] == temppal[j*3 + 0]) &&
              (temppal[i*3 + 2] == temppal[j*3 + 2]) &&
              tempuse[j] &&
              tempuse[i])
            {
              /* Move the 'used' tally from one to the other. */
              tempuse[i] += tempuse[j];
              /* zero one of them, deactivating its entry. */
              tempuse[j] = 0;

              /* change all mappings from this dead index to the live
               * one.
               */
              for (k = 0; k < *num_entries; k++)
                {
                  if (index_used_count[k] && (transmap[k] == j))
                    transmap[k] = i;
                }
            }
        }
    }

  /* Third pass - rank all used indices to the beginning of the
   * palette.
   */
  palentries = g_new (PalEntry, (guint) used);

  for (i = 0; i < used; i++)
    {
      palentries[i].initial_index = i;
      palentries[i].used_count    = tempuse[i];
    }

  qsort (palentries, used, sizeof (PalEntry), &mapping_compare);

  for (i = 0; i < *num_entries; i++)
    {
      if (index_used_count[i])
        {
          for (j = 0; j < used; j++)
            {
              if ((transmap[i] == palentries[j].initial_index)
                  && (palentries[j].used_count))
                {
                  remap_table[i] = j;
                  break;
                }
            }
        }
    }
  for (i = 0; i < *num_entries; i++)
    {
      if (index_used_count[i])
        {
          new_palette[remap_table[i] * 3 + 0] = old_palette[i * 3 + 0];
          new_palette[remap_table[i] * 3 + 1] = old_palette[i * 3 + 1];
          new_palette[remap_table[i] * 3 + 2] = old_palette[i * 3 + 2];
        }
    }

  *num_entries = 0;

  for (j = 0; j < used; j++)
    {
      if (palentries[j].used_count)
        {
          (*num_entries)++;
        }
    }

  g_free (palentries);
}

static void
remap_indexed_layer (GimpLayer    *layer,
                     const guchar *remap_table,
                     gint          num_entries)
{
  GeglBufferIterator *iter;
  const Babl         *format;
  gint                bpp;
  gboolean            has_alpha;

  format  = gimp_drawable_get_format (GIMP_DRAWABLE (layer));

  bpp       = babl_format_get_bytes_per_pixel (format);
  has_alpha = babl_format_has_alpha (format);

  iter = gegl_buffer_iterator_new (gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                                   NULL, 0, NULL,
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      guchar *data   = iter->items[0].data;
      gint    length = iter->length;

      if (has_alpha)
        {
          while (length--)
            {
              if (data[ALPHA_I])
                data[INDEXED] = remap_table[data[INDEXED]];
              else
                data[INDEXED] = 0;

              data += bpp;
            }
        }
      else
        {
          while (length--)
            {
              data[INDEXED] = remap_table[data[INDEXED]];

              data += bpp;
            }
        }
    }
}

static gint
color_quicksort (const void *c1,
                 const void *c2)
{
  Color *color1 = (Color *) c1;
  Color *color2 = (Color *) c2;

  gdouble v1 = GIMP_RGB_LUMINANCE (color1->red, color1->green, color1->blue);
  gdouble v2 = GIMP_RGB_LUMINANCE (color2->red, color2->green, color2->blue);

  if (v1 < v2)
    return -1;
  else if (v1 > v2)
    return 1;
  else
    return 0;
}

gboolean
gimp_image_convert_indexed (GimpImage               *image,
                            GimpConvertPaletteType   palette_type,
                            gint                     max_colors,
                            gboolean                 remove_duplicates,
                            GimpConvertDitherType    dither_type,
                            gboolean                 dither_alpha,
                            gboolean                 dither_text_layers,
                            GimpPalette             *custom_palette,
                            GimpProgress            *progress,
                            GError                 **error)
{
  QuantizeObj       *quantobj     = NULL;
  GimpObjectQueue   *queue        = NULL;
  GimpProgress      *sub_progress = NULL;
  GimpImageBaseType  old_type;
  GList             *all_layers;
  GList             *list;
  GimpColorProfile  *src_profile  = NULL;
  GimpColorProfile  *dest_profile = NULL;
  const Babl        *space;
  const Babl        *format;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (gimp_image_get_base_type (image) != GIMP_INDEXED, FALSE);
  g_return_val_if_fail (gimp_babl_is_valid (GIMP_INDEXED,
                                            gimp_image_get_precision (image)),
                        FALSE);
  g_return_val_if_fail (custom_palette == NULL ||
                        GIMP_IS_PALETTE (custom_palette), FALSE);
  g_return_val_if_fail (custom_palette == NULL ||
                        gimp_palette_get_n_colors (custom_palette) <= 256,
                        FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (palette_type == GIMP_CONVERT_PALETTE_CUSTOM)
    {
      if (! custom_palette)
        palette_type = GIMP_CONVERT_PALETTE_MONO;

      if (gimp_palette_get_n_colors (custom_palette) == 0)
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("Cannot convert image: palette is empty."));
          return FALSE;
        }
    }

  gimp_set_busy (image->gimp);

  all_layers = gimp_image_get_layer_list (image);

  g_object_freeze_notify (G_OBJECT (image));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_CONVERT,
                               C_("undo-type", "Convert Image to Indexed"));

  src_profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));

  /*  Push the image type to the stack  */
  gimp_image_undo_push_image_type (image, NULL);

  /*  Set the new base type  */
  old_type = gimp_image_get_base_type (image);

  space  = gimp_image_get_layer_space (image);
  format = babl_format_with_space ("R'G'B' float", space);

  /*  Build histogram if necessary.  */
  rgb_to_lab_fish = babl_fish (format, babl_format ("CIE Lab float"));
  rgb_to_linear_fish = babl_fish (format, babl_format_with_space ("RGB u16", space));
  gray_to_linear_fish = babl_fish (babl_format_with_space("Y' float", space), babl_format_with_space ("Y u16", space));
  linear_to_gray_float_fish = babl_fish (babl_format_with_space ("Y u16", space), babl_format_with_space ("Y' float", space));
  linear_to_rgb_float_fish = babl_fish (babl_format_with_space ("RGB u16", space), format);
  lab_to_rgb_fish = babl_fish (babl_format ("CIE Lab float"), format);

  /* don't dither if the input is grayscale and we are simply mapping
   * every color
   */
  if (old_type     == GIMP_GRAY &&
      max_colors   == 256       &&
      palette_type == GIMP_CONVERT_PALETTE_GENERATE)
    {
      dither_type = GIMP_CONVERT_DITHER_NONE;
    }

  if (progress)
    {
      queue        = gimp_object_queue_new (progress);
      sub_progress = GIMP_PROGRESS (queue);

      gimp_object_queue_push_list (queue, all_layers);
    }

  quantobj = initialize_median_cut (old_type, max_colors, dither_type,
                                    palette_type, custom_palette,
                                    dither_alpha,
                                    sub_progress);
  quantobj->space = space;

  if (palette_type == GIMP_CONVERT_PALETTE_GENERATE)
    {
      if (old_type == GIMP_GRAY)
        zero_histogram_gray (quantobj->histogram);
      else
        zero_histogram_rgb (quantobj->histogram);

      /* To begin, assume that there are fewer colors in the image
       *  than the user actually asked for.  In that case, we don't
       *  need to quantize or color-dither.
       */
      needs_quantize = FALSE;
      had_black = FALSE;
      had_white = FALSE;
      num_found_cols = 0;

      /*  Build the histogram  */
      for (list = all_layers;
           list;
           list = g_list_next (list))
        {
          GimpLayer *layer = list->data;

          if (queue)
            gimp_object_queue_pop (queue);

          if (old_type == GIMP_GRAY)
            {
              generate_histogram_gray (quantobj->histogram,
                                       layer, dither_alpha);
            }
          else
            {
              /* Note: generate_histogram_rgb may set needs_quantize
               * if the image contains more colors than the limit
               * specified by the user.
               */
              generate_histogram_rgb (quantobj->histogram,
                                      layer, max_colors, dither_alpha,
                                      sub_progress);
            }
        }
    }

  if (progress)
    gimp_progress_set_text_literal (progress,
                                    _("Converting to indexed colors (stage 2)"));

  if (old_type == GIMP_RGB &&
      ! needs_quantize     &&
      palette_type == GIMP_CONVERT_PALETTE_GENERATE)
    {
      gint i;

      /*  If this is an RGB image, and the user wanted a custom-built
       *  generated palette, and this image has no more colors than
       *  the user asked for, we don't need the first pass
       *  (quantization).
       *
       *  There's also no point in dithering, since there's no error
       *  to spread.  So we destroy the old quantobj and make a new
       *  one with the remapping function set to a special LUT-based
       *  no-dither remapper.
       */

      quantobj->delete_func (quantobj);
      quantobj = initialize_median_cut (old_type, max_colors,
                                        GIMP_CONVERT_DITHER_NODESTRUCT,
                                        palette_type,
                                        custom_palette,
                                        dither_alpha,
                                        sub_progress);
      /* We can skip the first pass (palette creation) */

      quantobj->actual_number_of_colors = num_found_cols;
      for (i = 0; i < num_found_cols; i++)
        {
          quantobj->cmap[i].red   = found_cols[i][0];
          quantobj->cmap[i].green = found_cols[i][1];
          quantobj->cmap[i].blue  = found_cols[i][2];
        }
    }
  else
    {
      quantobj->first_pass (quantobj);
    }

  if (palette_type == GIMP_CONVERT_PALETTE_GENERATE)
    qsort (quantobj->cmap,
           quantobj->actual_number_of_colors, sizeof (Color),
           color_quicksort);

  if (progress)
    {
      gimp_progress_set_text_literal (progress,
                                      _("Converting to indexed colors (stage 3)"));

      gimp_object_queue_clear (queue);
      gimp_object_queue_push_list (queue, all_layers);
    }

  /* Initialise data which must persist across indexed layer iterations */
  if (quantobj->second_pass_init)
    quantobj->second_pass_init (quantobj);

  /* Set the base type just before we also set the colormap. In
   * particular we must not let any GUI code in-between (like progress
   * update) in order to avoid any context switch. Some various pieces
   * of the code are relying on proper image state, and an indexed image
   * without a colormap is not a proper state (it will also have neither
   * babl format nor profile).
   * See #3070.
   */
  g_object_set (image, "base-type", GIMP_INDEXED, NULL);

  /*  Set the generated palette on the image, we need it to
   *  convert the layers. We optionally remove duplicate entries
   *  after the layer conversion.
   */
  {
    guchar colormap[GIMP_IMAGE_COLORMAP_SIZE];
    gint   i, j;

    for (i = 0, j = 0; i < quantobj->actual_number_of_colors; i++)
      {
        colormap[j++] = quantobj->cmap[i].red;
        colormap[j++] = quantobj->cmap[i].green;
        colormap[j++] = quantobj->cmap[i].blue;
      }

    _gimp_image_set_colormap (image, colormap,
                              quantobj->actual_number_of_colors, TRUE);
  }

  /* when converting from GRAY, always convert to the new type's
   * builtin profile
   */
  if (old_type == GIMP_GRAY)
    dest_profile = gimp_image_get_builtin_color_profile (image);

  /*  Convert all layers  */
  for (list = all_layers;
       list;
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;
      gboolean   quantize;

      if (queue)
        gimp_object_queue_pop (queue);

      if (gimp_item_is_text_layer (GIMP_ITEM (layer)))
        quantize = dither_text_layers;
      else
        quantize = TRUE;

      if (quantize)
        {
          GeglBuffer *new_buffer;
          gboolean    has_alpha;

          has_alpha = gimp_drawable_has_alpha (GIMP_DRAWABLE (layer));

          new_buffer =
            gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                             gimp_item_get_width  (GIMP_ITEM (layer)),
                                             gimp_item_get_height (GIMP_ITEM (layer))),
                             gimp_image_get_layer_format (image,
                                                          has_alpha));

          quantobj->second_pass (quantobj, layer, new_buffer);

          gimp_drawable_set_buffer (GIMP_DRAWABLE (layer), TRUE, NULL,
                                    new_buffer);
          g_object_unref (new_buffer);
        }
      else
        {
          gimp_drawable_convert_type (GIMP_DRAWABLE (layer), image,
                                      GIMP_INDEXED,
                                      gimp_drawable_get_precision (GIMP_DRAWABLE (layer)),
                                      gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)),
                                      src_profile,
                                      dest_profile,
                                      GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                      TRUE, sub_progress);
        }
    }

  /*  Set the final palette on the image  */
  if (remove_duplicates &&
      (palette_type != GIMP_CONVERT_PALETTE_GENERATE) &&
      (palette_type != GIMP_CONVERT_PALETTE_MONO))
    {
      guchar colormap[GIMP_IMAGE_COLORMAP_SIZE];
      gint   i, j;
      guchar old_palette[256 * 3];
      guchar new_palette[256 * 3];
      guchar remap_table[256];
      gint   num_entries;

      for (i = 0, j = 0; i < quantobj->actual_number_of_colors; i++)
        {
          old_palette[j++] = quantobj->cmap[i].red;
          old_palette[j++] = quantobj->cmap[i].green;
          old_palette[j++] = quantobj->cmap[i].blue;
        }

      num_entries = quantobj->actual_number_of_colors;

      /* Generate a remapping table */
      make_remap_table (old_palette, new_palette,
                        quantobj->index_used_count,
                        remap_table, &num_entries);

      /*  Convert all layers  */
      for (list = all_layers; list; list = g_list_next (list))
        {
          remap_indexed_layer (list->data, remap_table, num_entries);
        }

      for (i = 0, j = 0; i < num_entries; i++)
        {
          colormap[j] = new_palette[j]; j++;
          colormap[j] = new_palette[j]; j++;
          colormap[j] = new_palette[j]; j++;
        }

      _gimp_image_set_colormap (image, colormap, num_entries, TRUE);
    }

  /*  When converting from GRAY, set the new profile.
   */
  if (old_type == GIMP_GRAY)
    gimp_image_set_color_profile (image, dest_profile, NULL);

  /*  Delete the quantizer object, if there is one */
  if (quantobj)
    quantobj->delete_func (quantobj);

  gimp_image_undo_group_end (image);

  gimp_image_mode_changed (image);
  g_object_thaw_notify (G_OBJECT (image));

  g_clear_object (&queue);

  g_list_free (all_layers);

  gimp_unset_busy (image->gimp);

  return TRUE;
}

/*
 *  Indexed color conversion machinery
 */

static void
zero_histogram_gray (CFHistogram histogram)
{
  gint i;

  for (i = 0; i < 256; i++)
    histogram[i] = 0;
}


static void
zero_histogram_rgb (CFHistogram histogram)
{
  memset (histogram, 0,
          HIST_R_ELEMS * HIST_G_ELEMS * HIST_B_ELEMS * sizeof (ColorFreq));
}


static void
generate_histogram_gray (CFHistogram  histogram,
                         GimpLayer   *layer,
                         gboolean     dither_alpha)
{
  GeglBufferIterator *iter;
  const Babl         *format;
  gint                bpp;
  gboolean            has_alpha;

  format = gimp_drawable_get_format (GIMP_DRAWABLE (layer));

  g_return_if_fail (format == babl_format_with_space ("Y' u8", format) ||
                    format == babl_format_with_space ("Y'A u8", format));

  bpp       = babl_format_get_bytes_per_pixel (format);
  has_alpha = babl_format_has_alpha (format);

  iter = gegl_buffer_iterator_new (gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                                   NULL, 0, format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *data   = iter->items[0].data;
      gint          length = iter->length;

      if (has_alpha)
        {
          while (length--)
            {
              if (data[ALPHA_G] > 127)
                histogram[*data]++;

              data += bpp;
            }
        }
      else
        {
          while (length--)
            {
              histogram[*data]++;

              data += bpp;
            }
        }
    }
}

static void
check_white_or_black (const guchar *data)
{
  if (data[RED]   == 255 &&
      data[GREEN] == 255 &&
      data[BLUE]  == 255)
    had_white = TRUE;
  if (data[RED]  ==0 &&
      data[GREEN]==0 &&
      data[BLUE] ==0)
    had_black = TRUE;
}

static void
generate_histogram_rgb (CFHistogram   histogram,
                        GimpLayer    *layer,
                        gint          col_limit,
                        gboolean      dither_alpha,
                        GimpProgress *progress)
{
  GeglBufferIterator *iter;
  const Babl         *format;
  GeglRectangle      *roi;
  ColorFreq          *colfreq;
  gint                nfc_iter;
  gint                row, col, coledge;
  gint                offsetx, offsety;
  gint64              layer_size;
  gint64              total_size = 0;
  gint                count      = 0;
  gint                bpp;
  gboolean            has_alpha;

  format = gimp_drawable_get_format (GIMP_DRAWABLE (layer));

  g_return_if_fail (format == babl_format_with_space ("R'G'B' u8", format) ||
                    format == babl_format_with_space ("R'G'B'A u8", format));

  bpp       = babl_format_get_bytes_per_pixel (format);
  has_alpha = babl_format_has_alpha (format);

  gimp_item_get_offset (GIMP_ITEM (layer), &offsetx, &offsety);

  layer_size = (gimp_item_get_width  (GIMP_ITEM (layer)) *
                gimp_item_get_height (GIMP_ITEM (layer)));

  /*  g_printerr ("col_limit = %d, nfc = %d\n", col_limit, num_found_cols); */

  iter = gegl_buffer_iterator_new (gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                                   NULL, 0, format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);
  roi = &iter->items[0].roi;

  if (progress)
    gimp_progress_set_value (progress, 0.0);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *data   = iter->items[0].data;
      gint          length = iter->length;

      total_size += length;

      /* g_printerr (" [%d,%d - %d,%d]", srcPR.x, src_roi->y, offsetx, offsety); */

      if (needs_quantize)
        {
          if (dither_alpha)
            {
              /* if alpha-dithering,
                 we need to be deterministic w.r.t. offsets */

              col = roi->x + offsetx;
              coledge = col + roi->width;
              row = roi->y + offsety;

              while (length--)
                {
                  gboolean transparent = FALSE;

                  if (has_alpha &&
                      data[ALPHA] <
                      DM[col & DM_WIDTHMASK][row & DM_HEIGHTMASK])
                    transparent = TRUE;

                  if (! transparent)
                    {
                      colfreq = HIST_RGB (histogram,
                                          data[RED],
                                          data[GREEN],
                                          data[BLUE]);
                      check_white_or_black (data);
                      (*colfreq)++;
                    }

                  col++;
                  if (col == coledge)
                    {
                      col = roi->x + offsetx;
                      row++;
                    }

                  data += bpp;
                }
            }
          else
            {
              while (length--)
                {
                  if ((has_alpha && ((data[ALPHA] > 127)))
                      || (!has_alpha))
                    {
                      colfreq = HIST_RGB (histogram,
                                          data[RED],
                                          data[GREEN],
                                          data[BLUE]);
                      check_white_or_black (data);
                      (*colfreq)++;
                    }

                  data += bpp;
                }
            }
        }
      else
        {
          /* if alpha-dithering, we need to be deterministic w.r.t. offsets */
          col = roi->x + offsetx;
          coledge = col + roi->width;
          row = roi->y + offsety;

          while (length--)
            {
              gboolean transparent = FALSE;

              if (has_alpha)
                {
                  if (dither_alpha)
                    {
                      if (data[ALPHA] <
                          DM[col & DM_WIDTHMASK][row & DM_HEIGHTMASK])
                        transparent = TRUE;
                    }
                  else
                    {
                      if (data[ALPHA] <= 127)
                        transparent = TRUE;
                    }
                }

              if (! transparent)
                {
                  colfreq = HIST_RGB (histogram,
                                      data[RED],
                                      data[GREEN],
                                      data[BLUE]);
                  (*colfreq)++;

                  if (!needs_quantize)
                    {
                      for (nfc_iter = 0;
                           nfc_iter < num_found_cols;
                           nfc_iter++)
                        {
                          if ((data[RED]   == found_cols[nfc_iter][0]) &&
                              (data[GREEN] == found_cols[nfc_iter][1]) &&
                              (data[BLUE]  == found_cols[nfc_iter][2]))
                            goto already_found;
                        }

                      /* Color was not in the table of
                       * existing colors
                       */

                      num_found_cols++;

                      if (num_found_cols > col_limit)
                        {
                          /* There are more colors in the image than
                           *  were allowed.  We switch to plain
                           *  histogram calculation with a view to
                           *  quantizing at a later stage.
                           */
                          needs_quantize = TRUE;
                          /* g_print ("\nmax colors exceeded - needs quantize.\n");*/
                          goto already_found;
                        }
                      else
                        {
                          /* Remember the new color we just found.
                           */
                          found_cols[num_found_cols-1][0] = data[RED];
                          found_cols[num_found_cols-1][1] = data[GREEN];
                          found_cols[num_found_cols-1][2] = data[BLUE];

                          check_white_or_black (data);
                        }
                    }
                }
            already_found:

              col++;
              if (col == coledge)
                {
                  col = roi->x + offsetx;
                  row++;
                }

              data += bpp;
            }
        }

      if (progress && (count % 16 == 0))
        gimp_progress_set_value (progress,
                                 (gdouble) total_size / (gdouble) layer_size);
    }

/*  g_print ("O: col_limit = %d, nfc = %d\n", col_limit, num_found_cols);*/
}



static boxptr
find_split_candidate (const boxptr  boxlist,
                      const gint    numboxes,
                      AxisType     *which_axis,
                      const gint    desired_colors)
{
  boxptr  boxp;
  gint    i;
  etype   maxc = 0;
  boxptr  which = NULL;
  gdouble Lbias;

  *which_axis = AXIS_UNDEF;

  /* we only perform the initial L-split bias /at all/ if the final
     number of desired colors is quite low, otherwise it all comes
     out in the wash anyway and this initial bias generally only hurts
     us in the long run. */
  if (desired_colors <= 16)
    {
#define BIAS_FACTOR 2.66F
#define BIAS_NUMBER 2 /* 0 */

      /* we bias towards splitting across L* for first few colors */
      Lbias = (numboxes > BIAS_NUMBER) ? 1.0F : ((gdouble) (BIAS_NUMBER + 1) -
                                                 ((gdouble) numboxes)) /
        ((gdouble) BIAS_NUMBER / BIAS_FACTOR);
      /*Lbias = 1.0;
        fprintf(stderr, " [[%d]] ", numboxes);
        fprintf(stderr, "Using ramped L-split bias.\n");
        fprintf(stderr, "R\n");
      */
    }
  else
    Lbias = 1.0F;

  for (i = 0, boxp = boxlist; i < numboxes; i++, boxp++)
    {
      if (boxp->volume > 0)
        {
#ifndef _MSC_VER
          etype rpe = (double)((boxp->rerror) * R_SCALE * R_SCALE);
          etype gpe = (double)((boxp->gerror) * G_SCALE * G_SCALE);
          etype bpe = (double)((boxp->berror) * B_SCALE * B_SCALE);
#else
          /*
           * Sorry about the mess, otherwise would get :
           * error C2520: conversion from unsigned __int64 to double
           *              not implemented, use signed __int64
           */
          etype rpe = (double)(((__int64)boxp->rerror) * R_SCALE * R_SCALE);
          etype gpe = (double)(((__int64)boxp->gerror) * G_SCALE * G_SCALE);
          etype bpe = (double)(((__int64)boxp->berror) * B_SCALE * B_SCALE);
#endif

          if (Lbias * rpe > maxc &&
              boxp->Rmin < boxp->Rmax)
            {
              which = boxp;
              maxc  = Lbias * rpe;
              *which_axis = AXIS_RED;
            }

          if (gpe > maxc &&
              boxp->Gmin < boxp->Gmax)
            {
              which = boxp;
              maxc  = gpe;
              *which_axis = AXIS_GREEN;
            }

          if (bpe > maxc &&
              boxp->Bmin < boxp->Bmax)
            {
              which = boxp;
              maxc  = bpe;
              *which_axis = AXIS_BLUE;
            }
        }
    }

  /*  fprintf(stderr, " %f,%p ", maxc, which); */
  /*  fprintf(stderr, " %llu ", maxc); */

  return which;
}


/* Find the splittable box with the largest (scaled) volume Returns
 * NULL if no splittable boxes remain
 */
static boxptr
find_biggest_volume (const boxptr boxlist,
                     const gint   numboxes)
{
  boxptr boxp;
  gint   i;
  gint   maxv = 0;
  boxptr which = NULL;

  for (i = 0, boxp = boxlist; i < numboxes; i++, boxp++)
    {
      if (boxp->volume > maxv)
        {
          which = boxp;
          maxv = boxp->volume;
        }
    }

  return which;
}


/* Shrink the min/max bounds of a box to enclose only nonzero
 * elements, and recompute its volume and population
 */
static void
update_box_gray (const CFHistogram histogram,
                 boxptr            boxp)
{
  gint      i, min, max, dist;
  ColorFreq ccount;

  min = boxp->Rmin;
  max = boxp->Rmax;

  if (max > min)
    for (i = min; i <= max; i++)
      {
        if (histogram[i] != 0)
          {
            boxp->Rmin = min = i;
            break;
          }
      }

  if (max > min)
    for (i = max; i >= min; i--)
      {
        if (histogram[i] != 0)
          {
            boxp->Rmax = max = i;
            break;
          }
      }

  /* Update box volume.
   * We use 2-norm rather than real volume here; this biases the method
   * against making long narrow boxes, and it has the side benefit that
   * a box is splittable iff norm > 0.
   * Since the differences are expressed in histogram-cell units,
   * we have to shift back to JSAMPLE units to get consistent distances;
   * after which, we scale according to the selected distance scale factors.
   */
  dist = max - min;
  boxp->volume = dist * dist;

  /* Now scan remaining volume of box and compute population */
  ccount = 0;
  for (i = min; i <= max; i++)
    if (histogram[i] != 0)
      ccount++;

  boxp->colorcount = ccount;
}


/* Shrink the min/max bounds of a box to enclose only nonzero
 * elements, and recompute its volume, population and error
 */
static void
update_box_rgb (const CFHistogram histogram,
                boxptr            boxp,
                const gint        cells_remaining)
{
  gint      R, G, B;
  gint      Rmin, Rmax, Gmin, Gmax, Bmin, Bmax;
  gint      dist0, dist1, dist2;
  ColorFreq ccount;
  /*
  guint64 tempRerror;
  guint64 tempGerror;
  guint64 tempBerror;
  */
  QuantizeObj dummyqo;
  box         dummybox;

  /* fprintf(stderr, "U"); */

  Rmin = boxp->Rmin;  Rmax = boxp->Rmax;
  Gmin = boxp->Gmin;  Gmax = boxp->Gmax;
  Bmin = boxp->Bmin;  Bmax = boxp->Bmax;

  if (Rmax > Rmin)
    for (R = Rmin; R <= Rmax; R++)
      for (G = Gmin; G <= Gmax; G++)
        {
          for (B = Bmin; B <= Bmax; B++)
            {
              if (*HIST_LIN (histogram, R, G, B) != 0)
                {
                  boxp->Rmin = Rmin = R;
                  goto have_Rmin;
                }
            }
        }
 have_Rmin:
  if (Rmax > Rmin)
    for (R = Rmax; R >= Rmin; R--)
      for (G = Gmin; G <= Gmax; G++)
        {
          for (B = Bmin; B <= Bmax; B++)
            {
              if (*HIST_LIN (histogram, R, G, B) != 0)
                {
                  boxp->Rmax = Rmax = R;
                  goto have_Rmax;
                }
            }
        }
 have_Rmax:
  if (Gmax > Gmin)
    for (G = Gmin; G <= Gmax; G++)
      for (R = Rmin; R <= Rmax; R++)
        {
          for (B = Bmin; B <= Bmax; B++)
            {
              if (*HIST_LIN (histogram, R, G, B) != 0)
                {
                  boxp->Gmin = Gmin = G;
                  goto have_Gmin;
                }
            }
        }
 have_Gmin:
  if (Gmax > Gmin)
    for (G = Gmax; G >= Gmin; G--)
      for (R = Rmin; R <= Rmax; R++)
        {
          for (B = Bmin; B <= Bmax; B++)
            {
              if (*HIST_LIN (histogram, R, G, B) != 0)
                {
                  boxp->Gmax = Gmax = G;
                  goto have_Gmax;
                }
            }
        }
 have_Gmax:
  if (Bmax > Bmin)
    for (B = Bmin; B <= Bmax; B++)
      for (R = Rmin; R <= Rmax; R++)
        {
          for (G = Gmin; G <= Gmax; G++)
            {
              if (*HIST_LIN (histogram, R, G, B) != 0)
                {
                  boxp->Bmin = Bmin = B;
                  goto have_Bmin;
                }
            }
        }
 have_Bmin:
  if (Bmax > Bmin)
    for (B = Bmax; B >= Bmin; B--)
      for (R = Rmin; R <= Rmax; R++)
        {
          for (G = Gmin; G <= Gmax; G++)
            {
              if (*HIST_LIN (histogram, R, G, B) != 0)
                {
                  boxp->Bmax = Bmax = B;
                  goto have_Bmax;
                }
            }
        }
 have_Bmax:

  /* Update box volume.
   * We use 2-norm rather than real volume here; this biases the method
   * against making long narrow boxes, and it has the side benefit that
   * a box is splittable iff norm > 0. (ADM: note: this isn't true.)
   * Since the differences are expressed in histogram-cell units,
   * we have to shift back to JSAMPLE units to get consistent distances;
   * after which, we scale according to the selected distance scale factors.
   */
  dist0 = ((1 + Rmax - Rmin) << R_SHIFT) * R_SCALE;
  dist1 = ((1 + Gmax - Gmin) << G_SHIFT) * G_SCALE;
  dist2 = ((1 + Bmax - Bmin) << B_SHIFT) * B_SCALE;
  boxp->volume = dist0*dist0 + dist1*dist1 + dist2*dist2;
  /*  boxp->volume = dist0 * dist1 * dist2; */

  compute_color_lin8(&dummyqo, histogram, boxp, 0);

  /*printf("(%d %d %d)\n", dummyqo.cmap[0].red,dummyqo.cmap[0].green,dummyqo.cmap[0].blue);
    fflush(stdout);*/

  /* Now scan remaining volume of box and compute population */
  ccount = 0;
  boxp->error = 0;
  boxp->rerror = 0;
  boxp->gerror = 0;
  boxp->berror = 0;
  for (R = Rmin; R <= Rmax; R++)
    {
      for (G = Gmin; G <= Gmax; G++)
        {
          for (B = Bmin; B <= Bmax; B++)
            {
              ColorFreq freq_here;

              freq_here = *HIST_LIN (histogram, R, G, B);

              if (freq_here != 0)
                {
                  int ge, be, re;

                  dummybox.Rmin = dummybox.Rmax = R;
                  dummybox.Gmin = dummybox.Gmax = G;
                  dummybox.Bmin = dummybox.Bmax = B;
                  compute_color_lin8(&dummyqo, histogram, &dummybox, 1);

                  re = dummyqo.cmap[0].red   - dummyqo.cmap[1].red;
                  ge = dummyqo.cmap[0].green - dummyqo.cmap[1].green;
                  be = dummyqo.cmap[0].blue  - dummyqo.cmap[1].blue;

                  boxp->rerror += freq_here * (re) * (re);
                  boxp->gerror += freq_here * (ge) * (ge);
                  boxp->berror += freq_here * (be) * (be);

                  ccount += freq_here;
                }
            }
        }
    }

#if 0
  fg d;flg fd;kg fld;gflkfld
  /* Scan again, taking note of halfway error point for red axis */
  tempRerror = 0;
  boxp->Rhalferror = Rmin;
#warning r<=?
  for (R = Rmin; R <= Rmax; R++)
    {
      for (G = Gmin; G <= Gmax; G++)
        {
          for (B = Bmin; B <= Bmax; B++)
            {
              ColorFreq freq_here;
              freq_here = *HIST_LIN(histogram, R, G, B);
              if (freq_here != 0)
                {
                  int re;
                  int idist;
                  double dist;

                  dummybox.Rmin = dummybox.Rmax = R;
                  dummybox.Gmin = dummybox.Gmax = G;
                  dummybox.Bmin = dummybox.Bmax = B;
                  compute_color_lin8(&dummyqo, histogram, &dummybox, 1);

                  re = dummyqo.cmap[0].red   - dummyqo.cmap[1].red;

                  tempRerror += freq_here * (re) * (re);

                  if (tempRerror*2 >= boxp->rerror)
                    goto green_axisscan;
                  else
                    boxp->Rhalferror = R;
                }
            }
        }
    }
  fprintf(stderr, " D:");
 green_axisscan:

  fprintf(stderr, "<%d: %llu/%llu> ", R, tempRerror, boxp->rerror);
  /* Scan again, taking note of halfway error point for green axis */
  tempGerror = 0;
  boxp->Ghalferror = Gmin;
#warning G<=?
  for (G = Gmin; G <= Gmax; G++)
    {
      for (R = Rmin; R <= Rmax; R++)
        {
          for (B = Bmin; B <= Bmax; B++)
            {
              ColorFreq freq_here;
              freq_here = *HIST_LIN(histogram, R, G, B);
              if (freq_here != 0)
                {
                  int ge;
                  dummybox.Rmin = dummybox.Rmax = R;
                  dummybox.Gmin = dummybox.Gmax = G;
                  dummybox.Bmin = dummybox.Bmax = B;
                  compute_color_lin8(&dummyqo, histogram, &dummybox, 1);

                  ge = dummyqo.cmap[0].green - dummyqo.cmap[1].green;

                  tempGerror += freq_here * (ge) * (ge);

                  if (tempGerror*2 >= boxp->gerror)
                    goto blue_axisscan;
                  else
                    boxp->Ghalferror = G;
                }
            }
        }
    }

 blue_axisscan:
  /* Scan again, taking note of halfway error point for blue axis */
  tempBerror = 0;
  boxp->Bhalferror = Bmin;
#warning B<=?
  for (B = Bmin; B <= Bmax; B++)
    {
      for (R = Rmin; R <= Rmax; R++)
        {
          for (G = Gmin; G <= Gmax; G++)
            {
              ColorFreq freq_here;
              freq_here = *HIST_LIN(histogram, R, G, B);
              if (freq_here != 0)
                {
                  int be;
                  dummybox.Rmin = dummybox.Rmax = R;
                  dummybox.Gmin = dummybox.Gmax = G;
                  dummybox.Bmin = dummybox.Bmax = B;
                  compute_color_lin8(&dummyqo, histogram, &dummybox, 1);

                  be = dummyqo.cmap[0].blue  - dummyqo.cmap[1].blue;

                  tempBerror += freq_here * (be) * (be);

                  if (tempBerror*2 >= boxp->berror)
                    goto finished_axesscan;
                  else
                    boxp->Bhalferror = B;
                }
            }
        }
    }
 finished_axesscan:
#else

  boxp->Rhalferror = Rmin + (Rmax - Rmin + 1) / 2;
  boxp->Ghalferror = Gmin + (Gmax - Gmin + 1) / 2;
  boxp->Bhalferror = Bmin + (Bmax - Bmin + 1) / 2;

  if (dist0 && dist1 && dist2)
    {
      AxisType longest_ax      = AXIS_UNDEF;
      gint     longest_length  = 0;
      gint     longest_length2 = 0;
      gint     ratio;

      /*
        fprintf(stderr, "[%d,%d,%d=%d,%d,%d] ",
        (Rmax - Rmin), (Gmax - Gmin), (Bmax - Bmin),
        dist0, dist1, dist2);
      */

      if (dist0 >= longest_length)
        {
          longest_length2 = longest_length;
          longest_length = dist0;
          longest_ax = AXIS_RED;
        }
      else if (dist0 >= longest_length2)
        {
          longest_length2 = dist0;
        }

      if (dist1 >= longest_length)
        {
          longest_length2 = longest_length;
          longest_length = dist1;
          longest_ax = AXIS_GREEN;
        }
      else if (dist1 >= longest_length2)
        {
          longest_length2 = dist1;
        }

      if (dist2 >= longest_length)
        {
          longest_length2 = longest_length;
          longest_length = dist2;
          longest_ax = AXIS_BLUE;
        }
      else if (dist2 >= longest_length2)
        {
          longest_length2 = dist2;
        }

      if (longest_length2 == 0)
        longest_length2 = 1;

      ratio = (longest_length + longest_length2/2) / longest_length2;
      /* fprintf(stderr, " ratio:(%d/%d)=%d ", longest_length, longest_length2, ratio);
         fprintf(stderr, "C%d ", cells_remaining); */

      if (ratio > cells_remaining + 1)
        ratio = cells_remaining + 1;

      if (ratio > 2)
        {
          switch (longest_ax)
            {
            case AXIS_RED:
              if (Rmin + (Rmax - Rmin + ratio / 2) / ratio < Rmax)
                {
                  /* fprintf(stderr, "FR%d \007\n",ratio);*/
                  boxp->Rhalferror = Rmin +  (Rmax - Rmin + ratio / 2) / ratio;
                }
              break;
            case AXIS_GREEN:
              if (Gmin + (Gmax - Gmin + ratio / 2) / ratio < Gmax)
                {
                  /* fprintf(stderr, "FG%d \007\n",ratio);*/
                  boxp->Ghalferror = Gmin + (Gmax - Gmin + ratio / 2) / ratio;
                }
              break;
            case AXIS_BLUE:
              if (Bmin + (Bmax - Bmin + ratio / 2) / ratio < Bmax)
                {
                  /* fprintf(stderr, "FB%d \007\n",ratio);*/
                  boxp->Bhalferror = Bmin + (Bmax - Bmin + ratio / 2) / ratio;
                }
              break;
            default:
              g_warning ("GRR, UNDEF LONGEST AXIS\007\n");
            }
        }
    }

  if (boxp->Rhalferror == Rmax)
    boxp->Rhalferror = Rmin;
  if (boxp->Ghalferror == Gmax)
    boxp->Ghalferror = Gmin;
  if (boxp->Bhalferror == Bmax)
    boxp->Bhalferror = Bmin;

  /*
  boxp->Rhalferror = RSDF(dummyqo.cmap[0].red);
  boxp->Ghalferror = GSDF(dummyqo.cmap[0].green);
  boxp->Bhalferror = BSDF(dummyqo.cmap[0].blue);
  */

  /*
  boxp->Rhalferror = (RSDF(dummyqo.cmap[0].red) + (Rmin+Rmax)/2)/2;
  boxp->Ghalferror = (GSDF(dummyqo.cmap[0].green) + (Gmin+Gmax)/2)/2;
  boxp->Bhalferror = (BSDF(dummyqo.cmap[0].blue) + (Bmin+Bmax)/2)/2;
  */


#endif
  /*
  fprintf(stderr, " %d,%d", dummyqo.cmap[0].blue, boxp->Bmax);

  gimp_assert (boxp->Rhalferror >= boxp->Rmin);
  gimp_assert (boxp->Rhalferror < boxp->Rmax);
  gimp_assert (boxp->Ghalferror >= boxp->Gmin);
  gimp_assert (boxp->Ghalferror < boxp->Gmax);
  gimp_assert (boxp->Bhalferror >= boxp->Bmin);
  gimp_assert (boxp->Bhalferror < boxp->Bmax);*/

  /*boxp->error = (sqrt((double)(boxp->error/ccount)));*/
  /*  boxp->rerror = (sqrt((double)((boxp->rerror)/ccount)));
  boxp->gerror = (sqrt((double)((boxp->gerror)/ccount)));
  boxp->berror = (sqrt((double)((boxp->berror)/ccount)));*/
  /*printf(":%lld / %ld: ", boxp->error, ccount);
  printf("(%d-%d-%d)(%d-%d-%d)(%d-%d-%d)\n",
         Rmin, boxp->Rhalferror, Rmax,
         Gmin, boxp->Ghalferror, Gmax,
         Bmin, boxp->Bhalferror, Bmax
         );
         fflush(stdout);*/

  boxp->colorcount = ccount;
}


/* Repeatedly select and split the largest box until we have enough
 * boxes
 */
static gint
median_cut_gray (CFHistogram histogram,
                 boxptr      boxlist,
                 gint        numboxes,
                 gint        desired_colors)
{
  gint   lb;
  boxptr b1, b2;

  while (numboxes < desired_colors)
    {
      /* Select box to split.
       * Current algorithm: by population for first half, then by volume.
       */

      b1 = find_biggest_volume (boxlist, numboxes);

      if (b1 == NULL)           /* no splittable boxes left! */
        break;

      b2 = boxlist + numboxes;  /* where new box will go */
      /* Copy the color bounds to the new box. */
      b2->Rmax = b1->Rmax;
      b2->Rmin = b1->Rmin;

      /* Current algorithm: split at halfway point.
       * (Since the box has been shrunk to minimum volume,
       * any split will produce two nonempty subboxes.)
       * Note that lb value is max for lower box, so must be < old max.
       */
      lb = (b1->Rmax + b1->Rmin) / 2;
      b1->Rmax = lb;
      b2->Rmin = lb + 1;

      /* Update stats for boxes */
      update_box_gray (histogram, b1);
      update_box_gray (histogram, b2);
      numboxes++;
    }

  return numboxes;
}

/* Repeatedly select and split the largest box until we have enough
 * boxes
 */
static gint
median_cut_rgb (CFHistogram   histogram,
                boxptr        boxlist,
                gint          numboxes,
                gint          desired_colors,
                GimpProgress *progress)
{
  gint     lb;
  boxptr   b1, b2;
  AxisType which_axis;

  while (numboxes < desired_colors)
    {
      b1 = find_split_candidate (boxlist, numboxes, &which_axis, desired_colors);

      if (b1 == NULL)           /* no splittable boxes left! */
        break;

      b2 = boxlist + numboxes;  /* where new box will go */
      /* Copy the color bounds to the new box. */
      b2->Rmax = b1->Rmax; b2->Gmax = b1->Gmax; b2->Bmax = b1->Bmax;
      b2->Rmin = b1->Rmin; b2->Gmin = b1->Gmin; b2->Bmin = b1->Bmin;


      /* Choose split point along selected axis, and update box bounds.
       * Note that lb value is max for lower box, so must be < old max.
       */
      switch (which_axis)
        {
        case AXIS_RED:
          lb = b1->Rhalferror;/* *0 + (b1->Rmax + b1->Rmin) / 2; */
          b1->Rmax = lb;
          b2->Rmin = lb+1;
          g_return_val_if_fail (b1->Rmax >= b1->Rmin, numboxes);
          g_return_val_if_fail (b2->Rmax >= b2->Rmin, numboxes);
          break;
        case AXIS_GREEN:
          lb = b1->Ghalferror;/* *0 + (b1->Gmax + b1->Gmin) / 2; */
          b1->Gmax = lb;
          b2->Gmin = lb+1;
          g_return_val_if_fail (b1->Gmax >= b1->Gmin, numboxes);
          g_return_val_if_fail (b2->Gmax >= b2->Gmin, numboxes);
          break;
        case AXIS_BLUE:
          lb = b1->Bhalferror;/* *0 + (b1->Bmax + b1->Bmin) / 2; */
          b1->Bmax = lb;
          b2->Bmin = lb+1;
          g_return_val_if_fail (b1->Bmax >= b1->Bmin, numboxes);
          g_return_val_if_fail (b2->Bmax >= b2->Bmin, numboxes);
          break;
        default:
          g_error ("Uh-oh.");
        }
      /* Update stats for boxes */
      numboxes++;

      if (progress && (numboxes % 16 == 0))
        gimp_progress_set_value (progress, (gdouble) numboxes / desired_colors);

      update_box_rgb (histogram, b1, desired_colors - numboxes);
      update_box_rgb (histogram, b2, desired_colors - numboxes);
    }

  return numboxes;
}


/* Compute representative color for a box, put it in colormap[icolor]
 */
static void
compute_color_gray (QuantizeObj *quantobj,
                    CFHistogram  histogram,
                    boxptr       boxp,
                    int          icolor)
{
  gint    i, min, max;
  guint64 count;
  guint64 total;
  guint64 gtotal;

  min = boxp->Rmin;
  max = boxp->Rmax;

  total = 0;
  gtotal = 0;

  for (i = min; i <= max; i++)
    {
      count = histogram[i];
      if (count != 0)
        {
          total += count;
          gtotal += i * count;
        }
    }

  if (total != 0)
    {
      quantobj->cmap[icolor].red   =
      quantobj->cmap[icolor].green =
      quantobj->cmap[icolor].blue  = (gtotal + (total >> 1)) / total;
    }
  else
    {
      /* The only situation where total==0 is if the image was null or
       *  all-transparent.  In that case we just put a dummy value in
       *  the colormap.
       */
      quantobj->cmap[icolor].red   =
      quantobj->cmap[icolor].green =
      quantobj->cmap[icolor].blue  = 0;
    }
}


/* Compute representative color for a box, put it in colormap[icolor]
 */
static void
compute_color_rgb (QuantizeObj *quantobj,
                   CFHistogram  histogram,
                   boxptr       boxp,
                   int          icolor)
{
  /* Current algorithm: mean weighted by pixels (not colors) */
  /* Note it is important to get the rounding correct! */
  gint      R, G, B;
  gint      Rmin, Rmax;
  gint      Gmin, Gmax;
  gint      Bmin, Bmax;
  ColorFreq total  = 0;
  ColorFreq Rtotal = 0;
  ColorFreq Gtotal = 0;
  ColorFreq Btotal = 0;

  Rmin = boxp->Rmin;  Rmax = boxp->Rmax;
  Gmin = boxp->Gmin;  Gmax = boxp->Gmax;
  Bmin = boxp->Bmin;  Bmax = boxp->Bmax;

  for (R = Rmin; R <= Rmax; R++)
    for (G = Gmin; G <= Gmax; G++)
      {
        for (B = Bmin; B <= Bmax; B++)
          {
            ColorFreq this_freq = *HIST_LIN (histogram, R, G, B);

            if (this_freq != 0)
              {
                total += this_freq;
                Rtotal += R * this_freq;
                Gtotal += G * this_freq;
                Btotal += B * this_freq;
              }
          }
      }

  if (total > 0)
    {
      guchar red, green, blue;

      lin_to_rgb (/*(Rtotal + (total>>1)) / total,
                    (Gtotal + (total>>1)) / total,
                    (Btotal + (total>>1)) / total,*/
                  (double)Rtotal / (double)total,
                  (double)Gtotal / (double)total,
                  (double)Btotal / (double)total,
                  &red, &green, &blue);

      quantobj->cmap[icolor].red   = red;
      quantobj->cmap[icolor].green = green;
      quantobj->cmap[icolor].blue  = blue;
    }
  else
    {
      /* The only situation where total==0 is if the image was null or
       *  all-transparent.  In that case we just put a dummy value in
       *  the colormap.
       */
      quantobj->cmap[icolor].red   = 0;
      quantobj->cmap[icolor].green = 0;
      quantobj->cmap[icolor].blue  = 0;
    }
}


/* Compute representative color for a box, put it in colormap[icolor]
 */
static void
compute_color_lin8 (QuantizeObj *quantobj,
                    CFHistogram  histogram,
                    boxptr       boxp,
                    const gint   icolor)
{
  /* Current algorithm: mean weighted by pixels (not colors) */
  /* Note it is important to get the rounding correct! */
  gint      R, G, B;
  gint      Rmin, Rmax;
  gint      Gmin, Gmax;
  gint      Bmin, Bmax;
  ColorFreq total  = 0;
  ColorFreq Rtotal = 0;
  ColorFreq Gtotal = 0;
  ColorFreq Btotal = 0;

  Rmin = boxp->Rmin;  Rmax = boxp->Rmax;
  Gmin = boxp->Gmin;  Gmax = boxp->Gmax;
  Bmin = boxp->Bmin;  Bmax = boxp->Bmax;

  for (R = Rmin; R <= Rmax; R++)
    for (G = Gmin; G <= Gmax; G++)
      {
        for (B = Bmin; B <= Bmax; B++)
          {
            ColorFreq this_freq = *HIST_LIN (histogram, R, G, B);

            if (this_freq != 0)
              {
                Rtotal += R * this_freq;
                Gtotal += G * this_freq;
                Btotal += B * this_freq;
                total += this_freq;
              }
          }
      }

  if (total != 0)
    {
      quantobj->cmap[icolor].red   = ((Rtotal << R_SHIFT) + (total>>1)) / total;
      quantobj->cmap[icolor].green = ((Gtotal << G_SHIFT) + (total>>1)) / total;
      quantobj->cmap[icolor].blue  = ((Btotal << B_SHIFT) + (total>>1)) / total;
    }
  else
    {
      /* The only situation where total==0 is if the image was null or
       *  all-transparent.  In that case we just put a dummy value in
       *  the colormap.
       */
      g_warning ("eep.");
      quantobj->cmap[icolor].red   = 0;
      quantobj->cmap[icolor].green = 128;
      quantobj->cmap[icolor].blue  = 128;
    }
}


/* Master routine for color selection
 */
static void
select_colors_gray (QuantizeObj *quantobj,
                    CFHistogram  histogram)
{
  boxptr boxlist;
  gint   numboxes;
  gint   desired = quantobj->desired_number_of_colors;
  gint   i;

  /* Allocate workspace for box list */
  boxlist = g_new (box, desired);

  /* Initialize one box containing whole space */
  numboxes = 1;
  boxlist[0].Rmin = 0;
  boxlist[0].Rmax = 255;
  /* Shrink it to actually-used volume and set its statistics */
  update_box_gray (histogram, boxlist);
  /* Perform median-cut to produce final box list */
  numboxes = median_cut_gray (histogram, boxlist, numboxes, desired);

  quantobj->actual_number_of_colors = numboxes;
  /* Compute the representative color for each box, fill colormap */
  for (i = 0; i < numboxes; i++)
    compute_color_gray (quantobj, histogram, boxlist + i, i);
}


/* Master routine for color selection
 */
static void
select_colors_rgb (QuantizeObj *quantobj,
                   CFHistogram  histogram)
{
  boxptr boxlist;
  gint   numboxes;
  gint   desired = quantobj->desired_number_of_colors;
  gint  i;

  /* Allocate workspace for box list */
  boxlist = g_new (box, desired);

  /* Initialize one box containing whole space */
  numboxes = 1;
  boxlist[0].Rmin = 0;
  boxlist[0].Rmax = HIST_R_ELEMS - 1;
  boxlist[0].Gmin = 0;
  boxlist[0].Gmax = HIST_G_ELEMS - 1;
  boxlist[0].Bmin = 0;
  boxlist[0].Bmax = HIST_B_ELEMS - 1;
  /* Shrink it to actually-used volume and set its statistics */
  update_box_rgb (histogram, &boxlist[0], quantobj->desired_number_of_colors);
  /* Perform median-cut to produce final box list */
  numboxes = median_cut_rgb (histogram, boxlist, numboxes, desired,
                             quantobj->progress);

  quantobj->actual_number_of_colors = numboxes;
  /* Compute the representative color for each box, fill colormap */
  for (i = 0; i < numboxes; i++)
    {
      compute_color_rgb (quantobj, histogram, &boxlist[i], i);
    }

  g_free (boxlist);
}


/*
 * These routines are concerned with the time-critical task of mapping input
 * colors to the nearest color in the selected colormap.
 *
 * We re-use the histogram space as an "inverse color map", essentially a
 * cache for the results of nearest-color searches.  All colors within a
 * histogram cell will be mapped to the same colormap entry, namely the one
 * closest to the cell's center.  This may not be quite the closest entry to
 * the actual input color, but it's almost as good.  A zero in the cache
 * indicates we haven't found the nearest color for that cell yet; the array
 * is cleared to zeroes before starting the mapping pass.  When we find the
 * nearest color for a cell, its colormap index plus one is recorded in the
 * cache for future use.  The pass2 scanning routines call fill_inverse_cmap
 * when they need to use an unfilled entry in the cache.
 *
 * Our method of efficiently finding nearest colors is based on the "locally
 * sorted search" idea described by Heckbert and on the incremental distance
 * calculation described by Spencer W. Thomas in chapter III.1 of Graphics
 * Gems II (James Arvo, ed.  Academic Press, 1991).  Thomas points out that
 * the distances from a given colormap entry to each cell of the histogram can
 * be computed quickly using an incremental method: the differences between
 * distances to adjacent cells themselves differ by a constant.  This allows a
 * fairly fast implementation of the "brute force" approach of computing the
 * distance from every colormap entry to every histogram cell.  Unfortunately,
 * it needs a work array to hold the best-distance-so-far for each histogram
 * cell (because the inner loop has to be over cells, not colormap entries).
 * The work array elements have to be ints, so the work array would need
 * 256Kb at our recommended precision.  This is not feasible in DOS machines.
 *
 * To get around these problems, we apply Thomas' method to compute the
 * nearest colors for only the cells within a small subbox of the histogram.
 * The work array need be only as big as the subbox, so the memory usage
 * problem is solved.  Furthermore, we need not fill subboxes that are never
 * referenced in pass2; many images use only part of the color gamut, so a
 * fair amount of work is saved.  An additional advantage of this
 * approach is that we can apply Heckbert's locality criterion to quickly
 * eliminate colormap entries that are far away from the subbox; typically
 * three-fourths of the colormap entries are rejected by Heckbert's criterion,
 * and we need not compute their distances to individual cells in the subbox.
 * The speed of this approach is heavily influenced by the subbox size: too
 * small means too much overhead, too big loses because Heckbert's criterion
 * can't eliminate as many colormap entries.  Empirically the best subbox
 * size seems to be about 1/512th of the histogram (1/8th in each direction).
 *
 * Thomas' article also describes a refined method which is asymptotically
 * faster than the brute-force method, but it is also far more complex and
 * cannot efficiently be applied to small subboxes.  It is therefore not
 * useful for programs intended to be portable to DOS machines.  On machines
 * with plenty of memory, filling the whole histogram in one shot with Thomas'
 * refined method might be faster than the present code --- but then again,
 * it might not be any faster, and it's certainly more complicated.
 */


/* log2(histogram cells in update box) for each axis; this can be adjusted */
/*#define BOX_R_LOG  (PRECISION_R-3)
  #define BOX_G_LOG  (PRECISION_G-3)
  #define BOX_B_LOG  (PRECISION_B-3)*/

/*adam*/
#define BOX_R_LOG 0
#define BOX_G_LOG 0
#define BOX_B_LOG 0

#define BOX_R_ELEMS  (1<<BOX_R_LOG) /* # of hist cells in update box */
#define BOX_G_ELEMS  (1<<BOX_G_LOG)
#define BOX_B_ELEMS  (1<<BOX_B_LOG)

#define BOX_R_SHIFT  (R_SHIFT + BOX_R_LOG)
#define BOX_G_SHIFT  (G_SHIFT + BOX_G_LOG)
#define BOX_B_SHIFT  (B_SHIFT + BOX_B_LOG)


/*
 * The next three routines implement inverse colormap filling.  They
 * could all be folded into one big routine, but splitting them up
 * this way saves some stack space (the mindist[] and bestdist[]
 * arrays need not coexist) and may allow some compilers to produce
 * better code by registerizing more inner-loop variables.
 */

/* Locate the colormap entries close enough to an update box to be
 * candidates for the nearest entry to some cell(s) in the update box.
 * The update box is specified by the center coordinates of its first
 * cell.  The number of candidate colormap entries is returned, and
 * their colormap indexes are placed in colorlist[].
 *
 * This routine uses Heckbert's "locally sorted search" criterion to
 * select the colors that need further consideration.
 */
static gint
find_nearby_colors (QuantizeObj *quantobj,
                    int          minR,
                    int          minG,
                    int          minB,
                    int          colorlist[])
{
  int numcolors = quantobj->actual_number_of_colors;
  int maxR, maxG, maxB;
  int centerR, centerG, centerB;
  int i, x, ncolors;
  int minmaxdist, min_dist, max_dist, tdist;
  int mindist[MAXNUMCOLORS];    /* min distance to colormap entry i */

  /* Compute true coordinates of update box's upper corner and center.
   * Actually we compute the coordinates of the center of the upper-corner
   * histogram cell, which are the upper bounds of the volume we care about.
   * Note that since ">>" rounds down, the "center" values may be closer to
   * min than to max; hence comparisons to them must be "<=", not "<".
   */
  maxR = minR + ((1 << BOX_R_SHIFT) - (1 << R_SHIFT));
  centerR = (minR + maxR + 1) >> 1;
  maxG = minG + ((1 << BOX_G_SHIFT) - (1 << G_SHIFT));
  centerG = (minG + maxG + 1) >> 1;
  maxB = minB + ((1 << BOX_B_SHIFT) - (1 << B_SHIFT));
  centerB = (minB + maxB + 1) >> 1;

  /* For each color in colormap, find:
   *  1. its minimum squared-distance to any point in the update box
   *     (zero if color is within update box);
   *  2. its maximum squared-distance to any point in the update box.
   * Both of these can be found by considering only the corners of the box.
   * We save the minimum distance for each color in mindist[];
   * only the smallest maximum distance is of interest.
   */
  minmaxdist = 0x7FFFFFFFL;

  for (i = 0; i < numcolors; i++)
    {
      /* We compute the squared-R-distance term, then add in the other two. */
      x = quantobj->clab[i].red;
      if (x < minR)
        {
          tdist = (x - minR) * R_SCALE;
          min_dist = tdist*tdist;
          tdist = (x - maxR) * R_SCALE;
          max_dist = tdist*tdist;
        }
      else if (x > maxR)
        {
          tdist = (x - maxR) * R_SCALE;
          min_dist = tdist*tdist;
          tdist = (x - minR) * R_SCALE;
          max_dist = tdist*tdist;
        }
      else
        {
          /* within cell range so no contribution to min_dist */
          min_dist = 0;
          if (x <= centerR)
            {
              tdist = (x - maxR) * R_SCALE;
              max_dist = tdist*tdist;
            }
          else
            {
              tdist = (x - minR) * R_SCALE;
              max_dist = tdist*tdist;
            }
        }

      x = quantobj->clab[i].green;
      if (x < minG)
        {
          tdist = (x - minG) * G_SCALE;
          min_dist += tdist*tdist;
          tdist = (x - maxG) * G_SCALE;
          max_dist += tdist*tdist;
        }
      else if (x > maxG)
        {
          tdist = (x - maxG) * G_SCALE;
          min_dist += tdist*tdist;
          tdist = (x - minG) * G_SCALE;
          max_dist += tdist*tdist;
        }
      else
        {
          /* within cell range so no contribution to min_dist */
          if (x <= centerG)
            {
              tdist = (x - maxG) * G_SCALE;
              max_dist += tdist*tdist;
            }
          else
            {
              tdist = (x - minG) * G_SCALE;
              max_dist += tdist*tdist;
            }
        }

      x = quantobj->clab[i].blue;
      if (x < minB)
        {
          tdist = (x - minB) * B_SCALE;
          min_dist += tdist*tdist;
          tdist = (x - maxB) * B_SCALE;
          max_dist += tdist*tdist;
        }
      else if (x > maxB)
        {
          tdist = (x - maxB) * B_SCALE;
          min_dist += tdist*tdist;
          tdist = (x - minB) * B_SCALE;
          max_dist += tdist*tdist;
        }
      else
        {
          /* within cell range so no contribution to min_dist */
          if (x <= centerB)
            {
              tdist = (x - maxB) * B_SCALE;
              max_dist += tdist*tdist;
            }
          else
            {
              tdist = (x - minB) * B_SCALE;
              max_dist += tdist*tdist;
            }
        }

      mindist[i] = min_dist;      /* save away the results */
      if (max_dist < minmaxdist)
        minmaxdist = max_dist;
    }

  /* Now we know that no cell in the update box is more than minmaxdist
   * away from some colormap entry.  Therefore, only colors that are
   * within minmaxdist of some part of the box need be considered.
   */
  ncolors = 0;
  for (i = 0; i < numcolors; i++)
    {
      if (mindist[i] <= minmaxdist)
        colorlist[ncolors++] = i;
    }

  return ncolors;
}


/* Find the closest colormap entry for each cell in the update box,
 * given the list of candidate colors prepared by find_nearby_colors.
 * Return the indexes of the closest entries in the bestcolor[] array.
 * This routine uses Thomas' incremental distance calculation method
 * to find the distance from a colormap entry to successive cells in
 * the box.
 */
static void
find_best_colors (QuantizeObj *quantobj,
                  gint         minR,
                  gint         minG,
                  gint         minB,
                  gint         numcolors,
                  gint         colorlist[],
                  gint         bestcolor[])
{
  gint  iR, iG, iB;
  gint  i, icolor;
  gint *bptr;           /* pointer into bestdist[] array */
  gint *cptr;           /* pointer into bestcolor[] array */
  gint  dist0, dist1;     /* initial distance values */
  gint  dist2;            /* current distance in inner loop */
  gint  xx0, xx1;         /* distance increments */
  gint  xx2;
  gint  inR, inG, inB;    /* initial values for increments */

  /* This array holds the distance to the nearest-so-far color for each cell */
  gint  bestdist[BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS] = { 0, };

  /* Initialize best-distance for each cell of the update box */
  bptr = bestdist;
  for (i = BOX_R_ELEMS*BOX_G_ELEMS*BOX_B_ELEMS-1; i >= 0; i--)
    *bptr++ = 0x7FFFFFFFL;

  /* For each color selected by find_nearby_colors,
   * compute its distance to the center of each cell in the box.
   * If that's less than best-so-far, update best distance and color number.
   */

  /* Nominal steps between cell centers ("x" in Thomas article) */
#define STEP_R  ((1 << R_SHIFT) * R_SCALE)
#define STEP_G  ((1 << G_SHIFT) * G_SCALE)
#define STEP_B  ((1 << B_SHIFT) * B_SCALE)

  for (i = 0; i < numcolors; i++)
    {
      icolor = colorlist[i];
      /* Compute (square of) distance from minR/G/B to this color */
      inR = (minR - quantobj->clab[icolor].red) * R_SCALE;
      dist0 = inR*inR;
      /* special-case for L*==0: chroma diffs irrelevant */
      /*    if (minR > 0 || quantobj->clab[icolor].red > 0) */
      {
        inG = (minG - quantobj->clab[icolor].green) * G_SCALE;
        dist0 += inG*inG;
        inB = (minB - quantobj->clab[icolor].blue) * B_SCALE;
        dist0 += inB*inB;
      }
      /*    else
            {
            inG = 0;
            inB = 0;
            } */
      /* Form the initial difference increments */
      inR = inR * (2 * STEP_R) + STEP_R * STEP_R;
      inG = inG * (2 * STEP_G) + STEP_G * STEP_G;
      inB = inB * (2 * STEP_B) + STEP_B * STEP_B;
      /* Now loop over all cells in box, updating distance per Thomas method */
      bptr = bestdist;
      cptr = bestcolor;
      xx0 = inR;
      for (iR = BOX_R_ELEMS-1; iR >= 0; iR--)
        {
          dist1 = dist0;
          xx1 = inG;
          for (iG = BOX_G_ELEMS-1; iG >= 0; iG--)
            {
              dist2 = dist1;
              xx2 = inB;
              for (iB = BOX_B_ELEMS-1; iB >= 0; iB--)
                {
                  if (dist2 < *bptr)
                    {
                      *bptr = dist2;
                      *cptr = icolor;
                    }
                  dist2 += xx2;
                  xx2 += 2 * STEP_B * STEP_B;
                  bptr++;
                  cptr++;
                }
              dist1 += xx1;
              xx1 += 2 * STEP_G * STEP_G;
            }
          dist0 += xx0;
          xx0 += 2 * STEP_R * STEP_R;
        }
    }
}


/* Fill the inverse-colormap entries in the update box that contains
 * histogram cell R/G/B.  (Only that one cell MUST be filled, but we
 * can fill as many others as we wish.)
 */
static void
fill_inverse_cmap_gray (QuantizeObj *quantobj,
                        CFHistogram  histogram,
                        gint         pixel)
{
  Color *cmap = quantobj->cmap;
  gint64 mindist;
  gint   mindisti;
  gint   i;

  g_return_if_fail (quantobj->actual_number_of_colors > 0);

  mindist  = G_MAXLONG;
  mindisti = -1;

  for (i = 0; i < quantobj->actual_number_of_colors; i++)
    {
      gint64 dist = ABS (pixel - cmap[i].red);

      if (dist < mindist)
        {
          mindist  = dist;
          mindisti = i;

          if (mindist == 0)
            break;
        }
    }

  histogram[pixel] = mindisti + 1;
}


/* Fill the inverse-colormap entries in the update box that contains
 * histogram cell R/G/B.  (Only that one cell MUST be filled, but we
 * can fill as many others as we wish.)
 */
static void
fill_inverse_cmap_rgb (QuantizeObj *quantobj,
                       CFHistogram  histogram,
                       gint         R,
                       gint         G,
                       gint         B)
{
  gint  minR, minG, minB; /* lower left corner of update box */
  gint  iR, iG, iB;
  gint *cptr;           /* pointer into bestcolor[] array */
  /* This array lists the candidate colormap indexes. */
  gint  colorlist[MAXNUMCOLORS];
  gint  numcolors;                /* number of candidate colors */
  /* This array holds the actually closest colormap index for each cell. */
  gint  bestcolor[BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS] = { 0, };

  /* Convert cell coordinates to update box id */
  R >>= BOX_R_LOG;
  G >>= BOX_G_LOG;
  B >>= BOX_B_LOG;

  /* Compute true coordinates of update box's origin corner.
   * Actually we compute the coordinates of the center of the corner
   * histogram cell, which are the lower bounds of the volume we care about.
   */
  minR = (R << BOX_R_SHIFT) + ((1 << R_SHIFT) >> 1);
  minG = (G << BOX_G_SHIFT) + ((1 << G_SHIFT) >> 1);
  minB = (B << BOX_B_SHIFT) + ((1 << B_SHIFT) >> 1);

  /* Determine which colormap entries are close enough to be candidates
   * for the nearest entry to some cell in the update box.
   */
  numcolors = find_nearby_colors (quantobj, minR, minG, minB, colorlist);

  /* Determine the actually nearest colors. */
  find_best_colors (quantobj, minR, minG, minB, numcolors, colorlist,
                    bestcolor);

  /* Save the best color numbers (plus 1) in the main cache array */
  R <<= BOX_R_LOG;              /* convert id back to base cell indexes */
  G <<= BOX_G_LOG;
  B <<= BOX_B_LOG;
  cptr = bestcolor;
  for (iR = 0; iR < BOX_R_ELEMS; iR++)
    {
      for (iG = 0; iG < BOX_G_ELEMS; iG++)
        {
          for (iB = 0; iB < BOX_B_ELEMS; iB++)
            {
              *HIST_LIN (histogram, R + iR, G + iG, B + iB) = (*cptr++) + 1;
            }
        }
    }
}


/*  This is pass 1  */

static void
median_cut_pass1_gray (QuantizeObj *quantobj)
{
  select_colors_gray (quantobj, quantobj->histogram);
}

static void
snap_to_black_and_white (QuantizeObj *quantobj)
{
  /* find whitest and blackest colors in palette, if they are closer
   * than 24 units of euclidean distance in sRGB snap them to pure
   * black / white.
   */
#define POW2(a) ((a)*(a))
  gint   desired  = quantobj->desired_number_of_colors;
  gint   whitest  = 0;
  gint   blackest = 0;

  gint64 white_dist = POW2(255) * 3;
  gint64 black_dist = POW2(255) * 3;
  gint   i;

  for (i = 0; i < desired; i ++)
    {
       int dist;

       dist = POW2 (quantobj->cmap[i].red   - 255) +
              POW2 (quantobj->cmap[i].green - 255) +
              POW2( quantobj->cmap[i].blue  - 255);
       if (dist < white_dist)
         {
           white_dist = dist;
           whitest = i;
         }

       dist = POW2(quantobj->cmap[i].red   - 0) +
              POW2(quantobj->cmap[i].green - 0) +
              POW2(quantobj->cmap[i].blue  - 0);
       if (dist < black_dist)
         {
           black_dist = dist;
           blackest = i;
         }
    }

  if (desired > 2 &&
      had_white   &&
      white_dist < POW2(128))
  {
     quantobj->cmap[whitest].red   =
     quantobj->cmap[whitest].green =
     quantobj->cmap[whitest].blue  = 255;
  }
  if (desired > 2 &&
      had_black   &&
      black_dist < POW2(128))
  {
     quantobj->cmap[blackest].red   =
     quantobj->cmap[blackest].green =
     quantobj->cmap[blackest].blue  = 0;
  }
#undef POW2
}

static void
median_cut_pass1_rgb (QuantizeObj *quantobj)
{
  select_colors_rgb (quantobj, quantobj->histogram);
  snap_to_black_and_white (quantobj);
}


static void
monopal_pass1 (QuantizeObj *quantobj)
{
  quantobj->actual_number_of_colors = 2;

  quantobj->cmap[0].red   = 0;
  quantobj->cmap[0].green = 0;
  quantobj->cmap[0].blue  = 0;
  quantobj->cmap[1].red   = 255;
  quantobj->cmap[1].green = 255;
  quantobj->cmap[1].blue  = 255;
}

static void
webpal_pass1 (QuantizeObj *quantobj)
{
  int i;

  quantobj->actual_number_of_colors = 216;

  for (i=0; i < 216; i++)
    {
      quantobj->cmap[i].red   = webpal[i * 3];
      quantobj->cmap[i].green = webpal[i * 3 +1];
      quantobj->cmap[i].blue  = webpal[i * 3 +2];
    }
}

static void
custompal_pass1 (QuantizeObj *quantobj)
{
  gint   i;
  GList *list;

  /* fprintf(stderr,
             "custompal_pass1: using (theCustomPalette %s) from (file %s)\n",
             theCustomPalette->name, theCustomPalette->filename); */

  for (i = 0, list = gimp_palette_get_colors (quantobj->custom_palette);
       list;
       i++, list = g_list_next (list))
    {
      GimpPaletteEntry *entry = list->data;
      guchar            rgb[3];

      gegl_color_get_pixel (entry->color, babl_format_with_space ("R'G'B' u8", quantobj->space), rgb);

      quantobj->cmap[i].red   = (gint) rgb[0];
      quantobj->cmap[i].green = (gint) rgb[1];
      quantobj->cmap[i].blue  = (gint) rgb[2];
    }

  quantobj -> actual_number_of_colors = i;
}

/*
 * Map some rows of pixels to the output colormapped representation.
 */

static void
median_cut_pass2_no_dither_gray (QuantizeObj *quantobj,
                                 GimpLayer   *layer,
                                 GeglBuffer  *new_buffer)
{
  GeglBufferIterator *iter;
  CFHistogram         histogram = quantobj->histogram;
  ColorFreq          *cachep;
  const Babl         *src_format;
  const Babl         *dest_format;
  GeglRectangle      *src_roi;
  gint                src_bpp;
  gint                dest_bpp;
  gint                has_alpha;
  guint64            *index_used_count = quantobj->index_used_count;
  gboolean            dither_alpha     = quantobj->want_dither_alpha;
  gint                offsetx, offsety;

  gimp_item_get_offset (GIMP_ITEM (layer), &offsetx, &offsety);

  src_format  = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
  dest_format = gegl_buffer_get_format (new_buffer);

  src_bpp  = babl_format_get_bytes_per_pixel (src_format);
  dest_bpp = babl_format_get_bytes_per_pixel (dest_format);

  has_alpha = babl_format_has_alpha (src_format);

  iter = gegl_buffer_iterator_new (gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                                   NULL, 0, NULL,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);
  src_roi = &iter->items[0].roi;

  gegl_buffer_iterator_add (iter, new_buffer,
                            NULL, 0, NULL,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *src  = iter->items[0].data;
      guchar       *dest = iter->items[1].data;
      gint          row;

      for (row = 0; row < src_roi->height; row++)
        {
          gint col;

          for (col = 0; col < src_roi->width; col++)
            {
              /* get pixel value and index into the cache */
              gint pixel = src[GRAY];

              cachep = &histogram[pixel];
              /* If we have not seen this color before, find nearest
               * colormap entry and update the cache
               */
              if (*cachep == 0)
                fill_inverse_cmap_gray (quantobj, histogram, pixel);

              if (has_alpha)
                {
                  gboolean transparent = FALSE;

                  if (dither_alpha)
                    {
                      gint dither_x = (col + offsetx + src_roi->x) & DM_WIDTHMASK;
                      gint dither_y = (row + offsety + src_roi->y) & DM_HEIGHTMASK;

                      if ((src[ALPHA_G]) < DM[dither_x][dither_y])
                        transparent = TRUE;
                    }
                  else
                    {
                      if (src[ALPHA_G] <= 127)
                        transparent = TRUE;
                    }

                  if (transparent)
                    {
                      dest[ALPHA_I] = 0;
                    }
                  else
                    {
                      dest[ALPHA_I] = 255;
                      index_used_count[dest[INDEXED] = *cachep - 1]++;
                    }
                }
              else
                {
                  /* Now emit the colormap index for this cell */
                  index_used_count[dest[INDEXED] = *cachep - 1]++;
                }

              src  += src_bpp;
              dest += dest_bpp;
            }
        }
    }
}

static void
median_cut_pass2_fixed_dither_gray (QuantizeObj *quantobj,
                                    GimpLayer   *layer,
                                    GeglBuffer  *new_buffer)
{
  GeglBufferIterator *iter;
  CFHistogram         histogram = quantobj->histogram;
  ColorFreq          *cachep;
  const Babl         *src_format;
  const Babl         *dest_format;
  GeglRectangle      *src_roi;
  gint                src_bpp;
  gint                dest_bpp;
  gboolean            has_alpha;
  gint                pixval1 = 0;
  gint                pixval2 = 0;
  gint                err1;
  gint                err2;
  Color              *color1;
  Color              *color2;
  guint64            *index_used_count = quantobj->index_used_count;
  gboolean            dither_alpha     = quantobj->want_dither_alpha;
  gint                offsetx, offsety;

  gimp_item_get_offset (GIMP_ITEM (layer), &offsetx, &offsety);

  src_format  = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
  dest_format = gegl_buffer_get_format (new_buffer);

  src_bpp  = babl_format_get_bytes_per_pixel (src_format);
  dest_bpp = babl_format_get_bytes_per_pixel (dest_format);

  has_alpha = babl_format_has_alpha (src_format);

  iter = gegl_buffer_iterator_new (gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                                   NULL, 0, NULL,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);
  src_roi = &iter->items[0].roi;

  gegl_buffer_iterator_add (iter, new_buffer,
                            NULL, 0, NULL,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *src  = iter->items[0].data;
      guchar       *dest = iter->items[1].data;
      gint          row;

      for (row = 0; row < src_roi->height; row++)
        {
          gint col;

          for (col = 0; col < src_roi->width; col++)
            {
              gint       pixel;
              const gint dmval =
                DM[(col + offsetx + src_roi->x) & DM_WIDTHMASK]
                [(row + offsety + src_roi->y) & DM_HEIGHTMASK];

              /* get pixel value and index into the cache */
              pixel = src[GRAY];

              cachep = &histogram[pixel];
              /* If we have not seen this color before, find nearest
               * colormap entry and update the cache
               */
              if (*cachep == 0)
                fill_inverse_cmap_gray (quantobj, histogram, pixel);

              pixval1 = *cachep - 1;
              color1 = &quantobj->cmap[pixval1];

              if (quantobj->actual_number_of_colors > 2)
                {
                  const int re = src[GRAY] - (int)color1->red;
                  int RV = src[GRAY] + re;

                  do
                    {
                      const gint R = CLAMP0255 (RV);

                      cachep = &histogram[R];
                      /* If we have not seen this color before, find
                       * nearest colormap entry and update the cache
                       */
                      if (*cachep == 0)
                        fill_inverse_cmap_gray (quantobj, histogram, R);

                      pixval2 = *cachep - 1;
                      RV += re;
                    }
                  while ((pixval1 == pixval2) &&
                         (! (RV>255 || RV<0) ) &&
                         re);
                }
              else
                {
                  /* not enough colors to bother looking for an 'alternative'
                     color (we may fail to do so anyway), so decide that
                     the alternative color is simply the other cmap entry. */
                  pixval2 = (pixval1 + 1) %
                    (quantobj->actual_number_of_colors);
                }

              /* always deterministically sort pixval1 and pixval2, to
                 avoid artifacts in the dither range due to inverting our
                 relative color viewpoint -- most obvious in 1-bit dither. */
              if (pixval1 > pixval2)
                {
                  gint tmpval = pixval1;
                  pixval1 = pixval2;
                  pixval2 = tmpval;
                  color1 = &quantobj->cmap[pixval1];
                }

              color2 = &quantobj->cmap[pixval2];

              err1 = ABS(color1->red - src[GRAY]);
              err2 = ABS(color2->red - src[GRAY]);
              if (err1 || err2)
                {
                  const int proportion2 = (256 * 255 * err2) / (err1 + err2);

                  if ((dmval * 256) > proportion2)
                    {
                      pixval1 = pixval2; /* use color2 instead of color1*/
                    }
                }

              if (has_alpha)
                {
                  gboolean transparent = FALSE;

                  if (dither_alpha)
                    {
                      if (src[ALPHA_G] < dmval)
                        transparent = TRUE;
                    }
                  else
                    {
                      if (src[ALPHA_G] <= 127)
                        transparent = TRUE;
                    }

                  if (transparent)
                    {
                      dest[ALPHA_I] = 0;
                    }
                  else
                    {
                      dest[ALPHA_I] = 255;
                      index_used_count[dest[INDEXED] = pixval1]++;
                    }
                }
              else
                {
                  /* Now emit the colormap index for this cell, barfbarf */
                  index_used_count[dest[INDEXED] = pixval1]++;
                }

              src  += src_bpp;
              dest += dest_bpp;
            }
        }
    }
}

static void
median_cut_pass2_no_dither_rgb (QuantizeObj *quantobj,
                                GimpLayer   *layer,
                                GeglBuffer  *new_buffer)
{
  GeglBufferIterator *iter;
  CFHistogram         histogram = quantobj->histogram;
  ColorFreq          *cachep;
  const Babl         *src_format;
  const Babl         *dest_format;
  GeglRectangle      *src_roi;
  gint                src_bpp;
  gint                dest_bpp;
  gint                has_alpha;
  gint                R, G, B;
  gint                red_pix          = RED;
  gint                green_pix        = GREEN;
  gint                blue_pix         = BLUE;
  gint                alpha_pix        = ALPHA;
  gboolean            dither_alpha     = quantobj->want_dither_alpha;
  gint                offsetx, offsety;
  guint64            *index_used_count = quantobj->index_used_count;
  gint64              total_size       = 0;
  gint64              layer_size;
  gint                count            = 0;

  gimp_item_get_offset (GIMP_ITEM (layer), &offsetx, &offsety);

  src_format  = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
  dest_format = gegl_buffer_get_format (new_buffer);

  src_bpp  = babl_format_get_bytes_per_pixel (src_format);
  dest_bpp = babl_format_get_bytes_per_pixel (dest_format);

  has_alpha = babl_format_has_alpha (src_format);

  /*  In the case of web/mono palettes, we actually force
   *   grayscale drawables through the rgb pass2 functions
   */
  if (gimp_drawable_is_gray (GIMP_DRAWABLE (layer)))
    {
      red_pix = green_pix = blue_pix = GRAY;
      alpha_pix = ALPHA_G;
    }

  iter = gegl_buffer_iterator_new (gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                                   NULL, 0, NULL,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);
  src_roi = &iter->items[0].roi;

  gegl_buffer_iterator_add (iter, new_buffer,
                            NULL, 0, NULL,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  layer_size = (gimp_item_get_width  (GIMP_ITEM (layer)) *
                gimp_item_get_height (GIMP_ITEM (layer)));

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *src  = iter->items[0].data;
      guchar       *dest = iter->items[1].data;
      gint          row;

      total_size += src_roi->height * src_roi->width;

      for (row = 0; row < src_roi->height; row++)
        {
          gint col;

          for (col = 0; col < src_roi->width; col++)
            {
              if (has_alpha)
                {
                  gboolean transparent = FALSE;

                  if (dither_alpha)
                    {
                      gint dither_x = (col + offsetx + src_roi->x) & DM_WIDTHMASK;
                      gint dither_y = (row + offsety + src_roi->y) & DM_HEIGHTMASK;

                      if ((src[alpha_pix]) < DM[dither_x][dither_y])
                        transparent = TRUE;
                    }
                  else
                    {
                      if (src[alpha_pix] <= 127)
                        transparent = TRUE;
                    }

                  if (transparent)
                    {
                      dest[ALPHA_I] = 0;
                      goto next_pixel;
                    }
                  else
                    {
                      dest[ALPHA_I] = 255;
                    }
                }

              /* get pixel value and index into the cache */
              rgb_to_lin (src[red_pix], src[green_pix], src[blue_pix],
                          &R, &G, &B);

              cachep = HIST_LIN (histogram, R, G, B);
              /* If we have not seen this color before, find nearest
               * colormap entry and update the cache
               */
              if (*cachep == 0)
                fill_inverse_cmap_rgb (quantobj, histogram, R, G, B);

              /* Now emit the colormap index for this cell, barfbarf */
              index_used_count[dest[INDEXED] = *cachep - 1]++;

            next_pixel:

              src  += src_bpp;
              dest += dest_bpp;
            }
        }

      if (quantobj->progress && (count % 16 == 0))
         gimp_progress_set_value (quantobj->progress,
                                  (gdouble) total_size / (gdouble) layer_size);
    }
}

static void
median_cut_pass2_fixed_dither_rgb (QuantizeObj *quantobj,
                                   GimpLayer   *layer,
                                   GeglBuffer  *new_buffer)
{
  GeglBufferIterator *iter;
  CFHistogram         histogram = quantobj->histogram;
  ColorFreq          *cachep;
  const Babl         *src_format;
  const Babl         *dest_format;
  GeglRectangle      *src_roi;
  gint                src_bpp;
  gint                dest_bpp;
  gint                has_alpha;
  gint                pixval1 = 0;
  gint                pixval2 = 0;
  Color              *color1;
  Color              *color2;
  gint                R, G, B;
  gint                err1;
  gint                err2;
  gint                red_pix          = RED;
  gint                green_pix        = GREEN;
  gint                blue_pix         = BLUE;
  gint                alpha_pix        = ALPHA;
  gboolean            dither_alpha     = quantobj->want_dither_alpha;
  gint                offsetx, offsety;
  guint64            *index_used_count = quantobj->index_used_count;
  gint64              total_size       = 0;
  gint64              layer_size;
  gint                count            = 0;

  gimp_item_get_offset (GIMP_ITEM (layer), &offsetx, &offsety);

  src_format  = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
  dest_format = gegl_buffer_get_format (new_buffer);

  src_bpp  = babl_format_get_bytes_per_pixel (src_format);
  dest_bpp = babl_format_get_bytes_per_pixel (dest_format);

  has_alpha = babl_format_has_alpha (src_format);

  /*  In the case of web/mono palettes, we actually force
   *   grayscale drawables through the rgb pass2 functions
   */
  if (gimp_drawable_is_gray (GIMP_DRAWABLE (layer)))
    {
      red_pix = green_pix = blue_pix = GRAY;
      alpha_pix = ALPHA_G;
    }

  iter = gegl_buffer_iterator_new (gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                                   NULL, 0, NULL,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);
  src_roi = &iter->items[0].roi;

  gegl_buffer_iterator_add (iter, new_buffer,
                            NULL, 0, NULL,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  layer_size = (gimp_item_get_width  (GIMP_ITEM (layer)) *
                gimp_item_get_height (GIMP_ITEM (layer)));

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *src  = iter->items[0].data;
      guchar       *dest = iter->items[1].data;
      gint          row;

      total_size += src_roi->height * src_roi->width;

      for (row = 0; row < src_roi->height; row++)
        {
          gint col;

          for (col = 0; col < src_roi->width; col++)
            {
              const int dmval =
                DM[(col + offsetx + src_roi->x) & DM_WIDTHMASK]
                [(row + offsety + src_roi->y) & DM_HEIGHTMASK];

              if (has_alpha)
                {
                  gboolean transparent = FALSE;

                  if (dither_alpha)
                    {
                      if (src[alpha_pix] < dmval)
                        transparent = TRUE;
                    }
                  else
                    {
                      if (src[alpha_pix] <= 127)
                        transparent = TRUE;
                    }

                  if (transparent)
                    {
                      dest[ALPHA_I] = 0;
                      goto next_pixel;
                    }
                  else
                    {
                      dest[ALPHA_I] = 255;
                    }
                }

              /* get pixel value and index into the cache */
              rgb_to_lin (src[red_pix], src[green_pix], src[blue_pix],
                          &R, &G, &B);

              cachep = HIST_LIN (histogram, R, G, B);
              /* If we have not seen this color before, find nearest
               * colormap entry and update the cache
               */
              if (*cachep == 0)
                fill_inverse_cmap_rgb (quantobj, histogram, R, G, B);

              /* We now try to find a color which, when mixed in some
               * fashion with the closest match, yields something
               * closer to the desired color.  We do this by
               * repeatedly extrapolating the color vector from one to
               * the other until we find another color cell.  Then we
               * assess the distance of both mixer colors from the
               * intended color to determine their relative
               * probabilities of being chosen.
               */
              pixval1 = *cachep - 1;
              color1 = &quantobj->cmap[pixval1];

              if (quantobj->actual_number_of_colors > 2)
                {
                  const gint re = src[red_pix]   - (gint) color1->red;
                  const gint ge = src[green_pix] - (gint) color1->green;
                  const gint be = src[blue_pix]  - (gint) color1->blue;
                  gint       RV = src[red_pix]   + re;
                  gint       GV = src[green_pix] + ge;
                  gint       BV = src[blue_pix]  + be;

                  do
                    {
                      rgb_to_lin ((CLAMP0255(RV)),
                                  (CLAMP0255(GV)),
                                  (CLAMP0255(BV)),
                                  &R, &G, &B);

                      cachep = HIST_LIN (histogram, R, G, B);
                      /* If we have not seen this color before, find
                       * nearest colormap entry and update the cache
                       */
                      if (*cachep == 0)
                        fill_inverse_cmap_rgb (quantobj, histogram, R, G, B);

                      pixval2 = *cachep - 1;
                      RV += re;  GV += ge;  BV += be;
                    }
                  while ((pixval1 == pixval2) &&
                         (!( (RV>255 || RV<0) || (GV>255 || GV<0) || (BV>255 || BV<0) )) &&
                         (re || ge || be));
                }

              if (quantobj->actual_number_of_colors <= 2
                  /* || pixval1 == pixval2 */) {
                /* not enough colors to bother looking for an 'alternative'
                   color (we may fail to do so anyway), so decide that
                   the alternative color is simply the other cmap entry. */
                pixval2 = (pixval1 + 1) %
                  (quantobj->actual_number_of_colors);
              }

              /* always deterministically sort pixval1 and pixval2, to
                 avoid artifacts in the dither range due to inverting our
                 relative color viewpoint -- most obvious in 1-bit dither. */
              if (pixval1 > pixval2)
                {
                  gint tmpval = pixval1;
                  pixval1 = pixval2;
                  pixval2 = tmpval;
                  color1 = &quantobj->cmap[pixval1];
                }

              color2 = &quantobj->cmap[pixval2];

              /* now figure out the relative probabilities of choosing
                 either of our candidates. */
#define DISTP(R1,G1,B1,R2,G2,B2,D) do {D = sqrt( 30*SQR((R1)-(R2)) + \
                                                 59*SQR((G1)-(G2)) + \
                                                 11*SQR((B1)-(B2)) ); }while(0)
#define LIN_DISTP(R1,G1,B1,R2,G2,B2,D) do { \
                int spacer1, spaceg1, spaceb1; \
                int spacer2, spaceg2, spaceb2; \
                rgb_to_unshifted_lin (R1,G1,B1, &spacer1, &spaceg1, &spaceb1); \
                rgb_to_unshifted_lin (R2,G2,B2, &spacer2, &spaceg2, &spaceb2); \
                D = sqrt(R_SCALE * SQR((spacer1)-(spacer2)) +           \
                         G_SCALE * SQR((spaceg1)-(spaceg2)) + \
                         B_SCALE * SQR((spaceb1)-(spaceb2))); \
              } while(0)

              /* although LIN_DISTP is more correct, DISTP is much faster and
                 barely distinguishable. */
              DISTP (color1->red, color1->green, color1->blue,
                     src[red_pix], src[green_pix], src[blue_pix],
                     err1);
              DISTP (color2->red, color2->green, color2->blue,
                     src[red_pix], src[green_pix], src[blue_pix],
                     err2);

              if (err1 || err2)
                {
                  const int proportion2 = (255 * err2) / (err1 + err2);
                  if (dmval > proportion2)
                    {
                      pixval1 = pixval2; /* use color2 instead of color1*/
                    }
                }

              /* Now emit the colormap index for this cell, barfbarf */
              index_used_count[dest[INDEXED] = pixval1]++;

            next_pixel:

              src  += src_bpp;
              dest += dest_bpp;
            }
        }

      if (quantobj->progress && (count % 16 == 0))
        gimp_progress_set_value (quantobj->progress,
                                 (gdouble) total_size / (gdouble) layer_size);
    }
}

static void
median_cut_pass2_nodestruct_dither_rgb (QuantizeObj *quantobj,
                                        GimpLayer   *layer,
                                        GeglBuffer  *new_buffer)
{
  GeglBufferIterator *iter;
  const Babl         *src_format;
  const Babl         *dest_format;
  GeglRectangle      *src_roi;
  gint                src_bpp;
  gint                dest_bpp;
  gint                has_alpha;
  gboolean            dither_alpha = quantobj->want_dither_alpha;
  gint                red_pix      = RED;
  gint                green_pix    = GREEN;
  gint                blue_pix     = BLUE;
  gint                alpha_pix    = ALPHA;
  gint                lastindex    = 0;
  gint                lastred      = -1;
  gint                lastgreen    = -1;
  gint                lastblue     = -1;
  gint                offsetx, offsety;

  gimp_item_get_offset (GIMP_ITEM (layer), &offsetx, &offsety);

  src_format  = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
  dest_format = gegl_buffer_get_format (new_buffer);

  src_bpp  = babl_format_get_bytes_per_pixel (src_format);
  dest_bpp = babl_format_get_bytes_per_pixel (dest_format);

  has_alpha = babl_format_has_alpha (src_format);

  iter = gegl_buffer_iterator_new (gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                                   NULL, 0, NULL,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);
  src_roi = &iter->items[0].roi;

  gegl_buffer_iterator_add (iter, new_buffer,
                            NULL, 0, NULL,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *src  = iter->items[0].data;
      guchar       *dest = iter->items[1].data;
      gint          row;

      for (row = 0; row < src_roi->height; row++)
        {
          gint col;

          for (col = 0; col < src_roi->width; col++)
            {
              gboolean transparent = FALSE;

              if (has_alpha)
                {
                  if (dither_alpha)
                    {
                      gint dither_x = (col + src_roi->x + offsetx) & DM_WIDTHMASK;
                      gint dither_y = (row + src_roi->y + offsety) & DM_HEIGHTMASK;

                      if ((src[alpha_pix]) < DM[dither_x][dither_y])
                        transparent = TRUE;
                    }
                  else
                    {
                      if (src[alpha_pix] < 128)
                        transparent = TRUE;
                    }
                }

              if (! transparent)
                {
                  if ((lastred   == src[red_pix]) &&
                      (lastgreen == src[green_pix]) &&
                      (lastblue  == src[blue_pix]))
                    {
                      /*  same pixel color as last time  */
                      dest[INDEXED] = lastindex;
                      if (has_alpha)
                        dest[ALPHA_I] = 255;
                    }
                  else
                    {
                      gint i;

                      for (i = 0 ;
                           i < quantobj->actual_number_of_colors;
                           i++)
                        {
                          if ((quantobj->cmap[i].green == src[green_pix]) &&
                              (quantobj->cmap[i].red   == src[red_pix]) &&
                              (quantobj->cmap[i].blue  == src[blue_pix]))
                          {
                            lastred   = src[red_pix];
                            lastgreen = src[green_pix];
                            lastblue  = src[blue_pix];
                            lastindex = i;

                            goto got_color;
                          }
                        }
                      g_error ("Non-existent color was expected to "
                               "be in non-destructive colormap.");
                    got_color:
                      dest[INDEXED] = lastindex;
                      if (has_alpha)
                        dest[ALPHA_I] = 255;
                    }
                }
              else
                { /*  have alpha, and transparent  */
                  dest[ALPHA_I] = 0;
                }

              src  += src_bpp;
              dest += dest_bpp;
            }
        }
    }
}


/*
 * Initialize the error-limiting transfer function (lookup table).
 * The raw F-S error computation can potentially compute error values of up to
 * +- MAXJSAMPLE.  But we want the maximum correction applied to a pixel to be
 * much less, otherwise obviously wrong pixels will be created.  (Typical
 * effects include weird fringes at color-area boundaries, isolated bright
 * pixels in a dark area, etc.)  The standard advice for avoiding this problem
 * is to ensure that the "corners" of the color cube are allocated as output
 * colors; then repeated errors in the same direction cannot cause cascading
 * error buildup.  However, that only prevents the error from getting
 * completely out of hand; Aaron Giles reports that error limiting improves
 * the results even with corner colors allocated.
 * A simple clamping of the error values to about +- MAXJSAMPLE/8 works pretty
 * well, but the smoother transfer function used below is even better.  Thanks
 * to Aaron Giles for this idea.
 */

static int error_limit_16(int error_freedom, int in)
{
  if (error_freedom)
  {
    int neg = in < 0 ? -1 : 1;
    int val = abs(in);
    if (val < 24 * 256)
       return neg * val;
    if (val < 24 * 2 * 256)
       return neg * (((val-24*256)/2)+24*256);
    return neg * 24 * 2 * 256;
  }
  else
    return CLAMP(in, -192*256, 192*256);
}

/*
 * Map some rows of pixels to the output colormapped representation.
 * Perform floyd-steinberg dithering.
 */

static void
median_cut_pass2_fs_dither_gray (QuantizeObj *quantobj,
                                 GimpLayer   *layer,
                                 GeglBuffer  *new_buffer)
{
  GeglBuffer   *src_buffer;
  CFHistogram   histogram = quantobj->histogram;
  ColorFreq    *cachep;
  Color        *color;
  const Babl   *src_format;
  const Babl   *dest_format;
  gint          src_bpp;
  gint          dest_bpp;
  guchar       *src_buf, *dest_buf;
  gint         *next_row, *prev_row;
  gint         *nr, *pr;
  gint         *tmp;
  gint          pixel;
  gint          pixel_lin;
  gint          pixele;
  gint          row, col;
  gint          index;
  gint          step_dest, step_src;
  gint          odd_row;
  gboolean      has_alpha;
  gint          offsetx, offsety;
  gboolean      dither_alpha = quantobj->want_dither_alpha;
  gint          width, height;
  guint64      *index_used_count = quantobj->index_used_count;

  src_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gimp_item_get_offset (GIMP_ITEM (layer), &offsetx, &offsety);

  src_format  = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
  dest_format = gegl_buffer_get_format (new_buffer);

  src_bpp  = babl_format_get_bytes_per_pixel (src_format);
  dest_bpp = babl_format_get_bytes_per_pixel (dest_format);

  has_alpha = babl_format_has_alpha (src_format);

  width  = gimp_item_get_width  (GIMP_ITEM (layer));
  height = gimp_item_get_height (GIMP_ITEM (layer));

  src_buf  = g_malloc (width * src_bpp);
  dest_buf = g_malloc (width * dest_bpp);

  next_row = g_new (gint, width + 2);
  prev_row = g_new0 (gint, width + 2);

  odd_row = 0;

  for (row = 0; row < height; row++)
    {
      const guchar *src;
      guchar       *dest;

      gegl_buffer_get (src_buffer, GEGL_RECTANGLE (0, row, width, 1),
                       1.0, NULL, src_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      src  = src_buf;
      dest = dest_buf;

      nr = next_row;
      pr = prev_row + 1;

      if (odd_row)
        {
          step_dest = -dest_bpp;
          step_src  = -src_bpp;

          src  += (width * src_bpp) - src_bpp;
          dest += (width * dest_bpp) - dest_bpp;

          nr += width + 1;
          pr += width;

          *(nr - 1) = 0;
        }
      else
        {
          step_dest = dest_bpp;
          step_src  = src_bpp;

          *(nr + 1) = 0;
        }

      *nr = 0;

      for (col = 0; col < width; col++)
        {
          pixel_lin = gray_to_linear(src[GRAY]) + error_limit_16(quantobj->error_freedom, *pr);

          pixel_lin = CLAMP(pixel_lin, 0, 65535);

          pixel = linear_to_u8 (pixel_lin);

          cachep = &histogram[pixel];
          /* If we have not seen this color before, find nearest
           * colormap entry and update the cache
           */
          if (*cachep == 0)
            fill_inverse_cmap_gray (quantobj, histogram, pixel);

          if (has_alpha)
            {
              gboolean transparent = FALSE;

              if (odd_row)
                {
                  if (dither_alpha)
                    {
                      gint dither_x = ((width-col)+offsetx-1) & DM_WIDTHMASK;
                      gint dither_y = (row+offsety) & DM_HEIGHTMASK;

                      if ((src[ALPHA_G]) < DM[dither_x][dither_y])
                        transparent = TRUE;
                    }
                  else
                    {
                      if (src[ALPHA_G] <= 127)
                        transparent = TRUE;
                    }

                  if (transparent)
                    {
                      dest[ALPHA_I] = 0;
                      pr--;
                      nr--;
                      *(nr - 1) = 0;
                      goto next_pixel;
                    }
                  else
                    {
                      dest[ALPHA_I] = 255;
                    }
                }
              else
                {
                  if (dither_alpha)
                    {
                      gint dither_x = (col + offsetx) & DM_WIDTHMASK;
                      gint dither_y = (row + offsety) & DM_HEIGHTMASK;

                      if ((src[ALPHA_G]) < DM[dither_x][dither_y])
                        transparent = TRUE;
                    }
                  else
                    {
                      if (src[ALPHA_G] <= 127)
                        transparent = TRUE;
                    }

                  if (transparent)
                    {
                      dest[ALPHA_I] = 0;
                      pr++;
                      nr++;
                      *(nr + 1) = 0;
                      goto next_pixel;
                    }
                  else
                    {
                      dest[ALPHA_I] = 255;
                    }
                }
            }

          index = *cachep - 1;
          index_used_count[dest[INDEXED] = index]++;

          color = &quantobj->clin[index];
          pixele = pixel_lin - color->red;

          if (odd_row)
            {
              *(--pr) += (7 * pixele)>>4;
              *nr-- += (3 * pixele)>>4;
              *nr += (5 * pixele)>>4;
              *(nr-1) = (1*pixele)>>4;
            }
          else
            {
              *(++pr) += (7 * pixele)>>4;
              *nr++ += (3 * pixele)>>4;
              *nr += (5 * pixele)>>4;
              *(nr+1) = (1*pixele)>>4;
            }

        next_pixel:

          dest += step_dest;
          src += step_src;
        }

      tmp = next_row;
      next_row = prev_row;
      prev_row = tmp;

      odd_row = !odd_row;

      gegl_buffer_set (new_buffer, GEGL_RECTANGLE (0, row, width, 1),
                       0, NULL, dest_buf,
                       GEGL_AUTO_ROWSTRIDE);
    }

  g_free (next_row);
  g_free (prev_row);
  g_free (src_buf);
  g_free (dest_buf);
}

static void
median_cut_pass2_rgb_init (QuantizeObj *quantobj)
{
  int i;

  zero_histogram_rgb (quantobj->histogram);

  /* Mark all indices as currently unused */
  memset (quantobj->index_used_count, 0, 256 * sizeof (guint64));

  /* Make a version of our discovered colormap in lab space */
  for (i = 0; i < quantobj->actual_number_of_colors; i++)
    {
      rgb_to_unshifted_lin (quantobj->cmap[i].red,
                            quantobj->cmap[i].green,
                            quantobj->cmap[i].blue,
                            &quantobj->clab[i].red,
                            &quantobj->clab[i].green,
                            &quantobj->clab[i].blue);
    }
  /* Make a version of our discovered colormap in linear space */
  for (i = 0; i < quantobj->actual_number_of_colors; i++)
    {
      rgb_to_linear        (quantobj->cmap[i].red,
                            quantobj->cmap[i].green,
                            quantobj->cmap[i].blue,
                            &quantobj->clin[i].red,
                            &quantobj->clin[i].green,
                            &quantobj->clin[i].blue);
    }
}

static void
median_cut_pass2_gray_init (QuantizeObj *quantobj)
{
  zero_histogram_gray (quantobj->histogram);

  /* Mark all indices as currently unused */
  memset (quantobj->index_used_count, 0, 256 * sizeof (guint64));
  /* Make a version of our discovered colormap in linear space */
  for (int i = 0; i < quantobj->actual_number_of_colors; i++)
    {
      rgb_to_linear        (quantobj->cmap[i].red,
                            quantobj->cmap[i].green,
                            quantobj->cmap[i].blue,
                            &quantobj->clin[i].red,
                            &quantobj->clin[i].green,
                            &quantobj->clin[i].blue);
    }
}


static void
median_cut_pass2_fs_dither_rgb (QuantizeObj *quantobj,
                                GimpLayer   *layer,
                                GeglBuffer  *new_buffer)
{
  GeglBuffer   *src_buffer;
  CFHistogram   histogram = quantobj->histogram;
  ColorFreq    *cachep;
  Color        *linearcolor;
  const Babl   *src_format;
  const Babl   *dest_format;
  gint          src_bpp;
  gint          dest_bpp;
  guchar       *src_buf, *dest_buf;
  gint         *red_n_row, *red_p_row;
  gint         *grn_n_row, *grn_p_row;
  gint         *blu_n_row, *blu_p_row;
  gint         *rnr, *rpr;
  gint         *gnr, *gpr;
  gint         *bnr, *bpr;
  gint         *tmp;
  gint          re, ge, be;
  gint          row, col;
  gint          index;
  gint          step_dest, step_src;
  gint          odd_row;
  gboolean      has_alpha;
  gint          width, height;
  gint          red_pix   = RED;
  gint          green_pix = GREEN;
  gint          blue_pix  = BLUE;
  gint          alpha_pix = ALPHA;
  gint          offsetx, offsety;
  gboolean      dither_alpha     = quantobj->want_dither_alpha;
  guint64      *index_used_count = quantobj->index_used_count;
  gint          global_rmax = 0, global_rmin = G_MAXINT;
  gint          global_gmax = 0, global_gmin = G_MAXINT;
  gint          global_bmax = 0, global_bmin = G_MAXINT;

  src_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gimp_item_get_offset (GIMP_ITEM (layer), &offsetx, &offsety);

  /*  In the case of web/mono palettes, we actually force
   *   grayscale drawables through the rgb pass2 functions
   */
  if (gimp_drawable_is_gray (GIMP_DRAWABLE (layer)))
    red_pix = green_pix = blue_pix = GRAY;

  src_format  = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
  dest_format = gegl_buffer_get_format (new_buffer);

  src_bpp  = babl_format_get_bytes_per_pixel (src_format);
  dest_bpp = babl_format_get_bytes_per_pixel (dest_format);

  has_alpha = babl_format_has_alpha (src_format);

  width  = gimp_item_get_width  (GIMP_ITEM (layer));
  height = gimp_item_get_height (GIMP_ITEM (layer));

  /* find the bounding box of the palette colors --
     we use this for hard-clamping our error-corrected
     values so that we can't continuously accelerate outside
     of our attainable gamut, which looks icky. */
  for (index = 0; index < quantobj->actual_number_of_colors; index++)
    {
      global_rmax = MAX(global_rmax, quantobj->clin[index].red);
      global_rmin = MIN(global_rmin, quantobj->clin[index].red);
      global_gmax = MAX(global_gmax, quantobj->clin[index].green);
      global_gmin = MIN(global_gmin, quantobj->clin[index].green);
      global_bmax = MAX(global_bmax, quantobj->clin[index].blue);
      global_bmin = MIN(global_bmin, quantobj->clin[index].blue);
    }

  src_buf  = g_malloc (width * src_bpp);
  dest_buf = g_malloc (width * dest_bpp);

  red_n_row = g_new (gint, width + 2);
  red_p_row = g_new0 (gint, width + 2);
  grn_n_row = g_new (gint, width + 2);
  grn_p_row = g_new0 (gint, width + 2);
  blu_n_row = g_new (gint, width + 2);
  blu_p_row = g_new0 (gint, width + 2);

  odd_row = 0;

  for (row = 0; row < height; row++)
    {
      const guchar *src;
      guchar       *dest;

      gegl_buffer_get (src_buffer, GEGL_RECTANGLE (0, row, width, 1),
                       1.0, NULL, src_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      src  = src_buf;
      dest = dest_buf;

      rnr = red_n_row;
      gnr = grn_n_row;
      bnr = blu_n_row;
      rpr = red_p_row + 1;
      gpr = grn_p_row + 1;
      bpr = blu_p_row + 1;

      if (odd_row)
        {
          step_dest = -dest_bpp;
          step_src  = -src_bpp;

          src += (width * src_bpp) - src_bpp;
          dest += (width * dest_bpp) - dest_bpp;

          rnr += width + 1;
          gnr += width + 1;
          bnr += width + 1;
          rpr += width;
          gpr += width;
          bpr += width;

          *(rnr - 1) = *(gnr - 1) = *(bnr - 1) = 0;
        }
      else
        {
          step_dest = dest_bpp;
          step_src  = src_bpp;

          *(rnr + 1) = *(gnr + 1) = *(bnr + 1) = 0;
        }

      *rnr = *gnr = *bnr = 0;

      for (col = 0; col < width; col++)
        {
          if (has_alpha)
            {
              gboolean transparent = FALSE;

              if (odd_row)
                {
                  if (dither_alpha)
                    {
                      gint dither_x = ((width-col)+offsetx-1) & DM_WIDTHMASK;
                      gint dither_y = (row+offsety) & DM_HEIGHTMASK;

                      if ((src[alpha_pix]) < DM[dither_x][dither_y])
                        transparent = TRUE;
                    }
                  else
                    {
                      if (src[alpha_pix] <= 127)
                        transparent = TRUE;
                    }

                  if (transparent)
                    {
                      dest[ALPHA_I] = 0;
                      rpr--; gpr--; bpr--;
                      rnr--; gnr--; bnr--;
                      *(rnr - 1) = *(gnr - 1) = *(bnr - 1) = 0;
                      goto next_pixel;
                    }
                  else
                    {
                      dest[ALPHA_I] = 255;
                    }
                }
              else
                {
                  if (dither_alpha)
                    {
                      gint dither_x = (col + offsetx) & DM_WIDTHMASK;
                      gint dither_y = (row + offsety) & DM_HEIGHTMASK;

                      if ((src[alpha_pix]) < DM[dither_x][dither_y])
                        transparent = TRUE;
                    }
                  else
                    {
                      if (src[alpha_pix] <= 127)
                        transparent = TRUE;
                    }

                  if (transparent)
                    {
                      dest[ALPHA_I] = 0;
                      rpr++; gpr++; bpr++;
                      rnr++; gnr++; bnr++;
                      *(rnr + 1) = *(gnr + 1) = *(bnr + 1) = 0;
                      goto next_pixel;
                    }
                  else
                    {
                      dest[ALPHA_I] = 255;
                    }
                }
            }

          rgb_to_linear (src[red_pix], src[green_pix], src[blue_pix],
                         &re, &ge, &be);

          *rpr = error_limit_16 (quantobj->error_freedom, *rpr);
          *gpr = error_limit_16 (quantobj->error_freedom, *gpr);
          *bpr = error_limit_16 (quantobj->error_freedom, *bpr);

          re = re + *rpr;
          ge = ge + *gpr;
          be = be + *bpr;

          // in theory we should only do this before the look-up
          // comments in the source indicates that error running
          // away from gamut looks icky - for now trust that
          re = CLAMP(re, global_rmin, global_rmax);
          ge = CLAMP(ge, global_gmin, global_gmax);
          be = CLAMP(be, global_bmin, global_bmax);

          {
            guint16 rgb16[3]={re,ge,be};
            float rgbF[3];
            gint lab8[3];
            babl_process (linear_to_rgb_float_fish, rgb16, rgbF, 1);
            rgb_to_unshifted_lin(rgbF[0]*255,rgbF[1]*255,rgbF[2]*255, &lab8[0], &lab8[1], &lab8[2]);
            cachep = HIST_LIN (histogram,
                               RSDF (lab8[0]),
                               GSDF (lab8[1]),
                               BSDF (lab8[2]));
            /* If we have not seen this color before, find nearest
             * colormap entry and update the cache
             */
            if (*cachep == 0)
              fill_inverse_cmap_rgb (quantobj, histogram,
                                     RSDF (lab8[0]),
                                     GSDF (lab8[1]),
                                     BSDF (lab8[2]));
          }

          index = *cachep - 1;
          index_used_count[index]++;
          dest[INDEXED] = index;

          linearcolor = &quantobj->clin[index];

           re = re - linearcolor->red;
           ge = ge - linearcolor->green;
           be = be - linearcolor->blue;

          if (odd_row)
            {
              *(--rpr) += (7 * re)>>4;
              *(--gpr) += (7 * ge)>>4;
              *(--bpr) += (7 * be)>>4;

              *rnr-- += (3 * re)>>4;
              *gnr-- += (3 * ge)>>4;
              *bnr-- += (3 * be)>>4;

              *rnr += (5 * re)>>4;
              *gnr += (5 * ge)>>4;
              *bnr += (5 * be)>>4;

              *(rnr-1) = (1 * re)>>4;
              *(gnr-1) = (1 * ge)>>4;
              *(bnr-1) = (1 * be)>>4;
            }
          else
            {
              *(++rpr) += (7 * re)>>4;
              *(++gpr) += (7 * ge)>>4;
              *(++bpr) += (7 * be)>>4;

              *rnr++ += (3 * re)>>4;
              *gnr++ += (3 * ge)>>4;
              *bnr++ += (3 * be)>>4;

              *rnr += (5 * re)>>4;
              *gnr += (5 * ge)>>4;
              *bnr += (5 * be)>>4;

              *(rnr+1) = (1 * re)>>4;
              *(gnr+1) = (1 * ge)>>4;
              *(bnr+1) = (1 * be)>>4;
            }

        next_pixel:

          dest += step_dest;
          src += step_src;
        }

      tmp = red_n_row;
      red_n_row = red_p_row;
      red_p_row = tmp;

      tmp = grn_n_row;
      grn_n_row = grn_p_row;
      grn_p_row = tmp;

      tmp = blu_n_row;
      blu_n_row = blu_p_row;
      blu_p_row = tmp;

      odd_row = !odd_row;

      gegl_buffer_set (new_buffer, GEGL_RECTANGLE (0, row, width, 1),
                       0, NULL, dest_buf,
                       GEGL_AUTO_ROWSTRIDE);

      if (quantobj->progress && (row % 16 == 0))
        gimp_progress_set_value (quantobj->progress,
                                 (gdouble) row / (gdouble) height);
    }

  g_free (red_n_row);
  g_free (red_p_row);
  g_free (grn_n_row);
  g_free (grn_p_row);
  g_free (blu_n_row);
  g_free (blu_p_row);
  g_free (src_buf);
  g_free (dest_buf);
}


static void
delete_median_cut (QuantizeObj *quantobj)
{
  g_free (quantobj->histogram);
  g_free (quantobj);
}


void
gimp_image_convert_indexed_set_dither_matrix (const guchar *matrix,
                                              gint          width,
                                              gint          height)
{
  gint x;
  gint y;

  /* if matrix is invalid, restore the default matrix */
  if (matrix == NULL || width == 0 || height == 0)
    {
      matrix = (const guchar *) DM_ORIGINAL;
      width  = DM_WIDTH;
      height = DM_HEIGHT;
    }

  g_return_if_fail ((DM_WIDTH % width) == 0);
  g_return_if_fail ((DM_HEIGHT % height) == 0);

  for (y = 0; y < DM_HEIGHT; y++)
    {
      for (x = 0; x < DM_WIDTH; x++)
        {
          DM[x][y] = matrix[((x % width) * height) + (y % height)];
        }
    }
}


/**************************************************************/
static QuantizeObj *
initialize_median_cut (GimpImageBaseType       type,
                       gint                    num_colors,
                       GimpConvertDitherType   dither_type,
                       GimpConvertPaletteType  palette_type,
                       GimpPalette            *custom_palette,
                       gboolean                want_dither_alpha,
                       GimpProgress           *progress)
{
  QuantizeObj *quantobj;

  /* Initialize the data structures */
  quantobj = g_new (QuantizeObj, 1);

  if (type == GIMP_GRAY && palette_type == GIMP_CONVERT_PALETTE_GENERATE)
    quantobj->histogram = g_new (ColorFreq, 256);
  else
    quantobj->histogram = g_new (ColorFreq,
                                 HIST_R_ELEMS * HIST_G_ELEMS * HIST_B_ELEMS);

  quantobj->custom_palette           = custom_palette;
  quantobj->desired_number_of_colors = num_colors;
  quantobj->want_dither_alpha        = want_dither_alpha;
  quantobj->progress                 = progress;

  switch (type)
    {
    case GIMP_GRAY:
      switch (palette_type)
        {
        case GIMP_CONVERT_PALETTE_GENERATE:
          quantobj->first_pass = median_cut_pass1_gray;
          break;
        case GIMP_CONVERT_PALETTE_WEB:
          quantobj->first_pass = webpal_pass1;
          break;
        case GIMP_CONVERT_PALETTE_CUSTOM:
          quantobj->first_pass = custompal_pass1;
          needs_quantize = TRUE;
          break;
        case GIMP_CONVERT_PALETTE_MONO:
        default:
          quantobj->first_pass = monopal_pass1;
        }

      if (palette_type == GIMP_CONVERT_PALETTE_WEB ||
          palette_type == GIMP_CONVERT_PALETTE_CUSTOM)
        {
          switch (dither_type)
            {
            case GIMP_CONVERT_DITHER_NODESTRUCT:
            default:
              g_warning("Uh-oh, bad dither type, W1");
              /* FALLTHROUGH */
            case GIMP_CONVERT_DITHER_NONE:
              quantobj->second_pass_init = median_cut_pass2_rgb_init;
              quantobj->second_pass = median_cut_pass2_no_dither_rgb;
              break;
            case GIMP_CONVERT_DITHER_FS:
              quantobj->error_freedom = 0;
              quantobj->second_pass_init = median_cut_pass2_rgb_init;
              quantobj->second_pass = median_cut_pass2_fs_dither_rgb;
              break;
            case GIMP_CONVERT_DITHER_FS_LOWBLEED:
              quantobj->error_freedom = 1;
              quantobj->second_pass_init = median_cut_pass2_rgb_init;
              quantobj->second_pass = median_cut_pass2_fs_dither_rgb;
              break;
            case GIMP_CONVERT_DITHER_FIXED:
              quantobj->second_pass_init = median_cut_pass2_rgb_init;
              quantobj->second_pass = median_cut_pass2_fixed_dither_rgb;
              break;
            }
        }
      else
        {
          switch (dither_type)
            {
            case GIMP_CONVERT_DITHER_NODESTRUCT:
            default:
              g_warning("Uh-oh, bad dither type, W2");
              /* FALLTHROUGH */
            case GIMP_CONVERT_DITHER_NONE:
              quantobj->second_pass_init = median_cut_pass2_gray_init;
              quantobj->second_pass = median_cut_pass2_no_dither_gray;
              break;
            case GIMP_CONVERT_DITHER_FS:
              quantobj->error_freedom = 0;
              quantobj->second_pass_init = median_cut_pass2_gray_init;
              quantobj->second_pass = median_cut_pass2_fs_dither_gray;
              break;
            case GIMP_CONVERT_DITHER_FS_LOWBLEED:
              quantobj->error_freedom = 1;
              quantobj->second_pass_init = median_cut_pass2_gray_init;
              quantobj->second_pass = median_cut_pass2_fs_dither_gray;
              break;
            case GIMP_CONVERT_DITHER_FIXED:
              quantobj->second_pass_init = median_cut_pass2_gray_init;
              quantobj->second_pass = median_cut_pass2_fixed_dither_gray;
              break;
            }
        }
      break;

    case GIMP_RGB:
      switch (palette_type)
        {
        case GIMP_CONVERT_PALETTE_GENERATE:
          quantobj->first_pass = median_cut_pass1_rgb;
          break;
        case GIMP_CONVERT_PALETTE_WEB:
          quantobj->first_pass = webpal_pass1;
          needs_quantize = TRUE;
          break;
        case GIMP_CONVERT_PALETTE_CUSTOM:
          quantobj->first_pass = custompal_pass1;
          needs_quantize = TRUE;
          break;
        case GIMP_CONVERT_PALETTE_MONO:
        default:
          quantobj->first_pass = monopal_pass1;
        }

      switch (dither_type)
        {
        case GIMP_CONVERT_DITHER_NONE:
          quantobj->second_pass_init = median_cut_pass2_rgb_init;
          quantobj->second_pass = median_cut_pass2_no_dither_rgb;
          break;
        case GIMP_CONVERT_DITHER_FS:
          quantobj->error_freedom = 0;
          quantobj->second_pass_init = median_cut_pass2_rgb_init;
          quantobj->second_pass = median_cut_pass2_fs_dither_rgb;
          break;
        case GIMP_CONVERT_DITHER_FS_LOWBLEED:
          quantobj->error_freedom = 1;
          quantobj->second_pass_init = median_cut_pass2_rgb_init;
          quantobj->second_pass = median_cut_pass2_fs_dither_rgb;
          break;
        case GIMP_CONVERT_DITHER_NODESTRUCT:
          quantobj->second_pass_init = NULL;
          quantobj->second_pass = median_cut_pass2_nodestruct_dither_rgb;
          break;
        case GIMP_CONVERT_DITHER_FIXED:
          quantobj->second_pass_init = median_cut_pass2_rgb_init;
          quantobj->second_pass = median_cut_pass2_fixed_dither_rgb;
          break;
        }
      break;

    default:
      break;
    }

  quantobj->delete_func = delete_median_cut;

  return quantobj;
}
