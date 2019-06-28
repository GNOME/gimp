/**************************************************
 * file: nlfilt/nlfilt.c
 *
 * Copyright (c) 1997 Eric L. Hernes (erich@rrnet.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Algorithm fixes, V2.0 compatibility by David Hodson  hodsond@ozemail.com.au
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-nlfilt"
#define PLUG_IN_BINARY "nl-filter"
#define PLUG_IN_ROLE   "gimp-nl-filter"


typedef struct
{
  gdouble  alpha;
  gdouble  radius;
  gint     filter;
} NLFilterValues;

typedef enum
{
  filter_alpha_trim,
  filter_opt_est,
  filter_edge_enhance
} FilterType;

static NLFilterValues nlfvals =
{
  0.3,
  0.3,
  0
};

/* function protos */

static void     query            (void);
static void     run              (const gchar      *name,
                                  gint              nparam,
                                  const GimpParam  *param,
                                  gint             *nretvals,
                                  GimpParam       **retvals);

static void     nlfilter         (gint32            drawable_id,
                                  GimpPreview      *preview);
static void     nlfilter_preview (gpointer          drawable_id,
                                  GimpPreview      *preview);

static gboolean nlfilter_dialog  (gint32            drawable_id);

static gint     nlfiltInit       (gdouble           alpha,
                                  gdouble           radius,
                                  FilterType        filter);

static void     nlfiltRow        (guchar           *srclast,
                                  guchar           *srcthis,
                                  guchar           *srcnext,
                                  guchar           *dst,
                                  gint              width,
                                  gint              bpp,
                                  gint              filtno);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "img",      "The Image to Filter" },
    { GIMP_PDB_DRAWABLE, "drw",      "The Drawable" },
    { GIMP_PDB_FLOAT,    "alpha",    "The amount of the filter to apply" },
    { GIMP_PDB_FLOAT,    "radius",   "The filter radius" },
    { GIMP_PDB_INT32,    "filter",   "The Filter to Run, "
                                     "0 - alpha trimmed mean; "
                                     "1 - optimal estimation (alpha controls noise variance); "
                                     "2 - edge enhancement" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Nonlinear swiss army knife filter"),
                          "This is the pnmnlfilt, in gimp's clothing.  "
                          "See the pnmnlfilt manpage for details.",
                          "Graeme W. Gill, gimp 0.99 plug-in by Eric L. Hernes",
                          "Graeme W. Gill, Eric L. Hernes",
                          "1997",
                          N_("_NL Filter..."),
                          "RGB,GRAY",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Enhance");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  gint32             drawable_id;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode    = param[0].data.d_int32;
  drawable_id = param[2].data.d_drawable;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &nlfvals);

      if (! nlfilter_dialog (drawable_id))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 6)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          nlfvals.alpha  = param[3].data.d_float;
          nlfvals.radius = param[4].data.d_float;
          nlfvals.filter = param[5].data.d_int32;
        }

      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &nlfvals);
      break;

    default:
      break;
  }

  if (status == GIMP_PDB_SUCCESS)
    {
      nlfilter (drawable_id, NULL);

      /* Store data */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &nlfvals, sizeof (NLFilterValues));
    }

  values[0].data.d_status = status;
}

/* pnmnlfilt.c - 4 in 1 (2 non-linear) filter
**             - smooth an anyimage
**             - do alpha trimmed mean filtering on an anyimage
**             - do optimal estimation smoothing on an anyimage
**             - do edge enhancement on an anyimage
**
** Version 1.0
**
** The implementation of an alpha-trimmed mean filter
** is based on the description in IEEE CG&A May 1990
** Page 23 by Mark E. Lee and Richard A. Redner.
**
** The paper recommends using a hexagon sampling region around each
** pixel being processed, allowing an effective sub pixel radius to be
** specified. The hexagon values are sythesised by area sampling the
** rectangular pixels with a hexagon grid. The seven hexagon values
** obtained from the 3x3 pixel grid are used to compute the alpha
** trimmed mean. Note that an alpha value of 0.0 gives a conventional
** mean filter (where the radius controls the contribution of
** surrounding pixels), while a value of 0.5 gives a median filter.
** Although there are only seven values to trim from before finding
** the mean, the algorithm has been extended from that described in
** CG&A by using interpolation, to allow a continuous selection of
** alpha value between and including 0.0 to 0.5  The useful values
** for radius are between 0.3333333 (where the filter will have no
** effect because only one pixel is sampled), to 1.0, where all
** pixels in the 3x3 grid are sampled.
**
** The optimal estimation filter is taken from an article "Converting Dithered
** Images Back to Gray Scale" by Allen Stenger, Dr Dobb's Journal, November
** 1992, and this article references "Digital Image Enhancement andNoise Filtering by
** Use of Local Statistics", Jong-Sen Lee, IEEE Transactions on Pattern Analysis and
** Machine Intelligence, March 1980.
**
** Also borrow the  technique used in pgmenhance(1) to allow edge
** enhancement if the alpha value is negative.
**
** Author:
**         Graeme W. Gill, 30th Jan 1993
**         graeme@labtam.oz.au
**
** Permission is hereby granted, to use, copy, modify, distribute,
** and sell this software and its associated documentation files
** (the "Software") for any purpose without fee, provided
** that:
**
**     1) The above copyright notices and this permission notice
**        accompany all source code copies of the Software and
**        related documentation.
** and
**
**     2) If executable code based on the Software only is distributed,
**        then the accompanying documentation must acknowledge that
**        "this software is based in part on the work of Graeme W. Gill".
** and
**
**     3) It is accepted that Graeme W. Gill (the "Author") accepts
**        NO LIABILITY for damages of any kind.  The Software is
**        provided without fee by the Author "AS-IS" and without
**        warranty of any kind, express, implied or otherwise,
**        including without limitation, any warranty of merchantability
**        or fitness for a particular purpose.
** and
**
**     4) These conditions apply to any software derived from or based
**        on the Software, not just to the unmodified library.
**
*/

