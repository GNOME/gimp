/* refguts.c, 1/2/98 - this file contains the icky stuff.
 * refract: a plug-in for the GIMP 0.99
 * By Kevin Turner <kevint@poboxes.com>
 * http://www.poboxes.com/kevint/gimp/refract.html
 */

/*
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Refresher course in optics:
   Incident ray is the light ray hitting the surface.
   Angles are measured from the perpendicular to the surface.
   Angle of reflection is equal to angle of incidence.
   Angle of refraction is determined by

   Snell's law: index[a] * sin(a) = index[b] * sin(b)

   If second index is smaller than first, light is bent toward normal.
   Otherwise, away.
   */

#include "refract.h"
#include "libgimp/gimp.h"

typedef struct { /* Quartic's pixelfetcher thing */
	gint       col, row;
	gint       img_width, img_height, img_bpp, img_has_alpha;
	gint       tile_width, tile_height;
	guchar     bg_color[4];
	GDrawable *drawable;
	GTile     *tile;
} pixel_fetcher_t;

extern RefractValues refractvals;

void             go_refract(GDrawable *drawable, 
			    gint32 image_id);
static void      do_refract(GPixelRgn *dest_rgn,
			    GPixelRgn *lens_rgn, 
			    pixel_fetcher_t *pf);
static gint      delta (gdouble *offset, gdouble slope, gint height);
#ifndef OLD_SLOPE_MACROS
static gdouble   slope(gint h,           /* FIXME: I should probably be inlined. */
		       HEIGHT_TYPE p1, 
		       HEIGHT_TYPE p2, 
		       HEIGHT_TYPE p3, 
		       HEIGHT_TYPE p4);
#endif

/* More pixelfetcher things */
static pixel_fetcher_t *pixel_fetcher_new(GDrawable *drawable);
static void             pixel_fetcher_set_bg_color(pixel_fetcher_t *pf, guchar r, guchar g, guchar b, guchar a);
static void             pixel_fetcher_get_pixel(pixel_fetcher_t *pf, int x, int y, guchar *pixel);
static void             pixel_fetcher_destroy(pixel_fetcher_t *pf);

/* This bilinear interpolation function also borrowed. */
static guchar    bilinear_new    (double    x,
				  double    y,
				  guchar   v[4][4],
				  guint8      i);

static gint sel_x1=-1,sel_x2=-1,sel_y1=-1,sel_y2=-1; 
static gint sel_w, sel_h;
static guchar fg_color[4];

