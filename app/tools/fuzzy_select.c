/* The GIMP -- an image manipulation program
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
#include <stdlib.h>
#include <math.h>
#include "appenv.h"
#include "boundary.h"
#include "canvas.h"
#include "draw_core.h"
#include "drawable.h"
#include "edit_selection.h"
#include "fuzzy_select.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "gdisplay.h"
#include "rect_select.h"
#include "paint_funcs_area.h"
#include "pixelarea.h"
#include "pixelrow.h"


/* ------------------------------------------------------------
  
      prototype of a generalized PixelArea control structure
             with a sample seed-fill application
  
  ------------------------------------------------------------ */

/* a bounding box */

typedef struct _BBox BBox;
struct _BBox
{
  gint x, y, w, h;
};



/* a range of pixels in a row */

typedef struct _Segment Segment;
struct _Segment
{
  guint start;
  guint end;
  Segment * next;
};

static Segment * segment_new    (guint start, guint end, Segment * next);
static void      segment_delete (Segment * s);
static void      segment_add    (Segment ** s, guint start, guint end);
static void      segment_del    (Segment ** s, guint start, guint end);



/* a generalized PixelArea control structure */

typedef struct _PixelIter PixelIter;

/* an image area operation.  it may be precision neutral or not */
typedef guint (*PIFunc) (PixelIter * p);

/* the data block for the control structure */
struct _PixelIter
{
  /* the image on which to operate */
  Canvas * image;

  /* automatic canvas portion ref: none, RO, RW */
  guint reftype;
  
  /* the bounding box of the whole area */
  BBox area; 
  
  /* the bounding box of the current chunk */
  BBox chunk;
  
  /* user callback to invoke */
  PIFunc callback;

  /* user data */
  void * data;

  /* pixel to do list */
  Segment ** todo;  
};

/* public methods */
PixelIter *  pixeliter_new       (void);
void         pixeliter_delete    (PixelIter *);
guint        pixeliter_init      (PixelIter * p, Canvas * c,
                                  PIFunc callback, void * userdata,
                                  guint x, guint y, guint w, guint h,
                                  guint reftype);
guint        pixeliter_process   (PixelIter *);
guint        pixeliter_get_work  (PixelIter *, gint *, gint *);
void         pixeliter_add_work  (PixelIter *, gint, gint, gint, gint);
void         pixeliter_del_work  (PixelIter *, gint, gint, gint, gint);

/* private methods */
static guint pixeliter_choose_chunk (PixelIter *);
static guint pixeliter_first        (PixelIter *, BBox *, guint *, guint *);

/* types of refs for pixeliter_init */
#define REF_NONE 0
#define REF_RO   1
#define REF_RW   2








typedef struct _fuzzy_select FuzzySelect;
typedef struct _SeedFillData SeedFillData;

/* the precision-specific kernel for the seed fill */
typedef guint (*SFFunc) (PixelIter * p, SeedFillData * d);


struct _fuzzy_select
{
  DrawCore *     core;         /*  Core select object                      */

  int            x, y;         /*  Point from which to execute seed fill  */
  int            last_x;       /*                                         */
  int            last_y;       /*  variables to keep track of sensitivity */
  gfloat         threshold;    /*  threshold value for soft seed fill     */

  int            op;           /*  selection operation (ADD, SUB, etc)     */
};


struct _SeedFillData
{
  /* the specific kernel to use */
  SFFunc kernel;
  
  /* fill this with the contiguous area */
  Canvas * mask;

  /* the threshold */
  gfloat threshold;

  /* should we antialias */
  guint antialias;

  /* the color to match */
  PixelRow color;
  
  /* size of existing scratch space */
  guint x_w, x_h;

  /* temp copy of mask for this chunk */
  Canvas * x_mask;

  /* the output of absdiff for this chunk */
  Canvas * x_diff;
};


/*  fuzzy select action functions  */

static void   fuzzy_select_button_press   (Tool *, GdkEventButton *, gpointer);
static void   fuzzy_select_button_release (Tool *, GdkEventButton *, gpointer);
static void   fuzzy_select_motion         (Tool *, GdkEventMotion *, gpointer);
static void   fuzzy_select_draw           (Tool *);
static void   fuzzy_select_control        (Tool *, int, gpointer);

/*  fuzzy select action functions  */
static GdkSegment *   fuzzy_select_calculate (Tool *, void *, int *);


/*  XSegments which make up the fuzzy selection boundary  */

static GdkSegment *segs = NULL;
static int         num_segs = 0;
static Channel *   fuzzy_mask = NULL;
static SelectionOptions *fuzzy_options = NULL;

static void fuzzy_select (GImage *, GimpDrawable *, int, int, double);
static Argument *fuzzy_select_invoker (Argument *);


/* seed fill kernel */

static guint seed_fill        (PixelIter * p);
static guint seed_fill_alloc  (PixelIter * p, SeedFillData * d);
static void  seed_fill_free   (SeedFillData * d);
static guint seed_fill_u8     (PixelIter * p, SeedFillData * d);
static guint seed_fill_u16    (PixelIter * p, SeedFillData * d);
static guint seed_fill_float  (PixelIter * p, SeedFillData * d);


/*************************************/
/*  Fuzzy selection apparatus  */