/* ************************************************** */
/* Hexagon intersecting square area functions */
/* Compute the area of the intersection of a triangle */
/* and a rectangle */

static gdouble triang_area(gdouble, gdouble, gdouble, gdouble, gdouble,
                           gdouble, gdouble, gdouble, gint);
static gdouble rectang_area(gdouble, gdouble, gdouble, gdouble,
                            gdouble, gdouble, gdouble, gdouble);
static gdouble hex_area(gdouble, gdouble, gdouble, gdouble, gdouble);

static gint atfilt0 (gint *p);
static gint atfilt1 (gint *p);
static gint atfilt2 (gint *p);
static gint atfilt3 (gint *p);
static gint atfilt4 (gint *p);
static gint atfilt5 (gint *p);

gint (*atfuncs[6])(gint *) =
{
  atfilt0,
  atfilt1,
  atfilt2,
  atfilt3,
  atfilt4,
  atfilt5
};

static gint noisevariance;

#define MXIVAL 255                /* maximum input value */
#define NOIVAL (MXIVAL + 1)       /* number of possible input values */

#define SCALEB 8                  /* scale bits */
#define SCALE (1 << SCALEB)       /* scale factor */

#define CSCALEB 2                 /* coarse scale bits */
#define CSCALE (1 << CSCALEB)     /* coarse scale factor */
#define MXCSVAL (MXIVAL * CSCALE) /* maximum coarse scaled values */
#define NOCSVAL (MXCSVAL + 1)     /* number of coarse scaled values */
#define SCTOCSC(x) ((x) >> (SCALEB - CSCALEB))  /* convert from scaled to coarse scaled */
#define CSCTOSC(x) ((x) << (SCALEB - CSCALEB))  /* convert from course scaled to scaled */

/* round and scale floating point to scaled integer */
#define SROUND(x) ((gint)(((x) * (gdouble)SCALE) + 0.5))
/* round and un-scale scaled integer value */
#define RUNSCALE(x) (((x) + (1 << (SCALEB-1))) >> SCALEB)       /* rounded un-scale */
#define UNSCALE(x) ((x) >> SCALEB)

/* Note: modified by David Hodson, nlfiltRow now accesses
 * srclast, srcthis, and srcnext from [-bpp] to [width*bpp-1].
 * Beware if you use this code anywhere else!
 */
static void
nlfiltRow (guchar *srclast, guchar *srcthis, guchar *srcnext, guchar *dst,
           gint width, gint bpp, gint filtno)
{
  gint    pf[9];
  guchar *ip0, *ip1, *ip2, *or, *orend;

  orend = dst + width * bpp;
  ip0 = srclast;
  ip1 = srcthis;
  ip2 = srcnext;

  for (or = dst; or < orend; ip0++, ip1++, ip2++, or++)
    {
      pf[0] = *ip1;
      pf[1] = *(ip1 - bpp);
      pf[2] = *(ip2 - bpp);
      pf[3] = *(ip2);
      pf[4] = *(ip2 + bpp);
      pf[5] = *(ip1 + bpp);
      pf[6] = *(ip0 + bpp);
      pf[7] = *(ip0);
      pf[8] = *(ip0 - bpp);
      *or=(atfuncs[filtno])(pf);
    }
}

