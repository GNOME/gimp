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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "appenv.h"
#include "brushes.h"
#include "canvas.h"
#include "draw_core.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "layer.h"
#include "paint.h"
#include "paint_core_16.h"
#include "paint_funcs_area.h"
#include "paint_funcs_row.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "tools.h"
#include "undo.h"




/*  global variables--for use in the various paint tools  */
PaintCore16  non_gui_paint_core_16;



/*  local function prototypes  */
static void      paint_core_16_button_press    (Tool *, GdkEventButton *, gpointer);
static void      paint_core_16_button_release  (Tool *, GdkEventButton *, gpointer);
static void      paint_core_16_motion          (Tool *, GdkEventMotion *, gpointer);
static void      paint_core_16_cursor_update   (Tool *, GdkEventMotion *, gpointer);
static void      paint_core_16_control         (Tool *, int, gpointer);
static void      paint_core_16_no_draw         (Tool *);

static Canvas *  paint_core_16_get_brush_mask  (PaintCore16 *, int);
static Canvas *  paint_core_16_subsample_mask  (Canvas *, double, double);
static Canvas *  paint_core_16_solidify_mask   (Canvas *);

static void      paint_core_16_paste           (PaintCore16 *, Canvas *, GimpDrawable *,
                                                Paint *, Paint *, int, int);
static void      paint_core_16_replace         (PaintCore16 *, Canvas *, GimpDrawable *,
                                                Paint *, Paint *, int);

static void      paint_16_to_canvas_tiles      (PaintCore16 *, Canvas *, Paint *);
static void      paint_16_to_canvas_buf        (PaintCore16 *, Canvas *, Paint *);

static void      set_16_undo_tiles             (GimpDrawable *, int, int, int, int);
static void      set_16_canvas_tiles           (int, int, int, int);



/*  undo blocks variables  */
static Canvas *  undo_tiles = NULL;
static Canvas *  canvas_tiles = NULL;


/*  paint buffers variables  */
static Canvas *  canvas_buf = NULL;




Tool * 
paint_core_16_new  (
                    int type
                    )
{
  Tool * tool;
  PaintCore16 * private;

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (PaintCore16 *) g_malloc (sizeof (PaintCore16));

  private->core = draw_core_new (paint_core_16_no_draw);

  tool->type = type;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->gdisp_ptr = NULL;
  tool->private = (void *) private;

  tool->button_press_func = paint_core_16_button_press;
  tool->button_release_func = paint_core_16_button_release;
  tool->motion_func = paint_core_16_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = paint_core_16_cursor_update;
  tool->control_func = paint_core_16_control;

  return tool;
}


void 
paint_core_16_free  (
                     Tool * tool
                     )
{
  PaintCore16 * paint_core;

  paint_core = (PaintCore16 *) tool->private;

  /*  Make sure the selection core is not visible  */
  if (tool->state == ACTIVE && paint_core->core)
    draw_core_stop (paint_core->core, tool);

  /*  Free the selection core  */
  if (paint_core->core)
    draw_core_free (paint_core->core);

  /*  Cleanup memory  */
  paint_core_16_cleanup ();

  /*  Free the paint core  */
  g_free (paint_core);
}


int 
paint_core_16_init  (
                     PaintCore16 * paint_core,
                     GimpDrawable *drawable,
                     double x,
                     double y
                     )
{
  GBrushP brush;

  paint_core->curx = x;
  paint_core->cury = y;

  /*  Each buffer is the same size as the maximum bounds of the active brush... */
  if (!(brush = get_active_brush ()))
    {
      warning ("No brushes available for use with this tool.");
      return FALSE;
    }

  paint_core->spacing =
    (double) MAX (get_brush_height (brush), get_brush_width (brush)) *
    ((double) get_brush_spacing () / 100.0);
  if (paint_core->spacing < 1.0)
    paint_core->spacing = 1.0;
  paint_core->brush_mask = get_brush_canvas (brush);


  /*  Allocate the undo structure  */
  if (undo_tiles)
    canvas_delete (undo_tiles);

  undo_tiles = canvas_new (drawable_tag (drawable),
                           drawable_width (drawable),
                           drawable_height (drawable),
                           Tiling_NO);


  /*  Allocate the canvas blocks structure  */
  if (canvas_tiles)
    canvas_delete (canvas_tiles);

  canvas_tiles = canvas_new (tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_NO),
                             drawable_width (drawable),
                             drawable_height (drawable),
                             Tiling_NO);

  /*  Get the initial undo extents  */
  paint_core->x1 = paint_core->x2 = paint_core->curx;
  paint_core->y1 = paint_core->y2 = paint_core->cury;

  paint_core->distance = 0.0;

  return TRUE;
}
  
  
void 
paint_core_16_interpolate  (
                            PaintCore16 * paint_core,
                            GimpDrawable *drawable
                            )
{
#define    EPSILON  0.00001
  int n;
  double dx, dy;
  double left;
  double t;
  double initial;
  double dist;
  double total;

  /* see how far we've moved */
  dx = paint_core->curx - paint_core->lastx;
  dy = paint_core->cury - paint_core->lasty;

  /* bail if we haven't moved */
  if (!dx && !dy)
    return;

  dist = sqrt (SQR (dx) + SQR (dy));
  total = dist + paint_core->distance;
  initial = paint_core->distance;

  while (paint_core->distance < total)
    {
      n = (int) (paint_core->distance / paint_core->spacing + 1.0 + EPSILON);
      left = n * paint_core->spacing - paint_core->distance;
      paint_core->distance += left;

      if (paint_core->distance <= total)
	{
	  t = (paint_core->distance - initial) / dist;

	  paint_core->curx = paint_core->lastx + dx * t;
	  paint_core->cury = paint_core->lasty + dy * t;
	  (* paint_core->paint_func) (paint_core, drawable, MOTION_PAINT);
	}
    }

  paint_core->distance = total;
  paint_core->curx = paint_core->lastx + dx;
  paint_core->cury = paint_core->lasty + dy;
}


