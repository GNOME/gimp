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
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "brushes.h"
#include "channels_dialog.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "layers_dialog.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "selection.h"
#include "tools.h"
#include "undo.h"

#define    SQR(x) ((x) * (x))
#define    EPSILON  0.00001

/*  global variables--for use in the various paint tools  */
PaintCore  non_gui_paint_core;

/*  local function prototypes  */
static MaskBuf * paint_core_subsample_mask  (MaskBuf *, double, double);
static MaskBuf * paint_core_solidify_mask   (MaskBuf *);
static MaskBuf * paint_core_get_brush_mask  (PaintCore *, int);
static void      paint_core_paste           (PaintCore *, MaskBuf *, GimpDrawable *, int, int, int, int);
static void      paint_core_replace         (PaintCore *, MaskBuf *, GimpDrawable *, int, int, int);
static void      paint_to_canvas_tiles      (PaintCore *, MaskBuf *, int);
static void      paint_to_canvas_buf        (PaintCore *, MaskBuf *, int);
static void      set_undo_tiles             (GimpDrawable *, int, int, int, int);
static void      set_canvas_tiles           (int, int, int, int);


/***********************************************************************/


/*  undo blocks variables  */
static TileManager *  undo_tiles = NULL;
static TileManager *  canvas_tiles = NULL;


/***********************************************************************/


/*  paint buffers variables  */
static TempBuf *  orig_buf = NULL;
static TempBuf *  canvas_buf = NULL;


/*  brush buffers  */
static MaskBuf *  solid_brush;
static MaskBuf *  kernel_brushes[5][5];


/*  paint buffers utility functions  */
static void        free_paint_buffers     (void);


/***********************************************************************/


#define KERNEL_WIDTH   3
#define KERNEL_HEIGHT  3

/*  Brush pixel subsampling kernels  */
static int subsample[5][5][9] = {
	{
		{ 64, 64, 0, 64, 64, 0, 0, 0, 0, },
		{ 32, 96, 0, 32, 96, 0, 0, 0, 0, },
		{ 0, 128, 0, 0, 128, 0, 0, 0, 0, },
		{ 0, 96, 32, 0, 96, 32, 0, 0, 0, },
		{ 0, 64, 64, 0, 64, 64, 0, 0, 0, },
	},
	{
		{ 32, 32, 0, 96, 96, 0, 0, 0, 0, },
		{ 16, 48, 0, 48, 144, 0, 0, 0, 0, },
		{ 0, 64, 0, 0, 192, 0, 0, 0, 0, },
		{ 0, 48, 16, 0, 144, 48, 0, 0, 0, },
		{ 0, 32, 32, 0, 96, 96, 0, 0, 0, },
	},
	{
		{ 0, 0, 0, 128, 128, 0, 0, 0, 0, },
		{ 0, 0, 0, 64, 192, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 256, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 192, 64, 0, 0, 0, },
		{ 0, 0, 0, 0, 128, 128, 0, 0, 0, },
	},
	{
		{ 0, 0, 0, 96, 96, 0, 32, 32, 0, },
		{ 0, 0, 0, 48, 144, 0, 16, 48, 0, },
		{ 0, 0, 0, 0, 192, 0, 0, 64, 0, },
		{ 0, 0, 0, 0, 144, 48, 0, 48, 16, },
		{ 0, 0, 0, 0, 96, 96, 0, 32, 32, },
	},
	{
		{ 0, 0, 0, 64, 64, 0, 64, 64, 0, },
		{ 0, 0, 0, 32, 96, 0, 32, 96, 0, },
		{ 0, 0, 0, 0, 128, 0, 0, 128, 0, },
		{ 0, 0, 0, 0, 96, 32, 0, 96, 32, },
		{ 0, 0, 0, 0, 64, 64, 0, 64, 64, },
	},
};