void
go_refract( GDrawable * drawable, gint32 image_id)
{
     /* For Initialize pixel fetcher. */
     pixel_fetcher_t *pf;
     guchar bg_color[4];

     /* For Initialize lens region. */
     GDrawable *lensmap;
     GPixelRgn lens_rgn;
     gint lxoff, lyoff;
#if 0
     gint use_x1,use_x2,use_y1,use_y2, use_w, use_h;
#endif

     /* For Initialize dest region. */
     GPixelRgn dest_rgn;
     GDrawable *output_drawable;
     gint32 new_layer_id;
     char buf[256];

     /*****************************/
     /* Initialize pixel fetcher. */

     gimp_drawable_mask_bounds (drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);
     sel_w=sel_x2-sel_x1; sel_h=sel_y2-sel_y1;
     pf = pixel_fetcher_new(drawable);
     gimp_palette_get_background(&bg_color[0], &bg_color[1], &bg_color[2]);
     pixel_fetcher_set_bg_color(pf,
				bg_color[0],
				bg_color[1],
				bg_color[2],
				0);

     /***************************/
     /* Initialize lens region: */

     lensmap = gimp_drawable_get (refractvals.lens_id);
     
     /* Fortunately, this isn't really run repeatedly, so it's OK if
        it's not all that compact, right? */

#ifdef __PSYCHIC_COMPILER__ /* If your compiler can read my mind better than I can. */

     /* Crap.  We need a smegging lens-wrap toggle. */

     if (lens wrap) {

     if (no lens offsets) {
	  /* no problems. */
	  lens_rgn = foo(sel_x1, sel_x2, sel_w, sel_h);
	  /* To conserve resources, we might want to change that
	     to take less if the lens stops before the image
	     does. */
     } else { /* offsets.  Uhoh. */
	  if (lens_size >= drawable_size) {  /* Do this for each dimension. */
	       /* Oh, it'll be okay.  We'll just add offset and take absmod. */
	       if (split) {
		    lens_rgn = wholething; /* Oh well, we'll take the whole thing. */
	       } else { /* not split */
		    lens_rgn = foo(sel_x1-xoff, sel_y1-yoff, sel_w, sel_h);
	       } /* endif not split */
	  } else { /* lens is smaller */
	       if (split) { g_warning("refract: refguts.c: Go to hell.\n"); }
	       else { lens_rgn= foo(sel_x1-xoff, sel_y1-yoff, sel_w, sel_h); }
	  } /* lens is smaller */
     } /* endif offsets */

     } else { /* no lens wrap */
	  /* If there's no lens wrapping, I have no problem.  It's yo
	     own dang fault if your lens doesn't land on your drawable
	     any more. */
	  
	  /* Let's rub it in... */
	  if (((xoff > 0) ? (xoff >= sel_w) : (-xoff <= lens_w)) || 
	      ((yoff > 0) ? (yoff >= sel_h) : (-yoff <= lens_h))) {
	       g_warning("refract: refguts.c: loose nut detected between chair and keyboard.\n");
	       g_error("refract: refguts.c: Offsets move lens off image.\n");
	  } else {
	       lens_rgn = foo(sel_x1-xoff, sel_y1-yoff, sel_w, sel_h);
	       /* To conserve resources, we might want to change that
		  to take less if the lens stops before the image
		  does. */
	  }
	  
     } /* endif no lens wrap */

#else /* Compiler requires actual code. */

     gimp_drawable_offsets(lensmap->id,&lxoff, &lyoff);

#ifdef REFRACT_DEBUG

     g_print("x: %d\ty: %d\tw: %d\th: %d\n",
	     lxoff, lyoff,
	     lensmap->width,lensmap->height);

#endif /* REFRACT_DEBUG */

#if 0 /* If we *didn't* wrap the lens... */
     if( (sel_x1 > (lxoff + lensmap->width)) || 
	 (lxoff  > sel_x2) ||
	 (sel_y1 > (lyoff + lensmap->height)) ||
	 (lyoff  > sel_y2)) {
	  g_error("refract:refguts.c:Selection and lens don't overlap.  You lose.\n");
     } else {
	  use_x1=MAX(sel_x1,lxoff);
	  use_x2=MIN(sel_x2,lxoff + lensmap->width);
	  use_y1=MAX(sel_y1,lyoff);
	  use_y2=MIN(sel_y2,lyoff + lensmap->height);
	  use_w=use_x2-use_x1;
	  use_h=use_y2-use_y1;
     }
#endif /* if we didn't wrap lenses */
     
     gimp_pixel_rgn_init (&lens_rgn, lensmap, 
			  lxoff, lyoff, lensmap->width, lensmap->height,
			  FALSE, FALSE);

#endif /* Compiler requires actual code. */

     /**********************************/
     /* Initialize destination region: */

     sprintf(buf,"Refracted %s",gimp_drawable_name(drawable->id));

     if (refractvals.newl) { /* New layer?  Yes... */
	  new_layer_id=gimp_layer_new(image_id, buf, sel_w, sel_h,
				      gimp_drawable_gray(drawable->id) 
				      ? GRAYA_IMAGE : RGBA_IMAGE,
				      100.0, NORMAL_MODE);
	  /* For layer position (currently 0), how would I say 
	     "one above current layer"? */
	  gimp_image_add_layer(image_id,new_layer_id,0);
	  gimp_layer_set_offsets(new_layer_id, sel_x1, sel_y1);
	  output_drawable=gimp_drawable_get(new_layer_id);
     } else {  /* New layer No. */
	  output_drawable=drawable;
     } /* New layer No. */

     gimp_pixel_rgn_init (&dest_rgn, output_drawable, 
			  sel_x1, sel_y1, sel_w, sel_h,
			  TRUE, TRUE);

#ifdef REFRACT_DEBUG
     g_print("drawable-id: %d\toutput-id: %d:\tlens_rgn-id: %d\n",
	    drawable->id,output_drawable->id,lens_rgn.drawable->id);
#endif

     /********/
     /* Misc */

     gimp_palette_get_foreground(&fg_color[0], &fg_color[1], &fg_color[2]);
     fg_color[3] = 255;

     /**********/
     /* Do it! */

     do_refract(&dest_rgn,&lens_rgn,pf);

     /*************/
     /* Clean up. */

     /* FIXME: If we are cancelled, make sure there's none of that
        unsightly new layer residue. */

     pixel_fetcher_destroy(pf);

     /* I hope this works... */
     
     gimp_drawable_flush(output_drawable);
     gimp_drawable_merge_shadow(output_drawable->id,!refractvals.newl);
     gimp_drawable_update(output_drawable->id,sel_x1,sel_y1,sel_w,sel_h);
     gimp_drawable_detach(drawable);
     if (drawable != output_drawable) gimp_drawable_detach(output_drawable);
     gimp_drawable_detach(lensmap);

     /* return (refractvals.newl ? new_layer_id : NULL); */

} /* go_refract */

