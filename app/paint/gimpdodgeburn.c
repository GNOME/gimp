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
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "drawable.h"
#include "errors.h"
#include "dodgeburn.h"
#include "gdisplay.h"
#include "gimplut.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "paint_options.h"
#include "selection.h"
#include "tool_options_ui.h"
#include "tools.h"
#include "gimage.h"

#include "libgimp/gimpintl.h"
#define ROUND(x) (int)((x) + .5)

/*  the dodgeburn structures  */

typedef struct _DodgeBurnOptions DodgeBurnOptions;
struct _DodgeBurnOptions
{
  PaintOptions  paint_options;

  DodgeBurnType  type;
  DodgeBurnType  type_d;
  GtkWidget    *type_w[2];

  DodgeBurnMode  mode;     /*highlights,midtones,shadows*/
  DodgeBurnMode  mode_d;
  GtkWidget      *mode_w[3];

  double        exposure;
  double        exposure_d;
  GtkObject    *exposure_w;

  GimpLut      *lut;
};

static void 
dodgeburn_make_luts ( PaintCore *, double, DodgeBurnType, DodgeBurnMode, GimpLut *, GimpDrawable *);

static gfloat dodgeburn_highlights_lut_func(void *, int, int, gfloat);
static gfloat dodgeburn_midtones_lut_func(void *, int, int, gfloat);
static gfloat dodgeburn_shadows_lut_func(void *, int, int, gfloat);

/* The dodge burn lookup tables */
gfloat dodgeburn_highlights(void *, int, int, gfloat);
gfloat dodgeburn_midtones(void *, int, int, gfloat);
gfloat dodgeburn_shadows(void *, int, int, gfloat);

/*  the dodgeburn tool options  */
static DodgeBurnOptions * dodgeburn_options = NULL;

/* Non gui function */
static double  non_gui_exposure;
static GimpLut *non_gui_lut;

/* Default values */
#define DODGEBURN_DEFAULT_TYPE     DODGE
#define DODGEBURN_DEFAULT_EXPOSURE 50.0
#define DODGEBURN_DEFAULT_MODE     DODGEBURN_HIGHLIGHTS

static void         dodgeburn_motion 	(PaintCore *, double, GimpLut *, GimpDrawable *);
static void 	    dodgeburn_init   	(PaintCore *, GimpDrawable *);
static void         dodgeburn_finish    (PaintCore *, GimpDrawable *);

/* functions  */

static void
dodgeburn_options_reset (void)
{
  DodgeBurnOptions *options = dodgeburn_options;

  paint_options_reset ((PaintOptions *) options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->exposure_w),
			    options->exposure_d);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->mode_w[options->mode_d]), TRUE);
}

static DodgeBurnOptions *
dodgeburn_options_new (void)
{
  DodgeBurnOptions *options;

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *frame;

  gchar* type_label[2] = { N_("Dodge"), N_("Burn") };
  gint   type_value[2] = { DODGE, BURN };

  gchar* mode_label[3] = { N_("Highlights"), 
			   N_("Midtones"),
			   N_("Shadows") };

  gint   mode_value[3] = { DODGEBURN_HIGHLIGHTS, 
			   DODGEBURN_MIDTONES, 
			   DODGEBURN_SHADOWS };

  /*  the new dodgeburn tool options structure  */
  options = (DodgeBurnOptions *) g_malloc (sizeof (DodgeBurnOptions));
  paint_options_init ((PaintOptions *) options,
		      DODGEBURN,
		      dodgeburn_options_reset);

  options->type     = options->type_d     = DODGEBURN_DEFAULT_TYPE;
  options->exposure = options->exposure_d = DODGEBURN_DEFAULT_EXPOSURE;
  options->mode     = options->mode_d     = DODGEBURN_DEFAULT_MODE;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the exposure scale  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Exposure:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->exposure_w =
    gtk_adjustment_new (options->exposure_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->exposure_w));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);

  gtk_signal_connect (GTK_OBJECT (options->exposure_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->exposure);

  gtk_widget_show (scale);
  gtk_widget_show (hbox);

  /* the type (dodge or burn) */
  frame = tool_options_radio_buttons_new (_("Type"), 
					  &options->type,
					   options->type_w,
					   type_label,
					   type_value,
					   2);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  mode (highlights, midtones, or shadows)  */
  frame = tool_options_radio_buttons_new (_("Mode"), 
					  &options->mode,
					   options->mode_w,
					   mode_label,
					   mode_value,
					   3);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->mode_w[options->mode_d]), TRUE); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return options;
}

void *
dodgeburn_paint_func (PaintCore    *paint_core,
		     GimpDrawable *drawable,
		     int           state)
{
  switch (state)
    {
    case INIT_PAINT:
      dodgeburn_init (paint_core, drawable);
      break;
    case MOTION_PAINT:
      dodgeburn_motion (paint_core, dodgeburn_options->exposure, dodgeburn_options->lut, drawable);
      break;
    case FINISH_PAINT:
      dodgeburn_finish (paint_core, drawable);
      break;
    }

  return NULL;
}