void
paint_core_button_press (tool, bevent, gdisp_ptr)
     Tool *tool;
     GdkEventButton *bevent;
     gpointer gdisp_ptr;
{
  PaintCore * paint_core;
  GDisplay * gdisp;
  int draw_line = 0;
  double x, y;
  GimpDrawable *drawable;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore *) tool->private;

  gdisplay_untransform_coords_f (gdisp, (double) bevent->x, (double) bevent->y, &x, &y, TRUE);
  drawable = gimage_active_drawable (gdisp->gimage);

  if (! paint_core_init (paint_core, drawable, x, y))
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
      paint_core_interpolate (paint_core, drawable);
      paint_core->lastx = paint_core->curx;
      paint_core->lasty = paint_core->cury;
    }
  else
    (* paint_core->paint_func) (paint_core, drawable, MOTION_PAINT);

  gdisplay_flush (gdisp);
}

void
paint_core_button_release (tool, bevent, gdisp_ptr)
     Tool *tool;
     GdkEventButton *bevent;
     gpointer gdisp_ptr;
{
  GDisplay * gdisp;
  GImage * gimage;
  PaintCore * paint_core;

  gdisp = (GDisplay *) gdisp_ptr;
  gimage = gdisp->gimage;
  paint_core = (PaintCore *) tool->private;

  /*  resume the current selection and ungrab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage->ID, SelectionResume);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  /*  Let the specific painting function finish up  */
  (* paint_core->paint_func) (paint_core, gimage_active_drawable (gdisp->gimage), FINISH_PAINT);

  /*  Set tool state to inactive -- no longer painting */
  tool->state = INACTIVE;

  paint_core_finish (paint_core, gimage_active_drawable (gdisp->gimage), tool->ID);
  gdisplays_flush ();
}

void
paint_core_motion (tool, mevent, gdisp_ptr)
     Tool *tool;
     GdkEventMotion *mevent;
     gpointer gdisp_ptr;
{
  GDisplay * gdisp;
  PaintCore * paint_core;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore *) tool->private;

  gdisplay_untransform_coords_f (gdisp, (double) mevent->x, (double) mevent->y,
				 &paint_core->curx, &paint_core->cury, TRUE);
  paint_core->state = mevent->state;

  paint_core_interpolate (paint_core, gimage_active_drawable (gdisp->gimage));

  gdisplay_flush (gdisp);

  paint_core->lastx = paint_core->curx;
  paint_core->lasty = paint_core->cury;
}

void
paint_core_cursor_update (tool, mevent, gdisp_ptr)
     Tool *tool;
     GdkEventMotion *mevent;
     gpointer gdisp_ptr;
{
  GDisplay *gdisp;
  Layer *layer;
  GdkCursorType ctype = GDK_TOP_LEFT_ARROW;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, FALSE);
  if ((layer = gimage_get_active_layer (gdisp->gimage))) 
    {
      int off_x, off_y;
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
    if (x >= off_x && y >= off_y &&
	x < (off_x + drawable_width (GIMP_DRAWABLE(layer))) &&
	y < (off_y + drawable_height (GIMP_DRAWABLE(layer))))
      {
	/*  One more test--is there a selected region?
	 *  if so, is cursor inside?
	 */
	if (gimage_mask_is_empty (gdisp->gimage))
	  ctype = GDK_PENCIL;
	else if (gimage_mask_value (gdisp->gimage, x, y))
	  ctype = GDK_PENCIL;
      }
    }
  gdisplay_install_tool_cursor (gdisp, ctype);
}

void
paint_core_control (tool, action, gdisp_ptr)
     Tool * tool;
     int action;
     gpointer gdisp_ptr;
{
  PaintCore * paint_core;
  GDisplay *gdisp;
  GimpDrawable *drawable;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore *) tool->private;
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
      paint_core_cleanup ();
      break;
    }
}

void
paint_core_no_draw (tool)
     Tool * tool;
{
  return;
}