/* We restrict radius to the values: 0.333333 <= radius <= 1.0 */
/* so that no fewer and no more than a 3x3 grid of pixels around */
/* the pixel in question needs to be read. Given this, we only */
/* need 3 or 4 weightings per hexagon, as follows: */
/*                  _ _                         */
/* Virtical hex:   |_|_|  1 2                   */
/*                 |X|_|  0 3                   */
/*                                       _      */
/*              _                      _|_|   1 */
/* Middle hex: |_| 1  Horizontal hex: |X|_| 0 2 */
/*             |X| 0                    |_|   3 */
/*             |_| 2                            */

/* all filters */
gint V0[NOIVAL],V1[NOIVAL],V2[NOIVAL],V3[NOIVAL];   /* vertical hex */
gint M0[NOIVAL],M1[NOIVAL],M2[NOIVAL];              /* middle hex */
gint H0[NOIVAL],H1[NOIVAL],H2[NOIVAL],H3[NOIVAL];   /* horizontal hex */

/* alpha trimmed and edge enhancement only */
gint ALFRAC[NOIVAL * 8];               /* fractional alpha divider table */

/* optimal estimation only */
gint AVEDIV[7 * NOCSVAL];              /* divide by 7 to give average value */
gint SQUARE[2 * NOCSVAL];              /* scaled square lookup table */

/* Table initialisation function - return alpha range */
static gint
nlfiltInit (gdouble alpha, gdouble radius, FilterType filter)
{
   gint alpharange;                 /* alpha range value 0 - 3 */
   gdouble meanscale;               /* scale for finding mean */
   gdouble mmeanscale;              /* scale for finding mean - midle hex */
   gdouble alphafraction;   /* fraction of next largest/smallest
                             *  to subtract from sum
                             */
   switch (filter)
     {
       case filter_alpha_trim:
         {
          gdouble noinmean;
          /* alpha only makes sense in range 0.0 - 0.5 */
          alpha /= 2.0;
              /* number of elements (out of a possible 7) used in the mean */
          noinmean = ((0.5 - alpha) * 12.0) + 1.0;
          mmeanscale = meanscale = 1.0/noinmean;
          if (alpha == 0.0) {                    /* mean filter */
             alpharange = 0;
             alphafraction = 0.0;            /* not used */
          } else if (alpha < (1.0/6.0)) {    /* mean of 5 to 7 middle values */
             alpharange = 1;
             alphafraction = (7.0 - noinmean)/2.0;
          } else if (alpha < (1.0/3.0)) {    /* mean of 3 to 5 middle values */
             alpharange = 2;
             alphafraction = (5.0 - noinmean)/2.0;
          } else {                           /* mean of 1 to 3 middle values */
                                             /* alpha==0.5  => median filter */
             alpharange = 3;
             alphafraction = (3.0 - noinmean)/2.0;
          }
       }
       break;
       case filter_opt_est: {
          gint i;
          gdouble noinmean = 7.0;

              /* edge enhancement function */
          alpharange = 5;

              /* compute scaled hex values */
          mmeanscale=meanscale=1.0;

              /* Set up 1:1 division lookup - not used */
          alphafraction=1.0/noinmean;

              /* estimate of noise variance */
          noisevariance = alpha * (gdouble)255;
          noisevariance = noisevariance * noisevariance / 8.0;

              /* set yp optimal estimation specific stuff */

          for (i=0;i<(7*NOCSVAL);i++) { /* divide scaled value by 7 lookup */
             AVEDIV[i] = CSCTOSC(i)/7;       /* scaled divide by 7 */
          }
              /* compute square and rescale by
               * (val >> (2 * SCALEB + 2)) table
               */
          for (i=0;i<(2*NOCSVAL);i++) {
             gint val;
                 /* NOCSVAL offset to cope with -ve input values */
             val = CSCTOSC(i - NOCSVAL);
             SQUARE[i] = (val * val) >> (2 * SCALEB + 2);
          }
       }
       break;
       case filter_edge_enhance: {
          if (alpha == 1.0) alpha = 0.99;
          alpharange = 4;
              /* mean of 7 and scaled by -alpha/(1-alpha) */
          meanscale = 1.0 * (-alpha/((1.0 - alpha) * 7.0));

              /* middle pixel has 1/(1-alpha) as well */
          mmeanscale = 1.0 * (1.0/(1.0 - alpha) + meanscale);
          alphafraction = 0.0;    /* not used */
       }
       break;
       default:
         g_printerr ("unknown filter %d\n", filter);
         return -1;
   }
       /*
        * Setup pixel weighting tables -
        * note we pre-compute mean division here too.
        */
   {
      gint i;
      gdouble hexhoff,hexvoff;
      gdouble tabscale,mtabscale;
      gdouble v0,v1,v2,v3,m0,m1,m2,h0,h1,h2,h3;

          /* horizontal offset of virtical hex centers */
      hexhoff = radius/2;

          /* vertical offset of virtical hex centers */
      hexvoff = 3.0 * radius/sqrt(12.0);

          /*
           * scale tables to normalise by hexagon
           * area, and number of hexes used in mean
           */
      tabscale = meanscale / (radius * hexvoff);
      mtabscale = mmeanscale / (radius * hexvoff);
      v0 = hex_area(0.0,0.0,hexhoff,hexvoff,radius) * tabscale;
      v1 = hex_area(0.0,1.0,hexhoff,hexvoff,radius) * tabscale;
      v2 = hex_area(1.0,1.0,hexhoff,hexvoff,radius) * tabscale;
      v3 = hex_area(1.0,0.0,hexhoff,hexvoff,radius) * tabscale;
      m0 = hex_area(0.0,0.0,0.0,0.0,radius) * mtabscale;
      m1 = hex_area(0.0,1.0,0.0,0.0,radius) * mtabscale;
      m2 = hex_area(0.0,-1.0,0.0,0.0,radius) * mtabscale;
      h0 = hex_area(0.0,0.0,radius,0.0,radius) * tabscale;
      h1 = hex_area(1.0,1.0,radius,0.0,radius) * tabscale;
      h2 = hex_area(1.0,0.0,radius,0.0,radius) * tabscale;
      h3 = hex_area(1.0,-1.0,radius,0.0,radius) * tabscale;

      for (i=0; i <= MXIVAL; i++) {
         gdouble fi;
         fi = (gdouble)i;
         V0[i] = SROUND(fi * v0);
         V1[i] = SROUND(fi * v1);
         V2[i] = SROUND(fi * v2);
         V3[i] = SROUND(fi * v3);
         M0[i] = SROUND(fi * m0);
         M1[i] = SROUND(fi * m1);
         M2[i] = SROUND(fi * m2);
         H0[i] = SROUND(fi * h0);
         H1[i] = SROUND(fi * h1);
         H2[i] = SROUND(fi * h2);
         H3[i] = SROUND(fi * h3);
      }
          /* set up alpha fraction lookup table used on big/small */
      for (i=0; i < (NOIVAL * 8); i++) {
         ALFRAC[i] = SROUND((gdouble)i * alphafraction);
      }
   }
   return alpharange;
}

