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

#include "appenv.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimpbrushpipe.h"
#include "gradient.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "paint_options.h"
#include "paintbrush.h"
#include "pencil.h"
#include "selection.h"
#include "tool_options_ui.h"
#include "tools.h"

#include "libgimp/gimpintl.h"

/*  the pencil tool options  */
typedef struct _PencilOptions PencilOptions;
struct _PencilOptions
{
  PaintOptions  paint_options;
};

static PencilOptions *pencil_options = NULL; 

/*  forward function declarations  */
static void      pencil_motion (PaintCore *, GimpDrawable *, 
				PaintPressureOptions *, gboolean);

static gboolean  non_gui_incremental = FALSE;
static void      pencil_options_reset (void);

#define PENCIL_INCREMENTAL_DEFAULT FALSE

/*  functions  */

void *
pencil_paint_func (PaintCore    *paint_core,
		   GimpDrawable *drawable,
		   int           state)
{
  switch (state)
    {
    case INIT_PAINT :
      break;

    case MOTION_PAINT :
      pencil_motion (paint_core, drawable, 
		     pencil_options->paint_options.pressure_options, 
		     pencil_options->paint_options.incremental);
      break;

    case FINISH_PAINT :
      break;

    default :
      break;
    }

  return NULL;
}

static PencilOptions *
pencil_options_new (void)
{
  PencilOptions *options;

  options = (PencilOptions *) g_malloc (sizeof (PencilOptions));  
  paint_options_init ((PaintOptions *) options,
		      PENCIL,
		      pencil_options_reset);
  return options;
}

static void
pencil_options_reset (void)
{
  paint_options_reset ((PaintOptions *) pencil_options);
}

Tool *
tools_new_pencil (void)
{
  Tool * tool;
  PaintCore * private;

  /*  The tool options  */
  if (!pencil_options)
    {
      pencil_options = pencil_options_new ();
      tools_register (PENCIL, (ToolOptions *) pencil_options);
      pencil_options_reset();
    }

  tool = paint_core_new (PENCIL);

  private = (PaintCore *) tool->private;
  private->paint_func = pencil_paint_func;
  private->pick_colors = TRUE;
  private->flags |= TOOL_CAN_HANDLE_CHANGING_BRUSH;

  return tool;
}

void
tools_free_pencil (Tool *tool)
{
  paint_core_free (tool);
}

static void
pencil_motion (PaintCore            *paint_core,
	       GimpDrawable         *drawable,
	       PaintPressureOptions *pressure_options,
	       gboolean	             increment)
{
  GImage *gimage;
  TempBuf *area;
  unsigned char col[MAX_CHANNELS];
  gint opacity;
  gdouble scale;
  PaintApplicationMode paint_appl_mode = increment ? INCREMENTAL : CONSTANT;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  if (pressure_options->size)
    scale = paint_core->curpressure;
  else
    scale = 1.0;

  /*  Get a region which can be used to paint to  */
  if (! (area = paint_core_get_paint_area (paint_core, drawable, scale)))
    return;

  /*  color the pixels  */
  if (pressure_options->color)
    {
      gdouble r, g, b, a;
      
      gradient_get_color_at (gimp_context_get_gradient (NULL),
			     paint_core->curpressure, &r, &g, &b, &a);
      col[0] = r * 255.0;
      col[1] = g * 255.0;
      col[2] = b * 255.0;
      col[3] = a * 255.0;
      paint_appl_mode = INCREMENTAL;
      color_pixels (temp_buf_data (area), col,
		    area->width * area->height, area->bytes);
    }
  else if (GIMP_IS_BRUSH_PIXMAP (paint_core->brush))
    {
      /* if its a pixmap, do pixmap stuff */      
      paint_core_color_area_with_pixmap (paint_core, gimage, drawable, area, 
					 scale, HARD);
      paint_appl_mode = INCREMENTAL;
    }
  else
    {
      gimage_get_foreground (gimage, drawable, col);
      col[area->bytes - 1] = OPAQUE_OPACITY;
      color_pixels (temp_buf_data (area), col,
		    area->width * area->height, area->bytes);
    }

  opacity = 255 * gimp_context_get_opacity (NULL);
  if (pressure_options->opacity)
    opacity = opacity * 2.0 * paint_core->curpressure;

  /*  paste the newly painted canvas to the gimage which is being worked on  */
  paint_core_paste_canvas (paint_core, drawable, 
			   MIN (opacity, 255),
			   (int) (gimp_context_get_opacity (NULL) * 255),
			   gimp_context_get_paint_mode (NULL),
			   HARD, scale, paint_appl_mode);
}

static void *
pencil_non_gui_paint_func (PaintCore    *paint_core,
			   GimpDrawable *drawable,
			   int           state)
{
  pencil_motion (paint_core, drawable, &non_gui_pressure_options, 
		 non_gui_incremental);

  return NULL;
}


gboolean
pencil_non_gui (GimpDrawable *drawable,
		int           num_strokes,
		double       *stroke_array)
{
  int i;

  if (paint_core_init (&non_gui_paint_core, drawable,
		       stroke_array[0], stroke_array[1]))
    {
      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = pencil_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      pencil_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

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