Channel * 
find_contiguous_region  (
                         GImage * gimage,
                         GimpDrawable * drawable,
                         int antialias,
                         gfloat threshold,
                         int x,
                         int y,
                         int sample_merged
                         )
{
  Channel * mask;
  Canvas * s;

  /* use either the current layer or the composite */
  s = (sample_merged)
    ? gimage_projection (gimage)
    : drawable_data (drawable);

  /* create a new mask */
  mask = channel_new_mask (gimage->ID,
                           canvas_width (s), canvas_height (s),
                           tag_precision (canvas_tag (s)));

  /* if the image refs okay, do a seed fill */
  if (canvas_portion_refro (s, x, y) == REFRC_OK)
    {
      SeedFillData data;
      PixelIter * p;
      
      /* choose the right kernel */
      switch (tag_precision (canvas_tag (s)))
        {
        case PRECISION_U8:
          data.kernel = seed_fill_u8;
          break;
        case PRECISION_U16:
          data.kernel = seed_fill_u16;
          break;
        case PRECISION_FLOAT:
          data.kernel = seed_fill_float;
          break;
        case PRECISION_NONE:
        default:
          data.kernel = NULL;
        }

      /* init the seed fill parms */
      data.mask = drawable_data (GIMP_DRAWABLE(mask));
      data.threshold = threshold ? threshold : 0.000001;
      data.antialias = antialias;
      pixelrow_init (&data.color,
                     canvas_tag (s),
                     canvas_portion_data (s, x, y),
                     1);
        
      /* init the scratch space too */
      data.x_w = 0;
      data.x_h = 0;
      data.x_mask = NULL;
      data.x_diff = NULL;
      
      /* set up a processor for the seed fill */
      p = pixeliter_new ();
      
      if (pixeliter_init (p, s,
                          seed_fill, &data,
                          0, 0,
                          0, 0,
                          REF_RO))
        {
          /* mark the initial click point for processing */
          pixeliter_add_work (p, x, y, 1, 1);

          /* at this point, the seed fill operation could be queued
             for execution.  a smart scheduler could optimize the
             execution of the seed fill.  since no such mechanism
             exists, do the work immediately */
          while (pixeliter_process (p));

        }

      /* clean up */
      pixeliter_delete (p);
      seed_fill_free (&data);

      /* unref the canvas */
      canvas_portion_unref (s, x, y);
    }

  /* we don;t know the bounds anymore */
  channel_invalidate_bounds (mask);
  
  return mask;
}

static guint 
seed_fill  (
            PixelIter * p
            )
{
  PixelArea src, dst;
  SeedFillData * d;
  
  /* abort the entire operation if things get weird */
  g_return_val_if_fail (p != NULL, FALSE);
  d = (SeedFillData *) p->data;
  g_return_val_if_fail (d != NULL, FALSE);
  g_return_val_if_fail (d->kernel != NULL, FALSE);
  
  /* realloc scratch space */
  if (seed_fill_alloc (p, d) != TRUE)
    return FALSE;
  
  /* copy in part of the existing mask into our scratch pad */
  pixelarea_init (&src, d->mask,
                  p->chunk.x, p->chunk.y,
                  p->chunk.w, p->chunk.h,
                  FALSE);    
  pixelarea_init (&dst, d->x_mask,
                  0, 0,
                  0, 0,
                  TRUE);
  copy_area (&src, &dst);

  /* create the sufficiently_different map */
  pixelarea_init (&src, p->image,
                  p->chunk.x, p->chunk.y,
                  p->chunk.w, p->chunk.h,
                  FALSE);    
  pixelarea_init (&dst, d->x_diff,
                  0, 0,
                  0, 0,
                  TRUE);
  absdiff_area (&src, &dst, &d->color, d->threshold, d->antialias);

  /* process the chunk and save any modifications to the real mask */
  if (d->kernel (p, d) == TRUE)
    {
      pixelarea_init (&src, d->x_mask,
                      0, 0,
                      0, 0,
                      FALSE);
      pixelarea_init (&dst, d->mask,
                      p->chunk.x, p->chunk.y,
                      p->chunk.w, p->chunk.h,
                      TRUE);    
      copy_area (&src, &dst);
    }

  /* we want to be called again */
  return TRUE;
}


/* allocate scratch space for use while calculating contiguous mask */
static guint 
seed_fill_alloc  (
                  PixelIter * p,
                  SeedFillData * d
                  )
{
  /* see if the old stuff is the right size */
  if ((p->chunk.w == d->x_w) &&
      (p->chunk.h == d->x_h) &&
      (d->x_mask) && (d->x_diff))
    {
      return TRUE;
    }

  /* blow away the old stuff */
  seed_fill_free (d);

  /* save the new chunk size */
  d->x_w = p->chunk.w;
  d->x_h = p->chunk.h;

  /* temporary copy of existing mask */
  d->x_mask = canvas_new (canvas_tag (d->mask),
                          d->x_w, d->x_h,
                          STORAGE_FLAT);

  if (canvas_portion_refrw (d->x_mask, 0, 0) != REFRC_OK)
    {
      g_warning ("ooooops");
      return FALSE;
    }
  

  /* scratch pad holding abs diff values */
  d->x_diff = canvas_new (canvas_tag (d->mask),
                          d->x_w, d->x_h,
                          STORAGE_FLAT);

  if (canvas_portion_refrw (d->x_diff, 0, 0) != REFRC_OK)
    {
      g_warning ("ooooops2");
      return FALSE;
    }
  
  return TRUE;
}


/* delete any scratch space that's hanging around */
static void 
seed_fill_free  (
                 SeedFillData * d
                 )
{
  if (d->x_mask)
    {
      canvas_portion_unref (d->x_mask, 0, 0);
      canvas_delete (d->x_mask);
    }

  if (d->x_diff)
    {
      canvas_portion_unref (d->x_diff, 0, 0);
      canvas_delete (d->x_diff);
    }
}