void 
paint_core_16_finish  (
                       PaintCore16 * paint_core,
                       GimpDrawable *drawable,
                       int tool_id
                       )
{
  GImage *gimage;
  PaintUndo16 *pu;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((paint_core->x2 == paint_core->x1) || (paint_core->y2 == paint_core->y1))
    return;

  undo_push_group_start (gimage, PAINT_CORE_UNDO);

  pu = (PaintUndo16 *) g_malloc (sizeof (PaintUndo16));
  pu->tool_ID = tool_id;
  pu->lastx = paint_core->startx;
  pu->lasty = paint_core->starty;

  /*  Push a paint undo  */
  undo_push_paint (gimage, pu);

  /*  push an undo  */
  /* FIXME undo_tiles */
  drawable_apply_image (drawable, paint_core->x1, paint_core->y1,
			paint_core->x2, paint_core->y2, undo_tiles, TRUE);
  undo_tiles = NULL;

  /*  push the group end  */
  undo_push_group_end (gimage);

  /*  invalidate the drawable--have to do it here, because
   *  it is not done during the actual painting.
   */
  drawable_invalidate_preview (drawable);
}


void 
paint_core_16_cleanup  (
                        void
                        )
{
  if (undo_tiles)
    {
      canvas_delete (undo_tiles);
      undo_tiles = NULL;
    }

  if (canvas_tiles)
    {
      canvas_delete (canvas_tiles);
      canvas_tiles = NULL;
    }

  if (canvas_buf)
    {
      canvas_delete (canvas_buf);
      canvas_buf = NULL;
    }
}


void 
paint_core_16_paste_canvas  (
                             PaintCore16 * paint_core,
                             GimpDrawable * drawable,
                             Paint * brush_opacity,
                             Paint * image_opacity,
                             int paint_mode,
                             BrushHardness brush_hardness,
                             ApplyMode apply_mode
                             )
{
  Canvas *brush_mask;
  
  /*  get the brush mask  */
  brush_mask = paint_core_16_get_brush_mask (paint_core, brush_hardness);
  
  /*  paste the canvas buf  */
  paint_core_16_paste (paint_core, brush_mask, drawable,
                       brush_opacity, image_opacity, paint_mode, apply_mode);
}


/* Similar to paint_core_paste_canvas, but replaces the alpha channel
   rather than using it to composite (i.e. transparent over opaque
   becomes transparent rather than opauqe. */
void

paint_core_16_replace_canvas  (
                               PaintCore16 * paint_core,
                               GimpDrawable *drawable,
                               Paint * brush_opacity,
                               Paint * image_opacity,
                               BrushHardness brush_hardness,
                               ApplyMode apply_mode
                               )
{
  Canvas *brush_mask;

  /*  get the brush mask  */
  brush_mask = paint_core_16_get_brush_mask (paint_core, brush_hardness);

  /*  paste the canvas buf  */
  paint_core_16_replace (paint_core, brush_mask, drawable,
                         brush_opacity, image_opacity, apply_mode);
}


Canvas * 
paint_core_16_get_paint_area  (
                               PaintCore16 * paint_core,
                               GimpDrawable *drawable
                               )
{
  Tag   tag;
  int   x, y;
  int   dw, dh;
  int   bw, bh;
  int   x1, y1, x2, y2;
  

  /*  adjust the x and y coordinates to the upper left corner of the brush  */
  x = (int) paint_core->curx;
  y = (int) paint_core->cury;

  bw = canvas_width (paint_core->brush_mask) / 2 + 1;
  bh = canvas_height (paint_core->brush_mask) / 2 + 1;
  
  dw = drawable_width (drawable);
  dh = drawable_height (drawable);

  x1 = CLAMP (x - bw, 0, dw);
  y1 = CLAMP (y - bh, 0, dh);
  x2 = CLAMP (x + bw, 0, dw);
  y2 = CLAMP (y + bh, 0, dh);

  paint_core->x = x1;
  paint_core->y = y1;
  paint_core->w = x2 - x1;
  paint_core->h = y2 - y1;
  
  /*  configure the canvas buffer  */
  tag = tag_set_alpha (drawable_tag (drawable), ALPHA_YES);
  canvas_buf = canvas_realloc (canvas_buf, tag,
                               (x2 - x1), (y2 - y1),
                               Tiling_NEVER);

  return canvas_buf;
}