Tool *
paint_core_new (type)
     int type;
{
  Tool * tool;
  PaintCore * private;

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (PaintCore *) g_malloc (sizeof (PaintCore));

  private->core = draw_core_new (paint_core_no_draw);

  tool->type = type;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->gdisp_ptr = NULL;
  tool->private = (void *) private;
  tool->preserve = TRUE;

  tool->button_press_func = paint_core_button_press;
  tool->button_release_func = paint_core_button_release;
  tool->motion_func = paint_core_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = paint_core_cursor_update;
  tool->control_func = paint_core_control;

  return tool;
}

void
paint_core_free (tool)
     Tool * tool;
{
  PaintCore * paint_core;

  paint_core = (PaintCore *) tool->private;

  /*  Make sure the selection core is not visible  */
  if (tool->state == ACTIVE && paint_core->core)
    draw_core_stop (paint_core->core, tool);

  /*  Free the selection core  */
  if (paint_core->core)
    draw_core_free (paint_core->core);

  /*  Cleanup memory  */
  paint_core_cleanup ();

  /*  Free the paint core  */
  g_free (paint_core);
}

int
paint_core_init (paint_core, drawable, x, y)
     PaintCore *paint_core;
     GimpDrawable *drawable;
     double x, y;
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
    (double) MAXIMUM (brush->mask->width, brush->mask->height) *
    ((double) get_brush_spacing () / 100.0);
  if (paint_core->spacing < 1.0)
    paint_core->spacing = 1.0;
  paint_core->brush_mask = brush->mask;

  /*  free the block structures  */
  if (undo_tiles)
    tile_manager_destroy (undo_tiles);
  if (canvas_tiles)
    tile_manager_destroy (canvas_tiles);

  /*  Allocate the undo structure  */
  undo_tiles = tile_manager_new (drawable_width (drawable),
				 drawable_height (drawable),
				 drawable_bytes (drawable));

  /*  Allocate the canvas blocks structure  */
  canvas_tiles = tile_manager_new (drawable_width (drawable),
				   drawable_height (drawable), 1);

  /*  Get the initial undo extents  */
  paint_core->x1 = paint_core->x2 = paint_core->curx;
  paint_core->y1 = paint_core->y2 = paint_core->cury;
  paint_core->distance = 0.0;

  return TRUE;
}

