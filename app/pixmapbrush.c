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
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "appenv.h"
#include "gimpbrushpixmap.h"
#include "gimpbrushlist.h"
#include "gimpbrushpipe.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gradient.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "palette.h"
#include "paintbrush.h"
#include "paint_options.h"
#include "pixmapbrush.h"
#include "selection.h"
#include "tools.h"

#include "libgimp/gimpintl.h"

/*  forward function declarations  */
static void         pixmapbrush_motion      (PaintCore *, GimpDrawable *);
/* static Argument *   pixmapbrush_invoker     (Argument *); */
/* static Argument *   pixmapbrush_extended_invoker     (Argument *); */
/* static Argument *   pixmapbrush_extended_gradient_invoker     (Argument *); */

#if 0
static void paint_line_pixmap_mask (GImage        *dest,
				    GimpDrawable  *drawable,
				    GimpBrushPixmap  *brush,
				    unsigned char *d,
				    int            x,
				    int            offset_x,
				    int            y,
				    int            offset_y,
				    int            bytes,
				    int            width);
					 
#endif	
				 
/*  defines  */
#define  PAINT_LEFT_THRESHOLD  0.05

typedef struct _PixmapPaintOptions PixmapPaintOptions;
struct _PixmapPaintOptions
{

  PaintOptions paint_options;

  double fade_out;
  double gradient_length;
  gboolean incremental;
};

/*  local variables  */
static PixmapPaintOptions *pixmap_paint_options = NULL;



static void
pixmapbrush_options_reset (void)
{
  PixmapPaintOptions *options = pixmap_paint_options;

  paint_options_reset ((PaintOptions *) options);

}