Canvas * 
paint_core_16_get_orig_image  (
                               PaintCore16 * paint_core,
                               GimpDrawable *drawable,
                               int x1,
                               int y1,
                               int x2,
                               int y2
                               )
{
  static Canvas * orig_buf = NULL;
#if 0
  PixelArea srcPR, destPR;
  Tag   tag;
  Tile *undo_tile;
  int h;
  int pixelwidth;
  int refd;
  unsigned char * s, * d;
  void * pr;

  /*  configure the canvas buffer  */
  tag = tag_set_alpha (drawable_tag (drawable), ALPHA_YES);
  canvas_buf = canvas_realloc (canvas_buf, tag,
                               (x2 - x1), (y2 - y1),
                               Tiling_NEVER);

  orig_buf = temp_buf_resize (orig_buf, drawable_bytes (drawable),
			      x1, y1, (x2 - x1), (y2 - y1));
  x1 = CLAMP (x1, 0, drawable_width (drawable));
  y1 = CLAMP (y1, 0, drawable_height (drawable));
  x2 = CLAMP (x2, 0, drawable_width (drawable));
  y2 = CLAMP (y2, 0, drawable_height (drawable));

  /*  configure the pixel regions  */
  pixelarea_init (&srcPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
  destPR.x = 0; destPR.y = 0;
  destPR.w = (x2 - x1); destPR.h = (y2 - y1);
  destPR.private.tag = tag_by_bytes (orig_buf->bytes);
  destPR.rowstride = orig_buf->bytes * orig_buf->width;
  destPR.data = temp_buf_data (orig_buf) +
    (y1 - orig_buf->y) * destPR.rowstride +
    (x1 - orig_buf->x) * pixelarea_bytes (&destPR);

  for (pr = pixel_regions_register (2, &srcPR, &destPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      /*  If the undo tile corresponding to this location is valid, use it  */
      undo_tile = tile_manager_get_tile (undo_tiles, srcPR.x, srcPR.y, 0);
      if (undo_tile->valid == TRUE)
	{
	  tile_ref (undo_tile);
	  s = undo_tile->data + srcPR.rowstride * (srcPR.y % TILE_HEIGHT) +
	    pixelarea_bytes (&srcPR) * (srcPR.x % TILE_WIDTH);
	  refd = TRUE;
	}
      else
	{
	  s = srcPR.data;
	  refd = FALSE;
	}
      d = destPR.data;
      pixelwidth = srcPR.w * pixelarea_bytes (&srcPR);
      h = srcPR.h;
      while (h --)
	{
	  memcpy (d, s, pixelwidth);
	  s += srcPR.rowstride;
	  d += destPR.rowstride;
	}

      if (refd)
	tile_unref (undo_tile, FALSE);
    }
#endif

  return orig_buf;
}






/************************************************************
 *             LOCAL FUNCTION DEFINITIONS                   *
 ************************************************************/

static void 
paint_core_16_button_press  (
                             Tool * tool,
                             GdkEventButton * bevent,
                             gpointer gdisp_ptr
                             )
{
  PaintCore16 * paint_core;
  GDisplay * gdisp;
  int drawable;
  int draw_line = 0;
  double x, y;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore16 *) tool->private;

  gdisplay_untransform_coords_f (gdisp,
                                 (double) bevent->x, (double) bevent->y,
                                 &x, &y,
                                 TRUE);
  drawable = gimage_active_drawable (gdisp->gimage);

  if (! paint_core_16_init (paint_core, drawable, x, y))
    return;

  paint_core->state = bevent->state;

  /*  if this is a new image, reinit the core vals  */
  if (gdisp_ptr != tool->gdisp_ptr ||
      ! (bevent->state & GDK_SHIFT_MASK))
    {
      /*  initialize some values  */
      paint_core->startx = paint_core->lastx = paint_core->curx;
      paint_core->starty = paint_core->lasty = paint_core->cury;
    }
  /*  If shift is down and this is not the first paint
   *  stroke, then draw a line from the last coords to the pointer
   */
  else if (bevent->state & GDK_SHIFT_MASK)
    {
      draw_line = 1;
      paint_core->startx = paint_core->lastx;
      paint_core->starty = paint_core->lasty;
    }

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;
  tool->paused_count = 0;

  /*  pause the current selection and grab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage->ID, SelectionPause);

  /* add motion memory if you press mod1 first */
  if (bevent->state & GDK_MOD1_MASK)
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
  else
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
  
  /*  Let the specific painting function initialize itself  */
  (* paint_core->paint_func) (paint_core, drawable, INIT_PAINT);

  /*  Paint to the image  */
  if (draw_line)
    {
      paint_core_16_interpolate (paint_core, drawable);
      paint_core->lastx = paint_core->curx;
      paint_core->lasty = paint_core->cury;
    }
  else
    (* paint_core->paint_func) (paint_core, drawable, MOTION_PAINT);

  gdisplay_flush (gdisp);
}


static void 
paint_core_16_button_release  (
                               Tool * tool,
                               GdkEventButton * bevent,
                               gpointer gdisp_ptr
                               )
{
  GDisplay * gdisp;
  GImage * gimage;
  PaintCore16 * paint_core;

  gdisp = (GDisplay *) gdisp_ptr;
  gimage = gdisp->gimage;
  paint_core = (PaintCore16 *) tool->private;

  /*  resume the current selection and ungrab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage->ID, SelectionResume);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  /*  Let the specific painting function finish up  */
  (* paint_core->paint_func) (paint_core, gimage_active_drawable (gdisp->gimage), FINISH_PAINT);

  /*  Set tool state to inactive -- no longer painting */
  tool->state = INACTIVE;

  paint_core_16_finish (paint_core, gimage_active_drawable (gdisp->gimage), tool->ID);
  gdisplays_flush ();
}


static void 
paint_core_16_motion  (
                       Tool * tool,
                       GdkEventMotion * mevent,
                       gpointer gdisp_ptr
                       )
{
  GDisplay * gdisp;
  PaintCore16 * paint_core;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore16 *) tool->private;

  gdisplay_untransform_coords_f (gdisp,
                                 (double) mevent->x, (double) mevent->y,
				 &paint_core->curx, &paint_core->cury,
                                 TRUE);

  paint_core->state = mevent->state;

  paint_core_16_interpolate (paint_core, gimage_active_drawable (gdisp->gimage));

  gdisplay_flush (gdisp);

  paint_core->lastx = paint_core->curx;
  paint_core->lasty = paint_core->cury;
}


