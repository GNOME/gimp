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
#include "canvas.h"
#include "drawable.h"
#include "errors.h"
#include "gimage.h"
#include "paint_core_16.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pencil.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "tools.h"

/*  forward function declarations  */
static void         pencil_motion       (PaintCore16 *, GimpDrawable *);
static Argument *   pencil_invoker  (Argument *);

static void *  pencil_options = NULL;

void * 
pencil_paint_func  (
                    PaintCore16 * paint_core,
                    GimpDrawable * drawable,
                    int state
                    )
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
tools_new_pencil  (
                   void
                   )
{
  Tool * tool;
  PaintCore16 * private;

  if (!pencil_options)
    pencil_options = tools_register_no_options (PENCIL, "Pencil Options");

  tool = paint_core_16_new (PENCIL);

  private = (PaintCore16 *) tool->private;
  private->paint_func = pencil_paint_func;

  return tool;
}


void 
tools_free_pencil  (
                    Tool * tool
                    )
{
  paint_core_16_free (tool);
}


void 
pencil_motion  (
                PaintCore16 * paint_core,
                GimpDrawable * drawable
                )
{
  Canvas * painthit;
  PixelArea a;
  
  /* Get the working canvas */
  painthit = paint_core_16_area (paint_core, drawable);
  pixelarea_init (&a, painthit, NULL, 0, 0, 0, 0, TRUE);
  
  /* construct the paint hit */
  {
    COLOR16_NEW (paint, canvas_tag (painthit));

    COLOR16_INIT (paint);
    color16_foreground (&paint);
    color_area (&a, &paint);
  }
  
  /* apply it to the image */
  paint_core_16_area_paste (paint_core, drawable,
                            (gfloat) 1.0,
                            (gfloat) get_brush_opacity (),
                            HARD,
                            CONSTANT,
                            get_brush_paint_mode ());
}


static void * 
pencil_non_gui_paint_func  (
                            PaintCore16 * paint_core,
                            GimpDrawable * drawable,
                            int state
                            )
{
  pencil_motion (paint_core, drawable);

  return NULL;
}


/*  The pencil procedure definition  */
ProcArg pencil_args[] =
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
  4,
  pencil_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { pencil_invoker } },
};

static Argument *
pencil_invoker (args)
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
    success = paint_core_16_init (&non_gui_paint_core_16, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      /*  set the paint core's paint func  */
      non_gui_paint_core_16.paint_func = pencil_non_gui_paint_func;

      non_gui_paint_core_16.startx = non_gui_paint_core_16.lastx = stroke_array[0];
      non_gui_paint_core_16.starty = non_gui_paint_core_16.lasty = stroke_array[1];

      if (num_strokes == 1)
	pencil_non_gui_paint_func (&non_gui_paint_core_16, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core_16.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core_16.cury = stroke_array[i * 2 + 1];

	  paint_core_16_interpolate (&non_gui_paint_core_16, drawable);

	  non_gui_paint_core_16.lastx = non_gui_paint_core_16.curx;
	  non_gui_paint_core_16.lasty = non_gui_paint_core_16.cury;
	}

      /*  finish the painting  */
      paint_core_16_finish (&non_gui_paint_core_16, drawable, -1);

      /*  cleanup  */
      paint_core_16_cleanup ();
    }

  return procedural_db_return_args (&pencil_proc, success);
}
