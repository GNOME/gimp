/* refract.h, 1/2/98
 * refract: a plug-in for the GIMP 0.99
 * By Kevin Turner <kevint@poboxes.com>
 * http://www.poboxes.com/kevint/gimp/refract.html
 */

#ifndef REFRACT_DEBUG
#warning "REFRACT_DEBUG flag is Off."
#define REFRACT_TITLE "Refract 1/2/98-Beta"
#else
#warning "REFRACT_DEBUG flag is On."
#define REFRACT_TITLE "Refract 1/2/98 (debug)"
#endif

/* Update the progress bar every this-many rows...  */
#ifndef PROGRESS_ROWS
#define PROGRESS_ROWS 8
#endif

/* Realistically, this number should be 1.0.  An index of refraction
   of less than 1 means the speed of light in that substance is
   *faster* than in a vacuum!  But hey, it's GIMP, when was the last
   time we payed any attention to reality?  Go ahead...  Add
   "subspace" to the list of materials... */
#ifndef INDEX_SCALE_MIN
#define INDEX_SCALE_MIN 0.0
#endif

/* This can be whatever is convient.  However, I don't know of any
   substances (even artifically generated ones) that have an index of
   refraction higher than 4.7 or so...*/
   
#ifndef INDEX_SCALE_MAX
#define INDEX_SCALE_MAX 5.0
#endif

/* For now, our height maps only have one byte per pixel, so guchar
   should be sufficient.  May need to change in future versions of
   GIMP when it supports greater pixel depth.  */
#ifndef HEIGHT_TYPE
#define HEIGHT_TYPE guchar
#endif

/* Should we rely more on macros or functions? */
/* #define OLD_SLOPE_MACROS */

#include "gtk/gtk.h"

typedef struct {
    gint32 lensmap; /* lens map id */
    gint32 thick; /* lens thickness */
    gint32 dist;  /* distance */
    gdouble na;   /* index a */
    gdouble nb;   /* index b */
    gint edge;    /* wrap/transparent */
    gint newl;    /* new layer? */
    gint xofs;    /* offset x */
    gint yofs;    /* offset y */
} RefractValues;

/* for refractvals.edge */
/* If a point is outside the selection, then */
#define BACKGROUND 0 /* use background color (or leave transparent, if alpha) */
#define OUTSIDE 1 /* look outside the selection for the point.  If the point is
		     beyond the edge of the layer, use background or alpha.
		     Only makes sense if the drawable is a selection of only part
		     of the layer. */
#define WRAP 2 /* like OUTSIDE, but if the point is over the edge of the layer,
		  get the point by wrapping around.  Probably most useful on
		  images which are tileable. */

/* TO DO: provide a "smear" option?  Would take whatever pixel was on
   the edge of the selection or layer (depending if IN_ONLY or
   OUTSIDE) where we went over.  Then BACKGROUND/SMEAR would be a
   choice independant of IN_ONLY/OUTSIDE/WRAP.  BACKGROUND/SMEAR would
   be ignored in the case of WRAP. */

/* One can also imagine a WRAP_WITHIN_SELECTION option, but I don't
   think I would use it too often.  Would you?  Well, if you're
   enthusiastic enough, either write a patch to implement it or
   convince me to.  For now, take the selection and float it, make it
   a new layer, merge it back when you're done, or whatever. */
