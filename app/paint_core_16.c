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
#include <stdarg.h>

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

/* for brush_mask derefs */
#include "temp_buf.h"

#define    SQR(x) ((x) * (x))

/*  global variables--for use in the various paint tools  */
PaintCore16  non_gui_paint_core_16;

/*  local function prototypes  */
static void      paint_core_16_button_press    (Tool *, GdkEventButton *, gpointer);
static void      paint_core_16_button_release  (Tool *, GdkEventButton *, gpointer);
static void      paint_core_16_motion          (Tool *, GdkEventMotion *, gpointer);
static void      paint_core_16_cursor_update   (Tool *, GdkEventMotion *, gpointer);
static void      paint_core_16_control         (Tool *, int, gpointer);
static void      paint_core_16_no_draw         (Tool *);


static void      painthit_init                 (GimpDrawable *, int, int, int, int);
static void      painthit_create_constant      (PaintCore16 *, Canvas *, gfloat);
static void      painthit_create_incremental   (PaintCore16 *, Canvas *, gfloat);
static void      painthit_apply                (PaintCore16 *, Canvas *, gfloat,
                                                GimpDrawable *, int);
static void      painthit_replace              (PaintCore16 *, Canvas *, gfloat,
                                                GimpDrawable *, gfloat);
static void      painthit_finish               (GimpDrawable *, PaintCore16 *,
                                                Canvas *);


static Canvas *  brush_mask_get                (PaintCore16 *, int);
static Canvas *  brush_mask_subsample          (Canvas *, double, double);
static Canvas *  brush_mask_solidify           (Canvas *);

static void brush_solidify_mask_float ( Canvas *, Canvas *);
static void brush_solidify_mask_u8 ( Canvas *, Canvas *);
static void brush_solidify_mask_u16 ( Canvas *, Canvas *);


/* the portions of the original image which have been modified */
static Canvas *  undo_tiles = NULL;

/* a mask holding the cumulative brush stroke */
static Canvas *  canvas_tiles = NULL;

/* the paint hit to mask and apply */
static Canvas *  canvas_buf = NULL;



/* temporary */
static char indent[] = "                                                 ";
static int level = 48;

void 
trace_enter  (
              char * func
              )
{
  printf ("%s%s () {\n", indent+level, func);
  level -= 2;
}


void 
trace_exit  (
             void
             )
{
  level += 2;
  printf ("%s}\n", indent+level);
}


void 
trace_begin  (
              char * format,
              ...
              )
{
  va_list args, args2;
  char *buf;

  va_start (args, format);
  va_start (args2, format);
  buf = g_vsprintf (format, &args, &args2);
  va_end (args);
  va_end (args2);

  fputs (indent+level, stdout);
  fputs (buf, stdout);
  fputs (": {\n", stdout);

  level -= 2;
}


void 
trace_end  (
            void
            )
{
  level += 2;
  printf ("%s}\n", indent+level);
}


void 
trace_printf  (
               char * format,
               ...
               )
{
  va_list args, args2;
  char *buf;

  va_start (args, format);
  va_start (args2, format);
  buf = g_vsprintf (format, &args, &args2);
  va_end (args);
  va_end (args2);

  fputs (indent+level, stdout);
  fputs (buf, stdout);
  fputc ('\n', stdout);
}


  

/* ------------------------------------------------------------------------

   PaintCore Frontend

*/
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

  /* get the brush mask */
  if (!(brush = get_active_brush ()))
    {
      warning ("No brushes available for use with this tool.");
      return FALSE;
    }
 
 #define PAINT_CORE_16_C_2_cw 
  paint_core->spacing =
    (double) MAX (canvas_height (brush->mask_canvas), canvas_width (brush->mask_canvas)) *
    ((double) get_brush_spacing () / 100.0);

  if (paint_core->spacing < 1.0)
    paint_core->spacing = 1.0;
  
  paint_core->brush_mask = brush->mask_canvas;

  /*  Allocate the undo structure  */
  if (undo_tiles)
    canvas_delete (undo_tiles);
  undo_tiles = canvas_new (drawable_tag (drawable),
                           drawable_width (drawable),
                           drawable_height (drawable),
                           TILING_NEVER);


  /*  Allocate the cumulative brush stroke mask  */
  if (canvas_tiles)
    canvas_delete (canvas_tiles);
  canvas_tiles = canvas_new (tag_new (tag_precision (drawable_tag (drawable)),
                                      FORMAT_GRAY,
                                      ALPHA_NO),
                             drawable_width (drawable),
                             drawable_height (drawable),
                             TILING_NEVER);

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
  {
    TileManager * tm = canvas_to_tm (undo_tiles);
    drawable_apply_image (drawable, paint_core->x1, paint_core->y1,
                          paint_core->x2, paint_core->y2, tm, TRUE);
    /* temp deleteion until drawable_apply_image handles Canvas directly */
    canvas_delete (undo_tiles);
    undo_tiles = NULL;
  }

  /*  push the group end  */
  undo_push_group_end (gimage);

  /*  invalidate the drawable--have to do it here, because
   *  it is not done during the actual painting.
   */
  drawable_invalidate_preview (drawable);