static void 
paint_core_16_cursor_update  (
                              Tool * tool,
                              GdkEventMotion * mevent,
                              gpointer gdisp_ptr
                              )
{
  GDisplay *gdisp;
  Layer *layer;
  GdkCursorType ctype = GDK_TOP_LEFT_ARROW;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp,
                               mevent->x, mevent->y,
                               &x, &y,
                               FALSE, FALSE);
  
  if ((layer = gimage_get_active_layer (gdisp->gimage)))
    {
#if 0
      if (x >= layer->offset_x && y >= layer->offset_y &&
          x < (layer->offset_x + layer->width) &&
          y < (layer->offset_y + layer->height))
        {
          /*  One more test--is there a selected region?
           *  if so, is cursor inside?
           */
          if (gimage_mask_is_empty (gdisp->gimage))
            ctype = GDK_PENCIL;
          else if (gimage_mask_value (gdisp->gimage, x, y))
            ctype = GDK_PENCIL;
        }
#endif
    }
  gdisplay_install_tool_cursor (gdisp, ctype);
}


static void 
paint_core_16_control  (
                        Tool * tool,
                        int action,
                        gpointer gdisp_ptr
                        )
{
  PaintCore16 * paint_core;
  GDisplay *gdisp;
  int drawable;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore16 *) tool->private;
  drawable = gimage_active_drawable (gdisp->gimage);

  switch (action)
    {
    case PAUSE :
      draw_core_pause (paint_core->core, tool);
      break;
    case RESUME :
      draw_core_resume (paint_core->core, tool);
      break;
    case HALT :
      (* paint_core->paint_func) (paint_core, drawable, FINISH_PAINT);
      paint_core_16_cleanup ();
      break;
    }
}


static void 
paint_core_16_no_draw  (
                        Tool * tool
                        )
{
  return;
}


static Canvas * 
paint_core_16_get_brush_mask  (
                               PaintCore16 * paint_core,
                               int brush_hardness
                               )
{
  Canvas * bm;

  switch (brush_hardness)
    {
    case SOFT:
      bm = paint_core_16_subsample_mask (paint_core->brush_mask,
                                         paint_core->curx, paint_core->cury);
      break;
    case HARD:
      bm = paint_core_16_solidify_mask (paint_core->brush_mask);
      break;
    default:
      bm = NULL;
      break;
    }

  return bm;
}