#define ABSMOD(A,B) ( ((A) < 0) ? (((B) + (A)) % (B)) : ((A) % (B)) )

#define X(F) ( ABSMOD((F)+refractvals.xofs,lens_rgn->w) )
#define Y(F) ( ABSMOD((F)+refractvals.yofs,lens_rgn->h) )

#define ROWM2 (lm_rowm2[ X(x) * lens_rgn->bpp ])
#define ROWM1 (lm_rowm1[ X(x) * lens_rgn->bpp ])
#define ROW0(F)  (lm_row0[ X(x+(F)) * lens_rgn->bpp ])
#define ROWP1 (lm_rowp1[ X(x) * lens_rgn->bpp ])
#define ROWP2 (lm_rowp2[ X(x) * lens_rgn->bpp ])

#ifdef OLD_SLOPE_MACROS
#define SLOPE_X ((gdouble) 1.0 / (12 * h) * ( ROW0(-2*h) - 8 * ROW0(-1*h) + 8 * ROW0(1*h) - ROW0(2*h) ) * depths )
#define SLOPE_Y ((gdouble) 1.0 / (12 * h) * ( ROWM2    - 8 * ROWM1    + 8 * ROWP1   - ROWP2   ) * depths )
#else /* Macros calling the slope function. Functionally equivillant. */
#define SLOPE_X (slope(h,ROW0(-2*h), ROW0(-1*h), ROW0(1*h), ROW0(2*h)))
#define SLOPE_Y (slope(h,ROWM2,    ROWM1,    ROWP1,   ROWP2))
#endif /* OLD_SLOPE_MACROS */