#define PAINT_CORE_16_3_cw
  /*canvas_delete (paint_core->brush_mask);*/
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


static void 
paint_core_16_button_press  (
                             Tool * tool,
                             GdkEventButton * bevent,
                             gpointer gdisp_ptr
                             )
{
  PaintCore16 * paint_core;
  GDisplay * gdisp;
  GimpDrawable * drawable;
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
#define PAINT_CORE_16_C_1_cw
#if 0
  /* add motion memory if you press mod1 first */
  if (bevent->state & GDK_MOD1_MASK)
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
  else
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
#endif 
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


static void 
paint_core_16_control  (
                        Tool * tool,
                        int action,
                        gpointer gdisp_ptr
                        )
{
  PaintCore16 * paint_core;
  GDisplay *gdisp;
  GimpDrawable * drawable;

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













/* ------------------------------------------------------------------------

   PaintCore Backend

*/
Canvas * 
paint_core_16_area  (
                     PaintCore16 * paint_core,
                     GimpDrawable * drawable
                     )
{
  Tag   tag;
  int   x, y;
  int   dw, dh;
  int   bw, bh;
  int   x1, y1, x2, y2;
  

  bw = canvas_width (paint_core->brush_mask);
  bh = canvas_height (paint_core->brush_mask);
  
  /* adjust the x and y coordinates to the upper left corner of the brush */  
  x = (int) paint_core->curx - bw / 2;
  y = (int) paint_core->cury - bh / 2;
  
  dw = drawable_width (drawable);
  dh = drawable_height (drawable);

#define PAINT_CORE_16_C_4_cw  
#ifdef BRUSH_WITH_BORDER 
  x1 = CLAMP (x - 1 , 0, dw);
  y1 = CLAMP (y - 1, 0, dh);
  x2 = CLAMP (x + bw + 1, 0, dw);
  y2 = CLAMP (y + bh + 1, 0, dh);
#else  
  x1 = CLAMP (x , 0, dw);
  y1 = CLAMP (y , 0, dh);
  x2 = CLAMP (x + bw , 0, dw);
  y2 = CLAMP (y + bh , 0, dh);
#endif
  /* save the boundaries of this paint hit */
  paint_core->x = x1;
  paint_core->y = y1;
  paint_core->w = x2 - x1;
  paint_core->h = y2 - y1;
  
  /* configure the canvas buffer */
  tag = tag_set_alpha (drawable_tag (drawable), ALPHA_YES);

  if (canvas_buf)
    canvas_delete (canvas_buf);
  canvas_buf = canvas_new (tag, (x2 - x1), (y2 - y1), TILING_NEVER);
  
  return canvas_buf;
}



Canvas * 
paint_core_16_area_original  (
                              PaintCore16 * paint_core,
                              GimpDrawable *drawable,
                              int x1,
                              int y1,
                              int x2,
                              int y2
                              )
{
  static Canvas * orig_buf = NULL;
  Tag   tag;

  /*  configure the buffer  */
  tag = tag_set_alpha (drawable_tag (drawable), ALPHA_YES);
  if (orig_buf)
    canvas_delete (orig_buf);
  orig_buf = canvas_new (tag, (x2 - x1), (y2 - y1), TILING_NEVER);
  
  
  x1 = CLAMP (x1, 0, drawable_width (drawable));
  y1 = CLAMP (y1, 0, drawable_height (drawable));
  x2 = CLAMP (x2, 0, drawable_width (drawable));
  y2 = CLAMP (y2, 0, drawable_height (drawable));

  g_warning ("finish writing paint_core_16_area_original()");
  
#if 0
  PixelArea srcPR, destPR;
  Tag   tag;
  Tile *undo_tile;
  int h;
  int pixelwidth;
  int refd;
  unsigned char * s, * d;
  void * pr;

  /*  configure the pixel regions  */
  pixelarea_init (&srcPR, drawable_data (drawable), NULL, x1, y1, (x2 - x1), (y2 - y1), FALSE);
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


void 
paint_core_16_area_paste  (
                           PaintCore16 * paint_core,
                           GimpDrawable * drawable,
                           gfloat brush_opacity,
                           gfloat image_opacity,
                           BrushHardness brush_hardness,
                           ApplyMode apply_mode,
                           int paint_mode
                           )
{
  Canvas *brush_mask;

  if (! drawable_gimage (drawable))
    return;
  
  brush_mask = brush_mask_get (paint_core, brush_hardness);
  
  painthit_init (drawable,
                 paint_core->x, paint_core->y,
                 canvas_width (canvas_buf), canvas_height (canvas_buf));
  switch (apply_mode)
    {
    case CONSTANT:
      painthit_create_constant (paint_core, brush_mask, brush_opacity);
      painthit_apply (paint_core, undo_tiles, image_opacity, drawable, paint_mode);
      break;
    case INCREMENTAL: /* convolve tool */
      painthit_create_incremental (paint_core, brush_mask, brush_opacity);
      painthit_apply (paint_core, NULL, image_opacity, drawable, paint_mode);
      break;
    }
  
  painthit_finish (drawable, paint_core, canvas_buf);
}


void
paint_core_16_area_replace  (
                             PaintCore16 * paint_core,
                             GimpDrawable *drawable,
                             gfloat brush_opacity,
                             gfloat image_opacity,
                             BrushHardness brush_hardness,
                             ApplyMode apply_mode
                             )
{
  Canvas *brush_mask;

  if (! drawable_gimage (drawable))
    return;
  
  if (! drawable_has_alpha (drawable))
    {
      paint_core_16_area_paste (paint_core, drawable,
                                brush_opacity, image_opacity,
                                brush_hardness, apply_mode, NORMAL_MODE);
      return;
    }
  
  if (apply_mode != INCREMENTAL)
    return;

  brush_mask = brush_mask_get (paint_core, brush_hardness);
  
  painthit_init (drawable,
                 paint_core->x, paint_core->y,
                 canvas_width (canvas_buf), canvas_height (canvas_buf));
  
  painthit_replace (paint_core, brush_mask, brush_opacity,
                    drawable, image_opacity);
  
  painthit_finish (drawable, paint_core, canvas_buf);
}


static void 
painthit_init  (
                GimpDrawable *drawable,
                int x,
                int y,
                int w,
                int h
                )
{
  PixelArea a;
  Canvas *c;

  /* initialize the undo tiles by using the drawable data as the
     pixelarea initializer and touching each tile */
  c = drawable_data_canvas (drawable);
  pixelarea_init (&a, undo_tiles, c, x, y, w, h, TRUE);
  {
    void * pag;
    for (pag = pixelarea_register (1, &a);
         pag != NULL;
         pag = pixelarea_process (pag))
      {
        /* do nothing */
      }
  }
}


static void
painthit_create_constant (
                          PaintCore16 * paint_core,
                          Canvas * brush_mask,
                          gfloat brush_opacity 
                          )
{
  PixelArea srcPR, maskPR;

  /*  combine the mask and canvas tiles  */
  {
    int xoff, yoff;
    int x, y;
      
    x = (int) paint_core->curx - (canvas_width (brush_mask) >> 1);
    y = (int) paint_core->cury - (canvas_height (brush_mask) >> 1);
    xoff = (x < 0) ? -x : 0;
    yoff = (y < 0) ? -y : 0;
    
    pixelarea_init (&srcPR, canvas_tiles, NULL,
                    paint_core->x, paint_core->y,
                    canvas_width (canvas_buf), canvas_height (canvas_buf),
                    TRUE);      
    pixelarea_init (&maskPR, brush_mask, NULL,
                    xoff, yoff,
                    canvas_width (brush_mask), canvas_height (brush_mask),
                    TRUE);
    {
      static Paint * p;
      if (p == NULL)
        {
#ifdef U8_SUPPORT
          p = paint_new (tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_NO),
                         NULL);
#elif U16_SUPPORT
          p = paint_new (tag_new (PRECISION_U16, FORMAT_GRAY, ALPHA_NO),
                         NULL);
#elif FLOAT_SUPPORT
          p = paint_new (tag_new (PRECISION_FLOAT, FORMAT_GRAY, ALPHA_NO),
                         NULL);
#endif
        }
      paint_load (p,
                  tag_new (PRECISION_FLOAT, FORMAT_GRAY, ALPHA_NO),
                  &brush_opacity);
      combine_mask_and_area (&srcPR, &maskPR, p);
    }
  }
      
  /*  apply the canvas tiles to the canvas buf  */
  pixelarea_init (&srcPR, canvas_buf, NULL,
                  0, 0,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  TRUE);
  pixelarea_init (&maskPR, canvas_tiles, NULL,
                  paint_core->x, paint_core->y,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  FALSE);      
  {
    static Paint * p;
    if (p == NULL)
      {

#ifdef U8_SUPPORT
        p = paint_new (tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_NO),
                       NULL);
#elif U16_SUPPORT
          p = paint_new (tag_new (PRECISION_U16, FORMAT_GRAY, ALPHA_NO),
                         NULL);
#elif FLOAT_SUPPORT
          p = paint_new (tag_new (PRECISION_FLOAT, FORMAT_GRAY, ALPHA_NO),
                         NULL);
#endif
        {
          static gfloat  d[3] = {1.0, 1.0, 1.0};
          paint_load (p,
                      tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                      (void *) &d);
        }
      }
    apply_mask_to_area (&srcPR, &maskPR, p);
  }
}


static void
painthit_create_incremental (
                             PaintCore16 * paint_core,
                             Canvas * brush_mask,
                             gfloat brush_opacity 
                             )
{
  PixelArea srcPR, maskPR;

  
  /*  combine the canvas buf and the brush mask to the canvas buf  */
  pixelarea_init (&srcPR, canvas_buf, NULL,
                  0, 0,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  TRUE);
  pixelarea_init (&maskPR, brush_mask, NULL,
                  0, 0,
                  canvas_width (brush_mask), canvas_height (brush_mask),
                  FALSE);
  {
    static Paint * p;
    if (p == NULL)
      {
#ifdef U8_SUPPORT
        p = paint_new (tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_NO),
                       NULL);
#elif U16_SUPPORT
          p = paint_new (tag_new (PRECISION_U16, FORMAT_GRAY, ALPHA_NO),
                         NULL);
#elif FLOAT_SUPPORT
          p = paint_new (tag_new (PRECISION_FLOAT, FORMAT_GRAY, ALPHA_NO),
                         NULL);
#endif
      }
    paint_load (p,
                tag_new (PRECISION_FLOAT, FORMAT_GRAY, ALPHA_NO),
                &brush_opacity);
    apply_mask_to_area (&srcPR, &maskPR, p);
  }
}
  


