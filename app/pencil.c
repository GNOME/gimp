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
#include "gimpbrushlist.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "palette.h"
#include "paintbrush.h"
#include "pencil.h"
#include "selection.h"
#include "tools.h"

#include "libgimp/gimpintl.h"


/*  the pencil tool options  */
static ToolOptions *  pencil_options = NULL;


/*  forward function declarations  */
static void         pencil_motion   (PaintCore *, GimpDrawable *);
static Argument *   pencil_invoker  (Argument *);


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
      pencil_motion (paint_core, drawable);
      break;

    case FINISH_PAINT :
      break;

    default :
      break;
    }

  return NULL;
}

Tool *
tools_new_pencil ()
{
  Tool * tool;
  PaintCore * private;

  /*  The tool options  */
  if (!pencil_options)
    {
      pencil_options = tool_options_new (_("Pencil Options"));
      tools_register (PENCIL, pencil_options);
    }

  tool = paint_core_new (PENCIL);

  private = (PaintCore *) tool->private;
  private->paint_func = pencil_paint_func;

  return tool;
}

void
tools_free_pencil (Tool *tool)
{
  paint_core_free (tool);
}

static void
pencil_motion (PaintCore    *paint_core,
	       GimpDrawable *drawable)
{
  GImage *gimage;
  TempBuf * area;
  unsigned char col[MAX_CHANNELS];

  if (! (gimage = drawable_gimage (drawable)))
    return;

  gimage_get_foreground (gimage, drawable, col);

  /*  Get a region which can be used to paint to  */
  if (! (area = paint_core_get_paint_area (paint_core, drawable)))
    return;

  /*  set the alpha channel  */
  col[area->bytes - 1] = OPAQUE_OPACITY;

  /*  color the pixels  */
  color_pixels (temp_buf_data (area), col,
		area->width * area->height, area->bytes);

  /*  paste the newly painted canvas to the gimage which is being worked on  */
  paint_core_paste_canvas (paint_core, drawable, OPAQUE_OPACITY,
			   (int) (gimp_brush_get_opacity () * 255),
			   gimp_brush_get_paint_mode (), HARD, CONSTANT);
}

static void *
pencil_non_gui_paint_func (PaintCore    *paint_core,
			   GimpDrawable *drawable,
			   int           state)
{
  pencil_motion (paint_core, drawable);

  return NULL;
}


/*  The pencil procedure definition  */
ProcArg pencil_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "num_strokes",
    "number of stroke control points (count each coordinate as 2 points)"
  },
  { PDB_FLOATARRAY,
    "strokes",
    "array of stroke coordinates: {s1.x, s1.y, s2.x, s2.y, ..., sn.x, sn.y}"
  }
};


ProcRecord pencil_proc =
{
  "gimp_pencil",
  "Paint in the current brush without sub-pixel sampling",
  "This tool is the standard pencil.  It draws linearly interpolated lines through the specified stroke coordinates.  It operates on the specified drawable in the foreground color with the active brush.  The brush mask is treated as though it contains only black and white values.  Any value below half is treated as black; any above half, as white.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  pencil_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { pencil_invoker } },
};

static Argument *
pencil_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  int num_strokes;
  double *stroke_array;
  int int_value;
  int i;

  drawable = NULL;
  num_strokes = 0;

  /*  the drawable  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL)                                        
        success = FALSE;
      else
        gimage = drawable_gimage (drawable);
    }
  /*  num strokes  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (int_value > 0)
	num_strokes = int_value / 2;
      else
	success = FALSE;
    }

  /*  point array  */
  if (success)
    stroke_array = (double *) args[2].value.pdb_pointer;

  if (success)
    /*  init the paint core  */
    success = paint_core_init (&non_gui_paint_core, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = pencil_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	pencil_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cury = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;
	}

      /*  finish the painting  */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /*  cleanup  */
      paint_core_cleanup ();
    }

  return procedural_db_return_args (&pencil_proc, success);
}
