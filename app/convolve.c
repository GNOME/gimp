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

#include <gdk/gdkkeysyms.h>

#include "appenv.h"
#include "drawable.h"
#include "convolve.h"
#include "gdisplay.h"
#include "gimpui.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "paint_options.h"
#include "selection.h"
#include "tools.h"
#include "gimage.h"

#include "libgimp/gimpintl.h"


#define FIELD_COLS    4
#define MIN_BLUR      64         /*  (8/9 original pixel)   */
#define MAX_BLUR      0.25       /*  (1/33 original pixel)  */
#define MIN_SHARPEN   -512
#define MAX_SHARPEN   -64

/* defaults */
#define DEFAULT_CONVOLVE_RATE  50.0
#define DEFAULT_CONVOLVE_TYPE  BLUR_CONVOLVE


/* Different clip relationships between a blur-blob and edges: 
   see convolve_motion */

typedef enum
{
  CONVOLVE_NCLIP,       /* Left or top edge     */
  CONVOLVE_NOT_CLIPPED, /* No edges             */
  CONVOLVE_PCLIP        /* Right or bottom edge */
} ConvolveClipType;


/*  the convolve structures  */

typedef struct _ConvolveOptions ConvolveOptions;

struct _ConvolveOptions
{
  PaintOptions  paint_options;

  ConvolveType  type;
  ConvolveType  type_d;
  GtkWidget    *type_w[2];

  gdouble       rate;
  gdouble       rate_d;
  GtkObject    *rate_w;
};


/*  the convolve tool options  */
static ConvolveOptions * convolve_options = NULL;

/*  local variables  */
static gint         matrix [25];
static gint         matrix_size;
static gint         matrix_divisor;

static ConvolveType non_gui_type;
static gdouble      non_gui_rate;

static gfloat       custom_matrix [25] =
{
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 1, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
};

static gfloat       blur_matrix [25] =
{
  0, 0, 0, 0, 0,
  0, 1, 1, 1, 0,
  0, 1, MIN_BLUR, 1, 0,
  0, 1, 1, 1, 0,
  0, 0 ,0, 0, 0,
};

static gfloat       sharpen_matrix [25] =
{
  0, 0, 0, 0, 0,
  0, 1, 1, 1, 0,
  0, 1, MIN_SHARPEN, 1, 0,
  0, 1, 1, 1, 0,
  0, 0, 0, 0, 0,
};


/*  forward function declarations  */
static void         calculate_matrix     (ConvolveType, double);
static void         integer_matrix       (float *, int *, int);
static void         copy_matrix          (float *, float *, int);
static int          sum_matrix           (int *, int);

static void         convolve_motion      (PaintCore *, GimpDrawable *, 
					  PaintPressureOptions *,
					  ConvolveType, double);

/* functions  */


static void
convolve_options_reset (void)
{
  ConvolveOptions *options = convolve_options;

  paint_options_reset ((PaintOptions *) options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->rate_w), options->rate_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE);
}

static ConvolveOptions *
convolve_options_new (void)
{
  ConvolveOptions *options;

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *frame;

  /*  the new convolve tool options structure  */
  options = g_new (ConvolveOptions, 1);
  paint_options_init ((PaintOptions *) options,
		      CONVOLVE,
		      convolve_options_reset);
  options->type = options->type_d = DEFAULT_CONVOLVE_TYPE;
  options->rate = options->rate_d = DEFAULT_CONVOLVE_RATE;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the rate scale  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Rate:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->rate_w =
    gtk_adjustment_new (options->rate_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->rate_w));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->rate_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->rate);
  gtk_widget_show (scale);
  gtk_widget_show (hbox);

  frame = gimp_radio_group_new2 (TRUE, _("Convolve Type"),
				 gimp_radio_button_update,
				 &options->type, (gpointer) options->type,

				 _("Blur"), (gpointer) BLUR_CONVOLVE,
				 &options->type_w[0],
				 _("Sharpen"), (gpointer) SHARPEN_CONVOLVE,
				 &options->type_w[1],

				 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return options;
}

