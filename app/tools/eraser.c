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
#include "appenv.h"
#include "brushes.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "palette.h"
#include "eraser.h"
#include "selection.h"
#include "tools.h"

/*  forward function declarations  */
static void        eraser_motion   (PaintCore *, GimpDrawable *);
static Argument *  eraser_invoker  (Argument *);

static void *  eraser_options = NULL;

void *
eraser_paint_func (paint_core, drawable, state)
     PaintCore *paint_core;
     GimpDrawable *drawable;
     int state;
{
  switch (state)
    {
    case INIT_PAINT :
      break;

    case MOTION_PAINT :
      eraser_motion (paint_core, drawable);
      break;

    case FINISH_PAINT :
      break;

    default :
      break;
    }

  return NULL;
}


Tool *
tools_new_eraser ()
{
  Tool * tool;
  PaintCore * private;

  if (! eraser_options)
    eraser_options = tools_register_no_options (ERASER, "Eraser Options");

  tool = paint_core_new (ERASER);

  private = (PaintCore *) tool->private;
  private->paint_func = eraser_paint_func;

  return tool;
}


void
tools_free_eraser (tool)
     Tool * tool;
{
  paint_core_free (tool);
}


void
eraser_motion (paint_core, drawable)
     PaintCore *paint_core;
     GimpDrawable *drawable;
{
  GImage *gimage;
  TempBuf * area;
  unsigned char col[MAX_CHANNELS];

  if (! (gimage = drawable_gimage (drawable)))
    return;

  gimage_get_background (gimage, drawable, col);

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
			   (int) (get_brush_opacity () * 255),
			   ERASE_MODE, SOFT, CONSTANT);
}


static void *
eraser_non_gui_paint_func (PaintCore *paint_core, GimpDrawable *drawable, int state)
{
  eraser_motion (paint_core, drawable);

  return NULL;
}


/*  The eraser procedure definition  */
ProcArg eraser_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
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


ProcRecord eraser_proc =
{
  "gimp_eraser",
  "Erase using the current brush",
  "This tool erases using the current brush mask.  If the specified drawable contains an alpha channel, then the erased pixels will become transparent.  Otherwise, the eraser tool replaces the contents of the drawable with the background color.  Like paintbrush, this tool linearly interpolates between the specified stroke coordinates.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  4,
  eraser_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { eraser_invoker } },
};


static Argument *
eraser_invoker (args)
     Argument *args;
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

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  num strokes  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value > 0)
	num_strokes = int_value / 2;
      else
	success = FALSE;
    }

      /*  point array  */
  if (success)
    stroke_array = (double *) args[3].value.pdb_pointer;

  if (success)
    /*  init the paint core  */
    success = paint_core_init (&non_gui_paint_core, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = eraser_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	eraser_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

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

  return procedural_db_return_args (&eraser_proc, success);
}