/* Core pixel processing function - hand it 3x3 pixels and return result. */
/* Mean filter */
static gint
atfilt0(gint32 *p)
{
   gint retv;
       /* map to scaled hexagon values */
   retv = M0[p[0]] + M1[p[3]] + M2[p[7]];
   retv += H0[p[0]] + H1[p[2]] + H2[p[1]] + H3[p[8]];
   retv += V0[p[0]] + V1[p[3]] + V2[p[2]] + V3[p[1]];
   retv += V0[p[0]] + V1[p[3]] + V2[p[4]] + V3[p[5]];
   retv += H0[p[0]] + H1[p[4]] + H2[p[5]] + H3[p[6]];
   retv += V0[p[0]] + V1[p[7]] + V2[p[6]] + V3[p[5]];
   retv += V0[p[0]] + V1[p[7]] + V2[p[8]] + V3[p[1]];
   return UNSCALE(retv);
}

/* Mean of 5 - 7 middle values */
static gint
atfilt1 (gint32 *p)
{
   gint h0,h1,h2,h3,h4,h5,h6;       /* hexagon values    2 3   */
                                    /*                  1 0 4  */
                                    /*                   6 5   */
   gint big,small;
       /* map to scaled hexagon values */
   h0 = M0[p[0]] + M1[p[3]] + M2[p[7]];
   h1 = H0[p[0]] + H1[p[2]] + H2[p[1]] + H3[p[8]];
   h2 = V0[p[0]] + V1[p[3]] + V2[p[2]] + V3[p[1]];
   h3 = V0[p[0]] + V1[p[3]] + V2[p[4]] + V3[p[5]];
   h4 = H0[p[0]] + H1[p[4]] + H2[p[5]] + H3[p[6]];
   h5 = V0[p[0]] + V1[p[7]] + V2[p[6]] + V3[p[5]];
   h6 = V0[p[0]] + V1[p[7]] + V2[p[8]] + V3[p[1]];
       /* sum values and also discover the largest and smallest */
   big = small = h0;
#define CHECK(xx) \
        h0 += xx; \
        if (xx > big) \
                big = xx; \
        else if (xx < small) \
                small = xx;
        CHECK(h1)
        CHECK(h2)
        CHECK(h3)
        CHECK(h4)
        CHECK(h5)
        CHECK(h6)
#undef CHECK
       /* Compute mean of middle 5-7 values */
   return UNSCALE(h0 -ALFRAC[(big + small)>>SCALEB]);
}

