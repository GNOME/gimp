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
#include "gimpbrushpixmap.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "paint_options.h"
#include "paintbrush.h"
#include "palette.h"
#include "pencil.h"
/* for color_area_with_pixmap */
#include "pixmapbrush.h"
#include "selection.h"
#include "tools.h"

#include "libgimp/gimpintl.h"


/*  the pencil tool options  */

static PaintOptions *pencil_options = NULL; 

/*  forward function declarations  */
static void      pencil_motion (PaintCore *, GimpDrawable *, gboolean);

static gboolean  non_gui_incremental = FALSE;

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
      pencil_motion (paint_core, drawable, pencil_options->incremental);
      break;

    case FINISH_PAINT :
      break;

    default :
      break;
    }

  return NULL;
}

void
pencil_options_reset (void)
{
  paint_options_reset (pencil_options);
}

Tool *
tools_new_pencil (void)
{
  Tool * tool;
  PaintCore * private;

  /*  The tool options  */
  if (!pencil_options)
    {
      pencil_options = paint_options_new (PENCIL, pencil_options_reset);
      tools_register (PENCIL, (ToolOptions *) pencil_options);
    }

  tool = paint_core_new (PENCIL);

  private = (PaintCore *) tool->private;
  private->paint_func = pencil_paint_func;
  private->pick_colors = TRUE;

  return tool;
}

void
tools_free_pencil (Tool *tool)
{
  paint_core_free (tool);
}

static void
pencil_motion (PaintCore    *paint_core,
	       GimpDrawable *drawable,
	       gboolean	     increment)
{
  GImage *gimage;
  TempBuf * area;
  unsigned char col[MAX_CHANNELS];
  gint opacity;
  PaintApplicationMode paint_appl_mode = increment ? INCREMENTAL : CONSTANT;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  gimage_get_foreground (gimage, drawable, col);

  /*  Get a region which can be used to paint to  */
  if (! (area = paint_core_get_paint_area (paint_core, drawable)))
    return;

  /*  set the alpha channel  */
  col[area->bytes - 1] = OPAQUE_OPACITY;


  if (GIMP_IS_BRUSH_PIXMAP (paint_core->brush))
    {
      /* if its a pixmap, do pixmap stuff */
      color_area_with_pixmap (gimage, drawable, area, paint_core->brush);
      paint_appl_mode = INCREMENTAL;
    }
  else
    {
      /*  color the pixels  */
      color_pixels (temp_buf_data (area), col,
		    area->width * area->height, area->bytes);
    }

  /*Make the opacity dependent on the current pressure 
    This makes a more natural pencil since light pressure
    on a graphite pen will give transparent line */

  opacity = 255 * gimp_context_get_opacity (NULL) * (paint_core->curpressure / 0.5);
  if (opacity > 255)
    opacity = 255; 



  /*  paste the newly painted canvas to the gimage which is being worked on  */
  paint_core_paste_canvas (paint_core, drawable, opacity,
			   (int) (gimp_context_get_opacity (NULL) * 255),
			   gimp_context_get_paint_mode (NULL),
			   HARD, paint_appl_mode);
}

static void *
pencil_non_gui_paint_func (PaintCore    *paint_core,
			   GimpDrawable *drawable,
			   int           state)
{
  pencil_motion (paint_core, drawable, non_gui_incremental);

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