void
paint_core_interpolate (paint_core, drawable)
     PaintCore *paint_core;
     GimpDrawable *drawable;
{
  int n;
  double dx, dy;
  double left;
  double t;
  double initial;
  double dist;
  double total;

  dx = paint_core->curx - paint_core->lastx;
  dy = paint_core->cury - paint_core->lasty;

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
paint_core_finish (paint_core, drawable, tool_id)
     PaintCore *paint_core;
     GimpDrawable *drawable;
     int tool_id;
{
  GImage *gimage;
  PaintUndo *pu;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((paint_core->x2 == paint_core->x1) || (paint_core->y2 == paint_core->y1))
    return;

  undo_push_group_start (gimage, PAINT_CORE_UNDO);

  pu = (PaintUndo *) g_malloc (sizeof (PaintUndo));
  pu->tool_ID = tool_id;
  pu->lastx = paint_core->startx;
  pu->lasty = paint_core->starty;

  /*  Push a paint undo  */
  undo_push_paint (gimage, pu);

  /*  push an undo  */
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
paint_core_cleanup ()
{
  /*  CLEANUP  */
  /*  If the undo tiles exist, nuke them  */
  if (undo_tiles)
    {
      tile_manager_destroy (undo_tiles);
      undo_tiles = NULL;
    }

  /*  If the canvas blocks exist, nuke them  */
  if (canvas_tiles)
    {
      tile_manager_destroy (canvas_tiles);
      canvas_tiles = NULL;
    }

  /*  Free the temporary buffers if they exist  */
  free_paint_buffers ();
}


/************************/
/*  Painting functions  */
/************************/

TempBuf *
paint_core_get_paint_area (paint_core, drawable)
     PaintCore *paint_core;
     GimpDrawable *drawable;
{
  int x, y;
  int x1, y1, x2, y2;
  int bytes;

  bytes = drawable_has_alpha (drawable) ?
    drawable_bytes (drawable) : drawable_bytes (drawable) + 1;

  /*  adjust the x and y coordinates to the upper left corner of the brush  */
  x = (int) paint_core->curx - (paint_core->brush_mask->width >> 1);
  y = (int) paint_core->cury - (paint_core->brush_mask->height >> 1);

  x1 = BOUNDS (x - 1, 0, drawable_width (drawable));
  y1 = BOUNDS (y - 1, 0, drawable_height (drawable));
  x2 = BOUNDS (x + paint_core->brush_mask->width + 1,
	       0, drawable_width (drawable));
  y2 = BOUNDS (y + paint_core->brush_mask->height + 1,
	       0, drawable_height (drawable));

  /*  configure the canvas buffer  */
  if ((x2 - x1) && (y2 - y1))
    canvas_buf = temp_buf_resize (canvas_buf, bytes, x1, y1,
				  (x2 - x1), (y2 - y1));
  else
    return NULL;

  return canvas_buf;
}

TempBuf *
paint_core_get_orig_image (paint_core, drawable, x1, y1, x2, y2)
     PaintCore *paint_core;
     GimpDrawable *drawable;
     int x1, y1;
     int x2, y2;
{
  PixelRegion srcPR, destPR;
  Tile *undo_tile;
  int h;
  int pixelwidth;
  int refd;
  unsigned char * s, * d;
  void * pr;

  orig_buf = temp_buf_resize (orig_buf, drawable_bytes (drawable),
			      x1, y1, (x2 - x1), (y2 - y1));
  x1 = BOUNDS (x1, 0, drawable_width (drawable));
  y1 = BOUNDS (y1, 0, drawable_height (drawable));
  x2 = BOUNDS (x2, 0, drawable_width (drawable));
  y2 = BOUNDS (y2, 0, drawable_height (drawable));

  /*  configure the pixel regions  */
  pixel_region_init (&srcPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
  destPR.bytes = orig_buf->bytes;
  destPR.x = 0; destPR.y = 0;
  destPR.w = (x2 - x1); destPR.h = (y2 - y1);
  destPR.rowstride = orig_buf->bytes * orig_buf->width;
  destPR.data = temp_buf_data (orig_buf) +
    (y1 - orig_buf->y) * destPR.rowstride + (x1 - orig_buf->x) * destPR.bytes;

  for (pr = pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      /*  If the undo tile corresponding to this location is valid, use it  */
      undo_tile = tile_manager_get_tile (undo_tiles, srcPR.x, srcPR.y, 0);
      if (undo_tile->valid == TRUE)
	{
	  tile_ref (undo_tile);
	  s = undo_tile->data + srcPR.rowstride * (srcPR.y % TILE_HEIGHT) +
	    srcPR.bytes * (srcPR.x % TILE_WIDTH);
	  refd = TRUE;
	}
      else
	{
	  s = srcPR.data;
	  refd = FALSE;
	}

      d = destPR.data;
      pixelwidth = srcPR.w * srcPR.bytes;
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

  return orig_buf;
}

void
paint_core_paste_canvas (paint_core, drawable, brush_opacity, image_opacity, paint_mode,
			 brush_hardness, mode)
     PaintCore *paint_core;
     GimpDrawable *drawable;
     int brush_opacity;
     int image_opacity;
     int paint_mode;
     int brush_hardness;
     int mode;
{
  MaskBuf *brush_mask;

  /*  get the brush mask  */
  brush_mask = paint_core_get_brush_mask (paint_core, brush_hardness);

  /*  paste the canvas buf  */
  paint_core_paste (paint_core, brush_mask, drawable,
		    brush_opacity, image_opacity, paint_mode, mode);
}

/* Similar to paint_core_paste_canvas, but replaces the alpha channel
   rather than using it to composite (i.e. transparent over opaque
   becomes transparent rather than opauqe. */
void
paint_core_replace_canvas (paint_core, drawable, brush_opacity, image_opacity,
			   brush_hardness, mode)
     PaintCore *paint_core;
     GimpDrawable *drawable;
     int brush_opacity;
     int image_opacity;
     int brush_hardness;
     int mode;
{
  MaskBuf *brush_mask;

  /*  get the brush mask  */
  brush_mask = paint_core_get_brush_mask (paint_core, brush_hardness);

  /*  paste the canvas buf  */
  paint_core_replace (paint_core, brush_mask, drawable,
		      brush_opacity, image_opacity, mode);
}

/************************************************************
 *             LOCAL FUNCTION DEFINITIONS                   *
 ************************************************************/

static MaskBuf *
paint_core_subsample_mask (mask, x, y)
     MaskBuf * mask;
     double x, y;
{
  static MaskBuf *last_brush = NULL;
  MaskBuf * dest;
  double left;
  unsigned char * m, * d;
  int * k;
  int index1, index2;
  int * kernel;
  int new_val;
  int i, j;
  int r, s;

  x += (x < 0) ? mask->width : 0;
  left = x - floor(x) + 0.125;
  index1 = (int) (left * 4);

  y += (y < 0) ? mask->height : 0;
  left = y - floor(y) + 0.125;
  index2 = (int) (left * 4);

  kernel = subsample[index2][index1];

  if ((mask == last_brush) && kernel_brushes[index2][index1])
    return kernel_brushes[index2][index1];
  else if (mask != last_brush)
    for (i = 0; i < 5; i++)
      for (j = 0; j < 5; j++)
	{
	  if (kernel_brushes[i][j])
	    mask_buf_free (kernel_brushes[i][j]);
	  kernel_brushes[i][j] = NULL;
	}

  last_brush = mask;
  kernel_brushes[index2][index1] = mask_buf_new (mask->width + 2, mask->height + 2);
  dest = kernel_brushes[index2][index1];

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

  return dest;
}

static MaskBuf *
paint_core_solidify_mask (brush_mask)
     MaskBuf * brush_mask;
{
  static MaskBuf *last_brush = NULL;
  int i, j;
  unsigned char * data, * src;

  if (brush_mask == last_brush)
    return solid_brush;

  last_brush = brush_mask;
  if (solid_brush)
    mask_buf_free (solid_brush);
  solid_brush = mask_buf_new (brush_mask->width + 2, brush_mask->height + 2);

  /*  get the data and advance one line into it  */
  data = mask_buf_data (solid_brush) + solid_brush->width;
  src = mask_buf_data (brush_mask);

  for (i = 0; i < brush_mask->height; i++)
    {
      data++;
      for (j = 0; j < brush_mask->width; j++)
	{
	  *data++ = (*src++) ? OPAQUE_OPACITY : TRANSPARENT_OPACITY;
	}
      data++;
    }

  return solid_brush;
}

static MaskBuf *
paint_core_get_brush_mask (paint_core, brush_hardness)
     PaintCore * paint_core;
     int brush_hardness;
{
  MaskBuf * bm;

  switch (brush_hardness)
    {
    case SOFT:
      bm = paint_core_subsample_mask (paint_core->brush_mask, paint_core->curx, paint_core->cury);
      break;
    case HARD:
      bm = paint_core_solidify_mask (paint_core->brush_mask);
      break;
    default:
      bm = NULL;
      break;
    }

  return bm;
}

static void
paint_core_paste (paint_core, brush_mask, drawable, brush_opacity, image_opacity, paint_mode, mode)
     PaintCore *paint_core;
     MaskBuf *brush_mask;
     GimpDrawable *drawable;
     int brush_opacity;
     int image_opacity;
     int paint_mode;
     int mode;
{
  GImage *gimage;
  PixelRegion srcPR;
  TileManager *alt = NULL;
  int offx, offy;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /*  set undo blocks  */
  set_undo_tiles (drawable,
		  canvas_buf->x, canvas_buf->y,
		  canvas_buf->width, canvas_buf->height);

  /*  If the mode is CONSTANT:
   *   combine the canvas buf, the brush mask to the canvas tiles
   */
  if (mode == CONSTANT)
    {
      /*  initialize any invalid canvas tiles  */
      set_canvas_tiles (canvas_buf->x, canvas_buf->y,
			canvas_buf->width, canvas_buf->height);

      paint_to_canvas_tiles (paint_core, brush_mask, brush_opacity);
      alt = undo_tiles;
    }
  /*  Otherwise:
   *   combine the canvas buf and the brush mask to the canvas buf
   */
  else  /*  mode != CONSTANT  */
    paint_to_canvas_buf (paint_core, brush_mask, brush_opacity);

  /*  intialize canvas buf source pixel regions  */
  srcPR.bytes = canvas_buf->bytes;
  srcPR.x = 0; srcPR.y = 0;
  srcPR.w = canvas_buf->width;
  srcPR.h = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data = temp_buf_data (canvas_buf);

  /*  apply the paint area to the gimage  */
  gimage_apply_image (gimage, drawable, &srcPR,
		      FALSE, image_opacity, paint_mode,
		      alt,  /*  specify an alternative src1  */
		      canvas_buf->x, canvas_buf->y);

  /*  Update the undo extents  */
  paint_core->x1 = MINIMUM (paint_core->x1, canvas_buf->x);
  paint_core->y1 = MINIMUM (paint_core->y1, canvas_buf->y);
  paint_core->x2 = MAXIMUM (paint_core->x2, (canvas_buf->x + canvas_buf->width));
  paint_core->y2 = MAXIMUM (paint_core->y2, (canvas_buf->y + canvas_buf->height));

  /*  Update the gimage--it is important to call gdisplays_update_area
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  drawable_offsets (drawable, &offx, &offy);
  gdisplays_update_area (gimage->ID, canvas_buf->x + offx, canvas_buf->y + offy,
			 canvas_buf->width, canvas_buf->height);
}

/* This works similarly to paint_core_paste. However, instead of combining
   the canvas to the paint core drawable using one of the combination
   modes, it uses a "replace" mode (i.e. transparent pixels in the 
   canvas erase the paint core drawable).

   When not drawing on alpha-enabled images, it just paints using NORMAL
   mode.
*/
static void
paint_core_replace (paint_core, brush_mask, drawable, brush_opacity, image_opacity, mode)
     PaintCore *paint_core;
     MaskBuf *brush_mask;
     GimpDrawable *drawable;
     int brush_opacity;
     int image_opacity;
     int mode;
{
  GImage *gimage;
  PixelRegion srcPR, maskPR;
  int offx, offy;

  if (!drawable_has_alpha (drawable))
    {
      paint_core_paste (paint_core, brush_mask, drawable,
			brush_opacity, image_opacity, NORMAL_MODE,
			mode);
      return;
    }

  if (mode != INCREMENTAL)
    {
      g_warning ("paint_core_replace only works in INCREMENTAL mode");
      return;
    }

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /*  set undo blocks  */
  set_undo_tiles (drawable,
		  canvas_buf->x, canvas_buf->y,
		  canvas_buf->width, canvas_buf->height);

  maskPR.bytes = 1;
  maskPR.x = 0; maskPR.y = 0;
  maskPR.w = canvas_buf->width;
  maskPR.h = canvas_buf->height;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data = mask_buf_data (brush_mask);
  
  /*  intialize canvas buf source pixel regions  */
  srcPR.bytes = canvas_buf->bytes;
  srcPR.x = 0; srcPR.y = 0;
  srcPR.w = canvas_buf->width;
  srcPR.h = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data = temp_buf_data (canvas_buf);

  /*  apply the paint area to the gimage  */
  gimage_replace_image (gimage, drawable, &srcPR,
			FALSE, image_opacity,
			&maskPR,
			canvas_buf->x, canvas_buf->y);

  /*  Update the undo extents  */
  paint_core->x1 = MINIMUM (paint_core->x1, canvas_buf->x);
  paint_core->y1 = MINIMUM (paint_core->y1, canvas_buf->y);
  paint_core->x2 = MAXIMUM (paint_core->x2, (canvas_buf->x + canvas_buf->width));
  paint_core->y2 = MAXIMUM (paint_core->y2, (canvas_buf->y + canvas_buf->height));

  /*  Update the gimage--it is important to call gdisplays_update_area
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  drawable_offsets (drawable, &offx, &offy);
  gdisplays_update_area (gimage->ID, canvas_buf->x + offx, canvas_buf->y + offy,
			 canvas_buf->width, canvas_buf->height);
}

static void
paint_to_canvas_tiles (paint_core, brush_mask, brush_opacity)
     PaintCore *paint_core;
     MaskBuf *brush_mask;
     int brush_opacity;
{
  PixelRegion srcPR, maskPR;
  int x, y;
  int xoff, yoff;

  /*   combine the brush mask and the canvas tiles  */
  pixel_region_init (&srcPR, canvas_tiles,
		     canvas_buf->x, canvas_buf->y,
		     canvas_buf->width, canvas_buf->height, TRUE);

  x = (int) paint_core->curx - (brush_mask->width >> 1);
  y = (int) paint_core->cury - (brush_mask->height >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;

  maskPR.bytes = 1;
  maskPR.x = 0; maskPR.y = 0;
  maskPR.w = srcPR.w;
  maskPR.h = srcPR.h;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data = mask_buf_data (brush_mask) + yoff * maskPR.rowstride + xoff * maskPR.bytes;

  /*  combine the mask and canvas tiles  */
  combine_mask_and_region (&srcPR, &maskPR, brush_opacity);

  /*  combine the canvas tiles and the canvas buf  */
  srcPR.bytes = canvas_buf->bytes;
  srcPR.x = 0; srcPR.y = 0;
  srcPR.w = canvas_buf->width;
  srcPR.h = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data = temp_buf_data (canvas_buf);

  pixel_region_init (&maskPR, canvas_tiles,
		     canvas_buf->x, canvas_buf->y,
		     canvas_buf->width, canvas_buf->height, FALSE);

  /*  apply the canvas tiles to the canvas buf  */
  apply_mask_to_region (&srcPR, &maskPR, OPAQUE_OPACITY);
}

static void
paint_to_canvas_buf (paint_core, brush_mask, brush_opacity)
     PaintCore *paint_core;
     MaskBuf *brush_mask;
     int brush_opacity;
{
  PixelRegion srcPR, maskPR;
  int x, y;
  int xoff, yoff;

  x = (int) paint_core->curx - (brush_mask->width >> 1);
  y = (int) paint_core->cury - (brush_mask->height >> 1);
  xoff = (x < 0) ? -x : 0;
  yoff = (y < 0) ? -y : 0;


  /*  combine the canvas buf and the brush mask to the canvas buf  */
  srcPR.bytes = canvas_buf->bytes;
  srcPR.x = 0; srcPR.y = 0;
  srcPR.w = canvas_buf->width;
  srcPR.h = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data = temp_buf_data (canvas_buf);

  maskPR.bytes = 1;
  maskPR.x = 0; maskPR.y = 0;
  maskPR.w = srcPR.w;
  maskPR.h = srcPR.h;
  maskPR.rowstride = maskPR.bytes * brush_mask->width;
  maskPR.data = mask_buf_data (brush_mask) + yoff * maskPR.rowstride + xoff * maskPR.bytes;

  /*  apply the mask  */
  apply_mask_to_region (&srcPR, &maskPR, brush_opacity);
}

static void
set_undo_tiles (drawable, x, y, w, h)
     GimpDrawable *drawable;
     int x, y;
     int w, h;
{
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
}

static void
set_canvas_tiles (x, y, w, h)
     int x, y;
     int w, h;
{
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
}


/*****************************************************/
/*  Paint buffers utility functions                  */
/*****************************************************/


static void
free_paint_buffers ()
{
  if (orig_buf)
    temp_buf_free (orig_buf);
  orig_buf = NULL;

  if (canvas_buf)
    temp_buf_free (canvas_buf);
  canvas_buf = NULL;
}