/* Mean of 3 - 5 middle values */
static gint
atfilt2 (gint32 *p)
{
   gint h0,h1,h2,h3,h4,h5,h6;       /* hexagon values    2 3   */
                                    /*                  1 0 4  */
                                    /*                   6 5   */
   gint big0,big1,small0,small1;
       /* map to scaled hexagon values */
   h0 = M0[p[0]] + M1[p[3]] + M2[p[7]];
   h1 = H0[p[0]] + H1[p[2]] + H2[p[1]] + H3[p[8]];
   h2 = V0[p[0]] + V1[p[3]] + V2[p[2]] + V3[p[1]];
   h3 = V0[p[0]] + V1[p[3]] + V2[p[4]] + V3[p[5]];
   h4 = H0[p[0]] + H1[p[4]] + H2[p[5]] + H3[p[6]];
   h5 = V0[p[0]] + V1[p[7]] + V2[p[6]] + V3[p[5]];
   h6 = V0[p[0]] + V1[p[7]] + V2[p[8]] + V3[p[1]];
       /* sum values and also discover the 2 largest and 2 smallest */
   big0 = small0 = h0;
   small1 = G_MAXINT;
   big1 = 0;
#define CHECK(xx) \
        h0 += xx; \
        if (xx > big1) \
                { \
                if (xx > big0) \
                        { \
                        big1 = big0; \
                        big0 = xx; \
                        } \
                else \
                        big1 = xx; \
                } \
        if (xx < small1) \
                { \
                if (xx < small0) \
                        { \
                        small1 = small0; \
                        small0 = xx; \
                        } \
                else \
                        small1 = xx; \
                }
        CHECK(h1)
        CHECK(h2)
        CHECK(h3)
        CHECK(h4)
        CHECK(h5)
        CHECK(h6)
#undef CHECK
       /* Compute mean of middle 3-5 values */
  return UNSCALE(h0 -big0 -small0 -ALFRAC[(big1 + small1)>>SCALEB]);
}

/*
 * Mean of 1 - 3 middle values.
 * If only 1 value, then this is a median filter.
 */
static gint32
atfilt3(gint32 *p)
{
   gint h0,h1,h2,h3,h4,h5,h6;       /* hexagon values    2 3   */
                                   /*                  1 0 4  */
                                   /*                   6 5   */
   gint big0,big1,big2,small0,small1,small2;
       /* map to scaled hexagon values */
   h0 = M0[p[0]] + M1[p[3]] + M2[p[7]];
   h1 = H0[p[0]] + H1[p[2]] + H2[p[1]] + H3[p[8]];
   h2 = V0[p[0]] + V1[p[3]] + V2[p[2]] + V3[p[1]];
   h3 = V0[p[0]] + V1[p[3]] + V2[p[4]] + V3[p[5]];
   h4 = H0[p[0]] + H1[p[4]] + H2[p[5]] + H3[p[6]];
   h5 = V0[p[0]] + V1[p[7]] + V2[p[6]] + V3[p[5]];
   h6 = V0[p[0]] + V1[p[7]] + V2[p[8]] + V3[p[1]];
       /* sum values and also discover the 3 largest and 3 smallest */
   big0 = small0 = h0;
   small1 = small2 = G_MAXINT;
   big1 = big2 = 0;
#define CHECK(xx) \
        h0 += xx; \
        if (xx > big2) \
                { \
                if (xx > big1) \
                        { \
                        if (xx > big0) \
                                { \
                                big2 = big1; \
                                big1 = big0; \
                                big0 = xx; \
                                } \
                        else \
                                { \
                                big2 = big1; \
                                big1 = xx; \
                                } \
                        } \
                else \
                        big2 = xx; \
                } \
        if (xx < small2) \
                { \
                if (xx < small1) \
                        { \
                        if (xx < small0) \
                                { \
                                small2 = small1; \
                                small1 = small0; \
                                small0 = xx; \
                                } \
                        else \
                                { \
                                small2 = small1; \
                                small1 = xx; \
                                } \
                        } \
                else \
                        small2 = xx; \
                }
        CHECK(h1)
        CHECK(h2)
        CHECK(h3)
        CHECK(h4)
        CHECK(h5)
        CHECK(h6)
#undef CHECK
       /* Compute mean of middle 1-3 values */
   return  UNSCALE(h0-big0-big1-small0-small1-ALFRAC[(big2+small2)>>SCALEB]);
}