static void
dodgeburn_finish ( PaintCore *paint_core,
		   GimpDrawable * drawable)
{
  /* Here we destroy the luts to do the painting with.*/
  if (dodgeburn_options->lut)
  {
    gimp_lut_free (dodgeburn_options->lut);
    dodgeburn_options->lut = NULL;
  }
}

static void
dodgeburn_init ( PaintCore *paint_core,
		 GimpDrawable * drawable)
{
  /* Here we create the luts to do the painting with.*/
  dodgeburn_options->lut = gimp_lut_new();
  dodgeburn_make_luts (paint_core, 
		       dodgeburn_options->exposure,
		       dodgeburn_options->type,
		       dodgeburn_options->mode,
		       dodgeburn_options->lut,
		       drawable);
}

static void 
dodgeburn_make_luts ( PaintCore     *paint_core,
		      double        db_exposure,
		      DodgeBurnType type,
		      DodgeBurnMode mode,
		      GimpLut       *lut,
		      GimpDrawable  *drawable)
{
  GimpLutFunc lut_func;
  int nchannels = gimp_drawable_bytes (drawable);
  static gfloat exposure;

  exposure = (db_exposure)/100.0;

  /* make the exposure negative if burn for luts*/
  if (type == BURN)
    exposure = -exposure; 

  switch (mode)
    {
    case DODGEBURN_HIGHLIGHTS:
      lut_func = dodgeburn_highlights_lut_func; 
      break;
    case DODGEBURN_MIDTONES:
      lut_func = dodgeburn_midtones_lut_func; 
      break;
    case DODGEBURN_SHADOWS:
      lut_func = dodgeburn_shadows_lut_func; 
      break;
    default:
      lut_func = NULL; 
      break;
    }

  gimp_lut_setup_exact (lut, 
	lut_func, (void *)&exposure,
	nchannels);
	                  
}
		     
static void
dodgeburn_modifier_key_func (Tool        *tool,
			     GdkEventKey *kevent,
			     gpointer     gdisp_ptr)
{
  switch (kevent->keyval)
    {
    case GDK_Alt_L: case GDK_Alt_R:
      break;
    case GDK_Shift_L: case GDK_Shift_R:
      break;
    case GDK_Control_L: case GDK_Control_R:
      switch (dodgeburn_options->type)
	{
	case BURN:
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dodgeburn_options->type_w[DODGE]), TRUE);
	  break;
	case DODGE:
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dodgeburn_options->type_w[BURN]), TRUE);
	  break;
	default:
	  break;
	}
      break; 
    }
}

Tool *
tools_new_dodgeburn ()
{
  Tool * tool;
  PaintCore * private;

  /*  The tool options  */
  if (! dodgeburn_options)
    {
      dodgeburn_options = dodgeburn_options_new ();
      tools_register (DODGEBURN, (ToolOptions *) dodgeburn_options);

      /*  press all default buttons  */
      dodgeburn_options_reset ();
    }

  tool = paint_core_new (DODGEBURN);
  private = (PaintCore *) tool->private;

  private->paint_func = dodgeburn_paint_func;

  tool->modifier_key_func = dodgeburn_modifier_key_func;

  return tool;
}

void
tools_free_dodgeburn (Tool *tool)
{
  /* delete any luts here */
  paint_core_free (tool);
}

static void
dodgeburn_motion (PaintCore    *paint_core,
		  double        dodgeburn_exposure,
		  GimpLut      *lut,
		  GimpDrawable *drawable)
{
  GImage *gimage;
  TempBuf * area;
  TempBuf * orig;
  PixelRegion srcPR, destPR, tempPR;
  guchar *temp_data;
  gfloat exposure;
  gfloat brush_opacity;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /*  If the image type is indexed, don't dodgeburn  */
  if ((drawable_type (drawable) == INDEXED_GIMAGE) ||
      (drawable_type (drawable) == INDEXEDA_GIMAGE))
    return;

  /*  Get a region which can be used to paint to  */
  if (! (area = paint_core_get_paint_area (paint_core, drawable)))
    return;

  /* Constant painting --get a copy of the orig drawable (with
     no paint from this stroke yet) */

  {
      gint x1, y1, x2, y2;
      x1 = BOUNDS (area->x, 0, drawable_width (drawable));
      y1 = BOUNDS (area->y, 0, drawable_height (drawable));
      x2 = BOUNDS (area->x + area->width, 0, drawable_width (drawable));
      y2 = BOUNDS (area->y + area->height, 0, drawable_height (drawable));

      if (!(x2 - x1) || !(y2 - y1))
	return;

      /*  get the original untouched image  */
      orig = paint_core_get_orig_image (paint_core, drawable, x1, y1, x2, y2);
      srcPR.bytes = orig->bytes;
      srcPR.x = 0; 
      srcPR.y = 0;
      srcPR.w = x2 - x1;
      srcPR.h = y2 - y1;
      srcPR.rowstride = srcPR.bytes * orig->width;
      srcPR.data = temp_buf_data (orig);
   }

  /* tempPR will hold the dodgeburned region*/
  tempPR.bytes = srcPR.bytes;
  tempPR.x = srcPR.x; 
  tempPR.y = srcPR.y;
  tempPR.w = srcPR.w;
  tempPR.h = srcPR.h;
  tempPR.rowstride = tempPR.bytes * tempPR.w;
  temp_data = g_malloc (tempPR.h * tempPR.rowstride);
  tempPR.data = temp_data;

  brush_opacity = gimp_context_get_opacity (NULL);

  /* Enable pressure sensitive exposure */
  exposure = ((dodgeburn_exposure)/100.0 * (paint_core->curpressure)/0.5);

  /*  DodgeBurn the region  */
  gimp_lut_process (lut, &srcPR, &tempPR);

  /* The dest is the paint area we got above (= canvas_buf) */ 
  destPR.bytes = area->bytes;
  destPR.x = 0; destPR.y = 0;
  destPR.w = area->width;
  destPR.h = area->height;
  destPR.rowstride = area->width * destPR.bytes;
  destPR.data = temp_buf_data (area);

  /* Now add an alpha to the dodgeburned region 
     and put this in area = canvas_buf */ 
  if (!drawable_has_alpha (drawable))
    add_alpha_region (&tempPR, &destPR);
  else
    copy_region(&tempPR, &destPR);

  /*Replace the newly dodgedburned area (canvas_buf) to the gimage*/ 
  
  paint_core_replace_canvas (paint_core, drawable, ROUND(brush_opacity * 255.0),
				OPAQUE_OPACITY, PRESSURE, CONSTANT);
 
  g_free (temp_data);
}