static guint
seed_fill_u8 (
               PixelIter * p,
               SeedFillData * d
               )
{
  guint changed = FALSE;
  guint x, y;
  
  /* get pixels in this chunk which need attention */
  while (pixeliter_get_work (p, &x, &y))
    {
      guint8 * dd, * md;
      int xmin_c, xmax_c;

      /* see if this pixel should be done */
      dd = (guint8*) canvas_portion_data (d->x_diff, 0, y - p->chunk.y);
      if (dd[x - p->chunk.x] == 0)
        continue;

      /* see if this pixel is already done */
      md = (guint8*) canvas_portion_data (d->x_mask, 0, y - p->chunk.y);
      if (md[x - p->chunk.x] != 0)
        continue;
      
      /* find contig segment endpoints */
      for (xmin_c = x - p->chunk.x;
           (xmin_c >= 0) && (dd[xmin_c] != 0);
           xmin_c--);

      for (xmax_c = x - p->chunk.x;
           (xmax_c < p->chunk.w) && (dd[xmax_c] != 0);
           xmax_c++);
             
      /* add work to adjacent chunks if segment reached the edges */
      if (xmin_c < 0)
        {
          pixeliter_add_work (p,
                              p->chunk.x - 1, y,
                              1, 1);
        }

      if (xmax_c == p->chunk.w)
        {
          pixeliter_add_work (p,
                              p->chunk.x + p->chunk.w, y,
                              1, 1);
        }
      
      xmin_c++;
      xmax_c--;
      
      /* del work for those pixels */
      pixeliter_del_work (p,
                          p->chunk.x + xmin_c, y,
                          xmax_c - xmin_c + 1, 1);

      /* add work to next and prev rows */
      pixeliter_add_work (p,
                          p->chunk.x + xmin_c, y - 1,
                          xmax_c - xmin_c + 1, 1);
      
      pixeliter_add_work (p,
                          p->chunk.x + xmin_c, y + 1,
                          xmax_c - xmin_c + 1, 1);
      
      /* save those pixels on x_mask */
      memcpy (&md[xmin_c], &dd[xmin_c], (xmax_c - xmin_c + 1) * sizeof (guint8));
      changed = TRUE;
    }

  return changed;
}

static guint
seed_fill_u16 (
               PixelIter * p,
               SeedFillData * d
               )
{
  guint changed = FALSE;
  guint x, y;
  
  /* get pixels in this chunk which need attention */
  while (pixeliter_get_work (p, &x, &y))
    {
      guint16 * dd, * md;
      int xmin_c, xmax_c;

      /* see if this pixel should be done */
      dd = (guint16*) canvas_portion_data (d->x_diff, 0, y - p->chunk.y);
      if (dd[x - p->chunk.x] == 0)
        continue;

      /* see if this pixel is already done */
      md = (guint16*) canvas_portion_data (d->x_mask, 0, y - p->chunk.y);
      if (md[x - p->chunk.x] != 0)
        continue;
      
      /* find contig segment endpoints */
      for (xmin_c = x - p->chunk.x;
           (xmin_c >= 0) && (dd[xmin_c] != 0);
           xmin_c--);

      for (xmax_c = x - p->chunk.x;
           (xmax_c < p->chunk.w) && (dd[xmax_c] != 0);
           xmax_c++);
             
      /* add work to adjacent chunks if segment reached the edges */
      if (xmin_c < 0)
        {
          pixeliter_add_work (p,
                              p->chunk.x - 1, y,
                              1, 1);
        }

      if (xmax_c == p->chunk.w)
        {
          pixeliter_add_work (p,
                              p->chunk.x + p->chunk.w, y,
                              1, 1);
        }
      
      xmin_c++;
      xmax_c--;
      
      /* del work for those pixels */
      pixeliter_del_work (p,
                          p->chunk.x + xmin_c, y,
                          xmax_c - xmin_c + 1, 1);

      /* add work to next and prev rows */
      pixeliter_add_work (p,
                          p->chunk.x + xmin_c, y - 1,
                          xmax_c - xmin_c + 1, 1);
      
      pixeliter_add_work (p,
                          p->chunk.x + xmin_c, y + 1,
                          xmax_c - xmin_c + 1, 1);
      
      /* save those pixels on x_mask */
      memcpy (&md[xmin_c], &dd[xmin_c], (xmax_c - xmin_c + 1) * sizeof (guint16));
      changed = TRUE;
    }

  return changed;
}