/* Edge enhancement */
static gint
atfilt4 (gint *p)
{
   gint hav;
       /* map to scaled hexagon values and compute enhance value */
   hav = M0[p[0]] + M1[p[3]] + M2[p[7]];
   hav += H0[p[0]] + H1[p[2]] + H2[p[1]] + H3[p[8]];
   hav += V0[p[0]] + V1[p[3]] + V2[p[2]] + V3[p[1]];
   hav += V0[p[0]] + V1[p[3]] + V2[p[4]] + V3[p[5]];
   hav += H0[p[0]] + H1[p[4]] + H2[p[5]] + H3[p[6]];
   hav += V0[p[0]] + V1[p[7]] + V2[p[6]] + V3[p[5]];
   hav += V0[p[0]] + V1[p[7]] + V2[p[8]] + V3[p[1]];
   if (hav < 0)
      hav = 0;
   hav = UNSCALE(hav);
   if (hav > (gdouble)255)
      hav = (gdouble)255;
   return hav;
}

/* Optimal estimation - do smoothing in inverse proportion */
/* to the local variance. */
/* notice we use the globals noisevariance */
gint
atfilt5(gint *p) {
   gint mean,variance,temp;
   gint h0,h1,h2,h3,h4,h5,h6;       /* hexagon values    2 3   */
                                   /*                  1 0 4  */
                                   /*                   6 5   */
       /* map to scaled hexagon values */
   h0 = M0[p[0]] + M1[p[3]] + M2[p[7]];
   h1 = H0[p[0]] + H1[p[2]] + H2[p[1]] + H3[p[8]];
   h2 = V0[p[0]] + V1[p[3]] + V2[p[2]] + V3[p[1]];
   h3 = V0[p[0]] + V1[p[3]] + V2[p[4]] + V3[p[5]];
   h4 = H0[p[0]] + H1[p[4]] + H2[p[5]] + H3[p[6]];
   h5 = V0[p[0]] + V1[p[7]] + V2[p[6]] + V3[p[5]];
   h6 = V0[p[0]] + V1[p[7]] + V2[p[8]] + V3[p[1]];
   mean = h0 + h1 + h2 + h3 + h4 + h5 + h6;
       /* compute scaled mean by dividing by 7 */
   mean = AVEDIV[SCTOCSC(mean)];

       /* compute scaled variance */
   temp = (h1 - mean); variance = SQUARE[NOCSVAL + SCTOCSC(temp)];

       /* and rescale to keep */
   temp = (h2 - mean); variance += SQUARE[NOCSVAL + SCTOCSC(temp)];

 /* within 32 bit limits */
   temp = (h3 - mean); variance += SQUARE[NOCSVAL + SCTOCSC(temp)];
   temp = (h4 - mean); variance += SQUARE[NOCSVAL + SCTOCSC(temp)];
   temp = (h5 - mean); variance += SQUARE[NOCSVAL + SCTOCSC(temp)];
   temp = (h6 - mean); variance += SQUARE[NOCSVAL + SCTOCSC(temp)];
       /* (temp = h0 - mean) */
   temp = (h0 - mean); variance += SQUARE[NOCSVAL + SCTOCSC(temp)];
   if (variance != 0)      /* avoid possible divide by 0 */
          /* optimal estimate */
      temp = mean + (variance * temp) / (variance + noisevariance);
   else temp = h0;
   if (temp < 0)
      temp = 0;
   temp = RUNSCALE(temp);
   if (temp > (gdouble)255) temp = (gdouble)255;
   return temp;
}


/* Triangle orientation is per geometric axes (not graphical axies) */

#define NW 0    /* North west triangle /| */
#define NE 1    /* North east triangle |\ */
#define SW 2    /* South west triangle \| */
#define SE 3    /* South east triangle |/ */
#define STH 2
#define EST 1

#define SWAPI(a,b) (t = a, a = -b, b = -t)

/* compute the area of overlap of a hexagon diameter d, */
/* centered at hx,hy, with a unit square of center sx,sy. */
static gdouble
hex_area (gdouble sx, gdouble sy, gdouble hx, gdouble hy, gdouble d)
{
   gdouble hx0,hx1,hx2,hy0,hy1,hy2,hy3;
   gdouble sx0,sx1,sy0,sy1;

       /* compute square co-ordinates */
   sx0 = sx - 0.5;
   sy0 = sy - 0.5;
   sx1 = sx + 0.5;
   sy1 = sy + 0.5;
       /* compute hexagon co-ordinates */
   hx0 = hx - d/2.0;
   hx1 = hx;
   hx2 = hx + d/2.0;
   hy0 = hy - 0.5773502692 * d;    /* d / sqrt(3) */
   hy1 = hy - 0.2886751346 * d;    /* d / sqrt(12) */
   hy2 = hy + 0.2886751346 * d;    /* d / sqrt(12) */
   hy3 = hy + 0.5773502692 * d;    /* d / sqrt(3) */

   return
      triang_area(sx0,sy0,sx1,sy1,hx0,hy2,hx1,hy3,NW) +
      triang_area(sx0,sy0,sx1,sy1,hx1,hy2,hx2,hy3,NE) +
      rectang_area(sx0,sy0,sx1,sy1,hx0,hy1,hx2,hy2) +
      triang_area(sx0,sy0,sx1,sy1,hx0,hy0,hx1,hy1,SW) +
      triang_area(sx0,sy0,sx1,sy1,hx1,hy0,hx2,hy1,SE);
}