static void
do_refract(GPixelRgn *dest_rgn, GPixelRgn *lens_rgn, 
	   pixel_fetcher_t *pf)
{
     HEIGHT_TYPE *lm_rowm2, *lm_rowm1, *lm_row0, *lm_rowp1, *lm_rowp2;
     HEIGHT_TYPE *lm_rowfoo;

     guchar pixel[4][4];
     guint8 i, j;
     gint8 diff_bpp;
     guchar *dest, *dest_row;
     gdouble dx, dy;
     gint x, y, xf, yf;

     gdouble depths=(gdouble) refractvals.thick/ (gdouble) 256.0; /* Depth scalar */
     const gint h=1; /* The delta value for the slope interpolation equation. */
     /* FIXME: Give option of changing h?  */

     /* See if dest_rgn and pf have different bpp */
     diff_bpp = dest_rgn->bpp - pf->img_bpp;
     
     /***************/
     /* Allocations */

     lm_rowm2 = g_malloc(lens_rgn->w * lens_rgn->bpp * sizeof(HEIGHT_TYPE));
     lm_rowm1 = g_malloc(lens_rgn->w * lens_rgn->bpp * sizeof(HEIGHT_TYPE));
     lm_row0  = g_malloc(lens_rgn->w * lens_rgn->bpp * sizeof(HEIGHT_TYPE));
     lm_rowp1 = g_malloc(lens_rgn->w * lens_rgn->bpp * sizeof(HEIGHT_TYPE));
     lm_rowp2 = g_malloc(lens_rgn->w * lens_rgn->bpp * sizeof(HEIGHT_TYPE));

     dest_row = g_malloc(dest_rgn->w * dest_rgn->bpp * sizeof(guchar));

     /************************/
     /* Grab some lens rows. */

     gimp_pixel_rgn_get_row(lens_rgn, lm_rowm2, 0, Y(sel_y1 - 2*h), lens_rgn->w);
     gimp_pixel_rgn_get_row(lens_rgn, lm_rowm1, 0, Y(sel_y1 - 1*h), lens_rgn->w);
     gimp_pixel_rgn_get_row(lens_rgn, lm_row0,  0, Y(sel_y1),       lens_rgn->w);
     gimp_pixel_rgn_get_row(lens_rgn, lm_rowp1, 0, Y(sel_y1 + 1*h), lens_rgn->w);
     gimp_pixel_rgn_get_row(lens_rgn, lm_rowp2, 0, Y(sel_y1 + 2*h), lens_rgn->w);

     /***********************/
     /* Let's begin work... */
     for (y=sel_y1; y < sel_y2; y++) {

	  gimp_pixel_rgn_get_row(dest_rgn, dest_row, sel_x1, y, sel_w);
	  dest = dest_row;
	  
	  for (x=sel_x1; x < sel_x2; x++) {
	       
	       /* If offsets in both X and Y direction exist... */
	       /* (meaning no internal refraction) */
	       if (delta(&dx, SLOPE_X, ROW0(0) * depths) &&
		   delta(&dy, SLOPE_Y, ROW0(0) * depths)) {
		    
		    switch (refractvals.edge) {
		    case WRAP:
			 xf = ABSMOD(x + (gint) dx, dest_rgn->drawable->width);
			 yf = ABSMOD(y + (gint) dy, dest_rgn->drawable->height);
			 break;
		    case BACKGROUND:
		    case OUTSIDE:
			 xf = x + (gint) dx;
			 yf = y + (gint) dy;
			 break;
		    default:
			 g_error("refract: refract.c: Unanticipated value for edge in do_refract\n");
		    } /* switch refractvals.edge */
		    
		    pixel_fetcher_get_pixel(pf, xf,     yf,     pixel[0]);
		    pixel_fetcher_get_pixel(pf, xf + 1, yf,     pixel[1]);
		    pixel_fetcher_get_pixel(pf, xf,     yf + 1, pixel[2]);
		    pixel_fetcher_get_pixel(pf, xf + 1, yf + 1, pixel[3]);
		    
		    for (i = 0; i < pf->img_bpp; i++) {
			 *dest++ = bilinear_new(dx, dy, pixel, i);
		    } /* next i */
		    /* If dest_rgn has more bpp than pf's source,
		       then fill in the rest with 255's...  This helps
		       when making a new layer from a non-alpha layer. */
		    for (j = 0; j < diff_bpp; j++) {
			 *dest++ = 255;
		    }
		    
	       } else { /* if a delta() call returns false. */
		    for (i = 0; i < dest_rgn->bpp; i++) {
			 *dest++ = fg_color[i];
		    } /* next i */
	       } /* endif delta() */
	  } /* next x */
	  
	  gimp_pixel_rgn_set_row(dest_rgn, dest_row, sel_x1, y, sel_w);

	  if (!(y % PROGRESS_ROWS))
	       gimp_progress_update((double) (y-sel_y1) / (double) (sel_y2-sel_y1));

	  /* move lensmap pointers */

	  lm_rowfoo=lm_rowm2; 
	  lm_rowm2=lm_rowm1; 
	  lm_rowm1=lm_row0; 
	  lm_row0= lm_rowp1;
	  lm_rowp1=lm_rowp2; 
	  lm_rowp2=lm_rowfoo;

	  /* get new lensmap row */

	  gimp_pixel_rgn_get_row(lens_rgn, lm_rowp2, 0, Y(y+3*h), lens_rgn->w);

     } /* next y */
} /* do_refract */

#ifndef OLD_SLOPE_MACROS
static gdouble /* FIXME: I should probably be inlined. */
slope(gint h, HEIGHT_TYPE p1, HEIGHT_TYPE p2, HEIGHT_TYPE p3, HEIGHT_TYPE p4)
{
     /* p1 = f(x-2h), p2 = f(x-1h), p3 = f(x+1h), p4=f(x+2h) */
     return 1.0 / (12 * h) * ( p1 - 8 * p2 + 8 * p3 - p4 );
}
#endif /* OLD_SLOPE_MACROS */