static guint
seed_fill_float (
                 PixelIter * p,
                 SeedFillData * d
                 )
{
  guint changed = FALSE;
  guint x, y;
  
  /* get pixels in this chunk which need attention */
  while (pixeliter_get_work (p, &x, &y))
    {
      gfloat * dd, * md;
      int xmin_c, xmax_c;

      /* see if this pixel should be done */
      dd = (gfloat*) canvas_portion_data (d->x_diff, 0, y - p->chunk.y);
      if (dd[x - p->chunk.x] == 0)
        continue;

      /* see if this pixel is already done */
      md = (gfloat*) canvas_portion_data (d->x_mask, 0, y - p->chunk.y);
      if (md[x - p->chunk.x] != 0)
        continue;
      
      /* find contig segment endpoints */
      for (xmin_c = x - p->chunk.x;
           (xmin_c >= 0) && (dd[xmin_c] != 0);
           xmin_c--);

      for (xmax_c = x - p->chunk.x;
           (xmax_c < p->chunk.w) && (dd[xmax_c] != 0);
           xmax_c++);
             
      /* add work to adjacent chunks if segment reached the edges */
      if (xmin_c < 0)
        {
          pixeliter_add_work (p,
                              p->chunk.x - 1, y,
                              1, 1);
        }

      if (xmax_c == p->chunk.w)
        {
          pixeliter_add_work (p,
                              p->chunk.x + p->chunk.w, y,
                              1, 1);
        }
      
      xmin_c++;
      xmax_c--;
      
      /* del work for those pixels */
      pixeliter_del_work (p,
                          p->chunk.x + xmin_c, y,
                          xmax_c - xmin_c + 1, 1);

      /* add work to next and prev rows */
      pixeliter_add_work (p,
                          p->chunk.x + xmin_c, y - 1,
                          xmax_c - xmin_c + 1, 1);
      
      pixeliter_add_work (p,
                          p->chunk.x + xmin_c, y + 1,
                          xmax_c - xmin_c + 1, 1);
      
      /* save those pixels on x_mask */
      memcpy (&md[xmin_c], &dd[xmin_c], (xmax_c - xmin_c + 1) * sizeof (gfloat));
      changed = TRUE;
    }

  return changed;
}


static void
fuzzy_select (GImage *gimage, GimpDrawable *drawable, int op, int feather,
	      double feather_radius)
{
  int off_x, off_y;

  /*  if applicable, replace the current selection  */
  if (op == REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  drawable_offsets (drawable, &off_x, &off_y);
  if (feather)
    channel_feather (fuzzy_mask, gimage_get_mask (gimage),
		     feather_radius, op, off_x, off_y);
  else
    channel_combine_mask (gimage_get_mask (gimage),
			  fuzzy_mask, op, off_x, off_y);

  /*  free the fuzzy region struct  */
  channel_delete (fuzzy_mask);
  fuzzy_mask = NULL;
}

/*  fuzzy select action functions  */

static void
fuzzy_select_button_press (Tool *tool, GdkEventButton *bevent,
			   gpointer gdisp_ptr)
{
  GDisplay *gdisp;
  FuzzySelect *fuzzy_sel;

  gdisp = (GDisplay *) gdisp_ptr;
  fuzzy_sel = (FuzzySelect *) tool->private;

  fuzzy_sel->x = bevent->x;
  fuzzy_sel->y = bevent->y;
  fuzzy_sel->last_x = fuzzy_sel->x;
  fuzzy_sel->last_y = fuzzy_sel->y;
  fuzzy_sel->threshold = default_threshold;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  if (bevent->state & GDK_MOD1_MASK)
    {
      init_edit_selection (tool, gdisp_ptr, bevent, MaskTranslate);
      return;
    }
  else if ((bevent->state & GDK_SHIFT_MASK) && !(bevent->state & GDK_CONTROL_MASK))
    fuzzy_sel->op = ADD;
  else if ((bevent->state & GDK_CONTROL_MASK) && !(bevent->state & GDK_SHIFT_MASK))
    fuzzy_sel->op = SUB;
  else if ((bevent->state & GDK_CONTROL_MASK) && (bevent->state & GDK_SHIFT_MASK))
    fuzzy_sel->op = INTERSECT;
  else
    {
      if (! (layer_is_floating_sel (gimage_get_active_layer (gdisp->gimage))) &&
	  gdisplay_mask_value (gdisp, bevent->x, bevent->y) > HALF_WAY)
	{
	  init_edit_selection (tool, gdisp_ptr, bevent, MaskToLayerTranslate);
	  return;
	}
      fuzzy_sel->op = REPLACE;
    }

  /*  calculate the region boundary  */
  segs = fuzzy_select_calculate (tool, gdisp_ptr, &num_segs);

  draw_core_start (fuzzy_sel->core,
		   gdisp->canvas->window,
		   tool);
}

static void
fuzzy_select_button_release (Tool *tool, GdkEventButton *bevent,
			     gpointer gdisp_ptr)
{
  FuzzySelect * fuzzy_sel;
  GDisplay * gdisp;
  GimpDrawable *drawable;

  gdisp = (GDisplay *) gdisp_ptr;
  fuzzy_sel = (FuzzySelect *) tool->private;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  draw_core_stop (fuzzy_sel->core, tool);
  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      drawable = (fuzzy_options->sample_merged) ? NULL : gimage_active_drawable (gdisp->gimage);
      fuzzy_select (gdisp->gimage, drawable, fuzzy_sel->op,
		    fuzzy_options->feather, fuzzy_options->feather_radius);

      gdisplays_flush ();

      /*  adapt the threshold based on the final value of this use  */
      default_threshold = fuzzy_sel->threshold;
    }

  /*  If the segment array is allocated, free it  */
  if (segs)
    g_free (segs);
  segs = NULL;
}

