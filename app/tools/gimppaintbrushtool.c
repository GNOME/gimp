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

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "drawable.h"
#include "gdisplay.h"
#include "gimpbrush.h"
#include "gimpcontext.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "paint_funcs.h"
#include "selection.h"
#include "temp_buf.h"

#include "tools/gimppainttool.h"
#include "paint_options.h"
#include "tools/gimppaintbrushtool.h"
#include "tool_options.h"
#include "tools/tool.h"
#include "tools/tool_manager.h"

#include "pixmaps2.h"

#include "libgimp/gimpintl.h"

/*  defines  */
#define  PAINT_LEFT_THRESHOLD  0.05

/* defaults for the tool options */
#define PAINTBRUSH_DEFAULT_INCREMENTAL       FALSE
#define PAINTBRUSH_DEFAULT_USE_FADE          FALSE
#define PAINTBRUSH_DEFAULT_FADE_OUT          100.0
#define PAINTBRUSH_DEFAULT_FADE_UNIT         GIMP_UNIT_PIXEL
#define PAINTBRUSH_DEFAULT_USE_GRADIENT      FALSE
#define PAINTBRUSH_DEFAULT_GRADIENT_LENGTH   100.0
#define PAINTBRUSH_DEFAULT_GRADIENT_UNIT     GIMP_UNIT_PIXEL
#define PAINTBRUSH_DEFAULT_GRADIENT_TYPE     LOOP_TRIANGLE

/*  the paintbrush structures  */

typedef struct _PaintbrushOptions PaintbrushOptions;

struct _PaintbrushOptions
{
  PaintOptions  paint_options;
};

/*  the paint brush tool options  */
static PaintbrushOptions * paintbrush_options = NULL;

/*  local variables  */
static gdouble   non_gui_fade_out;
static gdouble   non_gui_gradient_length;
static gint      non_gui_gradient_type;
static gdouble   non_gui_incremental;
static GimpUnit  non_gui_fade_unit;
static GimpUnit  non_gui_gradient_unit;

static GimpPaintToolClass *parent_class;

/*  forward function declarations  */
static void  gimp_paintbrush_tool_motion     (GimpPaintTool        *,
					      GimpDrawable         *,
					      PaintPressureOptions *,
					      PaintGradientOptions *,
					      gdouble               ,
					      gdouble               ,
					      PaintApplicationMode  ,
					      GradientPaintMode     );
static void gimp_paintbrush_tool_paint       (GimpPaintTool        *paint_core,
					      GimpDrawable         *drawable,
					      PaintState            state);
static void gimp_paintbrush_tool_class_init  (GimpPaintbrushToolClass *klass);
static void gimp_paintbrush_tool_init        (GimpPaintbrushTool      *tool);


/*  functions  */

GtkType
gimp_paintbrush_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpPaintbrushTool",
        sizeof (GimpPaintbrushTool),
        sizeof (GimpPaintbrushToolClass),
        (GtkClassInitFunc) gimp_paintbrush_tool_class_init,
        (GtkObjectInitFunc) gimp_paintbrush_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        NULL
      };

      tool_type = gtk_type_unique (GIMP_TYPE_PAINT_TOOL, &tool_info);
    }

  return tool_type;
}


static void
gimp_paintbrush_tool_options_reset (void)
{
  PaintbrushOptions *options = paintbrush_options;

  paint_options_reset ((PaintOptions *) options);
}

static PaintbrushOptions *
gimp_paintbrush_tool_options_new (void)
{
  PaintbrushOptions *options;

  /*  the new paint tool options structure  */
  options = g_new0 (PaintbrushOptions, 1);
  paint_options_init ((PaintOptions *) options,
		      GIMP_TYPE_PAINTBRUSH_TOOL,
		      gimp_paintbrush_tool_options_reset);

  return options;
}