static Canvas * 
paint_core_16_subsample_mask  (
                               Canvas * mask,
                               double x,
                               double y
                               )
{
#define KERNEL_WIDTH   3
#define KERNEL_HEIGHT  3

  /*  Brush pixel subsampling kernels  */
  static int subsample[4][4][9] =
  {
    {
      { 16, 48, 0, 48, 144, 0, 0, 0, 0 },
      { 0, 64, 0, 0, 192, 0, 0, 0, 0 },
      { 0, 48, 16, 0, 144, 48, 0, 0, 0 },
      { 0, 32, 32, 0, 96, 96, 0, 0, 0 },
    },
    {
      { 0, 0, 0, 64, 192, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 256, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 192, 64, 0, 0, 0 },
      { 0, 0, 0, 0, 128, 128, 0, 0, 0 },
    },
    {
      { 0, 0, 0, 48, 144, 0, 16, 48, 0 },
      { 0, 0, 0, 0, 192, 0, 0, 64, 0 },
      { 0, 0, 0, 0, 144, 48, 0, 48, 16 },
      { 0, 0, 0, 0, 96, 96, 0, 32, 32 },
    },
    {
      { 0, 0, 0, 32, 96, 0, 32, 96, 0 },
      { 0, 0, 0, 0, 128, 0, 0, 128, 0 },
      { 0, 0, 0, 0, 96, 32, 0, 96, 32 },
      { 0, 0, 0, 0, 64, 64, 0, 64, 64 },
    },
  };
  
  static Canvas * kernel_brushes[4][4] = {
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL}
  };

  static Canvas * last_brush = NULL;

  Canvas * dest;
  double left;
  unsigned char * m, * d;
  int * k;
  int index1, index2;
  int * kernel;
  int new_val;
  int i, j;
  int r, s;

  x += (x < 0) ? canvas_width (mask) : 0;
  left = x - ((int) x);
  index1 = (int) (left * 4);
  index1 = (index1 < 0) ? 0 : index1;

  y += (y < 0) ? canvas_height (mask) : 0;
  left = y - ((int) y);
  index2 = (int) (left * 4);
  index2 = (index2 < 0) ? 0 : index2;

  kernel = subsample[index2][index1];

  if ((mask == last_brush) && kernel_brushes[index2][index1])
    return kernel_brushes[index2][index1];
  else if (mask != last_brush)
    for (i = 0; i < 4; i++)
      for (j = 0; j < 4; j++)
	{
	  if (kernel_brushes[i][j])
	    canvas_delete (kernel_brushes[i][j]);
	  kernel_brushes[i][j] = NULL;
	}

  last_brush = mask;
  kernel_brushes[index2][index1] = canvas_new (canvas_tag (mask),
                                               canvas_width (mask) + 2,
                                               canvas_height (mask) + 2,
                                               canvas_tiling (mask));
  dest = kernel_brushes[index2][index1];

  /* FIXME */
#if 0
  m = mask_buf_data (mask);
  for (i = 0; i < mask->height; i++)
    {
      for (j = 0; j < mask->width; j++)
	{
	  k = kernel;
	  for (r = 0; r < KERNEL_HEIGHT; r++)
	    {
	      d = mask_buf_data (dest) + (i+r) * dest->width + j;
	      s = KERNEL_WIDTH;
	      while (s--)
		{
		  new_val = *d + ((*m * *k++) >> 8);
		  *d++ = (new_val > 255) ? 255 : new_val;
		}
	    }
	  m++;
	}
    }
#endif
  return dest;
}


static Canvas * 
paint_core_16_solidify_mask  (
                              Canvas * brush_mask
                              )
{
  static Canvas * solid_brush = NULL;
  static Canvas * last_brush  = NULL;
  
#if 0
  int i, j;
  unsigned char * data, * src;

  if (brush_mask == last_brush)
    return solid_brush;

  last_brush = brush_mask;
  if (solid_brush)
    canvas_delete (solid_brush);
  solid_brush = canvas_new (canvas_tag (mask),
                            canvas_width (mask) + 2,
                            canvas_height (mask) + 2,
                            canvas_tiling (mask));

  /* FIXME */
  /*  get the data and advance one line into it  */
  data = mask_buf_data (solid_brush) + solid_brush->width;
  src = mask_buf_data (brush_mask);

  for (i = 0; i < brush_mask->height; i++)
    {
      data++;
      for (j = 0; j < brush_mask->width; j++)
	{
	  *data++ = (*src++) ? OPAQUE : TRANSPARENT;
	}
      data++;
    }
#endif
  return solid_brush;
}