static void
fuzzy_select_motion (Tool *tool, GdkEventMotion *mevent, gpointer gdisp_ptr)
{
  FuzzySelect * fuzzy_sel;
  GdkSegment * new_segs;
  int num_new_segs;
  int diff, diff_x, diff_y;

  if (tool->state != ACTIVE)
    return;

  fuzzy_sel = (FuzzySelect *) tool->private;

  diff_x = mevent->x - fuzzy_sel->last_x;
  diff_y = mevent->y - fuzzy_sel->last_y;

  diff = ((ABS (diff_x) > ABS (diff_y)) ? diff_x : diff_y) / 2;

  fuzzy_sel->last_x = mevent->x;
  fuzzy_sel->last_y = mevent->y;

  fuzzy_sel->threshold += diff/255.0;
  fuzzy_sel->threshold = BOUNDS (fuzzy_sel->threshold, 0, 1);

  /*  calculate the new fuzzy boundary  */
  new_segs = fuzzy_select_calculate (tool, gdisp_ptr, &num_new_segs);

  /*  stop the current boundary  */
  draw_core_pause (fuzzy_sel->core, tool);

  /*  make sure the XSegment array is freed before we assign the new one  */
  if (segs)
    g_free (segs);
  segs = new_segs;
  num_segs = num_new_segs;

  /*  start the new boundary  */
  draw_core_resume (fuzzy_sel->core, tool);
}

static GdkSegment *
fuzzy_select_calculate (Tool *tool, void *gdisp_ptr, int *nsegs)
{
  PixelArea maskPR;
  FuzzySelect *fuzzy_sel;
  GDisplay *gdisp;
  Channel *new;
  GdkSegment *segs;
  BoundSeg *bsegs;
  int i, x, y;
  GimpDrawable *drawable;
  int use_offsets;

  fuzzy_sel = (FuzzySelect *) tool->private;
  gdisp = (GDisplay *) gdisp_ptr;
  drawable = gimage_active_drawable (gdisp->gimage);

  use_offsets = (fuzzy_options->sample_merged) ? FALSE : TRUE;

  gdisplay_untransform_coords (gdisp, fuzzy_sel->x,
			       fuzzy_sel->y, &x, &y, FALSE, use_offsets);

  new = find_contiguous_region (gdisp->gimage, drawable, fuzzy_options->antialias,
				fuzzy_sel->threshold, x, y, fuzzy_options->sample_merged);

  if (fuzzy_mask)
    channel_delete (fuzzy_mask);
  fuzzy_mask = channel_ref (new);

  /*  calculate and allocate a new XSegment array which represents the boundary
   *  of the color-contiguous region
   */
  pixelarea_init (&maskPR, drawable_data (GIMP_DRAWABLE(fuzzy_mask)),
                  0, 0,
                  0, 0,
                  FALSE);

  bsegs = find_mask_boundary (&maskPR, nsegs, WithinBounds,
			      0, 0,
			      drawable_width (GIMP_DRAWABLE(fuzzy_mask)),
			      drawable_height (GIMP_DRAWABLE(fuzzy_mask)));

  segs = (GdkSegment *) g_malloc (sizeof (GdkSegment) * *nsegs);

  for (i = 0; i < *nsegs; i++)
    {
      gdisplay_transform_coords (gdisp, bsegs[i].x1, bsegs[i].y1, &x, &y, use_offsets);
      segs[i].x1 = x;  segs[i].y1 = y;
      gdisplay_transform_coords (gdisp, bsegs[i].x2, bsegs[i].y2, &x, &y, use_offsets);
      segs[i].x2 = x;  segs[i].y2 = y;
    }

  /*  free boundary segments  */
  g_free (bsegs);

  return segs;
}

static void
fuzzy_select_draw (Tool *tool)
{
  FuzzySelect * fuzzy_sel;

  fuzzy_sel = (FuzzySelect *) tool->private;

  if (segs)
    gdk_draw_segments (fuzzy_sel->core->win, fuzzy_sel->core->gc, segs, num_segs);
}

static void
fuzzy_select_control (Tool *tool, int action, gpointer gdisp_ptr)
{
  FuzzySelect * fuzzy_sel;

  fuzzy_sel = (FuzzySelect *) tool->private;

  switch (action)
    {
    case PAUSE :
      draw_core_pause (fuzzy_sel->core, tool);
      break;
    case RESUME :
      draw_core_resume (fuzzy_sel->core, tool);
      break;
    case HALT :
      draw_core_stop (fuzzy_sel->core, tool);
      break;
    }
}

Tool *
tools_new_fuzzy_select (void)
{
  Tool * tool;
  FuzzySelect * private;

  /*  The tool options  */
  if (!fuzzy_options)
    fuzzy_options = create_selection_options (FUZZY_SELECT);

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (FuzzySelect *) g_malloc (sizeof (FuzzySelect));

  private->core = draw_core_new (fuzzy_select_draw);

  tool->type = FUZZY_SELECT;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = fuzzy_select_button_press;
  tool->button_release_func = fuzzy_select_button_release;
  tool->motion_func = fuzzy_select_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = rect_select_cursor_update;
  tool->control_func = fuzzy_select_control;

  return tool;
}

void
tools_free_fuzzy_select (Tool *tool)
{
  FuzzySelect * fuzzy_sel;

  fuzzy_sel = (FuzzySelect *) tool->private;
  draw_core_free (fuzzy_sel->core);
  g_free (fuzzy_sel);
}

/*  The fuzzy_select procedure definition  */
ProcArg fuzzy_select_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_FLOAT,
    "x",
    "x coordinate of initial seed fill point: (image coordinates)"
  },
  { PDB_FLOAT,
    "y",
    "y coordinate of initial seed fill point: (image coordinates)"
  },
  { PDB_INT32,
    "threshold",
    "threshold in intensity levels: 0 <= threshold <= 255"
  },
  { PDB_INT32,
    "operation",
    "the selection operation: { ADD (0), SUB (1), REPLACE (2), INTERSECT (3) }"
  },
  { PDB_INT32,
    "antialias",
    "antialiasing On/Off"
  },
  { PDB_INT32,
    "feather",
    "feather option for selections"
  },
  { PDB_FLOAT,
    "feather_radius",
    "radius for feather operation"
  },
  { PDB_INT32,
    "sample_merged",
    "use the composite image, not the drawable"
  }
};