void *
convolve_paint_func (PaintCore    *paint_core,
		     GimpDrawable *drawable,
		     int           state)
{
  switch (state)
    {
    case MOTION_PAINT:
      convolve_motion (paint_core, drawable, 
		       convolve_options->paint_options.pressure_options,
		       convolve_options->type, convolve_options->rate);
      break;
    }

  return NULL;
}

static void
convolve_modifier_key_func (Tool        *tool,
			    GdkEventKey *kevent,
			    gpointer     gdisp_ptr)
{
  switch (kevent->keyval)
    {
    case GDK_Alt_L: 
    case GDK_Alt_R:
      break;
    case GDK_Shift_L: 
    case GDK_Shift_R:
      if (kevent->state & GDK_CONTROL_MASK)    /* reset tool toggle */
	{
	  switch (convolve_options->type)
	    {
	    case BLUR_CONVOLVE:
	      gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON (convolve_options->type_w[SHARPEN_CONVOLVE]),
		 TRUE);
	      break;
	    case SHARPEN_CONVOLVE:
	      gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON (convolve_options->type_w[BLUR_CONVOLVE]),
		 TRUE);
	      break;
	    default:
	      break;
	    }
	}
      break;
    case GDK_Control_L: 
    case GDK_Control_R:
      if ( !(kevent->state & GDK_SHIFT_MASK) ) /* shift enables line draw mode */
	{
	  switch (convolve_options->type)
	    {
	    case BLUR_CONVOLVE:
	      gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON (convolve_options->type_w[SHARPEN_CONVOLVE]),
		 TRUE);
	      break;
	    case SHARPEN_CONVOLVE:
	      gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON (convolve_options->type_w[BLUR_CONVOLVE]),
		 TRUE);
	      break;
	    default:
	      break;
	    }
	}
      break;
    }

  tool->toggled = (convolve_options->type == SHARPEN_CONVOLVE);
}

static void
convolve_cursor_update_func (Tool           *tool,
			     GdkEventMotion *mevent,
			     gpointer        gdisp_ptr)
{
  tool->toggled = (convolve_options->type == SHARPEN_CONVOLVE);

  paint_core_cursor_update (tool, mevent, gdisp_ptr);
}

Tool *
tools_new_convolve (void)
{
  Tool * tool;
  PaintCore * private;

  /*  The tool options  */
  if (! convolve_options)
    {
      convolve_options = convolve_options_new ();
      tools_register (CONVOLVE, (ToolOptions *) convolve_options);

      /*  press all default buttons  */
      convolve_options_reset ();
    }

  tool = paint_core_new (CONVOLVE);
  tool->modifier_key_func  = convolve_modifier_key_func;
  tool->cursor_update_func = convolve_cursor_update_func;

  private = (PaintCore *) tool->private;
  private->paint_func = convolve_paint_func;

  return tool;
}

void
tools_free_convolve (Tool *tool)
{
  paint_core_free (tool);
}

