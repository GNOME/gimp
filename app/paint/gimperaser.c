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
#include "cursorutil.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimpui.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "paint_options.h"
#include "eraser.h"
#include "selection.h"
#include "tools.h"

#include "libgimp/gimpintl.h"


/* Defaults */
#define ERASER_DEFAULT_HARD        FALSE
#define ERASER_DEFAULT_INCREMENTAL FALSE
#define ERASER_DEFAULT_ANTI_ERASE  FALSE

/*  the eraser structures  */

typedef struct _EraserOptions EraserOptions;
struct _EraserOptions
{
  PaintOptions  paint_options;

  gboolean      hard;
  gboolean      hard_d;
  GtkWidget    *hard_w;

  gboolean	anti_erase;
  gboolean	anti_erase_d;
  GtkWidget    *anti_erase_w;
};


/*  the eraser tool options  */
static EraserOptions *eraser_options = NULL;

/*  local variables  */
static gboolean       non_gui_hard;
static gboolean       non_gui_incremental;
static gboolean	      non_gui_anti_erase;

/*  forward function declarations  */
static void       eraser_motion            (PaintCore *, GimpDrawable *,
					    PaintPressureOptions *,
					    gboolean, gboolean, gboolean);


/*  functions  */

static void
eraser_options_reset (void)
{
  EraserOptions *options = eraser_options;

  paint_options_reset ((PaintOptions *) options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->hard_w),
				options->hard_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->anti_erase_w),
				options->anti_erase_d);
}
  
static EraserOptions *
eraser_options_new (void)
{
  EraserOptions *options;

  GtkWidget *vbox;

  /*  the new eraser tool options structure  */
  options = g_new (EraserOptions, 1);
  paint_options_init ((PaintOptions *) options,
		      ERASER,
		      eraser_options_reset);
  options->hard        = options->hard_d        = ERASER_DEFAULT_HARD;
  options->anti_erase  = options->anti_erase_d  = ERASER_DEFAULT_ANTI_ERASE;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /* the hard toggle */
  options->hard_w = gtk_check_button_new_with_label (_("Hard Edge"));
  gtk_box_pack_start (GTK_BOX (vbox), options->hard_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->hard_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->hard);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->hard_w),
				options->hard_d);
  gtk_widget_show (options->hard_w);

  /* the anti_erase toggle */
  options->anti_erase_w = gtk_check_button_new_with_label (_("Anti Erase"));
  gtk_box_pack_start (GTK_BOX (vbox), options->anti_erase_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->anti_erase_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->anti_erase);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->anti_erase_w),
				options->anti_erase_d);
  gtk_widget_show (options->anti_erase_w);

  return options;
}

static void
eraser_modifier_key_func (Tool        *tool,
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
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (eraser_options->anti_erase_w), ! eraser_options->anti_erase);
      break;
    case GDK_Control_L: 
    case GDK_Control_R:
      if ( !(kevent->state & GDK_SHIFT_MASK) ) /* shift enables line draw mode */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (eraser_options->anti_erase_w), ! eraser_options->anti_erase);
      break;
    }
}


void *
eraser_paint_func (PaintCore    *paint_core,
		   GimpDrawable *drawable,
		   int           state)
{
  switch (state)
    {
    case INIT_PAINT:
      break;

    case MOTION_PAINT:
      eraser_motion (paint_core,
		     drawable,
		     eraser_options->paint_options.pressure_options,
		     eraser_options->hard,
		     eraser_options->paint_options.incremental,
		     eraser_options->anti_erase);
      break;

    case FINISH_PAINT:
      break;

    default:
      break;
    }

  return NULL;
}

Tool *
tools_new_eraser (void)
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
  private->flags |= TOOL_CAN_HANDLE_CHANGING_BRUSH; 
  tool->modifier_key_func = eraser_modifier_key_func;

  return tool;
}


void
tools_free_eraser (Tool *tool)
{
  paint_core_free (tool);
}


static void
eraser_motion (PaintCore            *paint_core,
	       GimpDrawable         *drawable,
	       PaintPressureOptions *pressure_options,
	       gboolean              hard,
	       gboolean              incremental,
	       gboolean	             anti_erase)
{
  GImage *gimage;
  gint opacity;
  TempBuf * area;
  unsigned char col[MAX_CHANNELS];
  gdouble scale;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  gimage_get_background (gimage, drawable, col);

  if (pressure_options->size)
    scale = paint_core->curpressure;
  else
    scale = 1.0;

  /*  Get a region which can be used to paint to  */
  if (! (area = paint_core_get_paint_area (paint_core, drawable, scale)))
    return;

  /*  set the alpha channel  */
  col[area->bytes - 1] = OPAQUE_OPACITY;

  /*  color the pixels  */
  color_pixels (temp_buf_data (area), col,
		area->width * area->height, area->bytes);

  opacity = 255 * gimp_context_get_opacity (NULL);
  if (pressure_options->opacity)
    opacity = opacity * 2.0 * paint_core->curpressure;

  /*  paste the newly painted canvas to the gimage which is being worked on  */
  paint_core_paste_canvas (paint_core, drawable, 
			   MIN (opacity, 255),
			   (int) (gimp_context_get_opacity (NULL) * 255),
			   anti_erase ? ANTI_ERASE_MODE : ERASE_MODE,
			   hard ? HARD : (pressure_options->pressure ? PRESSURE : SOFT),
			   scale,
			   incremental ? INCREMENTAL : CONSTANT);
}


static void *
eraser_non_gui_paint_func (PaintCore    *paint_core,
			   GimpDrawable *drawable,
			   int           state)
{
  eraser_motion (paint_core, drawable,
		 &non_gui_pressure_options,
		 non_gui_hard, non_gui_incremental, non_gui_anti_erase);

  return NULL;
}

gboolean
eraser_non_gui_default (GimpDrawable *drawable,
			int           num_strokes,
			double       *stroke_array)
{
  gboolean  hardness   = ERASER_DEFAULT_HARD;
  gboolean  method     = ERASER_DEFAULT_INCREMENTAL;
  gboolean  anti_erase = ERASER_DEFAULT_ANTI_ERASE;

  EraserOptions *options = eraser_options;

  if (options)
    {
      hardness   = options->hard;
      method     = options->paint_options.incremental;
      anti_erase = options->anti_erase;
    }

  return eraser_non_gui (drawable, num_strokes, stroke_array,
			 hardness, method, anti_erase);
}

gboolean
eraser_non_gui (GimpDrawable *drawable,
    		int           num_strokes,
		double       *stroke_array,
		int           hardness,
		int           method,
		int	      anti_erase)
{
  int i;
  
  if (paint_core_init (&non_gui_paint_core, drawable,
		       stroke_array[0], stroke_array[1]))
    {
      non_gui_hard        = hardness;
      non_gui_incremental = method;
      non_gui_anti_erase  = anti_erase;

      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = eraser_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

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