ProcRecord fuzzy_select_proc =
{
  "gimp_fuzzy_select",
  "Create a fuzzy selection starting at the specified coordinates on the specified drawable",
  "This tool creates a fuzzy selection over the specified image.  A fuzzy selection is determined by a seed fill under the constraints of the specified threshold.  Essentially, the color at the specified coordinates (in the drawable) is measured and the selection expands outwards from that point to any adjacent pixels which are not significantly different (as determined by the threshold value).  This process continues until no more expansion is possible.  The antialiasing parameter allows the final selection mask to contain intermediate values based on close misses to the threshold bar at pixels along the seed fill boundary.  Feathering can be enabled optionally and is controlled with the \"feather_radius\" paramter.  If the sample_merged parameter is non-zero, the data of the composite image will be used instead of that for the specified drawable.  This is equivalent to sampling for colors after merging all visible layers.  In the case of a merged sampling, the supplied drawable is ignored.  If the sample is merged, the specified coordinates are relative to the image origin; otherwise, they are relative to the drawable's origin.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  10,
  fuzzy_select_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { fuzzy_select_invoker } },
};


static Argument *
fuzzy_select_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  int op;
  gfloat threshold;
  int antialias;
  int feather;
  int sample_merged;
  double x, y;
  double feather_radius;
  int int_value;

  drawable    = NULL;
  op          = REPLACE;
  threshold   = 0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  x, y  */
  if (success)
    {
      x = args[2].value.pdb_float;
      y = args[3].value.pdb_float;
    }
  /*  threshold  */
  if (success)
    {
      int_value = args[4].value.pdb_int;
      if (int_value >= 0 && int_value <= 255)
	threshold = int_value / 255.0;
      else
	success = FALSE;
    }
  /*  operation  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      switch (int_value)
	{
	case 0: op = ADD; break;
	case 1: op = SUB; break;
	case 2: op = REPLACE; break;
	case 3: op = INTERSECT; break;
	default: success = FALSE;
	}
    }
  /*  antialiasing?  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      antialias = (int_value) ? TRUE : FALSE;
    }
  /*  feathering  */
  if (success)
    {
      int_value = args[7].value.pdb_int;
      feather = (int_value) ? TRUE : FALSE;
    }
  /*  feather radius  */
  if (success)
    {
      feather_radius = args[8].value.pdb_float;
    }
  /*  sample merged  */
  if (success)
    {
      int_value = args[9].value.pdb_int;
      sample_merged = (int_value) ? TRUE : FALSE;
    }

  /*  call the fuzzy_select procedure  */
  if (success)
    {
      Channel *new;
      Channel *old_fuzzy_mask;

      new = find_contiguous_region (gimage, drawable, antialias, threshold, x, y, sample_merged);
      old_fuzzy_mask = fuzzy_mask;
      fuzzy_mask = new;

      drawable = (sample_merged) ? NULL : drawable;
      fuzzy_select (gimage, drawable, op, feather, feather_radius);

      fuzzy_mask = old_fuzzy_mask;
    }

  return procedural_db_return_args (&fuzzy_select_proc, success);
}











/* the rest of this stuff goes elsewhere eventually */

static Segment *
segment_new (
             guint start,
             guint end,
             Segment * next
             )
{
  Segment * s = (Segment *) g_malloc (sizeof (Segment));

  s->start = start;
  s->end = end;
  s->next = next;

  return s;
}


static void
segment_delete (
                Segment * s
                )
{
  g_free (s);
}


/* add (x1 <= x < x2) to the segment */
static void 
segment_add  (
              Segment ** s,
              guint start,
              guint end
              )
{
  g_return_if_fail (s != NULL);

  if (start >= end)
    return;
  
  /* insert a new segment or extend an existing one */
  while (1)
    {
      /* did we hit the end of the list */
      if (*s == NULL)
        {
          /* insert at the end of the (possibly empty) list */
          *s = segment_new (start, end, NULL);
          return;
        }

      /* do we start before this guy */
      if (start < (*s)->start)
        {
          /* insert a new segment */
          *s = segment_new (start, end, *s);
          break;
        }
      
      /* do we start in or immediately after this guy */
      if (start <= (*s)->end)
        {
          /* do we end before this guy */
          if (end <= (*s)->end)
            return;

          /* extend the current segment */
          (*s)->end = end;
          break;
        }

      /* try next segment */
      s = &((*s)->next);
    }

  /* at this point, *s points to a segment with the correct start and
     end points.  however, it might overlap the following segments */
  while (1)
    {
      /* are we at the end of the list */
      if ((*s)->next == NULL)
        return;

      /* is the next guy correct */
      if ((*s)->next->start > (*s)->end)
        return;

      {
        /* pull the next guy out of the list */
        Segment * n = (*s)->next;
        (*s)->next = n->next;

        /* does the next guy have a partial segment we need */
        if (n->end > (*s)->end)
          (*s)->end = n->end;

        segment_delete (n);
      }
    }
}