static void 
paint_core_16_paste  (
                      PaintCore16 * paint_core,
                      Canvas * brush_mask,
                      GimpDrawable *drawable,
                      Paint * brush_opacity,
                      Paint * image_opacity,
                      int paint_mode,
                      int mode
                      )
{
  GImage *gimage;
  PixelArea srcPR;
  Canvas *alt = NULL;
  int offx, offy;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /*  set undo blocks  */
  set_16_undo_tiles (drawable,
                     paint_core->x, paint_core->y,
                     canvas_width (canvas_buf), canvas_height (canvas_buf));

  /*  If the mode is CONSTANT:
   *   combine the canvas buf, the brush mask to the canvas tiles
   */
  if (mode == CONSTANT)
    {
      /*  initialize any invalid canvas tiles  */
      set_16_canvas_tiles (paint_core->x, paint_core->y,
                           canvas_width (canvas_buf), canvas_height (canvas_buf));

      paint_16_to_canvas_tiles (paint_core, brush_mask, brush_opacity);
      alt = undo_tiles;
    }
  /*  Otherwise:
   *   combine the canvas buf and the brush mask to the canvas buf
   */
  else  /*  mode != CONSTANT  */
    paint_16_to_canvas_buf (paint_core, brush_mask, brush_opacity);

  /*  initialize canvas buf source pixel regions  */
  pixelarea_init (&srcPR, canvas_buf,
                  0, 0,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  TRUE);

  /*  apply the paint area to the gimage  */
  /* FIXME srcPR alt image_opacity*/
  painthit_apply (gimage, drawable, &srcPR,
                  FALSE, image_opacity, paint_mode,
                  alt,  /*  specify an alternative src1  */
                  paint_core->x, paint_core->y);

  /*  Update the undo extents  */
  paint_core->x1 = MIN (paint_core->x1, paint_core->x);
  paint_core->y1 = MIN (paint_core->y1, paint_core->y);
  paint_core->x2 = MAX (paint_core->x2, (paint_core->x + canvas_width (canvas_buf)));
  paint_core->y2 = MAX (paint_core->y2, (paint_core->y + canvas_height (canvas_buf)));

  /*  Update the gimage--it is important to call gdisplays_update_area
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  drawable_offsets (drawable, &offx, &offy);
  gdisplays_update_area (gimage->ID,
                         paint_core->x + offx, paint_core->y + offy,
			 canvas_width (canvas_buf), canvas_height (canvas_buf));
}


/* This works similarly to paint_core_paste. However, instead of combining
   the canvas to the paint core drawable using one of the combination
   modes, it uses a "replace" mode (i.e. transparent pixels in the 
   canvas erase the paint core drawable).

   When not drawing on alpha-enabled images, it just paints using NORMAL
   mode.
*/
static void 
paint_core_16_replace  (
                        PaintCore16 * paint_core,
                        Canvas * brush_mask,
                        GimpDrawable *drawable,
                        Paint * brush_opacity,
                        Paint * image_opacity,
                        int mode
                        )
{
  GImage *gimage;
  PixelArea srcPR, maskPR;
  int offx, offy;

  if (!drawable_has_alpha (drawable))
    {
      paint_core_16_paste (paint_core, brush_mask, drawable,
                           brush_opacity, image_opacity, NORMAL_MODE,
                           mode);
      return;
    }

  if (mode != INCREMENTAL)
    {
      g_warning ("paint_core_16_replace only works in INCREMENTAL mode");
      return;
    }

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /*  set undo blocks  */
  set_16_undo_tiles (drawable,
                     paint_core->x, paint_core->y,
                     canvas_width (canvas_buf), canvas_height (canvas_buf));

  pixelarea_init (&maskPR, brush_mask,
                  0, 0,
                  canvas_width (brush_mask), canvas_height (brush_mask),
                  TRUE);
  
  pixelarea_init (&srcPR, canvas_buf,
                  0, 0,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  TRUE);

  /*  apply the paint area to the gimage  */
  /* FIXME srcPR maskPR image_opacity */
  painthit_replace (gimage, drawable, &srcPR,
                    FALSE, image_opacity,
                    &maskPR,
                    paint_core->x, paint_core->y);

  /*  Update the undo extents  */
  paint_core->x1 = MIN (paint_core->x1, paint_core->x);
  paint_core->y1 = MIN (paint_core->y1, paint_core->y);
  paint_core->x2 = MAX (paint_core->x2, (paint_core->x + canvas_width (canvas_buf)));
  paint_core->y2 = MAX (paint_core->y2, (paint_core->y + canvas_height (canvas_buf)));

  /*  Update the gimage--it is important to call gdisplays_update_area
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  drawable_offsets (drawable, &offx, &offy);
  gdisplays_update_area (gimage->ID,
                         paint_core->x + offx, paint_core->y + offy,
			 canvas_width (canvas_buf), canvas_height (canvas_buf));
}


static void 
paint_16_to_canvas_tiles  (
                           PaintCore16 * paint_core,
                           Canvas * brush_mask,
                           Paint * brush_opacity
                           )
{
  PixelArea srcPR, maskPR;
  int x, y;
  int xoff, yoff;

  x = (int) paint_core->curx - (canvas_width (brush_mask) >> 1);
  y = (int) paint_core->cury - (canvas_height (brush_mask) >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;


  /*  combine the mask and canvas tiles  */
  pixelarea_init (&srcPR, canvas_tiles,
                  paint_core->x, paint_core->y,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  TRUE);

  pixelarea_init (&maskPR, brush_mask,
                  xoff, yoff,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  TRUE);

  combine_mask_and_area (&srcPR, &maskPR, brush_opacity);

  /*  apply the canvas tiles to the canvas buf  */
  pixelarea_init (&srcPR, canvas_buf,
                  0, 0,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  TRUE);
  
  pixelarea_init (&maskPR, canvas_tiles,
                  paint_core->x, paint_core->y,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  FALSE);

  apply_mask_to_area (&srcPR, &maskPR, OPAQUE);
}


static void 
paint_16_to_canvas_buf  (
                         PaintCore16 * paint_core,
                         Canvas * brush_mask,
                         Paint * brush_opacity
                         )
{
  PixelArea srcPR, maskPR;

  /*  combine the canvas buf and the brush mask to the canvas buf  */
  pixelarea_init (&srcPR, canvas_buf,
                  0, 0,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  TRUE);
  
  pixelarea_init (&maskPR, brush_mask,
                  0, 0,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  FALSE);

  apply_mask_to_area (&srcPR, &maskPR, brush_opacity);
}


/* FIXME: the whole routine */
static void 
set_16_undo_tiles  (
                    GimpDrawable *drawable,
                    int x,
                    int y,
                    int w,
                    int h
                    )
{
#if 0
  int i, j;
  Tile *src_tile;
  Tile *dest_tile;

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
	{
	  dest_tile = tile_manager_get_tile (undo_tiles, j, i, 0);
	  if (dest_tile->valid == FALSE)
	    {
	      src_tile = tile_manager_get_tile (drawable_data (drawable), j, i, 0);
	      tile_ref (src_tile);
	      tile_ref (dest_tile);
	      memcpy (dest_tile->data, src_tile->data,
		      (src_tile->ewidth * src_tile->eheight * src_tile->bpp));
	      tile_unref (src_tile, FALSE);
	      tile_unref (dest_tile, TRUE);
	    }
	}
    }
#endif
}