static void *
dodgeburn_non_gui_paint_func (PaintCore *paint_core,
			     GimpDrawable *drawable,
			     int        state)
{
  dodgeburn_motion (paint_core, non_gui_exposure, non_gui_lut, drawable);

  return NULL;
}

gboolean
dodgeburn_non_gui_default (GimpDrawable *drawable,
			   int           num_strokes,
			   double       *stroke_array)
{
  double exposure = DODGEBURN_DEFAULT_TYPE;
  DodgeBurnType type = DODGEBURN_DEFAULT_TYPE;
  DodgeBurnMode mode = DODGEBURN_DEFAULT_MODE;
  DodgeBurnOptions *options = dodgeburn_options;

  if(options)
    {
      exposure = dodgeburn_options->exposure;
      type     = dodgeburn_options->type;
      mode     = dodgeburn_options->mode;
    }

  return dodgeburn_non_gui(drawable,exposure,type,mode,num_strokes,stroke_array);
}

gboolean
dodgeburn_non_gui (GimpDrawable *drawable,
		   double        exposure,
		   DodgeBurnType type,
		   DodgeBurnMode mode,
		   int           num_strokes,
		   double       *stroke_array)
{
  int i;

  if (paint_core_init (&non_gui_paint_core, drawable,
		       stroke_array[0], stroke_array[1]))
    {
      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = dodgeburn_non_gui_paint_func;

      non_gui_exposure = exposure;
      non_gui_lut = gimp_lut_new();
      dodgeburn_make_luts (&non_gui_paint_core, 
			   exposure,
			   type,
			   mode,
			   non_gui_lut,
			   drawable);

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      dodgeburn_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cury = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;
	}

      /* Finish the painting */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /* Cleanup */
      paint_core_cleanup ();

      gimp_lut_free (non_gui_lut);

      return TRUE;
    }
  else
    return FALSE;
}

static gfloat 
dodgeburn_highlights_lut_func(void * user_data, 
				int nchannels, 
				int channel, 
				gfloat value)
{
   gfloat * exposure_ptr = (gfloat *)user_data;
   gfloat exposure = *exposure_ptr;
   gfloat factor = 1.0 + exposure * (.333333);
            
  if ( (nchannels == 2 && channel == 1) ||
	(nchannels == 4 && channel == 3))
    return value;

   return factor * value;
}

static gfloat 
dodgeburn_midtones_lut_func(void * user_data, 
				int nchannels, 
				int channel, 
				gfloat value)
{
   gfloat * exposure_ptr = (gfloat *)user_data;
   gfloat exposure = *exposure_ptr;
   gfloat factor;

  if ( (nchannels == 2 && channel == 1) ||
	(nchannels == 4 && channel == 3))
    return value;

   if (exposure < 0)
     factor = 1.0 - exposure * (.333333);
   else
     factor = 1/(1.0 + exposure);
   return pow (value, factor); 
}

static gfloat 
dodgeburn_shadows_lut_func(void * user_data, 
				int nchannels, 
				int channel, 
				gfloat value)
{
  gfloat * exposure_ptr = (gfloat *)user_data;
  gfloat exposure = *exposure_ptr;
  gfloat new_value;
  gfloat factor;
  
  if ( (nchannels == 2 && channel == 1) ||
	(nchannels == 4 && channel == 3))
    return value;

  if (exposure >= 0)
  {
    factor = .333333 * exposure;
    new_value =  factor + value - factor * value; 
  }
  else /* exposure < 0 */ 
  {
    factor = -.333333 * exposure;
    if (value < factor)
      new_value = 0;
    else /*factor <= value <=1*/
      new_value = (value - factor)/(1 - factor);
  }
  return new_value; 
}