static void 
segment_del  (
              Segment ** s,
              guint start,
              guint end
              )
{
  g_return_if_fail (s != NULL);

  if (start >= end)
    return;
  
  while (1)
    {
      /* we hit the end of the list */
      if (*s == NULL)
        return;

      /* see if we overlap segment head */
      if ((*s)->end > end)
        {
          if ((*s)->start >= end)
            {
              /* first segment is completely after deletion */
              return;
            }
          else if ((*s)->start >= start)
            {
              /* first segment head overlaps us */
              (*s)->start = end;
              return;
            }
          else
            {
              /* we split first segment */
              *s = segment_new ((*s)->start, start, *s);
              (*s)->next->start = end;
              return;
            }
        }
      
      /* see if we overlap segment tail */
      else if ((*s)->end > start)
        {
          if ((*s)->start >= start)
            {
              Segment * n = *s;
              *s = (*s)->next;
              segment_delete (n);
              continue;
            }
          else
            {
              (*s)->end = start;
            }
        }

      /* next segment */
      s = &((*s)->next);
    }
}


PixelIter * 
pixeliter_new  (
                void
                )
{
  PixelIter * p;
  
  p = (PixelIter *) g_malloc (sizeof (PixelIter));
  
  p->image = NULL;
  p->reftype = REF_NONE;
  p->area.x = 0;
  p->area.y = 0;
  p->area.w = 0;
  p->area.h = 0;
  p->chunk.x = 0;
  p->chunk.y = 0;
  p->chunk.w = 0;
  p->chunk.h = 0;

  p->callback = NULL;
  p->data = NULL;
  p->todo = NULL;

  return p;
}


void 
pixeliter_delete  (
                   PixelIter * p
                   )
{
  guint h;
  
  g_return_if_fail (p != NULL);
  g_return_if_fail (p->todo != NULL);

  h = p->area.h;

  while (h-- > 0)
    {
      Segment * s = p->todo[h];
      while (s)
        {
          Segment * n = s->next;
          segment_delete (s);
          s = n;
        }
    }

  g_free (p->todo);
}


/* set up p so that it will work within the xywh bounding box on image
  calling callback with the pixeliter p as an arg.

  data is stored inside p so callback has access to it
*/
guint 
pixeliter_init  (
                 PixelIter * p,
                 Canvas * image,
                 PIFunc callback,
                 void * data,
                 guint x,
                 guint y,
                 guint w,
                 guint h,
                 guint reftype
                 )
{
  g_return_val_if_fail (p != NULL, FALSE);
  g_return_val_if_fail (image != NULL, FALSE);
  g_return_val_if_fail (w >= 0, FALSE);
  g_return_val_if_fail (h >= 0, FALSE);
  g_return_val_if_fail (callback != NULL, FALSE);
  g_return_val_if_fail (x < canvas_width (image), FALSE);
  g_return_val_if_fail (y < canvas_height (image), FALSE);

  p->image = image;
  p->reftype = reftype;
  p->area.x = x;
  p->area.y = y;
  p->area.w = canvas_width (image);
  if ((w > 0) && (w <= p->area.w))
    p->area.w = w;
  p->area.h = canvas_height (image);
  if ((h > 0) && (h <= p->area.h))
    p->area.h = h;
  p->chunk.x = 0;
  p->chunk.y = 0;
  p->chunk.w = 0;
  p->chunk.h = 0;
  
  p->callback = callback;
  p->data = data;
  p->todo = (Segment **) g_malloc (sizeof (Segment*) * p->area.h);
  memset (p->todo, 0, (sizeof (Segment*) * p->area.h));
  
  return TRUE;
}


/* choose a chunk of the image to process and invoke the user callback
  on that chunk.

  from the perspective of the user callback, the exact area chosen is
  random.  however, the entire area is guaranteed to be contained
  somewhere within a single portion of the canvas and inside the
  bounding box specified in the init call.

  the user callback may cause the entire operation to cease simply by
  returning FALSE.  this is useful for things like "is_area_empty"
  which may only need to see a single data point to return an answer.

  the user callback may add or delete work from the todo list.  this
  is useful for operations like seed fill that need to go exploring.

  it is trivial to emulate the current pixelarea_process behaviour.
  simply schedule the top left corner of the bounding box as the
  initial work, have the callback process the entire chunk, then add
  work for the strip of pixels directly to the right and below the
  chunk.  if pixeliter_choose_chunk() always prefers upper left chunks
  over lower right chunks, the chunks will be processed in exactly the
  same order.
  
  return true if more work remains, false if todo list is empty or if
  the operation was aborted by the user callback */
guint 
pixeliter_process  (
                    PixelIter * p
                    )
{
  guint rc = FALSE;
  
  g_return_val_if_fail (p != NULL, FALSE);

  /* see if work remains on this image */
  if (pixeliter_choose_chunk (p) != TRUE)
    return FALSE;
  
  /* if requested, ref the underlying image data */
  if (p->reftype == REF_RO)
    if (canvas_portion_refro (p->image, p->chunk.x, p->chunk.y) != REFRC_OK)
      return FALSE;

  if (p->reftype == REF_RW)
    if (canvas_portion_refrw (p->image, p->chunk.x, p->chunk.y) != REFRC_OK)
      return FALSE;

  /* ready ike?  kick the callback! */
  rc = p->callback (p);

  /* unref the image data if required */
  if ((p->reftype == REF_RO) || (p->reftype == REF_RW))
    {
      (void) canvas_portion_unref (p->image, p->chunk.x, p->chunk.y);
    }

  return rc;
}


/* pick a pixel that needs attention and decide how big of a chunk to
   work on.  set the chunk boundaries.

   any policy decisions can go in here, but we must at a minimum
   respect the users bounding box and the underlying canvas portion
   boundaries */