/* FIXME: the whole routine */
static void 
set_16_canvas_tiles  (
                      int x,
                      int y,
                      int w,
                      int h
                      )
{
#if 0
  int i, j;
  Tile *tile;

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
	{
	  tile = tile_manager_get_tile (canvas_tiles, j, i, 0);
	  if (tile->valid == FALSE)
	    {
	      tile_ref (tile);
	      memset (tile->data, 0, (tile->ewidth * tile->eheight * tile->bpp));
	      tile_unref (tile, TRUE);
	    }
	}
    }
#endif
}


void 
painthit_apply  (
                 GImage * gimage,
                 GimpDrawable * drawable,
                 PixelArea * src2PR,
                 int undo,
                 int opacity,
                 int mode,
                 Canvas * src1_tiles,
                 int x,
                 int y
                 )
{
#if 0
  Channel * mask;
  int x1, y1, x2, y2;
  int offset_x, offset_y;
  PixelRegion src1PR, destPR, maskPR;
  int operation;
  int active [MAX_CHANNELS];

  /*  get the selection mask if one exists  */
  mask = (gimage_mask_is_empty (gimage)) ?
    NULL : gimage_get_mask (gimage);

  /*  configure the active channel array  */
  gimage_get_active_channels (gimage, drawable, active);

  /*  determine what sort of operation is being attempted and
   *  if it's actually legal...
   */
  operation = valid_combinations [drawable_type (drawable)][src2PR->bytes];
  if (operation == -1)
    {
      warning ("gimage_apply_image sent illegal parameters\n");
      return;
    }

  /*  get the layer offsets  */
  drawable_offsets (drawable, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within gimage bounds  */
  x1 = CLAMP (x, 0, drawable_width (drawable));
  y1 = CLAMP (y, 0, drawable_height (drawable));
  x2 = CLAMP (x + src2PR->w, 0, drawable_width (drawable));
  y2 = CLAMP (y + src2PR->h, 0, drawable_height (drawable));

  if (mask)
    {
      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      x1 = CLAMP (x1, -offset_x, drawable_width (GIMP_DRAWABLE(mask)) - offset_x);
      y1 = CLAMP (y1, -offset_y, drawable_height (GIMP_DRAWABLE(mask)) - offset_y);
      x2 = CLAMP (x2, -offset_x, drawable_width (GIMP_DRAWABLE(mask)) - offset_x);
      y2 = CLAMP (y2, -offset_y, drawable_height (GIMP_DRAWABLE(mask)) - offset_y);
    }

  /*  If the calling procedure specified an undo step...  */
  if (undo)
    drawable_apply_image (drawable, x1, y1, x2, y2, NULL, FALSE);

  /* configure the pixel regions
   *  If an alternative to using the drawable's data as src1 was provided...
   */
  if (src1_tiles)
    pixel_region_init (&src1PR, src1_tiles, x1, y1, (x2 - x1), (y2 - y1), FALSE);
  else
    pixel_region_init (&src1PR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);
  pixel_region_resize (src2PR, src2PR->x + (x1 - x), src2PR->y + (y1 - y), (x2 - x1), (y2 - y1));

  if (mask)
    {
      int mx, my;

      /*  configure the mask pixel region
       *  don't use x1 and y1 because they are in layer
       *  coordinate system.  Need mask coordinate system
       */
      mx = x1 + offset_x;
      my = y1 + offset_y;

      pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE(mask)), mx, my, (x2 - x1), (y2 - y1), FALSE);
      combine_regions (&src1PR, src2PR, &destPR, &maskPR, NULL,
		       opacity, mode, active, operation);
    }
  else
    combine_regions (&src1PR, src2PR, &destPR, NULL, NULL,
		     opacity, mode, active, operation);
