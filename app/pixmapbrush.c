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
#include <stdio.h>
#include <math.h>
#include "appenv.h"
#include "gimpbrushpixmap.h"
#include "gimpbrushlist.h"
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
static void         pixmapbrush_motion      (PaintCore *, GimpDrawable *, double, double, gboolean);
/* static Argument *   pixmapbrush_invoker     (Argument *); */
/* static Argument *   pixmapbrush_extended_invoker     (Argument *); */
/* static Argument *   pixmapbrush_extended_gradient_invoker     (Argument *); */
static double non_gui_fade_out,non_gui_gradient_length, non_gui_incremental;

static void   paint_line_pixmap_mask (GImage        *dest,
				      GimpDrawable  *drawable,
				      GimpBrushPixmap  *brush,
				      unsigned char *d,
				      int            x,
				      int            y,
				      int            y2,
				      int            bytes,
				      int            width);


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
pixmapbrush_toggle_update (GtkWidget *w,
			  gpointer   data)
{
  gboolean *toggle_val;

  toggle_val = (gboolean *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
pixmapbrush_scale_update (GtkAdjustment *adjustment,
			 PixmapPaintOptions   *options)
{
 options->gradient_length = adjustment->value;
 if(options->gradient_length > 0.0)
   options->incremental = INCREMENTAL;
}

static void
pixmapbrush_fade_update (GtkAdjustment *adjustment,
			 PixmapPaintOptions   *options)
{
 options->fade_out = adjustment->value;
}

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
  GtkWidget *fade_out_scale;
  GtkObject *fade_out_scale_data;
  GtkWidget *gradient_length_scale;
  GtkObject *gradient_length_scale_data;
  GtkWidget *incremental_toggle;


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
      pixmapbrush_motion (paint_core, drawable, 
			 pixmap_paint_options->fade_out, 
			 pixmap_paint_options->gradient_length,
			 pixmap_paint_options->incremental);
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
		   GimpDrawable *drawable,
		   double     fade_out,
		   double     gradient_length,
		   gboolean   incremental)
{
  GImage *gimage;
  GimpBrushPixmap *brush = 0;
  TempBuf * pixmap_data;
  TempBuf * area;
  double position;
  unsigned char *d;
  int y;
  unsigned char col[MAX_CHANNELS];
  void * pr;
  PixelRegion destPR;
  pr = NULL;

  /* FIXME: this code doesnt work quite right at the far top and
     and left sides. need to handle those cases better */

  brush = GIMP_BRUSH_PIXMAP(paint_core->brush);
  pixmap_data = gimp_brush_pixmap_get_pixmap(brush);
  position = 0.0;

  /*  We always need a destination image */
  if (! (gimage = drawable_gimage (drawable)))
    return;


  /*  Get a region which can be used to p\\aint to  */
  if (! (area = paint_core_get_paint_area (paint_core, drawable)))
    return;

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

  for (; pr != NULL; pr = pixel_regions_process (pr))
    {
      d = destPR.data;
      for(y = 0; y < destPR.h; y++)
	{
	  /*  printf("y: %i destPR.h: %i\n", y, destPR.h); */
	  paint_line_pixmap_mask(gimage, drawable, brush,
				 d, area->x, area->y, y,
				 destPR.bytes, destPR.w);
	
	  d += destPR.rowstride;
	}
    }

      /* this will eventually get replaced with the code to merge 
	 the brush mask and the pixmap data into one temp_buf */
     /*  color the pixels  */

     /*          printf("temp_blend: %u grad_len: %f distance: %f \n",temp_blend, gradient_length, distance); */ 
  /* remove these once things are stable */
  /* printf("opacity: %i ", (int) (gimp_context_get_opacity (NULL) * 255)); */
  /*    printf("r: %i g: %i b: %i a: %i\n", col[0], col[1], col[2], col[3]); */ 
      paint_core_paste_canvas (paint_core, drawable, 255,
			       (int) (gimp_context_get_opacity (NULL) * 255),
			       gimp_context_get_paint_mode (NULL), SOFT, 
			       INCREMENTAL);
}



static void *
pixmapbrush_non_gui_paint_func (PaintCore *paint_core,
			       GimpDrawable *drawable,
			       int        state)
{	
  pixmapbrush_motion (paint_core, drawable, non_gui_fade_out,non_gui_gradient_length,  non_gui_incremental);

  return NULL;
}


static void
paint_line_pixmap_mask (GImage        *dest,
			GimpDrawable  *drawable,
			GimpBrushPixmap  *brush,
			unsigned char *d,
			int            x,
			int            y,
			int            y2,
			int            bytes,
			int            width)
{
  unsigned char *pat, *p;
  unsigned char *dp;
  int color, alpha;
  int i;
  unsigned char rgb[3];

  /* point to the approriate scanline */
  /* use "pat" here because i'm c&p from pattern clone */
  pat = temp_buf_data (brush->pixmap_mask) +
    (y2 * brush->pixmap_mask->width * brush->pixmap_mask->bytes);
    
  /*   dest = d +  (y * brush->pixmap_mask->width * brush->pixmap_mask->bytes); */
  color = RGB;

  alpha = bytes -1;

  
  
  for (i = 0; i < width; i++)
    {
      p = pat + (i % brush->pixmap_mask->width) * brush->pixmap_mask->bytes;
      /* printf("x: %i y: %i y2: %i i: %i  \n",x,y,y2,i); */
      /* printf("d->r: %i d->g: %i d->b: %i d->a: %i\n",(int)d[0], (int)d[1], (int)d[2], (int)d[3]); */
      gimage_transform_color (dest, drawable, p, d, color);

      d[3] = 255;
      d += bytes;
      
    }
}