static void
convolve_motion (PaintCore            *paint_core,
		 GimpDrawable         *drawable,
		 PaintPressureOptions *pressure_options,
		 ConvolveType          type,
		 double                rate)
{
  TempBuf         *area;
  guchar          *temp_data;
  PixelRegion      srcPR; 
  PixelRegion      destPR;
  gdouble          scale;
  ConvolveClipType area_hclip = CONVOLVE_NOT_CLIPPED;
  ConvolveClipType area_vclip = CONVOLVE_NOT_CLIPPED;
  gint             marginx = 0;
  gint             marginy = 0;

  if (!drawable_gimage (drawable))
    return;

  /*  If the image type is indexed, don't convolve  */
  if ((drawable_type (drawable) == INDEXED_GIMAGE) ||
      (drawable_type (drawable) == INDEXEDA_GIMAGE))
    return;

  /* If the brush is smaller than the convolution matrix, don't convolve */

  if((paint_core->brush->mask->width < matrix_size) || (paint_core->brush->mask->height < matrix_size))
    return;

  if (pressure_options->size)
    scale = paint_core->curpressure;
  else
    scale = 1.0;

  /*  Get image region around current brush (mask bbox + 1 pixel)  */
  if (! (area = paint_core_get_paint_area (paint_core, drawable, scale)))
    return;

  /*  configure the source pixel region  */
  pixel_region_init (&srcPR, drawable_data (drawable),
		     area->x, area->y, area->width, area->height, FALSE);

  /* Configure the destination pixel region - a paint_core TempBuf */

  destPR.bytes     = area->bytes;
  destPR.tiles     = NULL;
  destPR.x         = 0; 
  destPR.y         = 0;
  destPR.w         = area->width;
  destPR.h         = area->height;
  destPR.rowstride = area->width * destPR.bytes;
  destPR.data      = temp_buf_data (area);

  if (pressure_options->rate)
    rate = rate * 2.0 * paint_core->curpressure;

  calculate_matrix (type, rate); 
    
  /*  Image region near edges? If so, paint area will be clipped   */
  /*  with respect to brush mask + 1 pixel border (# 19285)        */
 
  if((marginx = (gint)paint_core->curx - paint_core->brush->mask->width/2 - 1)  != area->x)
    area_hclip = CONVOLVE_NCLIP;
  else
    if((marginx = area->width - paint_core->brush->mask->width - 2) != 0)
      area_hclip = CONVOLVE_PCLIP;

  if((marginy = (gint)paint_core->cury - paint_core->brush->mask->height/2 - 1) != area->y)
    area_vclip = CONVOLVE_NCLIP;
  else 
    if((marginy = area->height - paint_core->brush->mask->height - 2) != 0)
      area_vclip = CONVOLVE_PCLIP;

  /* Has the TempBuf been clipped by a canvas edge or two ?        */
  if((area_hclip == CONVOLVE_NOT_CLIPPED)  && (area_vclip == CONVOLVE_NOT_CLIPPED))
    {
      /* No clipping...                                              */
      /* Standard case: copy src to temp. convolve temp to dest.     */
      /* Brush defines pipe size and no edge adjustments are needed. */

      /*  If the source has no alpha, then add alpha pixels          */
      /*  Because paint_core.c is alpha-only code. See below.        */

      PixelRegion  tempPR;

      tempPR.x     = 0; 
      tempPR.y     = 0;
      tempPR.w     = area->width;
      tempPR.h     = area->height;
      tempPR.tiles = NULL;

      if (!drawable_has_alpha (drawable))
	{
	  /* note: this particular approach needlessly convolves the totally-
	     opaque alpha channel. A faster approach would be to keep
	     tempPR the same number of bytes as srcPR, and extend the
	     paint_core_replace_canvas API to handle non-alpha images. */

	  tempPR.bytes     = srcPR.bytes + 1;
	  tempPR.rowstride = tempPR.bytes * tempPR.w;
	  temp_data        = g_malloc (tempPR.h * tempPR.rowstride);
	  tempPR.data      = temp_data;
	  add_alpha_region (&srcPR, &tempPR);
	}
      else
	{
	  tempPR.bytes     = srcPR.bytes;
	  tempPR.rowstride = tempPR.bytes * tempPR.w;
	  temp_data        = g_malloc (tempPR.h * tempPR.rowstride);
	  tempPR.data      = temp_data;
	  copy_region (&srcPR, &tempPR);
	}

      /*  Convolve the region  */

      tempPR.x    = 0;
      tempPR.y    = 0;
      tempPR.w    = area->width;
      tempPR.h    = area->height;
      tempPR.data = temp_data;
      convolve_region (&tempPR, &destPR, matrix, matrix_size,
		       matrix_divisor, NORMAL_CONVOL);

      /*  Free the allocated temp space  */
      g_free (temp_data);
    }
  else
    {
      /* TempBuf clipping has occured on at least one edge...                   */
      /* Edge case: expand area under brush margin px on near edge(s), convolve */
      /* expanded buffers. copy src -> ovrsz1 convolve ovrsz1 -> ovrsz2         */
      /* copy-with-crop ovrsz2 -> dest                                          */

      PixelRegion   ovrsz1PR;
      PixelRegion   ovrsz2PR;
      guchar        *ovrsz1_data = NULL;
      guchar        *ovrsz2_data = NULL;
      guchar        *fillcolor   = gimp_drawable_get_color_at (drawable,
				     CLAMP ((gint) paint_core->curx, 0, gimp_drawable_width (drawable) - 1),
				     CLAMP ((gint) paint_core->cury, 0, gimp_drawable_height (drawable) - 1));

      marginx *= (marginx < 0) ? -1 : 0;
      marginy *= (marginy < 0) ? -1 : 0;

      ovrsz2PR.x         = 0;
      ovrsz2PR.y         = 0;
      ovrsz2PR.w         = area->width  + marginx;
      ovrsz2PR.h         = area->height + marginy;
      ovrsz2PR.bytes     = (drawable_has_alpha (drawable))? srcPR.bytes : srcPR.bytes + 1; 
      ovrsz2PR.offx      = 0;
      ovrsz2PR.offy      = 0;
      ovrsz2PR.rowstride = ovrsz2PR.bytes * ovrsz2PR.w;
      ovrsz2PR.tiles     = NULL;
      ovrsz2_data        = g_malloc (ovrsz2PR.h * ovrsz2PR.rowstride);
      ovrsz2PR.data      = ovrsz2_data;

      ovrsz1PR.x         = 0;
      ovrsz1PR.y         = 0;
      ovrsz1PR.w         = area->width  + marginx;
      ovrsz1PR.h         = area->height + marginy;
      ovrsz1PR.bytes     = (drawable_has_alpha (drawable))? srcPR.bytes : srcPR.bytes + 1; 
      ovrsz1PR.offx      = 0;
      ovrsz1PR.offy      = 0;
      ovrsz1PR.rowstride = ovrsz2PR.bytes * ovrsz2PR.w;
      ovrsz1PR.tiles     = NULL;
      ovrsz1_data        = g_malloc (ovrsz1PR.h * ovrsz1PR.rowstride);
      ovrsz1PR.data      = ovrsz1_data;
      color_region (&ovrsz1PR, (const guchar *)fillcolor); 

      ovrsz1PR.x         = (area_hclip == CONVOLVE_NCLIP)? marginx : 0; 
      ovrsz1PR.y         = (area_vclip == CONVOLVE_NCLIP)? marginy : 0;
      ovrsz1PR.w         = area->width;
      ovrsz1PR.h         = area->height;
      ovrsz1PR.data      = ovrsz1_data + (ovrsz1PR.rowstride * ovrsz1PR.y) + (ovrsz1PR.bytes * ovrsz1PR.x);

      if (!(drawable_has_alpha (drawable)))
	add_alpha_region (&srcPR, &ovrsz1PR);
      else
	copy_region (&srcPR, &ovrsz1PR);

      /*  Convolve the region  */

      ovrsz1PR.x    = 0;
      ovrsz1PR.y    = 0;
      ovrsz1PR.w    = area->width  + marginx;
      ovrsz1PR.h    = area->height + marginy;
      ovrsz1PR.data = ovrsz1_data;
      convolve_region (&ovrsz1PR, &ovrsz2PR, matrix, matrix_size,
		       matrix_divisor, NORMAL_CONVOL);

      /* Crop and copy to destination */

      ovrsz2PR.x    = (area_hclip == CONVOLVE_NCLIP)? marginx : 0; 
      ovrsz2PR.y    = (area_vclip == CONVOLVE_NCLIP)? marginy : 0; 
      ovrsz2PR.w    = area->width;
      ovrsz2PR.h    = area->height;
      ovrsz2PR.data = ovrsz2_data + (ovrsz2PR.rowstride * ovrsz2PR.y) + (ovrsz2PR.bytes * ovrsz2PR.x);
      copy_region (&ovrsz2PR, &destPR);
      g_free(ovrsz1_data);
      g_free(ovrsz2_data);
      g_free(fillcolor);
    }

  /*  paste the newly painted canvas to the gimage which is being worked on  */
  paint_core_replace_canvas (paint_core, drawable, OPAQUE_OPACITY,
			     (int) (gimp_context_get_opacity (NULL) * 255),
			     pressure_options->pressure ? PRESSURE : SOFT,
			     scale, INCREMENTAL);
}