static void
gimp_paintbrush_tool_paint (GimpPaintTool *paint_tool,
			    GimpDrawable  *drawable,
			    PaintState     state)
{
  PaintPressureOptions *pressure_options;
  PaintGradientOptions *gradient_options;
  gboolean              incremental;
  GimpImage            *gimage;
  gdouble               fade_out;
  gdouble               gradient_length;
  gdouble               unit_factor;

  gimage = gimp_drawable_gimage (drawable);

  g_return_if_fail (gimage != NULL);

  if (! gimage)
    return;

  if (paintbrush_options)
    {
      pressure_options = paintbrush_options->paint_options.pressure_options;
      gradient_options = paintbrush_options->paint_options.gradient_options;
      incremental      = paintbrush_options->paint_options.incremental;
    }
  else
    {
      pressure_options = &non_gui_pressure_options;
      gradient_options = &non_gui_gradient_options;
      incremental      = non_gui_incremental;
    }

  switch (state)
    {
    case INIT_PAINT:
      break;

    case MOTION_PAINT:
      switch (gradient_options->fade_unit)
	{
	case GIMP_UNIT_PIXEL:
	  fade_out = gradient_options->fade_out;
	  break;
	case GIMP_UNIT_PERCENT:
	  fade_out = (MAX (gimage->width, gimage->height) *
		      gradient_options->fade_out / 100);
	  break;
	default:
	  unit_factor = gimp_unit_get_factor (gradient_options->fade_unit);
	  fade_out = (gradient_options->fade_out *
		      MAX (gimage->xresolution,
			   gimage->yresolution) / unit_factor);
	  break;
	}

      switch (gradient_options->gradient_unit)
	{
	case GIMP_UNIT_PIXEL:
	  gradient_length = gradient_options->gradient_length;
	  break;
	case GIMP_UNIT_PERCENT:
	  gradient_length = (MAX (gimage->width, gimage->height) *
			     gradient_options->gradient_length / 100);
	  break;
	default:
	  unit_factor = gimp_unit_get_factor (gradient_options->gradient_unit);
	  gradient_length = (gradient_options->gradient_length *
			     MAX (gimage->xresolution,
				  gimage->yresolution) / unit_factor);
	  break;
	}

      gimp_paintbrush_tool_motion (paint_tool, drawable,
				   pressure_options,
				   gradient_options,
				   gradient_options->use_fade ? fade_out : 0,
				   gradient_options->use_gradient ? gradient_length : 0,
				   incremental,
				   gradient_options->gradient_type);
      break;

    case FINISH_PAINT:
      break;

    default:
      break;
    }
}


GimpTool *
gimp_paintbrush_tool_new (void)
{
  return gtk_type_new (GIMP_TYPE_PAINTBRUSH_TOOL);
}

static void
gimp_paintbrush_tool_init (GimpPaintbrushTool *paintbrush)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (paintbrush);
  paint_tool = GIMP_PAINT_TOOL (paintbrush);

  if (! paintbrush_options)
    {
      paintbrush_options = gimp_paintbrush_tool_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_PAINTBRUSH_TOOL,
                                          (ToolOptions *) paintbrush_options);
    }

  tool->tool_cursor = GIMP_PAINTBRUSH_TOOL_CURSOR;

  paint_tool->pick_colors =  TRUE;
  paint_tool->flags       |= TOOL_CAN_HANDLE_CHANGING_BRUSH;
}

static void
gimp_paintbrush_tool_class_init (GimpPaintbrushToolClass *klass)
{
  GimpPaintToolClass *paint_tool_class;

  paint_tool_class = (GimpPaintToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PAINT_TOOL);

  paint_tool_class->paint = gimp_paintbrush_tool_paint;
}

void
gimp_paintbrush_tool_register (void)
{
  tool_manager_register_tool (GIMP_TYPE_PAINTBRUSH_TOOL,
			      TRUE,
  			      "gimp:paintbrush_tool",
  			      _("Paintbrush"),
  			      _("Paint fuzzy brush strokes"),
      			      N_("/Tools/Paint Tools/Paintbrush"), "P",
  			      NULL, "tools/paintbrush.html",
			      (const gchar **) paint_bits);
}


static void
gimp_paintbrush_tool_motion (GimpPaintTool        *paint_tool,
			     GimpDrawable         *drawable,
			     PaintPressureOptions *pressure_options,
			     PaintGradientOptions *gradient_options,
			     double                fade_out,
			     double                gradient_length,
			     PaintApplicationMode  incremental,
			     GradientPaintMode     gradient_type)
{
  GimpImage  *gimage;
  TempBuf    *area;
  gdouble     x, paint_left;
  gdouble     position;
  guchar      local_blend = OPAQUE_OPACITY;
  guchar      temp_blend = OPAQUE_OPACITY;
  guchar      col[MAX_CHANNELS];
  GimpRGB     color;
  gint        mode;
  gint        opacity;
  gdouble     scale;
  PaintApplicationMode paint_appl_mode = incremental ? INCREMENTAL : CONSTANT;

  position = 0.0;
  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  if (pressure_options->size)
    scale = paint_tool->curpressure;
  else
    scale = 1.0;

  if (pressure_options->color)
    gradient_length = 1.0; /* not really used, only for if cases */

  /*  Get a region which can be used to paint to  */
  if (! (area = gimp_paint_tool_get_paint_area (paint_tool, drawable, scale)))
    return;

  /*  factor in the fade out value  */
  if (fade_out)
    {
      /*  Model the amount of paint left as a gaussian curve  */
      x = ((double) paint_tool->pixel_dist / fade_out);
      paint_left = exp (- x * x * 5.541);    /*  ln (1/255)  */
      local_blend = (int) (255 * paint_left);
    }

  if (local_blend)
    {
      /*  set the alpha channel  */
      temp_blend = local_blend;
      mode = gradient_type;

      if (gradient_length)
	{
	  if (pressure_options->color)
	    gimp_gradient_get_color_at (gimp_context_get_gradient (NULL),
					paint_tool->curpressure, &color);
	  else
	    gimp_paint_tool_get_color_from_gradient (paint_tool, gradient_length,
						     &color, mode);

	  temp_blend =  (gint) ((color.a * local_blend));

	  gimp_rgb_get_uchar (&color,
			      &col[RED_PIX],
			      &col[GREEN_PIX],
			      &col[BLUE_PIX]);
	  col[ALPHA_PIX] = OPAQUE_OPACITY;

	  /* always use incremental mode with gradients */
	  /* make the gui cool later */
	  paint_appl_mode = INCREMENTAL;
	  color_pixels (temp_buf_data (area), col,
			area->width * area->height, area->bytes);
	}
      /* we check to see if this is a pixmap, if so composite the
	 pixmap image into the are instead of the color */
      else if (paint_tool->brush && paint_tool->brush->pixmap)
	{
	  gimp_paint_tool_color_area_with_pixmap (paint_tool, gimage, drawable,
						  area,
						  scale, SOFT);
	  paint_appl_mode = INCREMENTAL;
	}
      else
	{
	  gimp_image_get_foreground (gimage, drawable, col);
	  col[area->bytes - 1] = OPAQUE_OPACITY;
	  color_pixels (temp_buf_data (area), col,
			area->width * area->height, area->bytes);
	}

      opacity = (gdouble)temp_blend;
      if (pressure_options->opacity)
	opacity = opacity * 2.0 * paint_tool->curpressure;

      gimp_paint_tool_paste_canvas (paint_tool, drawable,
				    MIN (opacity, 255),
				    gimp_context_get_opacity (NULL) * 255,
				    gimp_context_get_paint_mode (NULL),
				    pressure_options->pressure ? PRESSURE : SOFT,
				    scale, paint_appl_mode);
    }
}