static void
painthit_apply (
                PaintCore16 * paint_core,
                Canvas * orig_tiles,
                gfloat image_opacity,
                GimpDrawable * drawable,
                int paint_mode
                )
{
  GImage * gimage = drawable_gimage (drawable);
  PixelArea srcPR;
  
  pixelarea_init (&srcPR, canvas_buf, NULL,
                  0, 0,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  TRUE);

  gimage_apply_painthit (gimage, drawable, &srcPR,
                         FALSE, image_opacity, paint_mode,
                         orig_tiles, paint_core->x, paint_core->y);
}



static void
painthit_replace (
                  PaintCore16 * paint_core,
                  Canvas * brush_mask,
                  gfloat brush_opacity,
                  GimpDrawable * drawable,
                  gfloat image_opacity
                  )
{
  PixelArea srcPR, maskPR;
  GImage * gimage = drawable_gimage (drawable);
  
  /* apply the paint hit directly to the gimage */
  pixelarea_init (&maskPR, brush_mask, NULL,
                  0, 0,
                  canvas_width (brush_mask), canvas_height (brush_mask),
                  TRUE);
  pixelarea_init (&srcPR, canvas_buf, NULL,
                  0, 0,
                  canvas_width (canvas_buf), canvas_height (canvas_buf),
                  TRUE);
  gimage_replace_painthit (gimage, drawable, &srcPR,
                           FALSE, image_opacity,
                           &maskPR,
                           paint_core->x, paint_core->y);
}