static gdouble
triang_area (gdouble rx0, gdouble ry0, gdouble rx1, gdouble ry1, gdouble tx0,
             gdouble ty0, gdouble tx1, gdouble ty1, gint tt)
{
   gdouble a,b,c,d;
   gdouble lx0,ly0,lx1,ly1;
       /* Convert everything to a NW triangle */
   if (tt & STH) {
      gdouble t;
      SWAPI(ry0,ry1);
      SWAPI(ty0,ty1);
   } if (tt & EST) {
      gdouble t;
      SWAPI(rx0,rx1);
      SWAPI(tx0,tx1);
   }
       /* Compute overlapping box */
   if (tx0 > rx0)
      rx0 = tx0;
   if (ty0 > ry0)
      ry0 = ty0;
   if (tx1 < rx1)
      rx1 = tx1;
   if (ty1 < ry1)
      ry1 = ty1;
   if (rx1 <= rx0 || ry1 <= ry0)
      return 0.0;
       /* Need to compute diagonal line intersection with the box */
       /* First compute co-efficients to formulas x = a + by and y = c + dx */
   b = (tx1 - tx0)/(ty1 - ty0);
   a = tx0 - b * ty0;
   d = (ty1 - ty0)/(tx1 - tx0);
   c = ty0 - d * tx0;

       /* compute top or right intersection */
   tt = 0;
   ly1 = ry1;
   lx1 = a + b * ly1;
   if (lx1 <= rx0)
      return (rx1 - rx0) * (ry1 - ry0);
   else if (lx1 > rx1) {     /* could be right hand side */
      lx1 = rx1;
      ly1 = c + d * lx1;
      if (ly1 <= ry0)
         return (rx1 - rx0) * (ry1 - ry0);
      tt = 1; /* right hand side intersection */
   }
       /* compute left or bottom intersection */
   lx0 = rx0;
   ly0 = c + d * lx0;
   if (ly0 >= ry1)
      return (rx1 - rx0) * (ry1 - ry0);
   else if (ly0 < ry0) {    /* could be right hand side */
      ly0 = ry0;
      lx0 = a + b * ly0;
      if (lx0 >= rx1)
         return (rx1 - rx0) * (ry1 - ry0);
      tt |= 2;        /* bottom intersection */
   }

   if (tt == 0) {    /* top and left intersection */
                       /* rectangle minus triangle */
      return ((rx1 - rx0) * (ry1 - ry0))
         - (0.5 * (lx1 - rx0) * (ry1 - ly0));
   }
   else if (tt == 1) {       /* right and left intersection */
      return ((rx1 - rx0) * (ly0 - ry0))
         + (0.5 * (rx1 - rx0) * (ly1 - ly0));
   } else if (tt == 2) {      /* top and bottom intersection */
      return ((rx1 - lx1) * (ry1 - ry0))
         + (0.5 * (lx1 - lx0) * (ry1 - ry0));
   } else { /* tt == 3 */      /* right and bottom intersection */
          /* triangle */
      return (0.5 * (rx1 - lx0) * (ly1 - ry0));
   }
}

/* Compute rectangle area */
static gdouble
rectang_area (gdouble rx0, gdouble ry0, gdouble rx1, gdouble ry1, gdouble tx0,
              gdouble ty0, gdouble tx1, gdouble ty1)
{
  /* Compute overlapping box */
   if (tx0 > rx0)
      rx0 = tx0;
   if (ty0 > ry0)
      ry0 = ty0;
   if (tx1 < rx1)
      rx1 = tx1;
   if (ty1 < ry1)
      ry1 = ty1;
   if (rx1 <= rx0 || ry1 <= ry0)
      return 0.0;
   return (rx1 - rx0) * (ry1 - ry0);
}

