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

#include "gimppenciltool.h"
#include "gimppainttool.h"
#include "paint_options.h"
#include "tool_options.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"

#include "pixmaps2.h"


#define PENCIL_INCREMENTAL_DEFAULT FALSE


static void   gimp_pencil_tool_class_init (GimpPencilToolClass *klass);
static void   gimp_pencil_tool_init       (GimpPencilTool      *pancil);

static void   gimp_pencil_tool_paint      (GimpPaintTool        *paint_tool,
					   GimpDrawable         *drawable,
					   PaintState            state);
static void   gimp_pencil_tool_motion     (GimpPaintTool        *paint_tool,
					   GimpDrawable         *drawable,
					   PaintPressureOptions *pressure_options,
					   gboolean              incremental);


/*  private variables  */
static gboolean  non_gui_incremental = PENCIL_INCREMENTAL_DEFAULT;

static PaintOptions *pencil_options = NULL; 

static GimpPaintToolClass *parent_class = NULL;


/*  functions  */

void
gimp_pencil_tool_register (void)
{
  tool_manager_register_tool (GIMP_TYPE_PENCIL_TOOL,
                              TRUE,
                              "gimp:pencil_tool",
                              _("Pencil"),
                              _("Paint hard edged pixels"),
                              N_("/Tools/Paint Tools/Pencil"), "P",
                              NULL, "tools/pencil.html",
                              (const gchar **) pencil_bits);
}

GtkType
gimp_pencil_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpPencilTool",
        sizeof (GimpPencilTool),
        sizeof (GimpPencilToolClass),
        (GtkClassInitFunc) gimp_pencil_tool_class_init,
        (GtkObjectInitFunc) gimp_pencil_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        NULL
      };

      tool_type = gtk_type_unique (GIMP_TYPE_PAINT_TOOL, &tool_info);
    }

  return tool_type;
}

static void 
gimp_pencil_tool_class_init (GimpPencilToolClass *klass)
{  
  GimpPaintToolClass *paint_tool_class;

  paint_tool_class = (GimpPaintToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PAINT_TOOL);

  paint_tool_class->paint = gimp_pencil_tool_paint;
}

static void
gimp_pencil_tool_init (GimpPencilTool *pencil)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (pencil);
  paint_tool = GIMP_PAINT_TOOL (pencil);

  if (! pencil_options)
    {
      pencil_options = paint_options_new (GIMP_TYPE_PENCIL_TOOL,
					  paint_options_reset);

      tool_manager_register_tool_options (GIMP_TYPE_PENCIL_TOOL,
                                          (ToolOptions *) pencil_options);
     }

   tool->tool_cursor = GIMP_PENCIL_TOOL_CURSOR;

   paint_tool->pick_colors  = TRUE;
   paint_tool->flags       |= TOOL_CAN_HANDLE_CHANGING_BRUSH;
}
             
static void
gimp_pencil_tool_paint (GimpPaintTool *paint_tool,
                        GimpDrawable  *drawable,
                        PaintState     state)
{
  PaintPressureOptions *pressure_options;
  gboolean              incremental;

  if (pencil_options)
    {
      pressure_options = pencil_options->pressure_options;
      incremental      = pencil_options->incremental;
    }
  else
    {
      pressure_options = &non_gui_pressure_options;
      incremental      = non_gui_incremental;
    }

  switch (state)
    {
    case INIT_PAINT:
      break;

    case MOTION_PAINT:
      gimp_pencil_tool_motion (paint_tool, drawable, 
                               pressure_options, incremental);
      break;

    case FINISH_PAINT:
      break;

    default:
      break;
    }
}

static void
gimp_pencil_tool_motion (GimpPaintTool        *paint_tool,
                         GimpDrawable         *drawable,
                         PaintPressureOptions *pressure_options,
                         gboolean	       incremental)
{
  GimpImage            *gimage;
  TempBuf              *area;
  guchar                col[MAX_CHANNELS];
  gint                  opacity;
  gdouble               scale;
  PaintApplicationMode  paint_appl_mode = incremental ? INCREMENTAL : CONSTANT;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  if (pressure_options->size)
    scale = paint_tool->curpressure;
  else
    scale = 1.0;

  /*  Get a region which can be used to paint to  */
  if (! (area = gimp_paint_tool_get_paint_area (paint_tool, drawable, scale)))
    return;

  /*  color the pixels  */
  if (pressure_options->color)
    {
      GimpRGB color;

      gimp_gradient_get_color_at (gimp_context_get_gradient (NULL),
				  paint_tool->curpressure, &color);

      gimp_rgba_get_uchar (&color,
			   &col[RED_PIX],
			   &col[GREEN_PIX],
			   &col[BLUE_PIX],
			   &col[ALPHA_PIX]);

      paint_appl_mode = INCREMENTAL;
      color_pixels (temp_buf_data (area), col,
		    area->width * area->height, area->bytes);
    }
  else if (paint_tool->brush && paint_tool->brush->pixmap)
    {
      /* if its a pixmap, do pixmap stuff */      
      gimp_paint_tool_color_area_with_pixmap (paint_tool, gimage, drawable, 
										                                    area, scale, HARD);
      paint_appl_mode = INCREMENTAL;
    }
  else
    {
      gimp_image_get_foreground (gimage, drawable, col);
      col[area->bytes - 1] = OPAQUE_OPACITY;
      color_pixels (temp_buf_data (area), col,
		    area->width * area->height, area->bytes);
    }

  opacity = 255 * gimp_context_get_opacity (NULL);
  if (pressure_options->opacity)
    opacity = opacity * 2.0 * paint_tool->curpressure;

  /*  paste the newly painted canvas to the gimage which is being worked on  */
  gimp_paint_tool_paste_canvas (paint_tool, drawable, 
                                MIN (opacity, 255),
                                gimp_context_get_opacity (NULL) * 255,
                                gimp_context_get_paint_mode (NULL),
                                HARD, scale, paint_appl_mode);
}


/*  non-gui stuff  */

gboolean
pencil_non_gui (GimpDrawable *drawable,
		gint          num_strokes,
		gdouble      *stroke_array)
{
  static GimpPencilTool *non_gui_pencil = NULL;

  GimpPaintTool *paint_tool;
  gint           i;
  
  if (! non_gui_pencil)
    {
      non_gui_pencil = gtk_type_new (GIMP_TYPE_PENCIL_TOOL);
    }

  paint_tool = GIMP_PAINT_TOOL (non_gui_pencil);
		
  if (gimp_paint_tool_start (paint_tool, drawable,
                             stroke_array[0],
                             stroke_array[1]))
    {
      paint_tool->startx = paint_tool->lastx = stroke_array[0];
      paint_tool->starty = paint_tool->lasty = stroke_array[1];

      gimp_pencil_tool_paint (paint_tool, drawable, MOTION_PAINT);

      for (i = 1; i < num_strokes; i++)
	{
	  paint_tool->curx = stroke_array[i * 2 + 0];
	  paint_tool->cury = stroke_array[i * 2 + 1];

	  gimp_paint_tool_interpolate (paint_tool, drawable);

	  paint_tool->lastx = paint_tool->curx;
	  paint_tool->lasty = paint_tool->cury;
	}

      gimp_paint_tool_finish (paint_tool, drawable);

      return TRUE;
    }

  return FALSE;
}