static void
painthit_finish (
                 GimpDrawable * drawable,
                 PaintCore16 * paint_core,
                 Canvas * painthit
                 )
{
  GImage *gimage;
  int offx, offy;

  /*  Update the undo extents  */
  paint_core->x1 = MIN (paint_core->x1, paint_core->x);
  paint_core->y1 = MIN (paint_core->y1, paint_core->y);
  paint_core->x2 = MAX (paint_core->x2, (paint_core->x + canvas_width (painthit)));
  paint_core->y2 = MAX (paint_core->y2, (paint_core->y + canvas_height (painthit)));
  
  /*  Update the gimage--it is important to call gdisplays_update_area
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  gimage = drawable_gimage (drawable);
  drawable_offsets (drawable, &offx, &offy);
  gdisplays_update_area (gimage->ID,
                         paint_core->x + offx, paint_core->y + offy,
                         canvas_width (painthit), canvas_height (painthit));
}


static Canvas * 
brush_mask_get  (
                 PaintCore16 * paint_core,
                 int brush_hardness
                 )
{
  Canvas * bm;

  switch (brush_hardness)
    {
    case SOFT:
#ifdef BRUSH_WIDTH_BORDER 
      bm = brush_mask_subsample (paint_core->brush_mask,
                                 paint_core->curx, paint_core->cury);
#else
      bm = paint_core->brush_mask;
#endif
      break;
    case HARD:
      bm = brush_mask_solidify (paint_core->brush_mask);
      break;
    default:
      bm = NULL;
      break;
    }
  return bm;
}


static Canvas * 
brush_mask_subsample  (
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

  /* temp hack */
  canvas_ref (mask, 0, 0);
  canvas_ref (dest, 0, 0);

  for (i = 0; i < canvas_height (mask); i++)
    {
      for (j = 0; j < canvas_width (mask); j++)
	{
          m = canvas_data (mask, j, i);
	  k = kernel;
	  for (r = 0; r < KERNEL_HEIGHT; r++)
	    {
	      d = canvas_data (dest, j, i+r);
	      s = KERNEL_WIDTH;
	      while (s--)
		{
		  new_val = *d + ((*m * *k++) >> 8);
		  *d++ = (new_val > 255) ? 255 : new_val;
		}
	    }
	}
    }

  /* temp hack */
  canvas_unref (mask, 0, 0);
  canvas_unref (dest, 0, 0);

  return dest;
}