static void
nlfilter (gint32        drawable_id,
          GimpPreview  *preview)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *format;
  guchar     *srcbuf, *dstbuf;
  guchar     *lastrow, *thisrow, *nextrow, *temprow;
  gint        x1, y1, y2;
  gint        width, height, bpp;
  gint        filtno, y, rowsize, exrowsize, p_update;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
      y2 = y1 + height;
    }
  else
    {
      if (! gimp_drawable_mask_intersect (drawable_id,
                                          &x1, &y1, &width, &height))
        return;

      y2 = y1 + height;
    }

  if (gimp_drawable_has_alpha (drawable_id))
    format = babl_format ("R'G'B'A u8");
  else
    format = babl_format ("R'G'B' u8");

  src_buffer = gimp_drawable_get_buffer (drawable_id);

  if (preview)
    dest_buffer = gegl_buffer_new (gegl_buffer_get_extent (src_buffer), format);
  else
    dest_buffer = gimp_drawable_get_shadow_buffer (drawable_id);

  bpp = babl_format_get_bytes_per_pixel (format);

  rowsize = width * bpp;
  exrowsize = (width + 2) * bpp;
  p_update = width / 20 + 1;

  /* source buffer gives one pixel margin all around destination buffer */
  srcbuf = g_new0 (guchar, exrowsize * 3);
  dstbuf = g_new0 (guchar, rowsize);

  /* pointers to second pixel in each source row */
  lastrow = srcbuf + bpp;
  thisrow = lastrow + exrowsize;
  nextrow = thisrow + exrowsize;

  filtno = nlfiltInit (nlfvals.alpha, nlfvals.radius, nlfvals.filter);

  if (!preview)
    gimp_progress_init (_("NL Filter"));

  /* first row */
  gegl_buffer_get (src_buffer, GEGL_RECTANGLE (x1, y1, width, 1), 1.0,
                   format, thisrow,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  /* copy thisrow[0] to thisrow[-1], thisrow[width-1] to thisrow[width] */
  memcpy (thisrow - bpp, thisrow, bpp);
  memcpy (thisrow + rowsize, thisrow + rowsize - bpp, bpp);
  /* copy whole thisrow to lastrow */
  memcpy (lastrow - bpp, thisrow - bpp, exrowsize);

  for (y = y1; y < y2 - 1; y++)
    {
      if (((y % p_update) == 0) && !preview)
        gimp_progress_update ((gdouble) y / (gdouble) height);

      gegl_buffer_get (src_buffer, GEGL_RECTANGLE (x1, y + 1, width, 1), 1.0,
                       format, nextrow,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      memcpy (nextrow - bpp, nextrow, bpp);
      memcpy (nextrow + rowsize, nextrow + rowsize - bpp, bpp);
      nlfiltRow (lastrow, thisrow, nextrow, dstbuf, width, bpp, filtno);

      gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (x1, y, width, 1), 0,
                       format, dstbuf,
                       GEGL_AUTO_ROWSTRIDE);

      /* rotate row buffers */
      temprow = lastrow; lastrow = thisrow;
      thisrow = nextrow; nextrow = temprow;
    }

  /* last row */
  memcpy (nextrow - bpp, thisrow - bpp, exrowsize);
  nlfiltRow (lastrow, thisrow, nextrow, dstbuf, width, bpp, filtno);

  gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (x1, y2 - 1, width, 1), 0,
                   format, dstbuf,
                   GEGL_AUTO_ROWSTRIDE);

  g_free (srcbuf);
  g_free (dstbuf);

  if (preview)
    {
      guchar *buf = g_new (guchar, width * height * bpp);

      gegl_buffer_get (dest_buffer, GEGL_RECTANGLE (x1, y1, width, height), 1.0,
                       format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      gimp_preview_draw_buffer (GIMP_PREVIEW (preview), buf, width * bpp);

      g_free (buf);
    }

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  if (! preview)
    {
      gimp_progress_update (1.0);

      gimp_drawable_merge_shadow (drawable_id, TRUE);
      gimp_drawable_update (drawable_id, x1, y1, width, height);
      gimp_displays_flush ();
    }
}

static void
nlfilter_preview (gpointer     drawable_id,
                  GimpPreview *preview)
{
  nlfilter (GPOINTER_TO_INT (drawable_id), preview);
}

static gboolean
nlfilter_dialog (gint32 drawable_id)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *frame;
  GtkWidget *alpha_trim;
  GtkWidget *opt_est;
  GtkWidget *edge_enhance;
  GtkWidget *table;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("NL Filter"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new_from_drawable_id (drawable_id);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (nlfilter_preview),
                            GINT_TO_POINTER (drawable_id));

  frame = gimp_int_radio_group_new (TRUE, _("Filter"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &nlfvals.filter, nlfvals.filter,

                                    _("_Alpha trimmed mean"),
                                    filter_alpha_trim, &alpha_trim,
                                    _("Op_timal estimation"),
                                    filter_opt_est, &opt_est,
                                    _("_Edge enhancement"),
                                    filter_edge_enhance, &edge_enhance,

                                    NULL);

  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_signal_connect_swapped (alpha_trim, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (opt_est, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (edge_enhance, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("A_lpha:"), 0, 0,
                              nlfvals.alpha, 0.0, 1.0, 0.05, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &nlfvals.alpha);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("_Radius:"), 0, 0,
                              nlfvals.radius, 1.0 / 3.0, 1.0, 0.05, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &nlfvals.radius);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