static guint
pixeliter_choose_chunk (
                        PixelIter * p
                        )
{
  gint x, y, w, h;

  /* pick a pixel from the todo list */
  if (pixeliter_first (p, &p->area, &x, &y) != TRUE)
    return FALSE;
  
  /* size the chunk around the pixel, respecting user bounding box and
     canvas portion boundaries.  other criteria, such as an absolute
     maximum chunk size/aspect ratio/whatever can be added */
  x = MAX (p->area.x,
           canvas_portion_x (p->image, x, y));
  y = MAX (p->area.y,
           canvas_portion_y (p->image, x, y));
  w = MIN (p->area.w - (x - p->area.x),
           canvas_portion_width (p->image, x, y));
  h = MIN (p->area.h - (y - p->area.y),
           canvas_portion_height (p->image, x, y));

  /* save it so the callback knows where to work */
  p->chunk.x = x;
  p->chunk.y = y;
  p->chunk.w = w;
  p->chunk.h = h;

  return TRUE;
}         


/* find the first pixel in the todo list for p that lies within box b.
   return coords in x,y */
static guint 
pixeliter_first  (
                  PixelIter * p,
                  BBox * b,
                  guint * xx,
                  guint * yy
                  )
{
  guint x, y, w, h;
  
  g_return_val_if_fail (p != NULL, FALSE);
  g_return_val_if_fail (b != NULL, FALSE);
  g_return_val_if_fail (xx != NULL, FALSE);
  g_return_val_if_fail (yy != NULL, FALSE);

  x = b->x;
  y = b->y;
  w = b->w;
  h = b->h;
  
  /* is the work right or below area */
  if (x >= p->area.x + p->area.w)
    return FALSE;
  
  if (y >= p->area.y + p->area.h)
    return FALSE;

  /* does work start left or above area */
  if (x < p->area.x)
    {
      w = w - (p->area.x - x);
      x = p->area.x;
    }
  
  if (y < p->area.y)
    {
      h = h - (p->area.y - y);
      y = p->area.y;
    }

  w = CLAMP (w, 0, p->area.w);
  h = CLAMP (h, 0, p->area.h);

  if ((w == 0) || (h == 0))
    return FALSE;

  while (h--)
    {
      Segment * s = p->todo[y - p->area.y];
      while (s)
        {
          /* is segment completely to right of our range */
          if (s->start >= x+w)
            break;
          
          /* is segment completely to left of our range */
          if (s->end <= x)
            {
              s = s->next;
              continue;
            }
          
          *xx = MAX (s->start, x);
          *yy = y;
          
          return TRUE;
        }

      /* try next row */
      y++;
    }

  return FALSE;
}


/* add the specified rectangle to the PixelIter's todo list.  only the
  part that overlaps the PixelIters area will be added.

  no return code */
void 
pixeliter_add_work  (
                     PixelIter * p,
                     gint x,
                     gint y,
                     gint w,
                     gint h
                     )
{
  g_return_if_fail (p != NULL);
  g_return_if_fail (p->todo != NULL);

  /* is the work right or below area */
  if (x >= p->area.x + p->area.w)
    return;
  
  if (y >= p->area.y + p->area.h)
    return;

  /* does work start left or above area */
  if (x < p->area.x)
    {
      w = w - (p->area.x - x);
      x = p->area.x;
    }
  
  if (y < p->area.y)
    {
      h = h - (p->area.y - y);
      y = p->area.y;
    }

  w = CLAMP (w, 0, p->area.w);
  h = CLAMP (h, 0, p->area.h);

  if ((w == 0) || (h == 0))
    return;

  /* add to each segment list */
  while (h--)
    {
      Segment ** s = &(p->todo[y - p->area.y]);
      segment_add (s, x, x + w);
      y++;
    }
}


/* remove the specified rectangle to the PixelIter's todo list.  only
  the part that overlaps the PixelIters area will be cleared.

  no return code */
void
pixeliter_del_work  (
                     PixelIter * p,
                     gint x,
                     gint y,
                     gint w,
                     gint h
                     )
{
  g_return_if_fail (p != NULL);
  g_return_if_fail (p->todo != NULL);
  
  /* is the work right or below area */
  if (x >= p->area.x + p->area.w)
    return;
  
  if (y >= p->area.y + p->area.h)
    return;

  /* does work start left or above area */
  if (x < p->area.x)
    {
      w = w - (p->area.x - x);
      x = p->area.x;
    }
  
  if (y < p->area.y)
    {
      h = h - (p->area.y - y);
      y = p->area.y;
    }

  w = CLAMP (w, 0, p->area.w);
  h = CLAMP (h, 0, p->area.h);

  if ((w == 0) || (h == 0))
    return;

  /* add to each segment list */
  while (h--)
    {
      Segment ** s = &(p->todo[y - p->area.y]);
      segment_del (s, x, x + w);
      y++;
    }
}


/* see if there are any pixels in the todo list that fall within the
  chunk we are processing. if so, pick one, remove from todo list, and
  store in x,y.

  return true if a point was returned, false if not */
guint 
pixeliter_get_work  (
                     PixelIter * p,
                     gint * x,
                     gint * y
                     )
{
  g_return_val_if_fail (p != NULL, FALSE);
  g_return_val_if_fail (p->todo != NULL, FALSE);
  g_return_val_if_fail (x != NULL, FALSE);
  g_return_val_if_fail (y != NULL, FALSE);

  /* get a pixel */
  if (pixeliter_first (p, &p->chunk, x, y) != TRUE)
    return FALSE;

  /* mark it as done */
  pixeliter_del_work (p, *x, *y, 1, 1);
  
  return TRUE;
}