static GimpPaintbrushTool *non_gui_paintbrush = NULL;

gboolean
gimp_paintbrush_tool_non_gui_default (GimpDrawable *drawable,
				      gint          num_strokes,
				      double       *stroke_array)
{
  GimpPaintTool *paint_tool;
  gint           i;

  if (! non_gui_paintbrush)
    {
      non_gui_paintbrush = gtk_type_new (GIMP_TYPE_PAINTBRUSH_TOOL);
    }

  paint_tool = GIMP_PAINT_TOOL (non_gui_paintbrush);

  /* Hmmm... PDB paintbrush should have gradient type added to it!
   * thats why the code below is duplicated.
   */
  if (gimp_paint_tool_start (paint_tool, drawable,
			     stroke_array[0],
			     stroke_array[1]))
    {
      paint_tool->startx = paint_tool->lastx = stroke_array[0];
      paint_tool->starty = paint_tool->lasty = stroke_array[1];

      gimp_paint_tool_paint (paint_tool, drawable, MOTION_PAINT);

      for (i = 1; i < num_strokes; i++)
       {
         paint_tool->curx = stroke_array[i * 2 + 0];
         paint_tool->cury = stroke_array[i * 2 + 1];

         gimp_paint_tool_interpolate (paint_tool, drawable);

         paint_tool->lastx = paint_tool->curx;
         paint_tool->lasty = paint_tool->cury;
       }

      /* Finish the painting */
      gimp_paint_tool_finish (paint_tool, drawable);

      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

gboolean
gimp_paintbrush_tool_non_gui (GimpDrawable *drawable,
			      gint          num_strokes,
			      gdouble      *stroke_array,
			      gdouble       fade_out,
			      gint          method,
			      gdouble       gradient_length)
{
  GimpPaintTool *paint_tool;
  gint           i;

  if (! non_gui_paintbrush)
    {
      non_gui_paintbrush = gtk_type_new (GIMP_TYPE_PAINTBRUSH_TOOL);
    }

  paint_tool = GIMP_PAINT_TOOL (non_gui_paintbrush);

  /* Code duplicated above */
  if (gimp_paint_tool_start (paint_tool, drawable,
			     stroke_array[0],
			     stroke_array[1]))
    {
      non_gui_gradient_options.fade_out        = fade_out;
      non_gui_gradient_options.gradient_length = gradient_length;
      non_gui_gradient_options.gradient_type   = LOOP_TRIANGLE;
      non_gui_incremental                      = method;

      paint_tool->startx = paint_tool->lastx = stroke_array[0];
      paint_tool->starty = paint_tool->lasty = stroke_array[1];

      if (num_strokes == 1)
	gimp_paint_tool_paint (paint_tool, drawable, MOTION_PAINT);

      for (i = 1; i < num_strokes; i++)
       {
         paint_tool->curx = stroke_array[i * 2 + 0];
         paint_tool->cury = stroke_array[i * 2 + 1];

         gimp_paint_tool_interpolate (paint_tool, drawable);

	 paint_tool->lastx = paint_tool->curx;
         paint_tool->lasty = paint_tool->cury;
       }

      /* Finish the painting */
      gimp_paint_tool_finish (paint_tool, drawable);

      return TRUE;
    }
  else
    {
      return FALSE;
    }
}