#endif
}

void 
painthit_replace  (
                   GImage * gimage,
                   GimpDrawable * drawable,
                   PixelArea * src2PR,
                   int undo,
                   int opacity,
                   PixelArea * maskPR,
                   int x,
                   int y
                   )
{
#if 0
  Channel * mask;
  int x1, y1, x2, y2;
  int offset_x, offset_y;
  PixelRegion src1PR, destPR;
  PixelRegion mask2PR, tempPR;
  unsigned char *temp_data;
  int operation;
  int active [MAX_CHANNELS];

  /*  get the selection mask if one exists  */
  mask = (gimage_mask_is_empty (gimage)) ?
    NULL : gimage_get_mask (gimage);

  /*  configure the active channel array  */
  gimage_get_active_channels (gimage, drawable, active);

  /*  determine what sort of operation is being attempted and
   *  if it's actually legal...
   */
  operation = valid_combinations [drawable_type (drawable)][src2PR->bytes];
  if (operation == -1)
    {
      warning ("gimage_apply_image sent illegal parameters\n");
      return;
    }

  /*  get the layer offsets  */
  drawable_offsets (drawable, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within gimage bounds  */
  x1 = CLAMP (x, 0, drawable_width (drawable));
  y1 = CLAMP (y, 0, drawable_height (drawable));
  x2 = CLAMP (x + src2PR->w, 0, drawable_width (drawable));
  y2 = CLAMP (y + src2PR->h, 0, drawable_height (drawable));

  if (mask)
    {
      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      x1 = CLAMP (x1, -offset_x, drawable_width (GIMP_DRAWABLE(mask)) - offset_x);
      y1 = CLAMP (y1, -offset_y, drawable_height (GIMP_DRAWABLE(mask)) - offset_y);
      x2 = CLAMP (x2, -offset_x, drawable_width (GIMP_DRAWABLE(mask)) - offset_x);
      y2 = CLAMP (y2, -offset_y, drawable_height (GIMP_DRAWABLE(mask)) - offset_y);
    }

  /*  If the calling procedure specified an undo step...  */
  if (undo)
    drawable_apply_image (drawable, x1, y1, x2, y2, NULL, FALSE);

  /* configure the pixel regions
   *  If an alternative to using the drawable's data as src1 was provided...
   */
  pixel_region_init (&src1PR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);
  pixel_region_resize (src2PR, src2PR->x + (x1 - x), src2PR->y + (y1 - y), (x2 - x1), (y2 - y1));

  if (mask)
    {
      int mx, my;

      /*  configure the mask pixel region
       *  don't use x1 and y1 because they are in layer
       *  coordinate system.  Need mask coordinate system
       */
      mx = x1 + offset_x;
      my = y1 + offset_y;

      pixel_region_init (&mask2PR, drawable_data (GIMP_DRAWABLE(mask)), mx, my, (x2 - x1), (y2 - y1), FALSE);

      tempPR.bytes = 1;
      tempPR.x = 0;
      tempPR.y = 0;
      tempPR.w = x2 - x1;
      tempPR.h = y2 - y1;
      tempPR.rowstride = mask2PR.rowstride;
      temp_data = g_malloc (tempPR.h * tempPR.rowstride);
      tempPR.data = temp_data;

      copy_region (&mask2PR, &tempPR);

      /* apparently, region operations can mutate some PR data. */
      tempPR.x = 0;
      tempPR.y = 0;
      tempPR.w = x2 - x1;
      tempPR.h = y2 - y1;
      tempPR.data = temp_data;

      apply_mask_to_region (&tempPR, maskPR, OPAQUE_OPACITY);

      tempPR.x = 0;
      tempPR.y = 0;
      tempPR.w = x2 - x1;
      tempPR.h = y2 - y1;
      tempPR.data = temp_data;

      combine_regions_replace (&src1PR, src2PR, &destPR, &tempPR, NULL,
		       opacity, active, operation);

      g_free (temp_data);
    }
  else
    combine_regions_replace (&src1PR, src2PR, &destPR, maskPR, NULL,
		     opacity, active, operation);
#endif
}
