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
#include "eraser.h"
#include "selection.h"
#include "tool_options_ui.h"
#include "tools.h"

#include "libgimp/gimpintl.h"

/*  the eraser structures  */

typedef struct _EraserOptions EraserOptions;
struct _EraserOptions
{
  ToolOptions  tool_options;

  gboolean     hard;
  gboolean     hard_d;
  GtkWidget   *hard_w;

  gboolean     incremental;
  gboolean     incremental_d;
  GtkWidget   *incremental_w;
};


/*  the eraser tool options  */
static EraserOptions *eraser_options = NULL;

/*  local variables  */
static gboolean       non_gui_hard;
static gboolean       non_gui_incremental;


/*  forward function declarations  */
static void       eraser_motion            (PaintCore *, GimpDrawable *,
					    gboolean, gboolean);


/*  functions  */

static void
eraser_options_reset (void)
{
  EraserOptions *options = eraser_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->hard_w),
				options->hard_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->incremental_w),
				options->incremental_d);
}
  
static EraserOptions *
eraser_options_new (void)
{
  EraserOptions *options;

  GtkWidget *vbox;

  /*  the new eraser tool options structure  */
  options = (EraserOptions *) g_malloc (sizeof (EraserOptions));
  tool_options_init ((ToolOptions *) options,
		     _("Eraser Options"),
		     eraser_options_reset);
  options->hard        = options->hard_d        = FALSE;
  options->incremental = options->incremental_d = FALSE;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /* the hard toggle */
  options->hard_w = gtk_check_button_new_with_label (_("Hard edge"));
  gtk_box_pack_start (GTK_BOX (vbox), options->hard_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->hard_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
		      &options->hard);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->hard_w),
				options->hard_d);
  gtk_widget_show (options->hard_w);
  
  /* the incremental toggle */
  options->incremental_w = gtk_check_button_new_with_label (_("Incremental"));
  gtk_box_pack_start (GTK_BOX (vbox), options->incremental_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->incremental_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
		      &options->incremental);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->incremental_w),
				options->incremental_d);
  gtk_widget_show (options->incremental_w);
  
  return options;
}
  
void *
eraser_paint_func (PaintCore    *paint_core,
		   GimpDrawable *drawable,
		   int           state)
{
  switch (state)
    {
    case INIT_PAINT :
      break;

    case MOTION_PAINT :
      eraser_motion (paint_core, drawable, eraser_options->hard, eraser_options->incremental);
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

  /*  The tool options  */
  if (! eraser_options)
    {
      eraser_options = eraser_options_new ();
      tools_register (ERASER, (ToolOptions *) eraser_options);
    }

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


static void
eraser_motion (PaintCore    *paint_core,
	       GimpDrawable *drawable,
	       gboolean      hard,
	       gboolean      incremental)
{
  GImage *gimage;
  gint opacity;
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
  opacity = 255 * gimp_brush_get_opacity() * (paint_core->curpressure / 0.5);
  if(opacity > OPAQUE_OPACITY) opacity=OPAQUE_OPACITY;
  /*  paste the newly painted canvas to the gimage which is being worked on  */
  paint_core_paste_canvas (paint_core, drawable, opacity,
			   (int) (gimp_brush_get_opacity () * 255),
			   ERASE_MODE, hard? HARD : SOFT, incremental ? INCREMENTAL : CONSTANT);
}


static void *
eraser_non_gui_paint_func (PaintCore    *paint_core,
			   GimpDrawable *drawable,
			   int           state)
{
  eraser_motion (paint_core, drawable, non_gui_hard, non_gui_incremental);

  return NULL;
}

gboolean
eraser_non_gui (GimpDrawable *drawable,
    		int           num_strokes,
		double       *stroke_array,
		int           hardness,
		int           method)
{
  int i;
  
  if (paint_core_init (&non_gui_paint_core, drawable,
		       stroke_array[0], stroke_array[1]))
    {
      non_gui_hard = hardness;
      non_gui_incremental = method;

      /* Set the paint core's paint func */
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

      /* Finish the painting */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /* Cleanup */
      paint_core_cleanup ();
      return TRUE;
    }
  else
    return FALSE;
}