static gint 
delta(gdouble *offset, gdouble slope, gint height)
{
    gdouble alpha, beta;
    
    alpha = atan(slope);

    if( alpha > asin( refractvals.nb / refractvals.na )) {
	return FALSE; /* Total Internal Refraction.  Aiee! */
    }
    
    beta = asin(refractvals.na * sin(alpha)/refractvals.nb);
    *offset = -(refractvals.refr_dist + height) * tan(beta - alpha); 

    return TRUE;
}

/* A "borrowed" bilinear interpolation function, modified to select from a 
   two dimensional array instead of a linear one. */
static guchar
bilinear_new(double x, double y, guchar values[4][4], guint8 i)
{
	double m0, m1;

	x = fmod(x, 1.0);
	y = fmod(y, 1.0);

	if (x < 0.0)
		x += 1.0;

	if (y < 0.0)
		y += 1.0;

	m0 = (double) values[0][i] + x * ((double) values[1][i] - values[0][i]);
	m1 = (double) values[2][i] + x * ((double) values[3][i] - values[2][i]);

	return (guchar) (m0 + y * (m1 - m0));
} /* bilinear_new */

/************************************************************************
 *
 *   Fun pixel fetching stuff...  Quartic's code from whirlpinch.c
 *   Uses the globals sel_x1,sel_x2,sel_y1,sel_y2.
 */ 

static pixel_fetcher_t *
pixel_fetcher_new(GDrawable *drawable)
{
	pixel_fetcher_t *pf;

	pf = g_malloc(sizeof(pixel_fetcher_t));

	pf->col           = -1;
	pf->row           = -1;
	pf->img_width     = drawable->width;
	pf->img_height    = drawable->height;
	pf->img_bpp       = drawable->bpp;
	pf->img_has_alpha = gimp_drawable_has_alpha(drawable->id);
	pf->tile_width    = gimp_tile_width();
	pf->tile_height   = gimp_tile_height();
	pf->bg_color[0]   = 0;
	pf->bg_color[1]   = 0;
	pf->bg_color[2]   = 0;
	pf->bg_color[3]   = 0;

	pf->drawable    = drawable;
	pf->tile        = NULL;

	return pf;
} /* pixel_fetcher_new */


/*****/

static void
pixel_fetcher_set_bg_color(pixel_fetcher_t *pf, guchar r, guchar g, guchar b, guchar a)
{
	pf->bg_color[0] = r;
	pf->bg_color[1] = g;
	pf->bg_color[2] = b;

	if (pf->img_has_alpha)
		pf->bg_color[pf->img_bpp - 1] = a;
} /* pixel_fetcher_set_bg_color */


/*****/

static void
pixel_fetcher_get_pixel(pixel_fetcher_t *pf, int x, int y, guchar *pixel)
{
	gint    col, row;
	gint    coloff, rowoff;
	guchar *p;
	int     i;

	if ((x < sel_x1) || (x >= sel_x2) ||
	    (y < sel_y1) || (y >= sel_y2)) {
		for (i = 0; i < pf->img_bpp; i++)
			pixel[i] = pf->bg_color[i];

		return;
	} /* if */

	col    = x / pf->tile_width;
	coloff = x % pf->tile_width;
	row    = y / pf->tile_height;
	rowoff = y % pf->tile_height;

	if ((col != pf->col) ||
	    (row != pf->row) ||
	    (pf->tile == NULL)) {
		if (pf->tile != NULL)
			gimp_tile_unref(pf->tile, FALSE);

		pf->tile = gimp_drawable_get_tile(pf->drawable, FALSE, row, col);
		gimp_tile_ref(pf->tile);

		pf->col = col;
		pf->row = row;
	} /* if */

	p = pf->tile->data + pf->img_bpp * (pf->tile->ewidth * rowoff + coloff);

	for (i = pf->img_bpp; i; i--)
		*pixel++ = *p++;
} /* pixel_fetcher_get_pixel */


/*****/

static void
pixel_fetcher_destroy(pixel_fetcher_t *pf)
{
	if (pf->tile != NULL)
		gimp_tile_unref(pf->tile, FALSE);

	g_free(pf);
} /* pixel_fetcher_destroy */