static void
calculate_matrix (ConvolveType type,
		  double       rate)
{
  float percent;

  /*  find percent of tool pressure  */
  percent = MIN (rate / 100.0, 1.0);

  /*  get the appropriate convolution matrix and size and divisor  */
  switch (type)
    {
    case BLUR_CONVOLVE:
      matrix_size = 5;
      blur_matrix [12] = MIN_BLUR + percent * (MAX_BLUR - MIN_BLUR);
      copy_matrix (blur_matrix, custom_matrix, matrix_size);
      break;

    case SHARPEN_CONVOLVE:
      matrix_size = 5;
      sharpen_matrix [12] = MIN_SHARPEN + percent * (MAX_SHARPEN - MIN_SHARPEN);
      copy_matrix (sharpen_matrix, custom_matrix, matrix_size);
      break;

    case CUSTOM_CONVOLVE:
      matrix_size = 5;
      break;
    }

  integer_matrix (custom_matrix, matrix, matrix_size);
  matrix_divisor = sum_matrix (matrix, matrix_size);

  if (!matrix_divisor)
    matrix_divisor = 1;
}

static void
integer_matrix (float *source,
		int   *dest,
		int    size)
{
  int i;

#define PRECISION  10000

  for (i = 0; i < size*size; i++)
    *dest++ = (int) (*source ++ * PRECISION);
}