typedef void  (*BrushSolidifyMaskFunc) (Canvas*,Canvas*);
static BrushSolidifyMaskFunc brush_solidify_mask_funcs[] =
{
  brush_solidify_mask_u8,
  brush_solidify_mask_u16,
  brush_solidify_mask_float
};

static Canvas * 
brush_mask_solidify  (
                      Canvas * brush_mask
                      )
{
  static Canvas * solid_brush = NULL;
  static Canvas * last_brush  = NULL;
  Precision prec = tag_precision (canvas_tag (brush_mask));  

  int i, j;
  unsigned char * data, * src;

  if (brush_mask == last_brush)
    return solid_brush;

  last_brush = brush_mask;
  if (solid_brush)
    canvas_delete (solid_brush);
#ifdef BRUSH_WITH_BORDER  
  solid_brush = canvas_new (canvas_tag (brush_mask),
                            canvas_width (brush_mask) + 2,
                            canvas_height (brush_mask) + 2,
                            canvas_tiling (brush_mask));
#else 
  solid_brush = canvas_new (canvas_tag (brush_mask),
                            canvas_width (brush_mask) ,
                            canvas_height (brush_mask) ,
                            canvas_tiling (brush_mask));
#endif
  canvas_ref (solid_brush, 0, 0);
  canvas_ref (brush_mask, 0, 0);
  
  /*  get the data and advance one line into it  */
  data = canvas_data (solid_brush, 0, 1);
  src = canvas_data (brush_mask, 0, 0);
  
  (*brush_solidify_mask_funcs [prec-1]) (brush_mask, solid_brush); 
  canvas_unref (solid_brush, 0, 0);
  canvas_unref (brush_mask, 0, 0);
  
  return solid_brush;
}


static void brush_solidify_mask_u8 (
			Canvas *brush_mask,
			Canvas *solid_brush
		      )
{  
  /* get the data and advance one line into it  */
#ifdef BRUSH_WITH_BORDER
  guint8* data = (guint8*)canvas_data (solid_brush, 0, 1);
#else
  guint8* data = (guint8*)canvas_data (solid_brush, 0, 0);
#endif
  guint8* src = (guint8*)canvas_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? OPAQUE : TRANSPARENT;
	}
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
    }
}

static void brush_solidify_mask_u16 (
			Canvas *brush_mask,
			Canvas *solid_brush
		      )
{  
  /* get the data and advance one line into it  */
#ifdef BRUSH_WITH_BORDER
  guint16* data = (guint16*)canvas_data (solid_brush, 0, 1);
#else
  guint16* data = (guint16*)canvas_data (solid_brush, 0, 0);
#endif
  guint16* src = (guint16*)canvas_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 65535 : 0;
	}
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
    }
}

static void brush_solidify_mask_float (
			Canvas *brush_mask,
			Canvas *solid_brush
		      )
{  
  /* get the data and advance one line into it  */
#ifdef BRUSH_WITH_BORDER
  gfloat* data =(gfloat*)canvas_data (solid_brush, 0, 1);
#else
  gfloat* data =(gfloat*)canvas_data (solid_brush, 0, 0);
#endif
  gfloat* src = (gfloat*)canvas_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 1.0 : 0.0;
	}
#ifdef BRUSH_WITH_BORDER
      data++;
#endif
    }
}