static PixmapPaintOptions *
pixmapbrush_options_new (void)
{
  PixmapPaintOptions *options;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

  /*  the new options structure  */
  options = (PixmapPaintOptions *) g_malloc (sizeof (PixmapPaintOptions));
  paint_options_init ((PaintOptions *) options,
		      PIXMAPBRUSH,
		      pixmapbrush_options_reset);

  options->fade_out = 0.0;
  options->incremental = FALSE;
  options->gradient_length = 0.0;
  
  /*  the main vbox  */
  /*   vbox = gtk_vbox_new (FALSE, 1); */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the main label  */
  label = gtk_label_new (_("Pixmapbrush Options"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the fade-out scale  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Fade Out"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

 

  return options;
}


#define USE_SPEEDSHOP_CALIPERS 0
#define TIMED_BRUSH 0

#if USE_SPEEDSHOP_CALIPERS
#include <SpeedShop/api.h>
#endif

void *
pixmap_paint_func (PaintCore *paint_core,
		   GimpDrawable *drawable,
		   int        state)
{
#if TIMED_BRUSH
  static GTimer *timer;
#endif
  switch (state)
    {
    case INIT_PAINT :
#if TIMED_BRUSH
      timer = g_timer_new();
      g_timer_start(timer);
#if USE_SPEEDSHOP_CALIPERS
      ssrt_caliper_point(0, "Painting");
#endif /* USE_SPEEDSHOP_CALIPERS */
#endif /* TIMED_BRUSH */
      break;

    case MOTION_PAINT :
      pixmapbrush_motion (paint_core, drawable);
      break;

    case FINISH_PAINT :
#if TIMED_BRUSH
#if USE_SPEEDSHOP_CALIPERS
      ssrt_caliper_point(0, "Not Painting Anymore");
#endif /* USE_SPEEDSHOP_CALIPERS */
      g_timer_stop(timer);
      printf("painting took %f:\n", g_timer_elapsed(timer, NULL));
      g_timer_destroy(timer);
#endif /* TIMED_BRUSH */
      break;

    default :
      break;
    }

  return NULL;
}


Tool *
tools_new_pixmapbrush ()
{
  Tool * tool;
  PaintCore * private;

  if (! pixmap_paint_options)
    {
      pixmap_paint_options = pixmapbrush_options_new ();
      tools_register (PIXMAPBRUSH, (ToolOptions *) pixmap_paint_options);

      pixmapbrush_options_reset();
    }
  tool = paint_core_new (PIXMAPBRUSH);

  private = (PaintCore *) tool->private;
  private->paint_func = pixmap_paint_func;

  return tool;
}


void
tools_free_pixmapbrush (Tool *tool)
{
  paint_core_free (tool);
}


static void
pixmapbrush_motion (PaintCore *paint_core,
		   GimpDrawable *drawable)
{
  GImage *gimage;
  GimpBrush *saved_brush;
  TempBuf * area;
  int opacity;
  static int index = 0;
  gboolean RANDOM=1;

  /* We always need a destination image */ 
  if (! (gimage = drawable_gimage (drawable))) 
    return; 

  if(!( GIMP_IS_BRUSH_PIPE(paint_core->brush)))
    {
      return;
    }
  else
    {
      saved_brush = paint_core->brush;
      /* Set paint_core->brush, restore below before returning.
       * I wonder if this is wise?
       */
      if(RANDOM)
	{
	  index  = rand()%gimp_brush_list_length (GIMP_BRUSH_PIPE(paint_core->brush)->brush_list);
	  paint_core->brush = 
	    gimp_brush_list_get_brush_by_index(GIMP_BRUSH_PIPE(paint_core->brush)->brush_list, index);
      }
      else
	{
	  paint_core->brush = gimp_brush_list_get_brush_by_index(GIMP_BRUSH_PIPE(saved_brush)->brush_list, index++);
	  if (index == gimp_brush_list_length (GIMP_BRUSH_PIPE(saved_brush)->brush_list))
	    index = 0;
	}
    }
  
  /* Get a region which can be used to paint to  */
  if (! (area = paint_core_get_paint_area (paint_core, drawable))) 
    {
      paint_core->brush = saved_brush;
      return;
    }
  
  color_area_with_pixmap(gimage, drawable, area, paint_core->brush);

  /* steal the pressure sensiteive code from clone.c */
  opacity = 255 * gimp_context_get_opacity (NULL) * (paint_core->curpressure / 0.5);
  if (opacity > 255)
    opacity = 255;    
  
  paint_core_paste_canvas (paint_core, drawable, opacity,
			   (int) (gimp_context_get_opacity (NULL) * 255),
			   gimp_context_get_paint_mode (NULL), SOFT, 
			   INCREMENTAL);

  paint_core->brush = saved_brush;
}



static void *
pixmapbrush_non_gui_paint_func (PaintCore *paint_core,
			       GimpDrawable *drawable,
			       int        state)
{	
  pixmapbrush_motion (paint_core, drawable);

  return NULL;
}

void
color_area_with_pixmap (GImage *dest,
			GimpDrawable *drawable,
			TempBuf *area,
			GimpBrush *brush)
{

  PixelRegion destPR;
  void * pr;
  double position;
  unsigned char *d;
  int y;
  int offset_x;
  int offset_y;
  GimpBrushPixmap *pixmapbrush = 0;
  TempBuf *pixmap_data;
  

  pr = NULL;
  pixmapbrush = GIMP_BRUSH_PIXMAP(brush);
  pixmap_data = gimp_brush_pixmap_get_pixmap(GIMP_BRUSH_PIXMAP(brush));
  position = 0.0;
	 /* stolen from clone.c */
  /* this should be a similar thing to want to do, right? */
  /* if I understand correctly, this just initilizes the dest
     pixel region to the same bitdepth, height and width of
     the drawable (area), then copies the apprriate data
     from area to destPR.data */
  destPR.bytes = area->bytes;
  destPR.x = 0; destPR.y = 0;
  destPR.w = area->width;
  destPR.h = area->height;
  destPR.rowstride = destPR.bytes * area->width;
  destPR.data = temp_buf_data (area);
		
/* register this pixel region */
  pr = pixel_regions_register (1, &destPR);

  /* maybe its not so bizare. area is always 2 wider and
     2 taller than the brush image. No idea why */
  if (area->y == 0)
    offset_y = pixmapbrush->pixmap_mask->height - destPR.h + 1;
  else
    offset_y = -1;

  if(area->x == 0)
    {
      offset_x = destPR.w -1;
      offset_y++;  /* uh, i havent a clue. but it works */
    }
  else
    offset_x = 1;

  for (; pr != NULL; pr = pixel_regions_process (pr))
    {
      d = destPR.data;
      for(y = 0; y < destPR.h  ; y++)
	{
	  paint_line_pixmap_mask(dest, drawable, pixmapbrush,
				 d, area->x,offset_x, y, offset_y,
				 destPR.bytes, destPR.w);
	
	  d += destPR.rowstride;
	}
    }

}

void
paint_line_pixmap_mask (GImage        *dest,
			GimpDrawable  *drawable,
			GimpBrushPixmap  *brush,
			unsigned char *d,
			int            x,
			int            offset_x,
			int            y,
			int            offset_y,
			int            bytes,
			int            width)
{
  unsigned char *pat, *p;
  int color, alpha;
  int i;
  int temp;
  /* point to the approriate scanline */
  /* use "pat" here because i'm c&p from pattern clone */
  pat = temp_buf_data (brush->pixmap_mask) +
    (( y + offset_y ) * brush->pixmap_mask->width * brush->pixmap_mask->bytes);
    
  color = RGB;
  alpha = bytes -1;

  /*  printf("x: %i y: %i y2: %i   \n",x,y,y2); */
  for (i = 0; i < width; i++)
    {
      p = pat + (i-offset_x) * brush->pixmap_mask->bytes;
      /* pretty sure this is wrong. I think we get artifacts because of it */
      d[alpha] = 255;

      /* printf("i: %i d->r: %i d->g: %i d->b: %i d->a: %i\n",i,(int)d[0], (int)d[1], (int)d[2], (int)d[3]); */
      gimage_transform_color (dest, drawable, p, d, color);
      d += bytes;
      
    }
}