static void
copy_matrix (float *src,
	     float *dest,
	     int    size)
{
  int i;

  for (i = 0; i < size*size; i++)
    *dest++ = *src++;
}

static int
sum_matrix (int *matrix,
	    int  size)
{
  int sum = 0;

  size *= size;

  while (size --)
    sum += *matrix++;

  return sum;
}


static void *
convolve_non_gui_paint_func (PaintCore *paint_core,
			     GimpDrawable *drawable,
			     int        state)
{
  convolve_motion (paint_core, drawable, &non_gui_pressure_options,
		   non_gui_type, non_gui_rate);

  return NULL;
}

gboolean
convolve_non_gui_default (GimpDrawable *drawable,
			  int           num_strokes,
			  double       *stroke_array)
{
  double rate = DEFAULT_CONVOLVE_RATE;
  ConvolveType type = DEFAULT_CONVOLVE_TYPE;
  ConvolveOptions *options = convolve_options;

  if (options)
    {
      rate = options->rate;
      type = options->type;
    }

  return convolve_non_gui (drawable, rate, type, num_strokes, stroke_array);
}

gboolean
convolve_non_gui (GimpDrawable *drawable,
    		  double        rate,
		  ConvolveType  type,
		  int           num_strokes,
		  double       *stroke_array)
{
  int i;

  if (paint_core_init (&non_gui_paint_core, drawable,
		       stroke_array[0], stroke_array[1]))
    {
      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = convolve_non_gui_paint_func;
      
      non_gui_type = type;
      non_gui_rate = rate;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      convolve_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

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
      return TRUE;
    }
  else
    return FALSE;
}
